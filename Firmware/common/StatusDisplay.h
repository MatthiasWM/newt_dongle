//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_STATUS_DISPLAY_H
#define ND_STATUS_DISPLAY_H

#include "Task.h"

namespace nd {

class Task;

enum class AppStatus : uint8_t {
    IDLE = 0,            // Everything is fine
    ERROR,               // An error occurred
    USB_READY,           // USB terminal is ready
    USB_CONNECTED,       // USB terminal is connected
    DOCK_CONNECTED,      // Dock is connected
    SDCARD_ACTIVE,       // SD card is active
};

/** \brief Call the tasks of all registered Endpoints in an endless loop. */
class StatusDisplay : public Task 
{
public:
    StatusDisplay(Scheduler &scheduler);
    ~StatusDisplay() override;
    Result init() override;
    Result task() override;

    virtual void set(AppStatus new_status);
    virtual void repeat(AppStatus temp_status, int repeats);
};

} // namespace nd

#endif // ND_STATUS_DISPLAY_H