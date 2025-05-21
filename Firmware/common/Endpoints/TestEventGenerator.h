//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_ENDPOINTS_TEST_EVENT_GENERATOR_H
#define ND_ENDPOINTS_TEST_EVENT_GENERATOR_H

#include "common/Endpoint.h"

namespace nd {

class TestEventGenerator: public Endpoint {
    uint32_t index_ = 0;
public:
    TestEventGenerator(Scheduler &scheduler) : Endpoint(scheduler) {}
    ~TestEventGenerator() override = default;

    // -- Initialization
    Result init() override;
    Result task() override;
};

} // namespace nd

#endif // ND_ENDPOINTS_TEST_EVENT_GENERATOR_H