//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_DOCK_H
#define ND_PICO_DOCK_H

#include "common/Endpoint.h"

#include <vector>
#include <queue>

#ifndef ND_FOURCC
#  if __BYTE_ORDER == __LITTLE_ENDIAN 
#    define ND_FOURCC(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#  elif __BYTE_ORDER == __BIG_ENDIAN
#    define ND_FOURCC(a, b, c, d) ((uint32_t)(d) | ((uint32_t)(c) << 8) | ((uint32_t)(b) << 16) | ((uint32_t)(a) << 24))
#  else
#    error "Can't determine endianess"
#  endif
#endif

namespace nd {

constexpr uint8_t kMNP_Frame_LR = 1; // Link Request
constexpr uint8_t kMNP_Frame_LD = 2; // Link Disconnect
constexpr uint8_t kMNP_Frame_LT = 4; // Link Transfer
constexpr uint8_t kMNP_Frame_LA = 5; // Link Acknowledgement
constexpr uint8_t kMNP_Frame_LN = 6; // Link Attention
constexpr uint8_t kMNP_Frame_LNA = 7; // Link Attention Acknowledgement

class PicoDock : public Endpoint 
{
    constexpr static int kPoolSize = 4;

    class Frame {
    public:
        std::vector<uint8_t> header;
        std::vector<uint8_t> data;
        uint8_t header_size = 0;
        uint16_t crc = 0;
        uint8_t type = 0;
        bool escaping_dle = false;
        bool crc_valid = false;
        bool busy = false;

        Frame();
        Frame(const std::vector<uint8_t> &preset_header, const std::vector<uint8_t> &preset_data);
        void clear();
        uint16_t calculate_crc();
        void prepare_to_send();
        void print();
    };

    // -- "in" data, packages comming from the Newton directed to the Dock
    Frame in_pool[kPoolSize];
    Frame *in_frame = nullptr;
    uint8_t in_seq_no_ = 0;
    enum class InState {  // TODO: move to class Frame?
        ABORT,
        WAIT_FOR_SYN = ABORT,   // synchronous idle
        WAIT_FOR_DLE,           // data link escape
        WAIT_FOR_STX,           // start of text
        WAIT_FOR_HDR_SIZE,      // 0..254
        WAIT_FOR_HDR_TYPE,      // LR, LD, LT, or LA
        WAIT_FOR_HDR_DATA,      // header data
        WAIT_FOR_DATA,          // data link escape ends data
        WAIT_FOR_ETX,           // end of text
        WAIT_FOR_CRC_lo,        // low byte of CRC
        WAIT_FOR_CRC_hi,        // high byte of CRC
    } in_state_ = InState::WAIT_FOR_SYN;

    // -- "out" data, packages produced by this class directed to the Newton
    Frame out_pool[kPoolSize];
    Frame *out_frame = nullptr;
    uint8_t out_seq_no_ = 0;
    uint16_t data_index_ = 0;
    enum class OutState { // TODO: move to class Frame?
        SEND_SYN,
        SEND_DLE,
        SEND_STX,
        SEND_HDR_SIZE,
        SEND_HDR_DATA,
        SEND_DATA,
        SEND_DLE2,
        SEND_ETX,
        SEND_CRC_lo,
        SEND_CRC_hi,
    } out_state_ = OutState::SEND_SYN;

    // -- MNP state machine
    enum class MNPState {
        DISCONNECTED,
        NEGOTIATING,
        CONNECTED,
    } mnp_state_ = MNPState::DISCONNECTED;
    std::queue<Event> job_list;

public:
    PicoDock(Scheduler &scheduler);
    ~PicoDock() override;
    Result init() override;
    Result task() override;
    Result send(Event event) override;
    Result signal(Event event) override;

    void handle_valid_in_frame(Frame *frame);
    void handle_LT(Frame *frame);

    void reply_to_LR(Frame *frame);
    void reply_with_LD();
    void reply_with_LA();
    void reply_newtdockdock();

    void state_machine_reply(Frame *frame);

    void set_disconnected();
    void set_negotiating();
    void set_connected();
};

} // namespace nd

#endif // ND_PICO_DOCK_H