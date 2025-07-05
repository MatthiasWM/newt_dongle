//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_SYSTEM_TASK_H
#define ND_SYSTEM_TASK_H

#include "common/Task.h"

namespace nd {

class SystemTask: public nd::Task {
public:
    SystemTask(Scheduler &scheduler);
};

} // namespace nd

#endif // ND_SYSTEM_TASK_H