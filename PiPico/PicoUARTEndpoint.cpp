//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoUARTEndpoint.h"
#include "common/Pipe.h"

#include "hardware/uart.h"
#include "pico/stdlib.h"

#include <stdio.h>

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart0
#define BAUD_RATE 19200

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// \todo Hardware flow control: void uart_set_hw_flow (uart_inst_t * uart, bool cts, bool rts)
// Built-in hardware flow control is on ports 2 and 3, but we use those for SPI.
// We can emulate RTS and CTS using HSKI (28) and HSKO (29 on XIAO, 22 PiPico)

using namespace nd;

PicoUARTEndpoint::PicoUARTEndpoint() {
    // Constructor implementation
}

PicoUARTEndpoint::~PicoUARTEndpoint() {
    // Destructor implementation
}

int PicoUARTEndpoint::init() {
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));
    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_translate_crlf(UART_ID, false);
    // Set the TX and RX pins by using the function select on the GPIO
    // Some notes for hardware flow control:
    // HSKI (28) and HSKO (29 on XIAO, 22 PiPico)
        // void gpio_init (uint gpio) // Set function SIO
        // void gpio_set_input_enabled (uint gpio, bool enabled)
        // void gpio_set_function (uint gpio, gpio_function_t fn)
        // void gpio_pull_up (uint gpio)
        // void gpio_disable_pulls (uint gpio)
        // bool gpio_get (uint gpio)
        // void gpio_put (uint gpio, bool value)
        // void gpio_set_dir (uint gpio, bool out)
    // For debugging, note that we are alive
    //uart_puts(UART_ID, "Hello, Pico UART!\a\a\a\r\n");
    //printf("Pico UART Endpoint initialized.\n");
    // No error
    return 0;
}

int PicoUARTEndpoint::task() {
    // The score ensures that we don't get stuck in an infinite loop
    int score = 1000;
    bool busy = true;
    while (busy && (score > 0)) {
        busy = false;
        while ((score > 0) && uart_is_readable(UART_ID)) {
            busy = true;
            char c = uart_getc(UART_ID);
            printf("Received: %c\n", c);
            Pipe *out_pipe = out();
            if ((out_pipe == nullptr) || (!out_pipe->is_writable())) break;
            out_pipe->putc(c);
            score--;
        }
        Pipe *in_pipe = in();
        while ((score > 0) && (in_pipe) && (in_pipe->data_available()>0)) {
            busy = true;
            char c = in_pipe->getc();
            printf("Sending: %c\n", c);
            uart_putc(UART_ID, c);
            score--;
        }
        while ((score > 0) && in_pipe->ctrl_available()) {
            busy = true;
            CtrlBlock *ctrl_block = in_pipe->peek_ctrl();
            printf("Ctrl: %d %d %d %d %d\n", ctrl_block->cmd(), ctrl_block->d0(), ctrl_block->d1(), ctrl_block->d2(), ctrl_block->d3());
            in_pipe->pop_ctrl();
            score -= 100;
        }
    }
    return 0;
}

