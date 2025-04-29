//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoUARTEndpoint.h"
#include "common/Pipe.h"

#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include <stdio.h>

// MNP Block Start: 0x16 0x10 0x02
// MNP Block End:   0x10 0x03 crcL crcH

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
    gpio_set_dir(UART_HSKI_PIN, GPIO_OUT);  // we want to read the pin
    // For debugging, note that we are alive
    //uart_puts(UART_ID, "Hello, Pico UART!\a\a\a\r\n");
    //printf("Pico UART Endpoint initialized.\n");
    // No error
    return 0;
}

bool hski = 1;
bool hsko = 1;

int PicoUARTEndpoint::task() {
    uint32_t i;

    // Read all data from the FIFO and put it into out pipe
    Pipe *out_pipe = out();
    if (out_pipe) {
        // Handle a bunch of DATA events.
        for (i=cfgMaxBurstRead; i>0; --i) {
            // Clear the handshake pin if we run out of buffer capacity.
            gpio_put(UART_HSKI_PIN, !out_pipe->reached_high_water());
            if (debugShowUARTHandshake) {
                bool ctr = out_pipe->reached_high_water();
                if (ctr != hski) {
                    if (hski) putchar('I'); else putchar('i');
                }
                hski = ctr;
            }        
            if ((out_pipe->num_free() > 0) && uart_is_readable(UART_ID)) {
                uint8_t c = uart_getc(UART_ID);
                // printf("[%02x]", c);
                out_pipe->write(make_event(Type::DATA, c));
            } else {
                break;
            }
        }
    }

    // Read all data from the in pipe and send it to the device
    Pipe *in_pipe = in();
    if (in_pipe) {
        Event ev = in_pipe->peek();
        // Handle a bunch of DATA events first.
        for (i=cfgMaxBurstWrite; i>0; --i) {
            bool cts = gpio_get(UART_HSKO_PIN);
            if (debugShowUARTHandshake) {
                if (cts != hsko) {
                    if (hsko) putchar('O'); else putchar('o');
                    hsko = cts;
                }
            }
            if (cts && (event_type(ev) == Type::DATA) && uart_is_writable(UART_ID)) {
                uint8_t c = event_data(ev);
                if ((mnp_state_ > 0) || (c == 0x10)) {
                    if (handle_mnp(ev) == 0) break;
                }
                //printf("<%02x>", c);
                uart_putc_raw(UART_ID, c);
                in_pipe->pop();
                ev = in_pipe->peek();
            } else {
                break;
            }
        }
        // Now handle one control event if available.
        if ((event_type(ev) != Type::ERROR) && (event_type(ev) != Type::DATA)) {
            if (handle(ev)) {
                // If the event was handled, pop it from the pipe.
                // If it was not handled, it will be presented again.
                in_pipe->pop();
            }
        }
        // \todo How exactly do we want to handle error events?
    }
    return 0;
}

int PicoUARTEndpoint::handle(Event event) {
    switch (event_type(event)) {
        case Type::SET_BITRATE: {
            uint8_t data = event_data(event);
            uint32_t new_bitrate = id_to_bitrate(data);
            set_bitrate(new_bitrate);
            return 1; }
        case Type::DELAY_MS:    // Wait for the last byte to be processed, then wait n miliseconds
        case Type::DELAY_US:    // Wait n microseconds
        case Type::DELAY_CHAR:  // Wait for the time it would take to process n characters
            return handle_delay(event);
        default:
            printf("**** WARNING ****: UART: Unknown command %d ( %d )\n", 
                static_cast<int>(event_type(event)),
                event_data(event));
            return 1;
    }
    return 1;
}

int PicoUARTEndpoint::handle_delay(Event event) {
    uint16_t data = 0;
    switch (delay_state_) {
        case 0: // make sure that the UART tx FIFO is drained before calculating the delay
            if ((uart_get_hw(UART_ID)->fr & UART_UARTFR_RXFE_BITS) != 0) delay_state_ = 1;
            return 0;
        case 1: // tx FIFO is empty: calculate the delay from now
            data = event_data(event);
            // No delay, so we can return immediately.
            if (data == 0) {
                delay_state_ = 0;
                return 1; // Pop the delay from the pipe.
            }
            // Set the delay time and state.
            switch (event_type(event)) {
                case Type::DELAY_MS:
                    delay_until_ = make_timeout_time_ms(fp8_to_uint20(data));
                    break;
                case Type::DELAY_US:
                    delay_until_ = make_timeout_time_us(fp8_to_uint20(data));
                    break;
                case Type::DELAY_CHAR: { // up to 255 characters
                    uint32_t byterate = (bitrate_>=300) ? bitrate_/10 : 30;
                    uint32_t us = ((uint32_t)data) * 1'000'000 / byterate;
                    delay_until_ = make_timeout_time_us(us);
                    break; }
                default:
                    printf("**** WARNING ****: UART: Unknown delay type %d\n", event_type(event));
                    return -1;
            }
            delay_state_ = 2;
            return 0;
        case 2: // Just return to the handler until the delay time is reached.
            if (absolute_time_diff_us(delay_until_, get_absolute_time()) > 0) {
                // Delay time has passed, so we can return.
                delay_state_ = 0;
                // putchar(']');
                return 1; // Pop the DELAY event from the pipe when we are done.
            }
            return 0; 
        default:
            printf("**** WARNING ****: UART: Unknown delay state %d\n", delay_state_);
            return -1;
    }
}

int PicoUARTEndpoint::handle_mnp(Event event) {
    // printf("%d[%02x]:", mnp_state_, event_data(event));
    switch (mnp_state_) {
        case 0: // We just received a 0x10
            mnp_state_ = 1;
            return 1;
        case 1: // 0x02, 0x03, or 0x10 : We already forwarded the 0x10. 0x02 makes this a block start, 0x03 a block end, everything else does not matter.
            if (event_data(event) == 0x02) {
                mnp_state_ = 2;
                return 1;
            } else if (event_data(event) == 0x03) {
                mnp_state_ = 3;
                return 1;
            } else {
                mnp_state_ = 0;
                return 1;
            }
        case 2: // First byte in block: We are in a block. There is nothing I want to do here at the moment
            mnp_state_ = 0;
            return 1;
        case 3: // CRC LSB: We are almost at the end of a block.
            mnp_state_++;
            return 1;
        case 4: // CRC MSB: End of block
            in()->pop(); 
            //in()->prepend(make_delay(100'000)); // <------- Experimental. Nice for NCX, not working for NTK (Classic)
            //in()->prepend(make_delay(1'000));
            in()->prepend(make_delay(0));
            // putchar('[');
            in()->prepend(event);
            mnp_state_ = 0;
            return 0; // Don't pop the event!
    }
    return 1;
}

void PicoUARTEndpoint::set_bitrate(uint32_t new_bitrate) {
    if (new_bitrate != bitrate_) {
        uart_set_baudrate(UART_ID, new_bitrate);
        bitrate_ = new_bitrate;
        printf("------> Set UART bitrate to %d\n", bitrate_);
    }
}