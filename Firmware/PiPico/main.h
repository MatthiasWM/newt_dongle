//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_MAIN_H
#define ND_MAIN_H

#include "PicoAsyncLog.h"
#include "PicoUserSettings.h"
#include "PicoSDCard.h"
#include "PicoStatusDisplay.h"

#include <hardware/uart.h>

extern nd::PicoAsyncLog Log;
extern nd::PicoUserSettings user_settings;
extern nd::PicoSDCardEndpoint sdcard_endpoint;
extern nd::PicoStatusDisplay app_status;

namespace nd {

// Debugger settings
constexpr bool kDebugErrors = true; // Print errors to the console
constexpr bool kDebugHayes = false;
constexpr bool kDebugMNPThrottle = false;
constexpr bool kDebugCDC = false;
constexpr bool kDebugFlash = false;
#define ND_DEBUG_DOCK 1

// Log settings
constexpr bool kLogTime = false;
constexpr bool kLogUART = false;
constexpr bool kLogCDC = false;
constexpr bool kLogMNPErrors = true;
constexpr bool kLogMNPWarnings = false;
constexpr bool kLogMNPState = false;
constexpr bool kLogMNPFlow = false;
constexpr bool kLogDock = false;
constexpr bool kLogDockProgress = true;
constexpr bool kLogDockErrors = true;
constexpr bool kLogNSOF = false;
constexpr bool kLogSDCard = false;

// PiPico developer board settings

// Newton Interconnect Port serial UART
#define kUART uart0
constexpr uint kUART_BaudRate = 38400;
constexpr uint kUART_TX_Pin = 0;   // TX pin
constexpr uint kUART_RX_Pin = 1;   // RX pin
constexpr uint kUART_HSKI_Pin = 28; // HSKI pin
constexpr uint kUART_HSKO_Pin = 29; // HSKO pin on PiPico

constexpr uint kLED_RED = 17;
constexpr uint kLED_GREEN = 16;
constexpr uint kLED_BLUE = 18; // 25 on Dongle
constexpr uint kLED_INVERT = 0x00;

} // namespace nd

#endif // ND_MAIN_H