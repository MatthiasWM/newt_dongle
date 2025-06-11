//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_STATUS_DISPLAY_H
#define ND_PICO_STATUS_DISPLAY_H

#include "common/StatusDisplay.h"

namespace nd {

class Task;

/** \brief Call the tasks of all registered Endpoints in an endless loop. */
class PicoStatusDisplay : public StatusDisplay
{
protected:
    uint32_t main_pattern_ = 0x21212124; // Rapid flash
    uint32_t main_timer_ = 0;
    uint32_t temp_pattern_ = 0x00000000; // Off
    uint32_t temp_timer_ = 0;
    int temp_repeats_ = 0; // How many times to repeat the temporary status
    uint8_t main_phase_ = 0;
    uint8_t temp_phase_ = 0;

    uint32_t to_pattern_(AppStatus status);
    void set_color_(uint8_t color);
public:
    PicoStatusDisplay(Scheduler &scheduler);
    ~PicoStatusDisplay() override;
    Result init() override;
    Result task() override;

    void early_init();

    void set(AppStatus new_status) override;
    void repeat(AppStatus temp_status, int repeats) override;
};

} // namespace nd

#endif // ND_PICO_STATUS_DISPLAY_H