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
    /// Receive errors since startup, see `ATI3`.
    struct RxErrors {
        uint32_t overrun = 0;   ///< the hardware RX FIFO was full
        uint32_t framing = 0;   ///< no stop bit: wrong bitrate or noise
        uint32_t brk = 0;       ///< break condition on the line
        uint32_t lost = 0;      ///< our receive buffer was full
    };

    UARTEndpoint(Scheduler &scheduler);
    ~UARTEndpoint();

    Result send(Event) override;

    virtual void set_bitrate(uint32_t bitrate);
    uint32_t bitrate() const;
    virtual RxErrors rx_errors() const { return RxErrors(); }
};

} // namespace nd

#endif // ND_ENDPOINTS_UART_H