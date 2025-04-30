//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Spoke.h"

#include "Wheel.h"

using namespace nd;

Spoke::Spoke(Wheel &wheel) 
:   wheel_(wheel) 
{
    wheel.add(*this);
}

