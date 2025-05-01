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
#include "PicoSDCard.h"
#include "common/Endpoints/StdioLog.h"
#include "common/Endpoints/TestEventGenerator.h"
#include "common/Filters/HayesFilter.h"
#include "common/Pipes/BufferedPipe.h"
#include "common/Pipes/Tee.h"

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"

#include <stdio.h>

#if 0
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

/*

  UART ---------------> UART_Hayes ---> CDC Hayes ---> buffer ---> CDC
  UART <--- buffer <--- UART_Hayes <--- CDC Hayes <--------------- CDC
  
 */

// -- The scheduler spins while the dongle is powered and delivers time slices to its spokes.
nd::PicoScheduler scheduler;
TinyUSBTask tinyusb_task(scheduler);

// -- Instantiate all the endpoints we need.
nd::PicoUARTEndpoint uart_endpoint { scheduler };
nd::PicoCDCEndpoint cdc_endpoint { scheduler, 0 };

// -- Instantiate filters and loggers.
nd::HayesFilter uart_hayes(scheduler);
nd::HayesFilter cdc_hayes(scheduler);

// -- Instantiate pipes to connect everything.
nd::BufferedPipe buffer_to_cdc(scheduler);
nd::BufferedPipe buffer_to_uart(scheduler);

// -- Everything is already allocated. Now link the endpoints and run the scheduler.
int main(int argc, char *argv[])
{
    stdio_uart_init_full(uart1, 115200, 8, 9);

    test_sd_card();

    // -- Connect the Endpoints inside the dongle with pipes.
    // Connect the UART to USB
    uart_endpoint >> uart_hayes.downstream; 
    uart_hayes.upstream >> cdc_hayes.upstream;
    cdc_hayes.downstream >> buffer_to_cdc >> cdc_endpoint;
    // Connect USB to the UART
    cdc_endpoint >> cdc_hayes.downstream; 
    cdc_hayes.upstream >> uart_hayes.upstream;
    uart_hayes.downstream >> buffer_to_uart >> uart_endpoint;

    // -- The scheduler will call all instances of classes that are derived from Task.
    printf("Welcome to NewtDongle!\nInitializing...\n");
    scheduler.init();

    // -- Now we can start the scheduler. It will call all spokes in a loop.
    printf("Running...\n");
    scheduler.run();

    // -- Never reached.
    return 0;
} 
