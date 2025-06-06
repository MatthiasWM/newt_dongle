//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_TASK_H
#define ND_TASK_H

#include "common/Pipe.h"
#include "common/Scheduler.h"

namespace nd {

class Scheduler;

class Task : public Pipe {
    Scheduler &scheduler_;
public:
    Task(Scheduler &scheduler, uint8_t job_map = Scheduler::TASKS);
    virtual ~Task() = default;
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&&) = delete;
    Task& operator=(Task&&) = delete;

    Scheduler &scheduler() const { return scheduler_; }
    virtual Result init() { return Result::OK; }
    virtual Result task() { return Result::OK; }
    virtual Result signal(Event event) { return Result::OK; }
};

} // namespace nd

#endif // ND_TASK_H