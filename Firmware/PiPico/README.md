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
 - [ ] Rename `send()` to `relay()` to make it clearer what the function does?
 - [x] UserInterface class to control LEDs and listen to buttons
 - [ ] Optional firmware to reset Flash memory? Or any other way to reset it?
 - [x] MNP Filter that extracts Dock commands and data (takes care of 
       negotiation, blocks, replies, checksums, resends)
 - [x] Switching pipe between Dock and CDC based on CDC DTR signal
 - [x] Dock, a filter that understands and handles Newton Dock commands
    - [x] Rough first implementation
    - [x] Functional Reset for multiple connections
    - [x] Browse and donload packages from SD Card
    - [ ] Full Error handling an documentation
 - [ ] Async class for all things that take more time than we have in a slice
    - [x] Async log: logging via debugging serial port or to SD Card
    - [ ] Async SD Card data and directory reading
 - [ ] Fill in missing documentation
 - [ ] Pipe could be able to handle block data in addition streaming data
 - [ ] Verify that we actually run on a PiPico, and not a NewtCOM dongle

 ## Notes

 - In `CMakeLists.txt`, we set `pico_set_binary_type(newt_dongle no_flash)`.
   This is to keep wear on the Flash low and to make debugging iterations fast.
 - Do we need to apply for a USB Vendor ID: https://github.com/raspberrypi/usb-pid ?
 - Change USB Port name: https://sourcevu.sysprogs.com/rp2040/picosdk/files/src/rp2_common/pico_stdio_usb/stdio_usb_descriptors.c#tok356

## Hardware

The Developer Board is built around a PiPico board on a Breadboard with flying 
wires/ It features the Newton port and inverter, the SDCard socket, and the 
RGB LED of the XIAO board. The second LED is not supported. The BOOT button
is missing, and we don't care about the DC/DC inverter here.

## Staus Report

The dongle can pipe data from the serial port to the USB port and back reliably
at 115200. Various test were done with NCX and BasiliskII and Inspector.

The DOck is implemented in a very rough way, but does allow browsing of the 
SDCard and installing ppackages.

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
currently used by a receiving endpoint to give the sender hints for serial
port handshaking when buffers run full.

Pipes can have additional functionality. They can buffer events, or split
events into two streams which is great for logging traffic. Pipes can filter
events and add new events. The MNP Throttle looks for specifc sequences and
adds delays in the line to give the receive time to recover.

More complex filters can combine and upstream and a downstream pipe. The Hayes
interpreter is such a device. It understands Hayes 'AT' commands and can
reroute data as needed.



ChatgPT Name Ideas:

Here are some name suggestions for your Newton connector project, balancing retro charm, modern utility, and uniqueness:

---

### 🔌 **Retro-inspired + Functional**

1. **NewtonLink** – Clean, modern, sounds official.
2. **NewtBridge** – Emphasizes bridging old and new.
3. **NewtPort** – Evokes both serial/USB ports and "support".
4. **Connecton** – A twist on "Connection" + Newton.
5. **NewtNode** – Feels modern and networky.

---

### 🧠 **Quirky & Nostalgic**

1. **The Dongle** – A cheeky nod to Apple’s own term.
2. **NewtDongle** – Literal and fun.
3. **AppleCore** – A deep pun: core of Apple + computing.
4. **MessageBridge** – Homage to the Newton MessagePad.
5. **Gravitas** – The codename of the original Newton dongle.

---

### 🚀 **Modern & Techy**

1. **NewtonSyncer** – Self-descriptive and familiar.
2. **NewtFlash** – Hints at both speed and internal storage.
3. **RetroLink-C** – Adds USB-C flair.
4. **LegacyBridge** – Sums up its purpose clearly.
5. **PDA2USB** – A bit utilitarian, but SEO-friendly.

---

### 🧩 **Names with layered meaning**

1. **ArkPad** – A vessel to carry legacy data into the future.
2. **Newtonian** – Sounds techy, plays off Isaac Newton.
3. **TimeDongle** – Nostalgic, portable, like a time capsule.
4. **NewtCradle** – Evokes sync cradles from the 90s.
5. **CoreLink** – Hints at depth, legacy, and modern connection.

---

Would you like:

* Names available as `.com` domains?
* Logo ideas for the top picks?
* A poll to narrow down tone (playful, pro, nostalgic, etc.)?

Let me know your preferences and I can refine!
