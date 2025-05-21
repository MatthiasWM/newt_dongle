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
    pipe.in_ = this;
    return pipe;
}

/**
 * @brief Gets the pipe connected to this pipe on the input side.
 * 
 * @return Pipe* Pointer to the output pipe, or nullptr if no output is connected
 */
Pipe *Pipe::in() const {
    return in_;
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
 * @brief Rush an event from `in` to out and give an immediate reply.
 *
 * Rush an event past the queue of events further down the pipeline all the way
 * to the Endpoint. As opposed to `send()`, the return value of this method is 
 * also the immediate reply to the event.
 *
 * @param event The event to be forwarded
 * @return the result from the output's `rush()` method or `OK__NOT_CONNECTED`.
 */
Result Pipe::rush(Event event) {
    if (out_) {
        return out_->rush(event);
    } else {
        return Result::OK__NOT_CONNECTED;
    }
}

/**
 * @brief Rush an event from `out` upstream to `in` and give an immediate reply.
 *
 * @param event The event to be forwarded
 * @return the result from the input's `rush_back()` method or `OK__NOT_CONNECTED`.
 */
Result Pipe::rush_back(Event event) {
    if (in_) {
        return in_->rush_back(event);
    } else {
        return Result::OK__NOT_CONNECTED;
    }
}

/**
 * @brief Send a string of text to the next pipe in the pipeline.
 * 
 * This method sends a string of characters to the next pipe in the pipeline.
 * It is a convenience method that calls `send()` for each character in the string.
 * 
 * @param str The string to be sent
 */
Result Pipe::send_text(const char* str) {
    if (!out_) 
        return Result::OK__NOT_CONNECTED;
    if (!str || !*str) 
        return Result::OK;

    Result res = Result::OK;
    while (*str) {
        res = send(Event(*str++));
        if (!res.ok()) return res;
    }
    return res;
}   