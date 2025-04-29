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

### The Chirp

At a few points in the documentation, it was mentioned that the Newton
sends out "chirps" to let devices know that they are connected. These
chirps are a frequency sent out on HSKO when for example Dock is started.
Maybe we can use this for something?

### Pipes

The old system is unnecessarily complex, the source code grew out
of control, and bugs were introduced.

A new hope: the new system is based on pipes. The interface is as simple as
possible. A pipe offers a single method: `Result push(Event)` and an `out` 
member that points to the next pipe. The only job is to take incoming events,
push them in into the out pipe, or reject the event.

Communication *always* goes from a starting pipe all the way to the last out 
device. Exactly *one* event will be sent. An event can only be handled or 
rejected. If it is rejected, the previous pipe will (may?) resend the same
(or another) event in the next round of the wheel.
 
If there is no event to be sent, a NULL (IDLE) event is generated so that 
pipes can update states, buffer data, and generate addition data if needed.

There are a number of specialized pipes:

Device: represents a piece of hardware, commonly a serial port. The device can
connect as a receiver to the `out` port of a pipe, forwarding incoming events
to the underlying hardware device. The Wheel will regularly push a NULL (QUERY? 
IDLE? Other name?) event to the device. The device then generates an event based
on its hardware state, or just forwards the IDLE event.

Buffer: this pipe can buffer a number of commands that were pushed into the 
pipe, but rejected by the receiver. If the buffer is full, the buffer will 
reject incoming events. A buffered pipe can implement `delay` events for
data throttling.

Unidirectional Filters: a single pipe that can watch the data stream and add
and remove events. A data throttle filter for example could watch for CR 
characters and forward the CR, followed by a Delay event, so a physical carriage
return has enough time to be performed.

Bidirectional filter: two pipes that can be inserted between devices. A Hayes
filter for example could watch for the `+++` sequence and return the `OK`
response on the second pipe, back to the original device.

Switches: can have even more inputs and outputs. For our dongle, we can have
a switch that connects to the UART, the USB, and the SDCard at the same time.
Depending on a global setting (SOURCE event), mnp Dock blocks are forwarded 
either to USB for a live connection, or to the SDCard for the built-in NCT
emulation.

Events: are `uint32_t` where the lowest eight bits are the type of event, and the
upper 24 bit are data. The data format depends on the type. Events that need 
little or no additional data should be bundled into a main event type and
sub-events that are stored in the data field.
 
All unknown events must be forwarded unchanged to `out`. 

Bit 7 (rush) in the Type indicates if events must be queued (0), or if they 
should skip the queue all the way to the receiver (1). A request to FLUSH the 
buffer would be such an event. 

If an event needs more than 24 bit, it can request a larger buffer and store 
the index. Do we need to manage buffers?

Results: follow the same pattern as events. All unknown result codes from `out`
should be returned unchanged to `push()`. Result codes can be used to make 
direct queries into the pipe and back.


Example Filters:
- Hayes filter: this filter watches for `pause-"+++"-pause` to switch into Hayes
  modem emulation mode, and `ATO\n` to switch back into data mode.
- MNP Throttle: used to give receivers time to process the CRC at the end of 
  an MNP block (inserts a DELAY event after `0x10, 0x03, CRC0, CRC1`)
- Log: write all commands that pass through to memory or a serial debug line
- Newton Docking Protocol: a rather complex bidirectional filter that reads
  and understands the MNP blocks of the Dock protocol and generates events
  to read directories and file data. A storage device could then understand
  DIR and READ events and return the requested data from an SDCard in 
  its out pipe.
