//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_USER_SETTINGS_H
#define ND_USER_SETTINGS_H

#include "Event.h"

#include <cstdint>

namespace nd {

class UserSettings {
protected:
    typedef struct {
        uint8_t magic_[8];
        uint32_t page_map_;
        uint8_t page_size_;
        uint8_t sector_size_;
        uint8_t sector_count_;
        uint8_t version_;
        uint32_t serial_no_;
        uint16_t hardware_id_;
        uint16_t hardware_version_;
        uint16_t hardware_revision_;
    } Fingerprint;

    static Fingerprint factory_fingerprint_;

    typedef union {
        Fingerprint fingerprint_;
        uint8_t page_[256];
    } FingerprintPage;

    union {
        Fingerprint fingerprint_;
        uint8_t fingerprint_page_[256];
    };

public:
    typedef struct {
            uint32_t mnpt_absolute_delay;
            uint8_t mnpt_num_char_delay;
            uint8_t hayes0_esc_code_guard_time;
            uint8_t hayes1_esc_code_guard_time;
    } Data;

    static Data factory_data;

    typedef union {
        Data data;
        uint8_t page_[256];
    } DataPage;

    union {
        Data data;
        uint8_t data_page_[256];
    };
    
    UserSettings();
    virtual ~UserSettings() = default;
    UserSettings(const UserSettings&) = delete;
    UserSettings& operator=(const UserSettings&) = delete;
    UserSettings(UserSettings&&) = delete;
    UserSettings& operator=(UserSettings&&) = delete;

    virtual Result write_serial(uint32_t serial, uint16_t id, uint16_t version, uint16_t revision);
    virtual Result write();
    virtual Result read();
    void factory_reset();

    uint8_t version() const { return fingerprint_.version_; }
    uint32_t serial() const { return fingerprint_.serial_no_; }

    uint16_t hardware_id() const { return fingerprint_.hardware_id_; }
    uint16_t hardware_version() const { return fingerprint_.hardware_version_; }
    uint16_t hardware_revision() const { return fingerprint_.hardware_revision_; }

    static_assert(sizeof(UserSettings::Fingerprint) <= 256, "UserSettings::Fingerprint size must be less or equal to 256 bytes");
    static_assert(sizeof(UserSettings::Data) <= 256, "UserSettings::Data size must be less or equal to 256 bytes");
};


} // namespace nd

#endif // ND_USER_SETTINGS_H