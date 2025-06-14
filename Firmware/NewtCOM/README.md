```
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
```

# NewtCOM Dongle configuration

This is the build configuration for running the NewtCOM firmware on a 
NewtCOM Dongle with an RP2040 CPU. The firmware installes to Flash memory.

Parallel to this configuration is the PiPico setup for compiling the 
same firmware for the PiPico developer board.

## My TODO List for NewtCOM Dongle

see: [../PiPico/README.txt](../PiPico/README.txt)

Also:
 - [ ] User Documentation, reference to the resources that we use

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

The dongle can pipe data from the serial port to the USB port and back reliably at 115200. Various test were done with NCX and BasiliskII and 
Inspector.

The Dock protocol is implemented well enough so the Newton can establish
a connection to an emulated NCU, browse files and directories on the 
SDCard, and download and install them.
