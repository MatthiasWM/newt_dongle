//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

// Complete docking sessions through MNPFilter and Dock. The test plays the
// Newton: it sends Dock commands in LT frames, decodes every frame the dongle
// sends, and acknowledges the dongle's LT frames like the Newton does.

#include "test.h"

#include "main.h"

#include "common/Endpoints/Dock.h"
#include "common/Filters/MNPFilter.h"
#include "common/Newton/NSOF.h"
#include "common/Scheduler.h"

#include "captured.h"
#include "mnp_util.h"
#include "test_doubles.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace nd;

namespace {

class TestScheduler : public Scheduler {
protected:
    void update_time() override { cycle_time_ = 100; }
};

// Records the bytes that the dongle sends to the Newton.
class Wire : public Pipe {
public:
    Bytes bytes;
    Result send(Event event) override {
        if (event.is_data()) bytes.push_back(event.data());
        return Result::OK;
    }
};

struct Frame {
    Bytes header;
    Bytes data;
    uint8_t type() const { return header.empty() ? 0 : header[0]; }
    uint8_t seq() const { return header.size() > 1 ? header[1] : 0; }
};

struct DockCommand {
    std::string fourcc;
    uint32_t size = 0;
    Bytes payload;      // without padding
    bool truncated = false; // a new command started before the payload was complete
};

// Decode one frame at `pos`. Returns false if the bytes at `pos` are not a
// complete frame with a correct CRC.
bool decode_frame(const Bytes &bytes, size_t &pos, Frame &frame) {
    size_t i = pos;
    if (i + 3 > bytes.size() || bytes[i] != 0x16 || bytes[i+1] != 0x10 || bytes[i+2] != 0x02)
        return false;
    i += 3;
    Bytes content; // size, header and data, unescaped
    for (;;) {
        if (i + 1 >= bytes.size()) return false;
        if (bytes[i] == 0x10) {
            if (bytes[i+1] == 0x10) { content.push_back(0x10); i += 2; continue; }
            if (bytes[i+1] == 0x03) { i += 2; break; }
            return false;
        }
        content.push_back(bytes[i++]);
    }
    if (i + 2 > bytes.size() || content.empty() || content[0] + 1u > content.size())
        return false;
    frame.header.assign(content.begin() + 1, content.begin() + 1 + content[0]);
    frame.data.assign(content.begin() + 1 + content[0], content.end());
    i += 2; // CRC
    // Encoding the frame again must give exactly the same bytes, CRC included.
    if (make_frame(frame.header, frame.data) != Bytes(bytes.begin() + pos, bytes.begin() + i))
        return false;
    pos = i;
    return true;
}

struct NewtonSession {
    TestScheduler scheduler;
    MNPFilter mnp { scheduler };
    TestSDCard sdcard { scheduler };
    TestStatusDisplay status { scheduler };
    Dock dock { scheduler, sdcard, status };
    Wire wire;
    size_t wire_pos = 0;
    bool wire_ok = true;            // all bytes from the dongle were valid frames

    uint8_t newton_seq = 0;         // last LT sent by the Newton
    uint8_t dongle_seq = 0;         // last LT received from the dongle
    bool dongle_seq_ok = true;      // the dongle's LT numbers counted up by one
    std::vector<uint8_t> acks;      // LA sequence numbers sent by the dongle
    std::vector<uint8_t> types;     // types of all frames from the dongle
    Bytes dock_stream;              // payload of the dongle's LT frames
    std::vector<size_t> lt_starts;  // where each LT frame starts in dock_stream

    NewtonSession() {
        mnp.newt >> dock;
        dock >> mnp.dock;
        mnp.dock >> wire;
        scheduler.init();
    }

