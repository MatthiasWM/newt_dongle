//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_FILTERS_MNP_FILTER_H
#define ND_FILTERS_MNP_FILTER_H

#include "common/Task.h"

#include <vector>
#include <array>
#include <queue>


namespace nd {

constexpr uint8_t kMNP_Frame_Unknown = 0;
constexpr uint8_t kMNP_Frame_LR = 1; // Link Request
constexpr uint8_t kMNP_Frame_LD = 2; // Link Disconnect
constexpr uint8_t kMNP_Frame_LT = 4; // Link Transfer
constexpr uint8_t kMNP_Frame_LA = 5; // Link Acknowledgement
constexpr uint8_t kMNP_Frame_LN = 6; // Link Attention (not supported)
constexpr uint8_t kMNP_Frame_LNA = 7; // Link Attention Acknowledgement (not supported)


class MNPFrame;
class MNPFilter;

class NewtToDockPipe : public Pipe {
    friend class MNPFilter;
    std::queue<Event> job_list_;

    MNPFrame *in_frame_ = nullptr;
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

    uint8_t in_seq_no_ = 0;

    MNPFrame *out_frame_ = nullptr;
    uint16_t out_frame_crsr_ = 0;

    void mnp_to_dock_state_machine();
    void start_next_job();
    void handle_newt_frame(MNPFrame *frame);

public:
    MNPFilter &filter_;
    NewtToDockPipe(MNPFilter &filter) : filter_(filter) { }
    void task();
    Result send(Event event) override;
    Result rush(Event event) override;
	// TODO: rush_back()
    void add_job(Event event) { job_list_.push(event); }
    void reset();
};

class DockToNewtPipe : public Pipe {
    std::queue<Event> job_list_lt_;     // List all LT jobs here in a fifo.
    std::queue<Event> job_list_;        // Queue all other jobs here.

    // State machine for sending MNP frames to the Newton.
    MNPFrame *active_frame_ = nullptr;
    MNPFrame *ack_pending_frame_ = nullptr;
    enum class OutState {
        SEND_SYN,
        SEND_DLE,
        SEND_STX,
        SEND_HDR_SIZE,
        SEND_HDR_SIZE_DLE,
        SEND_HDR_DATA,
        SEND_HDR_DATA_DLE,
        SEND_DATA,
        SEND_DATA_DLE,
        SEND_DLE2,
        SEND_ETX,
        SEND_CRC_lo,
        SEND_CRC_hi,
    } out_state_ = OutState::SEND_SYN;
    uint16_t data_crsr_ = 0;
    uint8_t seq_no_ = 0; // sequence number for LT frames
    uint64_t ack_deadline_ = 0; // resend `ack_pending_frame_` at this time
    uint8_t retransmits_ = 0; // number of times we resent `ack_pending_frame_`

    MNPFrame *in_frame = nullptr;


    void mnp_to_newt_state_machine();
    void release_frame(MNPFrame *frame);
    void retain_until_ack();
    void handle_ack_timeout();
    void start_next_job();
    void flush_in_frame();
    void acknowledge_frame(uint8_t seq);

public:
    MNPFilter &filter_;
    DockToNewtPipe(MNPFilter &filter) : filter_(filter) { }
    void task();
    Result send(Event event) override;
	// TODO: rush()
	// TODO: rush_back()
    void add_job(Event event);
    void reset();
};


class MNPFilter : public Task 
{
    typedef Task super;
    friend class NewtToDockPipe;
    friend class DockToNewtPipe;

    constexpr static int kPoolSize = 8;
    constexpr static uint32_t kMaxData = 251; // DCL: 253???
    // constexpr static uint32_t kMaxData = 256; // DCL: 253???
    constexpr static uint32_t kMaxInData = 256; // Our LR allows 256 data bytes per LT (parameter 8).

    // -- MNP state machine
    enum class MNPState {
        DISCONNECTED,
        NEGOTIATING,
        CONNECTED,
    } mnp_state_ = MNPState::DISCONNECTED;

    uint8_t dock_state_ = 0;

    NewtToDockPipe newt_;       // Pipe from the Newton endpoint to the Dock.
    DockToNewtPipe dock_;       // Pipe from the Dock endpoint to the Newton.
    std::array<MNPFrame*, kPoolSize> frame_pool_; // A pool with a fixed number of frames.

    MNPFrame *acquire_frame();
    void release_frame(MNPFrame *frame);

public:
    /// Resend an LT if the Newton did not acknowledge it within this time (usec).
    constexpr static uint64_t kRetransmitTimeout = 3'000'000;
    /// Disconnect after resending an LT this many times without an LA.
    constexpr static uint8_t kMaxRetransmits = 4;

    /// Publicly accessible pipe from the Newton endpoint to the Dock.
    Pipe &newt;
    /// Publicly accessible pipe from the Dock endpoint to the Newton.
    Pipe &dock;

    // -- Ctor and Dtor
    MNPFilter(Scheduler &scheduler);
    ~MNPFilter() override;

    // -- Override Task methods
    Result task() override;
    Result send(Event event) override;

    void set_disconnected();
    void set_negotiating();
    void set_connected();
};

} // namespace nd

#endif // ND_FILTERS_MNP_FILTER_H