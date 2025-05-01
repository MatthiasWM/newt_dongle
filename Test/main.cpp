/*
  MIT License

  Copyright (c) 2025 Matthias Melcher, robowerk.de

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "TestStdioLog.h"
#include "common/Scheduler.h"
#include "common/Endpoints/StdioLog.h"
#include "common/Pipes/BufferedPipe.h"
#include "common/Endpoints/TestEventGenerator.h"

#include <cstdio>

// -- The scheduler spins while the dongle is powered and deliver time slices to its spokes.
nd::Scheduler scheduler;

// -- Allocate all the endpoints we need.
nd::StdioLog log_device(scheduler);
nd::TestEventGenerator test_data_generator(scheduler);

// -- Allocate the pipes and filters that connect the endpoints.
nd::BufferedPipe gen_to_log(scheduler);
nd::Pipe log_to_gen;

// -- Everything is already allocated. Now link the endpoints and run the scheduler.
int main(int argc, char *argv[])
{
    // -- Connect the Endpoints inside the dongle with pipes.
    test_data_generator >> gen_to_log >> log_device;
    log_device >> log_to_gen >> test_data_generator;

    // -- The scheduler will call all instances of classes that are derived from Task.
    scheduler.init();

    // -- Now we can start the scheduler. It will call all spokes in a loop.
    scheduler.run(32);

    return 0;
} 

