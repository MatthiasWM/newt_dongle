//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Endpoint.h"

#include "Pipe.h"

#include "pico/stdlib.h"
#include <stdio.h>

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
