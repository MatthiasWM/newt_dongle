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
)

FetchContent_MakeAvailable(fatfs_spi)

message(STATUS "FatFs_SPI_SOURCE_DIR: ${fatfs_spi_SOURCE_DIR}")
message(STATUS "FatFs_SPI_BINARY_DIR: ${fatfs_spi_BINARY_DIR}")
message(STATUS "FatFs_SPI_INCLUDE_DIR: ${fatfs_spi_INCLUDE_DIR}")
message(STATUS "FatFs_SPI_LIBRARIES: ${FatFs_SPI_LIBRARIES}")
message(STATUS "FatFs_SPI_DEFINITIONS: ${FatFs_SPI_DEFINITIONS}")
message(STATUS "FatFs_SPI_VERSION: ${FatFs_SPI_VERSION}")
message(STATUS "FatFs_SPI_VERSION_STRING: ${FatFs_SPI_VERSION_STRING}")

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
        usb_descriptors.c
        PicoCDCEndpoint.cpp
        PicoCDCEndpoint.h
        PicoUARTEndpoint.cpp
        PicoUARTEndpoint.h
        PicoScheduler.cpp
        PicoScheduler.h
        PicoStdioLog.cpp
        PicoStdioLog.h
        PicoSDCard.cpp
        PicoSDCard.h
)

pico_set_program_name(newt_dongle "newt_dongle")
pico_set_program_version(newt_dongle "0.1")

# no_flash means the target is to run from RAM
#pico_set_binary_type(newt_dongle no_flash)

# Generate PIO header
pico_generate_pio_header(newt_dongle ${CMAKE_CURRENT_LIST_DIR}/blink.pio)


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
)

# Add any user requested libraries
target_link_libraries(newt_dongle
        hardware_spi
        hardware_pio
        hardware_timer
        hardware_clocks
        tinyusb_device
        tinyusb_board
        FatFs_SPI
        )

add_subdirectory(common)

pico_add_extra_outputs(newt_dongle)

