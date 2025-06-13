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
const Symbol nd::symUnknown { "unknown" };


Ref::Ref(const Ref &other) : type_(other.type_) {
    switch (type_) {
        case Type::INT: int_ = other.int_; break;
        case Type::BOOL: bool_ = other.bool_; break;
        case Type::CHAR16: char_ = other.char_; break;
        case Type::REAL: real_ = other.real_; break;
        case Type::OBJECT: 
            object_ = other.object_; 
            if (object_->ref_count_) object_->ref_count_++;
            break;
    }
}

Ref::Ref(Ref &&other) noexcept : type_(other.type_) {
    switch (type_) {
        case Type::INT: int_ = other.int_; break;
        case Type::BOOL: bool_ = other.bool_; break;
        case Type::CHAR16: char_ = other.char_; break;
        case Type::REAL: real_ = other.real_; break;
        case Type::OBJECT: object_ = other.object_; break;
    }
    other.type_ = Type::INT; // Reset the moved-from object
}

Ref &Ref::operator=(const Ref &other) {
    if (this != &other) {
        clear_();
        type_ = other.type_;
        switch (type_) {
            case Type::INT: int_ = other.int_; break;
            case Type::BOOL: bool_ = other.bool_; break;
            case Type::CHAR16: char_ = other.char_; break;
            case Type::REAL: real_ = other.real_; break;
            case Type::OBJECT: 
                object_ = other.object_; 
                if (object_->ref_count_) object_->ref_count_++;
                break;
        }
    }
    return *this;
}

Ref &Ref::operator=(Ref &&other) noexcept {
    if (this != &other) {
        clear_();
        type_ = other.type_;
        switch (type_) {
            case Type::INT: int_ = other.int_; break;
            case Type::BOOL: bool_ = other.bool_; break;
            case Type::CHAR16: char_ = other.char_; break;
            case Type::REAL: real_ = other.real_; break;
            case Type::OBJECT: object_ = other.object_; break;
        }
        other.type_ = Type::INT; // Reset the moved-from object
    }
    return *this;
}

Ref Ref::Raw(uint32_t v) {
    if (v == 0x0000001A) return Ref(true);
    if (v == 0x00000002) return Ref(false);
    if ((v & 0x00000003) == 0x00000000) return Ref( ((int32_t)(v)) >> 2 );
    //if ((v & 0x00000003) == 0x00000003) return Ref( ((int32_t)(v)) >> 2 ); // Magic Pointer
    if ((v & 0x0000000F) == 0x00000006) return Ref( (char16_t)(v >> 4) ); // UniChar
    if (kLogNSOF) Log.log("NSOF: Ref::Raw: unknown type\n");
    return Ref(false);
}

Ref::~Ref()  // Destructor
{
    clear_();
}

void Ref::clear_() {
    if (type_ == Type::OBJECT && object_->ref_count_) {
        object_->ref_count_--; // Decrement ref count of the old object
        if (object_->ref_count_ == 0) {
            // Log.log("\n\nGC delete ");
            // object_->log(1, 0);
            // Log.log("\n");
            delete object_;
        }
    }
    type_ = Type::INT; // Reset to default type
    int_ = 0; // Reset the integer value
}


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

String *Ref::as_string() const
{
    if (type_ == Type::OBJECT && object_->is_string()) {
        return static_cast<String*>(object_);
    }
    return nullptr; // Not a string
}

Array *Ref::as_array() const
{
    if (type_ == Type::OBJECT && object_->is_array()) {
        return static_cast<Array*>(object_);
    }
    return nullptr; // Not an array
}

Frame *Ref::as_frame() const
{
    if (type_ == Type::OBJECT && object_->is_frame()) {
        return static_cast<Frame*>(object_);
    }
    return nullptr; // Not a frame
}

Symbol *Ref::as_symbol() const
{
    if (type_ == Type::OBJECT && object_->is_symbol()) {
        return static_cast<Symbol*>(object_);
    }
    return nullptr; // Not a symbol
}



// This array will be filled by the Symbol constructor
// and contains all known symbols. It is used to find symbols by name.
std::vector<Symbol*> Symbol::known_symbols_;

Symbol::Symbol(const char *name) : sym_(name) { 
    type_ = Type::SYMBOL; 
    known_symbols_.push_back(this); // Add to known symbols
}

Symbol::Symbol(const std::string &name) : sym_(name) { 
    type_ = Type::SYMBOL; 
    known_symbols_.push_back(this); // Add to known symbols
}

