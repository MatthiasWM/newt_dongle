//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_UART_ENDPOINT_H
#define ND_PICO_UART_ENDPOINT_H  

#include "common/Endpoints/UARTEndpoint.h"

#include <cstdint>

namespace nd {

class CtrlBlock;

class PicoUARTEndpoint : public UARTEndpoint {
    uint32_t bitrate_ = 38400; // Default to 38400
public:
    PicoUARTEndpoint();
    ~PicoUARTEndpoint();

    int init() override;
    int task() override;

    int handle(Event event);
    void set_bitrate(uint32_t new_bitrate);
};

} // namespace nd

#endif // ND_PICO_UART_ENDPOINT_H