//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Tee.h"

using namespace nd;

/**
 * @brief A pipe splitter that forwards events to two separate pipes
 *
 * The Tee class acts as a T-junction in a pipeline, allowing events to be
 * sent to two different pipes simultaneously. Any event sent through this pipe
 * will be forwarded to both pipe 'out' and pipe 'b'. Rushed events are only  
 * sent to the main pipe.
 *
 * @see Pipe The parent class that defines the basic pipe interface
 */

/**
 * @brief Sends an event to both destinations in the tee.
 *
 * Output 'out' is the primary destination, while output 'b' is a secondary
 * destination. The event is sent to both destinations, but only the result
 * from destination 'out' is returned. 
 *
 * @param event The event to be sent to both destinations
 * @return Result The result of sending the event to destination 'out'
 */
Result Tee::send(Event event) {
    Result r = Result::OK__NOT_CONNECTED;
    if (out())
        r = out()->send(event);
    if (b.out())
        b.send(event);
    return r;
}

/**
 * @brief Rushes an event through the Tee junction.
 * 
 * Output 'out' is the primary destination. The event is rushed to 'out', but not
 * sent to the secondary destination 'b'.
 * 
 * @param event The event to be rushed through the Tee
 * @return Result Status of the rush operation
 */
Result Tee::rush(Event event) {
    if (out() == nullptr)
        return Result::REJECTED__NOT_CONNECTED;
    return out()->rush(event);
}
