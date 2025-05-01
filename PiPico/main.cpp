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

// -- Minimal setup for the Newton Dongle.
//  UART ---> BufferPipe ----> CDC
//  UART <--- BufferPipe <---- CDC

// TODO: add mnp throttle filter
// TODO: add Hayes filter
// TODO: add three way switch and SDCard access


#include "PicoStdioLog.h"
#include "PicoUARTEndpoint.h"
#include "PicoCDCEndpoint.h"
#include "PicoScheduler.h"
#include "common/Endpoints/StdioLog.h"
#include "common/Endpoints/TestEventGenerator.h"
#include "common/Pipes/BufferedPipe.h"
#include "common/Pipes/Tee.h"

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"

#include <stdio.h>

#if 1
// -- The code below avoids the linker error "undefined reference to '__dso_handle'"
extern "C" void *__dso_handle;
void *__dso_handle = nullptr;
extern "C" int __cxa_atexit(void (*destructor)(void*), void *object, void *dso_handle) {
    return 0; // Do nothing
}
extern "C" void __cxa_finalize(void *dso_handle) {
    // Do nothing
}
#endif

// -- The task that runs the tinyusb device stack.
class TinyUSBTask: public nd::Task {
public:
    TinyUSBTask(nd::Scheduler &scheduler) : nd::Task(scheduler) { }
    nd::Result task() override {
        tud_task(); // tinyusb device task
        return nd::Result::OK;
    }
};

// -- The scheduler spins while the dongle is powered and delivers time slices to its spokes.
nd::PicoScheduler scheduler;
TinyUSBTask tinyusb_task(scheduler);

// -- Allocate all the endpoints we need.
nd::PicoUARTEndpoint uart_endpoint { scheduler };
nd::PicoCDCEndpoint cdc_endpoint { scheduler, 0 };
nd::PicoStdioLog log_endpoint { scheduler };

// -- Allocate the pipes and filters that connect the endpoints.
nd::BufferedPipe uart_to_tee(scheduler);
nd::BufferedPipe tee_to_cdc(scheduler);
nd::BufferedPipe cdc_to_uart(scheduler);
nd::Tee tee;

// -- Everything is already allocated. Now link the endpoints and run the scheduler.
int main(int argc, char *argv[])
{
    stdio_uart_init_full(uart1, 115200, 8, 9);

    // -- Connect the Endpoints inside the dongle with pipes.
    uart_endpoint >> uart_to_tee >> tee;
    tee.a >> cdc_endpoint;
    tee.b >> log_endpoint;
    cdc_endpoint >> cdc_to_uart >> uart_endpoint;

    // -- The scheduler will call all instances of classes that are derived from Task.
    printf("Welcome to NewtDongle!\nInitializing...\n");
    scheduler.init();

    // -- Now we can start the scheduler. It will call all spokes in a loop.
    printf("Running...\n");
    scheduler.run();

    // -- Never reached.
    return 0;
} 

#if 0

#include "PicoUARTEndpoint.h"
#include "PicoCDCEndpoint.h"
#include "common/Pipe.h"
#include "common/Event.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"

nd::PicoUARTEndpoint uart_endpoint;
nd::PicoCDCEndpoint cdc_endpoint(BOARD_TUD_RHPORT);
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

  absolute_time_t tBase = get_absolute_time();
  absolute_time_t tNext = make_timeout_time_ms(1000);

  while (true) {
    absolute_time_t t0 = get_absolute_time();
    tud_task(); // tinyusb device task
    absolute_time_t t1 = get_absolute_time();
    cdc_endpoint.task();
    absolute_time_t t2 = get_absolute_time();
    uart_endpoint.task();
    absolute_time_t tn = get_absolute_time();
    int64_t diff = absolute_time_diff_us(t0, tn);
#if 0
    if (absolute_time_diff_us(tNext, tn) > 0) {
      tNext = make_timeout_time_ms(1000);
      printf("Scheduler: tot:%d tud:%d cdc:%d uart:%d\n", 
        (int)absolute_time_diff_us(t0, tn),
        (int)absolute_time_diff_us(t0, t1),
        (int)absolute_time_diff_us(t1, t2),
        (int)absolute_time_diff_us(t2, tn));
    }
#endif
  }

}

#endif