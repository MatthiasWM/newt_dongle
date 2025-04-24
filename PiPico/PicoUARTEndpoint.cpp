

#include "PicoUARTEndpoint.h"

#include "hardware/uart.h"
#include "common/Pipe.h"

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
    // For debugging, note that we are alive
    uart_puts(UART_ID, "Hello, Pico UART!\a\a\a\r\n");
    printf("Pico UART Endpoint initialized.\n");
    // No error
    return 0;
}

int PicoUARTEndpoint::task() {
    // TODO: interleave send, ctrl, and receive, set a maximum of iterations
    for (;;) {
        if (!uart_is_readable(UART_ID)) break;
        Pipe *out_pipe = out();
        if ((out_pipe == nullptr) || (!out_pipe->is_writable())) break;
        char c = uart_getc(UART_ID);
        printf("Received: %c\n", c);
        out_pipe->putc(c);
    }
    for (;;) {
        Pipe *in_pipe = in();
        if ((in_pipe == nullptr) || (!in_pipe->data_available()>0)) break;
        // TODO: if cmd avaialble, interprete it
        char c = in_pipe->getc();
        printf("Sending: %c\n", c);
        uart_putc(UART_ID, c);
    }


    // while (uart_is_readable(UART_ID)) {
    //     char c = uart_getc(UART_ID);
    //     printf("Received: %c\n", c);
    //     // Echo the character back
    //     uart_putc(UART_ID, c);
    // }    
    return 0;
}