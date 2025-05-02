//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "common/Pipe.h"

#include <cassert>

using namespace nd;

/**
 * @class Pipe
 * @brief Represents a pipeline component that can route events to another pipe.
 * 
 * The Pipe class implements a pipeline pattern for event processing. Pipes connect
 * a sending Endpoint with a receiving Endpoint, They can be chained using 
 * the '>>' operator, forming a directed graph of event flow.
 * Events are sent through the pipeline with either normal priority (send) or high 
 * priority (rush) for immediate processing.
 *
 * Pipes form the foundation of the event-driven architecture, allowing modular
 * components to be combined while maintaining separation of concerns.
 */

/**
 * @brief Connects this pipe to another pipe for data flow.
 * 
 * This operator allows pipes to be chained together in a stream-like manner.
 * The current pipe's output is directed to the provided pipe.
 * 
 * @param pipe The destination pipe to receive output from this pipe
 * @return A reference to the destination pipe, allowing for method chaining
 */
Pipe &Pipe::operator>>(Pipe &pipe) {
    assert(active_); // The output of this pipe is not active. You can't connect a pipe to it.
    out_ = &pipe;
    return pipe;
}

/**
 * @brief Gets the output pipe connected to this pipe
 * 
 * @return Pipe* Pointer to the output pipe, or nullptr if no output is connected
 */
Pipe *Pipe::out() const {
    return out_;
}

/**
 * @brief Forwards an event to the output destination if one exists.
 * 
 * This method forwards the provided event to the pipe's output destination.
 * Pipes derived from this class can override this method to implement custom
 * behavior for event processing, e.g. event buffering. The return value
 * indicates that the event was sent further down the pipeline, but it does
 * not guarantee that the event was processed by the Endpoint.
 * 
 * @param event The event to be sent through the pipe
 * @return return the result from the output's send method.
 * @return if no output pipe is connected, the Event is discarded and we 
 *         return Result::OK with the Subtype NOT_CONNECTED.
*/
Result Pipe::send(Event event) {
    if (out_) {
        return out_->send(event);
    } else {
        return Result::OK__NOT_CONNECTED;
    }
}

/**
 * @brief Quickly forwards an event to the pipe's output destination.
 *
 * Rush an event past the queue of events further down the pipeline all the way
 * to the Endpoint. As opposed to `send()`, the return value of this method is 
 * also the immediate reply to the event.
 *
 * @param event The event to be forwarded
 * @return return the result from the output's rush method.
 * @return if no output pipe is connected, delivery was not possible, and we
 *        return Result::REJECTED with the Subtype NOT_CONNECTED.
 */
Result Pipe::rush(Event event) {
    if (out_) {
        return out_->send(event);
    } else {
        return Result::OK__NOT_CONNECTED;
    }
}

