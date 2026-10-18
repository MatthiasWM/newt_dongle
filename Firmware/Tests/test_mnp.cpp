//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "test.h"

#include "main.h"

#include "common/Endpoints/Dock.h"
#include "common/Filters/MNPFilter.h"
#include "common/Newton/NSOF.h"
#include "common/Scheduler.h"

#include "captured.h"
#include "mnp_util.h"
#include "test_doubles.h"

#include <vector>

using namespace nd;

// Link Disconnect sent by MNPFilter, reason 4 (retransmission limit reached).
// Not captured; the CRC was calculated like the CRCs of the captured frames.
static const Bytes kDockLD4 = {
    0x16, 0x10, 0x02, 0x04, 0x02, 0x01, 0x01, 0x04, 0x10, 0x03, 0x6A, 0xB9,
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
    void wait_us(uint64_t us) {
        scheduler.run((int)(us / 100)); // TestScheduler: 100 usec per cycle
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

// Our LA got lost and the Newton sends the same LT again.
TEST(mnp_acknowledges_duplicate_lt_but_drops_it) {
    Session s;
    s.connect();
    s.newton_sends(kNewtonRtdk);
    s.newton_sends(kNewtonRtdk);
    CHECK_BYTES(s.to_dock.bytes, lt_payload(kNewtonRtdk));
    Bytes two_las = kDockLA1;
    two_las.insert(two_las.end(), kDockLA1.begin(), kDockLA1.end());
    CHECK_BYTES(s.to_newt.bytes, two_las);

    s.to_dock.clear(); s.to_newt.clear();
    s.newton_sends(kNewtonName);
    CHECK_BYTES(s.to_dock.bytes, lt_payload(kNewtonName));
    CHECK_BYTES(s.to_newt.bytes, kDockLA2);
}

// An LT out of sequence is dropped, and we acknowledge the last good one again.
TEST(mnp_drops_lt_out_of_sequence) {
    Session s;
    s.connect();
    s.newton_sends(kNewtonName);
    CHECK(s.to_dock.bytes.empty());
    CHECK_BYTES(s.to_newt.bytes, kNewtonLA0); // same bytes as our LA for seq 0

    s.to_newt.clear();
    s.newton_sends(kNewtonRtdk);
    CHECK_BYTES(s.to_dock.bytes, lt_payload(kNewtonRtdk));
    CHECK_BYTES(s.to_newt.bytes, kDockLA1);
}

// Our LT got lost. We resend it after the timeout until the Newton acknowledges it.
TEST(mnp_resends_lt_without_la) {
    Session s;
    s.connect();
    s.dock_sends(lt_payload(kDockDock));
    s.to_newt.clear();

    s.wait_us(MNPFilter::kRetransmitTimeout - 100'000);
    CHECK(s.to_newt.bytes.empty());
    s.wait_us(100'000);
    CHECK_BYTES(s.to_newt.bytes, kDockDock);

    s.newton_sends(kDockLA1); // same bytes as the Newton's LA for seq 1
    s.to_newt.clear();
    s.wait_us(2 * MNPFilter::kRetransmitTimeout);
    CHECK(s.to_newt.bytes.empty());
    s.dock_sends(lt_payload(kDockDock));
    CHECK(s.to_newt.bytes.size() > 5 && s.to_newt.bytes[5] == 2); // next LT has seq 2
}

// The Newton is gone. We give up after the last resend and disconnect.
TEST(mnp_disconnects_after_retransmit_limit) {
    Session s;
    s.connect();
    s.dock_sends(lt_payload(kDockDock));
    s.wait_us((MNPFilter::kMaxRetransmits + 1) * MNPFilter::kRetransmitTimeout + 100'000);

    Bytes expected;
    for (int i = 0; i <= MNPFilter::kMaxRetransmits; i++)
        expected.insert(expected.end(), kDockDock.begin(), kDockDock.end());
    expected.insert(expected.end(), kDockLD4.begin(), kDockLD4.end());
    CHECK_BYTES(s.to_newt.bytes, expected);
    CHECK(s.to_dock.received(Event::Type::MNP, Event::Subtype::MNP_DISCONNECTED));

    s.connect();
    s.dock_sends(lt_payload(kDockDock));
    CHECK_BYTES(s.to_newt.bytes, kDockDock);
}

TEST(mnp_test_frame_builder_matches_capture) {
    CHECK_BYTES(make_frame({ 0x04, 0x01 }, lt_payload(kNewtonRtdk)), kNewtonRtdk);
    CHECK_BYTES(make_frame({ 0x04, 0x02 }, lt_payload(kNewtonName)), kNewtonName);
    CHECK_BYTES(make_frame({ 0x02, 0x01, 0x01, 0x04 }, {}), kDockLD4);
}

// SYN SYN DLE STX is a valid frame start.
TEST(mnp_accepts_repeated_syn) {
    Session s;
    s.connect();
    s.newton_sends({ 0x16 });
    s.newton_sends(kNewtonRtdk);
    CHECK_BYTES(s.to_dock.bytes, lt_payload(kNewtonRtdk));
    CHECK_BYTES(s.to_newt.bytes, kDockLA1);
}

// The end of an LT got lost. DLE STX starts the next frame.
TEST(mnp_restarts_on_dle_stx_in_data) {
    Session s;
    s.connect();
    s.newton_sends(Bytes(kNewtonRtdk.begin(), kNewtonRtdk.begin() + 20));
    s.newton_sends(kNewtonRtdk);
    CHECK_BYTES(s.to_dock.bytes, lt_payload(kNewtonRtdk));
    CHECK_BYTES(s.to_newt.bytes, kDockLA1);
}

// The end of an LR header got lost. DLE STX starts the next frame.
TEST(mnp_restarts_on_dle_stx_in_header) {
    Session s;
    s.newton_sends(Bytes(kLR.begin(), kLR.begin() + 10));
    s.newton_sends(kLR);
    CHECK(s.to_dock.received(Event::Type::MNP, Event::Subtype::MNP_NEGOTIATING));
    CHECK_BYTES(s.to_newt.bytes, kLR);
}

// A header size of 16 is sent as DLE DLE.
TEST(mnp_unescapes_header_size) {
    Session s;
    s.connect();
    Bytes ld = make_frame({ 0x02, 0x01, 0x01, 0xFF, 0x02, 0x01, 0x00,
                            0x03, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 }, {});
    CHECK(ld[3] == 0x10 && ld[4] == 0x10);
    s.newton_sends(ld);
    CHECK(s.to_dock.received(Event::Type::MNP, Event::Subtype::MNP_DISCONNECTED));
}

// An LT may carry up to 256 bytes of data. Larger frames are dropped.
TEST(mnp_drops_lt_with_too_much_data) {
    Session s;
    s.connect();
    s.newton_sends(make_frame({ 0x04, 0x01 }, Bytes(256, 'x')));
    CHECK_BYTES(s.to_dock.bytes, Bytes(256, 'x'));
    CHECK_BYTES(s.to_newt.bytes, kDockLA1);

    s.to_dock.clear(); s.to_newt.clear();
    s.newton_sends(make_frame({ 0x04, 0x02 }, Bytes(257, 'x')));
    CHECK(s.to_dock.bytes.empty());
    CHECK(s.to_newt.bytes.empty());
    s.newton_sends(kNewtonName);
    CHECK_BYTES(s.to_dock.bytes, lt_payload(kNewtonName));
    CHECK_BYTES(s.to_newt.bytes, kDockLA2);
}

// The Dock can't continue, e.g. after an SD card error, and ends the session.
TEST(mnp_sends_ld_for_dock) {
    Session s;
    s.connect();
    s.mnp.dock.send(Event(Event::Type::MNP, Event::Subtype::MNP_SEND_LD, 255));
    s.scheduler.run(500);
    CHECK_BYTES(s.to_newt.bytes, make_frame({ 0x02, 0x01, 0x01, 0xFF }, {}));
    CHECK(s.to_dock.received(Event::Type::MNP, Event::Subtype::MNP_DISCONNECTED));
}

// MNPFilter and Dock together: an SD card error in the middle of a package
// transfer ends the session with LD, and the Dock closes the package.
TEST(mnp_and_dock_end_session_on_sd_read_error) {
    TestScheduler scheduler;
    MNPFilter mnp { scheduler };
    TestSDCard sdcard { scheduler };
    TestStatusDisplay status { scheduler };
    Dock dock { scheduler, sdcard, status };
    Recorder to_newt;
    mnp.newt >> dock;
    dock >> mnp.dock;
    mnp.dock >> to_newt;
    scheduler.init();
    auto newton_sends = [&](const Bytes &frame) {
        for (auto b : frame) mnp.newt.send(Event(b));
        scheduler.run(2000);
    };
    newton_sends(kLR);
    newton_sends(kNewtonLA0);
    to_newt.clear();

    NSOF nsof;
    nsof.to_nsof(Ref(String::New(u"test.pkg")));
    Bytes lpfl = { 'n', 'e', 'w', 't', 'd', 'o', 'c', 'k', 'l', 'p', 'f', 'l',
                   0x00, 0x00, 0x00, (uint8_t)nsof.data().size() };
    lpfl.insert(lpfl.end(), nsof.data().begin(), nsof.data().end());
    while (lpfl.size() % 4) lpfl.push_back(0);

    sdcard.file = Bytes(600, 0x55);
    sdcard.readable = 100;
    newton_sends(make_frame({ 0x04, 0x01 }, lpfl));
    Bytes expected = kDockLA1;
    Bytes ld = make_frame({ 0x02, 0x01, 0x01, 0xFF }, {});
    expected.insert(expected.end(), ld.begin(), ld.end());
    CHECK_BYTES(to_newt.bytes, expected);
    CHECK(!sdcard.is_open);
}

// The Newton never acknowledges our LT and starts over with a new LR.
TEST(mnp_new_lr_resets_unacknowledged_session) {
    Session s;
    s.connect();
    s.dock_sends(lt_payload(kDockDock));
    s.to_newt.clear();

    s.newton_sends(kLR);
    CHECK(s.to_dock.received(Event::Type::MNP, Event::Subtype::MNP_DISCONNECTED));
    CHECK_BYTES(s.to_newt.bytes, kLR);
    s.newton_sends(kNewtonLA0);
    s.to_newt.clear();
    s.dock_sends(lt_payload(kDockDock));
    CHECK_BYTES(s.to_newt.bytes, kDockDock);
}

// The Newton disconnects while our LT is not acknowledged, then connects again.
TEST(mnp_ld_resets_unacknowledged_session) {
    Session s;
    s.connect();
    s.dock_sends(lt_payload(kDockDock));

    s.newton_sends(kLD);
    CHECK(s.to_dock.received(Event::Type::MNP, Event::Subtype::MNP_DISCONNECTED));
    s.connect();
    s.dock_sends(lt_payload(kDockDock));
    CHECK_BYTES(s.to_newt.bytes, kDockDock);
}

// DTR changes in the middle of a session and of a received frame.
TEST(mnp_dtr_change_resets_session) {
    Session s;
    s.connect();
    s.dock_sends(lt_payload(kDockDock));
    s.newton_sends(Bytes(kNewtonRtdk.begin(), kNewtonRtdk.begin() + 10));
    s.to_dock.clear();

    s.mnp.newt.rush(Event(Event::Type::UART, Event::Subtype::UART_DTR, 1));
    CHECK(s.to_dock.received(Event::Type::MNP, Event::Subtype::MNP_DISCONNECTED));
    s.mnp.newt.rush(Event(Event::Type::UART, Event::Subtype::UART_DTR, 0));
    s.to_newt.clear();

    s.newton_sends(kLR);
    CHECK_BYTES(s.to_newt.bytes, kLR);
    s.newton_sends(kNewtonLA0);
    s.to_newt.clear();
    s.dock_sends(lt_payload(kDockDock));
    CHECK_BYTES(s.to_newt.bytes, kDockDock);
}
