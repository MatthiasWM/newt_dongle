//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoStatusDisplay.h"
#include "main.h"

using namespace nd;

static constexpr uint32_t kCycleTime = 125'000;

PicoStatusDisplay::PicoStatusDisplay(Scheduler &scheduler) : StatusDisplay(scheduler) {
}

void PicoStatusDisplay::early_init() {
    set_color_(0);
    gpio_init(kLED_RED); // User LED red
    gpio_set_dir(kLED_RED, GPIO_OUT);
    gpio_init(kLED_GREEN); // User LED green
    gpio_set_dir(kLED_GREEN, GPIO_OUT);
    gpio_init(kLED_BLUE); // User LED blue
    gpio_set_dir(kLED_BLUE, GPIO_OUT);
    set_color_(0x04); // Start with red, so if we hang, we have an error indicator
}

PicoStatusDisplay::~PicoStatusDisplay() {
    // Destructor implementation can be added here if needed
}

Result PicoStatusDisplay::task() 
{
    uint8_t show_leds = 0;
    uint32_t td = scheduler().cycle_time();
    main_timer_ += td;
    if (main_timer_ >= kCycleTime) {
        main_timer_ = 0; // Reset the main timer every few ms
        main_phase_ = (main_phase_ + 1) & 0x07; // Cycle through 8 phases
        show_leds = 1; // Indicate that we should show the main status
    }
    if (temp_repeats_ > 0) {
        show_leds = 2;
        temp_timer_ += td;
        if (temp_timer_ >= kCycleTime) {
            temp_timer_ = 0; // Reset the temporary timer every few ms
            temp_repeats_--;
            if (temp_repeats_ == 0) {
                show_leds = 1; // Indicate that we should show the main status again
            } else {
                temp_phase_ = (temp_phase_ + 1) & 0x07; // Cycle through 8 phases
                show_leds = 2; // Indicate that we should show the temporary status
            }
        }
    }
    if (show_leds == 0) {
        return Result::OK; // Nothing to do
    } else if (show_leds == 1) {
        set_color_(main_pattern_ >> (main_phase_ * 4) & 0x0f);
    } else if (show_leds == 2) {
        set_color_(temp_pattern_ >> (temp_phase_ * 4) & 0x0f);
    }
    return Result::OK;
}

Result PicoStatusDisplay::init() {
    // Initialization logic for the status display task
    Result res = StatusDisplay::init();
    set(AppStatus::IDLE); // Start with IDLE status
    return res;
}

uint32_t PicoStatusDisplay::to_pattern_(AppStatus status)
{
    switch (status) {
        case AppStatus::IDLE: return 0x66666666; // steady yellow
        case AppStatus::ERROR: return 0x40404040; // rapid flash red
        case AppStatus::USB_READY: return 0x66662222; // slow yellow green flash
        case AppStatus::USB_CONNECTED: return 0x22222222; // steady green
        case AppStatus::SDCARD_ACTIVE: return 0x11111111; // steady blue
        default: return 0x46464646; // huh?
    }
}

/**
 * Set the main status.
 */
void PicoStatusDisplay::set(AppStatus new_status) {
    main_pattern_ = to_pattern_(new_status);
    main_phase_ = 0x0f; // Reset the main phase
    //main_timer_ = kCycleTime; // Reset the main timer
}

/**
 * Set a temporary status a for the given number of ticks.
 * When the ticks elapsed, go back to the main status.
 */
void PicoStatusDisplay::repeat(AppStatus temp_status, int repeats) {
    if (repeats <= 0) return;
    temp_pattern_ = to_pattern_(temp_status);
    temp_phase_ = 0x0f; // Reset the temporary phase
    temp_timer_ = kCycleTime; // Reset the temporary timer
    temp_repeats_ = repeats + 1; // Cycle 0 is disabled, cycle 1 restores the main status
    set_color_(temp_pattern_ & 0x0f); // Set this right away!
}

/**
 * Set the color using the low nibble (IRGB) of the byte.
 */
void PicoStatusDisplay::set_color_(uint8_t color) {
    uint8_t bits = color ^ kLED_INVERT;
    gpio_put(kLED_RED, (bits & 0x04) ? 1 : 0);
    gpio_put(kLED_GREEN, (bits & 0x02) ? 1 : 0);
    gpio_put(kLED_BLUE, (bits & 0x01) ? 1 : 0);
}

