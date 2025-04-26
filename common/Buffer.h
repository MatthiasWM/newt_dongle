//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_BUFFER_H
#define ND_BUFFER_H

#include <stdint.h>

namespace nd {

/* ---- Buffer -------------------------------------------------------------- */

class Buffer {
protected:
    Buffer *next_ = nullptr;
    uint8_t type_ = 0;
public:
    Buffer();
    virtual ~Buffer();
    virtual void reset() = 0;
    void next(Buffer *aNext) { next_ = aNext; }
    Buffer *next() { return next_; }    
    uint8_t type();
};

/* ---- BufferBool ---------------------------------------------------------- */

class BufferPool {
    Buffer **buffer_ = nullptr;
    Buffer *free_list_ = nullptr;
    uint32_t num_buffers_ = 0;
    uint32_t free_list_size_ = 0;
    uint32_t buffer_size_ = 0;
protected:
    virtual Buffer *new_buffer(uint32_t buffer_size) = 0;
    Buffer *get_buffer_();
public:
    BufferPool(uint32_t num_buffers = 32, uint32_t buffer_size = 32);
    virtual ~BufferPool();
    void allocate_buffers();
    void release_buffer(Buffer *buffer);
    uint32_t available() const;
};
    
} // namespace nd

#endif // ND_BUFFER_H