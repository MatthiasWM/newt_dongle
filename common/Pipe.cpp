//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "Pipe.h"
#include "Endpoint.h"
#include "DataBuffer.h"
#include "CtrlBuffer.h"

#include <stdio.h>

using namespace nd;

/**
 * \class Pipe Manage a stream of data and contol commands.
 * 
 * Pipes implement a FIFO buffer for data and commands. Data and commands
 * can be pushed into the pipe in any order. Pipe ensures that they must
 * be read in the same order.
 * 
 * \todo Pipe never allocates or deallocates memory, but uses fixed pools
 * of data and control buffers.
 * 
 * \todo The number of buffers will eventually be configurable
 */


/**
 * \brief Constructor
 * 
 * All the initialization is done in the header.
 */
Pipe::Pipe() {
    data_buffer_pool_.allocate_buffers();
    ctrl_buffer_pool_.allocate_buffers();
}

/** 
 * \brief Destructor
 * 
 * All dealocations are done inside the class.
 */
Pipe::~Pipe() {
}

/**
 * \brief Unlink the first buffer and return it to its pool.
 * 
 * The buffer is removed from the pipe. It will be returned to the
 * pool of buffers.
 */
void Pipe::unlink_first(Buffer *buffer) {
    if (!first_buffer_) {
        puts("**** ERROR **** in Pipe::unlink_first() - no buffer to unlink");
        return;
    }
    if (first_buffer_ != buffer) {
        puts("**** ERROR **** in Pipe::unlink_first() - buffer is not the first one");
        return;
    }
    first_buffer_ = buffer->next();
    if (first_buffer_ == nullptr) last_buffer_ = nullptr;
    if (buffer->type() == Buffer::Type::DATA) {
        data_buffer_pool_.release_buffer(buffer);
    } else if (buffer->type() == Buffer::Type::CTRL) {
        ctrl_buffer_pool_.release_buffer(buffer);
    } else {
        puts("**** ERROR **** in Pipe::unlink_first() - buffer is not a data or control buffer");
    }
}

/**
 * Add this buffer to the end of the pipe.
 */
void Pipe::enqueue_last(Buffer *buffer) {
    buffer->next(nullptr);
    // Make sure that partially used data buffers are purged before we add a new buffer
    if (last_buffer_ && (last_buffer_ == first_buffer_) && (last_buffer_->type() == Buffer::Type::DATA)) {
        DataBuffer *data_buffer = static_cast<DataBuffer*>(last_buffer_);
        data_buffer->next(buffer);
        if (data_buffer->done())
            unlink_first(data_buffer);
    }
    // Append the new buffer as a last buffer
    if (last_buffer_)
        last_buffer_->next(buffer);
    last_buffer_ = buffer;
    if (first_buffer_ == nullptr)
        first_buffer_ = last_buffer_;
}

/**
 * \brief Connect the pipe to a source endpoint.
 * 
 * The pipe can be connected to a source endpoint. The source endpoint
 * can stuff data and control commands into the pipe.
 */
Pipe &Pipe::connect_from(Endpoint &endpoint) {
    endpoint.set_out(this);
    source_ = &endpoint;
    return *this;
}

/**
 * \brief Connect the pipe to a sink endpoint.
 * 
 * The pipe can be connected to a sink endpoint. The sink endpoint
 * can read data and control commands from the pipe.
 */
Pipe &Pipe::connect_to(Endpoint &endpoint) {
    endpoint.set_in(this);
    sink_ = &endpoint;
    return *this;
}

/**
 * \brief Disconnect the pipe from the source and sink endpoints.
 * 
 * The pipe can be disconnected from the source and sink endpoints.
 * The endpoints will no longer be able to stuff data or read data
 * from the pipe.
 * 
 * \todo We must flush the pipe after disconnecting it.
 */
Pipe &Pipe::disconnect() {
    if (source_) {
        source_->set_out(nullptr);
        source_ = nullptr;
    }
    if (sink_) {
        sink_->set_in(nullptr);
        sink_ = nullptr;
    }
    return *this;
}

/**
 * Check if there is a space to send at least on byte.
 * 
 * \todo At this point, we assume that we always can have a buffer.
 */
bool Pipe::is_writable() {
    return true;
}

/**
 * Send one byte into the pipe.
 * 
 * \return the number of bytes sent, or a negative integer if an rerror occured
 */
int Pipe::putc(uint8_t c) {
    // Check if we must get a fresh buffer from the pool
    bool add_buffer = false;
    if (last_buffer_ == nullptr) {
        // No buffer yet, we must add one
        add_buffer = true;
    } else if (last_buffer_->type() != Buffer::Type::DATA) {
        // The last buffer is not a data buffer, we must add one
        add_buffer = true;
    } else if ((last_buffer_->type()==Buffer::Type::DATA) && static_cast<DataBuffer*>(last_buffer_)->is_full()) {
        // The last buffer is full, we must add one
        add_buffer = true;
    }
    // Append a bufffer to the linked list of buffers
    if (add_buffer) {
        Buffer *new_buffer = data_buffer_pool_.claim_buffer();
        if (new_buffer == nullptr) {
            puts("**** ERROR **** in Pipe::putc() - no data buffer available");
            return -1;
        }
        enqueue_last(new_buffer);
    }
    // At this point, the last buffer must exist, it must be a data buffer, and 
    // it must have room for at least one byte. So write that byte.
    static_cast<DataBuffer*>(last_buffer_)->append(c);
    return 1;
}

