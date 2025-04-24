
#ifndef ND_CTRL_BUFFER_H
#define ND_CTRL_BUFFER_H

#include "Buffer.h"
#include "CtrlBlock.h"

namespace nd {

class CtrlBuffer : public Buffer{
    CtrlBlock ctrl_block_;
public:
    CtrlBuffer();
    ~CtrlBuffer();
    CtrlBlock *ctrl_block() { return &ctrl_block_; }
};

class CtrlBufferPool : public BufferPool {
public:
    CtrlBufferPool() = default;
    ~CtrlBufferPool() = default;

    CtrlBuffer *get_buffer();
};

} // namespace nd    

#endif // ND_CTRL_BUFFER_H