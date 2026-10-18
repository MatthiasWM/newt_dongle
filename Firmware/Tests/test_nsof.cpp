//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "test.h"

#include "common/Newton/NSOF.h"

using namespace nd;

static const Symbol symId { "id" };
static const Symbol symVersion { "version" };
static const Symbol symDoesAuto { "doesAuto" };

// NSOF part of the `dinf` command as sent by NCU (see Dock::send_cmd_dinf()):
// [ { name: "Newton Connection", id: 2, version: 1, doesAuto: true } ]
static const std::vector<uint8_t> kDinfNSOF = {
    0x02, 0x05, 0x01, 0x06, 0x04,
    0x07, 0x04, 'n', 'a', 'm', 'e',
    0x07, 0x02, 'i', 'd',
    0x07, 0x07, 'v', 'e', 'r', 's', 'i', 'o', 'n',
    0x07, 0x08, 'd', 'o', 'e', 's', 'A', 'u', 't', 'o',
    0x08, 0x24,
    0x00, 'N', 0x00, 'e', 0x00, 'w', 0x00, 't', 0x00, 'o', 0x00, 'n', 0x00, ' ',
    0x00, 'C', 0x00, 'o', 0x00, 'n', 0x00, 'n', 0x00, 'e', 0x00, 'c', 0x00, 't',
    0x00, 'i', 0x00, 'o', 0x00, 'n', 0x00, 0x00,
    0x00, 0x08,     // 2
    0x00, 0x04,     // 1
    0x00, 0x1A,     // true
};

TEST(nsof_encodes_dinf_like_ncu) {
    Frame *connection = Frame::New();
    connection->add(symName, Ref(String::New(u"Newton Connection")));
    connection->add(symId, Ref(2));
    connection->add(symVersion, Ref(1));
    connection->add(symDoesAuto, Ref(true));
    Array list;
    list.add(Ref(connection));

    NSOF nsof;
    nsof.to_nsof(Ref(list));
    CHECK_BYTES(nsof.data(), kDinfNSOF);
}

TEST(nsof_decodes_a_string) {
    NSOF nsof({ 0x02, 0x08, 0x08, 0x00, 'a', 0x00, '.', 0x00, 'b', 0x00, 0x00 });
    int32_t error = 0;
    Ref ref = nsof.to_ref(error);
    CHECK(error == 0);
    String *str = ref.as_string();
    CHECK(str != nullptr);
    if (str) CHECK(str->str() == u"a.b");
}

TEST(nsof_decodes_an_array_of_strings) {
    // [ "SD", "Packages" ], like a path from `spth`
    NSOF nsof({
        0x02, 0x05, 0x02,
        0x08, 0x06, 0x00, 'S', 0x00, 'D', 0x00, 0x00,
        0x08, 0x12, 0x00, 'P', 0x00, 'a', 0x00, 'c', 0x00, 'k', 0x00, 'a',
                    0x00, 'g', 0x00, 'e', 0x00, 's', 0x00, 0x00,
    });
    int32_t error = 0;
    Ref ref = nsof.to_ref(error);
    CHECK(error == 0);
    Array *array = ref.as_array();
    CHECK(array != nullptr);
    if (!array) return;
    CHECK(array->size() == 2);
    if (array->size() != 2) return;
    CHECK(array->at(0).as_string() && array->at(0).as_string()->str() == u"SD");
    CHECK(array->at(1).as_string() && array->at(1).as_string()->str() == u"Packages");
}

TEST(nsof_decodes_a_symbol) {
    NSOF nsof({ 0x02, 0x07, 0x04, 'n', 'a', 'm', 'e' });
    int32_t error = 0;
    Ref ref = nsof.to_ref(error);
    CHECK(error == 0);
    CHECK(ref.as_symbol() == &symName);
}

TEST(nsof_decodes_and_reencodes_dinf) {
    NSOF in(kDinfNSOF);
    int32_t error = 0;
    Ref ref = in.to_ref(error);
    CHECK(error == 0);
    CHECK(ref.as_array() != nullptr);
    if (!ref.as_array()) return;
    CHECK(ref.as_array()->size() == 1);
    CHECK(ref.as_array()->at(0).as_frame() != nullptr);

    NSOF out;
    out.to_nsof(ref);
    CHECK_BYTES(out.data(), kDinfNSOF);
}

TEST(nsof_decodes_a_precedent_to_a_string) {
    // [ "A", <precedent 1> ] - precedent 0 is the array, 1 is the string
    NSOF nsof({ 0x02, 0x05, 0x02, 0x08, 0x04, 0x00, 'A', 0x00, 0x00, 0x09, 0x01 });
    int32_t error = 0;
    Ref ref = nsof.to_ref(error);
    CHECK(error == 0);
    Array *array = ref.as_array();
    CHECK(array != nullptr);
    if (!array || array->size() != 2) { CHECK(false); return; }
    CHECK(array->at(0).as_string() != nullptr);
    CHECK(array->at(1).as_string() == array->at(0).as_string());
    // When `ref` goes out of scope, the string must be deleted exactly once.
}

TEST(nsof_reuses_symbols_as_precedents) {
    // [ { name: "A" }, { name: "B" } ] - the second `name` is precedent 2
    const std::vector<uint8_t> list = {
        0x02, 0x05, 0x02,
        0x06, 0x01, 0x07, 0x04, 'n', 'a', 'm', 'e', 0x08, 0x04, 0x00, 'A', 0x00, 0x00,
        0x06, 0x01, 0x09, 0x02, 0x08, 0x04, 0x00, 'B', 0x00, 0x00,
    };
    NSOF in(list);
    int32_t error = 0;
    Ref ref = in.to_ref(error);
    CHECK(error == 0);
    NSOF out;
    out.to_nsof(ref);
    CHECK_BYTES(out.data(), list);
}
