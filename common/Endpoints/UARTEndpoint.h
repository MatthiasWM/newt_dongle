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

    Result init() override;
    Result task() override;

    // -- Pipe Stuff
    Result send(Event event) override;
    Result rush(Event event) override;

    // -- UART specific methods
    virtual void set_bitrate(uint32_t bitrate);
    uint32_t get_bitrate() const;
};

} // namespace nd

#endif // ND_ENDPOINTS_UART_H