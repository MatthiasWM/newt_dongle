//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "test.h"

#include "main.h"

#include "common/Endpoints/Dock.h"
#include "common/Newton/NSOF.h"
#include "common/Scheduler.h"

#include <string>
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

// `ninf` as sent by the Newton: protocol version 10 and the DES challenge.
static const Bytes kNewtonNinf = {
    'n', 'e', 'w', 't', 'd', 'o', 'c', 'k', 'n', 'i', 'n', 'f',
    0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x0A,
    0x00, 0x65, 0xE0, 0xBC, 0xFF, 0xA7, 0x74, 0x1B,
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

// Records the data bytes that the Dock sends to the Newton, and where MNP
// frames start and end, e.g. "[0 ]20 " for one frame with 20 bytes.
class Recorder : public Pipe {
public:
    Bytes bytes;
    std::string frames;
    std::vector<Event> events; // all other events
    Result send(Event event) override {
        if (event.is_data()) {
            bytes.push_back(event.data());
        } else if (event.subtype() == Event::Subtype::MNP_FRAME_START) {
            frames += "[" + std::to_string(bytes.size()) + " ";
        } else if (event.subtype() == Event::Subtype::MNP_FRAME_END) {
            frames += "]" + std::to_string(bytes.size()) + " ";
        } else {
            events.push_back(event);
        }
        return Result::OK;
    }
    bool received(Event::Subtype subtype) const {
        for (auto &e : events)
            if (e.type() == Event::Type::MNP && e.subtype() == subtype) return true;
        return false;
    }
};

// The header of a Dock command: "newtdock", the command, and the payload size.
Bytes command(const char *fourcc, uint32_t size) {
    Bytes cmd = { 'n', 'e', 'w', 't', 'd', 'o', 'c', 'k' };
    cmd.insert(cmd.end(), fourcc, fourcc + 4);
    for (int shift = 24; shift >= 0; shift -= 8) cmd.push_back((uint8_t)(size >> shift));
    return cmd;
}

// A Dock command with an object in NSOF as its payload, padded to 4 bytes.
Bytes command_with_ref(const char *fourcc, Ref ref) {
    NSOF nsof;
    nsof.to_nsof(ref);
    Bytes cmd = command(fourcc, (uint32_t)nsof.data().size());
    cmd.insert(cmd.end(), nsof.data().begin(), nsof.data().end());
    while (cmd.size() % 4) cmd.push_back(0);
    return cmd;
}

Bytes command_with_string(const char *fourcc, const char16_t *str) {
    return command_with_ref(fourcc, Ref(String::New(str)));
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
    ~DockSession() {
        sdcard_endpoint.file.clear();
        sdcard_endpoint.readable = SIZE_MAX;
        sdcard_endpoint.is_open = false;
    }
    // Send the bytes as one MNP frame, as MNPFilter does.
    void newton_sends(const Bytes &bytes) {
        dock.send(Event(Event::Type::MNP, Event::Subtype::MNP_FRAME_START));
        for (auto b : bytes) dock.send(Event(b));
        dock.send(Event(Event::Type::MNP, Event::Subtype::MNP_FRAME_END));
        scheduler.run(1000); // the Dock sends one byte per cycle
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

TEST(dock_answers_ninf_with_wicn) {
    DockSession s;
    s.newton_sends(kNewtonNinf);
    CHECK(s.to_newt.bytes.size() >= 12);
    CHECK_BYTES(Bytes(s.to_newt.bytes.begin(), s.to_newt.bytes.begin() + 12),
                (Bytes{ 'n', 'e', 'w', 't', 'd', 'o', 'c', 'k', 'w', 'i', 'c', 'n' }));
}

// `ninf` without the challenge.
TEST(dock_rejects_short_ninf) {
    DockSession s;
    s.newton_sends(command("ninf", 4) + Bytes{ 0x00, 0x00, 0x00, 0x0A });
    CHECK_BYTES(s.to_newt.bytes, kDockDresBadCommandLength);
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

// -- Commands sent by the Dock
//
// These bytes were recorded from the Dock before Job 3.3 replaced the static
// command buffers. The Dock must keep sending exactly these bytes.

// Selective sync is not allowed, because there never was a sync. Older
// firmware sent 1 here.
static const Bytes kDockDinf = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x64, 0x69, 0x6E, 0x66,
    0x00, 0x00, 0x00, 0x66, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00,
    0x5F, 0xFE, 0xF6, 0x6A, 0x5B, 0xE3, 0xDA, 0x62, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x02, 0x05, 0x01, 0x06, 0x04, 0x07, 0x04, 0x6E,
    0x61, 0x6D, 0x65, 0x07, 0x02, 0x69, 0x64, 0x07, 0x07, 0x76, 0x65, 0x72,
    0x73, 0x69, 0x6F, 0x6E, 0x07, 0x08, 0x64, 0x6F, 0x65, 0x73, 0x41, 0x75,
    0x74, 0x6F, 0x08, 0x24, 0x00, 0x4E, 0x00, 0x65, 0x00, 0x77, 0x00, 0x74,
    0x00, 0x6F, 0x00, 0x6E, 0x00, 0x20, 0x00, 0x43, 0x00, 0x6F, 0x00, 0x6E,
    0x00, 0x6E, 0x00, 0x65, 0x00, 0x63, 0x00, 0x74, 0x00, 0x69, 0x00, 0x6F,
    0x00, 0x6E, 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x00, 0x1A, 0x00, 0x00,
};
// The icon bitmap: kInstallIcon. Older firmware sent 0 in the size field.
static const Bytes kDockWicn = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x77, 0x69, 0x63, 0x6E,
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04,
};
static const Bytes kDockStim = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x73, 0x74, 0x69, 0x6D,
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0A,
};
// The answer to the challenge in kNewtonNinf, with an empty password.
static const Bytes kDockPass = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x70, 0x61, 0x73, 0x73,
    0x00, 0x00, 0x00, 0x08, 0x36, 0x5A, 0x08, 0xFF, 0xA1, 0xD8, 0xFC, 0x13,
};
static const Bytes kDockDresOK = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x64, 0x72, 0x65, 0x73,
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
};
// [ { name: "NewtCOM", type: 0 }, { name: "TEST", type: 3 } ]
static const Bytes kDockPath = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x70, 0x61, 0x74, 0x68,
    0x00, 0x00, 0x00, 0x39, 0x02, 0x05, 0x02, 0x06, 0x02, 0x07, 0x04, 0x6E,
    0x61, 0x6D, 0x65, 0x07, 0x04, 0x74, 0x79, 0x70, 0x65, 0x08, 0x10, 0x00,
    0x4E, 0x00, 0x65, 0x00, 0x77, 0x00, 0x74, 0x00, 0x43, 0x00, 0x4F, 0x00,
    0x4D, 0x00, 0x00, 0x00, 0x00, 0x06, 0x02, 0x09, 0x02, 0x09, 0x03, 0x08,
    0x0A, 0x00, 0x54, 0x00, 0x45, 0x00, 0x53, 0x00, 0x54, 0x00, 0x00, 0x00,
    0x0C, 0x00, 0x00, 0x00,
};
// An empty array: the test SD card has no directory entries.
static const Bytes kDockFile = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x66, 0x69, 0x6C, 0x65,
    0x00, 0x00, 0x00, 0x03, 0x02, 0x05, 0x00, 0x00,
};
// File info for "test.pkg" with 5 bytes.
static const Bytes kDockFinf = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x66, 0x69, 0x6E, 0x66,
    0x00, 0x00, 0x00, 0x5B, 0x02, 0x06, 0x06, 0x07, 0x04, 0x6B, 0x69, 0x6E,
    0x64, 0x07, 0x04, 0x73, 0x69, 0x7A, 0x65, 0x07, 0x07, 0x63, 0x72, 0x65,
    0x61, 0x74, 0x65, 0x64, 0x07, 0x08, 0x6D, 0x6F, 0x64, 0x69, 0x66, 0x69,
    0x65, 0x64, 0x07, 0x04, 0x70, 0x61, 0x74, 0x68, 0x07, 0x04, 0x69, 0x63,
    0x6F, 0x6E, 0x08, 0x10, 0x00, 0x50, 0x00, 0x61, 0x00, 0x63, 0x00, 0x6B,
    0x00, 0x61, 0x00, 0x67, 0x00, 0x65, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x12, 0x00, 0x74, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74,
    0x00, 0x2E, 0x00, 0x70, 0x00, 0x6B, 0x00, 0x67, 0x00, 0x00, 0x0A, 0x00,
};
// The package { 1, 2, 3, 4, 5 } and its padding.
static const Bytes kDockLpkg = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x6C, 0x70, 0x6B, 0x67,
    0x00, 0x00, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x00, 0x00,
};
// `dres` with kDockErrBrowserFileNotFound (-28105). Older firmware sent -48403.
static const Bytes kDockDresNoFile = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x64, 0x72, 0x65, 0x73,
    0x00, 0x00, 0x00, 0x04, 0xFF, 0xFF, 0x92, 0x37,
};
// `dres` with NewtonScript errors -48401 (expected an array) and -48402
// (expected a string).
static const Bytes kDockDresNotAnArray = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x64, 0x72, 0x65, 0x73,
    0x00, 0x00, 0x00, 0x04, 0xFF, 0xFF, 0x42, 0xEF,
};
static const Bytes kDockDresNotAString = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x64, 0x72, 0x65, 0x73,
    0x00, 0x00, 0x00, 0x04, 0xFF, 0xFF, 0x42, 0xEE,
};
static const Bytes kDockOcaa = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x6F, 0x63, 0x61, 0x61,
    0x00, 0x00, 0x00, 0x00,
};
static const Bytes kDockDisc = {
    0x6E, 0x65, 0x77, 0x74, 0x64, 0x6F, 0x63, 0x6B, 0x64, 0x69, 0x73, 0x63,
    0x00, 0x00, 0x00, 0x00,
};

