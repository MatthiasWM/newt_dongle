
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PosixSystemTask.h"

#include "main.h"

using namespace nd;

/**
 * @class SystemTask
 * @brief A task that supports the overall system.
 *
 * The system task registers with the scheduler and handles system-level
 * maintenance tasks that need to be performed periodically.
 */

PosixSystemTask::PosixSystemTask(Scheduler &scheduler) : SystemTask(scheduler) { }
