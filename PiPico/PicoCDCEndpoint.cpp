//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoCDCEndpoint.h"
#include "common/Pipe.h"

#include "pico/stdlib.h"
#include "bsp/board_api.h"
#include "tusb.h"

#include <stdio.h>

using namespace nd;

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
    return UARTEndpoint::init();
}

Result PicoCDCEndpoint::task() {
    return UARTEndpoint::task();
}

Result PicoCDCEndpoint::send(Event event) {
    return UARTEndpoint::send(event);
}

Result PicoCDCEndpoint::rush(Event event) {
    return UARTEndpoint::rush(event);
}

void PicoCDCEndpoint::set_bitrate(uint32_t new_bitrate) {
    UARTEndpoint::set_bitrate(new_bitrate);
}


#if 0

int PicoCDCEndpoint::init() {
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(index_, &dev_init);
    
    if (board_init_after_tusb) {
        board_init_after_tusb();
    }    
    // printf("Pico CDC Endpoint initialized.\n");
    return 0;
}

int PicoCDCEndpoint::task() {
    uint32_t i;

    // Read all data from the FIFO and put it into out pipe
    Pipe *out_pipe = out();
    if (out_pipe) {
        // Handle a bunch of DATA events.
        for (i=cfgMaxBurstRead; i>0; --i) {
            if ((out_pipe->num_free() > 0) && tud_cdc_available()) {
                uint8_t c = tud_cdc_read_char();
                //printf("[%02x]", c);
                out_pipe->write(make_event(Type::DATA, c));
            } else {
                break;
            }
        }
    }

    // Read all data from the in pipe and send it to the device
    Pipe *in_pipe = in();
    if (in_pipe) {
        Event ev = in_pipe->peek();
        // Handle a bunch of DATA events first.
        bool flush = false;
        for (i=cfgMaxBurstWrite; i>0; --i) {
            if ((event_type(ev) == Type::DATA) /* && tud_cdc_connected()*/) {
                uint8_t c = event_data(ev);
                //printf("<%02x>", c);
                tud_cdc_write_char(c);
                in_pipe->pop();
                flush = true;
                ev = in_pipe->peek();
            } else {
                break;
            }
        }
        // Flush the data to the USB 
        // \todo This should be on a timer so we don't flush too often
        if (flush) {
            tud_cdc_write_flush();
        }
        // Now handle one control event if available.
        if ((event_type(ev) != Type::ERROR) && (event_type(ev) != Type::DATA)) {
            handle(ev);
            in_pipe->pop();
        }
        // Note that ERROR events must never be popped from the pipe.
    }
    return 0;
}

int PicoCDCEndpoint::handle(Event event) {
    switch (event_type(event)) {
        default:
            printf("**** WARNING ****: CDC: Unknown command %d ( %d )\n", 
                static_cast<int>(event_type(event)),
                event_data(event));
            return -1;
    }
    return -1;
}


/**
 * Tell our client that the host has changed the bitrate.
 */
void PicoCDCEndpoint::host_set_bitrate(uint32_t new_bitrate) {
    Pipe *out_pipe = out();
    if (out_pipe) {
        uint8_t bitrate_id = bitrate_to_id(new_bitrate);
        Event ev = make_event(Type::SET_BITRATE, bitrate_id);
        out_pipe->write(ev);
    }
}


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

// We can use hardware run locks for interrupt handling:
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
        cdc->host_set_bitrate(p_line_coding->bit_rate);
    }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  (void)itf;
  (void)rts;

  // TODO set some indicator
  if (dtr) {
    // Terminal connected
    printf("CDC Terminal Connected\n");
  } else {
    // Terminal disconnected
    printf("CDC Terminal Disconnected\n");
  }
}

// Invoked when CDC interface received data from host
// void tud_cdc_rx_cb(uint8_t itf) {
//     puts("CDC RX\n");
//   (void)itf;
// }

#endif