TEST(dock_sends_dock) {
    DockSession s;
    s.newton_sends(command("rtdk", 0));
    CHECK_BYTES(s.to_newt.bytes, kDockDock);
    CHECK(s.to_newt.frames == "[0 ]20 ");
}

TEST(dock_sends_dinf) {
    DockSession s;
    s.newton_sends(command("name", 0));
    CHECK_BYTES(s.to_newt.bytes, kDockDinf);
    CHECK(s.to_newt.frames == "[0 ]120 ");
}

TEST(dock_sends_wicn_and_stim) {
    DockSession s;
    s.newton_sends(kNewtonNinf);
    s.newton_sends(command("dres", 0));
    CHECK_BYTES(s.to_newt.bytes, kDockWicn + kDockStim);
    CHECK(s.to_newt.frames == "[0 ]20 [20 ]40 ");
}

TEST(dock_sends_pass) {
    DockSession s;
    s.newton_sends(kNewtonNinf);
    s.newton_sends(command("pass", 0));
    CHECK_BYTES(s.to_newt.bytes, kDockWicn + kDockPass);
    CHECK(s.to_newt.frames == "[0 ]20 [20 ]44 ");
}

TEST(dock_sends_dres) {
    DockSession s;
    s.newton_sends(command("rtbr", 0));
    CHECK_BYTES(s.to_newt.bytes, kDockDresOK);
    CHECK(s.to_newt.frames == "[0 ]20 ");
}

