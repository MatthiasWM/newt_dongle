//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "TestDataDevice.h"

using namespace nd;

Result TestDataDevice::init(Wheel &wheel) {
    Result r = Device::init(wheel);
    if (r.ok()) {
        // Initialize the device here
        // For example, set up GPIO pins, UART, etc.
        // uart_init(UART_ID, BAUD_RATE);
        // uart_set_hw_flow(UART_ID, false, false);
        // uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    }
    return r;
}

Result TestDataDevice::release() {
    return Device::release();
}

Result TestDataDevice::task() {
    send(Event('!'));
    return Device::task();
}

Result TestDataDevice::stop() {
    return Device::stop();
}


