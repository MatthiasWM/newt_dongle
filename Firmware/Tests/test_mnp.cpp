//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "test.h"

#include "common/Filters/MNPFilter.h"
#include "common/Scheduler.h"

#include <vector>

using namespace nd;

typedef std::vector<uint8_t> Bytes;

// -- Frames captured from a docking session between a Newton and NCU
//    (see PiPico/unused.cpp). All CRCs were verified independently.

// Link Request as sent by NCU. MNPFilter sends exactly this frame. The filter
// does not check LR parameters, so it is also used as the Newton's LR here.
static const Bytes kLR = {
    0x16, 0x10, 0x02, 0x17, 0x01, 0x02, 0x01, 0x06, 0x01, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0x02, 0x01, 0x02, 0x03, 0x01, 0x01, 0x04, 0x02, 0x40, 0x00,
    0x08, 0x01, 0x03, 0x10, 0x03, 0x9A, 0x1B,
};

// Link Acknowledge for the LR, seq 0, sent by the Newton.
static const Bytes kNewtonLA0 = {
    0x16, 0x10, 0x02, 0x03, 0x05, 0x00, 0x01, 0x10, 0x03, 0x05, 0x5D,
};

// Link Transfer frames sent by the Newton, seq 1 to 3.
static const Bytes kNewtonRtdk = {
    0x16, 0x10, 0x02, 0x02, 0x04, 0x01, 'n', 'e', 'w', 't', 'd', 'o', 'c', 'k',
    'r', 't', 'd', 'k', 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x09,
    0x10, 0x03, 0xB4, 0x89,
};
static const Bytes kNewtonName = { // contains an escaped DLE (0x10 0x10)
    0x16, 0x10, 0x02, 0x02, 0x04, 0x02, 0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F,
    0x63, 0x6B, 0x6E, 0x61, 0x6D, 0x65, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00,
    0x00, 0x48, 0xED, 0x7C, 0x17, 0xE0, 0x01, 0x00, 0x00, 0x00, 0x10, 0x10,
    0x00, 0x30, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x80, 0x00, 0x00,
    0x3E, 0xD0, 0x00, 0x00, 0x00, 0x01, 0xE0, 0x00, 0x00, 0x01, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xE6, 0x51, 0x05, 0x00,
    0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE6, 0x51, 0x05, 0x00,
    0x00, 0x00, 0x0B, 0x00, 0x42, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x20, 0x00,
    0x54, 0x00, 0x65, 0x00, 0x6D, 0x00, 0x00, 0x10, 0x03, 0xF3, 0x6B,
};
static const Bytes kNewtonNinf = {
    0x16, 0x10, 0x02, 0x02, 0x04, 0x03, 'n', 'e', 'w', 't', 'd', 'o', 'c', 'k',
    'n', 'i', 'n', 'f', 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x0A,
    0x00, 0x65, 0xE0, 0xBC, 0xFF, 0xA7, 0x74, 0x1B, 0x10, 0x03, 0xC5, 0xD2,
};

// Link Acknowledge frames sent by NCU for the LT frames above.
static const Bytes kDockLA1 = { 0x16, 0x10, 0x02, 0x03, 0x05, 0x01, 0x01, 0x10, 0x03, 0x54, 0x9D };
static const Bytes kDockLA2 = { 0x16, 0x10, 0x02, 0x03, 0x05, 0x02, 0x01, 0x10, 0x03, 0xA4, 0x9D };
static const Bytes kDockLA3 = { 0x16, 0x10, 0x02, 0x03, 0x05, 0x03, 0x01, 0x10, 0x03, 0xF5, 0x5D };

// Link Transfer sent by NCU, seq 1, carrying `dock` (session type 1).
static const Bytes kDockDock = {
    0x16, 0x10, 0x02, 0x02, 0x04, 0x01, 'n', 'e', 'w', 't', 'd', 'o', 'c', 'k',
    'd', 'o', 'c', 'k', 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01,
    0x10, 0x03, 0xBA, 0x4F,
};

