//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PIPE_H
#define ND_PIPE_H

#include "nd_config.h"
#include "Event.h"

namespace nd {

class Pipe {
    Pipe *out_ = nullptr;
public:
    Pipe() = default;
    virtual ~Pipe() = default;
    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;
    Pipe(Pipe&&) = delete;
    Pipe& operator=(Pipe&&) = delete;

    // -- Pipe Routing
    Pipe &operator>>(Pipe &pipe);
    void disconnect();

    // -- Writing to the next pipe
    virtual Result send(Event event);
    virtual Result rush(Event event);
}; 

    
#if 0    

#include "pico/time.h"

class Endpoint;

class Pipe {
    Endpoint *source_ = nullptr;
    Endpoint *target_ = nullptr;
    Event buffer_[cfgRingBufferSize];
    absolute_time_t prev_event_time_ = nil_time;
    uint32_t head_ = 0;
    uint32_t tail_ = 0;

public:
    // -- Construction
    Pipe();
    ~Pipe() = default;
    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;
    Pipe(Pipe&&) = delete;
    Pipe& operator=(Pipe&&) = delete;

    // -- Pipe Routing
    Pipe &connect_from(Endpoint &endpoint);
    Pipe &connect_to(Endpoint &endpoint);
    Pipe &disconnect();

    // -- Reading the Pipe
    int32_t num_avail() const;
    Event peek() const;
    Event read();
    void pop();

    // -- Writing the Pipe
    int32_t num_free() const;
    Error write(Event event);
    Error prepend(Event event); // For debugging only!

    // -- Buffer Management
    bool reached_high_water() const;
    void flush();
};

#endif

} // namespace nd

#endif // ND_PIPE_H