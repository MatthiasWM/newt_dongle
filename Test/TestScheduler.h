//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_TEST_SCHEDULER_H
#define ND_TEST_SCHEDULER_H

#include "common/Scheduler.h"

namespace nd {

class TestScheduler : public Scheduler{
    uint64_t start_time_ = 0;
    uint64_t prev_cycle_ = 0;
protected:
    void update_time() override;
public:
    TestScheduler() = default;
};

} // namespace nd

#endif // ND_TEST_SCHEDULER_H