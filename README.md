
# Newton MessagePad 2x00 and eMate 300 USB Dongle Firmware

Welcome to the firmware repository for the Newton MessagePad 2x00 and eMate 300 USB dongle! This project brings Apple's iconic 1995 handheld PDA into the modern era by enabling seamless data transfer between the Newton devices and modern computers via a USB-C connection.

## What Does This Firmware Do?

This firmware powers a hardware dongle that acts as a bridge between the Newton's serial port and a USB-C port. It allows you to:
- Transfer files, packages, and data between your Newton device and a modern computer.
- Use tools like NCX, BasiliskII, and Inspector to interact with your Newton.
- Emulate Newton's docking protocol for advanced data handling.

The firmware is designed to handle high-speed communication reliably, supporting baud rates of up to 230,400 bps.

## Features

- **Serial-to-USB Communication**: Pipes data between the Newton's serial port and the USB port.
- **Error Handling**: Implements mechanisms to handle data loss and synchronization issues.
- **Flexible Event System**: Uses a modular, event-driven architecture for handling data and commands.
- **Support for Multiple Baud Rates**: Works with 38,400, 57,600, 115,200, and 230,400 bps.
- **Extensible Design**: Includes support for filters, buffers, and switches to customize data handling.
- **Future-Proof**: Designed to support additional features like SD card storage and advanced Newton protocols.

## How It Works

The firmware is built around a **pipe-based architecture**:
- **Pipes**: Modular components that process and forward events (e.g., data, commands).
- **Events**: Compact 32-bit messages that represent actions or data to be processed.
- **Filters**: Specialized pipes that modify or analyze data streams (e.g., Hayes command handling, MNP throttling).
- **Devices**: Hardware interfaces like UART (serial port) and USB endpoints.

This architecture ensures flexibility, scalability, and maintainability, making it easy to add new features or adapt to different hardware configurations.

## Current Status

- **Working**: Reliable data transfer at 115,200 bps. Successfully tested with NCX, BasiliskII, and Inspector.
- **In Progress**: Addressing occasional synchronization issues during large file transfers.
- **Planned**: Adding support for advanced features like SD card storage, statistics, and Newton Docking Protocol emulation.

## Getting Started

1. **Hardware Requirements**:
   - Raspberry Pi Pico or compatible RP2040-based board.
   - USB-C connection to your computer.
   - Serial connection to your Newton device.

2. **Building the Firmware**:
   - Clone this repository.
   - Follow the instructions in the `CMakeLists.txt` file to build the firmware using the Pico SDK.

3. **Flashing the Firmware**:
   - Use the Raspberry Pi Pico's bootloader to flash the firmware onto the device.

4. **Connecting Your Newton**:
   - Plug the dongle into your Newton's serial port and your computer's USB port.
   - Use your favorite Newton tools to start transferring data!

## Contributing

Contributions are welcome! Whether it's fixing bugs, adding features, or improving documentation, feel free to open a pull request or issue.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

---

Thank you for keeping the Newton alive! If you have any questions or need help, feel free to reach out.

