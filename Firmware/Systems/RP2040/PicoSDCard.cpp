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

#include <pico/time.h>

#include <cstdint>
#include <stdio.h>

using namespace nd;

static const TCHAR Drive0[] = { '0', ':', 0 };

/*
  NOTE: the UTF16 option does not work on teh SDCard driver. We must convert 
  UTF8 to UTF16 by hand.


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
        // .set_drive_strength = true, // Set drive strength for GPIO outputs
        // .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA, // 12mA
        // .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA, // 12mA
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


        // Drive strength levels for GPIO outputs.
        // enum gpio_drive_strength { GPIO_DRIVE_STRENGTH_2MA = 0, GPIO_DRIVE_STRENGTH_4MA = 1, GPIO_DRIVE_STRENGTH_8MA = 2,
        // GPIO_DRIVE_STRENGTH_12MA = 3 }
        // enum gpio_drive_strength gpio_get_drive_strength (uint gpio)
        // if (spi_p->set_drive_strength) {
        //     gpio_set_drive_strength(spi_p->mosi_gpio, spi_p->mosi_gpio_drive_strength);
        //     gpio_set_drive_strength(spi_p->sck_gpio, spi_p->sck_gpio_drive_strength);
        // }

/* ********************************************************************** */

size_t sd_get_num() {
    return count_of(sd_cards); 
}

sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}

size_t spi_get_num() { 
    return count_of(spis); 
}

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

    // DSTATUS status = disk_status(0);

    // FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    FRESULT fr = f_mount(&pSD->fatfs, (const uint16_t*)u"0:", 1);
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    FIL fil;
    const char* const filename = "filename.txt";
    fr = f_open(&fil, (const uint16_t*)u"filenäme.txt", FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    // if (f_printf(&fil, (const uint16_t*)u"Hello, Matt!\n") < 0) {
    //     printf("f_printf failed\n");
    // }
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }
    f_unmount((const uint16_t*)u"0:");

    puts("Goodbye, world!");
 

}

void PicoSDCardEndpoint::early_init()
{
    auto cs = sd_cards[0].ss_gpio;
    gpio_init(cs);
    gpio_put(cs, 1);
    gpio_set_dir(cs, GPIO_OUT);

    auto mosi = spis[0].mosi_gpio;
    gpio_init(mosi);
    gpio_put(mosi, 0);
    gpio_set_dir(mosi, GPIO_OUT);

    auto miso = spis[0].miso_gpio;
    gpio_init(miso);
    gpio_set_dir(miso, GPIO_IN); // MISO is input
    gpio_pull_up(miso); // Enable pull-up on MISO

    auto sck = spis[0].sck_gpio;
    gpio_init(sck);
    gpio_put(sck, 0);
    gpio_set_dir(sck, GPIO_OUT);
}

PicoSDCardEndpoint::PicoSDCardEndpoint(Scheduler &scheduler)
:   SDCardEndpoint(scheduler)
{
}

PicoSDCardEndpoint::~PicoSDCardEndpoint() {
}

uint32_t PicoSDCardEndpoint::mount_() {
    if (!mounted_) {
        app_status.repeat(AppStatus::SDCARD_ACTIVE, 2); // Flash blue
        sd_card_t *pSD = sd_get_by_num(0);
        FRESULT fr = f_mount(&pSD->fatfs, Drive0, 1);
        status_ = fr;
        if (fr != FR_OK) {
            if (kLogSDCard) Log.logf("f_mount error: %s (%d)\n", strerr(fr), fr);
            f_unmount(Drive0);
            spis[0].initialized = false; // Reset the SPI driver
            return fr;
        }
        mounted_ = true;
    }
    return status_;
}

Result PicoSDCardEndpoint::init() 
{
    SDCardEndpoint::init();
    if (!initialized_) {
        time_init();
        sd_card_t *pSD = sd_get_by_num(0);
        f_unmount((const uint16_t*)u"0:");
    }
    return Result::OK;
}

Result PicoSDCardEndpoint::task() {
    return SDCardEndpoint::task();
}

