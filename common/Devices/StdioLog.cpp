//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "StdioLog.h"

#include <cstdio>

using namespace nd;

/**
 * \class StdioLog
 * 
 * \brief The StdioLog class is a simple device that prints events to the console.
 * 
 * This class is used for debugging and testing purposes. It receives events
 * through a pipe and prints them to the standard output (stdout).
 * 
 * \todo On embedded devices, printing to the console takes to long and blocks
 *       other tasks. We need to buffer all incomming events and print them
 *       one by one whenever the wheel gives us the next task slot.
 */

/**
 * \brief We received and Event through the pipe, so print it to the console.
 */
Result StdioLog::send(Event event) {
    switch (event.type()) {
        case Event::Type::DATA:
            putchar(event.data());
            break;
        case Event::Type::ERROR:
            printf("Error: %d\n", event.data());
            break;
        default:
            printf("Unknown event type: %d\n", static_cast<int>(event.type()));
            break;
    }
    return Result::OK;
}

/**
 * \brief We received and urgent Event through the pipe, so print it to the console.
 */
Result StdioLog::rush(Event event) {
    puts("\n**** RUSH ****:");
    return send(event);
}
