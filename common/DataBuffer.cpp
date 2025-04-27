//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "DataBuffer.h"
#include "Pipe.h"

using namespace nd;

/* ---- DataBuffer ---------------------------------------------------------- */

/**
 * \class DataBuffer
 * 
 * \brief A data buffer for the pipe.
 * 
 * The data buffer is a fixed size buffer that can be used to store
 * data. It is used by the pipe to store data that is being sent
 * or received.
 */

/**
 * \brief Constructor
 */
DataBuffer::DataBuffer(uint32_t size) 
:   Buffer(Type::DATA),
    data_(new uint8_t[size]),
    size_(size)
{
}

/**
 * \brief Destructor
 */
DataBuffer::~DataBuffer() {
    delete[] data_;
}

/**
 * Prepare for reuse.
 */
void DataBuffer::reset() { 
    head_ = 0; 
    tail_ = 0; 
}

/**
 * \brief Append a character to the buffer.
 * 
 * The character is added to the end of the buffer. If the buffer
 * is full, the character is not added and an error code is returned.
 */
int DataBuffer::append(uint8_t c) {
    // Append character to the buffer
    if (is_full()) {
        return -1; // Buffer is full
    }
    // Add character to the buffer (implementation depends on the buffer structure)
    data_[head_] = c;
    head_++;
    return 1; // Success
}

/**
 * \brief Get a single character from the buffer.
 * 
 * The character is removed from the buffer and returned. If the
 * buffer is empty, an error code is returned.
 */
int DataBuffer::getc() {
    // Get a character from the buffer
    if (tail_ >= head_) {
        return -1; // No data available
    }
    char c = data_[tail_];
    tail_++;
    return c; // Return the character
}

/**
 * \brief Check if the buffer can still be used.
 * 
 * \return true if the buffer is done and can be returned to the pool.
 */
bool DataBuffer::done() {
    if (next()) {
        return (head_ == tail_);
    } else {
        return (tail_ == size_);
    }
}

/* ---- DataBufferPool ------------------------------------------------------ */

/**
 * Allocate a pool of data buffers.
 * 
 * \param num_buffers Number of buffers to pre-allocate
 * \param buffer_size Size of each buffer in bytes
 */
DataBufferPool::DataBufferPool(uint32_t num_buffers, uint32_t buffer_size)
:   BufferPool(Buffer::Type::DATA, num_buffers, buffer_size) 
{
}

/**
 * Allocate a new buffer of the required type
 */
Buffer *DataBufferPool::new_buffer(uint32_t buffer_size)
{
    return new DataBuffer(buffer_size);
}

/**
 * Get a buffer from the pool.
 */
DataBuffer *DataBufferPool::claim_buffer() {
    return static_cast<DataBuffer*>(claim_buffer_());
}
