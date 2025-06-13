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

/*
 Use this project for live logging and debugging with the BreadBoard PiPico.

 This is Version 0.6. It has the following features:
  - Connects the UART to the CDC endpoint.
  - Hayes filter on both ends to allow Modem commands
  - Throttle MNP Packages from CDC to UART
  - Save settings in Flash RAM
  - Status Display to show the current status of the system.

  It has the following experimantal features:
  - MNP Filter to allow MNP encoded data to be sent and received.
  - Dock endpoint to allow docking with the Newton.
  - SDCard endpoint to allow reading and writing to the SDCard.

  TODO: 
   - Add three way switch between CDC and Dock endpoint.
   - Read the BOOT button: https://github.com/raspberrypi/pico-examples/blob/master/picoboard/button/button.c
   - Improved SDCard Reset.
 */

#include "main.h"

#include "PicoAsyncLog.h"
#include "PicoUARTEndpoint.h"
#include "PicoCDCEndpoint.h"
#include "PicoScheduler.h"
#include "PicoSystem.h"

#include "common/Endpoints/Dock.h"
#include "common/Endpoints/StdioLog.h"
#include "common/Filters/HayesFilter.h"
#include "common/Filters/MNPFilter.h"
#include "common/Pipes/BufferedPipe.h"
#include "common/Pipes/MNPThrottle.h"
#include "common/Pipes/Tee.h"

#include <pico/stdlib.h>

#include <stdio.h>


// Need to upload firmware into RAM instead of ROM.
void* __dso_handle = nullptr;
void* _fini = nullptr;


using namespace nd;


// Saves setting into Flash Memory.
PicoUserSettings user_settings;

// Log to debugging probe on uart1.
PicoAsyncLog Log(0);

// Distributes times slices to all tasks.
PicoScheduler scheduler;

// One task to manage the overall system.
PicoSystemTask system_task(scheduler);

// Task that updates the status display.
PicoStatusDisplay app_status(scheduler);

// Endpoints for data exchange.
PicoUARTEndpoint uart_endpoint { scheduler };
PicoCDCEndpoint cdc_endpoint { scheduler, 0 };
PicoSDCardEndpoint sdcard_endpoint { scheduler };
Dock dock_endpoint(scheduler);

// Filters and loggers.
HayesFilter uart_hayes(scheduler, 0);
HayesFilter cdc_hayes(scheduler, 1);
MNPFilter mnp_filter(scheduler);

// Pipes to connect everything.
MNPThrottle mnp_throttle(scheduler);
BufferedPipe buffer_to_cdc(scheduler);
BufferedPipe buffer_to_uart(scheduler);


// -- Everything is already allocated. Now link the endpoints and run the scheduler.
int main(int argc, char *argv[])
{
    static const uint32_t reset_mask = 
        0u
        |(1u<<RESET_ADC)
        |(1u<<RESET_BUSCTRL)
        |(1u<<RESET_DMA)
        |(1u<<RESET_I2C0)
        |(1u<<RESET_I2C1)
        |(1u<<RESET_IO_BANK0)
        |(1u<<RESET_IO_QSPI)
        // |(1u<<RESET_JTAG)
        |(1u<<RESET_PADS_BANK0)
        |(1u<<RESET_PADS_QSPI)
        |(1u<<RESET_PIO0)
        |(1u<<RESET_PIO1)
        // |(1u<<RESET_PLL_SYS)
        // |(1u<<RESET_PLL_USB)
        // |(1u<<RESET_PWM)
        |(1u<<RESET_RTC)
        |(1u<<RESET_SPI0)
        |(1u<<RESET_SPI1)
        |(1u<<RESET_SYSCFG)
        |(1u<<RESET_SYSINFO)
        |(1u<<RESET_TBMAN)
        |(1u<<RESET_TIMER)
        |(1u<<RESET_UART0)
        |(1u<<RESET_UART1)
        // |(1u<<RESET_USBCTRL)
    ;
    reset_block_mask(reset_mask);
    unreset_block_mask_wait_blocking(reset_mask);

    sdcard_endpoint.early_init(); // Initialize the GPIOs for the SD card
    app_status.early_init(); // Initialize the status display

    stdio_uart_init_full(uart1, 115200, 8, 9);
    Log.log("Starting Newton Dongle...\n");

    // user_settings.mess_up_flash();
    user_settings.read();
    scheduler.signal_all( Event {Event::Type::SIGNAL, Event::Subtype::USER_SETTINGS_CHANGED} );

    // -- Connect the Endpoints inside the dongle with pipes.
    // UART ---------------> UART_Hayes --------------------> CDC Hayes ---> buffer ---> CDC
    // UART <--- buffer <--- UART_Hayes <--- MNPThrottle <--- CDC Hayes <--------------- CDC
    
    // -- Goal:
    // UART ---------------> UART_Hayes --------------------> Matrix ---> CDC Hayes ---> buffer ---> CDC
    // UART <--- buffer <--- UART_Hayes <--- MNPThrottle <--- Matrix <--- CDC Hayes <--------------- CDC
    //                                                          ↑↓
    //                                                          MNP
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
