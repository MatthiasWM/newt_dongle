//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Device.h"

using namespace nd;

Result Device::init(Wheel &wheel) {
    wheel_ = &wheel;
    return Result::OK;
}
