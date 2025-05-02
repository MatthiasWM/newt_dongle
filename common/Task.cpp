//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Task.h"

#include "Scheduler.h"

using namespace nd;

/**
 * @brief Task A task is a pipe that will be called regularly by the scheduler.
 * 
 * The Task class is an abstract base class that inherits from Pipe. Derived 
 * classes must implement the task() method to define the specific behavior 
 * of the task.
 * 
 * Tasks are used for more complex pipes that need to update their state outside
 * of the Event sending process. A typical use is a buffering pipe that needs
 * additional time to empty the buffer when the output pipe becomes ready.
 * 
 * Tasks register with the scheduler. They cannot be copied or moved.
 */


Task::Task(Scheduler &scheduler) 
:   scheduler_(scheduler) 
{
    scheduler.add(*this);
}

