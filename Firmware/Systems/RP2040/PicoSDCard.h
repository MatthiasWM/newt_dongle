//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_SD_CARD_H
#define ND_PICO_SD_CARD_H

// We are using FatFS to implement SD Card access:
// https://elm-chan.org/fsw/ff/00index_e.html

// This is the particular implementation for the RP2040:
// https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git

extern void test_sd_card();

#include "common/Endpoints/SDCardEndpoint.h"

#include "ff.h"
#include "sd_card.h"

namespace nd {

constexpr uint32_t FR_IS_DIRECTORY = FR_INVALID_PARAMETER + 1;
constexpr uint32_t FR_IS_PACKAGE = FR_INVALID_PARAMETER + 2;

class PicoSDCardEndpoint : public SDCardEndpoint {
    FATFS fat_fs_;
    sd_card_t *sd_card_ = nullptr;
    uint32_t status_ = FR_INVALID_PARAMETER; // Status of the disk
    bool initialized_ = false; // true if the SD card is initialized
    bool mounted_ = false; // true if the SD card is mounted
    std::u16string label_; // Volume label
    uint32_t mount_();
    DIR dir_;
    FIL file_;
public:
    PicoSDCardEndpoint(Scheduler &scheduler);
    ~PicoSDCardEndpoint() override;
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

    uint32_t openfile(std::u16string &name) override;
    uint32_t filesize() override;
    uint32_t readfile(uint8_t *buffer, uint32_t size) override;
    uint32_t closefile() override;

    uint32_t chdir(std::u16string &path) override;
};

} // namespace nd

#endif // ND_PICO_SD_CARD_H
