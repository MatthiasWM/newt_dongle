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
constexpr bool kDebugErrors = false; // Print errors to the console
constexpr bool kDebugHayes = false;
constexpr bool kDebugMNPThrottle = false;
constexpr bool kDebugCDC = false;
constexpr bool kDebugFlash = false;
#define ND_DEBUG_DOCK 0

// Log settings
constexpr bool kLogTime = false;
constexpr bool kLogUART = false;
constexpr bool kLogCDC = false;
constexpr bool kLogMNPErrors = false;
constexpr bool kLogMNPWarnings = false;
constexpr bool kLogMNPState = false;
constexpr bool kLogMNPFlow = false;
constexpr bool kLogDock = false;
constexpr bool kLogDockProgress = false;
constexpr bool kLogDockErrors = false;
constexpr bool kLogNSOF = false;
constexpr bool kLogSDCard = false;
constexpr bool kLogDTRSwitch = false;

// PiPico developer board settings

// Newton Interconnect Port serial UART
#define kUART uart0
constexpr uint kUART_BaudRate = 38400;

// --- All XIAO Board RP2040 ports:
// External
constexpr uint kUART_TX_Pin = 0;   // TX pin
constexpr uint kUART_RX_Pin = 1;   // RX pin
constexpr uint kUART_HSKI_Pin = 28; // HSKI pin
constexpr uint kUART_HSKO_Pin = 29; // HSKO pin on Dongle (22 on PiPico)
constexpr uint kSD_SEL_Pin = 27; // SD_SEL pin
constexpr uint kSD_MOSI_Pin = 3; // SD_MOSI pin
constexpr uint kSD_MISO_Pin = 4; // SD_MISO pin
constexpr uint kSD_SCK_Pin = 2; // SD_SCK pin
constexpr uint kPORT_SEL_Pin = 26; // PORT_SEL pin
constexpr uint kGPIO_Pin = 6; // GPIO pin
constexpr uint kDOCK_ATTACH_Pin = 7; // DOCK_ATTACH pin
// Internal
constexpr uint kLED_RED = 17;
constexpr uint kLED_GREEN = 16;
constexpr uint kLED_BLUE = 25; // (18 on PiPico)
constexpr uint kLED_INVERT = 0x07; // Invert all.
// Missing: Select, Reset, Second LED

} // namespace nd

#endif // ND_MAIN_H