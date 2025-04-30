//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Wheel.h"

#include "Device.h"

using namespace nd;

/** \class Wheel 
 * 
 */

Wheel &Wheel::add(Device &device) {
    device_list_.push_front(&device);
    return *this;
}

void Wheel::init() {
    for (auto &device : device_list_) {
        device->init(*this);
    }
}

void Wheel::release() {
    for (auto &device : device_list_) {
        device->release();
    }
}

void Wheel::spin(int n) {
    int i = 0;
    for (;;) {
        for (auto &device : device_list_) {
            device->task();
        }
        ticks_++;
        if (n > 0) {
            i++;
            if (i >= n) {
                break;
            }
        }
    }
}

void Wheel::pause() {
    for (auto &device : device_list_) {
        device->stop();
    }
}
 