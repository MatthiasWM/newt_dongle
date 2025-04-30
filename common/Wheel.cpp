//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Wheel.h"

#include "Spoke.h"

using namespace nd;

/** \class Wheel 
 * 
 */

Wheel &Wheel::add(Spoke &spoke) {
    spoke_list_.push_front(&spoke);
    return *this;
}

void Wheel::init() {
    for (auto &spoke : spoke_list_) {
        spoke->init();
    }
}

void Wheel::release() {
    for (auto &spoke : spoke_list_) {
        spoke->release();
    }
}

void Wheel::spin(int n) {
    int i = 0;
    for (;;) {
        for (auto &spoke : spoke_list_) {
            spoke->task();
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
    for (auto &spoke : spoke_list_) {
        spoke->stop();
    }
}
 