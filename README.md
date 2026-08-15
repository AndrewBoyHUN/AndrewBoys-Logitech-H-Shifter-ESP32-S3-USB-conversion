# AndrewBoy's Logitech H Shifter ESP32-S3 USB conversion

Native USB HID gamepad firmware for converting a Logitech Driving Force Shifter
(G29/G920/G923 shifter) into a standalone Windows USB game controller using an
ESP32-S3.

## What It Does

- Reads the Logitech shifter's analog X/Y position directly on the ESP32-S3 ADC.
- Reads the reverse signal on a digital input.
- Exposes a native USB HID gamepad to Windows.
- Maps each gear to exactly one HID button.
- Releases all buttons in neutral.
- Uses hysteresis and debounce so gear changes do not flicker while shifting.
- Keeps 115200 baud serial debug output enabled on the native USB CDC interface.

## Gear Mapping

| Shifter position | HID output |
| --- | --- |
| 1st gear | Button 1 |
| 2nd gear | Button 2 |
| 3rd gear | Button 3 |
| 4th gear | Button 4 |
| 5th gear | Button 5 |
| 6th gear | Button 6 |
| Reverse | Button 7 |
| Neutral | No buttons pressed |

Only one gear button is active at any time. When changing gears, the firmware
releases the previous gear button before pressing the new one.

## Bill of Materials

- ESP32-S3 development board
- DB9 male connector
- 6 Dupont wires
- Enclosure
- USB-C cable

## Wiring

The Logitech shifter uses a DE-9 connector. This project uses only the analog
X/Y axes, power, ground, chip select tied high, and the reverse data signal.

| Logitech DE-9 pin | DB9 male pin | Logitech signal | ESP32-S3 connection |
| --- | --- | --- | --- |
| Pin 2 | Male pin 2 | Reverse data | GPIO4 |
| Pin 3 | Male pin 3 | CS | 3.3V |
| Pin 4 | Male pin 4 | X axis | GPIO1 |
| Pin 6 | Male pin 6 | GND | GND |
| Pin 8 | Male pin 8 | Y axis | GPIO2 |
| Pin 9 | Male pin 9 | Power | 3.3V |

Pins 1, 5, and 7 are not connected.

## Calibration Measurements

These are the measured stable 12-bit ADC values from the tested shifter.
ADC range is 0-4095.

| Position | X center | X range | Y center | Y range | Reverse input |
| --- | ---: | ---: | ---: | ---: | ---: |
| Neutral | 2056 | 2055-2057 | 1976 | 1975-1977 | 0 |
| 1st | 957 | 956-958 | 3918 | 3915-3923 | 0 |
| 2nd | 924 | 923-925 | 347 | 347-351 | 0 |
| 3rd | 1982 | 1981-1983 | 4009 | 4006-4011 | 0 |
| 4th | 1901 | 1901-1903 | 309 | 308-311 | 0 |
| 5th | 2779 | 2777-2781 | 4095 | 4094-4095 | 0 |
| 6th | 2769 | 2767-2771 | 293 | 292-293 | 0 |
| Reverse | 2876 | 2874-2877 | 293 | 292-293 | 1 |

The reverse input on GPIO4 is active HIGH.

## Thresholds Used

X axis:

| X value | Column |
| ---: | --- |
| `< 1430` | Left column, gears 1/2 |
| `1430-2375` | Center column, gears 3/4 |
| `> 2375` | Right column, gears 5/6/Reverse |

Y axis:

| Y value | Row |
| ---: | --- |
| `< 1160` | Bottom row, gears 2/4/6/Reverse |
| `1160-2945` | Neutral band |
| `> 2945` | Top row, gears 1/3/5 |

Reverse logic:

| Position logic | Gear |
| --- | --- |
| Right + bottom + `REV=0` | 6th gear |
| Right + bottom + `REV=1` | Reverse |

## Filtering, Hysteresis, and Debounce

- ADC resolution: 12-bit, 0-4095.
- Analog filtering: 21 samples per reading, median selected.
- Reverse filtering: majority vote across the same 21 samples.
- X hysteresis: approximately +/-80 ADC counts.
- Y hysteresis: approximately +/-100 ADC counts.
- Gear debounce: 25 ms stable candidate state before applying a HID button
  change.

## Build and Upload

This project uses PlatformIO with the Arduino framework.

## Installation Tutorial

You do not need Codex to install this firmware. The easiest way is Visual Studio
Code with the PlatformIO extension.

### 1. Install the Tools

Install these on Windows:

- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO IDE extension for VS Code](https://platformio.org/install/ide?install=vscode)
- Git for Windows, if you want to clone the repository from the command line

The ESP32-S3 normally appears as an Espressif USB Serial/JTAG device. Windows 10
and Windows 11 usually install the driver automatically. If your board does not
show up as a COM port, install Espressif's USB/JTAG driver or use Espressif's
driver tools for your specific board.

### 2. Download the Project

Clone this repository:

```powershell
git clone https://github.com/AndrewBoyHUN/AndrewBoys-Logitech-H-Shifter-ESP32-S3-USB-conversion.git
cd AndrewBoys-Logitech-H-Shifter-ESP32-S3-USB-conversion
```

Or download the repository as a ZIP from GitHub and extract it.

### 3. Open It in VS Code

Open the extracted/cloned folder in VS Code. PlatformIO should detect
`platformio.ini` automatically.

If this is your first PlatformIO ESP32 project, the first build may take a while
because PlatformIO downloads the ESP32 platform, Arduino framework, compiler, and
upload tools.

### 4. Connect the ESP32-S3

Connect the ESP32-S3 to the PC with a USB data cable. Check Device Manager under
"Ports (COM & LPT)" and note the COM port.

Use the COM port that belongs to your own ESP32-S3 board. On Windows it will
look like `COM3`, `COM7`, `COM12`, etc. In the commands below this is written as
`COMxx`; replace it with your actual port number.

The port can change after flashing because the final firmware exposes native USB
HID plus a USB CDC debug interface.

### 5. Set the Upload Port

Open `platformio.ini` and set `upload_port` to your ESP32-S3 upload COM port:

```ini
upload_port = COMxx
```

If you want to use the serial debug monitor after flashing, set `monitor_port`
to the COM port that appears after the firmware is running:

```ini
monitor_port = COMxx
```

### 6. Build the Firmware

In VS Code, click PlatformIO's "Build" button, or run:

```powershell
pio run
```

The build should finish with `SUCCESS`.

### 7. Upload the Firmware

In VS Code, click PlatformIO's "Upload" button, or run:

```powershell
pio run -t upload
```

If upload works, the ESP32-S3 will reset and enumerate as a USB HID game
controller.

### 8. If Upload Does Not Start

Some ESP32-S3 boards do not automatically enter bootloader mode. If upload fails
with a connection error, try this:

1. Hold the `BOOT` button on the ESP32-S3.
2. Tap and release `RESET` while still holding `BOOT`.
3. Release `BOOT`.
4. Run `pio run -t upload` again.

After a successful upload, press `RESET` once if the board stays in bootloader
mode.

### 9. Verify in Windows

Open Windows "Set up USB game controllers" or run:

```powershell
joy.cpl
```

You should see a game controller/HID device. Depending on Windows' HID driver
view, it may appear as `HID-compliant game controller`; the USB composite product
string is `Logitech H-Shifter`.

Move the shifter through the gears and verify:

- 1st gear presses Button 1.
- 2nd gear presses Button 2.
- 3rd gear presses Button 3.
- 4th gear presses Button 4.
- 5th gear presses Button 5.
- 6th gear presses Button 6.
- Reverse presses Button 7.
- Neutral releases all buttons.

### 10. Optional Serial Debug Monitor

The firmware prints debug lines when the detected gear changes:

```text
X=0957 Y=3918 REV=0 GEAR=1
X=2056 Y=1976 REV=0 GEAR=N
X=2876 Y=0293 REV=1 GEAR=R
```

Replace `COMxx` with the USB CDC debug COM port that appears for your own
ESP32-S3 after flashing. Serial speed is `115200`.

Open the serial monitor at 115200 baud:

```powershell
pio device monitor --port COMxx --baud 115200
```

## Windows Verification

After upload, Windows detected the device as:

- `HID-compliant game controller`
- USB composite bus-reported product name: `Logitech H-Shifter`
- Native USB CDC debug interface: `USB Serial Device (COMxx)`

No vJoy, background PC software, or game-specific configuration is required by
the firmware itself.

## Important Notes

- This firmware is intended for ESP32-S3 native USB.
- The Logitech shifter is powered from 3.3V in this build.
- The 5th gear Y reading reached ADC full scale (`4095`) on the tested unit,
  but the gear separation is still very large and reliable with the thresholds
  above.
- If your shifter or ESP32-S3 board gives different ADC values, run a diagnostic
  calibration firmware first and adjust the thresholds in `src/main.cpp`.
