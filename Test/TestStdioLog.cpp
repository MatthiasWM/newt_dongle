//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "StdioLog.h"

#include <sys/select.h>

using namespace nd;

/**
 * \return true if the next call to write a byte would block.
 */
bool TestStdioLog::would_block() {
    fd_set write_fds;
    struct timeval timeout { .tv_sec = 0, .tv_usec = 0 };
    FD_ZERO(&write_fds);
    FD_SET(fileno(stdout), &write_fds);
    int result = select(fileno(stdout) + 1, NULL, &write_fds, NULL, &timeout);
    if (result > 0 && FD_ISSET(fileno(stdout), &write_fds)) {
        return false; // writable
    } else if (result == 0) {
        return true; // would block
    } else {
        return false; // error, assume we can write
    }
}
