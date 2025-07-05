//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_POSIX_SCHEDULER_H
#define ND_POSIX_SCHEDULER_H

#include "common/Scheduler.h"

namespace nd {

class PosixScheduler : public Scheduler{
    // absolute_time_t pico_start_time_ = nil_time;
    // absolute_time_t pico_prev_cycle_ = nil_time;
protected:
    void update_time() override;
public:
    PosixScheduler() = default;
    ~PosixScheduler() override;
};

} // namespace nd

#endif // ND_POSIX_SCHEDULER_H