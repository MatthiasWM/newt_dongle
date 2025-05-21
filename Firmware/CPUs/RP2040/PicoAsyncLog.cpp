//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoAsyncLog.h"

#include "pico/multicore.h"
#include "pico/stdlib.h"
#include <stdio.h>

using namespace nd;

PicoAsyncLog *PicoAsyncLog::the_instance_ = nullptr;
queue_t PicoAsyncLog::queue_ = {};

/**
 * @brief PicoAsyncLog constructor
 * @param dest 0 to log to stdio, 1 to log to sd card
 */
PicoAsyncLog::PicoAsyncLog(int dest) {
    the_instance_ = this;
    dest_ = dest;
    queue_init(&queue_, sizeof(Event), 1024);
    multicore_launch_core1(run_);
}

void PicoAsyncLog::log(Event event, int pipe) {
    uint32_t e = event.raw();
    if (pipe == 1) e |= 0x8000'0000;
    queue_try_add(&queue_, &e);
}

void PicoAsyncLog::log(Result result, int pipe) {
    uint32_t e = result.raw() | 0x4000'0000;
    if (pipe == 1) e |= 0x8000'0000;
    queue_try_add(&queue_, &e);
}

void PicoAsyncLog::log(const char *message, int pipe) {
    for (const char *src = message; *src != 0; src++) {
        log(Event(Event::Type::TEXT, *src), pipe);
    }
}

/**
 * @brief Run the logging on the second processor
 */
void PicoAsyncLog::run() {
    static char hex_lut[] = "0123456789ABCDEF";
    while (true) {
        uint32_t e;
        queue_remove_blocking(&queue_, &e);

        if (prev_event_time_ == nil_time) {
            prev_event_time_ = get_absolute_time();
        } else {
            absolute_time_t now = get_absolute_time();
            uint32_t diff = absolute_time_diff_us(prev_event_time_, now);
            if (diff > 4000) {
                uint32_t sec = diff / 1'000'000;
                uint32_t msec = (diff % 1'000'000) / 1000;
                uint32_t usec = diff % 1000;
                printf("\nt[%d.%03d.%03d]\n", sec, msec, usec);
            }
            prev_event_time_ = now;
        }

        int pipe = (e & 0x8000'0000) ? 1 : 0;
        bool is_event = (e & 0x4000'0000) == 0;
        e &= 0x3FFF'FFFF;
        if (is_event) {
            Event event;
            event.raw(e);
            if (event.type() == Event::Type::DATA) {
                uint8_t data = event.data();
                if (pipe == 0) 
                    putchar('>');
                else
                    putchar('<');
                putchar(hex_lut[(data >> 4) & 0x0f]);
                putchar(hex_lut[data & 0x0f]);
                if (data>=' ' && data<='~') {
                    putchar(data);
                } else {
                    putchar('.');
                }
                putchar(' ');
            } else if (event.type() == Event::Type::TEXT) {
                putchar(event.data());
            } else if (event.type() == Event::Type::ERROR) {
                printf("\nE[%d,%d]", event.subtype(), event.data());
            } else if (event.type() == Event::Type::SET_BITRATE) {
                printf("\nB[%d]", event.bitrate());
            } else if (event.type() == Event::Type::DELAY) {
                printf("\nD[%d,%d]", event.subtype(), event.data());
            } else if (event.type() == Event::Type::HIGH_WATER) {
                printf("\nH[%d,%d]", event.subtype(), event.data());
            } else {
                printf("\n?[%d,%d,%d]", event.type(), event.subtype(), event.data());
            }
        } else {
            Result result;
            result.raw(e);
            if (result.type() == Result::Type::REJECTED) {
                // Handle REJECTED result
            }
        }
    }
}
