//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Logger.h"

#include <stdio.h>
#include <stdarg.h>

using namespace nd;

Logger::Logger() {
}

void Logger::log(Event event, int pipe) {
}

void Logger::log(Result result, int pipe) {
}

void Logger::log(const char *message, int pipe) {
}

void Logger::logf(const char *message, ...) {
    va_list args;
    va_start(args, message);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);
    log(buffer, 0);
}


#if 0

// TODO: Event::to_string(buffer, size);

/**
 * @brief Run the logging on the second processor
 */
void PicoAsyncLog::run() {
    static char hex_lut[] = "0123456789ABCDEF";
    flash_safe_execute_core_init();
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

#endif