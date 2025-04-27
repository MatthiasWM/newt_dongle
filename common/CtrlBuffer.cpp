//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "CtrlBuffer.h"
#include "Pipe.h"

using namespace nd;

/* ---- CtrlBuffer ---------------------------------------------------------- */

CtrlBuffer::CtrlBuffer() 
:   Buffer(Type::CTRL)
{
}

CtrlBuffer::~CtrlBuffer() {
}

/**
 * Prepare for reuse.
 */
void CtrlBuffer::reset() {
    ctrl_block_.cmd(Cmd::AFTER_RESET);
}

/* ---- CtrlBufferPool ------------------------------------------------------ */

/**
 * Allocate a pool of control buffers.
 * 
 * \param num_buffers Number of buffers to pre-allocate
 * \param buffer_size Size of each buffer in bytes
 */
CtrlBufferPool::CtrlBufferPool(uint32_t num_buffers)
:   BufferPool(Buffer::Type::CTRL, num_buffers, 0) 
{
}

/**
 * Allocate a new buffer of the required type
 */
Buffer *CtrlBufferPool::new_buffer(uint32_t buffer_size)
{
    (void)buffer_size; // Unused
    return new CtrlBuffer();
}

/**
 * Get a buffer from the pool.
 */
CtrlBuffer *CtrlBufferPool::claim_buffer() {
    return static_cast<CtrlBuffer*>(claim_buffer_());
}
