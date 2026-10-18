//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoUARTEndpoint.h"

#include "main.h"
#include "common/Pipe.h"
#include "common/Scheduler.h"

#include <hardware/irq.h>
#include <hardware/uart.h>
#include <pico/stdlib.h>
#include <pico/time.h>

#include <stdio.h>

using namespace nd;

// The UART interrupt moves received bytes from the 32 byte hardware FIFO into
// this ring buffer, so a slow scheduler cycle doesn't lose data. Only the
// interrupt writes `rx_head`, and only task() writes `rx_tail`.
static constexpr uint32_t kRxBufferSize = 1024; // must be a power of 2
static volatile uint8_t rx_buffer[kRxBufferSize];
static volatile uint32_t rx_head = 0;
static volatile uint32_t rx_tail = 0;
static volatile UARTEndpoint::RxErrors rx_error_count;

static void __not_in_flash_func(on_uart_rx)()
{
    while (uart_is_readable(kUART)) {
        // The data register holds the error flags of each byte above bit 7.
        uint32_t dr = uart_get_hw(kUART)->dr;
        if (dr & UART_UARTDR_OE_BITS) rx_error_count.overrun++;
        if (dr & UART_UARTDR_FE_BITS) rx_error_count.framing++;
        if (dr & UART_UARTDR_BE_BITS) rx_error_count.brk++;
        uint32_t next = (rx_head + 1) & (kRxBufferSize - 1);
        if (next == rx_tail) {
            rx_error_count.lost++;
        } else {
            rx_buffer[rx_head] = (uint8_t)dr;
            rx_head = next;
        }
    }
}

PicoUARTEndpoint::PicoUARTEndpoint(Scheduler &scheduler)
:   UARTEndpoint(scheduler)
{
}

PicoUARTEndpoint::~PicoUARTEndpoint() {
}

Result PicoUARTEndpoint::init() 
{
    // -- Set TX and RX pin mode
    gpio_set_function(kUART_TX_Pin, UART_FUNCSEL_NUM(kUART, kUART_TX_Pin));
    gpio_set_function(kUART_RX_Pin, UART_FUNCSEL_NUM(kUART, kUART_RX_Pin));

    // -- Initialize UART
    uart_init(kUART, kUART_BaudRate);
    uart_set_format(kUART, 8, 1, UART_PARITY_NONE);
    uart_set_translate_crlf(kUART, false);
    uart_set_fifo_enabled(kUART, true);

    // -- Take care of hardware flow control
    uart_set_hw_flow(kUART, false, false);
    // Handshake Out (from MP to the Dongle)
    gpio_init(kUART_HSKO_Pin);               // output from MP to dongle
    gpio_pull_up(kUART_HSKO_Pin);            // let the MP do its work
    gpio_set_dir(kUART_HSKO_Pin, GPIO_IN);   // we want to read the pin
    // Handshake In (from the Dongle to the MP)
    gpio_init(kUART_HSKI_Pin);               // output from MP to dongle
    gpio_put(kUART_HSKI_Pin, 1);             // we are able to receive data
    gpio_set_dir(kUART_HSKI_Pin, GPIO_OUT);  // we want to write the pin

    // -- Receive by interrupt
    irq_set_exclusive_handler(UART_IRQ_NUM(kUART), on_uart_rx);
    irq_set_enabled(UART_IRQ_NUM(kUART), true);
    uart_set_irqs_enabled(kUART, true, false);

    return UARTEndpoint::init();
}

/**
 * \brief Called regularly by the scheduler to take care of the UART device.
 * 
 * Peek for data that arrived at the CDC and needs to be sent to the pipe.
 * Other events (e.g. bitrate changes) are handled in TinyUSBTask which calls
 * tud_task() in every time slice.
 * 
 * \return Result::OK
 * 
 * Receive errors and buffer overflows are counted, see `ATI3`.
 */
Result PicoUARTEndpoint::task() {
    // Forward the received bytes until the pipe rejects one. A rejected byte
    // stays in the ring buffer and is sent again in the next cycle.
    while (rx_tail != rx_head) {
        Event event { rx_buffer[rx_tail] };
        Result r = out()->send(event);
        if (r.rejected())
            break;
        if (kLogUART) Log.log(event, 0);
        rx_tail = (rx_tail + 1) & (kRxBufferSize - 1);
    }
    return Result::OK;
}

