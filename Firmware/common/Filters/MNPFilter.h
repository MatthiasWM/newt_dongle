//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_FILTERS_MNP_FILTER_H
#define ND_FILTERS_MNP_FILTER_H

#include "common/Task.h"

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
constexpr uint8_t kMNP_Frame_LN = 6; // Link Attention (not supported)
constexpr uint8_t kMNP_Frame_LNA = 7; // Link Attention Acknowledgement (not supported)

class MNPFilter : public Task 
{
    typedef Task super;

    constexpr static int kPoolSize = 4;
    constexpr static uint32_t kMaxData = 256; // DCL: 253???

    class DockPipe: public Pipe {
    public:
        MNPFilter &filter_;
        DockPipe(MNPFilter &filter) : filter_(filter) { }
        Result send(Event event) override { return filter_.dock_send(event); }
    };

    class Frame {
    public:
        std::vector<uint8_t> header;
        std::vector<uint8_t> data;
        uint8_t header_size = 0;
        uint16_t crc = 0;
        uint8_t type = 0;
        uint8_t pool_index = 0;
        bool escaping_dle = false;
        bool crc_valid = false;
        bool in_use = false;

        Frame();
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
        ABORT,                  // clear the in_buffer and start over
        WAIT_FOR_SYN,           // synchronous idle
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
    std::queue<Event> newton_job_list;
    std::queue<Event> dock_job_list;

    uint8_t dock_state_ = 0;

    Frame *dock_out_frame = nullptr;
    uint16_t dock_out_index_ = 0;

    Frame *dock_in_frame = nullptr;
    void flush_dock_in_frame();

public:
    DockPipe dock { *this };

    MNPFilter(Scheduler &scheduler);
    ~MNPFilter() override = default;
    Result task() override;
    Result send(Event event) override;
    Result signal(Event event) override;

    Result dock_send(Event event);

    Frame *acquire_in_frame();
    void release_in_frame(Frame *frame);
    Frame *acquire_out_frame();
    void release_out_frame(Frame *frame);

    void handle_valid_in_frame(Frame *frame);

    void prepare_LR_frame(Frame *lr, uint8_t in_index);
    void prepare_LD_frame(Frame *ld, uint8_t reason);
    void prepare_LA_frame(Frame *la, uint8_t seq_no);

    void state_machine_reply(Frame *frame);

    void set_disconnected();
    void set_negotiating();
    void set_connected();
};

} // namespace nd

#endif // ND_FILTERS_MNP_FILTER_H