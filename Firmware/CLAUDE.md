# Newton Dongle Firmware

USB-C dongle for original MessagePads and eMates. Two modes, selected by the
USB CDC DTR line (`DTRSwitch`):

- **Direct connect** (DTR high): USB CDC <-> Newton serial port. MNP blocks
  from the host are passed through; `MNPThrottle` inserts a pause after each
  block, and the UART honours the Newton's handshake line (HSKO).
- **Standalone** (DTR low): the dongle itself speaks MNP (`MNPFilter`) and the
  Newton Dock protocol (`Dock`), so the Newton can browse the microSD card and
  install `.pkg` files.

## Layout

- `common/` – platform independent code: `Event`, `Pipe`, `Task`, `Endpoint`,
  `Scheduler`, filters (`Hayes`, `MNP`, `DTRSwitch`), pipes (`BufferedPipe`,
  `MNPThrottle`, `Tee`), `Dock`, `Newton/NSOF`, `Newton/DESKey`.
- `Systems/RP2040/` – Pico implementations (UART, CDC, SD card, flash
  settings, async log on core 1, status LED).
- `Systems/Posix/`, `CommandLine/` – host builds (useful for unit tests).
- `Systems/ESP32*` – early experiments, not in scope right now.
- `PiPico/` – the RP2040 build. `PiPico/common` and `PiPico/RP2040` are
  symlinks to `../common` and `../Systems/RP2040`.
- `PiPico/main.cpp` – all objects are globals; the pipe graph is wired there.
- `PiPico/main.h` – pin map and the `kLog*` / `kDebug*` switches.

## Build

```sh
cd Firmware/PiPico
~/.pico-sdk/ninja/v1.12.1/ninja -C build
```

`NewtCOM/` builds the ROM image for the dongle hardware the same way. Binary
type is `no_flash` (runs from RAM). Flashing and hardware tests are done by
Matthias; Claude cannot run code on the dongle. C++17, exceptions and RTTI are
enabled. The version number lives in `common/version.h` only.

Host unit tests (macOS, AddressSanitizer + UBSan; `char` is signed here):

```sh
cd Firmware/Tests
cmake -B build -G Ninja && cmake --build build && ./build/tests
```

New `Tests/test_*.cpp` files are picked up automatically. `Tests/main.h`
stands in for the platform `main.h`. Test data comes from real captures where
possible (`PiPico/unused.cpp` has a full Newton <-> NCU session).

## Working agreement

- Work through the job list below **in order, one job at a time**.
- Before changing code, explain the problem and the planned fix. After the
  change, walk through the diff so every line is understood.
- Keep each job a small, self-contained diff. No drive-by refactoring.
- The firmware must build without new warnings after every job.
- Where possible, add a host test that fails before the fix and passes after.
- **Never commit or push without explicit approval.** One job = one commit.
  Tick the job's checkbox in this file in the same commit.
- Match the surrounding style: `nd` namespace, `snake_case`, trailing `_` on
  members, Doxygen `\brief` comments, `if (kLogXxx) Log.logf(...)` logging.
  Note that `Dock.cpp` and `MNPFilter.cpp` use tabs in places; keep the file's
  existing indentation.
- Priority right now is a **bug free RP2040 implementation**. ESP32 work waits
  until the hardware is chosen (see the end of this file).

---

## Jobs

Each job lists the problem, where it is, the intended fix, and how to verify.
Line numbers drift, so jobs refer to functions.

### Phase 0 – Test harness

- [x] **0.1 Host unit test target.**
  Add a small host CMake target (e.g. `Firmware/Tests/`) that compiles
  selected `common/` files against a stub `main.h` (stub `Log`, all `kLog*`
  flags false). Start with `Newton/NSOF.cpp` and the MNP CRC. No framework
  needed; plain `assert`/`printf` is fine.
  *Verify:* `cmake -B build && cmake --build build && ./build/tests` on macOS.

### Phase 1 – Confirmed bugs, small fixes

