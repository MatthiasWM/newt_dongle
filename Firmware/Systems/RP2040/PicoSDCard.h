//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_SD_CARD_H
#define ND_PICO_SD_CARD_H

extern void test_sd_card();

#include "common/Endpoints/SDCardEndpoint.h"

#include "ff.h"
#include "sd_card.h"

namespace nd {

class PicoSDCardEndpoint : public SDCardEndpoint {
    FATFS fat_fs_;
    sd_card_t *sd_card_ = nullptr;
public:
    PicoSDCardEndpoint(Scheduler &scheduler);
    ~PicoSDCardEndpoint() override;
    Result init() override;
    Result task() override;
    Result send(Event event) override;

    const char *strerr(uint32_t err) override;
    uint32_t disk_status() override;
    uint32_t disk_initialize() override;
    uint32_t get_label(char *label_buffer) override;
};

} // namespace nd

#endif // ND_PICO_SD_CARD_H
