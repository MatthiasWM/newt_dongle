//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

// A minimal test framework for the host unit tests.
//
//   TEST(nsof_parses_a_string) {
//       CHECK(1 + 1 == 2);
//       CHECK_BYTES(actual_vector, expected_vector);
//   }

#ifndef ND_TESTS_TEST_H
#define ND_TESTS_TEST_H

#include <cstdint>
#include <vector>

namespace nd_test {

typedef void (*TestFunction)();

struct Registrar {
    Registrar(const char *name, TestFunction fn);
};

void check(bool ok, const char *file, int line, const char *expression);
void check_bytes(const std::vector<uint8_t> &actual, const std::vector<uint8_t> &expected,
                 const char *file, int line, const char *expression);

} // namespace nd_test

#define TEST(name) \
    static void name(); \
    static nd_test::Registrar name##_registrar(#name, name); \
    static void name()

#define CHECK(expression) \
    nd_test::check((expression), __FILE__, __LINE__, #expression)

#define CHECK_BYTES(actual, expected) \
    nd_test::check_bytes((actual), (expected), __FILE__, __LINE__, #actual)

#endif // ND_TESTS_TEST_H
