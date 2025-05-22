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

Scheduler &Scheduler::add(Task &task, uint8_t job_map) {
    if (job_map & TASKS)
        task_list_.push_front(&task);
    if (job_map & SIGNALS)
        signal_list_.push_front(&task);
    return *this;
}

void Scheduler::init() {
    for (auto &task : task_list_) {
        task->init();
    }
}

void Scheduler::run(int n) {
    for (; (n == -1) || (n > 0); ) {
        update_time();
        // -- Call all registered tasks
        for (auto &task : task_list_) {
            task->task();
        }
        // -- Forward all signals to registered tasks
        while (!signal_queue_.empty()) {
            Event event = signal_queue_.front();
            signal_queue_.pop();
            for (auto &task : signal_list_) {
                task->signal(event);
            }
        }
        // -- Update the time and the ticks
        ticks_++;
        if (n > 0) --n;
    }
}

uint32_t Scheduler::cycle_time() const {
    return cycle_time_;
}

void Scheduler::signal_all(Event event) {
    signal_queue_.push(event);
}