/**
 * \brief Called whenever the in pipe has sent us an Event.
 * 
 * Data events are forwarded to the UART. Known other events are handled.
 * Unknown events are ignored, still returning OK.
 * 
 * \param event The event to send to the UART.
 * \return Result::OK if the event was handled. Result::REJECTED if the event 
 * must be resent again later by the caller.
 */
Result PicoUARTEndpoint::send(Event event) {
    switch (event.type()) {
        case Event::Type::DATA: {
            if (tx_wait_for_idle_) {
                // BUSY stays set until the stop bit of the last byte has left
                // the shift register. TXFE would start the pause one byte early.
                if (uart_get_hw(kUART)->fr & UART_UARTFR_BUSY_BITS) {
                    return Result::REJECTED;
                } else {
                    // Start the pause now. Its length doesn't depend on how often we are called.
                    tx_wait_for_idle_ = false;
                    tx_deadline_ = scheduler().now_us() + tx_delay_;
                    tx_delay_ = 0;
                }
            }
            if (scheduler().now_us() < tx_deadline_) {
                return Result::REJECTED;
            }
            bool cts = gpio_get(kUART_HSKO_Pin);
            // Only write into an empty TX FIFO. When the Newton drops CTS, at most
            // this byte and the one in the shift register still go out.
            bool fifo_empty = uart_get_hw(kUART)->fr & UART_UARTFR_TXFE_BITS;
            if (cts && fifo_empty) {
                uart_putc_raw(kUART, event.data());
                if (kLogUART) Log.log(event, 1);
                return Result::OK;
            } else {
                return Result::REJECTED;
            }
        }
        default:
            break;
    }
    if (kLogUART) Log.log(event, 1);
    return UARTEndpoint::send(event);
}

/**
 * \brief Delay the transmission of data for the given time.
 * 
 * \param usec The delay in microseconds.
 * \param chars The delay in characters at the current bitrate.
 */
void PicoUARTEndpoint::delay(uint32_t usec, uint32_t chars) {
    if (chars > 0) {
        // bits per second time ten (start bit, 8 data bits, stop bit)
        usec += ((chars * 1'000'000) / bitrate()) * 10;  
    }
    if (usec > 0) {
        if (tx_wait_for_idle_) {
            tx_delay_ += usec;          // the pause has not started yet
        } else if (scheduler().now_us() < tx_deadline_) {
            tx_deadline_ += usec;       // extend the running pause
        } else {
            tx_wait_for_idle_ = true;
            tx_delay_ = usec;
        }
    }
}

/**
 * \brief Handle handshake for data comming for the device.
 * 
 * A pipe on the output side is filling up and ask us to throttle sending data.
 */
void PicoUARTEndpoint::set_high_water(bool on) {
    if (on) {
        // Set the handshake pin to 0, so the Newton stops sending data.
        gpio_put(kUART_HSKI_Pin, 0);
    } else {
        // Set the handshake pin to 1, so the Newton can send data again.
        gpio_put(kUART_HSKI_Pin, 1);
    }
}

/**
 * \brief Set the new bitrate for the UART.
 * 
 * Some pipe or endpoint switched bitrate (usually the USB port). So we need to
 * set the new bitrate for the UART as well.
 * 
 * \param new_bitrate The new bitrate to set.
 */
void PicoUARTEndpoint::set_bitrate(uint32_t new_bitrate) {
    UARTEndpoint::set_bitrate(new_bitrate); // keep bitrate() in sync for delay()
    uart_set_baudrate(kUART, new_bitrate);
}

/**
 * \brief Get the number of receive errors since startup.
 */
UARTEndpoint::RxErrors PicoUARTEndpoint::rx_errors() const {
    RxErrors errors;
    errors.overrun = rx_error_count.overrun;
    errors.framing = rx_error_count.framing;
    errors.brk = rx_error_count.brk;
    errors.lost = rx_error_count.lost;
    return errors;
}