/**
 * \brief Return the number of bytes available in the pipe.
 * More bytes may be available as the internal buffers shift.
 * 
 * \return the number of bytes available in the pipe
 */
int Pipe::data_available() {
    if (first_buffer_ && first_buffer_->type() == Buffer::Type::DATA) {
        DataBuffer *data_buffer = static_cast<DataBuffer*>(first_buffer_);
        return data_buffer->available();
    }
    return 0;
}

/**
 * \brief Get one byte from the pipe.
 * 
 * The byte is removed from the pipe. If the buffer is empty, it will
 * be removed from the pipe.
 * 
 * \return the byte received, or a negative integer if an rerror occured
 */
int Pipe::getc() {
    if (first_buffer_ && first_buffer_->type() == Buffer::Type::DATA) {
        DataBuffer *data_buffer = static_cast<DataBuffer*>(first_buffer_);
        int c = data_buffer->getc();
        if (data_buffer->done())
            unlink_first(data_buffer);
        return c;
    }
    return -1;
}

/**
 * \brief Put a control command into the pipe.
 * 
 * The command is added to the end of the pipe. If the buffer is full,
 * an error code is returned.
 * 
 * \return the number of control blocks sent, or a negative integer if an rerror occured
 */
int Pipe::put_ctrl(Cmd cmd, int32_t data0, int32_t data1, int32_t data2, int32_t data3) {
    CtrlBuffer *new_buffer = ctrl_buffer_pool_.claim_buffer();
    if (new_buffer == nullptr) {
        puts("**** ERROR **** in Pipe::put_ctrl() - no ctrl buffer available");
        return -1;
    }
    new_buffer->ctrl_block()->cmd(cmd);
    new_buffer->ctrl_block()->data(data0, data1, data2, data3);
    enqueue_last(new_buffer);
    return 1;
}
/**
 * \brief Check if there is a control command available in the pipe.
 * 
 * \return 1 if a control block is available next
 */
int Pipe::ctrl_available() {
    if (first_buffer_ && first_buffer_->type() == Buffer::Type::CTRL) {
        return 1;
    } else {
        return 0;
    }
}
/**
 * \brief Peek at the first control command in the pipe.
 * 
 * The command is not removed from the pipe. If the buffer is empty, it will
 * be removed from the pipe.
 * 
 * \return a pointer to the control block, or nullptr if an error occured
 */
CtrlBlock *Pipe::peek_ctrl() {
    if (first_buffer_ && first_buffer_->type() == Buffer::Type::CTRL) {
        CtrlBuffer *ctrl_buffer = static_cast<CtrlBuffer*>(first_buffer_);
        return ctrl_buffer->ctrl_block();
    }
    return nullptr;
}
/**
 * \brief Pop the first control command from the pipe.
 * 
 * The command is removed from the pipe. If the buffer is empty, it will
 * be removed from the pipe.
 */
int Pipe::pop_ctrl() {
    if (first_buffer_ && first_buffer_->type() == Buffer::Type::CTRL) {
        unlink_first(first_buffer_);
        return 0;
    } else {
        puts("**** ERROR **** in Pipe::pop_ctrl() - first buffer is not a control buffer");
        return -1;
    }
}


#if OLD_PIPE_TEST

    // uart_test_pipe.putc('a');
    // uart_test_pipe.putc('b');
    // uart_test_pipe.putc('c');
    // uart_test_pipe.putc('d');
    // uart_test_pipe.putc('e');
    // uart_test_pipe.put_ctrl(1, 4711, 1501);
    // uart_test_pipe.putc('f');
    // uart_test_pipe.putc('g');
    // uart_test_pipe.putc('h');
    // uart_test_pipe.putc('i');
    // uart_test_pipe.put_ctrl(2, 4711, 1501);
    // uart_test_pipe.put_ctrl(3, 4711, 1501);
    // uart_test_pipe.putc('j');
    // uart_test_pipe.putc('k');
    // uart_test_pipe.putc('l');
    // uart_test_pipe.putc('m');
    // uart_test_pipe.putc('n');
    // uart_test_pipe.putc('o');
    // uart_test_pipe.putc('p');
    // uart_test_pipe.putc('q');
    // uart_test_pipe.putc('r');

    // for (;;) {
    //     if (uart_test_pipe.data_available() > 0) {
    //         char c = uart_test_pipe.getc();
    //         printf("Rcvd: %c\n", c);
    //     }
    //     if (uart_test_pipe.ctrl_available() > 0) {
    //         nd::CtrlBlock *ctrl_block = uart_test_pipe.peek_ctrl();
    //         printf("Ctrl: %d %d %d %d %d\n", ctrl_block->cmd(), ctrl_block->d0(), ctrl_block->d1(), ctrl_block->d2(), ctrl_block->d3());
    //         uart_test_pipe.pop_ctrl();
    //     }
    // }

#endif // OLD_PIPE_TEST
