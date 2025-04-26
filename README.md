# Newton MP2x00 and eMate serial port dongle

Theis git repo contains the plans and software for a little hardware dongle
that connects Apple's' 1995 handheld PDA Newton MessagePad MP2x00 and the
educational version sMate 300 to a USB-C port, allowing data transfer with 
modern computers.   

## Documentation will follow

## Various thoughts

### Timing

All devices are handles in a loop. The Newton typically communicates at 
38'400bps, but also allow 57.6, 115.2, and even 230.4kbps. The main loop
must handle at least 4'000, and at a maximum 24'000 characters per second.

The RP2040 uart supports a 32 bytes rx and tx fifo buffer. To avoid overflow,
the FIFO must be handled 125 to 750 times a second, or every 8000 or 1300 
microseconds. We can use interrupts to help us stay in sync.

If we can't guarantee that, the uart can also use DMA to write directly to 
memory and buffer a lot more data. This requires code that is much closer
to the hardware and not portable. Let's wait until all alternatives are 
researched.

---


The uart can set HSKI to 0 to suspend the data stream from the Newton.  
Do we need to call irq_set_enabled(UART_IRQ, true);

 32×8 Tx and 32×12 Rx FIFOs
 
 framing, parity, or break error are stored in the fifo! 
 
 Individually-maskable interrupts from the receive (including timeout), transmit, modem status and error conditions
 
 https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf pg. 417

The UART provides an interface to connect to the DMA controller as UART DMA interface in Section 4.2.5 describes.

 disable control flow
