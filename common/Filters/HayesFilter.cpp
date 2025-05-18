//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "HayesFilter.h"

#include "common/Scheduler.h"
#include "common/Endpoints/SDCardEndpoint.h"

#include <stdio.h>
#include <cstring>

using namespace nd;

constexpr uint32_t kCommandModeTimeout = 1'000'000;
constexpr uint32_t kCommandModePause = 1'000'000;

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
HayesFilter::HayesFilter(Scheduler &scheduler)
:   Task(scheduler),
    upstream(*this),
    downstream(*this)
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
                if (command_mode_timeout_ > kCommandModePause) {
                    command_mode_progress_ = 1;
                    command_mode_timeout_ = 0;
                    //printf("**** HayesFilter task: 0 -> 1 (Good: no character for a while)\n");
                }
                break;
            case 1: // waiting for the first '+'
                break;
            case 2: // waiting for the second '+'
                if (command_mode_timeout_ > kCommandModeTimeout) {
                    //printf("**** HayesFilter task: %d -> 0 (Bad: no + in time)\n", command_mode_progress_);
                    command_mode_timeout_ = 0;
                    command_mode_progress_ = 0;
                    upstream.out()->send(Event('+')); // Make up for the '+' we withheld.
                }
                break;
            case 3: // waiting for the third '+'
                if (command_mode_timeout_ > kCommandModeTimeout) {
                    //printf("**** HayesFilter task: %d -> 0 (Bad: no + in time)\n", command_mode_progress_);
                    command_mode_timeout_ = 0;
                    command_mode_progress_ = 0;
                    upstream.out()->send(Event('+')); // Make up for the two '+'s we withheld.
                    upstream.out()->send(Event('+'));
                }
                break;
            case 4: // waiting for the last pause
                if (command_mode_timeout_ > kCommandModePause) {
                    command_mode_progress_ = 0;
                    //printf("**** HayesFilter task: COMMAND MODE (Good: no character for a while)\n");
                    switch_to_command_mode();
                    return Result::OK;        
                }
                break;
        }
    } else {
        // In command mode, we need to check for the "ATO" command.
        if (cmd_ready_) {
            cmd_ready_ = false;
            if ( ( (cmd_[0]=='A') || (cmd_[0]=='a') ) && ( ((cmd_[1]=='T') || cmd_[1]=='t') ) ) {
                run_cmd_line();
            }
            cmd_.clear();
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
                    //printf("**** HayesFilter send: 1 -> 2 (Good: received first +)\n");
                    return Result::OK; // Don't send the '+' just yet.
                } else {
                    //printf("**** HayesFilter send: 1 -> 0 (Bad: received unexpected character)\n");
                    command_mode_progress_ = 0;
                    command_mode_timeout_ = 0;
                }
                break; // Continue and send the latest event.
            case 2: // waiting for the second '+'
                if (event.data() == '+') {
                    command_mode_progress_ = 3;
                    command_mode_timeout_ = 0;
                    //printf("**** HayesFilter send: 2 -> 3 (Good: received second +)\n");
                    return Result::OK; // Don't send the '+' just yet.
                } else {
                    upstream.out()->send(Event ('+')); // Make up for the '+' we withheld.
                    //printf("**** HayesFilter send: 2 -> 0 (Bad: received unexpected character)\n");
                    command_mode_progress_ = 0;
                    command_mode_timeout_ = 0;
                }
                break; // Continue and send the latest event.
            case 3: // waiting for the third '+'
                if (event.data() == '+') {
                    //printf("**** HayesFilter send: 3 -> 4 (Good: received third +)\n");
                    command_mode_progress_ = 4;
                    command_mode_timeout_ = 0;
                    return Result::OK; // Don't send the '+' just yet.
                } else {
                    upstream.out()->send(Event ('+')); // Make up for the two '+'s we withheld.
                    upstream.out()->send(Event ('+'));
                    //printf("**** HayesFilter send: 3 -> 0 (Bad: received unexpected character)\n");
                    command_mode_progress_ = 0;
                    command_mode_timeout_ = 0;
                }
                break; // Continue and send the latest event. 
            case 4: // waiting for the last pause
                //printf("**** HayesFilter send: 4 -> 0 (Bad: received unexpected character)\n");
                upstream.out()->send(Event ('+')); // Make up for the three '+'s we withheld.
                upstream.out()->send(Event ('+'));
                upstream.out()->send(Event ('+'));
                command_mode_progress_ = 0;
                command_mode_timeout_ = 0;
                break; // Continue and send the latest event.
        }
    }

    // If the downstream pipe is not connected, we can't do anything.
    // Simplified mode change:
    if (event.type() == Event::Type::DATA) {
        uint8_t c = event.data();
        if ((data_mode_ == false) && (c == '-')) {
            switch_to_data_mode();
            return Result::OK;
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
        if (event.is_data() && (cmd_.length() < 255)) {
            uint8_t c = event.data();
            if (c == '\r') {
            } else if (c == '\n') {
                cmd_ready_ = true;
            } else {
                cmd_.push_back(c);
            }
            // TODO: if we want to allow "A/", we need to check for that here.
        }
        return down->send(event);   
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
    if (c>32 && c<127) c = toupper(c);
    switch (c) {
        case 0: // End of command line.
            send_OK();
            return nullptr;
        case 'D': // The remainder of the command line is a phone number.
            cmd = nullptr; // Skip the rest of the command line.
            send_ERROR();
            return nullptr;
        case 'I':
            // TODO: read_int() and output the corresponding string
            send_string("NewtDongle V0.0.2\r\n");
            break;
        case 'O':
            switch_to_data_mode();
            return nullptr; // Nothing to be done after going online.
        //case 'S':
        // ATS : set current register, ATS? return value fo current register
        // ATS3=5 : set register 3 to value 5, ATS3? return value of register 3
        // ATS=3 : set current register to value 3
        // S2: escape character ("+")
        // S3: carriage return (13)
        // S4: line feed (10)
        // S12: escape code guard time (1/50th of a second)
        // Z: Reset
        // &D1: DTR signal (0=on, 1=off) etc.
        // &F0: factory reset
        // &K0: flow control (0=none, 1=hardware, 2=software)
        // &T0: self test
        // &W0: write to NVRAM
        // &Yn: reset to profile
        // &V: show current profile
        // %E0: Escape method ("+++", break, DTR?, etc.)
        case '[': 
            return run_sdcard_cmd(cmd);
        default:
            send_ERROR();
            return nullptr;
    }
    send_OK();
    return cmd;
}


void HayesFilter::link(SDCardEndpoint *sdcard) {
    sdcard_ = sdcard;
}

const char *HayesFilter::run_sdcard_cmd(const char *cmd) {
    if ((strncasecmp(cmd, "DS", 2) == 0) || (strncasecmp(cmd, "DI", 2) == 0)) { // disk status
        auto status = 0;
        if (strncasecmp(cmd, "DS", 2) == 0)
            status = sdcard_->disk_status();
        else
            status = sdcard_->disk_initialize();
        bool pad = false;
        if (status | 1) { send_string("NOINIT"); pad = true; }
        if (status | 2) { if (pad) send_string(" "); send_string("NODISK"); pad = true; }
        if (status | 4) { if (pad) send_string(" "); send_string("PROTECT"); pad = true; }
        if (pad) send_string("\r\n");
        return cmd + 2;
    }
    if (strncasecmp(cmd, "GL", 2) == 0) { // Get Label
        char label[36]; label[0] = 0;
        auto err = sdcard_->get_label(label);
        if (err) {
            send_string(sdcard_->strerr(err));
            send_string("\r\n");
            send_ERROR();
            return nullptr;
        }
        send_string("\"");
        send_string(label);
        send_string("\"\r\n");
        return cmd + 2;
    }
    send_ERROR();
    return nullptr;
}
