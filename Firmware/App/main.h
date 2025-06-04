//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_MAIN_H
#define ND_MAIN_H

#include "PicoAsyncLog.h"
#include "PicoUserSettings.h"

#include <hardware/uart.h>

extern nd::PicoAsyncLog Log;
extern nd::PicoUserSettings user_settings;

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

// PiPico developer board settings

// Newton Interconnect Port serial UART
#define kUART uart0
constexpr uint kUART_BaudRate = 38400;
constexpr uint kUART_TX_Pin = 0;   // TX pin
constexpr uint kUART_RX_Pin = 1;   // RX pin
constexpr uint kUART_HSKI_Pin = 28; // HSKI pin
constexpr uint kUART_HSKO_Pin = 29; // HSKO pin on PiPico

} // namespace nd

#endif // ND_MAIN_H