Result PicoSDCardEndpoint::send(Event event) {
    switch (event.type()) {
        case Event::Type::DATA: {
        }
    }
    if (kLogSDCard) Log.log(event, 1);
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
        case FR_IS_DIRECTORY: return "IS_DIRECTORY";
        case FR_IS_PACKAGE: return "IS_PACKAGE"; // Custom error for package files
    }
    return "UNKNOWN";
}

const std::u16string &PicoSDCardEndpoint::get_label() {
    if (!mounted_) {
        label_.clear();
        uint32_t err = mount_();
        if (err != FR_OK) {
            // TODO: set to NO_DISK or NO_CARD, etc.
            label_ = u"ERROR";
            return label_;
        }
        sd_card_t *pSD = sd_get_by_num(0);  // Get the first SD card
        TCHAR label_buffer[14];  // FF_LFN_UNICODE=1
        label_buffer[0] = 0; // Initialize the buffer to empty
        app_status.repeat(AppStatus::SDCARD_ACTIVE, 1); // Flash blue
        FRESULT fr = f_getlabel(Drive0, label_buffer, NULL);
        if (fr == FR_OK) {
            label_.assign((char16_t*)label_buffer); // Convert the label to std::u16string
        } else {
            label_ = u"ERROR";
        }
    }
    return label_;
}
// vim: set sw=4 ts=4 et:


uint32_t PicoSDCardEndpoint::opendir() 
{
    if (!mounted_) {
        uint32_t err = mount_();
        if (err != FR_OK) {
            if (kLogSDCard) Log.logf("opendir: mount error: %s (%d)\n", strerr(err), err);
            return err;
        }
    }
    app_status.repeat(AppStatus::SDCARD_ACTIVE, 1); // Flash blue
    FRESULT fr = f_opendir(&dir_, Drive0);
    if (fr != FR_OK) {
        if (kLogSDCard) Log.logf("opendir: f_opendir error: %s (%d)\n", strerr(fr), fr);
        return fr;
    }
    return FR_OK;
}

static int strlen16(const TCHAR* strarg)
{
   if(!strarg)
        return -1; //strarg is NULL pointer
   const TCHAR* str = strarg;
   for(;*str;++str)
        ; // empty body
   return str-strarg;
}

uint32_t PicoSDCardEndpoint::readdir(std::u16string &name) {
    for (;;) {
        FILINFO fno;
        app_status.repeat(AppStatus::SDCARD_ACTIVE, 1); // Flash blue
        FRESULT fr = f_readdir(&dir_, &fno);
        if (fr != FR_OK) {
            if (kLogSDCard) Log.logf("readdir: f_readdir error: %s (%d)\n", strerr(fr), fr);
            return fr;
        }
        if (fno.fname[0]==0) {
            // No more files in the directory
            if (kLogSDCard) Log.log("readdir: no more files\n");
            return FR_NO_FILE; // No more files
        }
        if (fno.fattrib & AM_HID) {
            // Skip hidden files
            if (kLogSDCard) Log.logf("readdir: skipping hidden file: %s\n", fno.fname);
            continue;
        }
        if (fno.fattrib & AM_SYS) {
            // Skip system files
            if (kLogSDCard) Log.logf("readdir: skipping system file: %s\n", fno.fname);
            continue;
        }
        if (fno.fattrib & AM_DIR) {
            // Send directories
            name = (const char16_t*)fno.fname; // Convert the TCHAR name to std::u16string
            if (kLogSDCard) Log.logf("readdir: returning a directory: %s\n", fno.fname);
            return FR_IS_DIRECTORY; // Indicates a directory
        }
        auto n = fno.fname; // Get the file name
        auto len = strlen16(n);
        if (len>=4 && n[len-4]=='.' && (n[len-3]=='p' || n[len-3]=='P') && (n[len-2]=='k' || n[len-2]=='K') && (n[len-1]=='g' || n[len-1]=='G')) {
            // Do return package files
            name = (const char16_t*)fno.fname; // Convert the TCHAR name to std::u16string
            if (kLogSDCard) Log.logf("readdir: returning a package file: %s\n", fno.fname);
            return FR_IS_PACKAGE;
        }
        if (kLogSDCard) Log.logf("readdir: skipping file: %s\n", fno.fname);
    }
}

