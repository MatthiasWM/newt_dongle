//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "SDCardEndpoint.h"

using namespace nd; 

SDCardEndpoint::SDCardEndpoint(Scheduler &scheduler) 
:   Endpoint(scheduler)
{
    // Constructor implementation
}

SDCardEndpoint::~SDCardEndpoint() {
    // Destructor implementation
}

Result SDCardEndpoint::init() {
    // Initialization code for the SD card endpoint
    return Endpoint::init();
}

/**
 * \brief Handle events that were send via the `in` pipe.
 */
Result SDCardEndpoint::send(Event event) {
    // switch (event.type()) {
    //     case Event::Type::SET_BITRATE:
    //         set_bitrate(event.bitrate());
    //         return Result::OK;
    // }
    return Endpoint::send(event);
}


