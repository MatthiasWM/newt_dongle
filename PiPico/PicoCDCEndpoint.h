//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_CDC_ENDPOINT_H
#define ND_PICO_CDC_ENDPOINT_H  

#include "common/Endpoints/CDCEndpoint.h"

namespace nd {

class PicoCDCEndpoint : public CDCEndpoint {
public:
    PicoCDCEndpoint();
    ~PicoCDCEndpoint();

    int init() override;
    int task() override;
};

} // namespace nd

#endif // ND_PICO_CDC_ENDPOINT_H