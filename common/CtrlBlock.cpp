
#include "CtrlBlock.h"

using namespace nd;

void CtrlBlock::cmd(uint8_t cmd) { 
    cmd_ = cmd; 
}

uint8_t CtrlBlock::cmd() { 
    return cmd_; 
}

void CtrlBlock::data(int32_t data1, int32_t data2, int32_t data3, int32_t data4) {
    data_[0] = data1;
    data_[1] = data2;
    data_[2] = data3;
    data_[3] = data4;
}

int32_t *CtrlBlock::data() { 
    return &data_[0]; 
}

int32_t CtrlBlock::d0() {
    return data_[0];
}

int32_t CtrlBlock::d1() {
    return data_[1];
}

int32_t CtrlBlock::d2() {
    return data_[2];
}

int32_t CtrlBlock::d3() {
    return data_[3];
}


