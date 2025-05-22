//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "UserSettings.h"

#include <cstring>

using namespace nd;

/**
 * @brief All the setting that the user can store in Flash or an EEPROM.
 */

UserSettings::Fingerprint UserSettings::factory_fingerprint_ = {
    .magic_ = { 0xC1, 0xA5, 0x51, 0xF1, 0xED, 0xC0, 0xFF, 0xEE },
    .page_map_ = 0xffffffff,
    .page_size_ = 1, // 1x256 bytes
    .sector_size_ = 16, // 16x256 bytes = 4kB
    .sector_count_ = 1,
    .version_ = 0,
    .serial_no_ = 0,
    .hardware_version_ = 0,
    .hardware_revision_ = 0,
};

UserSettings::Data UserSettings::factory_data = {
    .mnpt_absolute_delay = 400,         // S300
    .mnpt_num_char_delay = 8,           // S301
    .hayes0_esc_code_guard_time = 50,   // S12, time in 50ths of a second
    .hayes1_esc_code_guard_time = 50,
};

UserSettings::UserSettings() {
    factory_reset();
}

void UserSettings::factory_reset() {
    memset(fingerprint_page_, 0xff, sizeof(fingerprint_page_));
    fingerprint_ = factory_fingerprint_;
    memset(data_page_, 0xff, sizeof(data_page_));
    data = factory_data;
}

Result UserSettings::write_serial(uint32_t serial, uint16_t id, uint16_t version, uint16_t revision) {
    fingerprint_.serial_no_ = serial;
    fingerprint_.hardware_id_ = id;
    fingerprint_.hardware_version_ = version;
    fingerprint_.hardware_revision_ = revision;
    return Result::OK;
}

Result UserSettings::write() {
    return Result::OK__NOT_HANDLED;
}

Result UserSettings::read() {
    return Result::OK__NOT_HANDLED;
}
