//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PIPES_MNP_THROTTLE_H
#define ND_PIPES_MNP_THROTTLE_H

#include "../Pipe.h"

#include <vector>

namespace nd {

class MNPThrottle: public Pipe {
    enum class State: uint8_t {
        WAIT_FOR_DLE,
        WAIT_FOR_ETX,
        WAIT_FOR_CRC_lo,
        WAIT_FOR_CRC_hi,
        RESEND_DELAY
    };
    State state_ = State::WAIT_FOR_DLE;
    Event resend_event_;
    uint32_t bitrate_ = 38400;
public:
    MNPThrottle() = default;
    ~MNPThrottle() override = default;

    Result send(Event event) override;

    static uint32_t reg_absolute_delay;
    static uint32_t reg_num_char_delay;
}; 


} // namespace nd

#endif // ND_PIPES_MNP_THROTTLE_H