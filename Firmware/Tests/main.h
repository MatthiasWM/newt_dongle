//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

// Stand-in for the platform `main.h` when building the host unit tests.
// It provides only what the tested files in `common/` use.

#ifndef ND_MAIN_H
#define ND_MAIN_H

#include <cstdint>

namespace nd {

// Same signatures as nd::Logger (see PicoAsyncLog).
class TestLog {
public:
    void log(const char *message, int pipe = 0);
    void logf(const char *format, ...) __attribute__((format(printf, 2, 3)));
    void indent(uint32_t depth);
};

// Newton Interconnect Port serial UART, also the bitrate for Dock mode
constexpr uint32_t kUART_BaudRate = 38400;

// Debugger settings
constexpr bool kDebugErrors = false;

// Log settings
constexpr bool kLogDock = false;
constexpr bool kLogDockProgress = false;
constexpr bool kLogDockErrors = false;
constexpr bool kLogDTRSwitch = false;
constexpr bool kLogMNPErrors = false;
constexpr bool kLogMNPWarnings = false;
constexpr bool kLogMNPState = false;
constexpr bool kLogMNPFlow = false;
constexpr bool kLogNSOF = false;

} // namespace nd

extern nd::TestLog Log;

#endif // ND_MAIN_H
