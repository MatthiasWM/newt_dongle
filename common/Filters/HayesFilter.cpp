//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "HayesFilter.h"

using namespace nd;

/**
 * \brief Create a new Hayes filter.
 * 
 * The Hayes filter adds Hayes modem functionality via AT commands to the line.
 * The filter has two pipes: Upstream is the pipe from the user to the modem,
 * or in this case, from the MP to the dongle, or from the PC to the dongle.
 * Downstream is the pipe from the domgle to any device.alignas
 * 
 * To get the filter into command mode, send pause-"+++"-pause to th Upstream
 * pipe. The filter will then send a response to the downstream pipe with "OK".
 * 
 * To go back into data mode, send "ATO".
 * 
 * \param scheduler The scheduler to use for this filter.
 */
HayesFilter::HayesFilter(Scheduler &scheduler)
:   Task(scheduler),
    upstream(*this),
    downstream(*this)
{
}

HayesFilter::~HayesFilter() {
}

/**
 * \brief Called regularly by the scheduler to take care of the filter.
 */
Result HayesFilter::task() {
    // Nothing yet.
    return Result::OK;
}

/**
 * \brief Called whenever the upstream pipe has sent us an Event.
 * 
 * Upstream is always the dongle iself, as it behaves like a modem to the MP,
 * either via UART or USB.
 */
Result HayesFilter::upstream_send(Event event) {
    if (data_mode_) {
        // Forward the events down stream.
        Pipe *down = downstream.out();
        if (!down)
            return Result::OK__NOT_CONNECTED;
        return down->send(event);
    } else {
        // Every event sent from upstream to downstream in command mode is lost.
        // \todo We can buffer stuff and send it later.
        // \todo We can send a "RING" Event to the downstream pipe on high water.
        return Result::OK__NOT_CONNECTED;
    }
}

/**
 * \brief Called whenever the downstream pipe has sent us an Event.
 * 
 * Downstream would be the MessagePad or the PC.
 */
Result HayesFilter::downstream_send(Event event) 
{
    Pipe *up = upstream.out();
    Pipe *down = downstream.out();

    // If the downstream pipe is not connected, we can't do anything.
    // Simplified mode change:
    if (event.type() == Event::Type::DATA) {
        uint8_t c = event.data();
        if (data_mode_ && (c == '+')) {
            data_mode_ = false;
            if (down) {
                down->send(Event('O'));
                down->send(Event('K'));
                down->send(Event('\r'));
                down->send(Event('\n'));
            }
            return Result::OK;
        } else if ((data_mode_ == false) && (c == '-')) {
            if (down) {
                down->send(Event('C'));
                down->send(Event('O'));
                down->send(Event('N'));
                down->send(Event('N'));
                down->send(Event('E'));
                down->send(Event('C'));
                down->send(Event('T'));
                down->send(Event('\r'));
                down->send(Event('\n'));
            }
            data_mode_ = true;
            return Result::OK;
        }
    }

    if (data_mode_) {
        // Forward the events up stream.
        // \todo Listen for pause-"+++"-pause to switch to command mode.
        if (!up)
            return Result::OK__NOT_CONNECTED;
        return up->send(event);
    } else {
        // For now, we just echo the events.
        // \todo Copy text into a command buffer and run the commands on reciving a CR.
        if (!down)
            return Result::OK__NOT_CONNECTED;
        return down->send(event);   
    }
}
