//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "test.h"

#include "common/Scheduler.h"

#include <vector>

using namespace nd;

namespace {

// Plays back a list of cycle times.
class ScriptedScheduler : public Scheduler {
    std::vector<uint32_t> cycles_;
    size_t ix_ = 0;
public:
    ScriptedScheduler(std::vector<uint32_t> cycles) : cycles_(cycles) { }
protected:
    void update_time() override { cycle_time_ = cycles_[ix_++ % cycles_.size()]; }
};

} // namespace

TEST(scheduler_tracks_longest_cycle) {
    ScriptedScheduler scheduler({ 100, 2500, 300 });
    scheduler.run(3);
    CHECK(scheduler.max_cycle_time() == 2500);
    CHECK(scheduler.now_us() == 2900);

    scheduler.reset_max_cycle_time();
    scheduler.run(1); // 100
    CHECK(scheduler.max_cycle_time() == 100);
}
