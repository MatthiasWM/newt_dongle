//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_PICO_ASYNC_LOG_H
#define ND_PICO_ASYNC_LOG_H

#include "common/Event.h"
#include "pico/util/queue.h"
#include "pico/time.h"

namespace nd {

class PicoAsyncLog {
    static PicoAsyncLog *the_instance_;
    static void run_() { the_instance_->run(); }
    static queue_t queue_;
    absolute_time_t prev_event_time_ = nil_time;
    int dest_ = 0; // 0 = stdio, 1 = sd card
    void run();
public:
    PicoAsyncLog(int dest);
    ~PicoAsyncLog() = default;
    void log(Event event, int pipe=0);
    void log(Result result, int pipe=0);
    void log(const char *message, int pipe=0);
};

} // namespace nd

#endif // ND_PICO_ASYNC_LOG_H