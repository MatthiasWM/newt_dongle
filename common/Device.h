//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_DEVICE_H
#define ND_DEVICE_H

#include "nd_config.h"
#include "common/Pipe.h"
#include "common/Spoke.h"

namespace nd {

class Wheel;

class Device: public Pipe, public Spoke {
public:
    Device(Wheel &wheel) : Spoke(wheel) { }
    virtual ~Device() = default;
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;
};

} // namespace nd

#endif // ND_DEVICE_H