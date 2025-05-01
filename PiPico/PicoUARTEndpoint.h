//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_UART_ENDPOINT_H
#define ND_PICO_UART_ENDPOINT_H  

#include "common/Endpoints/UARTEndpoint.h"

#include "pico/time.h"
#include <cstdint>

namespace nd {

class CtrlBlock;

class PicoUARTEndpoint : public UARTEndpoint {
    // uint32_t bitrate_ = 38400; // Default to 38400
    // absolute_time_t delay_until_ = 0;
    // uint8_t delay_state_ = 0; // See handle_delay() for state machine
    // uint8_t mnp_state_ = 0;   // MNP state machine
    bool event_pending_ = false;
    Event pending_event_ { Event::Type::NIL};
public:
    PicoUARTEndpoint(Scheduler &scheduler);
    ~PicoUARTEndpoint() override;
    Result init() override;
    Result task() override;
    Result send(Event event) override;
    Result rush(Event event) override;
    void set_bitrate(uint32_t new_bitrate) override;
};

} // namespace nd

#endif // ND_PICO_UART_ENDPOINT_H