//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_LOGGER_H
#define ND_LOGGER_H

#include "Event.h"

extern int itoa10(int value, char *buffer);

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
    // `this` is the first argument, so the format is argument 2.
    virtual void logf(const char *message, ...) __attribute__((format(printf, 2, 3)));
    void indent(uint32_t level);
};

} // namespace nd

#endif // ND_LOGGER_H