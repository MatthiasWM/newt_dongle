//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Buffer.h"

#include <cstdio>
#include <cassert>

using namespace nd;

/* ---- Buffer -------------------------------------------------------------- */

Buffer::Buffer() {
    // Constructor implementation
}

Buffer::~Buffer() {
    // Destructor implementation
}

uint8_t Buffer::type() { 
    return type_; 
}

/* ---- BufferPool ---------------------------------------------------------- */

/**
 * Allocate a pool of buffers of the same size.
 * 
 * \param num_buffers Number of buffers to pre-allocate
 * \param buffer_size Size of each buffer in bytes
 */
BufferPool::BufferPool(uint32_t num_buffers, uint32_t buffer_size) 
:   num_buffers_ { num_buffers },
    buffer_size_ { buffer_size }
{
}

/**
 * Deallocate the buffer pool.
 */
BufferPool::~BufferPool() {
    if (buffer_) {
        for (uint32_t i = 0; i < free_list_size_; ++i) {
            delete buffer_[i];
        }
        delete[] buffer_;
    }
}

/**
 * Allocate buffers for the pool.
 */
void BufferPool::allocate_buffers() {
    buffer_ = new Buffer*[num_buffers_];
    buffer_[0] = new_buffer(buffer_size_);
    for (uint32_t i = 1; i < num_buffers_; ++i) {
        buffer_[i] = new_buffer(buffer_size_);
        buffer_[i - 1]->next(buffer_[i]);
    }
    buffer_[num_buffers_-1]->next(nullptr);
    free_list_ = buffer_[0];
    free_list_size_ = num_buffers_;
}

/**
 * Get a buffer from the pool.
 * 
 * \return Pointer to the buffer, or nullptr if no buffers are available
 */
Buffer *BufferPool::get_buffer_() {
    assert(buffer_. "Call allocate_buffers() before using the buffer pool");
    if (free_list_ == nullptr) {
        puts("**** ERROR **** in BufferPool::get_buffer_() - no buffer available");
        return nullptr;
    }
    Buffer *buffer = free_list_;
    free_list_ = buffer->next();
    buffer->next(nullptr);
    free_list_size_--;
    // printf("BufferPool::get_buffer_() %d buffers still available\n", free_list_size_);
    return buffer;
}

/**
 * Return a buffer back to the pool.
 * 
 * The caller is responsible that this buffer does come from this pool.
 * 
 * \param buffer Pointer to the buffer to be released
 */
void BufferPool::release_buffer(Buffer *buffer) {
    if (buffer == nullptr) return;
    buffer->reset();
    buffer->next(free_list_);
    free_list_ = buffer;
    free_list_size_++;
    // printf("BufferPool::release_buffer() now %d buffers available\n", free_list_size_);
}

/**
 * Return the number of free buffers available in the pool.
 * 
 * \return True if there are free buffers available, false otherwise
 */
uint32_t BufferPool::available() const {
    return free_list_size_;
}

