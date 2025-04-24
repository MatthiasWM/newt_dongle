
#include "CtrlBuffer.h"
#include "Pipe.h"

using namespace nd;

CtrlBuffer::CtrlBuffer() 
{
    type_ = 2;
}

CtrlBuffer::~CtrlBuffer() {
    // Destructor implementation
}



/**
 * Get a buffer from the pool.
 */
CtrlBuffer *CtrlBufferPool::get_buffer() {
    // Get a buffer implementation
    return new CtrlBuffer();
}
