//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "BufferedPipe.h"

using namespace nd;

BufferedPipe::BufferedPipe(Scheduler &scheduler, uint8_t buffer_size_pow2)
:   Task { scheduler },
    ring_size_ { static_cast<uint32_t>(1 << buffer_size_pow2) },
    ring_mask_ { ring_size_ - 1 },
    head_ { 0 },
    tail_ { 0 }
{
    buffer_.resize(ring_size_);
}

// -- Task Stuff

Result BufferedPipe::task() {
    if (is_empty())
        return Result::OK;

    Event buffered_event = peek_front();
    Result r = out()->send(buffered_event);
    if (r.ok())
        pop_front();
        
    return r;
}

// -- Pipe Stuff
void BufferedPipe::push_back(Event event) {
    buffer_[head_] = event;
    head_ = (head_ + 1) & ring_mask_;
}

Event BufferedPipe::pop_front() {
    Event event = buffer_[tail_];
    tail_ = (tail_ + 1) & ring_mask_;
    return event;
}

Event BufferedPipe::peek_front() {
    return buffer_[tail_];
}

bool BufferedPipe::is_full() const {
    return ((head_ + 1) & ring_mask_) == tail_;
}

bool BufferedPipe::is_empty() const {
    return head_ == tail_;
}

bool BufferedPipe::is_high_water_mark() const {
    return false; // TODO: write me
}


Result BufferedPipe::send(Event event) {   
    // TODO: continue here
    // Return in subytype if the high water mark is reached
    // Implement the delay functionality
    // Rename the new Device class back to Endpoint
    // Serialize the Log Endpoint to run better on Pico (just buffer it?)
    // Better Log output
    // Implement the UART endpoint API
    // (Re)Implement the Pico UART and Pico CDC classes


    Result r = Result::REJECTED;
    if (out() == nullptr)
        return Result::REJECTED;

    if (is_empty()) {
        r = out()->send(event);
        if (r.ok()) {
            return r;
        }
    } else {
        Event buffered_event = peek_front();
        r = out()->send(buffered_event);
        if (r.ok()) {
            pop_front();
            // Fall through: We still need to push the new event to the buffer
        }
    }
    if (is_full()) {
        return Result::REJECTED;
    } else {
        push_back(event);
        return Result::OK;
    }
}

Result BufferedPipe::rush(Event event) {
    // TODO: continue here
    return Pipe::rush(event);
}
