//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_DATA_BUFFER_H
#define ND_DATA_BUFFER_H

#include "Buffer.h"

namespace nd {

/* ---- DataBuffer ---------------------------------------------------------- */

class DataBuffer : public Buffer {    
    uint8_t *data_ = nullptr;
    uint32_t size_ = 0;
    uint32_t head_ = 0;
    uint32_t tail_ = 0;
public:
    DataBuffer(uint32_t size);
    ~DataBuffer();
    void reset() override;
    bool is_full() { return (head_ >= size_); }
    int append(uint8_t c);
    int available() { return (head_ - tail_); }
    int getc();
    bool done();
};

/* ---- DataBufferPool ------------------------------------------------------ */

class DataBufferPool : public BufferPool {
protected:
    Buffer *new_buffer(uint32_t buffer_size) override;
public:
    DataBufferPool(uint32_t num_buffers = 32, uint32_t buffer_size = 32);
    ~DataBufferPool() override = default;
    DataBuffer *get_buffer();
};

} // namespace nd

#endif // ND_DATA_BUFFER_H