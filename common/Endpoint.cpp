//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Endpoint.h"

#include "Pipe.h"

using namespace nd;


Endpoint::Endpoint() {
    // Constructor implementation
}

Endpoint::~Endpoint() {
    // Destructor implementation
}

int Endpoint::init() {
    // Initialization implementation
    return 0;
}

int Endpoint::task() {
    // Task implementation
    return 0;
}

Endpoint &Endpoint::set_in(Pipe *pipe) {
    in_pipe_ = pipe;
    return *this;
}

Endpoint &Endpoint::set_out(Pipe *pipe) {
    out_pipe_ = pipe;
    return *this;
}
