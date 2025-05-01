//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_FILTERS_HAYES_FILTER_H
#define ND_FILTERS_HAYES_FILTER_H

#include "common/Task.h"

#include <string>

namespace nd {

class HayesFilter: public Task {
    class UpstreamPipe: public Pipe {
    public:
        HayesFilter &filter_;
        UpstreamPipe(HayesFilter &filter) : filter_(filter) { }
        Result send(Event event) override { return filter_.upstream_send(event); }
    };
    class DownstreamPipe: public Pipe {
    public:
        HayesFilter &filter_;
        DownstreamPipe(HayesFilter &filter) : filter_(filter) { }
        Result send(Event event) override { return filter_.downstream_send(event); }
    };
    bool data_mode_ = true;
    uint8_t command_mode_progress_ = 0;
    uint32_t command_mode_timeout_ = 0;
    std::string cmd_;
    bool cmd_ready_ = false;

    void send_string(const char *str);

public:
    UpstreamPipe upstream { *this };
    DownstreamPipe downstream { *this };
    
    HayesFilter(Scheduler &scheduler);
    ~HayesFilter() override;

    void switch_to_command_mode();
    void switch_to_data_mode();

    Result task() override;
    Result upstream_send(Event event);
    Result downstream_send(Event event);

    void run_cmd_line();
    const char *run_next_cmd(const char *cmd);

    void send_OK();
    void send_CONNECT();
    void send_ERROR();
};

} // namespace nd

#endif // ND_FILTERS_HAYES_FILTER_H