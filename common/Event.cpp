//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Event.h"

using namespace nd;

/**
 * Store the time since the last event in milliseconds with the event.alignas
 * 
 * \note It's binary mSec as 1024 uSecs in a mSec. Dividing is expensive.alignas
 * 
 * \param[inout] event update the event time in this event
 * \param[in] uSecs time delta in microseconds 
 * 
 * \see pico/time.h: absolute_time_diff_us()
 */
void nd::event_delta_t(Event event, int64_t uSecs) {
    int64_t mSecs = uSecs >> 10; // divide by 1024
    if (mSecs > 0xffff) mSecs = 0xffff;
    event.delta_t = static_cast<uint16_t>(mSecs);
}