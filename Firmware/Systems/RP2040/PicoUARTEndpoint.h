//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_UART_ENDPOINT_H
#define ND_PICO_UART_ENDPOINT_H  

#include "common/Endpoints/UARTEndpoint.h"

#include <pico/time.h>
#include <cstdint>

namespace nd {

class CtrlBlock;

class PicoUARTEndpoint : public UARTEndpoint {
    bool event_pending_ = false;
    Event pending_event_ { Event::Type::NIL};
    uint32_t tx_delay_ = 0;
    bool tx_wait_for_fifo_empty_ = false;
public:
    PicoUARTEndpoint(Scheduler &scheduler);
    ~PicoUARTEndpoint() override;
    Result init() override;
    Result task() override;
    Result send(Event event) override;

    void delay(uint32_t usec, uint32_t chars) override;
    void set_high_water(bool) override;
    void set_bitrate(uint32_t new_bitrate) override;
};

} // namespace nd

#endif // ND_PICO_UART_ENDPOINT_H