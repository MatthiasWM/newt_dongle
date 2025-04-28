//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_ENDPOINT_H
#define ND_ENDPOINT_H

#include "Event.h"

namespace nd {

class Pipe;

class Endpoint {
    Pipe *in_pipe_ = nullptr;
    Pipe *out_pipe_ = nullptr;
public:
    Endpoint();
    virtual ~Endpoint();

    virtual int init();
    virtual int task();

    Endpoint &set_in(Pipe *pipe);
    Endpoint &set_out(Pipe *pipe);
    Pipe *in() const { return in_pipe_; }
    Pipe *out() const { return out_pipe_; }
};


} // namespace nd

#endif // ND_ENDPOINT_H