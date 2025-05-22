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
 * classes must implement the task() or signal() method to define the specific 
 * behavior of the task.
 * 
 * Tasks are used for more complex pipes that need to update their state outside
 * of the Event sending process. A typical use is a buffering pipe that needs
 * additional time to empty the buffer when the output pipe becomes ready.
 * 
 * Tasks register with the scheduler. They cannot be copied or moved.
 */

/**
 * \brief Task are always registered with the scheduler.
 * \param[in] scheduler The scheduler to register with.
 * \param[in] job_map A bitmap of the jobs that this task is interested in.
 */
Task::Task(Scheduler &scheduler, uint8_t job_map) 
:   scheduler_(scheduler) 
{
    scheduler.add(*this, job_map);
}

/**
 * \function Task::scheduler() const
 * \brief Return the scheduler that this task is registered with.
 */

/**
 * \function Task::init()
 * \brief Override this to be notified by the scheduler that the system is about to start.
 * 
 * This is called once before any task() method is called. And is meant for 
 * last minute initializations after the base configuration is made, but before
 * the scheduler runs.
 * 
 * signal() may be called before init() (see `USER_SETTINGS_CHANGED`).
 * 
 * \return Result::OK, the value is currently not used.
 */

/** 
 * \function Task::task()
 * \brief Override this is your task wants to receive a regular time slice.
 * 
 * This is called periodically by the scheduler. It should return as fast
 * as possible. If the task needs to wait for something, it it should store
 * its status in a state machine and wait for the next call to task().
 * 
 * The scheduler provides the time in usec since the last call to task()
 * in Scheduler::cycle_time().
 * 
 * \return Result::OK, the value is currently not used.
 */

/**
 * \function Task::signal(Event event)
 * \brief Override this is your task wants to receive signals from the scheduler.
 * 
 * The scheduler uses Signalsnotify all tasks of the specified event. A typical
 * use is a change in configuration, or a user interaction.
 * 
 * The signal handler should return as fast as possible.
 * 
 * \return Result::OK, the value is currently not used.
 */