// -- Test fixture

namespace {

class TestScheduler : public Scheduler {
protected:
    void update_time() override { cycle_time_ = 100; }
};

// Accepts everything and records data bytes and other events separately.
class Recorder : public Pipe {
public:
    Bytes bytes;
    std::vector<Event> events;
    Result send(Event event) override {
        if (event.is_data()) bytes.push_back(event.data());
        else events.push_back(event);
        return Result::OK;
    }
    bool received(Event::Type type, Event::Subtype subtype) const {
        for (auto &e : events)
            if (e.type() == type && e.subtype() == subtype) return true;
        return false;
    }
    void clear() { bytes.clear(); events.clear(); }
};

// The payload of an LT frame as the Dock should see it: no header, no
// trailer, and escaped DLEs reduced to a single DLE.
Bytes lt_payload(const Bytes &frame) {
    Bytes payload;
    for (size_t i = 6; i < frame.size() - 4; i++) {
        payload.push_back(frame[i]);
        if (frame[i] == 0x10) i++;
    }
    return payload;
}

struct Session {
    TestScheduler scheduler;
    MNPFilter mnp { scheduler };
    Recorder to_dock;   // what the filter sends to the Dock
    Recorder to_newt;   // what the filter sends to the Newton

    Session() {
        mnp.newt >> to_dock;
        mnp.dock >> to_newt;
        scheduler.init();
    }
    void newton_sends(const Bytes &frame) {
        for (auto b : frame) mnp.newt.send(Event(b));
        scheduler.run(500);
    }
    void dock_sends(const Bytes &payload) {
        mnp.dock.send(Event(Event::Type::MNP, Event::Subtype::MNP_FRAME_START));
        for (auto b : payload) mnp.dock.send(Event(b));
        mnp.dock.send(Event(Event::Type::MNP, Event::Subtype::MNP_FRAME_END));
        scheduler.run(500);
    }
    void connect() {
        newton_sends(kLR);
        newton_sends(kNewtonLA0);
        to_dock.clear();
        to_newt.clear();
    }
};

} // namespace

// -- Tests

TEST(mnp_answers_lr_like_ncu) {
    Session s;
    s.newton_sends(kLR);
    CHECK(s.to_dock.received(Event::Type::MNP, Event::Subtype::MNP_NEGOTIATING));
    CHECK_BYTES(s.to_newt.bytes, kLR);
}

TEST(mnp_connects_after_la) {
    Session s;
    s.newton_sends(kLR);
    s.newton_sends(kNewtonLA0);
    CHECK(s.to_dock.received(Event::Type::MNP, Event::Subtype::MNP_CONNECTED));
}

TEST(mnp_forwards_lt_to_dock_and_acknowledges) {
    Session s;
    s.connect();

    s.newton_sends(kNewtonRtdk);
    CHECK_BYTES(s.to_dock.bytes, lt_payload(kNewtonRtdk));
    CHECK(s.to_dock.received(Event::Type::MNP, Event::Subtype::MNP_FRAME_START));
    CHECK(s.to_dock.received(Event::Type::MNP, Event::Subtype::MNP_FRAME_END));
    CHECK_BYTES(s.to_newt.bytes, kDockLA1);

    s.to_dock.clear(); s.to_newt.clear();
    s.newton_sends(kNewtonName);
    CHECK_BYTES(s.to_dock.bytes, lt_payload(kNewtonName));
    CHECK_BYTES(s.to_newt.bytes, kDockLA2);

    s.to_dock.clear(); s.to_newt.clear();
    s.newton_sends(kNewtonNinf);
    CHECK_BYTES(s.to_dock.bytes, lt_payload(kNewtonNinf));
    CHECK_BYTES(s.to_newt.bytes, kDockLA3);
}

TEST(mnp_sends_dock_data_like_ncu) {
    Session s;
    s.connect();
    s.dock_sends(lt_payload(kDockDock));
    CHECK_BYTES(s.to_newt.bytes, kDockDock);
}
