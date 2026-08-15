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

## Wiring

The Logitech shifter uses a DE-9 connector. This project uses only the analog
X/Y axes, power, ground, chip select tied high, and the reverse data signal.

| Logitech DE-9 pin | Logitech signal | ESP32-S3 connection |
| --- | --- | --- |
| Pin 2 | Reverse data | GPIO4 |
| Pin 3 | CS | 3.3V |
| Pin 4 | X axis | GPIO1 |
| Pin 6 | GND | GND |
| Pin 8 | Y axis | GPIO2 |
| Pin 9 | Power | 3.3V |

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

```powershell
pio run
pio run -t upload
```

The project was uploaded to the tested ESP32-S3 using:

- Upload port: `COM31`
- Debug/Serial Monitor port after HID firmware upload: `COM32`
- Serial speed: `115200`

To open the debug monitor:

```powershell
pio device monitor --port COM32 --baud 115200
```

The firmware prints a debug line when the debounced detected gear changes:

```text
X=2056 Y=1976 REV=0 GEAR=N
X=0957 Y=3918 REV=0 GEAR=1
X=2876 Y=0293 REV=1 GEAR=R
```

## Windows Verification

After upload, Windows detected the device as:

- `HID-compliant game controller`
- USB composite bus-reported product name: `Logitech H-Shifter`
- Native USB CDC debug interface: `USB Serial Device (COM32)`

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
