//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

// Newton Dongle configuration file

#include "common/Event.h"

namespace nd {

// ---- User defined:
// Maximum number of burst reads for all endpoints
constexpr uint32_t cfgMaxBurstRead = 32;

// ---- User defined:
// Maximum number of burst writes for all endpoints
constexpr uint32_t cfgMaxBurstWrite = cfgMaxBurstRead;

// ---- User defined:
// Size of ring buffer in power of two's (must be 5 or more, typically 8 to 10)
constexpr uint32_t cfgRingBufferSizePow2 = 9;

// Size of ring buffer in number of Event entries
constexpr uint32_t cfgRingBufferSize = 1 << cfgRingBufferSizePow2;

// Ring buffer index mask
constexpr uint32_t cfgRingBufferMask = cfgRingBufferSize - 1;

// Ring buffer high water mark (7/8th filled)
constexpr uint32_t cfgRingBufferHighWater = 1 << (cfgRingBufferSizePow2 - 3);


} // namespace nd