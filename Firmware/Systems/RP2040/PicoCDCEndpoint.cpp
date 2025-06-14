//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoCDCEndpoint.h"

#include "main.h"
#include "common/Pipe.h"
#include "common/Scheduler.h"

#include <pico/stdlib.h>
#include <bsp/board_api.h>
#include <tusb.h>

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
        if (kDebugErrors) printf("**** ERROR ****: PicoCDCEndpoint: index out of range\n");
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
 * \brief Called whenever the in pipe has sent us an Event.
 * 
 * Data events are forwarded to the USB device. All other events are 
 * currently ignored.
 * 
 * \param event The event to send to the USB device.
 * \return Result::OK if the event was handled. Result::REJECTED if the event 
 * must be resent again later by the caller.
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

void PicoCDCEndpoint::set_bitrate(uint32_t new_bitrate) {
    UARTEndpoint::set_bitrate(new_bitrate);
}

// ------------------------------------------------------------------------------

// // Invoked when received new data
// TU_ATTR_WEAK void tud_cdc_rx_cb(uint8_t itf);

// // Invoked when received `wanted_char`
// TU_ATTR_WEAK void tud_cdc_rx_wanted_cb(uint8_t itf, char wanted_char);

// // Invoked when a TX is complete and therefore space becomes available in TX buffer
// TU_ATTR_WEAK void tud_cdc_tx_complete_cb(uint8_t itf);

// // Invoked when line state DTR & RTS are changed via SET_CONTROL_LINE_STATE
// TU_ATTR_WEAK void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts);

// // Invoked when line coding is change via SET_LINE_CODING
// TU_ATTR_WEAK void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding);
// p_line_coding->bit_rate, p_line_coding->stop_bits, p_line_coding->parity, p_line_coding->data_bits

// // Invoked when received send break
// // \param[in]  itf  interface for which send break was received.
// // \param[in]  duration_ms  the length of time, in milliseconds, of the break signal. If a value of FFFFh, then the
// //                          device will send a break until another SendBreak request is received with value 0000h.
// TU_ATTR_WEAK void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms);

// We can use hardware spin locks for interrupt handling:
// void spin_lock_claim (uint lock_num)
// void spin_lock_unclaim (uint lock_num)
// spin_lock_t * spin_lock_init (uint lock_num)
// Higher level pico_sync functions: critical section, mutex, etc.

// All callbacks are called from within tud_task(), the TinyUSB device taks,
// so we don't need to worry about thread safety.

/**
 * USB Host changed the connection speed.
 * 
 * Forward this information by sending a control block to the UART endpoint.
 */
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding) {
    PicoCDCEndpoint *cdc = PicoCDCEndpoint::instance(itf);
    if (cdc) {
        cdc->device_changed_bitrate(p_line_coding->bit_rate);
    }
}

/**
 * \brief Called whenever the USB host has changed out bitrate.
 * 
 * Send a bitrate changed event down the pipe.alarm_pool_add_repeating_timer_us
 * 
 * \note This event is sent to the out pipe without priority. Since this is
 * a low frequency event, no measures are taken if the pipe rejects the event.
 * It would be nice to fix that!
 */
void PicoCDCEndpoint::device_changed_bitrate(uint32_t bitrate) {
    if (out()) {
        // TODO: if sending the event fails, we do nothing, instead we should resend it later...
        out()->send(Event::make_bitrate_event(bitrate));
        set_bitrate(bitrate);
    }
}


// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void)itf;
    (void)rts;

    PicoCDCEndpoint *cdc = PicoCDCEndpoint::instance(itf);

    if (kLogCDC) Log.logf("\n\nCDC line state %d %d\n", dtr, rts);

    if (dtr) {
        if (cdc && cdc->out()) {
            cdc->out()->rush(Event(Event::Type::UART, Event::Subtype::UART_DTR, 1));
        }
    } else {
        if (cdc && cdc->out()) {
            cdc->out()->rush(Event(Event::Type::UART, Event::Subtype::UART_DTR, 0));
        }
    }
}

// Invoked when CDC interface received data from host
// void tud_cdc_rx_cb(uint8_t itf) {
//     puts("CDC RX\n");
//   (void)itf;
// }