- [x] **1.1 NSOF: received symbols and precedents are broken.** (confirmed)
  Three bugs: (a) `Ref(sym)` / `Ref(precedent_[index])` pass a `const
  Object*`, which silently converts to `Ref(bool)`; (b) the symbol parser
  reads one character too few and refuses the last byte of the buffer;
  (c) `Symbol::known_symbols_` is initialized *after* the global symbols
  registered themselves, which erases the list (verified in the RP2040
  binary), so `Symbol::find()` always returned `symUnknown`.
  *Fix:* `Ref::Share()` adds a counted reference; deleted `Ref(const void*)`
  turns any future pointer mix-up into a compile error; symbol loop fixed;
  registry is a function-local static.
  *Verify:* `Tests/test_nsof.cpp` decodes and re-encodes the captured `dinf`.

- [x] **1.2 MNP CRC uses `const char[]`.**
  `update_crc16()` in `MNPFilter.cpp` indexes the table with a possibly signed
  `char`. Correct on ARM by luck, wrong (and out of bounds) on x86 hosts.
  *Fix:* use `const uint8_t*`.
  *Verify:* `Tests/test_mnp.cpp` replays the captured session start; our LR,
  LAs and LT must match NCU byte for byte.

- [x] **1.3 `Dock::task()` uses `data` after `data_queue_.pop()`.**
  `data` is a reference to `front()`; `free_after_send_`/`bytes_` are read
  after the pop. *Fix:* copy the fields (or delete) before popping.
  *Verify:* build; install a package on hardware.

- [x] **1.4 `PicoUARTEndpoint::set_bitrate()` does not call the base class.**
  `UARTEndpoint::bitrate()` stays at 38400, so `DELAY`/`CHARS` events are
  computed with the wrong rate. *Fix:* call `UARTEndpoint::set_bitrate()`.

- [ ] **1.5 `sd_get_by_num()` / `spi_get_by_num()` use `<=`.**
  Off by one in `PicoSDCard.cpp`; `spi_get_by_num` also uses `sd_get_num()`.

### Phase 2 – MNP robustness (standalone/Dock mode)

- [ ] **2.1 Clean up all MNP state on disconnect and on a new LR.**
  `MNPFilter::set_disconnected()` is a stub. `ack_pending_frame_`,
  `active_frame_`, `in_frame`, both job queues, `out_frame_`, `seq_no_` and
  the frame pool survive. After a session ends with an unacknowledged LT, the
  next session can't send anything. *Fix:* reset both pipes and release all
  frames. Also call it when DTR switches modes.
  *Verify:* on hardware, cancel docking in the middle of a package install,
  then connect again without replugging.

