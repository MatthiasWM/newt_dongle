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
public:
    enum class Type: uint8_t {
        NONE = 0,
        DATA,
        CTRL,
    };
protected:
    Buffer *next_ { nullptr };
    Type type_ { Type::NONE };
public:
    Buffer(Buffer::Type the_type);
    virtual ~Buffer();
    virtual void reset() = 0;
    void next(Buffer *aNext);
    Buffer *next() const;
    Type type() const;
};

/* ---- BufferBool ---------------------------------------------------------- */

class BufferPool {
    Buffer **buffer_ = nullptr;
    Buffer *free_list_ = nullptr;
    uint32_t num_buffers_ = 0;
    uint32_t free_list_size_ = 0;
    uint32_t buffer_size_ = 0;
    Buffer::Type type_ = Buffer::Type::NONE;
protected:
    virtual Buffer *new_buffer(uint32_t buffer_size) = 0;
    Buffer *claim_buffer_();
public:
    BufferPool(Buffer::Type the_type, uint32_t num_buffers = 32, uint32_t buffer_size = 32);
    virtual ~BufferPool();
    void allocate_buffers();
    void release_buffer(Buffer *buffer);
    uint32_t available() const;
};
    
} // namespace nd

#endif // ND_BUFFER_H