//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_VERSION_H
#define ND_VERSION_H

// Single source for the firmware version. The CMake builds in PiPico/ and
// NewtCOM/ read this line for `pico_set_program_version()`, so keep the format.
#define ND_VERSION_STRING "0.7a"

#endif // ND_VERSION_H
