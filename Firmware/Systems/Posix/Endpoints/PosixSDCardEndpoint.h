//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_POSIX_SD_CARD_H
#define ND_POSIX_SD_CARD_H

extern void test_sd_card();

#include "common/Endpoints/SDCardEndpoint.h"

namespace nd {

constexpr uint32_t FR_OK = 0;
constexpr uint32_t FR_INVALID_PARAMETER = 37; // Arbitrary
constexpr uint32_t FR_IS_DIRECTORY = FR_INVALID_PARAMETER + 1;
constexpr uint32_t FR_IS_PACKAGE = FR_INVALID_PARAMETER + 2;

class PosixSDCardEndpoint : public SDCardEndpoint {
    std::u16string label_ { u"ERROR" }; // Label of the SD card
    uint32_t status_ = FR_INVALID_PARAMETER; // Status of the disk
public:
    PosixSDCardEndpoint(Scheduler &scheduler);
    ~PosixSDCardEndpoint() override;
    Result init() override;
    Result task() override;
    Result send(Event event) override;

    void early_init();

    const char *strerr(uint32_t err) override;
    uint32_t status() override { return status_; }
    const std::u16string &get_label() override;

    uint32_t opendir() override;
    uint32_t readdir(std::u16string &name) override;
    uint32_t closedir() override;

    uint32_t openfile(const std::u16string &name) override;
    uint32_t filesize() override;
    uint32_t readfile(uint8_t *buffer, uint32_t size) override;
    uint32_t closefile() override;

    uint32_t chdir(const std::u16string &path) override;
    uint32_t getcwd(std::u16string &path) override;
};

} // namespace nd

#endif // ND_POSIX_SD_CARD_H
