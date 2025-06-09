//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "HayesFilter.h"

#include "main.h"

#include "common/Scheduler.h"
#include "common/Endpoints/SDCardEndpoint.h"
#include "common/Pipes/MNPThrottle.h"

//#include <pico/unique_id.h>
#include <stdio.h>
#include <cstring>
#include <stdlib.h>

using namespace nd;

/**
 * \brief Create a new Hayes filter.
 * 
 * The Hayes filter adds Hayes modem functionality via AT commands to the line.
 * The filter has two pipes: Upstream is the pipe from the user to the modem,
 * or in this case, from the MP to the dongle, or from the PC to the dongle.
 * Downstream is the pipe from the dongle to any device.
 * 
 * To get the filter into command mode, send pause-"+++"-pause to the Upstream
 * pipe. The filter will then send a response to the downstream pipe with "OK".
 * 
 * To go back into data mode, send "ATO".
 * 
 * \todo allow dropping DTR (dropping GPI on the InterconnectPort?) to go into 
 *       command mode. Could be a user settable option.
 * 
 * \note When entering "+++", the current implementation holds back the '+'
 *       characters until the timeout expires. This is not a standard Hayes
 *       implementation and may vary between devices.
 * 
 * \param scheduler The scheduler to use for this filter.
 */
HayesFilter::HayesFilter(Scheduler &scheduler, uint8_t ix)
:   Task(scheduler, Scheduler::TASKS | Scheduler::SIGNALS),
    upstream(*this),
    downstream(*this),
    index_(ix)
{
}

HayesFilter::~HayesFilter() {
}

void HayesFilter::switch_to_command_mode() {
    data_mode_ = false;
    command_mode_timeout_ = 0;
    command_mode_progress_ = 0;
    send_OK();
}

void HayesFilter::switch_to_data_mode() {
    send_CONNECT();
    data_mode_ = true;
    command_mode_timeout_ = 0;
    command_mode_progress_ = 0;
}

Result HayesFilter::signal(Event event) {
    uint8_t value = 0;
    if (event.type() != Event::Type::SIGNAL)
        return Result::OK;
    switch (event.subtype()) {
        case Event::Subtype::USER_SETTINGS_CHANGED:
            if (index_ == 0) {
                value = user_settings.data.hayes0_esc_code_guard_time;
            } else {
                value = user_settings.data.hayes1_esc_code_guard_time;
            }
            esc_code_guard_timeout_ = value * 20'000; // 20ms
            break;
    }
    return Result::OK;
}


/**
 * \brief Called regularly by the scheduler to take care of the filter.
 */
Result HayesFilter::task() {
    // Filter out the pause-"+++"-pause sequence."
    // This works only in cooperation with downstream_send(Event).
    command_mode_timeout_ += scheduler().cycle_time();
    if (data_mode_) {
        switch (command_mode_progress_) {
            case 0: // waiting for the first pause
                if (command_mode_timeout_ > esc_code_guard_timeout_) {
                    command_mode_progress_ = 1;
                    command_mode_timeout_ = 0;
                    if (kDebugHayes) printf("**** HayesFilter task: 0 -> 1 (Good: no character for a while)\n");
                }
                break;
            case 1: // waiting for the first '+'
                break;
            case 2: // waiting for the second '+'
                if (command_mode_timeout_ > esc_code_guard_timeout_) {
                    if (kDebugHayes) printf("**** HayesFilter task: %d -> 0 (Bad: no + in time)\n", command_mode_progress_);
                    command_mode_timeout_ = 0;
                    command_mode_progress_ = 0;
                    upstream.out()->send(Event('+')); // Make up for the '+' we withheld.
                }
                break;
            case 3: // waiting for the third '+'
                if (command_mode_timeout_ > esc_code_guard_timeout_) {
                    if (kDebugHayes) printf("**** HayesFilter task: %d -> 0 (Bad: no + in time)\n", command_mode_progress_);
                    command_mode_timeout_ = 0;
                    command_mode_progress_ = 0;
                    upstream.out()->send(Event('+')); // Make up for the two '+'s we withheld.
                    upstream.out()->send(Event('+'));
                }
                break;
            case 4: // waiting for the last pause
                if (command_mode_timeout_ > esc_code_guard_timeout_) {
                    command_mode_progress_ = 0;
                    if (kDebugHayes) printf("**** HayesFilter task: COMMAND MODE (Good: no character for a while)\n");
                    switch_to_command_mode();
                    return Result::OK;        
                }
                break;
        }
    }
    
    return Result::OK;
}

