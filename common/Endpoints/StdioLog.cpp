//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "StdioLog.h"

#include <stdio.h>

using namespace nd;

/**
 * \class StdioLog
 * 
 * \brief The StdioLog class is a simple End that prints events to the console.
 * 
 * This class is used for debugging and testing purposes. It receives events
 * through a pipe and prints them to the standard output (stdout).
 * 
 * \todo On embedded Endpoints, printing to the console takes to long and blocks
 *       other tasks. We need to buffer all incomming events and print them
 *       one by one whenever the scheduler gives us the next task slot.
 */

/**
 * \brief Check if wrting to stout would block.
 * 
 * \return true if the pipe would block, false otherwise.   
 */
bool StdioLog::would_block() {
    return false;
}

/**
 * \brief We received and Event through the pipe, so print it to the console.
 */
Result StdioLog::send(Event event) {
    static char hex_lut[] = "0123456789ABCDEF";
    static int count = 0;
    // TODO: testing
    if ((count++ & 1)||(count & 2)) {
        return Result::REJECTED;
    }
    if (would_block()) {
        return Result::REJECTED;
    }
    switch (event.type()) {
        case Event::Type::DATA: {
            uint8_t data = event.data();
            if ((data >= 0x20) && (data <= 0x7e)) {
                putchar(data);
            } else {
                putchar('\\');
                putchar(hex_lut[(data >> 4) & 0x0f]);
                putchar(hex_lut[data & 0x0f]);
            }
            break; }
        case Event::Type::ERROR:
            printf("E[%d]\n", event.data());
            break;
        default:
            printf("U[%d]\n", static_cast<int>(event.type()));
            break;
    }
    return Result::OK;
}

/**
 * \brief We received and urgent Event through the pipe, so print it to the console.
 */
Result StdioLog::rush(Event event) {
    puts("\n!");
    return send(event);
}
