
# My TODO List for ESP32

## Initial steps:

 - [ ] List the SP devices that we want to support (Internal, Debugging, Dongle board)
 - [ ] Make it easy to switch between devices (lcan we do CDC/MSC on all devices?)
 - [ ] Check if the current Workspace compiles and debugs on our current hardware
 - [ ] Change install type to RAM (ESP?)
 - [ ] Implement the base classes in `common/` for USB to Serial (also in PiPico)
 - [ ] Implement the derived classes for the ESP
 - [ ] Get a minimal serial port forwader running
 - [ ] Test the serial port forwarder
 - [ ] Implement data vs. command mode and a few AT commands in `common` (also in PiPico)
 - [ ] Test with script
 - [ ] Implement statistics (also in PiPico)
 - [ ] Clean up Workspace!

## Basic hardware support:

 - [ ] Get SD Card support going
 - [ ] add modem endpoint basics
 - [ ] add BLE endpoint basics
 - [ ] add Wifi endpoint basics

