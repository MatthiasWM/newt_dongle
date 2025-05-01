//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Scheduler.h"

#include "Task.h"

using namespace nd;

/** \class Scheduler 
 * 
 */

Scheduler &Scheduler::add(Task &task) {
    spoke_list_.push_front(&task);
    return *this;
}

void Scheduler::init() {
    for (auto &task : spoke_list_) {
        task->init();
    }
}

void Scheduler::run(int n) {
    for (; (n == -1) || (n > 0); ) {
        for (auto &task : spoke_list_) {
            task->task();
        }
        ticks_++;
        if (n > 0) --n;
    }
}
 