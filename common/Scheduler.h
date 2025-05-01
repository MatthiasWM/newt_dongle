//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_SCHEDULER_H
#define ND_SCHEDULER_H

#include "nd_config.h"

#include <forward_list>

namespace nd {

class Task;

/** \brief Call the tasks of all registered Endpoints in an endless loop. */
class Scheduler 
{
    std::forward_list<Task*> spoke_list_;

protected:
    uint32_t ticks_ = 0;
    uint32_t cycle_time_ = 1;
    virtual void update_time() = 0;
    
public:
    Scheduler() = default;
    virtual ~Scheduler() = default;
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler(Scheduler&&) = delete;
    Scheduler& operator=(Scheduler&&) = delete;

    // -- Add a Task to the scheduler
    Scheduler &add(Task &task);

    // -- Spin the scheduler
    void init();
    void run(int n=-1);

    // -- Additional scheduler features
    uint32_t ticks() const { return ticks_; }
    uint32_t cycle_time() const;
};

} // namespace nd

#endif // ND_SCHEDULER_H