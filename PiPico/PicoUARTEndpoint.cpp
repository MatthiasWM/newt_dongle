//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoUARTEndpoint.h"
#include "common/Pipe.h"

#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pico/time.h"

// absolute_time_t get_absolute_time (void)
// int64_t absolute_time_diff_us (absolute_time_t from, absolute_time_t to)

#include <stdio.h>

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
#define UART_HSKO_PIN 22
//#define UART_HSKO_PIN 29

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

    uart_set_fifo_enabled(UART_ID, true);
    // int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;
    // irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    // irq_set_enabled(UART_IRQ, true);
    // uart_set_irq_enables(UART_ID, true, false);

    // Set the TX and RX pins by using the function select on the GPIO
    // Some notes for hardware flow control:
    // HSKI (28) and HSKO (29 on XIAO, 22 PiPico)
    // -- Initialize Handshake Out pin (HSKO, MP to dongle)
    // HSKO is 0 if the port in not open on the MP, or if the buffer is full
    // It's 1 whenever the MP is ready to recive data
    gpio_init(UART_HSKO_PIN);               // output from MP to dongle
    gpio_disable_pulls(UART_HSKO_PIN);      // let the MP do its work
    gpio_set_dir(UART_HSKO_PIN, GPIO_IN);   // we want to read the pin
    // -- Initialize Handshake In pin (HSKI, dongle to MP)
    gpio_init(UART_HSKI_PIN);               // output from MP to dongle
    gpio_put(UART_HSKI_PIN, 1);             // we are able to recive data
    gpio_set_dir(UART_HSKO_PIN, GPIO_OUT);  // we want to readthe pin
    // For debugging, note that we are alive
    //uart_puts(UART_ID, "Hello, Pico UART!\a\a\a\r\n");
    //printf("Pico UART Endpoint initialized.\n");
    // No error
    return 0;
}

int PicoUARTEndpoint::task() {
    uint32_t i;

    // Read all data from the fifo and put it into out pipe
    Pipe *out_pipe = out();
    if (out_pipe) {
        for (i=32; i>0; --i) {
            if (out_pipe->is_writable() && uart_is_readable(UART_ID)) {
                uint8_t c = uart_getc(UART_ID);
                //printf("[%02x]", c);
                out_pipe->putc(c);
            } else {
                break;
            }
        }    
    }

    Pipe *in_pipe = in();
    if (in_pipe) {
        // Read all data from the in pipe and send it to the USB
        for (i=32; i>0; --i) {
            bool cts = gpio_get(UART_HSKO_PIN);
            if (cts && in_pipe->data_available() && uart_is_writable(UART_ID)) {
                char c = in_pipe->getc();
                uart_putc_raw(UART_ID, c);
            } else {
                break;
            }
        }

        // Now check if there are any control blocks pending (handle one at a time)
        if (in_pipe->ctrl_available()) {
            CtrlBlock *ctrl_block = in_pipe->peek_ctrl();
            handle(in_pipe->peek_ctrl());
            //printf("UART Ctrl: %d %d %d %d %d\n", ctrl_block->cmd(), ctrl_block->d0(), ctrl_block->d1(), ctrl_block->d2(), ctrl_block->d3());
            in_pipe->pop_ctrl();
        }
    }
    return 0;
}

int PicoUARTEndpoint::handle(CtrlBlock *ctrl_block) {
    if (ctrl_block == nullptr) {
        puts("**** ERROR **** in PicoUARTEndpoint::handle() - no control block");
        return -1;
    }
    switch (ctrl_block->cmd()) {
        case Cmd::SET_BITRATE:
            set_bitrate(ctrl_block->d0());
            return 1;
        default:
            printf("**** ERROR ****: UART: Unknown command %d ( %d %d %d %d )\n", 
                ctrl_block->cmd(), 
                ctrl_block->d0(), ctrl_block->d1(), ctrl_block->d2(), ctrl_block->d3());
            return -1;
    }
    return -1;
}

void PicoUARTEndpoint::set_bitrate(uint32_t new_bitrate) {
    if (new_bitrate != bitrate_) {
        uart_set_baudrate(UART_ID, new_bitrate);
        bitrate_ = new_bitrate;
        //printf("Set UART bitrate to %d\n", bitrate_);
    }
}