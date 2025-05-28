//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "NSOF.h"

#include "main.h"

#include <typeinfo>

using namespace nd;

void nsof_test() {
    Ref ref1 { int(1024) };
    Ref ref2 = true;
    Ref ref3 = nullptr;
    Ref ref4 = u'X';
    Ref ref5 = 3.14;
    Ref ref6 = std::make_shared<Symbol>("example_symbol");
    Ref ref7 = std::make_shared<String>(u"example_string");
    Ref ref8 = std::make_shared<Array>(Array{ref1, ref2, ref3});
    Ref ref9 = "test";
    Ref ref10 = u"Toast";

    Ref ref11 = std::make_shared<Frame>(
        std::unordered_map<SymbolRef, Ref>{
            {std::make_shared<Symbol>("key1"), Ref(42)},
            {std::make_shared<Symbol>("key2"), Ref(true)}
        }
    );

    SymbolRef symType = std::make_shared<Symbol>("type");
    SymbolRef symName = std::make_shared<Symbol>("name");
    FrameRef frame = std::make_shared<Frame>(
        std::unordered_map<SymbolRef, Ref>{
            {symType, std::make_shared<String>(u"ex1")},
            {symName, symType} //std::make_shared<String>(u"Peter")}
        }
    );

    ref1.logln();
    ref2.logln();
    ref3.logln();
    ref4.logln();
    ref5.logln();
    ref6.logln();
    ref7.logln();
    ref8.logln();
    ref9.logln();
    ref10.logln();
    ref11.logln();

    NSOF s;
    s.clear(); s.to_nsof(ref1); s.log();
    s.clear(); s.to_nsof(ref2); s.log();
    s.clear(); s.to_nsof(ref3); s.log();
    s.clear(); s.to_nsof(ref4); s.log();
    s.clear(); s.to_nsof(ref5); s.log();
    s.clear(); s.to_nsof(ref6); s.log();
    s.clear(); s.to_nsof(ref7); s.log();
    s.clear(); s.to_nsof(ref8); s.log();
    s.clear(); s.to_nsof(ref9); s.log();
    s.clear(); s.to_nsof(ref10); s.log();
    s.clear(); s.to_nsof(ref11); s.log();
    s.clear(); s.to_nsof(frame); s.log();
}

void Ref::log(uint32_t depth, uint32_t indent)
{
    if (depth == 0) return; // No logging if depth is zero
    switch (this->index()) {
        case 0: // int
            Log.indent(indent);
            Log.logf("%d", std::get<int>(*this));
            break;
        case 1: // bool
            Log.indent(indent);
            if (std::get<bool>(*this)) {
                Log.log("TRUE");
            } else {
                Log.log("NIL");
            }
            break;
        case 2: // char16_t
            Log.indent(indent);
            Log.logf("%c", std::get<char16_t>(*this));
            break;
        case 3: // double
            Log.indent(indent);
            Log.logf("%f", std::get<double>(*this));
            break;
        case 4: // SymbolRef
            std::get<SymbolRef>(*this)->log(depth, indent);
            break;
        case 5: // StringRef
            std::get<StringRef>(*this)->log(depth, indent);
            break;
        case 6: // ArrayRef
            std::get<ArrayRef>(*this)->log(depth, indent);
            break;
        case 7: // FrameRef
            std::get<FrameRef>(*this)->log(depth, indent);
            break;
        default:
            Log.log("Unknown type\n");
    }
    // If the Ref is a nullptr, we can log it as well
}   

void Ref::logln(uint32_t depth, uint32_t indent) {
    log(depth, indent);
    Log.log("\n");
}

void Ref::logcomma(uint32_t depth, uint32_t indent) {
    log(depth, indent);
    Log.log(", ");    
}

void Symbol::log(uint32_t depth, uint32_t indent) {
    if (depth == 0) return; // No logging if depth is zero
    Log.indent(indent);
    Log.logf("'%s", this->c_str());
}

