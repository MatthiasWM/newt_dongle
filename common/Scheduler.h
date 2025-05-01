//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_WHEEL_H
#define ND_WHEEL_H

#include "nd_config.h"

#include <forward_list>

namespace nd {

class Task;

/** \brief Call the tasks of all registered Endpoints in an endless loop. */
class Scheduler {
    std::forward_list<Task*> spoke_list_;
    uint32_t ticks_ = 0;
public:
    Scheduler() = default;
    ~Scheduler() = default;
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler(Scheduler&&) = delete;
    Scheduler& operator=(Scheduler&&) = delete;

    // -- Add a Task to the scheduler
    Scheduler &add(Task &task);

    // -- Spin the scheduler
    void init();
    void run(int n=-1);

    // -- Additionl scheduler features
    uint32_t ticks() const { return ticks_; }
};

} // namespace nd

#endif // ND_WHEEL_H