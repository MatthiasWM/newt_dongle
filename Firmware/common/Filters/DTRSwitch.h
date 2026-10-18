//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_FILTERS_DTR_SWITCH_H
#define ND_FILTERS_DTR_SWITCH_H

#include "common/Pipe.h"

namespace nd {

class DTRSwitch: public Pipe {
    typedef Pipe super;
    friend class ToDockPipe;
    friend class ToCDCPipe;

    class ToDockPipe *dock_ = nullptr; // Pipe to the Dock emulator
    class ToCDCPipe *cdc_ = nullptr;   // Pipe to the USB CDC port
    uint32_t host_bitrate_ = 0;        // last bitrate set by the USB host, 0 if none yet
    uint32_t pending_bitrate_ = 0;     // bitrate the UART side did not accept yet, 0 if none

    Result send_pending_bitrate_();
public:
    bool dtr_set = false; // If true, route to CDC, else route to Dock.

    DTRSwitch();
    ~DTRSwitch() override;

    /// Publicly accessible pipe to the Dock emulator.
    Pipe &dock;
    /// Publicly accessible pipe to the USB CDC port.
    Pipe &cdc;

    // -- Pipe Stuff
    Result send(Event event) override;
    Result rush(Event event) override;
    Result rush_back(Event event) override;

};

} // namespace nd

#endif // ND_FILTERS_DTR_SWITCH_H