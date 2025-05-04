//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_ENDPOINTS_UART_H
#define ND_ENDPOINTS_UART_H

#include "../Endpoint.h" // Adjusted the path to ensure the Endpoint header is correctly included

namespace nd {

class UARTEndpoint : public Endpoint {
    uint32_t bitrate_ = 38400; // Newton default
public:
    UARTEndpoint(Scheduler &scheduler);
    ~UARTEndpoint();

    Result send(Event) override;

    virtual void set_bitrate(uint32_t bitrate);
    uint32_t bitrate() const;
};

} // namespace nd

#endif // ND_ENDPOINTS_UART_H