//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_MAIN_H
#define ND_MAIN_H

#include <cstdint>

#include "common/Logger.h"
#include "common/UserSettings.h"

//#include <hardware/uart.h>

extern nd::Logger Log;
extern nd::UserSettings user_settings;

namespace nd {

// Debugger settings
constexpr bool kDebugErrors = true; // Print errors to the console
constexpr bool kDebugHayes = false;
constexpr bool kDebugMNPThrottle = false;
constexpr bool kDebugCDC = false;
constexpr bool kDebugFlash = false;

// Log settings
constexpr bool kLogTime = false;
constexpr bool kLogUART = true;
constexpr bool kLogMNPErrors = true;
constexpr bool kLogMNPWarnings = false;
constexpr bool kLogMNPState = false;
constexpr bool kLogMNFlow = false;
constexpr bool kLogMNDock = false;
constexpr bool kLogDock = true;

} // namespace nd

#endif // ND_MAIN_H