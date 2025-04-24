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

PicoCDCEndpoint::PicoCDCEndpoint() {
    // Constructor implementation
}

PicoCDCEndpoint::~PicoCDCEndpoint() {
    // Destructor implementation
}

int PicoCDCEndpoint::init() {
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);
    
    if (board_init_after_tusb) {
        board_init_after_tusb();
    }    
    printf("Pico CDC Endpoint initialized.\n");
    // No error
    return 0;
}

int PicoCDCEndpoint::task() {
    // The score ensures that we don't get stuck in an infinite loop
    int score = 1000;
    bool flush = false;
    bool busy = true;
    // \todo tud_cdc_ready
    // \todo tud_cdc_connected
    while (busy && (score > 0)) {
        busy = false;
        while ((score > 0) && tud_cdc_available()) {
            char c = tud_cdc_read_char();
            // char c = '!';
            // uint32_t count = tud_cdc_read(&c, 1);
            // if (count != 1) break;
            busy = true;
            printf("Received: %c\n", c);
            Pipe *out_pipe = out();
            if ((out_pipe == nullptr) || (!out_pipe->is_writable())) break;
            out_pipe->putc(c);
            score--;
        }
        Pipe *in_pipe = in();
        while ((score > 0) && (in_pipe) && (in_pipe->data_available()>0)) {
            busy = true;
            char c = in_pipe->getc();
            printf("Sending: %c\n", c);
            tud_cdc_write_char(c);
            flush = true;
            score--;
        }
        while ((score > 0) && (in_pipe) && (in_pipe->ctrl_available())) {
            busy = true;
            CtrlBlock *ctrl_block = in_pipe->peek_ctrl();
            printf("Ctrl: %d %d %d %d %d\n", ctrl_block->cmd(), ctrl_block->d0(), ctrl_block->d1(), ctrl_block->d2(), ctrl_block->d3());
            in_pipe->pop_ctrl();
            score -= 100;
        }
    }
    if (flush)
        tud_cdc_write_flush();
    return 0;
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


// Invoked when cdc when line state changed e.g connected/disconnected
// void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
//   (void)itf;
//   (void)rts;

//   // TODO set some indicator
//   if (dtr) {
//     // Terminal connected
//     puts("CDC Terminal Connected\n");
//   } else {
//     // Terminal disconnected
//     puts("CDC Terminal Disconnected\n");
//   }
// }

// Invoked when CDC interface received data from host
// void tud_cdc_rx_cb(uint8_t itf) {
//     puts("CDC RX\n");
//   (void)itf;
// }
