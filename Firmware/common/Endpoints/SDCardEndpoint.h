//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_ENDPOINTS_SDCARD_H
#define ND_ENDPOINTS_SDCARD_H

#include "common/Endpoint.h" // Adjusted the path to ensure the Endpoint header is correctly included

#include <string>

namespace nd {

class SDCardEndpoint : public Endpoint {
public:
    SDCardEndpoint(Scheduler &scheduler);
    ~SDCardEndpoint();

    Result init() override;
    Result send(Event) override;

    virtual const char *strerr(uint32_t err) = 0;
    virtual uint32_t status() = 0;
    virtual const std::u16string &get_label() = 0;

    virtual uint32_t opendir() = 0;
    virtual uint32_t readdir(std::u16string &name) = 0;
    virtual uint32_t closedir() = 0;

    virtual uint32_t chdir(std::u16string &path) = 0;

    // Set file system to UTF16
    // Allow better directory handling
    // Check all other predefined compile settings 
    // Check if there is an SD Card in the slot

    // File Access  https://elm-chan.org/fsw/ff/00index_e.html
    // f_open - Open/Create a file
    // f_close - Close an open file
    // f_read - Read data from the file
    // (f_lseek - Move read/write pointer, Expand size)
    // (f_tell - Get current read/write pointer)
    // (f_eof - Test for end-of-file)
    // f_size - Get size
    // f_error - Test for an error
    // Directory Access
    // f_opendir - Open a directory
    // f_closedir - Close an open directory
    // f_readdir - Read a directory item
    // (f_findfirst - Open a directory and read the first item matched)
    // (f_findnext - Read a next item matched)
    // File and Directory Management
    // (f_stat - Check existance of a file or sub-directory)
    // f_chdir - Change current directory
    // (f_getcwd - Retrieve the current directory and drive)
    // Volume Management and System Configuration
    // f_mount - Register/Unregister the work area of the volume
    // f_getlabel - Get volume label
};

} // namespace nd

#endif // ND_ENDPOINTS_SDCARD_H