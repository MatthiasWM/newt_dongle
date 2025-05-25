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
        TEXT,
        SET_BITRATE,        // see id_to_bitrate(), bitrate_to_id()
        DELAY,              // Delay transmission of data for the given time.
        HIGH_WATER,         // Receiving endpoint buffer about to flood.
        SIGNAL,             // Signal from the scheduler.
        MNP,                // MNP protocol event.
        ERROR = 0xff,       // Not an event, but an error message.
    };
    enum class Subtype: uint8_t {
        NIL = 0,
        ON,
        OFF,
        RESET,
        USECS = 128,        // DELAY: Delay in microseconds   
        MSECS,              // DELAY: Delay in milliseconds       
        CHARS,              // DELAY: Delay in characters at the current bitrate 
        USER_SETTINGS_CHANGED = 128, // SIGNAL: User settings changed
        MNP_SEND_LA = 128,  // MNP: Send Link Acknowledgement (sequence number)
        MNP_SEND_LD,        // MNP: Send Link Disconnect (reason)
        MNP_SEND_LR,        // MNP: Send Link Request (in buffer index)
        MNP_SEND_LT,        // MNP: Send Link Transfer (out buffer index)
        MNP_RECEIVED_LA,    // MNP: Received Link Acknowledgement (sequence number)
        MNP_DATA_TO_DOCK,   // MNP: Stream data on to dock (in buffer index)
        MNP_NEGOTIATING,    // MNP: sent from MNP to Dock if a connection is about to be established
        MNP_CONNECTED,      // MNP: sent from MNP to Dock if a connection was established
        MNP_DISCONNECTED,   // MNP: sent from MNP to Dock if a connection was terminated
        MNP_FRAME_START,    // MNP: sent from MNP to Dock when the original data had a frame start
        MNP_FRAME_END,      // MNP: sent from MNP to Dock when the original data had a frame end
    };

private:
    union {
        uint32_t event_ = 0;
        struct {
            Type type_;
            Subtype subtype_;
            uint16_t data_;
        };
    };

    static uint32_t id_to_bitrate(uint8_t id);
    static uint8_t bitrate_to_id(uint32_t bitrate);
public:
    Event() = default;
    constexpr Event(Type type) : type_(type), subtype_(Subtype::NIL), data_(0) {}
    constexpr Event(uint8_t data) : type_(Type::DATA), subtype_(Subtype::NIL), data_(data) {}
    constexpr Event(Type type, uint32_t data) : type_(type), subtype_(Subtype::NIL), data_(data) {}
    constexpr Event(Type type, Subtype subtype, uint32_t data=0) : type_(type), subtype_(subtype), data_(data) {}
    ~Event() = default;
    // Event(const Event&) = delete;
    // Event& operator=(const Event&) = delete;
    // Event(Event&&) = delete;
    // Event& operator=(Event&&) = delete;

    bool is_data() const { return type_ == Type::DATA; }

    static Event make_bitrate_event(uint32_t bitrate);
    uint32_t bitrate() const { return id_to_bitrate(data_); }

    static Event make_delay_event(uint32_t usec);

    Type type() const { return type_; }
    void type(Type t) { type_ = t; }
    Subtype subtype() const { return subtype_; }
    void subtype(Subtype c) { subtype_ = c; }
    uint32_t data() const { return data_; }
    void data(uint32_t d) { data_ = d; }
    uint32_t raw() const { return event_; }
    void raw(uint32_t r) { event_ = r; }
};

static_assert(sizeof(Event) == 4, "Event class size must be 4 bytes");


class Result 
{
public:
    enum class Type: uint8_t {
        OK = 0,
        REJECTED,
    };
    enum class Subtype: uint8_t {
        UNDEFINED = 0,
        NOT_CONNECTED,      // No output pipe connected.
        NOT_HANDLED,            // Endpoint does not recognize this event type.
    };

private:
    union {
        uint32_t result_ = 0;
        struct {
            Type type_;
            Subtype subtype_;
            uint16_t data_;
        };
    };

public:
    Result() = default;
    constexpr Result(Type type, uint32_t data) : type_(type), subtype_(Subtype::UNDEFINED), data_(data) {}
    constexpr Result(Type type, Subtype subtype, uint32_t data=0) : type_(type), subtype_(subtype), data_(data) {}
    ~Result() = default;

    static Result OK;
    static Result OK__NOT_HANDLED;
    static Result OK__NOT_CONNECTED;
    static Result REJECTED;
    static Result REJECTED__NOT_CONNECTED;

    Type type() const { return type_; }
    void type(Type t) { type_ = t; }
    Subtype subtype() const { return subtype_; }
    void subtype(Subtype c) { subtype_ = c; }
    uint32_t data() const { return data_; }
    void data(uint32_t d) { data_ = d; }
    uint32_t raw() const { return result_; }
    void raw(uint32_t r) { result_ = r; }

    bool ok() const { return type_ == Type::OK; }
    bool rejected() const { return type_ == Type::REJECTED; }
};

static_assert(sizeof(Result) == 4, "Result class size must be 4 bytes");

} // namespace nd

#endif // ND_EVENT_H