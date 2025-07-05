//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PosixSDCardEndpoint.h"

#include "main.h"
#include "common/Pipe.h"
#include "common/Scheduler.h"

#include <cstdint>
#include <stdio.h>

using namespace nd;

void PosixSDCardEndpoint::early_init()
{
}

PosixSDCardEndpoint::PosixSDCardEndpoint(Scheduler &scheduler)
:   SDCardEndpoint(scheduler)
{
}

PosixSDCardEndpoint::~PosixSDCardEndpoint() {
}

Result PosixSDCardEndpoint::init()
{
    SDCardEndpoint::init();
    return Result::OK;
}

Result PosixSDCardEndpoint::task() {
    return SDCardEndpoint::task();
}

Result PosixSDCardEndpoint::send(Event event) {
    if (kLogSDCard) Log.log(event, 1);
    return SDCardEndpoint::send(event);
}


const char *PosixSDCardEndpoint::strerr(uint32_t err) {
    switch (err) {
        case FR_OK: return "OK";
        // case FR_DISK_ERR: return "DISK_ERR";
        // case FR_INT_ERR: return "INT_ERR";
        // case FR_NOT_READY: return "NOT_READY";
        // case FR_NO_FILE: return "NO_FILE";
        // case FR_NO_PATH: return "NO_PATH";
        // case FR_INVALID_NAME: return "INVALID_NAME";
        // case FR_DENIED: return "DENIED";
        // case FR_EXIST: return "EXIST";
        // case FR_INVALID_OBJECT: return "INVALID_OBJECT";
        // case FR_WRITE_PROTECTED: return "WRITE_PROTECTED";
        // case FR_INVALID_DRIVE: return "INVALID_DRIVE";
        // case FR_NOT_ENABLED: return "NOT_ENABLED";
        // case FR_NO_FILESYSTEM: return "NO_FILESYSTEM";
        // case FR_MKFS_ABORTED: return "MKFS_ABORTED";
        // case FR_TIMEOUT: return "TIMEOUT";
        // case FR_LOCKED: return "LOCKED";
        // case FR_NOT_ENOUGH_CORE: return "NOT_ENOUGH_CORE";
        // case FR_TOO_MANY_OPEN_FILES: return "TOO_MANY_OPEN_FILES";
        case FR_INVALID_PARAMETER: return "INVALID_PARAMETER";
        case FR_IS_DIRECTORY: return "IS_DIRECTORY";
        case FR_IS_PACKAGE: return "IS_PACKAGE"; // Custom error for package files
    }
    return "UNKNOWN";
}

const std::u16string &PosixSDCardEndpoint::get_label() {
    return label_;
}

uint32_t PosixSDCardEndpoint::opendir()
{
    return FR_OK;
}

uint32_t PosixSDCardEndpoint::readdir(std::u16string &name)
{
    return FR_OK;
}

uint32_t PosixSDCardEndpoint::closedir() {
    return FR_OK;
}

uint32_t PosixSDCardEndpoint::chdir(const std::u16string &path)
{
    return FR_OK;
}

uint32_t PosixSDCardEndpoint::getcwd(std::u16string &path)
{
    return FR_OK;
}

uint32_t PosixSDCardEndpoint::openfile(const std::u16string &name)
{
    return FR_OK;
}

uint32_t PosixSDCardEndpoint::filesize()
{
    return 0;
}

uint32_t PosixSDCardEndpoint::readfile(uint8_t *buffer, uint32_t size)
{
    return 0;
}

uint32_t PosixSDCardEndpoint::closefile()
{
    return FR_OK;
}
