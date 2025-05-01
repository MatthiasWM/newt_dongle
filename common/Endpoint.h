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

class Endpoint: public Pipe, public Spoke {
public:
    Endpoint(Wheel &wheel) : Spoke(wheel) { }
    virtual ~Endpoint() = default;
    Endpoint(const Endpoint&) = delete;
    Endpoint& operator=(const Endpoint&) = delete;
    Endpoint(Endpoint&&) = delete;
    Endpoint& operator=(Endpoint&&) = delete;
};

} // namespace nd

#endif // ND_DEVICE_H