//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "test.h"

#include "common/Endpoints/Dock.h"
#include "common/Scheduler.h"

#include <vector>

using namespace nd;

typedef std::vector<uint8_t> Bytes;

// `rtdk` as sent by the Newton, and the `dock` reply as sent by NCU. These
// are the payloads of the captured LT frames in test_mnp.cpp.
static const Bytes kNewtonRtdk = {
    'n', 'e', 'w', 't', 'd', 'o', 'c', 'k', 'r', 't', 'd', 'k',
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x09,
};
static const Bytes kDockDock = {
    'n', 'e', 'w', 't', 'd', 'o', 'c', 'k', 'd', 'o', 'c', 'k',
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01,
};

// `dres` with error kDockErrBadCommandLength (-28007).
static const Bytes kDockDresBadCommandLength = {
    'n', 'e', 'w', 't', 'd', 'o', 'c', 'k', 'd', 'r', 'e', 's',
    0x00, 0x00, 0x00, 0x04, 0xFF, 0xFF, 0x92, 0x99,
};

// -- Test fixture

namespace {

class TestScheduler : public Scheduler {
protected:
    void update_time() override { cycle_time_ = 100; }
};

// Records the data bytes that the Dock sends to the Newton.
class Recorder : public Pipe {
public:
    Bytes bytes;
    Result send(Event event) override {
        if (event.is_data()) bytes.push_back(event.data());
        return Result::OK;
    }
};

// The header of a Dock command: "newtdock", the command, and the payload size.
Bytes command(const char *fourcc, uint32_t size) {
    Bytes cmd = { 'n', 'e', 'w', 't', 'd', 'o', 'c', 'k' };
    cmd.insert(cmd.end(), fourcc, fourcc + 4);
    for (int shift = 24; shift >= 0; shift -= 8) cmd.push_back((uint8_t)(size >> shift));
    return cmd;
}

Bytes operator+(Bytes a, const Bytes &b) {
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

struct DockSession {
    TestScheduler scheduler;
    Dock dock { scheduler };
    Recorder to_newt;

    DockSession() {
        dock >> to_newt;
        scheduler.init();
    }
    // Send the bytes as one MNP frame, as MNPFilter does.
    void newton_sends(const Bytes &bytes) {
        dock.send(Event(Event::Type::MNP, Event::Subtype::MNP_FRAME_START));
        for (auto b : bytes) dock.send(Event(b));
        dock.send(Event(Event::Type::MNP, Event::Subtype::MNP_FRAME_END));
        scheduler.run(100);
    }
};

} // namespace

// -- Tests

TEST(dock_answers_rtdk_like_ncu) {
    DockSession s;
    s.newton_sends(kNewtonRtdk);
    CHECK_BYTES(s.to_newt.bytes, kDockDock);
}

// A size that we can't buffer, e.g. after the stream got out of sync.
TEST(dock_rejects_payload_over_limit) {
    DockSession s;
    s.newton_sends(command("xxxx", Dock::kMaxPayload + 1));
    CHECK_BYTES(s.to_newt.bytes, kDockDresBadCommandLength);

    s.to_newt.bytes.clear();
    s.newton_sends(kNewtonRtdk);
    CHECK_BYTES(s.to_newt.bytes, kDockDock);
}

TEST(dock_accepts_payload_at_limit) {
    DockSession s;
    s.newton_sends(command("xxxx", Dock::kMaxPayload) + Bytes(Dock::kMaxPayload, 0));
    CHECK(s.to_newt.bytes.empty());
    s.newton_sends(kNewtonRtdk);
    CHECK_BYTES(s.to_newt.bytes, kDockDock);
}

// Commands start at an MNP frame, so a new frame ends a half received header.
TEST(dock_restarts_header_on_new_frame) {
    DockSession s;
    s.newton_sends(Bytes(kNewtonRtdk.begin(), kNewtonRtdk.begin() + 10));
    s.newton_sends(kNewtonRtdk);
    CHECK_BYTES(s.to_newt.bytes, kDockDock);
}

// A payload may continue in the next MNP frame.
TEST(dock_continues_payload_in_next_frame) {
    DockSession s;
    s.newton_sends(command("xxxx", 8) + Bytes{ 1, 2, 3, 4 });
    s.newton_sends(Bytes{ 5, 6, 7, 8 });
    s.newton_sends(kNewtonRtdk);
    CHECK_BYTES(s.to_newt.bytes, kDockDock);
}

// A failed match of "newtdock" may be the start of the next "newtdock".
TEST(dock_resyncs_after_stray_n) {
    DockSession s;
    s.newton_sends(Bytes{ 'n' } + kNewtonRtdk);
    CHECK_BYTES(s.to_newt.bytes, kDockDock);

    s.to_newt.bytes.clear();
    s.newton_sends(Bytes{ 'n', 'e', 'w', 't' } + kNewtonRtdk);
    CHECK_BYTES(s.to_newt.bytes, kDockDock);
}
