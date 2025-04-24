//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_ENDPOINTS_CDC_H
#define ND_ENDPOINTS_CDC_H

#include "../Endpoint.h" // Adjusted the path to ensure the Endpoint header is correctly included

namespace nd {

class CDCEndpoint : public Endpoint {
public:
    CDCEndpoint();
    ~CDCEndpoint();

    int init() override;
    int task() override;
};

} // namespace nd

#endif // ND_ENDPOINTS_CDC_H