uint32_t PicoSDCardEndpoint::closedir() {
    app_status.repeat(AppStatus::SDCARD_ACTIVE, 1); // Flash blue
    FRESULT fr = f_closedir(&dir_);
    if (fr != FR_OK) {
        if (kLogSDCard) Log.logf("closedir: f_closedir error: %s (%d)\n", strerr(fr), fr);
        return fr;
    }
    return FR_OK;
}   

uint32_t PicoSDCardEndpoint::chdir(const std::u16string &path)
{
    if (!mounted_) {
        uint32_t err = mount_();
        if (err != FR_OK) {
            if (kLogSDCard) Log.logf("chdir: mount error: %s (%d)\n", strerr(err), err);
            return err;
        }
    }
    app_status.repeat(AppStatus::SDCARD_ACTIVE, 1); // Flash blue
    FRESULT fr = f_chdir((const TCHAR*)path.c_str());
    if (fr != FR_OK) {
        if (kLogSDCard) Log.logf("chdir: f_chdir error: %s (%d)\n", strerr(fr), fr);
        return fr;
    }
    return FR_OK;
}

uint32_t PicoSDCardEndpoint::getcwd(std::u16string &path) 
{
    path.clear();
    if (!mounted_) {
        uint32_t err = PicoSDCardEndpoint::mount_();
        if (err != FR_OK) {
            if (kLogSDCard) Log.logf("getcwd: mount error: %s (%d)\n", PicoSDCardEndpoint::strerr(err), err);
            return err;
        }
    }
    app_status.repeat(AppStatus::SDCARD_ACTIVE, 1); // Flash blue
    TCHAR* path_buffer = new TCHAR[512]; // A bit big for the stack, so use heap
    FRESULT fr = f_getcwd(path_buffer, 512);
    if (fr != FR_OK) {
        if (kLogSDCard) Log.logf("getcwd: f_getcwd error: %s (%d)\n", PicoSDCardEndpoint::strerr(fr), fr);
        delete[] path_buffer;
        return fr;
    }
    path.assign((char16_t*)path_buffer); // Convert the TCHAR path to std::u16string
    delete[] path_buffer;
    return FR_OK;
}

uint32_t PicoSDCardEndpoint::openfile(const std::u16string &name)
{
    app_status.repeat(AppStatus::SDCARD_ACTIVE, 1); // Flash blue
    FRESULT fr = f_open(&file_, (const TCHAR*)name.data(), FA_READ|FA_OPEN_EXISTING);
    if (fr != FR_OK) {
        if (kLogSDCard) Log.logf("openfile: f_open error: %s (%d)\n", strerr(fr), fr);
        return fr;
    }
    return FR_OK;
}

uint32_t PicoSDCardEndpoint::filesize() 
{
    return f_size(&file_); // Returns the size of the file in bytes
}

uint32_t PicoSDCardEndpoint::readfile(uint8_t *buffer, uint32_t size) 
{
    app_status.repeat(AppStatus::SDCARD_ACTIVE, 1); // Flash blue
    UINT bytes_read = 0;
    FRESULT fr = f_read(&file_, buffer, size, &bytes_read);
    if (fr != FR_OK) {
        if (kLogSDCard) Log.logf("readfile: f_read error: %s (%d)\n", strerr(fr), fr);
        return 0xffffffff;
    }
    return bytes_read;
}

uint32_t PicoSDCardEndpoint::closefile() 
{
    app_status.repeat(AppStatus::SDCARD_ACTIVE, 1); // Flash blue
    FRESULT fr = f_close(&file_);
    if (fr != FR_OK) {
        if (kLogSDCard) Log.logf("closefile: f_close error: %s (%d)\n", strerr(fr), fr);
        return fr;
    }
    return FR_OK;
}
