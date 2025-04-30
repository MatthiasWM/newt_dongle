
# Pico RP2040 based Newton Dongle

```
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
```
## My TODO List for PiPico

 - [x] Check if the current Workspace compiles and debugs on our current hardware
 - [x] Change install type to RAM
 - [x] Implement the base classes in `common/`
 - [x] Implement the derived classes for the Pico
 - [x] Implement pipes for data and control
 - [x] Get a minimal serial port forwader running
 - [x] Test the serial port forwarder
 - [ ] Implement data vs. command mode and a few AT commands in `common`
 - [ ] Test with script
 - [ ] Implement statistics
 - [ ] Clean up Workspace
 - [ ] Fill in missing documentation
 - [ ] Pipe should be able to hand block data in addition streaming data

 ## Notes

 - In `CMakeLists.txt`, we set `pico_set_binary_type(newt_dongle no_flash)`. 
    Delete this line to permanetly flash devices.
 - Set HSKO to 29 on XIAO, and to 22 on PiPico)
 - Apply for a USB Vendor ID: https://github.com/raspberrypi/usb-pid
 - Change USB Port name: https://sourcevu.sysprogs.com/rp2040/picosdk/files/src/rp2_common/pico_stdio_usb/stdio_usb_descriptors.c#tok356
 - `#if defined(PICO_RP2040) ...`