TEST(dock_sends_path) {
    DockSession s;
    s.newton_sends(command("dpth", 0));
    CHECK_BYTES(s.to_newt.bytes, kDockPath);
    CHECK(s.to_newt.frames == "[0 ]76 ");
}

TEST(dock_sends_file) {
    DockSession s;
    s.newton_sends(command("gfil", 0));
    CHECK_BYTES(s.to_newt.bytes, kDockFile);
    CHECK(s.to_newt.frames == "[0 ]20 ");
}

TEST(dock_sends_finf) {
    DockSession s;
    sdcard_endpoint.file = { 1, 2, 3, 4, 5 };
    s.newton_sends(command_with_string("gfin", u"test.pkg"));
    CHECK_BYTES(s.to_newt.bytes, kDockFinf);
    CHECK(s.to_newt.frames == "[0 ]108 ");
}

// The header and the package data are queued separately, but in one frame.
TEST(dock_sends_lpkg) {
    DockSession s;
    sdcard_endpoint.file = { 1, 2, 3, 4, 5 };
    s.newton_sends(command_with_string("lpfl", u"test.pkg"));
    CHECK_BYTES(s.to_newt.bytes, kDockLpkg);
    CHECK(s.to_newt.frames == "[0 ]24 ");
}

TEST(dock_reports_missing_package) {
    DockSession s;
    s.newton_sends(command_with_string("lpfl", u"test.pkg"));
    CHECK_BYTES(s.to_newt.bytes, kDockDresNoFile);
}

