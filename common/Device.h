//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_DEVICE_H
#define ND_DEVICE_H

#include "nd_config.h"
#include "common/Pipe.h"

namespace nd {

class Wheel;

class Device: public Pipe {
    Wheel *wheel_ = nullptr;
public:
    Device() = default;
    virtual ~Device() = default;
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    // -- Initialization
    virtual Result init(Wheel &Wheel);
    virtual Result release() { return Result::OK; }
    virtual Result task() { return Result::OK; }
    virtual Result stop() { return Result::OK; }
};

} // namespace nd

#endif // ND_DEVICE_H