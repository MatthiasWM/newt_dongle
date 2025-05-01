//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_TEST_STDIO_LOG_H
#define ND_TEST_STDIO_LOG_H

#include "common/Endpoints/StdioLog.h"

namespace nd {

/** 
 * \brief Log events to stdio on Posix machines. 
 */
class TestStdioLog: public StdioLog 
{
protected:
    // -- Customize the base class
    /// \brief Check if the next call to write a byte would block.
    bool would_block() override;

public:
    // -- Constructors
    /// \brief Create a StdioLog endpoint that is called regularly by the scheduler.
    TestStdioLog(Scheduler &scheduler) : StdioLog(scheduler) { }

    /// \brief Destructor.
    ~TestStdioLog() override = default;
};

} // namespace nd

#endif // ND_TEST_STDIO_LOG_H