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
 * @brief Handle an event that was sent to this endpoint.alignas
 * 
 * This method will be overridden by the derived class to handle the event.
 * The default implementation does nothing and returns Result::OK to indicate
 * that the event reached the endpoint.
 */
Result Endpoint::send(Event event) {
    return Result(Result::Type::OK, Result::Subtype::UNKNOWN);
}

/**
 * @brief Handle an event that was sent to this endpoint with high priority.
 * 
 * This method will be overridden by the derived class to handle the event.
 * The default implementation does nothing and returns Result::OK to indicate
 * that the event reached the endpoint.
 */
Result Endpoint::rush(Event event) {
    return Result(Result::Type::OK, Result::Subtype::UNKNOWN);
}


