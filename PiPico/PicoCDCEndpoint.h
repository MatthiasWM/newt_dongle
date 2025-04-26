//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_CDC_ENDPOINT_H
#define ND_PICO_CDC_ENDPOINT_H  

#include "common/Endpoints/CDCEndpoint.h"

#include <cstdint>

namespace nd {

class PicoCDCEndpoint : public CDCEndpoint {
    static PicoCDCEndpoint *list_[4];
    uint32_t index_ = 0;
public:
    static PicoCDCEndpoint *instance(uint32_t index);
    PicoCDCEndpoint(uint32_t index);
    ~PicoCDCEndpoint();
    int init() override;
    int task() override;
    void host_set_bitrate(uint32_t new_bitrate);
};

} // namespace nd

#endif // ND_PICO_CDC_ENDPOINT_H