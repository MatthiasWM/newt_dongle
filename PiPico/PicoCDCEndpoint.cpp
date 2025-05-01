//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoCDCEndpoint.h"
#include "common/Pipe.h"
#include "common/Scheduler.h"

#include "pico/stdlib.h"
#include "bsp/board_api.h"
#include "tusb.h"

#include <stdio.h>

using namespace nd;

// TODO: make the interface complete:
// bool tud_cdc_ready(void)  ( bool tud_cdc_n_ready(void) )
// bool tud_cdc_connected(void)
// uint8_t tud_cdc_get_line_state(void)


PicoCDCEndpoint *PicoCDCEndpoint::list_[] = {  };

PicoCDCEndpoint *PicoCDCEndpoint::instance(uint32_t index) {
    if (index < 4) {
        return list_[index];
    }
    return nullptr;
}

PicoCDCEndpoint::PicoCDCEndpoint(Scheduler &scheduler, uint32_t index)
:   UARTEndpoint(scheduler),
    index_(index)
{
    if (index < 4) {
        list_[index] = this;
    } else {
        printf("**** ERROR ****: PicoCDCEndpoint: index out of range\n");
    }
}

PicoCDCEndpoint::~PicoCDCEndpoint() {
    // Destructor implementation
}

Result PicoCDCEndpoint::init() {
    UARTEndpoint::init();
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(index_, &dev_init);
    
    if (board_init_after_tusb) {
        board_init_after_tusb();
    }    
    return Result::OK;
}

/**
 * \brief Called regularly by the scheduler to take care of the CDC USB device.
 * 
 * Peek for data that arrived at the CDC and needs to be sent to the pipe.
 * Other events (e.g. bitrate changes) are handled in TinyUSBTask which calls
 * tud_task() in every time slice.
 * 
 * \return Result::OK
 * 
 * \todo Notify the user of a buffer overflow.
 */
Result PicoCDCEndpoint::task() {
    // Let the caller know if we are not connected to a pipe.
    if (out() == nullptr)
        return Result::OK__NOT_CONNECTED;

    // Check if we have data in the USB buffer and send it to the pipe.
    uint8_t data;
    if (tud_cdc_n_peek(index_, &data)) {
        Event data_event { data };
        Result r = out()->send(data_event);
        // If the pipe accepted the data, remove it from the internal CDC buffer.
        if (r.ok()) 
            tud_cdc_n_read_char(index_);
    }

    // Check here if we need to flush data to the USB.
    // The USB port may already have self-flushed (buffer size is typically 
    // 64 bytes), but we can't verify that, and there is no harm in 
    // flushing again, just to be sure
    if (tx_num_pending_ > 0) {
        uint32_t cycle_time = scheduler().cycle_time();
        if (tx_timeout_ < cycle_time) {
            tud_cdc_n_write_flush(index_);
            tx_num_pending_ = 0;
            tx_timeout_ = 0;
        } else {
            tx_timeout_ -= cycle_time;
        }
    }

    // Everything went fine.
    return Result::OK;
}

/**
 * \brief Called whenever the pipe has data to send to the USB device.
 * 
 * Data events are forwarded to the USB device. Other events are ignored.
 * 
 * \param event The event to send to the USB device.
 * \return Result::OK if the event was handled. Result::REJECTED if the event 
 * must be resent later by the caller.
 */
Result PicoCDCEndpoint::send(Event event) {
    if (event.type() == Event::Type::DATA) {
        if (tud_cdc_n_write_available(index_) > 0) {
            tud_cdc_n_write_char(index_, event.data());
            if (tx_num_pending_ == 0)
                tx_timeout_ = 1000; // microseconds
            tx_num_pending_++;
            return Result::OK;
        } else {
            return Result::REJECTED;
        }
    } else {
        return Result::OK;
    }
}

Result PicoCDCEndpoint::rush(Event event) {
    return UARTEndpoint::rush(event);
}

void PicoCDCEndpoint::set_bitrate(uint32_t new_bitrate) {
    UARTEndpoint::set_bitrate(new_bitrate);
}

