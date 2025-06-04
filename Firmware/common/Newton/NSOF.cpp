//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "NSOF.h"

#include "main.h"

#include <typeinfo>

using namespace nd;

const Symbol nd::symName { "name" };
const Symbol nd::symType { "type" };
const Symbol nd::symDiskType { "diskType" };
const Symbol nd::symWhichVol { "whichvol" };

void Ref::log(uint32_t depth, uint32_t indent) const
{
    if (depth == 0) return; // No logging if depth is zero
    switch (type_) {
        case Type::INT: // int32_t
            Log.indent(indent);
            Log.logf("%d", as_int());
            break;
        case Type::BOOL: // bool
            Log.indent(indent);
            if (as_bool()) {
                Log.log("TRUE");
            } else {
                Log.log("NIL");
            }
            break;
        case Type::CHAR16: // char16_t
            Log.indent(indent);
            Log.logf("%c", as_char16());
            break;
        case Type::REAL: // float
            Log.indent(indent);
            Log.logf("%f", as_real());
            break;
        case Type::OBJECT: // call virtual function Object::log()
            as_object()->log(depth, indent);
            break;
    }
    // If the Ref is a nullptr, we can log it as well
}   

void Ref::logln(uint32_t depth, uint32_t indent) const {
    log(depth, indent);
    Log.log("\n");
}

void Ref::logcomma(uint32_t depth, uint32_t indent) const {
    log(depth, indent);
    Log.log(", ");    
}

void Symbol::log(uint32_t depth, uint32_t indent) const {
    if (depth == 0) return; // No logging if depth is zero
    Log.indent(indent);
    Log.logf("'%s", sym_.c_str());
}

void String::log(uint32_t depth, uint32_t indent) const {
    if (depth == 0) return; // No logging if depth is zero
    Log.indent(indent);
    Log.log("\"");
    for (auto c: str_) {
        if (c == '"') {
            Log.log("\\\"");
        } else if (c == '\\') {
            Log.log("\\\\");
        } else if (c < 32 || c > 126) {
            Log.logf("\\x%04x", (uint16_t)c);
        } else {
            Log.logf("%c", c);
        }
    }
    Log.log("\"");
}

void Array::log(uint32_t depth, uint32_t indent) const {
    if (depth == 0) return; // No logging if depth is zero
    Log.indent(indent);
    if (depth == 1) {
        Log.log("[...]");
        return; // Avoid logging contents if depth is 1
    } else {
        Log.log("[\n");
        for (auto &ref: elements_) {
            ref.logcomma(depth - 1, indent + 1);
            Log.log("\n");
        }
        Log.indent(indent);
        Log.log("]");
    }
}

void Frame::log(uint32_t depth, uint32_t indent) const {
    if (depth == 0) return; // No logging if depth is zero
    Log.indent(indent);
    if (depth == 1) {
        Log.log("{...}");
        return; // Avoid logging contents if depth is 1
    } else {
        Log.log("{\n");
        for (auto &item: frame_) {
            item.first->log(depth - 1, indent + 1);
            Log.log(": ");
            item.second.log(depth - 1);
            Log.log("\n");
        }
        Log.indent(indent);
        Log.log("}");
    }
}

// -----------------------------------------------------------------------------


void push_xlong(std::vector<uint8_t> &vec, int32_t value) {
    if ((value >= 0) && (value < 255)) {
        vec.push_back((uint8_t)value); // 1-byte integer
    } else {
        vec.push_back(0xFF);
        vec.push_back((value >> 24) & 0xFF);
        vec.push_back((value >> 16) & 0xFF);
        vec.push_back((value >> 8) & 0xFF);
        vec.push_back(value & 0xFF);
    }
}

void Ref::to_nsof(NSOF &nsof) const
{
    int32_t v;
    switch (type_) {
        case Type::INT: // int
            nsof.data().push_back(0);
            push_xlong(nsof.data(), as_int() << 2);
            break;
        case Type::BOOL: // bool
            nsof.data().push_back(0);
            if (as_bool()) {
                nsof.data().push_back(0x1A);
            } else {
                nsof.data().push_back(0x02);
            }
            break;
        case Type::CHAR16: // char16_t
            nsof.data().push_back(0);
            push_xlong(nsof.data(), (as_char16() << 4) | 6);
            break;
        case Type::REAL: // float
            // TODO: later
            nsof.data().push_back(0);
            nsof.data().push_back(0x02);
            break;
        case Type::OBJECT: // SymbolRef
            as_object()->to_nsof(nsof);
            break;
    }
}

void Symbol::to_nsof(NSOF &nsof) const {
    nsof.data().push_back(7);
    push_xlong(nsof.data(), sym_.size());
    for (auto &c: sym_) {
        nsof.data().push_back(c);
    }
}

void String::to_nsof(NSOF &nsof) const {
    nsof.data().push_back(8);
    push_xlong(nsof.data(), str_.size()*2 + 2);
    for (auto &c: str_) {
        nsof.data().push_back(c >> 8);
        nsof.data().push_back(c & 0xFF);
    }
    nsof.data().push_back(0);
    nsof.data().push_back(0);
}

void Array::to_nsof(NSOF &nsof) const {
    nsof.data().push_back(5);
    push_xlong(nsof.data(), elements_.size());
    for (auto &ref: elements_) {
        ref.to_nsof(nsof);
    }
}

void Frame::to_nsof(NSOF &nsof) const {
    nsof.data().push_back(6);
    push_xlong(nsof.data(), frame_.size());
    for (auto &v: frame_) {
        v.first->to_nsof(nsof);
    }
    for (auto &v: frame_) {
        v.second.to_nsof(nsof);
    }
}

void NSOF::log() {
    Log.logf("NSOF: %d bytes\n", size());
    for (auto byte : data_) {
        uint8_t c = (byte>=20 && byte<127) ? byte : '.';
        Log.logf("%02x:%c ", byte, c);
    }
    Log.log("\n");
}