TEST(dock_sends_ocaa) {
    DockSession s;
    s.newton_sends(command("opca", 0));
    CHECK_BYTES(s.to_newt.bytes, kDockOcaa);
}

TEST(dock_sends_disc) {
    DockSession s;
    s.dock.send_disc();
    s.scheduler.run(100);
    CHECK_BYTES(s.to_newt.bytes, kDockDisc);
}

// Two replies of the same kind queued before the first one was sent.
TEST(dock_queues_two_dres_with_different_results) {
    DockSession s;
    s.newton_sends(command("xxxx", Dock::kMaxPayload + 1) + command("rtbr", 0));
    CHECK_BYTES(s.to_newt.bytes, kDockDresBadCommandLength + kDockDresOK);
}

TEST(dock_rejects_spth_without_array) {
    DockSession s;
    s.newton_sends(command_with_string("spth", u"TEST"));
    CHECK_BYTES(s.to_newt.bytes, kDockDresNotAnArray);
}

TEST(dock_rejects_spth_with_a_folder_that_is_not_a_string) {
    DockSession s;
    Ref path = Ref(Array::New({ Ref(String::New(u"NewtCOM")), Ref(String::New(u"TEST")), Ref((int32_t)5) }));
    s.newton_sends(command_with_ref("spth", path));
    CHECK_BYTES(s.to_newt.bytes, kDockDresNotAString);
}

// -- Package transfer errors

TEST(dock_reports_file_info_for_missing_file) {
    DockSession s;
    s.newton_sends(command_with_string("gfin", u"test.pkg"));
    CHECK_BYTES(s.to_newt.bytes, kDockDresNoFile);
}

// A package that can't be read completely must not be installed. The Dock
// asks MNP to end the session, because the Newton reads anything that follows
// the lpkg header as package data.
TEST(dock_disconnects_on_sd_read_error) {
    DockSession s;
    sdcard_endpoint.file = Bytes(600, 0x55);
    sdcard_endpoint.readable = 100;
    s.newton_sends(command_with_string("lpfl", u"test.pkg"));
    CHECK_BYTES(s.to_newt.bytes, command("lpkg", 600));
    CHECK(s.to_newt.received(Event::Subtype::MNP_SEND_LD));
    CHECK(!sdcard_endpoint.is_open);

    s.to_newt.bytes.clear();
    s.newton_sends(command("rtdk", 0));
    CHECK_BYTES(s.to_newt.bytes, kDockDock);
}

TEST(dock_closes_package_on_disconnect) {
    DockSession s;
    sdcard_endpoint.file = Bytes(4000, 0x55);
    s.newton_sends(command_with_string("lpfl", u"test.pkg"));
    CHECK(sdcard_endpoint.is_open);
    s.dock.send(Event(Event::Type::MNP, Event::Subtype::MNP_DISCONNECTED));
    CHECK(!sdcard_endpoint.is_open);
}
