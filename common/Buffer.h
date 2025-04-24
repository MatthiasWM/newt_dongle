
#ifndef ND_BUFFER_H
#define ND_BUFFER_H

#include <stdint.h>

namespace nd {

class Buffer {
protected:
    Buffer *next_ = nullptr;
    uint8_t type_ = 0;
public:
    Buffer();
    virtual ~Buffer();
    void next(Buffer *aNext) { next_ = aNext; }
    Buffer *next() { return next_; }    
    uint8_t type();
};

class BufferPool {
public:
    BufferPool();
    ~BufferPool();

    void release_buffer(Buffer *buffer);
    bool available() const;
};
    
} // namespace nd

#endif // ND_BUFFER_H