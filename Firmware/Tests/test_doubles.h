//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

// Test doubles for the platform classes that `common/` gets injected.

#ifndef ND_TESTS_TEST_DOUBLES_H
#define ND_TESTS_TEST_DOUBLES_H

#include "common/Endpoints/SDCardEndpoint.h"
#include "common/StatusDisplay.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace nd {

// Stand-in for PicoStatusDisplay. Remembers the last status.
class TestStatusDisplay : public StatusDisplay {
public:
    AppStatus status = AppStatus::IDLE;
    TestStatusDisplay(Scheduler &scheduler) : StatusDisplay(scheduler) { }
    Result init() override { return Result::OK; } // without the log message
    void set(AppStatus new_status) override { status = new_status; }
};

// Stand-in for PicoSDCardEndpoint: a card without directory entries. Every
// file name opens `file`, unless `file` is empty. Reading fails at `readable`.
class TestSDCard : public SDCardEndpoint {
public:
    std::u16string label = u"TEST";
    std::vector<uint8_t> file;
    size_t readable = SIZE_MAX;
    size_t pos = 0;
    bool is_open = false;

    TestSDCard(Scheduler &scheduler) : SDCardEndpoint(scheduler) { }
    const char *strerr(uint32_t) override { return "test error"; }
    uint32_t status() override { return kOK; }
    const std::u16string &get_label() override { return label; }
    uint32_t opendir() override { return kOK; }
    uint32_t readdir(std::u16string &name) override { name.clear(); return kOK; }
    uint32_t closedir() override { return kOK; }
    uint32_t openfile(const std::u16string &) override {
        if (file.empty()) return kOK + 1;
        pos = 0;
        is_open = true;
        return kOK;
    }
    uint32_t filesize() override { return (uint32_t)file.size(); }
    uint32_t readfile(uint8_t *buffer, uint32_t size) override {
        uint32_t n = 0;
        while (n < size && pos < file.size() && pos < readable) buffer[n++] = file[pos++];
        return n;
    }
    uint32_t closefile() override { is_open = false; return kOK; }
    uint32_t chdir(const std::u16string &) override { return kOK; }
    uint32_t getcwd(std::u16string &path) override { path = u"/"; return kOK; }
};

} // namespace nd

#endif // ND_TESTS_TEST_DOUBLES_H
