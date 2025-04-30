//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_TEST_DATA_DEVICE_H
#define ND_TEST_DATA_DEVICE_H

#include "common/Device.h"

namespace nd {

class TestDataDevice: public Device {
public:
    TestDataDevice() = default;
    ~TestDataDevice() override = default;

    // -- Initialization
    Result init(Wheel &wheel) override;
    Result release() override;
    Result task() override;
    Result stop() override;
};

} // namespace nd

#endif // ND_TEST_DATA_DEVICE_H