const Symbol *Symbol::find(const std::string &name) {
    for (auto &sym : known_symbols_) {
        if (sym->sym_ == name) return sym;
    }
    return &symUnknown;
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
            if (as_bool()) {
                nsof.data().push_back(0);
                nsof.data().push_back(0x1A);
            } else {
                nsof.data().push_back(10);
            }
            break;
        case Type::CHAR16: { // char16_t
            char16_t c = as_char16();
            if (c < 127) {
                nsof.data().push_back(1);
                nsof.data().push_back((uint8_t)c); // 1-byte character
            } else {
                nsof.data().push_back(1);
                nsof.data().push_back((c >> 8) & 0xFF); // high byte
                nsof.data().push_back(c & 0xFF); // low byte
            }
            break; }
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

/** 
 * \brief Write a precedent marker if the object was previously written.
 * If it was not, add the object to the precedent list and return false, so the
 * caller will write the object.
 */
bool NSOF::write_precedent(const Object *obj) {
    if (!obj) return false;
    int i = 0;
    for (i = 0; i<precedent_.size(); i++) {
        if (precedent_[i] == obj) {
            // Object was already written, write a precedent marker
            data().push_back(0x09);
            push_xlong(data(), i);
            return true; // Indicate that the object was already written
        }
    }
    precedent_.push_back(obj);
    return false;
}

void Symbol::to_nsof(NSOF &nsof) const {
    if (nsof.write_precedent(this)) return; // If already written, just return
    nsof.data().push_back(7);
    push_xlong(nsof.data(), sym_.size());
    for (auto &c: sym_) {
        nsof.data().push_back(c);
    }
}

void String::to_nsof(NSOF &nsof) const {
    if (nsof.write_precedent(this)) return; // If already written, just return
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
    if (nsof.write_precedent(this)) return; // If already written, just return
    nsof.data().push_back(5);
    push_xlong(nsof.data(), elements_.size());
    for (auto &ref: elements_) {
        ref.to_nsof(nsof);
    }
}

void Frame::to_nsof(NSOF &nsof) const {
    if (nsof.write_precedent(this)) return; // If already written, just return
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


int32_t NSOF::get_xlong(int32_t &error_code) {
    if (data_.empty() || data_.size() < crsr_ + 1) {
        error_code = -54002; // Zero Length data
        return 0;
    }
    uint32_t ret = data_[crsr_++];
    if (ret == 0xFF) {
        // Read a 4-byte integer
        if (data_.size() < crsr_ + 4) {
            error_code = -54002; // Zero Length data
            return 0;
        }
        ret = (data_[crsr_] << 24) | (data_[crsr_ + 1] << 16) |
              (data_[crsr_ + 2] << 8) | data_[crsr_ + 3];
        crsr_ += 4;
    }
    return ret;
}

/**
 * Recursive part of converting an NSOF stream into a Ref.
 */
Ref NSOF::to_ref_(int32_t &error_code)
{
    // TODO: precedents
    if (data_.empty() || data_.size() <= crsr_) {
        error_code = -54002; // Zero Length data
        return Ref(false);
    }
    uint8_t type = data_[crsr_++];
    switch (type) {
        case 0: { // immediate
            uint32_t value = get_xlong(error_code);
            if (error_code) return Ref(false);
            if (kLogNSOF) Log.logf("NSOF: to_ref: immediate %d\n", value);
            return Ref::Raw(value); }
        case 1: { // character
            if (data_.size() <= crsr_) {
                if (kLogNSOF) Log.log("NSOF: to_ref: character too short\n");
                error_code = -54002; // Zero Length data
                return Ref(false);
            }
            uint8_t c = data_[crsr_++];
            if (kLogNSOF) Log.logf("NSOF: to_ref: character '%c'\n", c);
            return Ref((char16_t)c); }
        case 2: { // unichar
            if (data_.size() <= crsr_ + 1) {
                if (kLogNSOF) Log.log("NSOF: to_ref: unichar too short\n");
                error_code = -54002; // Zero Length data
                return Ref(false);
            }
            uint16_t c = (data_[crsr_] << 8) | data_[crsr_ + 1];
            crsr_ += 2;
            if (kLogNSOF) Log.logf("NSOF: to_ref: unichar '%c'\n", c);
            return Ref((char16_t)c); }
        // 3: binary [size, class, data]
        // 4: array [#slots, class, values...]
        case 5: { // plain array [#slots, values...]
            uint32_t size = get_xlong(error_code);
            if (error_code) return Ref(false);
            if (kLogNSOF) Log.logf("NSOF: to_ref: array with %d slots\n", size);
            Array *array = Array::New();
            precedent_.push_back(array); // Add to precedent list
            for (uint32_t i = 0; i < size; i++) {
                Ref ref = to_ref_(error_code);
                if (error_code) { delete array; return Ref(false); }
                array->add(ref);
            }
            return Ref(array); }
        case 6: { // frame [#slots, keys..., values...]
            uint32_t size = get_xlong(error_code);
            if (error_code) return Ref(false);
            if (kLogNSOF) Log.logf("NSOF: to_ref: frame with %d slots\n", size);
            Frame *frame = Frame::New();
            precedent_.push_back(frame); // Add to precedent list
            for (uint32_t i = 0; i < size; i++) {
                Ref ref = to_ref_(error_code);
                if (error_code) { delete frame; return Ref(false); }
                if (!ref.is_object() || !ref.as_object()->is_symbol()) {
                    if (kLogNSOF) Log.log("NSOF: to_ref: frame key is not a symbol\n");
                    error_code = -28210; // Invalid Type
                    delete frame;
                    return Ref(false);
                }
                const Symbol *key = static_cast<Symbol*>(ref.as_object());
                frame->add(*key, Ref(false));
            }
            for (uint32_t i = 0; i < size; i++) {
                Ref ref = to_ref_(error_code);
                if (error_code) { delete frame; return Ref(false); }
                frame->set(i, ref);
            }
            return Ref(frame); }
        case 7: { // symbol [#characters, characters (no trailing nul)]
            uint32_t size = get_xlong(error_code);
            if (error_code) return Ref(false);
            std::string str;
            if (kLogNSOF) Log.log("\n\nSYMBOL:\n");
            while (size > 1) {
                if (crsr_ + 1 >= data_.size()) {
                    if (kLogNSOF) Log.log("NSOF: to_ref: Symbol too short\n");
                    error_code = -54002; // Zero Length data
                    return Ref(false);
                }
                uint8_t c = data_[crsr_++];
                str.push_back(c);
                if (kLogNSOF) {
                    if (c<33 || c>126) {
                        Log.logf("<%04x>", c);
                    } else {
                        Log.logf("%c", c);
                    }
                }
                size -= 1;
            }
            if (kLogNSOF) Log.log("\nDONE\n");
            const Symbol *sym = Symbol::find(str); // -> existing symbols only!
            precedent_.push_back(sym); // Add to precedent list
            return Ref(sym); }
        case 8: { // String
            uint32_t size = get_xlong(error_code);
            if (error_code) return Ref(false);
            std::u16string str;
            if (kLogNSOF) Log.log("\n\nSTRING:\n");
            while (size > 2) {
                if (crsr_ + 1 >= data_.size()) {
                    if (kLogNSOF) Log.log("NSOF: to_ref: String too short\n");
                    error_code = -54002; // Zero Length data
                    return Ref(false);
                }
                uint16_t c = (data_[crsr_] << 8) | data_[crsr_ + 1];
                str.push_back(c);
                if (kLogNSOF) {
                    if (c<33 || c>126) {
                        Log.logf("<%04x>", c);
                    } else {
                        Log.logf("%c", c);
                    }
                }
                crsr_ += 2;
                size -= 2;
            }
            crsr_ += 2; // skip trailing 'nul' bytes
            if (kLogNSOF) Log.log("\nDONE\n");
            String *str_obj = String::New(str);
            precedent_.push_back(str_obj); // Add to precedent list
            return Ref(str_obj); }
        case 9: { // Precedent
            uint32_t index = get_xlong(error_code);
            if (error_code) return Ref(false);
            if (kLogNSOF) Log.logf("NSOF: to_ref: precedent %d\n", index);
            if (index >= precedent_.size()) {
                if (kLogNSOF) Log.log("NSOF: to_ref: precedent index out of bounds\n");
                error_code = -28210; // Invalid Type
                return Ref(false);
            }
            return Ref(precedent_[index]); }
        case 10: // NIL
            if (kLogNSOF) Log.log("NSOF: to_ref: NIL\n");
            return Ref(false); // Return a Ref with false, indicating NIL
        // 11: small rect [bytes: top, left, bottom, right]
        // 12: large binary [ignore!]
        default:
            if (kLogNSOF) Log.log("NSOF: to_ref: unknown type\n");
            error_code = -28210; // Invalid Type
            return Ref(false);
    }
}


/**
 * Convert the data block into a NewtonScript Object.
 */
Ref NSOF::to_ref(int32_t &error_code) {
    if (data_.empty() || data_.size() <= crsr_) {
        if (kLogNSOF) Log.log("NSOF: to_ref: data is empty\n");
        error_code = -54002; // Zero Length data
        return Ref(false);
    }
    if (data_[crsr_++] != 0x02) {
        if (kLogNSOF) Log.log("NSOF: to_ref: expected 0x02 at cursor\n");
        error_code = -28210; // Invalid Type
        return Ref(false);
    }
    precedent_.clear(); // Clear precedent list for this conversion
    Ref ref = to_ref_(error_code);
    ref.logln();
    return ref;
}