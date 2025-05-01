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

void Wheel::spin(int n) {
    for (; (n == -1) || (n > 0); ) {
        for (auto &spoke : spoke_list_) {
            spoke->task();
        }
        ticks_++;
        if (n > 0) --n;
    }
}
 