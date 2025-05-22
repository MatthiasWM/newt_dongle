//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_FILTERS_HAYES_FILTER_H
#define ND_FILTERS_HAYES_FILTER_H

#include "common/Task.h"

#include <string>

namespace nd {

class SDCardEndpoint;

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
    uint8_t index_ = 0;
    bool data_mode_ = true;
    uint8_t command_mode_progress_ = 0;
    uint32_t command_mode_timeout_ = 0;
    std::string cmd_, prev_cmd_;
    bool cmd_ready_ = false;
    bool cr_rcvd_ = false;
    uint32_t current_register_ = 0;
    uint32_t esc_code_guard_timeout_ = 1'000'000; // 1 second (see Register 12)

    SDCardEndpoint *sdcard_ = nullptr;

    void send_string(const char *str);

public:
    UpstreamPipe upstream { *this };
    DownstreamPipe downstream { *this };
    
    HayesFilter(Scheduler &scheduler, uint8_t ix);
    ~HayesFilter() override;

    void switch_to_command_mode();
    void switch_to_data_mode();

    Result signal(Event event) override;
    Result task() override;
    Result upstream_send(Event event);
    Result downstream_send(Event event);

    void run_cmd_line();
    const char *run_next_cmd(const char *cmd);
    const char *run_ampersand_cmd(const char *cmd);
    const char *run_sdcard_cmd(const char *cmd);
    uint32_t read_int(const char **cmd);

    void send_OK();
    void send_CONNECT();
    void send_ERROR();
    bool send_info(uint32_t ix);

    void link(SDCardEndpoint *sdcard);

    bool set_register(uint32_t reg, uint32_t value);
    uint32_t get_register(uint32_t reg) const;
};

} // namespace nd

#endif // ND_FILTERS_HAYES_FILTER_H