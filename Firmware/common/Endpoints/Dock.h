//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_ENDPOINTS_DOCK_H
#define ND_ENDPOINTS_DOCK_H

#include "common/Endpoint.h"

#include <queue>
#include <vector>

#ifndef ND_FOURCC
#  if __BYTE_ORDER == __LITTLE_ENDIAN 
#    define ND_FOURCC(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#  elif __BYTE_ORDER == __BIG_ENDIAN
#    define ND_FOURCC(a, b, c, d) ((uint32_t)(d) | ((uint32_t)(c) << 8) | ((uint32_t)(b) << 16) | ((uint32_t)(a) << 24))
#  else
#    error "Can't determine endianess"
#  endif
#endif

namespace nd {

class Dock : public Endpoint 
{
    typedef Endpoint super;

    struct Data {
        uint8_t *data_ = nullptr;
        uint16_t size_ = 0; // size of the data in bytes
        uint16_t pos_ = 0; // current position in the data
        bool start_frame_ = false; // if true, send a start frame marker
        bool end_frame_ = false; // if true, send an end frame marker
        bool free_after_send_ = false; // if true, the data will be freed after sending
    };
    std::queue<Data> data_queue_; // queue of data to be sent

    std::vector<uint8_t> in_data_;
    union {
        uint8_t cmd_[5] = {0}; // buffer for the command
        uint32_t cmd;
    };
    uint32_t size = 0;
    uint32_t aligned_size = 0;
    uint8_t state_ = 0;
    uint32_t in_index_ = 0;

    bool package_sent = false;

public:
    Dock(Scheduler &scheduler) : Endpoint(scheduler) { 
        in_data_.reserve(400); // reserve space for incoming data
    }
    ~Dock() override = default;
    Dock(const Dock&) = delete;
    Dock& operator=(const Dock&) = delete;
    Dock(Dock&&) = delete;
    Dock& operator=(Dock&&) = delete;

    // -- Pipe
    Result send(Event event) override;
    // Result rush(Event event) override;
    // Result rush_back(Event event) override;

    // -- Task
    // Result init() override;
    Result task() override;
    // Result signal(Event event) override;

    // -- Dock specific methods
    void process_command();
    void reply_dock();
    void reply_stim();
    void reply_lpkg();
    void reply_disc();
};

} // namespace nd

#endif // ND_ENDPOINTS_DOCK_H