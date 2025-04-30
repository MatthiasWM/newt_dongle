//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Tee.h"

using namespace nd;

// TODO: this is a stub implementation. We need to handle rejection from either output.
Result Tee::send(Event event) {
    a.send(event);
    b.send(event);
    return Result::OK;
}

Result Tee::rush(Event event) {
    return send(event); // TODO: this is obviously wrong
}
