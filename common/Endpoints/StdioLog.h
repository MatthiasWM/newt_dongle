//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_ENDPOINTS_STDIO_LOG_H
#define ND_ENDPOINTS_STDIO_LOG_H

#include "common/Endpoint.h"

namespace nd {

class StdioLog: public Endpoint {
protected:
    virtual bool would_block();
public:
    StdioLog(Wheel &wheel) : Endpoint(wheel) { }
    ~StdioLog() override = default;

    // -- Pipe Stuff
    Result send(Event event) override;
    Result rush(Event event) override;
};

} // namespace nd

#endif // ND_ENDPOINTS_STDIO_LOG_H