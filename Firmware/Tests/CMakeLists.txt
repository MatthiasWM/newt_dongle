#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Matthias Melcher, robowerk.de
#

# Host unit tests for the platform independent code in `common/`.
#
#   cd Firmware/Tests
#   cmake -B build && cmake --build build && ./build/tests
#
# Every `test_*.cpp` file in this directory is added automatically.

cmake_minimum_required(VERSION 3.13)

project(newt_dongle_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ND_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

file(GLOB ND_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/test_*.cpp)

add_executable(tests
        ${ND_TEST_SOURCES}
        ${ND_ROOT}/common/Endpoint.cpp
        ${ND_ROOT}/common/Endpoints/Dock.cpp
        ${ND_ROOT}/common/Endpoints/SDCardEndpoint.cpp
        ${ND_ROOT}/common/Event.cpp
        ${ND_ROOT}/common/Pipe.cpp
        ${ND_ROOT}/common/Scheduler.cpp
        ${ND_ROOT}/common/StatusDisplay.cpp
        ${ND_ROOT}/common/Task.cpp
        ${ND_ROOT}/common/Filters/DTRSwitch.cpp
        ${ND_ROOT}/common/Filters/MNPFilter.cpp
        ${ND_ROOT}/common/Newton/DESKey.cpp
        ${ND_ROOT}/common/Newton/NSOF.cpp
        ${ND_ROOT}/common/Pipes/BufferedPipe.cpp
)

# This directory comes first, so `#include "main.h"` finds the test stub.
target_include_directories(tests PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${ND_ROOT}
)

# The same warnings as the firmware builds.
target_compile_options(tests PRIVATE -Wall -Wextra -Wno-missing-field-initializers)

# Catch use-after-free, double free, out of bounds access, and undefined behavior.
target_compile_options(tests PRIVATE -g -fsanitize=address,undefined -fno-omit-frame-pointer)
target_link_options(tests PRIVATE -fsanitize=address,undefined)

enable_testing()
add_test(NAME tests COMMAND tests)
