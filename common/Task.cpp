//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Task.h"

#include "Scheduler.h"

using namespace nd;

Task::Task(Scheduler &scheduler) 
:   scheduler_(scheduler) 
{
    scheduler.add(*this);
}

