//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_SPOKE_H
#define ND_SPOKE_H

#include "nd_config.h"
#include "common/Pipe.h"

namespace nd {

class Wheel;

class Spoke {
    Wheel &wheel_;
public:
    Spoke(Wheel &wheel);
    virtual ~Spoke() = default;
    Spoke(const Spoke&) = delete;
    Spoke& operator=(const Spoke&) = delete;
    Spoke(Spoke&&) = delete;
    Spoke& operator=(Spoke&&) = delete;

    Wheel &wheel() { return wheel_; }
    virtual Result init() { return Result::OK; }
    virtual Result task() { return Result::OK; }
    virtual Result release() { return Result::OK; }
    virtual Result stop() { return Result::OK; }
};

} // namespace nd

#endif // ND_SPOKE_H