/**
 * \brief Called whenever the upstream pipe has sent us an Event.
 * 
 * Upstream is always the dongle iself, as it behaves like a modem to the MP,
 * either via UART or USB.
 */
Result HayesFilter::upstream_send(Event event) {
    if (data_mode_) {
        // Forward the events down stream.
        Pipe *down = downstream.out();
        if (!down)
            return Result::OK__NOT_CONNECTED;
        return down->send(event);
    } else {
        // Every event sent from upstream to downstream in command mode is lost.
        // \todo We can buffer stuff and send it later.
        // \todo We can send a "RING" Event to the downstream pipe on high water.
        return Result::OK__NOT_CONNECTED;
    }
}

/**
 * \brief Called whenever the downstream pipe has sent us an Event.
 * 
 * Downstream would be the MessagePad or the PC.
 */
Result HayesFilter::downstream_send(Event event) 
{
    Pipe *up = upstream.out();
    Pipe *down = downstream.out();

    // Filter out the pause-"+++"-pause sequence."
    // This works only in cooperation with task() and the scheduler.
    if (data_mode_ && event.is_data()) {
        switch (command_mode_progress_) {
            case 0: // waiting for the first pause
                command_mode_timeout_ = 0;
                break;
            case 1: // waiting for the first '+'
                if (event.data() == '+') {
                    command_mode_progress_ = 2;
                    command_mode_timeout_ = 0;
                    if (kDebugHayes) printf("**** HayesFilter send: 1 -> 2 (Good: received first +)\n");
                    return Result::OK; // Don't send the '+' just yet.
                } else {
                    if (kDebugHayes) printf("**** HayesFilter send: 1 -> 0 (Bad: received unexpected character)\n");
                    command_mode_progress_ = 0;
                    command_mode_timeout_ = 0;
                }
                break; // Continue and send the latest event.
            case 2: // waiting for the second '+'
                if (event.data() == '+') {
                    command_mode_progress_ = 3;
                    command_mode_timeout_ = 0;
                    if (kDebugHayes) printf("**** HayesFilter send: 2 -> 3 (Good: received second +)\n");
                    return Result::OK; // Don't send the '+' just yet.
                } else {
                    upstream.out()->send(Event ('+')); // Make up for the '+' we withheld.
                    if (kDebugHayes) printf("**** HayesFilter send: 2 -> 0 (Bad: received unexpected character)\n");
                    command_mode_progress_ = 0;
                    command_mode_timeout_ = 0;
                }
                break; // Continue and send the latest event.
            case 3: // waiting for the third '+'
                if (event.data() == '+') {
                    if (kDebugHayes) printf("**** HayesFilter send: 3 -> 4 (Good: received third +)\n");
                    command_mode_progress_ = 4;
                    command_mode_timeout_ = 0;
                    return Result::OK; // Don't send the '+' just yet.
                } else {
                    upstream.out()->send(Event ('+')); // Make up for the two '+'s we withheld.
                    upstream.out()->send(Event ('+'));
                    if (kDebugHayes) printf("**** HayesFilter send: 3 -> 0 (Bad: received unexpected character)\n");
                    command_mode_progress_ = 0;
                    command_mode_timeout_ = 0;
                }
                break; // Continue and send the latest event. 
            case 4: // waiting for the last pause
                if (kDebugHayes) printf("**** HayesFilter send: 4 -> 0 (Bad: received unexpected character)\n");
                upstream.out()->send(Event ('+')); // Make up for the three '+'s we withheld.
                upstream.out()->send(Event ('+'));
                upstream.out()->send(Event ('+'));
                command_mode_progress_ = 0;
                command_mode_timeout_ = 0;
                break; // Continue and send the latest event.
        }
    }

    if (data_mode_) {
        // Forward the events up stream.
        if (!up)
            return Result::OK__NOT_CONNECTED;
        return up->send(event);
    } else {
        // For now, we just echo the events.
        if (!down)
            return Result::OK__NOT_CONNECTED;
        if (event.is_data()) {
            uint8_t c = event.data();
            if (c == '\r') {
                // Some treminals send only a CR (PT100), so use this to launch the command.
                cmd_ready_ = true;
                cr_rcvd_ = true;
                // We always send a CRLF to the downstream pipe.
                down->send('\r');
                event = '\n';
            } else if (c == '\n') {
                // Some terminals send LF (Unix) and some send CRLF (Windows).
                if (cr_rcvd_) {
                    // CR already triggered the command, so ignore the LF.
                    cr_rcvd_ = false;
                    return Result::OK;
                } else {
                    // We have a LF without a CR. This is a command line.
                    cmd_ready_ = true;
                    // We always send a CRLF to the downstream pipe.
                    down->send('\r');
                    // Fall through to send the LF.
                }
            } else if (c == 127) { // Backspace (or 8)
                // Remove the last character from the command line.
                if (cmd_.length() > 0) {
                    Result res = down->send_text("\x1b[1D \x1b[1D"); // Move cursor left
                    cmd_.pop_back();
                    return res;
                }
            } else if (c == 27) { // Escape
                // Escape pressed. Clear the buffer and start a nuw line.
                cmd_.clear();
                down->send('\r');
                return down->send('\n');
            } else if ((c == '/') && (cmd_.length() == 1) && ((cmd_[0] == 'A') || (cmd_[0] == 'a'))) {
                // "A/" repeats the last command. It does not need CR/LF
                cmd_ = prev_cmd_;
                cmd_ready_ = true;
                down->send(event);
                down->send('\r');
                event = '\n';
            } else if (cmd_.length() < 255) {
                // Add the character to the command line.
                // We could check for printable characters here.
                cmd_.push_back(c);
            } else {
                // Command line too long. Send a bell.
                event = '\a'; // Bell
            }
        }
        auto ret = down->send(event);
        if (cmd_ready_) {
            cmd_ready_ = false;
            if ( ( (cmd_[0]=='A') || (cmd_[0]=='a') ) && ( ((cmd_[1]=='T') || cmd_[1]=='t') ) ) {
                run_cmd_line();
            }
            prev_cmd_ = cmd_;
            cmd_.clear();
        }
        return ret;
    }
}

