//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_USER_SETTINGS_H
#define ND_PICO_USER_SETTINGS_H

#include "common/UserSettings.h"

namespace nd {

class PicoUserSettings : public UserSettings {

    static void factory_reset_flash_cb_(void *param);
    void do_factory_reset_flash_();
    Result factory_reset_flash_();

    uint8_t current_page_ = 0;
    bool override_serial_ = false;
    static void write_flash_page_cb_(void *param);
    void do_write_flash_page_(uint8_t page);
    Result write_flash_page_(uint8_t page);

public:
    PicoUserSettings() = default;
    // DANGER: for initialization only!
    Result write_serial(uint32_t serial, uint16_t id, uint16_t version, uint16_t revision) override;
    // DANGER: for testing only!
    Result mess_up_flash();
    Result write() override;
    Result read() override;
};

} // namespace nd

#endif // ND_PICO_USER_SETTINGS_H