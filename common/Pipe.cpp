//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Pipe.h"
#include "common/Endpoint.h"

#include <stdio.h>

using namespace nd;

/**
 * \class Pipe The Pipe class manages a stream of events.
 */

// -- Pipe Ponstruction --------------------------------------------------------

/**
 * Creat a pipe that can be linked between two endpoints.
 */
Pipe::Pipe() {
    prev_event_time_ = get_absolute_time();
}

// -- Pipe Routing -------------------------------------------------------------

/**
 * \brief Set the source endpoint.
 * \param endpoint The source endpoint
 * \return self, so we can stack calls
 */
Pipe &Pipe::connect_from(Endpoint &endpoint) {
    endpoint.set_out(this);
    source_ = &endpoint;
    prev_event_time_ = get_absolute_time();
    return *this;
}

/**
 * \brief Set the target endpoint.
 * \param endpoint The target endpoint
 * \return self, so we can stack calls
 */
Pipe &Pipe::connect_to(Endpoint &endpoint) {
    endpoint.set_in(this);
    target_ = &endpoint;
    prev_event_time_ = get_absolute_time();
    return *this;
}

/**
 * \brief Disconnect the pipe from the source and sink endpoints.
 * 
 * The pipe can be disconnected from the source and sink endpoints.
 * The endpoints will no longer be able to stuff data or read data
 * from the pipe.
 * 
 * \return self, so we can stack calls
 */
Pipe &Pipe::disconnect() {
    if (source_) {
        source_->set_out(nullptr);
        source_ = nullptr;
    }
    if (target_) {
        target_->set_in(nullptr);
        target_ = nullptr;
    }
    return *this;
}

// -- Reading the Pipe ---------------------------------------------------------

/**
 * \brief Return the number of events available in the pipe.
 * 
 * \return the number of events available in the pipe
 */
int32_t Pipe::num_avail() const {
    return (head_ - tail_) & cfgRingBufferMask;
}

/**
 * \brief Peek at the first event in the pipe.
 * \return the first event in the pipe
 * \return an error event with PIPE_EMPTY if there are no events in the pipe
 */
Event Pipe::peek() const {
    if (head_ == tail_) {
        return make_event(Type::ERROR, static_cast<uint8_t>(Error::PIPE_EMPTY));
    }
    return buffer_[tail_];
}

/**
 * \brief Get the first event from the pipe.
 * 
 * The event is not removed from the pipe.
 * 
 * \return the first event in the pipe
 * \return an error event with PIPE_EMPTY if there are no events in the pipe
 */
Event Pipe::read() {
    Event e = peek();
    if (e.type != Type::ERROR) pop();
    return e;
}

/**
 * \brief Remove the first event from the pipe.
 */
void Pipe::pop() {
    if (head_ == tail_) {
        puts("**** ERROR **** in Pipe::pop() - no event to pop");
        return;
    }
    tail_ = (tail_ + 1) & cfgRingBufferMask;
}

// -- Writing the Pipe

/**
 * \brief Return the number of free events in the pipe.
 * 
 * \return the number of free events in the pipe
 */
int32_t Pipe::num_free() const {
    return cfgRingBufferSize - num_avail() - 1;
}

/**
 * \brief Write an event into the pipe.
 * 
 * The event is added to the end of the pipe. If the pipe is full,
 * the PIPE_FULL error code is returned.
 * 
 * \return Error::OK or PIPE_FULL
 */
Error Pipe::write(Event event) {  
    // -- Check if we have room for a new event
    auto new_head = (head_ + 1) & cfgRingBufferMask;
    if (new_head == tail_) {
        return Error::PIPE_FULL;
    }
    // -- Calculate the time since the last event
    absolute_time_t now = get_absolute_time();
    int64_t delta_t_us = absolute_time_diff_us(prev_event_time_, now);
    event_delta_t(event, delta_t_us);
    prev_event_time_ = now;
    // -- push the event into the pipe
    buffer_[head_] = event;
    head_ = new_head;
    return Error::OK;
}

/**
 * \brief Prepend an event to the pipe, so it is read next.
 * 
 * This is used for debugging only. It will not be used in the final
 * implementation.
 * 
 * \note we can make this universal if we always keep two or so event slots
 *      free. This would always allow us to prepend events.
 * 
 * \return Error::OK or PIPE_FULL
 */
Error Pipe::prepend(Event event) {
    if (num_free() < 1) {
        return Error::PIPE_FULL;
    }
    auto new_tail = (tail_ - 1) & cfgRingBufferMask;
    buffer_[new_tail] = event;
    tail_ = new_tail;
    return Error::OK;
}

// -- Buffer Management --------------------------------------------------------

/**
 * \brief Check if the pipe is above the high water mark.
 * \return true when the pipe fills up and the enpoint may want to clear CTS
 */
bool Pipe::reached_high_water() const {
    return (num_free() < cfgRingBufferHighWater);
}

/**
 * \brief Flush the pipe.
 * 
 * The pipe is flushed. All events are removed from the pipe.
 */
void Pipe::flush() {
    head_ = 0;
    tail_ = 0;
}

