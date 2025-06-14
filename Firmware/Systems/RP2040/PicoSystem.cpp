//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoSystem.h"

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