    void newton_sends_frame(const Bytes &frame) {
        for (auto b : frame) mnp.newt.send(Event(b));
        run();
    }
    // Run the dongle, and acknowledge its LT frames until it is quiet, or
    // until `max_las` frames were acknowledged.
    void run(int max_las = 1000) {
        for (int round = 0; round < 1000; round++) {
            scheduler.run(2000);
            bool sent_la = false;
            Frame frame;
            while (wire_pos < wire.bytes.size()) {
                size_t frame_pos = wire_pos;
                if (!decode_frame(wire.bytes, wire_pos, frame)) {
                    wire_ok = false;
                    wire_pos = wire.bytes.size();
                    break;
                }
                if ((frame.type() == kMNP_Frame_LT) && (max_las-- <= 0)) {
                    wire_pos = frame_pos; // acknowledge it in the next run()
                    return;
                }
                types.push_back(frame.type());
                if (frame.type() == kMNP_Frame_LA) {
                    acks.push_back(frame.seq());
                } else if (frame.type() == kMNP_Frame_LT) {
                    if (frame.seq() != (uint8_t)(dongle_seq + 1)) dongle_seq_ok = false;
                    dongle_seq = frame.seq();
                    lt_starts.push_back(dock_stream.size());
                    dock_stream.insert(dock_stream.end(), frame.data.begin(), frame.data.end());
                    for (auto b : make_frame({ kMNP_Frame_LA, dongle_seq, 0x01 }, {}))
                        mnp.newt.send(Event(b));
                    sent_la = true;
                }
            }
            if (!sent_la) return;
        }
    }
    void connect() {
        newton_sends_frame(kLR);
        newton_sends_frame(kNewtonLA0);
    }
    // Send a Dock command in an LT frame.
    void newton_sends(const Bytes &command) {
        newton_seq++;
        newton_sends_frame(make_frame({ kMNP_Frame_LT, newton_seq }, command));
    }
    bool starts_command(size_t i) const {
        static const Bytes kNewtDock = { 'n', 'e', 'w', 't', 'd', 'o', 'c', 'k' };
        return (i + 16 <= dock_stream.size())
            && std::equal(kNewtDock.begin(), kNewtDock.end(), dock_stream.begin() + i);
    }
    // Split the dongle's LT payload into Dock commands. Every command starts
    // in a new LT frame, which ends a payload that was cut short (cancel).
    std::vector<DockCommand> dock_commands() const {
        std::vector<DockCommand> commands;
        size_t i = 0;
        while (starts_command(i)) {
            DockCommand cmd;
            cmd.fourcc.assign(dock_stream.begin() + i + 8, dock_stream.begin() + i + 12);
            for (int k = 12; k < 16; k++) cmd.size = (cmd.size << 8) | dock_stream[i + k];
            size_t start = i + 16;
            size_t end = std::min<size_t>(start + ((cmd.size + 3) & ~3u), dock_stream.size());
            for (size_t lt : lt_starts) {
                if (lt > i && lt < end && starts_command(lt)) {
                    end = lt;
                    cmd.truncated = true;
                    break;
                }
            }
            cmd.payload.assign(dock_stream.begin() + start,
                               dock_stream.begin() + std::min<size_t>(start + cmd.size, end));
            commands.push_back(cmd);
            i = end;
        }
        return commands;
    }
    std::vector<std::string> fourccs() const {
        std::vector<std::string> list;
        for (auto &cmd : dock_commands()) list.push_back(cmd.fourcc);
        return list;
    }
};

Bytes command(const char *fourcc, const Bytes &payload = {}) {
    Bytes cmd = { 'n', 'e', 'w', 't', 'd', 'o', 'c', 'k' };
    cmd.insert(cmd.end(), fourcc, fourcc + 4);
    uint32_t size = (uint32_t)payload.size();
    for (int shift = 24; shift >= 0; shift -= 8) cmd.push_back((uint8_t)(size >> shift));
    cmd.insert(cmd.end(), payload.begin(), payload.end());
    while (cmd.size() % 4) cmd.push_back(0);
    return cmd;
}

Bytes nsof_string(const char16_t *str) {
    NSOF nsof;
    nsof.to_nsof(Ref(String::New(str)));
    return nsof.data();
}

// The Newton's side of a session, up to the point where the user can browse.
void dock_and_browse(NewtonSession &s) {
    s.connect();
    s.newton_sends(lt_payload(kNewtonRtdk));
    s.newton_sends(lt_payload(kNewtonName));
    s.newton_sends(lt_payload(kNewtonNinf));
    s.newton_sends(command("dres", { 0, 0, 0, 0 }));
    s.newton_sends(command("pass", { 0, 0, 0, 0, 0, 0, 0, 0 }));
    s.newton_sends(command("rtbr", nsof_string(u"packages")));
    s.newton_sends(command("dpth"));
    s.newton_sends(command("gfil"));
    s.newton_sends(command("gfin", nsof_string(u"test.pkg")));
}

} // namespace

// -- Tests

TEST(session_installs_a_package) {
    NewtonSession s;
    Bytes package;
    for (int i = 0; i < 600; i++) package.push_back((uint8_t)(i * 7));
    s.sdcard.file = package;

    dock_and_browse(s);
    s.newton_sends(command("lpfl", nsof_string(u"test.pkg")));
    s.newton_sends(command("dres", { 0, 0, 0, 0 })); // the Newton installed the package
    s.newton_sends(command("disc"));
    s.newton_sends_frame(kLD);

    CHECK(s.wire_ok);
    CHECK(s.dongle_seq_ok);
    CHECK(s.status.status == AppStatus::IDLE); // the Newton disconnected
    // Every LT from the Newton was acknowledged once, in order.
    std::vector<uint8_t> expected_acks;
    for (uint8_t i = 1; i <= s.newton_seq; i++) expected_acks.push_back(i);
    CHECK(s.acks == expected_acks);

    std::vector<std::string> expected = {
        "dock", "dinf", "wicn", "stim", "pass", "dres", "path", "file", "finf", "lpkg" };
    CHECK(s.fourccs() == expected);

    auto commands = s.dock_commands();
    CHECK(commands.size() == expected.size());
    if (commands.size() == expected.size()) {
        CHECK(commands[5].payload == Bytes({ 0, 0, 0, 0 }));   // rtbr: OK
        CHECK(commands[9].size == 600);                        // lpkg: the whole file
        CHECK_BYTES(commands[9].payload, package);
    }
    // The 600 byte package does not fit into one LT frame.
    CHECK(s.dongle_seq >= 12);
    CHECK(!s.sdcard.is_open);
}

// The user taps [X] while the package is sent.
TEST(session_cancels_a_package_install) {
    NewtonSession s;
    s.sdcard.file = Bytes(20000, 0x55);

    dock_and_browse(s);
    // Send lpfl, but acknowledge only the first few frames of the package.
    s.newton_seq++;
    for (auto b : make_frame({ kMNP_Frame_LT, s.newton_seq }, command("lpfl", nsof_string(u"test.pkg"))))
        s.mnp.newt.send(Event(b));
    s.run(3);
    CHECK(s.sdcard.is_open);
    s.newton_sends(command("opca"));

    CHECK(s.wire_ok);
    CHECK(s.dongle_seq_ok);
    auto commands = s.dock_commands();
    CHECK(commands.size() >= 2);
    if (commands.size() >= 2) {
        CHECK(commands[commands.size() - 2].fourcc == "lpkg");
        CHECK(commands[commands.size() - 2].truncated);
        CHECK(commands.back().fourcc == "ocaa");
    }
    CHECK(!s.sdcard.is_open);
    // The Dock is ready for the next command.
    s.newton_sends(command("gfil"));
    auto list = s.fourccs();
    CHECK(!list.empty() && list.back() == "file");
}
