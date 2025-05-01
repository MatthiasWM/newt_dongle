//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_CDC_ENDPOINT_H
#define ND_PICO_CDC_ENDPOINT_H  

#include "common/Endpoints/UARTEndpoint.h"

#include <cstdint>

namespace nd {

class PicoCDCEndpoint : public UARTEndpoint {
    static PicoCDCEndpoint *list_[4];
    uint32_t index_ = 0;
public:
    static PicoCDCEndpoint *instance(uint32_t index);
    PicoCDCEndpoint(Scheduler &scheduler, uint32_t index);
    ~PicoCDCEndpoint();
    Result init() override;
    Result task() override;
    Result send(Event event) override;
    Result rush(Event event) override;
    void set_bitrate(uint32_t new_bitrate) override;
};

} // namespace nd

#endif // ND_PICO_CDC_ENDPOINT_H