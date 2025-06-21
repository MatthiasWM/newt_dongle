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
 Use this project to create ROM Images for the NewtCOM Dongle.

 This is Version 0.6a. It has the following features:
  - Dock protocol emulation 
    - Connect to emulated NCU via Dock protocol
    - Browse files on SD Card, download , and install
 The Version 0.5. It has the following features:
  - Connects the UART to the CDC endpoint.
  - Hayes filter on both ends to allow Modem commands
  - Throttle MNP Packages from CDC to UART
  - Save settings in Flash RAM
  - Status Display to show the current status of the system.

Hardware Version 4.0

                __[   ]__  USB-C
     PORT_SEL -[ 26      ]- +5V
       SD_SEL -[ 27      ]- GND
         HSKI -[ 28      ]- +3.3V
         HSKO -[ 29    3 ]- SD_MOSI
         GPIO -[ 6     4 ]- SD_MISO
  DOCK_ATTACH -[ 7     2 ]- SD_SCK
          TX0 -[_0__()_1_]- RX0
                  BATT+


 DOCK_ATTACH must be pulled high. To wake the Newton up, pull low for 1ms (max. 2ms).
 If held low for more than 10ms, Newton will see it as a Docking Event.
 It should be held low until the dock is removed.
 The implementation for NewtCOM could mirror the DTR on the USB port. It could 
 also reflect if a Dock connection to the SD Card is active.

 PORT_SEL can be read. If the internal serial port is selected, we should ignore
 all traffic on Serial 0.

 GPIO can be defined as an input or output. For NewtCOM it is useless without a
 Newton application. If we have a Newton app, we could use iit to get us into
 Hayes command mode, even if all settings are messed up.

 */

#include "main.h"

#include "PicoAsyncLog.h"
#include "PicoUARTEndpoint.h"
#include "PicoCDCEndpoint.h"
#include "PicoScheduler.h"
#include "PicoSystem.h"

#include "common/Endpoints/Dock.h"
#include "common/Endpoints/StdioLog.h"
#include "common/Filters/DTRSwitch.h"
#include "common/Filters/HayesFilter.h"
#include "common/Filters/MNPFilter.h"
#include "common/Pipes/BufferedPipe.h"
#include "common/Pipes/MNPThrottle.h"
#include "common/Pipes/Tee.h"

#include <pico/stdlib.h>

#include <stdio.h>


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
DTRSwitch dtr_switch;

// Pipes to connect everything.
MNPThrottle mnp_throttle(scheduler);
BufferedPipe buffer_to_cdc(scheduler);
BufferedPipe buffer_to_uart(scheduler);


// -- Everything is already allocated. Now link the endpoints and run the scheduler.
int main(int argc, char *argv[])
{
    system_task.reset_hardware(); 

    sdcard_endpoint.early_init(); // Initialize the GPIOs for the SD card
    app_status.early_init(); // Initialize the status display

    stdio_uart_init_full(uart1, 115200, 8, 9);
    
    // user_settings.mess_up_flash();
    user_settings.read();
    scheduler.signal_all( Event {Event::Type::SIGNAL, Event::Subtype::USER_SETTINGS_CHANGED} );

    // -- Connect the Endpoints inside the dongle with pipes.
    // UART ---------------> UART_Hayes --------------------> DTRSwitch ---> CDC Hayes ---> buffer ---> CDC
    // UART <--- buffer <--- UART_Hayes <--- MNPThrottle <--- DTRSwitch <--- CDC Hayes <--------------- CDC
    //                                                           ↑↓
    //                                                           MNP
    //                                                           ↑↓
    //                                                          Dock    
    //                                                           ↑↓
    //                                                         SDCard    

    // Connect the UART to the Dock or the CDC, depending on the CDC DTR pin.
    /**/  uart_endpoint >> uart_hayes.downstream;
    /**/    uart_hayes.upstream >> dtr_switch;
    /**/      dtr_switch.dock >> mnp_filter.newt;
    /**/        mnp_filter.newt >> dock_endpoint;
    /**/      dtr_switch.cdc >> cdc_hayes.upstream;
    /**/        cdc_hayes.downstream >> buffer_to_cdc >> cdc_endpoint;
    // Connect the USB CDC and the Dock back to the UART.
    /**/  dock_endpoint >> mnp_filter.dock;
    /**/    mnp_filter.dock >> dtr_switch.dock;
    /**/  cdc_endpoint >> cdc_hayes.downstream;
    /**/    cdc_hayes.upstream >> dtr_switch.cdc;
    /**/      dtr_switch >> mnp_throttle >> uart_hayes.upstream;
    /**/        uart_hayes.downstream >> buffer_to_uart >> uart_endpoint;

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
