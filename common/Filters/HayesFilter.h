//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_FILTERS_HAYES_FILTER_H
#define ND_FILTERS_HAYES_FILTER_H

#include "common/Task.h"

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
public:
    UpstreamPipe upstream { *this };
    DownstreamPipe downstream { *this };
    
    HayesFilter(Scheduler &scheduler);
    ~HayesFilter() override;

    Result task() override;
    Result upstream_send(Event event);
    Result downstream_send(Event event);
};

} // namespace nd

#endif // ND_FILTERS_HAYES_FILTER_H