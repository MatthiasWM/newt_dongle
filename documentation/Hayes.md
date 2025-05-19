# Hayes Command Set

This firmware supports the Hayes protocol ("AT commands") on both the Interconnect Port and the USB port.

To switch from **data mode** to **Hayes command mode**, follow these steps:
1. Do not send any characters for at least 1 second.
2. Type three `+` characters (`+++`) with no more than 1 second beteen them.
3. Do not send anything again for 1 second.

The device will then switch to Hayes command mode and reply with `OK⮐`.  
To return to **data mode**, type `ATO⮐`.

## Supported Commands

| Command | Description                          |
|---------|--------------------------------------|
| `+++`   | In data mode: enter Command Mode     |
| `ATO`   | Switch back to data mode             |
| `ATI`   | Display device information           |

## Setting and reading Registers

| Command  | Description                                 |
|----------|---------------------------------------------|
| `ATS12`  | escape code guard time (1/50th of a second) |
| `ATS300` | absolute MNP throttle delay in microseconds |
| `ATS301` | relative MNP throttle delay in characters   |


### SD Card debugging Commands

| Command    | Description                                      |
|------------|--------------------------------------------------|
| `AT[DS`    | Disk Status (NOINIT, NODISK, PROTECT)            |
| `AT[DI`    | Disk Initialize (NOINIT, NODISK, PROTECT)        |
| `AT[GL`    | Get Label                                        |

### Commands in Development

| Command    | Description                                      |
|------------|--------------------------------------------------|
| `ATS2=n`   | Set a new escape character (default is `+`)      |
| `ATS12=n`  | Set escape code guard time (1/50th of a second)  |
| `ATZ`      | Reset Hayes settings                             |
| `AT%En`    | Set escape method (`+++`, DTR, etc.)             |
| `AT&F0`    | Restore factory settings                         |
| `AT&K0`    | Set flow control (0=none, 1=hardware)            |
| `AT&W0`    | Save settings to NVRAM                           |

### Future Plans

#### MNP Control
- Implement package throttling.

#### Crossbar
- **Downstream**: Select between Serial and Modem.
- **Upstream**: Select between USB, SD, Wi-Fi, or Bluetooth.

#### Wi-Fi
- Add Wi-Fi settings (e.g., find networks, set SSID, and password).
- Support Serial and PPP communication over Wi-Fi.

#### Bluetooth
- Add Bluetooth settings (e.g., connect, set passcode).
- Support Serial communication over Bluetooth.


