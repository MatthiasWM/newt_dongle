//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_SYSTEM_H
#define ND_PICO_SYSTEM_H

#include "common/Endpoints/StdioLog.h"

namespace nd {

class PicoSystemTask: public nd::Task {
public:
    PicoSystemTask(Scheduler &scheduler) : Task(scheduler) { }
    Result task() override;
};

} // namespace nd

#endif // ND_PICO_SYSTEM_H