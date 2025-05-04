//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Endpoint.h"

using namespace nd;

/**
 * @brief Base class for defining endpoints within a task scheduling system.
 * 
 * The Endpoint class extends the Task class to provide a foundation for implementing
 * specific endpoint functionality. Endpoints are used as the start and end
 * points of a pipe. They often represent a hardware device on the host 
 * microcontroller, such as a UART or USB device.
 * 
 * Incoming events end here and are handled by the endpoint.
 * Outgoing events originate here and are sent to the out pipe.
 * There is no connection between the `send()` method and the `out()` pipe.
 * 
 * @see Task
 * @see Scheduler
 */

/**
 * @brief Handle an event that was sent form `in`.
 * 
 * This method overrides Pipe::send(). Enpoints are the final destination for
 * events. Known events are handled here and return OK, or REJECTED and an 
 * error code.
 * 
 * If the event is unknown to this endpoint, OK__NOT_HANDLED is returned, 
 * giving the `send()` method in a derived class a chance to handle this event.
 */
Result Endpoint::send(Event event) {
    switch (event.type()) {
        case Event::Type::DELAY:
            if (event.subtype() == Event::Subtype::USECS) {
                delay(event.data(), 0);
            } else if (event.subtype() == Event::Subtype::MSECS) {
                delay(event.data() * 1000, 0);
            } else if (event.subtype() == Event::Subtype::CHARS) {
                delay(0, event.data());
            }
            return Result::OK;
    }
    return Result::OK__NOT_HANDLED;
}

/**
 * @brief Handle an event that was sent to this endpoint with high priority.
 * 
 * This method will be overridden by the derived class to handle the event.
 * The default implementation does nothing and returns Result::OK to indicate
 * that the event reached the endpoint.
 */
Result Endpoint::rush(Event event) {
    return Result(Result::Type::OK, Result::Subtype::NOT_HANDLED);
}

/**
 * @brief Handle an event that was sent from `out`.
 */
Result Endpoint::rush_back(Event event) {
    if (event.type() == Event::Type::HIGH_WATER) {
        if (event.subtype() == Event::Subtype::ON) {
            high_water_count_++;
            if (high_water_count_ == 1) set_high_water(true);
        } else if (event.subtype() == Event::Subtype::OFF) {
            high_water_count_--;
            if (high_water_count_ == 0) set_high_water(false);  
        } else if (event.subtype() == Event::Subtype::RESET) {
            high_water_count_ = 0;
            set_high_water(false);
        } 
    }
    return Result::OK__NOT_HANDLED;
}


