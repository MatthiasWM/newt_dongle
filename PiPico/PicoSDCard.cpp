//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoSDCard.h"

#include "main.h"
#include "common/Pipe.h"
#include "common/Scheduler.h"

#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "rtc.h"
#include "sd_card.h"
#include "hw_config.h"
#include "diskio.h"
#include "pico/time.h"

#include <stdio.h>

using namespace nd;

constexpr bool log_sdcard = false;

/*
 SD Card operations can be quite time consuming while serial data is pounding
 our CPU. To avoid interrupting the serial data stream, we will handle all slow 
 devices using the second core of the RP2040.

 Don't use multicore_fifo - it's only 8 words anyway
 Use multicore_doorbell to wake up the other core via IRQ. So the other core can sleep.


 https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#group_pico_multicore

 target_link_libraries(... pico_multicore)

 #include "pico/multicore.h"

 void core1_entry() {
    while(1) {
        
    }
 }
 multicore_launch_core1(core1_entry);


 Multicore IRQ safe queue:

 https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#group_queue


 // the Chip Select (CS) line can be used for Card Detection: "At power up this 
 line has a 50KOhm pull up enabled in the card... For Card detection, the host 
 detects that the line is pulled high."
 
 disk_status and FatFS functions:
 https://elm-chan.org/fsw/ff/00index_e.html

Also please explicitly set the configuration that we really want:
https://elm-chan.org/fsw/ff/doc/config.html
 */

// Hardware Configuration of SPI "objects"


// MOSI D10  P3
// MISO  D9  P4
// CLK   D8  P2
// CS    D1 P27

static spi_t spis[] = {  // One for each SPI.
    {
        .hw_inst = spi0,  // SPI component
        .miso_gpio = 4, // GPIO number (not pin number)
        .mosi_gpio = 3,
        .sck_gpio = 2,
        .baud_rate = 12500 * 1000,  
        //.baud_rate = 25 * 1000 * 1000, // Actual frequency: 20833333. 
    }
};

// Hardware Configuration of the SD Card "objects"
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0:",   // Name used to mount device
        .spi = &spis[0],  // Pointer to the SPI driving this card
        .ss_gpio = 27,    // The SPI slave select GPIO for this SD card
        .use_card_detect = false, // Use card detect GPIO
        .card_detect_gpio = 13,   // Card detect
        .card_detected_true = 1  // What the GPIO read returns when a card is
                                 // present. Use -1 if there is no card detect.
    }};

/* ********************************************************************** */

size_t sd_get_num() { return count_of(sd_cards); }
sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}

size_t spi_get_num() { return count_of(spis); }
spi_t *spi_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &spis[num];
    } else {
        return NULL;
    }
}


void test_sd_card() {

    time_init();

    printf("Testing SD card...\n");

    sd_card_t *pSD = sd_get_by_num(0);

    DSTATUS status = disk_status(0);

    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    FIL fil;
    const char* const filename = "filename.txt";
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    if (f_printf(&fil, "Hello, Matt!\n") < 0) {
        printf("f_printf failed\n");
    }
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }
    f_unmount(pSD->pcName);

    puts("Goodbye, world!");
 

}



PicoSDCardEndpoint::PicoSDCardEndpoint(Scheduler &scheduler)
:   SDCardEndpoint(scheduler)
{
    time_init();
    sd_card_ = sd_get_by_num(0);
}

PicoSDCardEndpoint::~PicoSDCardEndpoint() {
}

Result PicoSDCardEndpoint::init() 
{
    return SDCardEndpoint::init();
}

Result PicoSDCardEndpoint::task() {
    return SDCardEndpoint::task();
}

Result PicoSDCardEndpoint::send(Event event) {
    switch (event.type()) {
        case Event::Type::DATA: {
        }
    }
    if (log_sdcard) Log.log(event, 1);
    return SDCardEndpoint::send(event);
}


const char *PicoSDCardEndpoint::strerr(uint32_t err) {
    switch (err) {
        case FR_OK: return "OK";
        case FR_DISK_ERR: return "DISK_ERR";
        case FR_INT_ERR: return "INT_ERR";
        case FR_NOT_READY: return "NOT_READY";
        case FR_NO_FILE: return "NO_FILE";
        case FR_NO_PATH: return "NO_PATH";
        case FR_INVALID_NAME: return "INVALID_NAME";
        case FR_DENIED: return "DENIED";
        case FR_EXIST: return "EXIST";
        case FR_INVALID_OBJECT: return "INVALID_OBJECT";
        case FR_WRITE_PROTECTED: return "WRITE_PROTECTED";
        case FR_INVALID_DRIVE: return "INVALID_DRIVE";
        case FR_NOT_ENABLED: return "NOT_ENABLED";
        case FR_NO_FILESYSTEM: return "NO_FILESYSTEM";
        case FR_MKFS_ABORTED: return "MKFS_ABORTED";
        case FR_TIMEOUT: return "TIMEOUT";
        case FR_LOCKED: return "LOCKED";
        case FR_NOT_ENOUGH_CORE: return "NOT_ENOUGH_CORE";
        case FR_TOO_MANY_OPEN_FILES: return "TOO_MANY_OPEN_FILES";
        case FR_INVALID_PARAMETER: return "INVALID_PARAMETER";
    }
    return "UNKNOWN";
}

uint32_t PicoSDCardEndpoint::disk_status() {
    DSTATUS status = ::disk_status(0);
    uint32_t ret = 0;
    if (status & STA_NOINIT) ret |= 1;
    if (status & STA_NODISK) ret |= 2;
    if (status & STA_PROTECT) ret |= 4;
    return ret;
}

uint32_t PicoSDCardEndpoint::disk_initialize() {
    DSTATUS status = ::disk_initialize(0);
    uint32_t ret = 0;
    if (status & STA_NOINIT) ret |= 1;
    if (status & STA_NODISK) ret |= 2;
    if (status & STA_PROTECT) ret |= 4;
    return ret;
}

uint32_t PicoSDCardEndpoint::get_label(char *label_buffer) {
    FRESULT fr = f_mount(&fat_fs_, "", 1);
    if (fr==FR_OK)
        fr = f_getlabel("", label_buffer, NULL);
    f_unmount("");
    return fr;
}
// vim: set sw=4 ts=4 et:


