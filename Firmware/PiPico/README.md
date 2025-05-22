```
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
```

# PiPico configuration

This is the build configuration for running the NewtCOM firmware on a 
PiPico developer card with an RP2040 CPU. The firmware installes to RAM
for speed and to avoid Flash use.

Parallel to this configuration is the NewtCOM setup for compiling the 
same firmware directly for the RP2040 based dongle.

## My TODO List for PiPico

 - [x] Check if the current Workspace compiles and debugs on our current hardware
 - [x] Change install type to RAM
 - [x] Implement the base classes in `common/`
 - [x] Implement the derived classes for the Pico
 - [x] Implement pipes for data and control
 - [x] Get a minimal serial port forwader running
 - [x] Test the serial port forwarder
 - [x] Implement data vs. command mode and a few AT commands in `common`
 - [x] Implement a DELAY event (absolute time, number of bytes, flush FIFO)
 - [x] Implement a MNP scanner that can insert delays after a block is received
 - [x] Test with script
 - [x] Clean up Workspace
 - [ ] UserInterface class to control LEDs and listen to buttons
 - [ ] Optional firmware to reset Flash memory? Or any other way to reset it?
 - [ ] MNP Filter that extracts Dock commands and data (takes care of negotiation, blocks, replies, checksums, resends)
 - [ ] Matrix, a filter that routes data between multiple sockets
 - [ ] Dock, a filter that understands and handles Newton Dock commands
 - [ ] Async class for all things that take more time than we have in a slice
    - [ ] Async log: logging via debugging serial port or to SD Card
    - [ ] Async SD Card data and directory reading
 - [ ] Fill in missing documentation
 - [ ] Pipe could be able to handle block data in addition streaming data
 - [ ] Verify that we actually run on a PiPico, and not a NewtCOM dongle

 ## Notes

 - In `CMakeLists.txt`, we set `pico_set_binary_type(newt_dongle no_flash)`. 
    Delete this line to permanetly flash devices.
 - Set HSKO to 29 on XIAO, and to 22 on PiPico)
 - Apply for a USB Vendor ID: https://github.com/raspberrypi/usb-pid
 - Change USB Port name: https://sourcevu.sysprogs.com/rp2040/picosdk/files/src/rp2_common/pico_stdio_usb/stdio_usb_descriptors.c#tok356
 - `#if defined(PICO_RP2040) ...`

 ## Hardware

 The pack is built with 10.5mm long pin headers, giving a top-bottom distance
 of 9.1 to 9.5mm. The overall hight is 12.5mm without USB and 13.9mm with USB
 connector. 
 
 Tip of the plug to front edge of PCBs is 10.4mm. Length over all is 31mm and
 32.5mm including the USB port.

 Width is 17.8mm. 

 The original dongle is 21.4 to 24.7mm wide, 20mm tall, and 39.4mm long 
 with Interconnect and 32,4mm without.

## Staus Report

The dongle can pipe data from the serial port to the USB port and back reliably
at 115200. Various test were done with NCX and BasiliskII and Inspector. 

### Timing

All devices are handles in a loop. The Newton typically communicates at 
38'400bps, but also allows 57.6, 115.2, and even 230.4kbps. The main loop
must handle at least 4'000, and at a maximum 24'000 characters per second.

The RP2040 uart supports a 32 bytes rx and tx fifo buffer. To avoid overflow,
the FIFO must be handled 125 to 750 times a second, or every 8000 or 1300 
microseconds. In the current implementation, I measured a maximum of 13 
microseconds in the CDC endpoint and 15 usec in the entire wheel.

Note however that writing to stdout messes this up tremendously and makes
the dongle lose data.

Talking about lost data. Even with HSKI and HSKO implemented, we still run out
of sync on larger files and screen dumps. I need code that triggers this
error reliably and more code to insert delays into the data transfer. 

### The Chirp

At a few points in the documentation, it was mentioned that the Newton
sends out "chirps" to let devices know that they are connected. These
chirps are a frequency sent out on HSKO when for example Dock is started.
Maybe we can use this for something?

### Pipes

(move this chapter elsewhere)

This is a firmware for a microcontroller that connects an Apple Newton 
MessagePad (MP2x00, eMate300) to the modern outside world over USB-C.
Wifi and Bluetooth are planned, but not implemented yet.

This code sees devices like the serial port and USB es Endpoints. An endpoint
can send Events to another enpoint using Pipes. Pipes generally unidirectional.
Connecting two endpoints will usually require two pipelines, one in each 
direction.

Pipe::send() will pass Events from the sending Endpoint's output to the 
reciving Endpoint. The reciver will either handel the event and return OK, 
or reject the event, expecting the sender to send the same event again in a 
later cycle of the scheduler. These events may be buffered along the way,
but will always stay in order.

Endpoints can bypass buffers in the pipeline using Pipe::rush() and will
immediatly receive an reply to the event. This will change the order of
events.

Lastly, pipes can send events up the pipe using Pipe::rush_back(). This is 
currently used by a receiving andpoint to give the sender hints for serial
port handshaking when buffers run full.

Pipes can have additional functionality. They can buffer events, or split
events into two streams which is great for logging traffic. Pipes can filter
events and add new events. The MNP Throttle looks for specifc sequences and
adds delays in the line to give the receive time to recover.

More complex filters can combine and upstream and a downstream pipe. The Hayes
interpreter is such a device. It understands Hayes 'AT' commands and can
reroute data as needed.

