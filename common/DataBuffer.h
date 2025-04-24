//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_DATA_BUFFER_H
#define ND_DATA_BUFFER_H

#include "Buffer.h"

namespace nd {

class DataBuffer : public Buffer {    
    static constexpr int max_size_ = 4;    // \todo Set thsi higher after debugging
    uint8_t data_[max_size_];
    int head_ = 0;
    int tail_ = 0;
public:
    static constexpr int max_size = 256;
    DataBuffer();
    ~DataBuffer();
    bool is_full() { return (head_ >= max_size_); }
    int append(uint8_t c);
    int available() { return (head_ - tail_); }
    int getc();
    bool done();
};

class DataBufferPool : public BufferPool {
public:
    DataBufferPool() = default;
    ~DataBufferPool() = default;

    DataBuffer *get_buffer();
};

} // namespace nd

#endif // ND_DATA_BUFFER_H