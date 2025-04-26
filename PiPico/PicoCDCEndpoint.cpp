//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoCDCEndpoint.h"
#include "common/Pipe.h"
#include "common/CtrlBlock.h"

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

PicoCDCEndpoint::PicoCDCEndpoint(uint32_t index)
:   CDCEndpoint(),
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
    // \todo tud_cdc_ready
    // \todo tud_cdc_connected
    uint32_t i;

    // Read all data from the fifo and put it into out pipe
    Pipe *out_pipe = out();
    if (out_pipe) {
        for (i=32; i>0; --i) {
            if (out_pipe->is_writable() && tud_cdc_available()) {
                char c = tud_cdc_read_char();
                out_pipe->putc(c);
            } else {
                break;
            }
        }
    }

    Pipe *in_pipe = in();
    if (in_pipe) {
        bool flush = false;
        // Read all data from the in pipe and send it to the USB
        for (i=32; i>0; --i) {
            if (in_pipe->data_available() /* && tud_cdc_connected()*/) {
                char c = in_pipe->getc();
                //printf("<%02x>", c);
                tud_cdc_write_char(c);
                flush = true;
            } else {
                break;
            }
        }

        // Flush the data to the USB 
        // \todo This should be on a timer so we don't flush too often
        if (flush) {
            tud_cdc_write_flush();
        }

        // Now check if there are any control blocks pending (handle one at a time)
        if (in_pipe->ctrl_available()) {
            CtrlBlock *ctrl_block = in_pipe->peek_ctrl();
            //printf("CDC Ctrl: %d %d %d %d %d\n", ctrl_block->cmd(), ctrl_block->d0(), ctrl_block->d1(), ctrl_block->d2(), ctrl_block->d3());
            in_pipe->pop_ctrl();
        }
    }
    return 0;
}

/**
 * Tell our client that the host has changed the bitrate.
 */
void PicoCDCEndpoint::host_set_bitrate(uint32_t new_bitrate) {
    Pipe *out_pipe = out();
    if (out_pipe) {
       out_pipe->put_ctrl(Cmd::SET_BITRATE, new_bitrate, 0, 0, 0);
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
