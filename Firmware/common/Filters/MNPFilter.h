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
class NewtToDockPipe;
class DockToNewtPipe;


class MNPFilter : public Task 
{
    typedef Task super;
    friend class NewtToDockPipe;
    friend class DockToNewtPipe;

    constexpr static int kPoolSize = 8;
    constexpr static uint32_t kMaxData = 35; // DCL: 253???
    // constexpr static uint32_t kMaxData = 256; // DCL: 253???

    // -- MNP state machine
    enum class MNPState {
        DISCONNECTED,
        NEGOTIATING,
        CONNECTED,
    } mnp_state_ = MNPState::DISCONNECTED;

    uint8_t dock_state_ = 0;

    NewtToDockPipe *newt_;      // Pipe from the Newton endpoint to the Dock.
    DockToNewtPipe *dock_;      // Pipe from the Dock endpoint to the Newton.
    std::array<MNPFrame*, kPoolSize> frame_pool_; // A pool with a fixed number of frames.

    MNPFrame *acquire_frame();
    void release_frame(MNPFrame *frame);

public:
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