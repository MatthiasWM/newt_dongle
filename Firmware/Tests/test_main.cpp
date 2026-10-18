//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "test.h"

#include "main.h"

#include <cstdarg>
#include <cstdio>
#include <vector>

nd::TestLog Log;

void nd::TestLog::log(const char *message, int pipe) {
    (void)pipe;
    fputs(message, stdout);
}

void nd::TestLog::logf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void nd::TestLog::indent(uint32_t depth) {
    for (uint32_t i = 0; i < depth; i++) fputs("  ", stdout);
}

namespace {

struct TestCase {
    const char *name;
    nd_test::TestFunction fn;
};

// Function-local static, so registration works regardless of the order in
// which the test files are initialized.
std::vector<TestCase> &all_tests() {
    static std::vector<TestCase> tests;
    return tests;
}

int failures_in_current_test = 0;

} // namespace

nd_test::Registrar::Registrar(const char *name, TestFunction fn) {
    all_tests().push_back({ name, fn });
}

void nd_test::check(bool ok, const char *file, int line, const char *expression) {
    if (ok) return;
    printf("    FAILED %s:%d: %s\n", file, line, expression);
    failures_in_current_test++;
}

void nd_test::check_bytes(const std::vector<uint8_t> &actual, const std::vector<uint8_t> &expected,
                          const char *file, int line, const char *expression) {
    if (actual == expected) return;
    size_t i = 0;
    while (i < actual.size() && i < expected.size() && actual[i] == expected[i]) i++;
    printf("    FAILED %s:%d: %s differs at byte %zu (size %zu, expected %zu)\n",
           file, line, expression, i, actual.size(), expected.size());
    printf("      actual:  ");
    for (size_t j = i; j < actual.size() && j < i + 16; j++) printf("%02X ", actual[j]);
    printf("\n      expected:");
    for (size_t j = i; j < expected.size() && j < i + 16; j++) printf(" %02X", expected[j]);
    printf("\n");
    failures_in_current_test++;
}

int main() {
    int failed_tests = 0;
    for (auto &test : all_tests()) {
        failures_in_current_test = 0;
        test.fn();
        printf("%s %s\n", failures_in_current_test ? "FAIL" : "ok  ", test.name);
        if (failures_in_current_test) failed_tests++;
    }
    printf("\n%zu tests, %d failed\n", all_tests().size(), failed_tests);
    return failed_tests ? 1 : 0;
}
