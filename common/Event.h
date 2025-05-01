//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_EVENT_H
#define ND_EVENT_H

#include <cstdint>
#include <cassert>

namespace nd {

class Event 
{
public:
    enum class Type: uint8_t {
        NIL = 0,
        DATA,
        SET_BITRATE,        // see id_to_bitrate(), bitrate_to_id()
        DELAY_MS,           // Delay in milliseconds
        DELAY_US,           // Delay in microseconds
        DELAY_CHAR,         // Delay in characters at the current bitrate
        ERROR = 0xff,       // Not an event, but an error message.
    };

private:
    union {
        uint32_t event_ = 0;
        struct {
            Type type_;
            uint8_t subtype_;
            uint16_t data_;
        };
    };

    static uint32_t id_to_bitrate(uint8_t id);
    static uint8_t bitrate_to_id(uint32_t bitrate);
public:
    Event() = default;
    constexpr Event(Type type) : type_(type), subtype_(0), data_(0) {}
    constexpr Event(uint8_t data) : type_(Type::DATA), subtype_(0), data_(data) {}
    constexpr Event(Type type, uint32_t data) : type_(type), subtype_(0), data_(data) {}
    ~Event() = default;
    // Event(const Event&) = delete;
    // Event& operator=(const Event&) = delete;
    // Event(Event&&) = delete;
    // Event& operator=(Event&&) = delete;

    bool is_data() const { return type_ == Type::DATA; }

    static Event make_bitrate_event(uint32_t bitrate);
    uint32_t get_bitrate() const { return id_to_bitrate(data_); }

    Type type() const { return type_; }
    void type(Type t) { type_ = t; }
    uint32_t data() const { return data_; }
    void data(uint32_t d) { data_ = d; }
};

static_assert(sizeof(Event) == 4, "Event class size must be 4 bytes");


class Result 
{
public:
    enum class Type: uint8_t {
        OK = 0,
        REJECTED,
    };
    enum class Cause: uint8_t {
        UNDEFINED = 0,
        NOT_CONNECTED,
    };

private:
    union {
        uint32_t result_ = 0;
        struct {
            Type type_;
            Cause cause_;
            uint16_t data_;
        };
    };

public:
    Result() = default;
    constexpr Result(Type type, uint32_t data) : type_(type), cause_(Cause::UNDEFINED), data_(data) {}
    constexpr Result(Type type, Cause cause, uint32_t data=0) : type_(type), cause_(cause), data_(data) {}
    ~Result() = default;

    static Result OK;
    static Result OK__NOT_CONNECTED;
    static Result REJECTED;

    Type type() const { return type_; }
    void type(Type t) { type_ = t; }
    Cause cause() const { return cause_; }
    void cause(Cause c) { cause_ = c; }
    uint32_t data() const { return data_; }
    void data(uint32_t d) { data_ = d; }

    bool ok() const { return type_ == Type::OK; }
    bool rejected() const { return type_ == Type::REJECTED; }
};

static_assert(sizeof(Result) == 4, "Result class size must be 4 bytes");



#if 0

enum class Test { };

enum class Type: uint8_t {
    DATA = 0,
    SET_BITRATE,        // see id_to_bitrate(), bitrate_to_id()
    DELAY_MS,           // Delay in milliseconds
    DELAY_US,           // Delay in microseconds
    DELAY_CHAR,         // Delay in characters at the current bitrate
    ERROR = 0xff,       // Not an event, but an erroro message.
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
Event make_delay(uint32_t delay_us);

uint32_t id_to_bitrate(uint8_t);
uint8_t bitrate_to_id(uint32_t);

uint32_t fp8_to_uint20(uint8_t id);
uint8_t uint20_to_fp8(uint32_t us);
    
#endif

} // namespace nd

#endif // ND_EVENT_H