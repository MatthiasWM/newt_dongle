//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_LOGGER_H
#define ND_LOGGER_H

#include "Event.h"

namespace nd {

class Logger {
    // Logger is an abstract base class for logging events and results.
    // It can be extended to log to different destinations (e.g., console, file, etc.).
public:
    Logger();
    virtual ~Logger() = default;
    virtual void log(Event event, int pipe=0);
    virtual void log(Result result, int pipe=0);
    virtual void log(const char *message, int pipe=0);
    virtual void logf(const char *message, ...);
};

} // namespace nd

#endif // ND_LOGGER_H