void HayesFilter::send_string(const char *str) {
    Pipe *down = downstream.out();
    if (down) {
        while (*str) {
            down->send(Event(*str++));
        }
    }
}

void HayesFilter::send_OK() {
    send_string("OK\r\n"); 
}

void HayesFilter::send_CONNECT() { 
    send_string("CONNECT\r\n"); 
}

void HayesFilter::send_ERROR() { 
    send_string("ERROR\r\n"); 
}

/** 
 * \brief Interprete the command line and run the commands we find
 */
void HayesFilter::run_cmd_line() {
    const char *cmd = &cmd_[2];
    while (cmd) {
        cmd = run_next_cmd(cmd);
    }
}

// A single command line can hold multiple concatenated commands.
// - basic command set: A capital character followed by a digit. For example, M1.
// - extended command set: An "&" and a capital character followed by a digit. "&M1".
// - proprietary command set: Usually starting either with a backslash (“\”) or with a percent sign (“%”);
// - register commands – Sr=n or Sr?.
// ATD*99# : Dial Access Point
const char *HayesFilter::run_next_cmd(const char *cmd) {
    char c = *cmd++;
    uint32_t a = 0, v = 0;
    if (c>32 && c<127) c = toupper(c);
    switch (c) {
        case 0: // End of command line.
            send_OK();
            return nullptr;
        // A - Answer incoming call.
        // B - Select Communication Standard
        // C - Carrier Control Selection
        case 'D': // The remainder of the command line is a phone number.
            cmd = nullptr; // Skip the rest of the command line.
            send_ERROR();
            return nullptr;
        // E - Command State Character Echo Selection
        // F - On-line State Character Echo Selection
        // H - Hook Command Options
        case 'I':
            a = read_int(&cmd); // We can add various info strings here, depending of the command argument.
            if (!send_info(a)) {
                send_ERROR();
                return nullptr;
            }
            break;
        // L - Speaker Volume Level Selection
        // M - Speaker On/Off Selection
        // N - Negotiation of Handshake Options
        case 'O':
            switch_to_data_mode();
            return nullptr; // Nothing to be done after going online.
        // P - Pulse Dialing Selection
        // Q - Result Code Display Options
        case 'S':
            if (*cmd >= '0' && *cmd <= '9') {
                current_register_ = read_int(&cmd);
            }
            if (*cmd == '=') {
                cmd++;
                if (!set_register(current_register_,read_int(&cmd))) {
                    send_ERROR();
                    return nullptr;
                }
                return cmd;
            }
            if (*cmd == '?') {
                cmd++;
                char buf[16];
                itoa10(get_register(current_register_), buf);
                send_string(buf);
                send_string("\r\n");
                return cmd;
            }
        // T - Select Tone Dialing Method
        // V - Result Code Format Options
        // W - Negotiation Progress Message Selection
        // X - Call Progress Options
        // Y - Long Space Disconnect Options
        // Zn: Reset to profile n (see &Wn)
        case '&':
            return run_ampersand_cmd(cmd);
        case '[': 
            return run_sdcard_cmd(cmd);
        default:
            send_ERROR();
            return nullptr;
    }
    return cmd;
}

