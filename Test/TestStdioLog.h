//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_TEST_STDIO_LOG_H
#define ND_TEST_STDIO_LOG_H

#include "common/Endpoints/StdioLog.h"

namespace nd {

class TestStdioLog: public StdioLog {
protected:
    bool would_block() override;
public:
    TestStdioLog(Wheel &wheel) : StdioLog(wheel) { }
    ~TestStdioLog() override = default;
};

} // namespace nd

#endif // ND_TEST_STDIO_LOG_H