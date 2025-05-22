//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Event.h"

#include "main.h"

#include <stdio.h>

using namespace nd;

/**
 * @brief Event class for managing various types of communication and control events.
 * 
 * The Event class represents different types of communication events like data transmission,
 * bitrate settings, and pause/delay events. It uses a memory-efficient union representation
 * to store event information, with a type, subtype, and associated data.
 * 
 * @details Events can be one of several types:
 *   - NIL: Empty/null event
 *   - DATA: Represents a data byte to be transmitted
 *   - SET_BITRATE: Configuration event to change communication bitrate
 *   - PAUSE: Delay/pause event with configurable duration
 *   - ERROR: Error indicator with associated error data
 * 
 * Pause events can have different time representations through the Subtype enum:
 *   - USECS: Microsecond delay
 *   - MSECS: Millisecond delay
 *   - CHARS: Character-time based delay (dependent on current bitrate)
 */


Event nd::Event::make_bitrate_event(uint32_t bitrate) {
    return Event(Type::SET_BITRATE, bitrate_to_id(bitrate));
}

uint32_t Event::id_to_bitrate(uint8_t id) {
    switch (id) {
        case 0: return 300;
        case 1: return 1200;
        case 2: return 2400; 
        case 3: return 4800; 
        case 4: return 9600; 
        case 5: return 14400;
        case 6: return 19200;
        case 7: return 28800; 
        case 8: return 38400; 
        case 9: return 57600; 
        case 10: return 115200; 
        case 11: return 230400;
        default:
            if (kDebugErrors) printf("**** ERROR ***: unsupported bitrate ID %d\n", id);
            return 38400;
    }
}

uint8_t Event::bitrate_to_id(uint32_t bitrate) {
    switch (bitrate) {
        case    300: return 0;
        case   1200: return 1;
        case   2400: return 2; 
        case   4800: return 3; 
        case   9600: return 4; 
        case  14400: return 5;
        case  19200: return 6;
        case  28800: return 7; 
        case  38400: return 8; 
        case  57600: return 9; 
        case 115200: return 10; 
        case 230400: return 11;
        default:
            if (kDebugErrors) printf("**** ERROR ***: unsupported bitrate %d\n", bitrate);
            return 0xff;
    }
}

Event Event::make_delay_event(uint32_t usec) {
    if (usec > 0xFFFF) {
        return Event(Type::DELAY, Subtype::MSECS, usec/1000);
    }
    return Event(Type::DELAY, Subtype::USECS, usec);
}


/**
 * @brief Result class for operation outcome representation
 * 
 * A utility class to represent the result of operations with type information,
 * subtypes, and additional data. This provides a structured way to handle
 * success and failure cases with contextual information.
 * 
 * The class defines two basic instances for common result types:
 * - Result::OK: The event was processed in some way. Subtypes may 
 *   give more details.
 * - Result::REJECTED: The event was rejected and should be resent in the next 
 *   scheduler cycle. The Subtype may give the reason for rejecting the event.
 * 
 * @see Type Enumeration of primary result types (OK, REJECTED)
 * @see Subtype Enumeration of additional context (UNDEFINED, NOT_CONNECTED)
 */

Result Result::OK = { Type::OK, 0 };
Result Result::OK__NOT_HANDLED = { Type::OK, Subtype::NOT_HANDLED };
Result Result::OK__NOT_CONNECTED = { Type::OK, Subtype::NOT_CONNECTED };
Result Result::REJECTED = { Type::REJECTED, 0 };
Result Result::REJECTED__NOT_CONNECTED = { Type::REJECTED, Subtype::NOT_CONNECTED };
