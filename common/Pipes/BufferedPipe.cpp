//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "BufferedPipe.h"

using namespace nd;

/**
 * @brief A buffered pipe.
 * 
 * BufferedPipe implements a ring buffer for Event objects, allowing events to be
 * queued and processed asynchronously. It inherits from Task, enabling it to be
 * scheduled for execution within a Scheduler.
 * 
 * The internal buffer is implemented as a power-of-2 sized ring buffer for
 * efficient circular operation using bit masking.
 * 
 * @note Buffer size is specified as a power of 2 (default is 2^9 = 512 elements)
 */


BufferedPipe::BufferedPipe(Scheduler &scheduler, uint8_t buffer_size_pow2)
:   Task { scheduler },
    ring_size_ { static_cast<uint32_t>(1 << buffer_size_pow2) },
    ring_mask_ { ring_size_ - 1 },
    head_ { 0 },
    tail_ { 0 },
    high_water_on_mark { ring_size_/16 },       // default is 32 for a 512 event buffer
    high_water_off_mark_ { ring_size_/32*3 }    // defaults to 48 for a 512 event buffer
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

    if (high_water_mark_set_) {
        if (space() > high_water_off_mark_) {
            Event event { Event::Type::HIGH_WATER, 0 };
            r = out()->rush_back(event);
            high_water_mark_set_ = false;
        }
    } else {
        if (space() < high_water_on_mark) {
            Event event { Event::Type::HIGH_WATER, 1 };
            r = out()->rush_back(event);
            high_water_mark_set_ = true;
        }
    }
        
    return r;
}

// -- Pipe Stuff
void BufferedPipe::push_back(Event event) {
    buffer_[head_] = event;
    head_ = (head_ + 1) & ring_mask_;
    if (!high_water_mark_set_) {
        if (space() < high_water_on_mark) {
            Event event { Event::Type::HIGH_WATER, 1 };
            out()->rush_back(event);
            high_water_mark_set_ = true;
        }
    }

}

Event BufferedPipe::pop_front() {
    Event event = buffer_[tail_];
    tail_ = (tail_ + 1) & ring_mask_;
    if (high_water_mark_set_) {
        if (space() > high_water_off_mark_) {
            Event event { Event::Type::HIGH_WATER, 0 };
            out()->rush_back(event);
            high_water_mark_set_ = false;
        }
    }
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

uint32_t BufferedPipe::space() const {
    return (head_ - tail_) & ring_mask_;
}


Result BufferedPipe::send(Event event) 
{
    if (out() == nullptr)
        return Result::OK__NOT_CONNECTED;

    if (is_empty()) {
        Result r = out()->send(event);
        if (r.ok())
            return r;
        // Continue and buffer the current event
    } else {
        Event buffered_event = peek_front();
        Result r = out()->send(buffered_event);
        if (r.ok())
            pop_front();
        // Continue and buffer the current event
    }
    if (is_full()) {
        return Result::REJECTED;
    } else {
        push_back(event);
        return Result::OK;
    }
}

