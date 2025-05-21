//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoUserSettings.h"

#include <pico/flash.h>
#include <hardware/flash.h>

#include <cstring>
#include <cstdio>

using namespace nd;

/**
 * @brief Store settings in a RP2040 Flash page
 */

void PicoUserSettings::factory_reset_flash_cb_(void *param) {
    ((PicoUserSettings*)param)->do_factory_reset_flash_();
}

void PicoUserSettings::do_factory_reset_flash_() {
    fingerprint_ = factory_fingerprint_;
    fingerprint_.page_map_ = 0xFFFFFFFC; // bit 0 is the fingerprint, bit 1 is the initial data page
    flash_range_erase(PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
    flash_range_program(PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE, fingerprint_page_, FLASH_PAGE_SIZE);
    flash_range_program(PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE + FLASH_PAGE_SIZE, data_page_, FLASH_PAGE_SIZE);
}

Result PicoUserSettings::factory_reset_flash_() {
    auto ret = flash_safe_execute(factory_reset_flash_cb_, (void*)this, 10);
    if (ret != PICO_OK) {
        printf("Error factory_reset_flash_: flash_safe_execute failed (%d)\n", ret);
        return Result::REJECTED;
    }
    FingerprintPage *fp = (FingerprintPage *)(XIP_BASE + PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE);
    if (memcmp(fp->fingerprint_.magic_, factory_fingerprint_.magic_, sizeof(factory_fingerprint_.magic_)) != 0) {
        printf("Error factory_reset_flash_: flash magic number test failed\n");
        return Result::REJECTED;
    }
    return Result::OK;
}


void PicoUserSettings::write_flash_page_cb_(void *param) {
    ((PicoUserSettings*)param)->do_write_flash_page_(((PicoUserSettings*)param)->current_page_);
}

void PicoUserSettings::do_write_flash_page_(uint8_t page) {
    printf("Writing Flash page %d\n", page);
    FingerprintPage *fp = (FingerprintPage *)(XIP_BASE + PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE);
    if (override_serial_) {
        // Don't copy the serial number from Flash; use the one in RAM
        page = 1; // make sure we erase the sector and rewrite the fingerprint
    } else {
        fingerprint_ = fp->fingerprint_; // copy fingerprint from Flash, preserving the serial number
    }
    if (page == 16) page = 1;
    if (page == 1) {
        fingerprint_.page_map_ = 0xFFFFFFFC; // reset to use page 1
        flash_range_erase(PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
    } else {
        fingerprint_.page_map_ &= ~(1 << current_page_);
    }
    flash_range_program(PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE, fingerprint_page_, FLASH_PAGE_SIZE);
    flash_range_program(PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE + page * FLASH_PAGE_SIZE, data_page_, FLASH_PAGE_SIZE);
}

Result PicoUserSettings::write_flash_page_(uint8_t page) {
    current_page_ = page;
    auto ret = flash_safe_execute(write_flash_page_cb_, (void*)this, 10);
    if (ret != PICO_OK) {
        printf("Error do_write_flash_page_: flash_safe_execute failed (%d)\n", ret);
        return Result::REJECTED;
    }
    return Result::OK;
}


Result PicoUserSettings::write_serial(uint32_t serial, uint16_t version, uint16_t revision) {
    UserSettings::write_serial(serial, version, revision);
    override_serial_ = true;
    Result res = write();
    override_serial_ = false;
    return res;
}

Result PicoUserSettings::mess_up_flash() {
    memset(fingerprint_page_, 0xFF, sizeof(fingerprint_page_));
    override_serial_ = true;
    Result res = write_flash_page_(1);
    override_serial_ = false;
    return res;
}

Result PicoUserSettings::write() {
    FingerprintPage *fp = (FingerprintPage *)(XIP_BASE + PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE);
    if ((memcmp(fp->fingerprint_.magic_, factory_fingerprint_.magic_, sizeof(factory_fingerprint_.magic_)) != 0)
        || (fp->fingerprint_.page_size_ != factory_fingerprint_.page_size_)
        || (fp->fingerprint_.sector_size_ != factory_fingerprint_.sector_size_))
    {
        Result res = factory_reset_flash_();
        return res;
    }
    int j; 
    for (j = 1; j < 16; j++) {
        if (fp->fingerprint_.page_map_ & (1 << j)) break;
    }
    return write_flash_page_(j);
}

Result PicoUserSettings::read() {
    FingerprintPage *fp = (FingerprintPage *)(XIP_BASE + PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE);
    if ((memcmp(fp->fingerprint_.magic_, factory_fingerprint_.magic_, sizeof(factory_fingerprint_.magic_)) != 0)
        || (fp->fingerprint_.page_size_ != factory_fingerprint_.page_size_)
        || (fp->fingerprint_.sector_size_ != factory_fingerprint_.sector_size_))
    {
        printf("Error: invalid fingerprint.\n");
        return Result::REJECTED;
    }
    uint32_t page_base = XIP_BASE + PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;
    int j; 
    for (j = 1; j < 16; j++) {
        if (fp->fingerprint_.page_map_ & (1 << j)) break;
        page_base += FLASH_PAGE_SIZE;
    }
    printf("Reading Flash page %d\n", j-1);
    if (j == 1) {
        printf("Error: invalid flash map.\n");
        return Result::REJECTED;
    } else {
        memcpy(fingerprint_page_, fp->page_, FLASH_PAGE_SIZE);
        memcpy(data_page_, (void*)page_base, FLASH_PAGE_SIZE);
    }
    return Result::OK;
}
