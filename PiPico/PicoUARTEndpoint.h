//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_UART_ENDPOINT_H
#define ND_PICO_UART_ENDPOINT_H  

#include "common/Endpoints/UARTEndpoint.h"

namespace nd {

class PicoUARTEndpoint : public UARTEndpoint {
public:
    PicoUARTEndpoint();
    ~PicoUARTEndpoint();

    int init() override;
    int task() override;
};

} // namespace nd

#endif // ND_PICO_UART_ENDPOINT_H