//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoSystem.h"

#include "hardware/uart.h"
#include "tusb.h"
#include "pico/stdlib.h"

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