const char *HayesFilter::run_ampersand_cmd(const char *cmd) {
    char c = *cmd++;
    if (c>32 && c<127) c = toupper(c);
    switch (c) {
        // &D1: DTR signal (0=on, 1=off) etc.
        // &F: Recall Factory Profile
        // &K0: flow control (0=none, 1=hardware, 2=software)
        // &T0: self test (various tests)
        // &Yn: reset to profile
        // &V: show current profile
        // %E0: Escape method ("+++", break, DTR?, etc.)
        case 0:  // End of command line.
        case 'W': // write current settings to NVRAM
            read_int(&cmd);
            user_settings.write();
            break;
        default: // Unknown command AT&...
            send_ERROR();
            return nullptr; 
    }
    return cmd;
}

void HayesFilter::link(SDCardEndpoint *sdcard) {
    sdcard_ = sdcard;
}

const char *HayesFilter::run_sdcard_cmd(const char *cmd) {
    // if ((strncasecmp(cmd, "DS", 2) == 0) || (strncasecmp(cmd, "DI", 2) == 0)) { // disk status
    //     auto status = 0;
    //     if (strncasecmp(cmd, "DS", 2) == 0)
    //         status = sdcard_->disk_status();
    //     else
    //         status = sdcard_->disk_initialize();
    //     bool pad = false;
    //     if (status | 1) { send_string("NOINIT"); pad = true; }
    //     if (status | 2) { if (pad) send_string(" "); send_string("NODISK"); pad = true; }
    //     if (status | 4) { if (pad) send_string(" "); send_string("PROTECT"); pad = true; }
    //     if (pad) send_string("\r\n");
    //     return cmd + 2;
    // }
    if (strncasecmp(cmd, "GL", 2) == 0) { // Get Label
        Pipe *down = downstream.out();
        const std::u16string &label = sdcard_->get_label();
        send_string("\"");
        for (auto c: label) {
            if (c < 32 || c > 126) {
                down->send('.'); // Replace non-printable characters with a dot.
            } else {
                down->send(c);
            }
        }
        send_string("\"\r\n");
        uint32_t err = sdcard_->status();
        //if (err) {
            send_string(sdcard_->strerr(err));
            send_string("\r\n");
            send_ERROR();
            return nullptr;
        //}
        return cmd + 2;
    }
    if (strncasecmp(cmd, "SN", 2) == 0) { // Write a new serial number, hardware version, and revision
        cmd += 2;
        uint32_t serial = read_int(&cmd);
        if (*cmd != ':') { send_ERROR(); return nullptr; } else cmd++;
        uint16_t id = read_int(&cmd);
        if (*cmd != '.') { send_ERROR(); return nullptr; } else cmd++;
        uint16_t version = read_int(&cmd);
        if (*cmd != '.') { send_ERROR(); return nullptr; } else cmd++;
        uint16_t revision = read_int(&cmd);
        Result res = user_settings.write_serial(serial, id, version, revision);
        if (res.rejected()) {
            send_string("Rejected\r\n");
            send_ERROR();
            return nullptr;
        }
        char buf[64];
        snprintf(buf, 64, "Flashed %d %d %d %d\r\n", serial, id, version, revision);
        send_string(buf);
        return cmd; 
    }
    send_ERROR();
    return nullptr;
}

