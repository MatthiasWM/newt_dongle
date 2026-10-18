//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

// Helpers for building MNP frames in the host tests.

#ifndef ND_TESTS_MNP_UTIL_H
#define ND_TESTS_MNP_UTIL_H

#include <cstdint>
#include <vector>

typedef std::vector<uint8_t> Bytes;

// The payload of an LT frame as the Dock should see it: no header, no
// trailer, and escaped DLEs reduced to a single DLE.
inline Bytes lt_payload(const Bytes &frame) {
    Bytes payload;
    for (size_t i = 6; i < frame.size() - 4; i++) {
        payload.push_back(frame[i]);
        if (frame[i] == 0x10) i++;
    }
    return payload;
}

// Build a frame that is not in the capture. This bitwise CRC is independent
// of the table in MNPFilter, and gives the same CRCs as the captured frames.
inline Bytes make_frame(const Bytes &header, const Bytes &data) {
    Bytes content = { (uint8_t)header.size() };
    content.insert(content.end(), header.begin(), header.end());
    content.insert(content.end(), data.begin(), data.end());
    Bytes frame = { 0x16, 0x10, 0x02 };
    uint16_t crc = 0;
    content.push_back(0x03); // ETX is part of the CRC, but not of the escaped content
    for (size_t i = 0; i < content.size(); i++) {
        uint8_t c = content[i];
        crc ^= c;
        for (int bit = 0; bit < 8; bit++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
        if (i == content.size() - 1) break;
        frame.push_back(c);
        if (c == 0x10) frame.push_back(0x10);
    }
    frame.insert(frame.end(), { 0x10, 0x03, (uint8_t)(crc & 0xFF), (uint8_t)(crc >> 8) });
    return frame;
}

#endif // ND_TESTS_MNP_UTIL_H
