
# Command Line based Newton Dongle test suite

```
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
```
## My TODO List for Test

 - [ ] Build from scratch
 - [ ] continue with Result BufferedPipe::send(Event event)
 - [ ] Implement the ring buffer
 - [ ] Return in subytype if the high water mark is reached
 - [ ] Implement the delay functionality
 - [ ] Rename the new Device class back to Endpoint
 - [ ] Serialize the Log Endpoint to run better on Pico (just buffer it?)
 - [ ] Better Log output
 - [ ] Implement the UART endpoint API
 - [ ] (Re)Implement the Pico UART and Pico CDC classes

 ## Notes

 - We must somehow make a Log class available to everything in Common
 - We need a list of functions that must be implemented on all
   platforms, i.e. the microsecond timestamps that are the base for our delay feature