uint32_t HayesFilter::read_int(const char **cmd) {
    uint32_t value = 0;
    while (**cmd >= '0' && **cmd <= '9') {
        value = value * 10 + (**cmd - '0');
        (*cmd)++;
    }
    return value;
}

bool HayesFilter::set_register(uint32_t reg, uint32_t value) {
    switch (reg) {
        // S2: escape character ("+")
        // S3: carriage return (13)
        // S4: line feed (10)
        // S37: Desired DCE Line Speed (http://www.messagestick.net/modem/Hayes_Ch1-3.html)
        case 12: // S12: escape code guard time (1/50th of a second)
            if (index_ == 0) {
                user_settings.data.hayes0_esc_code_guard_time = value;
            } else {
                user_settings.data.hayes1_esc_code_guard_time = value;
            }
            break;
        case 300: // ATS300: absolute throttle delay in microseconds
            user_settings.data.mnpt_absolute_delay = value;
            break;
        case 301: // ATS301: relative MNP throttle delay in characters
            user_settings.data.mnpt_num_char_delay = value;
            break;
        default:
            return false;
    }
    scheduler().signal_all(Event {Event::Type::SIGNAL, Event::Subtype::USER_SETTINGS_CHANGED});
    return true;
}

uint32_t HayesFilter::get_register(uint32_t reg) const {
    switch (reg) {
        case 12: // S12: escape code guard time (1/50th of a second)
            if (index_ == 0) {
                return user_settings.data.hayes0_esc_code_guard_time;
            } else {
                return user_settings.data.hayes1_esc_code_guard_time;
            }
        case 13: // S13: escape code guard time (1/50th of a second)
            return user_settings.data.hayes0_esc_code_guard_time;
        case 300: // ATS300: absolute throttle delay in microseconds
            return user_settings.data.mnpt_absolute_delay;
        case 301: // ATS301: relative MNP throttle delay in characters
            return user_settings.data.mnpt_num_char_delay;
    }
    return 0;
}

bool HayesFilter::send_info(uint32_t ix) {
    char buf[32]; buf[0] = 0;
    switch (ix) {
        case 0:
            send_string("NewtDongle V0.0.4\r\n");
            break;
        case 1:
            itoa10(user_settings.serial(), buf);
            send_string("Serial No.: ");
            send_string(buf);
            send_string("\r\n");
            break;
        case 2:
            send_string("Hardware: V");
            itoa10(user_settings.hardware_version(), buf);
            send_string(buf);
            send_string(".");
            itoa10(user_settings.hardware_revision(), buf);
            send_string(buf);
            send_string("\r\n");
            break;
        default:
            return false;
    }
    return true;
}