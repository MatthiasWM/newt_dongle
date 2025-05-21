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

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart0
// Default for Dock is 38400, but the USB CDC should set the right speed.
// #define BAUD_RATE 19200
#define BAUD_RATE 38400

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_HSKI_PIN 28

// Define this for the PiPico board
// #define UART_HSKO_PIN 22
// Define this for the XIAO board
#define UART_HSKO_PIN 29

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
    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    // -- Initialize UART
    uart_init(UART_ID, BAUD_RATE);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_translate_crlf(UART_ID, false);
    uart_set_fifo_enabled(UART_ID, true);

    // -- Take care of hardware flow control
    uart_set_hw_flow(UART_ID, false, false);
    // Handshake Out (from MP to the Dongle)
    gpio_init(UART_HSKO_PIN);               // output from MP to dongle
    gpio_pull_up(UART_HSKO_PIN);            // let the MP do its work
    gpio_set_dir(UART_HSKO_PIN, GPIO_IN);   // we want to read the pin
    // Handshake In (from the Dongle to the MP)
    gpio_init(UART_HSKI_PIN);               // output from MP to dongle
    gpio_put(UART_HSKI_PIN, 1);             // we are able to receive data
    gpio_set_dir(UART_HSKI_PIN, GPIO_OUT);  // we want to write the pin

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
    if (uart_is_readable(UART_ID)) {
        uint8_t c = uart_getc(UART_ID);
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
                if (!(uart_get_hw(UART_ID)->fr & UART_UARTFR_TXFE_BITS)) {
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
            bool cts = gpio_get(UART_HSKO_PIN);
            if (cts && uart_is_writable(UART_ID)) {
                uart_putc_raw(UART_ID, event.data());
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
        gpio_put(UART_HSKI_PIN, 0);
    } else {
        // Set the handshake pin to 1, so the Newton can send data again.
        gpio_put(UART_HSKI_PIN, 1);
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
    uart_set_baudrate(UART_ID, new_bitrate);
}

