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

    virtual uint32_t openfile(const std::u16string &name) = 0;
    virtual uint32_t filesize() = 0;
    virtual uint32_t readfile(uint8_t *buffer, uint32_t size) = 0;
    virtual uint32_t closefile() = 0;

    virtual uint32_t chdir(const std::u16string &path) = 0;
    virtual uint32_t getcwd(std::u16string &path) = 0; 
};

} // namespace nd

#endif // ND_ENDPOINTS_SDCARD_H