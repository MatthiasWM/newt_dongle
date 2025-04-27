//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_CTRL_BLOCK_H
#define ND_CTRL_BLOCK_H

#include <stdint.h>

namespace nd {

enum class Cmd {
    NONE = 0,
    SET_BITRATE,
    AFTER_RESET,
};

class CtrlBlock {
    Cmd cmd_ = Cmd::NONE;
    int32_t data_[4] = {0, 0, 0, 0};
public:
    CtrlBlock() = default;
    ~CtrlBlock() = default;

    void cmd(Cmd cmd);
    Cmd cmd();

    void data(int32_t data1, int32_t data2=0, int32_t data3=0, int32_t data4=0);
    int32_t *data();
    int32_t d0();
    int32_t d1();
    int32_t d2();
    int32_t d3();
};

} // namespace nd    

#endif // ND_CTRL_BLOCK_H