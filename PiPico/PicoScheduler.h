//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_SCHEDULER_H
#define ND_PICO_SCHEDULER_H

#include "common/Scheduler.h"

#include "pico/time.h"

namespace nd {

class PicoScheduler : public Scheduler{
    absolute_time_t pico_start_time_ = nil_time;
    absolute_time_t pico_prev_cycle_ = nil_time;
protected:
    void update_time() override;
public:
    PicoScheduler() = default;
};

} // namespace nd

#endif // ND_PICO_SCHEDULER_H