//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PosixScheduler.h"


using namespace nd;


PosixScheduler::~PosixScheduler() {
    // Destructor implementation if needed
}


void PosixScheduler::update_time() {
    // if (pico_start_time_ == nil_time) {
    //     pico_start_time_ = get_absolute_time();
    //     pico_prev_cycle_ = pico_start_time_;
    // }
    // absolute_time_t now = get_absolute_time();
    // cycle_time_ = absolute_time_diff_us(pico_prev_cycle_, now);
    // if (cycle_time_ == 0)
    //     cycle_time_ = 1;
    // pico_prev_cycle_ = now;
}
