//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_WHEEL_H
#define ND_WHEEL_H

#include "nd_config.h"

#include <forward_list>

namespace nd {

class Spoke;

/** \brief Call the tasks of all registered Endpoints in an endless loop. */
class Wheel {
    std::forward_list<Spoke*> spoke_list_;
    uint32_t ticks_ = 0;
public:
    Wheel() = default;
    ~Wheel() = default;
    Wheel(const Wheel&) = delete;
    Wheel& operator=(const Wheel&) = delete;
    Wheel(Wheel&&) = delete;
    Wheel& operator=(Wheel&&) = delete;

    // -- Add a Spoke to the wheel
    Wheel &add(Spoke &spoke);

    // -- Spin the wheel
    void init();
    void spin(int n=-1);

    // -- Additionl wheel features
    uint32_t ticks() const { return ticks_; }
};

} // namespace nd

#endif // ND_WHEEL_H