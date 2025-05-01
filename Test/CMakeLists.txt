#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Matthias Melcher, robowerk.de
#

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(newt_dongle C CXX ASM)

add_executable(newt_dongle
        main.cpp 
)

# User defined macros, but also see nd_config.h
target_compile_definitions(newt_dongle PRIVATE
        NEWT_TEST=1
)

# Any additional libraries
#target_link_libraries(newt_dongle)
        # none yet
#)

# All include paths can be relative to the project root
target_include_directories(newt_dongle PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add the platform agnostic part of the Dongle Firmware
add_subdirectory(common)


