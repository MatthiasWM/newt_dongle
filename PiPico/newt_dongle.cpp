/*
  MIT License

  Copyright (c) 2025 Matthias Melcher, robowerk.de

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"

#include "PicoUARTEndpoint.h"
#include "PicoCDCEndpoint.h"
#include "common/Pipe.h"

nd::PicoUARTEndpoint uart_endpoint;
nd::PicoCDCEndpoint cdc_endpoint;
nd::Pipe uart_test_pipe;
nd::Pipe cdc_test_pipe;

nd::Pipe uart_to_cdc_pipe;
nd::Pipe cdc_to_uart_pipe;

int main()
{
    stdio_uart_init_full(uart1, 115200, 8, 9);

    cdc_endpoint.init();
    uart_endpoint.init();

#if 0 // loopbak for UART and CDC
    cdc_test_pipe.connect_from(cdc_endpoint).connect_to(cdc_endpoint);
    uart_test_pipe.connect_from(uart_endpoint).connect_to(uart_endpoint);
#else
    uart_test_pipe.connect_from(uart_endpoint).connect_to(cdc_endpoint);
    cdc_test_pipe.connect_from(cdc_endpoint).connect_to(uart_endpoint);
#endif

    while (true) {
        tud_task(); // tinyusb device task
        cdc_endpoint.task();
        uart_endpoint.task();
    }
}

