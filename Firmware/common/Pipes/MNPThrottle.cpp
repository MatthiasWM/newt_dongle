//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "MNPThrottle.h"

#include <cstdint>

using namespace nd;

/**
 * @class MNPThrottle
 * @brief Throttle MNP data to the `out` pipe.
 * 
 * The Newton can't deal with MNP blocks that come too quickly and eventually
 * hangs the connection. Resyncing does not work, and the whole transfer must
 * be restarted.
 * 
 * The Throttle pipe watches the data stream for the end signature of the 
 * MNP block and inserts a short delay after the last byte of the block.
 * This should give the Newton enough time to recover. 
 * 
 * The start of a block is SYN, DLE, STX. The of the block is DLE, ETX, 
 * CRC_lo, and CRC_hi. If the data stream contains a DLE, it is escaped
 * with another DLE.
 * 
 * SYN: 0x16, "synchronous idle"
 * DLE: 0x10, "data link escape"
 * STX: 0x02, "start of text"
 * ETX: 0x03, "end of text"
 * LA:  0x05, "logical address"
 * LT:  0x04, "logical type"
 */

 #include "MNPThrottle.h"

 #include "main.h"

 #include <stdio.h>

constexpr uint8_t kDLE = 0x10;
constexpr uint8_t kETX = 0x03;

MNPThrottle::MNPThrottle(Scheduler &scheduler) 
:   Task(scheduler, Scheduler::TASKS | Scheduler::SIGNALS) 
{
} 

Result MNPThrottle::send(Event event) 
{
    if (state_ == State::RESEND_DELAY) {
        if (Pipe::send(resend_event_).ok()) {
            if (kDebugMNPThrottle) printf("Delay resent ok\n");
            state_ = State::WAIT_FOR_DLE;
        } else {
            if (kDebugMNPThrottle) printf("Delay rejected again\n");
            return Result::REJECTED;
        }
    }

    if (event.type() == Event::Type::SET_BITRATE) {
        bitrate_ = event.bitrate();
    }

    Result r = Pipe::send(event);
    if (r.rejected())
        return r;

    if (event.type() == Event::Type::DATA) {
        switch (state_) {
            case State::WAIT_FOR_DLE:
                if (event.data() == kDLE) {
                    if (kDebugMNPThrottle) printf("DLE ");
                    state_ = State::WAIT_FOR_ETX;
                }
                break;
            case State::WAIT_FOR_ETX:
                if (event.data() == kETX) {
                    if (kDebugMNPThrottle) printf("ETX ");
                    state_ = State::WAIT_FOR_CRC_lo;
                } else {
                    if (kDebugMNPThrottle) printf("esc ");
                    state_ = State::WAIT_FOR_DLE;
                }
                break;
            case State::WAIT_FOR_CRC_lo:
                if (kDebugMNPThrottle) printf("CRC ");
                state_ = State::WAIT_FOR_CRC_hi;
                break;
            case State::WAIT_FOR_CRC_hi:
                if (kDebugMNPThrottle) printf("CRC ");
                // Set the delay with a fixed value plus a number of characters at the curren bitrate.
                // Hayes `ATS300`: absolute throttle delay in microseconds
                // Hayes `ATS301`: relative MNP throttle delay in characters
                resend_event_ = Event::make_delay_event(reg_absolute_delay_ + ((reg_num_char_delay_ * 1'000'000) / bitrate_) * 10);
                if (Pipe::send(resend_event_).ok()) {
                    if (kDebugMNPThrottle) printf("Delay added\n");
                    state_ = State::WAIT_FOR_DLE;
                } else {
                    if (kDebugMNPThrottle) printf("Delay rejected\n");
                    state_ = State::RESEND_DELAY;
                }
                break;
        }
    }
    return r;
}

Result MNPThrottle::signal(Event event) {
    uint8_t value = 0;
    if (event.type() != Event::Type::SIGNAL)
        return Result::OK;
    switch (event.subtype()) {
        case Event::Subtype::USER_SETTINGS_CHANGED:
            reg_absolute_delay_ = user_settings.data.mnpt_absolute_delay;
            reg_num_char_delay_ = user_settings.data.mnpt_num_char_delay;
            break;
    }
    return Result::OK;
}


