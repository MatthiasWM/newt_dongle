//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Event.h"

using namespace nd;



Result Result::OK = { Type::OK, 0 };
Result Result::OK__NOT_CONNECTED = { Type::OK, Cause::NOT_CONNECTED };
Result Result::REJECTED = { Type::REJECTED, 0 };


#if 0

#include "pico/stdlib.h"
#include <stdio.h>

using namespace nd;

/**
 * \brief Create a new delay event.
 * 
 * \param delay_us The delay in microseconds
 * \return A new delay event with the given timeouts
 */
Event nd::make_delay(uint32_t delay_us) {
    if ((delay_us & 0x000fffff) != 0) {
        return make_event(Type::DELAY_MS, uint20_to_fp8(delay_us/1000));
    }
    return make_event(Type::DELAY_US, delay_us);
}


/**
 * Store the time since the last event in milliseconds with the event.alignas
 * 
 * \note It's binary mSec as 1024 uSecs in a mSec. Dividing is expensive.alignas
 * 
 * \param[inout] event update the event time in this event
 * \param[in] uSecs time delta in microseconds 
 * 
 * \see pico/time.h: absolute_time_diff_us()
 */
void nd::event_delta_t(Event event, int64_t uSecs) {
    int64_t mSecs = uSecs >> 10; // divide by 1024
    if (mSecs > 0xffff) mSecs = 0xffff;
    event.delta_t = static_cast<uint16_t>(mSecs);
}

uint32_t nd::id_to_bitrate(uint8_t id) {
    switch (id) {
        case 0: return 300;
        case 1: return 1200;
        case 2: return 2400; 
        case 3: return 4800; 
        case 4: return 9600; 
        case 5: return 19200; 
        case 6: return 38400; 
        case 7: return 57600; 
        case 8: return 115200; 
        case 9: return 230400;
        default:
            printf("**** ERROR ***: unsupported bitrate ID %d\n", id);
            return 38400;
    }
}

uint8_t nd::bitrate_to_id(uint32_t bitrate) {
    switch (bitrate) {
        case    300: return 0;
        case   1200: return 1;
        case   2400: return 2; 
        case   4800: return 3; 
        case   9600: return 4; 
        case  19200: return 5; 
        case  38400: return 6; 
        case  57600: return 7; 
        case 115200: return 8; 
        case 230400: return 9;
        default:
            printf("**** ERROR ***: unsupported bitrate %d\n", bitrate);
            return 0xff;
    }
}

/**
 * Convert an 8 bit float into n integer value between 0 and 1'000'000.
 * 
 * \param id 8 bit floating point(4 bits exponent, 4 bits mantissa)
 * \return an unsigned integer up to 0x000f'ffff
 */
uint32_t nd::fp8_to_uint20(uint8_t id) {
    uint32_t mantissa = id & 0x0F;
    uint8_t exponent = (id & 0xF0) >> 4;
    uint32_t us = mantissa << exponent;
    return us;
}

/**
 * Convert an integer value between 0 and 1'000'000 into an 8 bit float.
 * 
 * \param us 20 bit unsigned integer
 * \return an 8 bit floating point(4 bits exponent, 4 bits mantissa)
 */
uint8_t nd::uint20_to_fp8(uint32_t us) {
    uint8_t exponent = 0;
    while ((us & 0xFFFFFFF0) > 0) {
        us >>= 1;
        exponent++;
    }
    uint8_t mantissa = static_cast<uint8_t>(us);
    return (mantissa | (exponent << 4));
}

#endif