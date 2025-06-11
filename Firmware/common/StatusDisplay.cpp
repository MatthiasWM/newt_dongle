//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "StatusDisplay.h"
#include "Scheduler.h"
#include "main.h"

using namespace nd;

StatusDisplay::StatusDisplay(Scheduler &scheduler) : Task(scheduler) {
    // Constructor implementation can be added here if needed
}

StatusDisplay::~StatusDisplay() {
    // Destructor implementation can be added here if needed
}

Result StatusDisplay::task() {
    // Get the current time. 
    // When .25 seconds elapsed, show the next LED combination.
    // If this was a temp status, decrement, and pop back to the main status.
    return Result::OK;
}

Result StatusDisplay::init() {
    // Initialization logic for the status display task
    Log.log("StatusDisplay: Initialized.\n");
    return Result::OK;
}

/**
 * Set the main status.
 */
void StatusDisplay::set(AppStatus new_status) {

}

/**
 * Set a temporary status a for the given number of ticks.
 * When the ticks elapsed, go back to the main status.
 */
void StatusDisplay::repeat(AppStatus temp_status, int repeats) {

}
