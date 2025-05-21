//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoUARTEndpoint.h"

#include "main.h"
#include "common/Pipe.h"
#include "common/Scheduler.h"

#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include <stdio.h>

constexpr bool log_uart = true;

// \todo Hardware flow control: void uart_set_hw_flow (uart_inst_t * uart, bool cts, bool rts)
// Built-in hardware flow control is on ports 2 and 3, but we use those for SPI.
// We can emulate RTS and CTS using HSKI (28) and HSKO (29 on XIAO, 22 PiPico)

using namespace nd;

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
 * \todo Notify the user of a buffer overflow.
 */
Result PicoUARTEndpoint::task() {
    // RP2040 hardware does not allow us to peek at the next character in the FIFO.
    // So we must read and buffer it in case the out pipe rejects the event.
    if (event_pending_) {
        Result r = out()->send(pending_event_);
        if (r.rejected())
            return Result::OK;
        else
            event_pending_ = false;
        if (log_uart) Log.log(pending_event_, 0);
    }
    if (uart_is_readable(kUART)) {
        uint8_t c = uart_getc(kUART);
        Event event { c };
        Result r = out()->send(event);
        if (r.rejected()) {
            event_pending_ = true;
            pending_event_ = event;
            return Result::OK;
        }
        if (log_uart) Log.log(event, 0);
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
            if (tx_wait_for_fifo_empty_) {
                if (!(uart_get_hw(kUART)->fr & UART_UARTFR_TXFE_BITS)) {
                    return Result::REJECTED;
                } else {
                    tx_wait_for_fifo_empty_ = false;
                }
            }
            if (tx_delay_ > 0) {
                uint32_t cycle_time = scheduler().cycle_time();
                if (tx_delay_ >= cycle_time) {
                    tx_delay_ -= cycle_time;
                    return Result::REJECTED;
                } else {
                    tx_delay_ = 0;
                }
            }
            bool cts = gpio_get(kUART_HSKO_Pin);
            if (cts && uart_is_writable(kUART)) {
                uart_putc_raw(kUART, event.data());
                if (log_uart) Log.log(event, 1);
                return Result::OK;
            } else {
                return Result::REJECTED;
            }
        }
    }
    if (log_uart) Log.log(event, 1);
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
        if (tx_delay_ > 0) {
            tx_delay_ += usec;
        } else {
            tx_wait_for_fifo_empty_ = true;
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
    uart_set_baudrate(kUART, new_bitrate);
}

