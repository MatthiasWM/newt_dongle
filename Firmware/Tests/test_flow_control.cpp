//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "test.h"

#include "common/Endpoint.h"
#include "common/Filters/DTRSwitch.h"
#include "common/Pipes/BufferedPipe.h"
#include "common/Scheduler.h"

#include <vector>

using namespace nd;

namespace {

class TestScheduler : public Scheduler {
protected:
    void update_time() override { cycle_time_ = 100; }
};

// Stands in for the UART: remembers the state of its handshake line (HSKI).
class HandshakeEndpoint : public Endpoint {
public:
    bool high_water = false;
    HandshakeEndpoint(Scheduler &scheduler) : Endpoint(scheduler) { }
    void set_high_water(bool on) override { high_water = on; }
};

// Stands in for the USB CDC: accepts data only while `open`.
class Sink : public Pipe {
public:
    bool open = false;
    Result send(Event event) override {
        (void)event;
        return open ? Result::OK : Result::REJECTED;
    }
};

} // namespace

// A 512 event buffer raises HIGH_WATER when less than 32 events are free, and
// drops it when more than 48 are free again.
TEST(buffered_pipe_raises_high_water_upstream) {
    TestScheduler scheduler;
    HandshakeEndpoint uart { scheduler };
    BufferedPipe buffer { scheduler, 9 };
    Sink cdc;
    uart >> buffer >> cdc;
    scheduler.init();

    for (int i = 0; i < 470; i++) buffer.send(Event((uint8_t)i));
    CHECK(!uart.high_water);
    for (int i = 0; i < 20; i++) buffer.send(Event((uint8_t)i));
    CHECK(uart.high_water);

    cdc.open = true;
    scheduler.run(10);
    CHECK(uart.high_water);
    scheduler.run(500);
    CHECK(!uart.high_water);
}

// An OFF without an ON must not disable the next ON.
TEST(endpoint_ignores_extra_high_water_off) {
    TestScheduler scheduler;
    HandshakeEndpoint uart { scheduler };
    uart.rush_back(Event(Event::Type::HIGH_WATER, Event::Subtype::OFF));
    CHECK(!uart.high_water);
    uart.rush_back(Event(Event::Type::HIGH_WATER, Event::Subtype::ON));
    CHECK(uart.high_water);
    uart.rush_back(Event(Event::Type::HIGH_WATER, Event::Subtype::OFF));
    CHECK(!uart.high_water);
}

// After a mode change, a full buffer on the old side must not keep the Newton
// from sending.
TEST(dtr_switch_resets_high_water) {
    TestScheduler scheduler;
    HandshakeEndpoint uart { scheduler };
    DTRSwitch dtr_switch;
    uart >> dtr_switch;
    dtr_switch.cdc.rush(Event(Event::Type::UART, Event::Subtype::UART_DTR, 1));
    uart.rush_back(Event(Event::Type::HIGH_WATER, Event::Subtype::ON));
    CHECK(uart.high_water);
    dtr_switch.cdc.rush(Event(Event::Type::UART, Event::Subtype::UART_DTR, 0));
    CHECK(!uart.high_water);
}

// -- Host bitrate in direct connect mode

namespace {

// Stands in for the pipe towards the UART: records events while `open`.
class UARTSide : public Pipe {
public:
    bool open = true;
    std::vector<Event> events;
    Result send(Event event) override {
        if (!open) return Result::REJECTED;
        events.push_back(event);
        return Result::OK;
    }
    uint32_t last_bitrate() const {
        for (auto it = events.rbegin(); it != events.rend(); ++it)
            if (it->type() == Event::Type::SET_BITRATE) return it->bitrate();
        return 0;
    }
};

struct SwitchSession {
    DTRSwitch dtr_switch;
    UARTSide uart;
    SwitchSession() { dtr_switch >> uart; }
    void set_dtr(bool on) {
        dtr_switch.cdc.rush(Event(Event::Type::UART, Event::Subtype::UART_DTR, on ? 1 : 0));
    }
    Result host_sets_bitrate(uint32_t bitrate) {
        return dtr_switch.cdc.send(Event::make_bitrate_event(bitrate));
    }
};

} // namespace

// Most host apps set the bitrate before they raise DTR.
TEST(dtr_switch_applies_host_bitrate_when_dtr_rises) {
    SwitchSession s;
    CHECK(s.host_sets_bitrate(115200).ok());
    CHECK(s.uart.last_bitrate() == 0);
    s.set_dtr(true);
    CHECK(s.uart.last_bitrate() == 115200);
}

TEST(dtr_switch_forwards_host_bitrate_while_dtr_is_high) {
    SwitchSession s;
    s.set_dtr(true);
    s.host_sets_bitrate(57600);
    CHECK(s.uart.last_bitrate() == 57600);
}

// The Dock always talks to the Newton at 38400.
TEST(dtr_switch_restores_dock_bitrate_when_dtr_drops) {
    SwitchSession s;
    s.set_dtr(true);
    s.host_sets_bitrate(115200);
    s.set_dtr(false);
    CHECK(s.uart.last_bitrate() == 38400);
}

// A bitrate that the UART side can't take yet is sent before the next byte.
TEST(dtr_switch_keeps_rejected_bitrate) {
    SwitchSession s;
    s.set_dtr(true);
    s.uart.open = false;
    CHECK(s.host_sets_bitrate(115200).ok());
    s.uart.open = true;
    CHECK(s.dtr_switch.cdc.send(Event((uint8_t)'x')).ok());
    CHECK(s.uart.events.size() == 2 && s.uart.events[0].type() == Event::Type::SET_BITRATE);
    CHECK(s.uart.last_bitrate() == 115200);
}
