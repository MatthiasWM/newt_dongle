//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

// Newton Dongle configuration file

#include "common/Event.h"

#ifndef ND_CONFIG_H
#define ND_CONFIG_H

namespace nd {

// ---- User defined:
// Indicate use of UART hardware handshake 
constexpr bool debugShowUARTHandshake = true;

} // namespace nd

#endif // ND_CONFIG_H