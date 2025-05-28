//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#ifndef ND_NEWTON_NSOF_H
#define ND_NEWTON_NSOF_H

#include <variant>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <memory>


namespace nd {

/*

  

 */




class Ref;
class Symbol;
using SymbolRef = std::shared_ptr<Symbol>;
class String;
using StringRef = std::shared_ptr<String>;
class Array;
using ArrayRef = std::shared_ptr<Array>;
class Frame;
using FrameRef = std::shared_ptr<Frame>;

using RefBaseType = std::variant<
    int,                // Integer value
    bool,               // Boolean value
    char16_t,           // Unicode character
    double,             // Floating point number
    SymbolRef,          // Symbol reference
    StringRef,          // String reference
    ArrayRef,           // Array reference
    FrameRef            // Frame reference
>;

class Ref : public RefBaseType
{
public:
    Ref() = default;
    Ref(int value) : RefBaseType(value) {}
    Ref(bool value) : RefBaseType(value) {}
    Ref(char16_t value) : RefBaseType(value) {}
    Ref(double value) : RefBaseType(value) {}
    Ref(SymbolRef value) : RefBaseType(value) {}
    Ref(StringRef value) : RefBaseType(value) {}
    Ref(ArrayRef value) : RefBaseType(value) {}
    Ref(FrameRef value) : RefBaseType(value) {}
    Ref(nullptr_t value) : RefBaseType(false) {}
    Ref(const char *value) : RefBaseType(std::make_shared<Symbol>(value)) {}
    Ref(const char16_t *value) : RefBaseType(std::make_shared<String>(value)) {}
    void log(uint32_t depth=999, uint32_t indent=0);
    void logln(uint32_t depth=999, uint32_t indent=0);
    void logcomma(uint32_t depth=999, uint32_t indent=0);
    void to_nsof(std::vector<uint8_t> &vec);
    void to_nsof(std::vector<uint8_t> *vec, int32_t &precedent);
};

class Symbol : public std::string {
    int32_t precedent_ = 0;
public:
    Symbol(const char *name) : std::string(name) {}
    void log(uint32_t depth=999, uint32_t indent=0);
    void to_nsof(std::vector<uint8_t> *vec, int32_t &precedent);
};

class String : public std::u16string {
    int32_t precedent_ = 0;
public:
    String(const char16_t *name) : std::u16string(name) {}
    void log(uint32_t depth=999, uint32_t indent=0);
    void to_nsof(std::vector<uint8_t> *vec, int32_t &precedent);
};

class Array : public std::vector<Ref> {
    int32_t precedent_ = 0;
public:
    Array() = default;
    Array(std::initializer_list<Ref> init) : std::vector<Ref>(init) {}
    void log(uint32_t depth=999, uint32_t indent=0);
    void to_nsof(std::vector<uint8_t> *vec, int32_t &precedent);
};

class Frame : public std::unordered_map<SymbolRef, Ref> {
    int32_t precedent_ = 0;
public:
    Frame() = default;
    Frame(const std::unordered_map<SymbolRef, Ref> &other) : std::unordered_map<SymbolRef, Ref>(other) {}
    void log(uint32_t depth=999, uint32_t indent=0);
    void to_nsof(std::vector<uint8_t> *vec, int32_t &precedent);
};

class NSOF {
    std::vector<uint8_t> data_;
public:
    NSOF() = default;
    NSOF(const std::vector<uint8_t> &data) : data_(data) {}
    bool append(uint8_t byte); // return true when the stream reached its end
    void assign(const std::vector<uint8_t> &vec) { data_ = vec; }
    void clear() { data_.clear(); }
    int size() const { return data_.size(); }
    Ref to_ref() { return Ref(false); }
    std::vector<uint8_t> &to_nsof(Ref ref) { ref.to_nsof(data_); return data_; }
    std::vector<uint8_t> &data() { return data_; }
    void log();
};

} // namespace nd

#endif // ND_NEWTON_NSOF_H