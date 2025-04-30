//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "BufferedPipe.h"

using namespace nd;

BufferedPipe::BufferedPipe(uint8_t buffer_size_pow2)
:   ring_size_ { static_cast<uint32_t>(1 << buffer_size_pow2) },
    ring_mask_ { ring_size_ - 1 },
    head_ { 0 },
    tail_ { 0 }
{
    buffer_.resize(ring_size_);
}

Result BufferedPipe::send(Event event) {   
    // TODO: continue here
    // Implement the ring buffer
    // Return in subytype if the high water mark is reached
    // Implement the delay functionality
    // Rename the new Device class back to Endpoint
    // Serialize the Log Endpoint to run better on Pico (just buffer it?)
    // Better Log output
    // Implement the UART endpoint API
    // (Re)Implement the Pico UART and Pico CDC classes
    return Pipe::send(event);
}

Result BufferedPipe::rush(Event event) {
    // TODO: continue here
    return Pipe::rush(event);
}
