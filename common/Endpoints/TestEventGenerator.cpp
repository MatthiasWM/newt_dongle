//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "TestEventGenerator.h"

using namespace nd;

Result TestEventGenerator::init() {
    return Result::OK;
}

Result TestEventGenerator::task() {
    if (index_ < 128) {
        uint8_t data = (index_ & 0x0f) + 'A';
        if (send(Event(data)).ok()) {
            index_++;
        }
    } else {
        send(Event('.'));
    }
    return Endpoint::task();
}

