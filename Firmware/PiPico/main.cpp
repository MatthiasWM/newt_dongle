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

// TODO: add three way switch and SDCard access
// // https://www.makermatrix.com/blog/read-and-write-data-with-the-pi-pico-onboard-flash/
// XIP_BASE # The base address of the flash memory
// PICO_FLASH_SIZE_BYTES # The total size of the RP2040 flash, in bytes
// FLASH_SECTOR_SIZE     # The size of one sector, in bytes (the minimum amount you can erase)
// FLASH_PAGE_SIZE       # The size of one page, in bytes (the mimimum amount you can write)
// extern "C" {
//   #include <hardware/flash.h>
// };
// flash_range_erase(uint32_t flash_offs, size_t count);
// flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count);
//
//  __flash_binary_end (which is a memory address, not an offset in flash)
//
// https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#group_pico_standard_binary_info
//
// 264kB RAM
// pico_cmake_set_default PICO_FLASH_SIZE_BYTES = (2 * 1024 * 1024)
//
// read the BOOT button: https://github.com/raspberrypi/pico-examples/blob/master/picoboard/button/button.c

//
// https://community.element14.com/products/raspberry-pi/b/blog/posts/raspberry-pico-c-sdk-reserve-a-flash-memory-block-for-persistent-storage
// pico_set_linker_script("pico/pico_standard_link.ld")

#include "main.h"

#include "PicoStdioLog.h"
#include "PicoAsyncLog.h"
#include "PicoUARTEndpoint.h"
#include "PicoCDCEndpoint.h"
#include "PicoScheduler.h"
#include "PicoSDCard.h"
#include "PicoSystem.h"

#include "common/Endpoints/Dock.h"
#include "common/Endpoints/StdioLog.h"
#include "common/Endpoints/TestEventGenerator.h"
#include "common/Filters/HayesFilter.h"
#include "common/Filters/MNPFilter.h"
#include "common/Pipes/BufferedPipe.h"
#include "common/Pipes/MNPThrottle.h"
#include "common/Pipes/Tee.h"


#include "pico/stdlib.h"

#include <stdio.h>

void* __dso_handle = nullptr;
void* _fini = nullptr;

using namespace nd;

PicoUserSettings user_settings;
PicoAsyncLog Log(0);

// -- The scheduler spins while the dongle is powered and delivers time slices to its spokes.
PicoScheduler scheduler;
PicoSystemTask system_task(scheduler);

// -- Instantiate all the endpoints we need.
PicoUARTEndpoint uart_endpoint { scheduler };
PicoCDCEndpoint cdc_endpoint { scheduler, 0 };
PicoSDCardEndpoint sdcard_endpoint { scheduler };
Dock dock_endpoint(scheduler);

// -- Instantiate filters and loggers.
HayesFilter uart_hayes(scheduler, 0);
HayesFilter cdc_hayes(scheduler, 1);
// -- Experimental MNP Filter
MNPFilter mnp_filter(scheduler);

// -- Instantiate pipes to connect everything.
MNPThrottle mnp_throttle(scheduler);
BufferedPipe buffer_to_cdc(scheduler);
BufferedPipe buffer_to_uart(scheduler);


// -- Everything is already allocated. Now link the endpoints and run the scheduler.
int main(int argc, char *argv[])
{
    stdio_uart_init_full(uart1, 115200, 8, 9);
    Log.log("Starting Newton Dongle...\n");

    // user_settings.mess_up_flash();
    user_settings.read();
    scheduler.signal_all( Event {Event::Type::SIGNAL, Event::Subtype::USER_SETTINGS_CHANGED} );

    // Set the LED to yellow for now (0=on, 1=off).
    gpio_init(17); // User LED red
    gpio_put(17, 0);
    gpio_set_dir(17, GPIO_OUT);

    gpio_init(16); // User LED green
    gpio_put(16, 0);
    gpio_set_dir(16, GPIO_OUT);

    gpio_init(25); // User LED red
    gpio_put(25, 1);
    gpio_set_dir(25, GPIO_OUT);


    //Log.log("Starting...\n");
    //test_sd_card();

    // -- Connect the Endpoints inside the dongle with pipes.
    // UART ---------------> UART_Hayes --------------------> CDC Hayes ---> buffer ---> CDC
    // UART <--- buffer <--- UART_Hayes <--- MNPThrottle <--- CDC Hayes <--------------- CDC
    
    // -- Goal:
    // UART ---------------> UART_Hayes --------------------> Matrix ---> CDC Hayes ---> buffer ---> CDC
    // UART <--- buffer <--- UART_Hayes <--- MNPThrottle <--- Matrix <--- CDC Hayes <--------------- CDC
    //                                                          ↑↓
    //                                                       MNP/V.24
    //                                                          ↑↓
    //                                                         Dock    
    //                                                          ↑↓
    //                                                        SDCard    

#if 0
    // Connect the UART to USB
    /**/  uart_endpoint >> uart_hayes.downstream; 
    /**/    uart_hayes.upstream >> cdc_hayes.upstream;
    /**/      cdc_hayes.downstream >> buffer_to_cdc >> cdc_endpoint;
    // Connect USB to the UART
    /**/  cdc_endpoint >> cdc_hayes.downstream; 
    /**/    cdc_hayes.upstream >> mnp_throttle >> uart_hayes.upstream;
    /**/      uart_hayes.downstream >> buffer_to_uart >> uart_endpoint;
#else
    // Connect the UART to Dock
    /**/  uart_endpoint >> uart_hayes.downstream; 
    /**/    uart_hayes.upstream >> cdc_hayes.upstream;
    /**/      cdc_hayes.downstream >> buffer_to_cdc >> mnp_filter.newt;
    /**/        mnp_filter.newt >> dock_endpoint;
    // Connect USB to the UART
    /**/  dock_endpoint >> mnp_filter.dock;
    /**/    mnp_filter.dock >> cdc_hayes.downstream; 
    /**/      cdc_hayes.upstream >> mnp_throttle >> uart_hayes.upstream;
    /**/        uart_hayes.downstream >> buffer_to_uart >> uart_endpoint;
#endif

    // -- Give both serial ports access to the SD Card (currently for debugging only)
    uart_hayes.link(&sdcard_endpoint);
    cdc_hayes.link(&sdcard_endpoint);

    // -- The scheduler will call all instances of classes that are derived from Task.
    scheduler.init();

    // -- Now we can start the scheduler. It will call all spokes in a loop.
    scheduler.run();

    // -- Never reached.
    return 0;
} 