void String::log(uint32_t depth, uint32_t indent) {
    if (depth == 0) return; // No logging if depth is zero
    Log.indent(indent);
    Log.log("\"");
    for (auto c: *this) {
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

void Array::log(uint32_t depth, uint32_t indent) {
    if (depth == 0) return; // No logging if depth is zero
    Log.indent(indent);
    if (depth == 1) {
        Log.log("[...]");
        return; // Avoid logging contents if depth is 1
    } else {
        Log.log("[\n");
        for (auto &ref: *this) {
            ref.logcomma(depth - 1, indent + 1);
            Log.log("\n");
        }
        Log.indent(indent);
        Log.log("]");
    }
}

void Frame::log(uint32_t depth, uint32_t indent) {
    if (depth == 0) return; // No logging if depth is zero
    Log.indent(indent);
    if (depth == 1) {
        Log.log("{...}");
        return; // Avoid logging contents if depth is 1
    } else {
        Log.log("{\n");
        for (auto &[key, value]: *this) {
            key->log(depth - 1, indent + 1);
            Log.log(": ");
            value.log(depth - 1);
            Log.log("\n");
        }
        Log.indent(indent);
        Log.log("}");
    }
}

// -----------------------------------------------------------------------------

void Ref::to_nsof(std::vector<uint8_t> &vec) {
    // Reset all the precedence values
    int32_t reset_precedent = -1;
    to_nsof(nullptr, reset_precedent);
    // Now convert the Ref to NSOF
    vec.push_back(0x02); // Start of NSOF
    int32_t current_precedent = 0;
    to_nsof(&vec, current_precedent);
}

void push_xlong(std::vector<uint8_t> *vec, int32_t value) {
    if ((value >= 0) && (value < 255)) {
        vec->push_back((uint8_t)value); // 1-byte integer
    } else {
        vec->push_back(0xFF);
        vec->push_back((value >> 24) & 0xFF);
        vec->push_back((value >> 16) & 0xFF);
        vec->push_back((value >> 8) & 0xFF);
        vec->push_back(value & 0xFF);
    }
}

void Ref::to_nsof(std::vector<uint8_t> *vec, int32_t &precedent)
{
    if (vec == nullptr) {
        switch (this->index()) {
            case 4: // SymbolRef
                std::get<SymbolRef>(*this)->to_nsof(nullptr, precedent);
                break;
            case 5: // StringRef
                std::get<StringRef>(*this)->to_nsof(nullptr, precedent);
                break;
            case 6: // ArrayRef
                std::get<ArrayRef>(*this)->to_nsof(nullptr, precedent);
                break;
            case 7: // FrameRef
                std::get<FrameRef>(*this)->to_nsof(nullptr, precedent);
                break;
        }
        return;
    } else {
        int32_t v;
        switch (this->index()) {
            case 0: // int
                vec->push_back(0);
                push_xlong(vec, std::get<int>(*this) << 2);
                break;
            case 1: // bool
                vec->push_back(0);
                if (std::get<bool>(*this)) {
                    vec->push_back(0x1A);
                } else {
                    vec->push_back(0x02);
                }
                break;
            case 2: // char16_t
                vec->push_back(0);
                push_xlong(vec, (std::get<char16_t>(*this) << 4) | 6);
                break;
            case 3: // double
                // TODO: later
                vec->push_back(0);
                vec->push_back(0x02);
                break;
            case 4: // SymbolRef
                std::get<SymbolRef>(*this)->to_nsof(vec, precedent);
                break;
            case 5: // StringRef
                std::get<StringRef>(*this)->to_nsof(vec, precedent);
                break;
            case 6: // ArrayRef
                std::get<ArrayRef>(*this)->to_nsof(vec, precedent);
                break;
            case 7: // FrameRef
                std::get<FrameRef>(*this)->to_nsof(vec, precedent);
                break;
        }
    }
}   

void Symbol::to_nsof(std::vector<uint8_t> *vec, int32_t &precedent) {
    if (vec == nullptr) {
        precedent_ = precedent;
    } else if (precedent_ >= 0) {
        vec->push_back(9);
        push_xlong(vec, precedent_);
    } else {
        precedent_ = precedent++;
        vec->push_back(7);
        push_xlong(vec, size());
        for (int i=0; i<size(); ++i) {
            vec->push_back(operator[](i));
        }
    }
}

void String::to_nsof(std::vector<uint8_t> *vec, int32_t &precedent) {
    if (vec == nullptr) {
        precedent_ = precedent;
    } else if (precedent_ >= 0) {
        vec->push_back(9);
        push_xlong(vec, precedent_);
    } else {
        precedent_ = precedent++;
        vec->push_back(8);
        push_xlong(vec, size()*2 + 2);
        for (int i=0; i<size(); ++i) {
            int16_t c = operator[](i);
            vec->push_back(c >> 8);
            vec->push_back(c & 0xFF);
        }
        vec->push_back(0);
        vec->push_back(0);
    }
}

void Array::to_nsof(std::vector<uint8_t> *vec, int32_t &precedent) {
    if (vec == nullptr) {
        precedent_ = precedent;
        for (auto &ref: *this) {
            ref.to_nsof(vec, precedent);
        }
    } else if (precedent_ >= 0) {
        vec->push_back(9);
        push_xlong(vec, precedent_);
    } else {
        precedent_ = precedent++;
        vec->push_back(5);
        push_xlong(vec, size());
        for (auto &ref: *this) {
            ref.to_nsof(vec, precedent);
        }
    }
}

void Frame::to_nsof(std::vector<uint8_t> *vec, int32_t &precedent) {
    if (vec == nullptr) {
        precedent_ = precedent;
        for (auto &v: *this) {
            v.first->to_nsof(vec, precedent);
            v.second.to_nsof(vec, precedent);}
    } else if (precedent_ >= 0) {
        vec->push_back(9);
        push_xlong(vec, precedent_);
    } else {
        precedent_ = precedent++;
        vec->push_back(6);
        push_xlong(vec, size());
        for (auto &v: *this) {
            v.first->to_nsof(vec, precedent); // SymbolRef
        }
        for (auto &v: *this) {
            v.second.to_nsof(vec, precedent); // Ref
        }
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