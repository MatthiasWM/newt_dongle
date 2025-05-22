//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_SCHEDULER_H
#define ND_SCHEDULER_H

#include "Event.h"

#include <forward_list>
#include <queue>
#include <cstdint>

namespace nd {

class Task;

/** \brief Call the tasks of all registered Endpoints in an endless loop. */
class Scheduler 
{
    std::forward_list<Task*> task_list_;
    std::forward_list<Task*> signal_list_;
    std::queue<Event> signal_queue_;

protected:
    uint32_t ticks_ = 0;
    uint32_t cycle_time_ = 1;
    virtual void update_time() = 0;
    
public:
    constexpr static uint8_t TASKS = 0x01;
    constexpr static uint8_t SIGNALS = 0x02;

    Scheduler() = default;
    virtual ~Scheduler() = default;
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler(Scheduler&&) = delete;
    Scheduler& operator=(Scheduler&&) = delete;

    // -- Add a Task to the scheduler
    Scheduler &add(Task &task, uint8_t job_map);

    // -- Spin the scheduler
    void init();
    void run(int n=-1);

    // -- Let user send a signal to all tasks.
    void signal_all(Event event);

    // -- Additional scheduler features
    uint32_t ticks() const { return ticks_; }
    uint32_t cycle_time() const;
};

} // namespace nd

#endif // ND_SCHEDULER_H