//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_SYSTEM_TASK_H
#define ND_PICO_SYSTEM_TASK_H

#include "common/SystemTask.h"

namespace nd {

class PicoSystemTask: public SystemTask {
public:
    PicoSystemTask(Scheduler &scheduler) : SystemTask(scheduler) { }
    Result task() override;
    void reset_hardware();
};

} // namespace nd

#endif // ND_PICO_SYSTEM_TASK_H