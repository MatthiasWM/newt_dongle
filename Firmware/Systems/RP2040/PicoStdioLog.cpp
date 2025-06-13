//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoStdioLog.h"

#include <pico/stdlib.h>

using namespace nd;

/**
 * \return true if the next call to write a byte would block.
 */
bool PicoStdioLog::would_block() {
    // TODO: check the FIFO in uart1
    return false;
}
