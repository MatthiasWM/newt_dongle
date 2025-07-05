
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_POSIX_SYSTEM_TASK_H
#define ND_POSIX_SYSTEM_TASK_H

#include "common/SystemTask.h"

namespace nd {

class PosixSystemTask: public SystemTask {
public:
    PosixSystemTask(Scheduler &scheduler);
};

} // namespace nd

#endif // ND_POSIX_SYSTEM_TASK_H