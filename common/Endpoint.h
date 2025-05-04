//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_DEVICE_H
#define ND_DEVICE_H

#include "nd_config.h"
#include "common/Pipe.h"
#include "common/Task.h"

namespace nd {

class Scheduler;

class Endpoint: public Task {
    int32_t high_water_count_ = 0;
public:
    Endpoint(Scheduler &scheduler) : Task(scheduler) { }
    virtual ~Endpoint() = default;
    Endpoint(const Endpoint&) = delete;
    Endpoint& operator=(const Endpoint&) = delete;
    Endpoint(Endpoint&&) = delete;
    Endpoint& operator=(Endpoint&&) = delete;

    Result send(Event event) override;
    Result rush(Event event) override;
    Result rush_back(Event event) override;

    virtual void set_high_water(bool on) { (void)on; }
    virtual void delay(uint32_t usec, uint32_t chars) { (void)usec; (void)chars; }
};

} // namespace nd

#endif // ND_DEVICE_H