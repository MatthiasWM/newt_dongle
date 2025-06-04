//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_STDIO_LOG_H
#define ND_PICO_STDIO_LOG_H

#include "common/Endpoints/StdioLog.h"

namespace nd {

/** 
 * \brief Log events to stdio on Posix machines. 
 */
class PicoStdioLog: public StdioLog 
{
protected:
    // -- Customize the base class
    /// \brief Check if the next call to write a byte would block.
    bool would_block() override;

public:
    // -- Constructors
    /// \brief Create a StdioLog endpoint that is called regularly by the scheduler.
    PicoStdioLog(Scheduler &scheduler) : StdioLog(scheduler) { }

    /// \brief Destructor.
    ~PicoStdioLog() override = default;
};

} // namespace nd

#endif // ND_PICO_STDIO_LOG_H