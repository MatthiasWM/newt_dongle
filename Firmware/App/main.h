//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_MAIN_H
#define ND_MAIN_H

#include <cstdint>

#include "common/Logger.h"
#include "common/UserSettings.h"
#include "Posix/Endpoints/PosixSDCardEndpoint.h"
#include "common/StatusDisplay.h"

extern nd::Logger Log;
extern nd::UserSettings user_settings;
extern nd::PosixSDCardEndpoint sdcard_endpoint;
extern nd::StatusDisplay app_status;

namespace nd {

// Debugger settings
constexpr bool kDebugErrors = true; // Print errors to the console
constexpr bool kDebugHayes = false;
constexpr bool kDebugMNPThrottle = false;
constexpr bool kDebugCDC = false;
constexpr bool kDebugFlash = false;
#define ND_DEBUG_DOCK 0

// Log settings
constexpr bool kLogTime = false;
constexpr bool kLogUART = true;
constexpr bool kLogCDC = false;
constexpr bool kLogMNPErrors = true;
constexpr bool kLogMNPWarnings = false;
constexpr bool kLogMNPState = false;
constexpr bool kLogMNPFlow = false;
constexpr bool kLogDock = true;
constexpr bool kLogDockProgress = false;
constexpr bool kLogDockErrors = false;
constexpr bool kLogNSOF = false;
constexpr bool kLogSDCard = false;
constexpr bool kLogDTRSwitch = false;

constexpr uint kUART_BaudRate = 38400;

} // namespace nd

#endif // ND_MAIN_H
