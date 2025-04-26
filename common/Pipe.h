//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PIPE_H
#define ND_PIPE_H

#include <stddef.h>

#include "DataBuffer.h"
#include "CtrlBuffer.h"

namespace nd {

class Endpoint;
class Buffer;
class CtrlBlock;

class Pipe {
    Endpoint *source_ = nullptr;
    Endpoint *sink_ = nullptr;

    DataBufferPool data_buffer_pool_ { 32, 32 };
    CtrlBufferPool ctrl_buffer_pool_ { 32 };
    Buffer *first_buffer_ = nullptr;
    Buffer *last_buffer_ = nullptr;

    void unlink_first(Buffer *buffer);
    void enqueue_last(Buffer *buffer);

public:
    Pipe();
    ~Pipe();

    Pipe &connect_from(Endpoint &endpoint);
    Pipe &connect_to(Endpoint &endpoint);
    Pipe &disconnect();

    bool is_writable();
    int putc(uint8_t c);

    int data_available();
    int getc();


    int put_ctrl(Cmd cmd, int32_t data0=0, int32_t data1=0, int32_t data2=0, int32_t data3=0);

    int ctrl_available();
    CtrlBlock *peek_ctrl();
    int pop_ctrl();

};


} // namespace nd



#endif // ND_PIPE_H