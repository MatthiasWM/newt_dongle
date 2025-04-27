# Newton MP2x00 and eMate serial port dongle

Theis git repo contains the plans and software for a little hardware dongle
that connects Apple's' 1995 handheld PDA Newton MessagePad MP2x00 and the
educational version sMate 300 to a USB-C port, allowing data transfer with 
modern computers.   

## Staus Report

The dongle can pipe data from the serial port to the USB port and back reliably
at 115200. Various test were done with NCX and BasiliskII and Inspector. 

There are still hiccups, resulting in a hanging connection, on large package 
transfers. I will need to test if that is dongle issue or a MessagePad issue.

I was able to transfer a 900k package at 38400 bps without errors. 

## Documentation will follow

## Various thoughts

### Timing

All devices are handles in a loop. The Newton typically communicates at 
38'400bps, but also allow 57.6, 115.2, and even 230.4kbps. The main loop
must handle at least 4'000, and at a maximum 24'000 characters per second.

The RP2040 uart supports a 32 bytes rx and tx fifo buffer. To avoid overflow,
the FIFO must be handled 125 to 750 times a second, or every 8000 or 1300 
microseconds. In the current implementation, I measured a maximum of 13 
microseconds in the CDC endpoint and 15 usec in the entire wheel.

Not however that writing to stdout messes this up tremendously and makes
the dongle lose data.

Talking about lost data. Even with HSKI and HSKO implemented, we still run out
of sync on larger files and screen dumps. I need code that triggers this
error reliably and more code to insert delays into the data transfer. 
