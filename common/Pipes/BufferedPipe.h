//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PIPES_BUFFERED_PIPE_H
#define ND_PIPES_BUFFERED_PIPE_H

#include "../Pipe.h"
#include "../Task.h"

#include <vector>

namespace nd {

class BufferedPipe: public Task {
    std::vector<Event> buffer_;
    uint32_t ring_size_ = 0;
    uint32_t ring_mask_ = 0;
    uint32_t head_ = 0;
    uint32_t tail_ = 0;

protected:
    // -- Pipe Stuff
    void push_back(Event event);
    Event pop_front();
    Event peek_front();
    bool is_full() const;
    bool is_empty() const;
    bool is_high_water_mark() const;

public:
    BufferedPipe(Scheduler &scheduler, uint8_t buffer_size_pow2 = 9);
    ~BufferedPipe() override = default;

    // -- Task stuff
    Result task() override;

    // -- Writing to the next pipe
    Result send(Event event) override;
    Result rush(Event event) override;
}; 


} // namespace nd

#endif // ND_PIPES_BUFFERED_PIPE_H