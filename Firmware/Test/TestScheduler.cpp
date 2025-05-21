//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "TestScheduler.h"

#include <stdio.h>
#include <sys/time.h>

using namespace nd;

void TestScheduler::update_time() {
    struct timeval tv;
    if (start_time_ == 0) {
        gettimeofday(&tv, NULL);
        start_time_ = tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
        prev_cycle_ = start_time_;
    }
    gettimeofday(&tv, NULL);
    uint64_t now = tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
    cycle_time_ = now - prev_cycle_;
    if (cycle_time_ == 0)
        cycle_time_ = 1;
    prev_cycle_ = now;
}


