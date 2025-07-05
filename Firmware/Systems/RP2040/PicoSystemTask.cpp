//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoSystemTask.h"

#include "main.h"

#include <hardware/uart.h>
#include <tusb.h>
#include <pico/stdlib.h>

using namespace nd;

/**
 * @class PicoSystemTask
 * @brief A task class for the PicoSystem, inheriting from nd::Task.
 *
 * The system task registers with the scheduler and handles system-level
 * maintenance tasks that need to be performed periodically. For example, 
 * all boards with USB port must call `tud_task()` to handle USB events.
 */

/**
 * @brief Executes the main task for the PicoSystem.
 *
 * This function performs the TinyUSB device task by invoking `tud_task()`.
 * It is responsible for handling USB-related operations and ensures the
 * system's USB functionality is maintained.
 *
 * @return nd::Result::OK Always returns a successful result.
 */
Result PicoSystemTask::task() {
    tud_task(); // tinyusb device task
    return nd::Result::OK;
}

void PicoSystemTask::reset_hardware()
{
    // See PicoUARTEndpoint::init() for pin setup
    // constexpr uint kUART_TX_Pin = 0;   // TX pin
    // constexpr uint kUART_RX_Pin = 1;   // RX pin
    // constexpr uint kUART_HSKI_Pin = 28; // HSKI pin
    // constexpr uint kUART_HSKO_Pin = 29; // HSKO pin on Dongle (22 on PiPico)

    // See PicoCDCEndpoint::early_init() for pin setup.
    // constexpr uint kSD_SEL_Pin = 27; // SD_SEL pin
    // constexpr uint kSD_MOSI_Pin = 3; // SD_MOSI pin
    // constexpr uint kSD_MISO_Pin = 4; // SD_MISO pin
    // constexpr uint kSD_SCK_Pin = 2; // SD_SCK pin

    // Initialize the GPIO pins.

    // constexpr uint kPORT_SEL_Pin = 26; // PORT_SEL pin
    // TODO: This pin indicates if the internal or external serial port is active.
    gpio_init(kPORT_SEL_Pin); // User LED red
    gpio_set_dir(kPORT_SEL_Pin, GPIO_IN);

    // constexpr uint kGPIO_Pin = 6; // GPIO pin
    // TODO: We can use this pin to get into Hayes Mode immediatly
    gpio_init(kGPIO_Pin); // User LED red
    gpio_set_dir(kGPIO_Pin, GPIO_IN);
    
    // constexpr uint kDOCK_ATTACH_Pin = 7; // DOCK_ATTACH pin
    // TODO: send docking signals through this pin
    gpio_init(kDOCK_ATTACH_Pin); // User LED red
    gpio_put(kDOCK_ATTACH_Pin, 1); 
    gpio_set_dir(kDOCK_ATTACH_Pin, GPIO_OUT);

    // See PicoStatusDisplay::early_init() for pin setup.
    // constexpr uint kLED_RED = 17;
    // constexpr uint kLED_GREEN = 16;
    // constexpr uint kLED_BLUE = 25; // (18 on PiPico)

#if 0 // Adding this does nothing for PiPico and hangs the NewtCOM Dongle.
    static const uint32_t reset_mask = 
        0u
        |(1u<<RESET_ADC)
        // |(1u<<RESET_BUSCTRL)
        // |(1u<<RESET_DMA)
        |(1u<<RESET_I2C0)
        |(1u<<RESET_I2C1)
        |(1u<<RESET_IO_BANK0)
        // |(1u<<RESET_IO_QSPI)
        // |(1u<<RESET_JTAG)
        |(1u<<RESET_PADS_BANK0)
        // |(1u<<RESET_PADS_QSPI)
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
#endif
}