- [ ] **2.2 Detect duplicate LT frames from the Newton.**
  `NewtToDockPipe::handle_newt_frame()` accepts any sequence number. If our LA
  is lost, the Newton resends and the Dock gets the payload twice (the "State
  out of sync" log in `Dock.cpp`). *Fix:* only accept `last + 1`; for a
  duplicate, send the LA again and drop the data.
  *Verify:* host test feeding the same LT twice.

- [ ] **2.3 Retransmit timer for our LT frames.**
  There is no timeout while waiting for an LA. A lost frame stalls the session
  forever. *Fix:* timer started after the last CRC byte; resend on timeout,
  give up after N retries with LD and `MNP_DISCONNECTED`. Needs a time source
  (see 4.1 – do 4.1's `Scheduler::now_us()` first if convenient).
  *Verify:* host test with a dropped frame; on hardware, pull the cable
  during a transfer.

- [ ] **2.4 Bound and resynchronise MNP input.**
  `WAIT_FOR_DATA` grows `data` without limit. `SYN SYN DLE STX` is not
  recognised; a `DLE STX` inside a frame (lost ETX) is not treated as a new
  frame start. A header size of 0x10 isn't un-escaped on input.
  *Fix:* cap data at the negotiated max (256), abort on overflow, restart on
  `DLE STX`, tolerate repeated SYN.
  *Verify:* host tests with noise, truncated frames, and back to back frames.

### Phase 3 – Dock robustness

- [ ] **3.1 Cap the Dock payload size and resync on frame start.**
  `Dock::send()` trusts the 32-bit length. After a desync, random bytes become
  a huge size: the parser waits in state 16 forever and `in_data_` grows until
  the heap is gone. *Fix:* cap the size (e.g. 64 KB); if the cap is exceeded,
  or `MNP_FRAME_START` arrives while the header is half parsed, reset to
  state 0. Also re-check `'n'` after a failed match so `nnewtdock` resyncs.

- [ ] **3.2 Bounds checks on received payloads.**
  `kDNewtonInfo` reads `in_data_[4..11]`, `kDResult` logs `in_data_[3]`,
  without checking the size.

- [ ] **3.3 One command builder instead of static buffers.**
  `send_cmd_dres`, `send_cmd_dock`, `send_cmd_wicn` and the `lpkg` header
  write into a `static std::vector` and queue a pointer to it; a second
  command queued before the first is sent overwrites it. The header + NSOF +
  padding + size code is also copied three times.
  *Fix:* one helper, e.g. `queue_command(fourcc, payload, start, end)`, that
  owns its buffer. Behaviour must stay byte for byte identical.
  *Verify:* host test comparing the generated bytes with the current tables.

- [ ] **3.4 Package transfer error handling.**
  - A short SD read in `send_package_task()` sends zero filled data → corrupt
    install. Cancel the transfer and report an error instead.
  - `MNP_DISCONNECTED` during `lpkg` leaves the file open; close it in
    `reset_()`.
  - `handle_GetFileInfo()` ignores `openfile()` errors.
  - Use the Dock error codes for file errors (`kDockErrBrowserFileNotFound`
    -28105 etc., already listed in `Dock.h`) instead of -48403.

- [ ] **3.5 Password protected Newtons.**
  The password is hard-coded to empty. At minimum reply with a clear error
  (`kDockErrBadPasswordError`); optionally store a password in user settings
  (`AT` command). Discuss before implementing.

### Phase 4 – Throttling and serial flow control (direct connect mode)

- [ ] **4.1 Time based TX delay.**
  `PicoUARTEndpoint::send()` subtracts `cycle_time` on every send attempt.
  `BufferedPipe` calls it from both `task()` and `send()`, so the pause runs
  up to twice as fast under load and depends on the cycle time.
  *Fix:* add `Scheduler::now_us()`; on `DELAY`, wait until the UART is idle,
  then set `deadline = now + usec` and reject until it has passed.
  *Note:* the ATS300/ATS301 defaults were tuned against the old behaviour and
  will need re-tuning on hardware.

- [ ] **4.2 Start the pause when the line is idle, not when the FIFO is empty.**
  Use `UART_UARTFR_BUSY_BITS` instead of `TXFE`; otherwise the last CRC byte
  (~260 µs at 38400) overlaps the pause.

- [ ] **4.3 Keep the TX FIFO short for software CTS.**
  HSKO is checked per byte, but up to 32 bytes sit in the hardware FIFO and
  still go out after the Newton drops CTS. *Fix:* only write when the FIFO is
  (nearly) empty, e.g. `TXFE`, or a low FIFO level via `UARTIFLS`/`UARTRIS`.
  Measure throughput at 38400 and 115200 afterwards.

- [ ] **4.4 Make `HIGH_WATER` work.**
  Three bugs that together make it a no-op:
  `BufferedPipe` builds `Event{HIGH_WATER, 1}` (data, not subtype ON/OFF);
  `space()` returns the *used* count; and the event goes to `out()`
  (downstream) instead of upstream via `in()`. Once fixed, `buffer_to_cdc`
  filling up drops HSKI so the Newton stops sending.
  *Verify:* host test of `BufferedPipe` thresholds; on hardware, stop reading
  the USB port while the Newton sends.

- [ ] **4.5 Don't lose the host baud rate.**
  `PicoCDCEndpoint::device_changed_bitrate()` sends `SET_BITRATE` downstream,
  but `ToCDCPipe::send()` drops it while DTR is low, and a rejected event is
  lost (existing TODO). *Fix:* remember the last host bitrate and apply it to
  the UART when DTR goes high; retry when rejected. The host baud rate must
  always reach the Newton UART in direct connect mode.
  *Verify:* NCX/NTK at 38400 and 115200, with apps that set the rate before
  and after raising DTR.

- [ ] **4.6 UART receive: drain the FIFO and count errors.**
  `PicoUARTEndpoint::task()` reads one byte per scheduler cycle and
  `uart_getc()` discards overrun/framing errors. A slow cycle (SD mount,
  directory listing) can overflow the 32 byte RX FIFO unnoticed.
  *Fix:* IRQ (or per task call drain) into a ring buffer; count OE/FE/BE and
  make the counters visible (AT command or log).

- [ ] **4.7 Measure worst case scheduler cycle time.**
  Track the maximum `cycle_time` and show it next to the UART error counters.
  Decide afterwards whether SD access must move out of `Dock::send()` (it
  currently runs inside the UART receive call chain).

### Phase 5 – Code quality (no behaviour change)

- [ ] **5.1 Compiler warnings.** Build `common/` with `-Wall -Wextra` and fix
  the findings (printf formats with `uint32_t`, `Log.logf` with unused
  arguments, unused variables, signed/unsigned compares). Also look for
  `Log.log("...%d", value)`: the second argument is the *pipe*, so the value
  is never printed (e.g. `handle_newt_frame()` default case in MNPFilter).
- [ ] **5.2** Make `Result::OK` etc. `const` (or `constexpr`).
- [ ] **5.3** `ND_FOURCC`: use `__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__`;
  today, if both macros are undefined, `0 == 0` silently picks little endian.
- [ ] **5.4** Replace `new` sub-pipes in `DTRSwitch` / `MNPFilter` with
  members; remove stale comments (e.g. `BufferedPipe` buffer sizes).
- [ ] **5.5 Dependency injection.** `common/` reaches into the platform's
  `main.h` for `Log`, `user_settings`, `sdcard_endpoint`, `app_status`. Pass
  an SD card interface into `Dock` (and similar) via the constructor. This is
  the main preparation for a second platform and makes `Dock` host-testable.
- [ ] **5.6** More host tests: Dock parser and a full scripted docking session
  (rtdk → dock → name → dinf → … → lpkg) against a fake SD card.

### Phase 6 – Later / optional

- [ ] **6.1 MNP window > 1.** Currently stop and wait (k=1 in our LR).
  A larger window would speed up package installs. Only after 2.1–2.4.
- [ ] **6.2 Generic frame gap throttle.** Turn `MNPThrottle` into a throttle
  with a pluggable end-of-frame detector (MNP `DLE ETX CRC CRC`, PPP `0x7E`).
- [ ] **6.3 Modem emulation with PPP** (WiFi): `HayesFilter` already answers
  `ATDT` with `CONNECT`; then lwIP PPPoS in server mode plus NAT.

---

## Next generation hardware (open decision, not a job yet)

Requirements: USB CDC **with** host baud rate and DTR, **serial over
Bluetooth** (Classic SPP, so Mac/PC see a normal serial port), WiFi for PPP,
microSD, small module with a working antenna.

Findings so far (verify against datasheets before deciding):

- ESP32-C3, C6, C5, H2, S3: **BLE only**, no Classic Bluetooth, so no SPP.
- ESP32-C3/C6: USB is the fixed function USB-Serial/JTAG controller; as far
  as we know, no line coding (baud) callback, and DTR/RTS are used for
  auto-reset. Does not meet the USB requirement.
- ESP32-S3: full USB OTG (TinyUSB, baud rate + DTR work), but BLE only.
- Original ESP32: WiFi + Classic BT (SPP) + BLE, but no native USB.
- So no single ESP32 meets all requirements. Options:
  1. **RP2040/RP2350 + Infineon CYW43439** (Pico W / Pico 2 W, or the
     Raspberry Pi RM2 radio module): WiFi + Classic BT via BTstack
     (`pico_btstack_classic`, SPP examples exist) + lwIP in the Pico SDK.
     Keeps all existing RP2040 code and USB. Check RM2 availability, size and
     antenna certification.
  2. RP2040 (USB, SD, Newton UART) + original ESP32 module as a WiFi/BT
     coprocessor over a second UART.
  3. ESP32-S3 with a custom BLE app on the host side (drops the "normal
     serial port" requirement).
