//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

// Stand-in for the platform `main.h` when building the host unit tests.
// It provides only what the tested files in `common/` use.

#ifndef ND_MAIN_H
#define ND_MAIN_H

#include "common/StatusDisplay.h"

#include <cstdint>
#include <string>
#include <vector>

namespace nd {

// Same signatures as nd::Logger (see PicoAsyncLog).
class TestLog {
public:
    void log(const char *message, int pipe = 0);
    void logf(const char *format, ...);
    void indent(uint32_t depth);
};

// Stand-in for PicoStatusDisplay. Remembers the last status.
class TestAppStatus {
public:
    AppStatus status = AppStatus::IDLE;
    void set(AppStatus new_status) { status = new_status; }
};

// Same values as FatFS and PicoSDCard.h.
constexpr uint32_t FR_OK = 0;
constexpr uint32_t FR_IS_DIRECTORY = 20;
constexpr uint32_t FR_IS_PACKAGE = 21;

// Stand-in for PicoSDCardEndpoint: a card without directory entries. Every
// file name opens `file`, unless `file` is empty.
class TestSDCard {
public:
    std::u16string label = u"TEST";
    std::vector<uint8_t> file;
    size_t pos = 0;
    const std::u16string &get_label() { return label; }
    uint32_t opendir() { return FR_OK; }
    uint32_t readdir(std::u16string &name) { name.clear(); return FR_OK; }
    uint32_t closedir() { return FR_OK; }
    uint32_t openfile(const std::u16string &) { pos = 0; return file.empty() ? FR_OK + 1 : FR_OK; }
    uint32_t filesize() { return (uint32_t)file.size(); }
    uint32_t readfile(uint8_t *buffer, uint32_t size) {
        uint32_t n = 0;
        while (n < size && pos < file.size()) buffer[n++] = file[pos++];
        return n;
    }
    uint32_t closefile() { return FR_OK; }
    uint32_t chdir(const std::u16string &) { return FR_OK; }
};

// Debugger settings
constexpr bool kDebugErrors = false;

// Log settings
constexpr bool kLogDock = false;
constexpr bool kLogDockProgress = false;
constexpr bool kLogDockErrors = false;
constexpr bool kLogMNPErrors = false;
constexpr bool kLogMNPWarnings = false;
constexpr bool kLogMNPState = false;
constexpr bool kLogMNPFlow = false;
constexpr bool kLogNSOF = false;

} // namespace nd

extern nd::TestLog Log;
extern nd::TestAppStatus app_status;
extern nd::TestSDCard sdcard_endpoint;

#endif // ND_MAIN_H
