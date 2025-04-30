//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_DEVICES_STDIO_LOG_H
#define ND_DEVICES_STDIO_LOG_H

#include "common/Device.h"

namespace nd {

class StdioLog: public Device {
public:
StdioLog() = default;
    ~StdioLog() override = default;

    // -- Pipe Stuff
    Result send(Event event) override;
    Result rush(Event event) override;

    // -- Device Stuff
    //Result init(Wheel &wheel) override;
    //Result release() override;
    //Result task() override;
    //Result stop() override;
};

} // namespace nd

#endif // ND_DEVICES_STDIO_LOG_H