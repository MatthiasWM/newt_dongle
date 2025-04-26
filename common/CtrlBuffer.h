//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_CTRL_BUFFER_H
#define ND_CTRL_BUFFER_H

#include "Buffer.h"
#include "CtrlBlock.h"

namespace nd {

/* ---- CtrlBuffer ---------------------------------------------------------- */

class CtrlBuffer : public Buffer{
    CtrlBlock ctrl_block_;
public:
    CtrlBuffer();
    ~CtrlBuffer();
    void reset() override;
    CtrlBlock *ctrl_block() { return &ctrl_block_; }
};

/* ---- CtrlBufferPool ------------------------------------------------------ */

class CtrlBufferPool : public BufferPool {
protected:
    Buffer *new_buffer(uint32_t buffer_size) override;
public:
    CtrlBufferPool(uint32_t num_buffers = 32);
    ~CtrlBufferPool() override = default;
    CtrlBuffer *get_buffer();
};

} // namespace nd    

#endif // ND_CTRL_BUFFER_H