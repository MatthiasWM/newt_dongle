//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "UARTEndpoint.h"

using namespace nd; 

UARTEndpoint::UARTEndpoint(Scheduler &scheduler) 
:   Endpoint(scheduler)
{
    // Constructor implementation
}

UARTEndpoint::~UARTEndpoint() {
    // Destructor implementation
}

Result UARTEndpoint::init() {
    // Initialization implementation
    return Result::OK;
}

Result UARTEndpoint::task() {
    // Task implementation
    return Result::OK;
}

Result UARTEndpoint::send(Event event) {
    return Result::OK;
}

Result UARTEndpoint::rush(Event event) {
    return Result::OK;
}

// -- UART specific methods
void UARTEndpoint::set_bitrate(uint32_t bitrate) {
    bitrate_ = bitrate;
}

uint32_t UARTEndpoint::get_bitrate() const {
    return bitrate_;
}
