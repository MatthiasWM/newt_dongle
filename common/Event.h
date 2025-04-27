//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_EVENT_H
#define ND_EVENT_H

#include <cstdint>

namespace nd {

enum class Test { };

enum class Type: uint8_t {
    DATA = 0,
    SET_BITRATE,
    ERROR = 0xff,   // Not an event, but an erroro message.
};

enum class Error: uint8_t {
    OK = 0,
    PIPE_EMPTY,
    PIPE_FULL,
};

// An event can be raw data or control events. Delta_t is the time since the 
// last event or 0xffff for time beyond range.
typedef union {
    uint32_t value;
    struct {
        Type type;
        uint8_t data;
        uint16_t delta_t;
    };
} Event;


static_assert(sizeof(Event) == 4, "Event size must be 4 bytes");

constexpr Type event_type(nd::Event event) { return event.type; }
constexpr uint8_t event_data(Event event) { return event.data; }
constexpr uint16_t event_delta_t(Event event) { return event.delta_t; }
void event_delta_t(Event event, int64_t uSecs);
constexpr Error event_error(Event event) { 
    return (event.type==Type::ERROR)?static_cast<Error>(event.data):(Error::OK); }

constexpr Event make_event(Type type, uint8_t data) {
    return { .type = type, .data = data, .delta_t = 0 };
}

    
} // namespace nd

#endif // ND_EVENT_H