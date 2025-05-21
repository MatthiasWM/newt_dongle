//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_ENDPOINTS_SDCARD_H
#define ND_ENDPOINTS_SDCARD_H

#include "../Endpoint.h" // Adjusted the path to ensure the Endpoint header is correctly included

namespace nd {

class SDCardEndpoint : public Endpoint {
public:
    SDCardEndpoint(Scheduler &scheduler);
    ~SDCardEndpoint();

    Result send(Event) override;

    virtual const char *strerr(uint32_t err) = 0;
    virtual uint32_t disk_status() = 0;
    virtual uint32_t disk_initialize() = 0;
    virtual uint32_t get_label(char *label_buffer) = 0;
};

} // namespace nd

#endif // ND_ENDPOINTS_SDCARD_H