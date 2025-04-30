//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "TestDataDevice.h"

using namespace nd;

Result TestDataDevice::init() {
    return Result::OK;
}

Result TestDataDevice::release() {
    return Device::release();
}

Result TestDataDevice::task() {
    if (index_ < 128) {
        uint8_t data = (index_ & 0x0f) + 'A';
        if (send(Event(data)).ok()) {
            index_++;
        }
    } else {
        send(Event('.'));
    }
    return Device::task();
}

Result TestDataDevice::stop() {
    return Device::stop();
}


