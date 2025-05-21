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

/**
 * \brief Handle events that were send via the `in` pipe.
 */
Result UARTEndpoint::send(Event event) {
    switch (event.type()) {
        case Event::Type::SET_BITRATE:
            set_bitrate(event.bitrate());
            return Result::OK;
    }
    return Endpoint::send(event);
}

// -- UART specific methods

void UARTEndpoint::set_bitrate(uint32_t bitrate) {
    bitrate_ = bitrate;
}

uint32_t UARTEndpoint::bitrate() const {
    return bitrate_;
}

