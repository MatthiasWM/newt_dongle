# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

# ====================================================================================

include(FetchContent)

FetchContent_Declare(
        fatfs_spi
        GIT_REPOSITORY https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git
        GIT_TAG        bbccc5d2deea09e128e2576ef35f72b473c5ba5f
        PATCH_COMMAND sed -i.bak s/FF_USE_LABEL.\*0/FF_USE_LABEL\ 1/ FatFs_SPI/ff15/source/ffconf.h
        # FF_FS_READONLY 1
        # FF_USE_STRFUNC 0
        # FF_PRINT_FLOAT 0
        # FF_USE_LFN 1
        # FF_LFN_UNICODE 2 for UTF-8 (Newton is UTF-16!)
        # FF_FS_RPATH 1
        # FF_FS_TIMEOUT ??
        UPDATE_DISCONNECTED 1
)

FetchContent_MakeAvailable(fatfs_spi)

message(STATUS "FatFs_SPI_SOURCE_DIR: ${fatfs_spi_SOURCE_DIR}")

add_subdirectory( ${fatfs_spi_SOURCE_DIR}/FatFs_SPI ${fatfs_spi_BINARY_DIR}/FatFs_SPI )

# ====================================================================================

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(newt_dongle C CXX ASM)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

set(PICO_CXX_ENABLE_RTTI 1)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(newt_dongle 
        main.cpp 
        main.h
        usb_descriptors.c
        tusb_config.h
)

pico_set_program_name(newt_dongle "newt_dongle")
pico_set_program_version(newt_dongle "0.0.4")

# Modify the below lines to enable/disable output over UART/USB
target_compile_definitions(newt_dongle PRIVATE
        PICO_DEFAULT_UART=1
        PICO_DEFAULT_UART_TX_PIN=8
        PICO_DEFAULT_UART_RX_PIN=9 
)
pico_enable_stdio_uart(newt_dongle 1)
pico_enable_stdio_usb(newt_dongle 0)

# Add the standard library to the build
target_link_libraries(newt_dongle
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(newt_dongle PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/RP2040
)

# Add any user requested libraries
target_link_libraries(newt_dongle
        hardware_spi
        hardware_pio
        hardware_timer
        hardware_clocks
        pico_multicore
        tinyusb_device
        tinyusb_board
        FatFs_SPI
        )

set(APP newt_dongle)
add_subdirectory(common)
add_subdirectory(RP2040)

pico_add_extra_outputs(newt_dongle)

