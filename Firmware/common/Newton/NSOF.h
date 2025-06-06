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

class Ref;
class Object;
class Symbol;
class String;
class Array;
class Frame;
class NSOF;

using real = float;

class Ref
{
public:
    enum class Type {
        INT, BOOL, CHAR16, REAL, OBJECT
    };
private:    
    union {
        int32_t     int_ = 0;
        bool        bool_;
        char16_t    char_;
        real        real_;
        Object      *object_;
    };
    Type type_ = Type::INT;
public:
    Ref() : bool_(false), type_(Type::BOOL) {}
    Ref(int32_t i) : int_(i), type_(Type::INT) {}
    Ref(bool b) : bool_(b), type_(Type::BOOL) {}
    Ref(char16_t c) : char_(c), type_(Type::CHAR16) {}
    Ref(real d) : real_(d), type_(Type::REAL) {}
    Ref(Object *obj) : object_(obj), type_(Type::OBJECT) {}
    Ref(Object &obj) : object_(&obj), type_(Type::OBJECT) {}
    Ref(const Ref &other) = default;

    Type type() const { return type_; }
    bool is_int() const { return type_ == Type::INT; }
    bool is_bool() const { return type_ == Type::BOOL; }
    bool is_char16() const { return type_ == Type::CHAR16; }
    bool is_real() const { return type_ == Type::REAL; }
    bool is_object() const { return type_ == Type::OBJECT; }
    int32_t as_int() const { return int_; }
    bool as_bool() const { return bool_; }
    char16_t as_char16() const { return char_; }
    real as_real() const { return real_; }
    Object *as_object() const { return object_; }

    void log(uint32_t depth=999, uint32_t indent=0) const;
    void logln(uint32_t depth=999, uint32_t indent=0) const;
    void logcomma(uint32_t depth=999, uint32_t indent=0) const;
    void to_nsof(NSOF &nsof) const;
};

class Object {
public:    
    enum class Type {
        UNKNOWN, SYMBOL, STRING, ARRAY, FRAME
    };
protected:
    Type type_ = Type::UNKNOWN;
public:
    Object() = default;
    virtual ~Object() = default;
    virtual void log(uint32_t depth=999, uint32_t indent=0) const = 0;
    virtual void to_nsof(NSOF &nsof) const = 0;
    Type type() const { return type_; }
    bool is_symbol() const { return type_ == Type::SYMBOL; }
    bool is_string() const { return type_ == Type::STRING; }
    bool is_array() const { return type_ == Type::ARRAY; }
    bool is_frame() const { return type_ == Type::FRAME; }
};

class Symbol : public Object {
protected:
    std::string sym_;
public:
    Symbol(const char *name) : sym_(name) {}
    Symbol(const std::string &name) : sym_(name) {}
    void log(uint32_t depth=999, uint32_t indent=0) const override;
    void to_nsof(NSOF &nsof) const override;

};

extern const Symbol symName;
extern const Symbol symType;
extern const Symbol symDiskType;
extern const Symbol symWhichVol;

class String : public Object {
protected:
    std::u16string str_;
public:
    String(const char16_t *name) : str_(name) {}
    String(const std::u16string &name) : str_(name) {}
    void log(uint32_t depth=999, uint32_t indent=0) const override;
    void to_nsof(NSOF &nsof) const override;
};

class Array : public Object {
protected:
    std::vector<Ref> elements_;
public:
    Array() = default;
    Array(std::initializer_list<Ref> init) : elements_(init) {}
    void add(const Ref &ref) { elements_.push_back(ref); }
    void log(uint32_t depth=999, uint32_t indent=0) const override;
    void to_nsof(NSOF &nsof) const override;
};

class Frame : public Object {
    std::vector<std::pair<const Symbol*, Ref>> frame_;
public:
    Frame() { type_ = Type::FRAME; }
    void add(const Symbol &key, const Ref &value) {
        frame_.push_back(std::make_pair(&key, value));
    }
    void log(uint32_t depth=999, uint32_t indent=0) const override;
    void to_nsof(NSOF &nsof) const override;
};

class NSOF {
    std::vector<uint8_t> data_;
    std::vector<const Object*> precedent_;
public:
    NSOF() = default;
    NSOF(const std::vector<uint8_t> &data) : data_(data) {}
    bool append(uint8_t byte); // return true when the stream reached its end
    void assign(const std::vector<uint8_t> &vec) { data_ = vec; }
    void clear() { data_.clear(); precedent_.clear(); }
    int size() const { return data_.size(); }
    //Ref to_ref() { return Ref(false); }
    std::vector<uint8_t> &to_nsof(Ref ref) { data_.push_back(0x02); ref.to_nsof(*this); return data_; }
    std::vector<uint8_t> &data() { return data_; }
    void log();
    bool write_precedent(const Object *obj);
};

} // namespace nd

#endif // ND_NEWTON_NSOF_H