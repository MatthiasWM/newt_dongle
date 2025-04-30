//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PIPES_BUFFERED_PIPE_H
#define ND_PIPES_BUFFERED_PIPE_H

#include "../Pipe.h"

#include <vector>

namespace nd {

class BufferedPipe: public Pipe {
    std::vector<Event> buffer_;
    uint32_t ring_size_ = 0;
    uint32_t ring_mask_ = 0;
    uint32_t head_ = 0;
    uint32_t tail_ = 0;
public:
    BufferedPipe(uint8_t buffer_size_pow2 = 9);
    ~BufferedPipe() override = default;

    // -- Writing to the next pipe
    Result send(Event event) override;
    Result rush(Event event) override;
}; 


} // namespace nd

#endif // ND_PIPES_BUFFERED_PIPE_H