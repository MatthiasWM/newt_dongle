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
    Pipe *in_ = nullptr;
    Pipe *out_ = nullptr;
    bool active_ = true;
protected:
    void deactivate() { active_ = false; }
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
    Pipe *in() const;
    Pipe *out() const;

    // -- Writing to the next pipe
    virtual Result send(Event event);
    virtual Result rush(Event event);
    virtual Result rush_back(Event event);
}; 

} // namespace nd

#endif // ND_PIPE_H