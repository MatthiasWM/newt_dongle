//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PIPES_TEE_H
#define ND_PIPES_TEE_H

#include "common/Pipe.h"

namespace nd {

class Tee: public Pipe {
public:
    Tee() = default;
    ~Tee() override = default;

    Pipe b;

    // -- Pipe Stuff
    Result send(Event event) override;
    Result rush(Event event) override;
};

} // namespace nd

#endif // ND_PIPES_TEE_H