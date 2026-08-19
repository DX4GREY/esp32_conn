# RF24 Suite — ESP32-S3 Dual nRF24 2.4 GHz Analyzer

A standalone firmware project for an ESP32-S3, two nRF24L01+ modules, and a 1.8-inch ST7735 TFT. It provides a 2.4 GHz spectrum analyzer, waterfall history, channel inspection, occupancy surveys, RF event recording, CSV logging, per-radio diagnostics, and an RF Test module for controlled laboratory testing.

The interface is designed for a 160 × 128 landscape display. It uses partial/dirty rendering: the complete screen is cleared only during page transitions, while graphs, status values, and menu cards are redrawn only where their content changes. This reduces flicker and keeps the UI responsive.

> [!CAUTION]
> RF Test can transmit traffic that disrupts 2.4 GHz communications. Use it only on equipment you own or are explicitly authorized to test, inside an RF shield box or Faraday enclosure. Never operate it against public networks, third-party devices, safety-critical services, or outside local radio regulations. You are responsible for using this project lawfully and safely.

## Features

- Two nRF24L01+ radios on a shared SPI bus with separate CE and CSN pins.
- 126-channel RF24 spectrum analyzer (`0–125`, or `2400–2525 MHz`).
- Four receive modes: `FAST`, `DIV`, `R1`, and `R2`.
- `ALL`, `Wi-Fi`, and `Bluetooth` scan ranges.
- Modern spectrum graph with an intensity gradient and peak hold.
- 24-sweep waterfall with the newest data at the top.
- Single-channel inspector with live activity and peak readings.
- Channel survey showing the five channels with the highest average occupancy.
- RF event detector with a 60% threshold, 750 ms cooldown, and eight-event buffer.
- One-line-per-sweep CSV logging over USB Serial.
- `FAST`, `BALANCED`, `DEEP`, and `CUSTOM` analyzer profiles.
- Independent connectivity diagnostics for both radios.
- Three-page System Status based on live ESP32 and radio data.
- Six selectable display themes: Cyber, Ocean, Amber, Matrix, Violet, and Ice.
- Persistent configuration using ESP32 NVS.
- Software restart and low-power shutdown using ESP32 deep sleep.
- Three-second watchdog for automatic recovery from an unresponsive main loop.

## Understanding analyzer results

The nRF24L01+ does not provide continuous RSSI measurements. Values displayed as `0–100%` are the percentage of samples in which carrier/RPD activity was detected during an observation window. They are not calibrated dBm readings.

The results are useful for comparing relative activity, finding busy areas, and observing changes over time, but they do not replace a calibrated spectrum analyzer.

Channels shown by the UI are nRF24 RF channels:

```text
frequency_MHz = 2400 + RF24_channel
```

These numbers are not Wi-Fi channel numbers. For example, the center of Wi-Fi channel 1 is near RF24 channel 12 (`2412 MHz`), Wi-Fi channel 6 near RF24 channel 37, and Wi-Fi channel 11 near RF24 channel 62.

## Required hardware

- ESP32-S3 DevKitC-1 or a compatible board.
- 2 × nRF24L01+ modules.
- 1.8-inch 128 × 160 ST7735 TFT.
- 4 × normally-open push buttons.
- One 10–100 µF decoupling capacitor for each nRF24L01+.
- A stable 3.3 V supply and a USB data cable.

Power the nRF24L01+ modules from **3.3 V, never 5 V**. Connect the grounds of the ESP32, both radios, the display, and all buttons. External PA/LNA radio modules may require a dedicated 3.3 V regulator capable of supplying sufficient current.

## Wiring

### nRF24L01+ shared SPI bus

| Signal | Radio 1 | Radio 2 | ESP32-S3 GPIO |
|---|---:|---:|---:|
| VCC | VCC | VCC | 3.3 V |
| GND | GND | GND | GND |
| SCK | SCK | SCK | 12 |
| MOSI | MOSI | MOSI | 11 |
| MISO | MISO | MISO | 13 |
| CE | CE | — | 7 |
| CSN | CSN | — | 6 |
| CE | — | CE | 4 |
| CSN | — | CSN | 2 |
| IRQ | Not used | Not used | — |

Place each decoupling capacitor as close as possible to the corresponding radio's VCC and GND pins. Keep SPI wiring short to reduce communication errors.

### ST7735 TFT

| Module signal | ESP32-S3 GPIO |
|---|---:|
| SCK/CLK | 18 |
| SDA/MOSI | 17 |
| A0/DC | 16 |
| RST/RES | 15 |
| CS | 14 |
| VCC | Match the module, typically 3.3 V |
| GND | GND |

The firmware initializes the display with `INITR_BLACKTAB` and rotation `3`. If colors, offsets, or orientation are incorrect, check the panel variant in `DisplayManager::init()`.

### Navigation buttons

All buttons use `INPUT_PULLUP` and are active-low. Connect one side of each button to its GPIO and the other side to GND.

| Button | ESP32-S3 GPIO | General purpose |
|---|---:|---|
| UP | 10 | Previous item or change analyzer band |
| RIGHT | 9 | Open, confirm, or perform an action |
| DOWN | 8 | Next item or change analyzer mode |
| B | 5 | Back or switch main-menu page |

## Setup and build

The project uses PlatformIO with the Arduino framework.

1. Install [Visual Studio Code](https://code.visualstudio.com/) and the PlatformIO IDE extension, or install PlatformIO Core.
2. Open this directory as a PlatformIO project.
3. Connect the ESP32-S3 using a USB data cable.
4. Build the firmware:

   ```bash
   pio run
   ```

5. Upload it to the board:

   ```bash
   pio run --target upload
   ```

6. Open the Serial Monitor at 115200 baud:

   ```bash
   pio device monitor --baud 115200
   ```

The active environment is defined in `platformio.ini`:

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
```

The current configuration selects a 4 MB upload flash size, the `default.csv` partition table, DIO flash mode, and USB CDC on boot. Adjust `board_upload.flash_size` if the physical board has a different flash capacity.

### Dependencies

PlatformIO installs these dependencies automatically:

- RF24 `^1.6.2`
- Adafruit ST7735 and ST7789 Library `^1.11.0`
- Adafruit GFX Library `^1.12.6`
- `Preferences` and `SPI` from the Arduino ESP32 framework

## User interface

The main menu has two 2 × 3 grid pages. The last page and selected card remain in memory while the device is powered.

| Control | Main-menu action |
|---|---|
| `UP` / `DOWN` | Move between feature cards |
| `RIGHT` | Open the selected feature |
| `B` | Switch between Analyze and Tools |
| Hold `UP` or `DOWN` | Alternative page-switch gesture |

### Page 1 — Analyze

| Feature | Purpose | Controls |
|---|---|---|
| Spectrum | Live activity graph and peak hold | `UP`: band, `DOWN`: radio mode, `RIGHT`: reset peaks, `B`: back |
| Waterfall | Last 24 sweeps, newest at the top | `UP`: band, `DOWN`: radio mode, `RIGHT`: clear history |
| Inspect | Deeper observation of one RF channel | `UP/DOWN`: channel ±1, `RIGHT`: channel +10 |
| Survey | Top five average channel occupancies | `UP/DOWN`: change band and reset, `RIGHT`: reset survey |
| Events | Most recent strong-activity events | `RIGHT`: clear events |
| Logging | Enable one CSV summary per completed sweep | `RIGHT`: start or stop logging |

### Page 2 — Tools

| Feature | Purpose | Controls |
|---|---|---|
| RF Test | Transmission testing in a shielded RF lab | `UP/DOWN`: target, `RIGHT`: start or stop |
| Radio Diag | Check Radio 1 and Radio 2 connectivity | `RIGHT`: refresh |
| Profiles | Select analyzer sampling depth | `UP/DOWN`: profile, `RIGHT`: change CUSTOM value |
| Settings | Configure RF power, dwell time, and display theme | `UP/DOWN`: select field, `RIGHT`: next value |
| Status | Hardware, memory, radio, and software data | `UP/DOWN`: page, `RIGHT`: refresh |
| Power | Restart or enter deep sleep | `UP/DOWN`: option, `RIGHT`: confirm |

Press `B` to return to the main menu from any feature screen.

## Scan ranges, radio modes, and profiles

### Scan ranges

| Range | RF24 channels | Approximate frequency |
|---|---:|---:|
| ALL | 0–125 | 2400–2525 MHz |
| Wi-Fi | 1–73 | 2401–2473 MHz |
| Bluetooth | 2–80 | 2402–2480 MHz |

### Receive modes

| Mode | Behavior |
|---|---|
| FAST | Both radios scan adjacent channels in parallel |
| DIV | Both radios observe the same channel and their results are combined |
| R1 | Use Radio 1 only |
| R2 | Use Radio 2 only |

### Sampling profiles

| Profile | Spectrum | Channel Inspector | Characteristics |
|---|---:|---:|---|
| FAST | 12 samples/channel | 50 samples | Fastest refresh |
| BALANCED | 30 samples/channel | 100 samples | Default |
| DEEP | 60 samples/channel | 200 samples | More stable, slower sweeps |
| CUSTOM | 10–100 samples/channel | 2× spectrum value, capped at 200 | Adjustable with `RIGHT` |

The selected profile and CUSTOM sample count are stored in NVS.

## CSV logging

Open `Analyze → Logging`, enable recording, and connect a Serial Monitor at 115200 baud. Each line follows this format:

```text
RFLOG,<timestamp_ms>,<sweep>,<peak_channel>,<peak_percent>,<band>,<radio_mode>
```

Example:

```text
RFLOG,18240,57,37,73,Wi-Fi (1-73),FAST
```

Logging produces output only while analyzer sweeps are running. Its enabled state is held in RAM and shown as a red badge on the Logging card, but it is not preserved after a reboot.

## System Status

The Status screen reads live runtime values instead of displaying hard-coded hardware information:

1. **Device Info** — chip model, revision and core count, CPU frequency, flash size and clock, and uptime.
2. **Memory Info** — total, free, and minimum heap, largest allocation block, sketch size, and PSRAM status.
3. **Radio / Software** — connectivity of each nRF24, scan mode, RF power, ESP-IDF version, and build date.

A radio status of `CONNECTED` confirms SPI communication with the chip. It does not prove that the antenna, RF matching, or receiver sensitivity is working correctly.

## NVS persistence

The following settings are saved automatically and restored during boot:

- RF power.
- Dwell time.
- Last RF Test target.
- Analyzer profile.
- CUSTOM profile sample count.
- Display theme.

Waterfall history, survey results, RF events, receive mode, analyzer range, and logging state exist only in RAM and are cleared by a restart or shutdown.

## Shutdown and wake-up

Select `Tools → Power → Shutdown`. The firmware will:

1. Stop both radios.
2. Disable the TFT and place its controller into sleep mode.
3. Put the ESP32-S3 into deep sleep.

To wake the device, hold `RIGHT` for approximately 1.5 seconds. A short press returns the device to deep sleep, preventing accidental boots caused by contact noise.

Deep sleep is a very-low-power software shutdown, not physical power disconnection. The board regulator, power LED, and external peripherals may still draw current. A hardware power latch or load switch is required to disconnect the supply completely.

## Serial CLI

The command interface runs at 115200 baud.

| Command | Description |
|---|---|
| `help` | Show the available commands |
| `status` | Show system and radio status |
| `config` / `settings` | Show the current RF configuration |
| `scan` / `spectrum` | Run one sweep and print an ASCII graph |
| `inspect <0-125>` | Measure activity on one RF channel |
| `power` / `pwr` | Show the current RF power |
| `power <min\|low\|high\|max>` | Change RF power |
| `dwell` | Show the current dwell time |
| `dwell <10-10000>` | Set dwell time in microseconds |
| `jam <wifi\|bt\|ble\|bledata\|all\|zigbee>` | Select a target and start RF Test; shielded lab use only |
| `start` | Start RF Test with the active target |
| `stop` | Stop all transmission |

The `scan` and `inspect` commands stop RF Test first so that both radios enter the correct receive mode.

## Firmware architecture

```text
Core 0                          Core 1 / Arduino loop
┌─────────────────────┐         ┌────────────────────────────┐
│ RF Test FreeRTOS    │         │ Buttons + Serial CLI       │
│ task (when active)  │         │ Analyzer dispatcher        │
│                     │         │ Partial display renderer   │
└──────────┬──────────┘         └─────────────┬──────────────┘
           └──────── dual nRF24 shared SPI ───┘
```

- `AppState` owns UI, analyzer, event, survey, and NVS state.
- `RadioManager` controls RX/TX transitions and access to both nRF24 modules.
- `DisplayManager` implements the menu grid and dirty-region rendering.
- `MenuCatalog` is the single source of truth for menu labels, destination modes, icons, and open actions.
- `AppModePolicy` centralizes which screens run continuous spectrum acquisition.
- `ButtonManager` provides 50 ms debouncing and long-press detection.
- `SerialCommander` handles the CLI and logging output.
- `Watchdog` monitors the main loop with a three-second timeout.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for module boundaries and the feature-extension checklist.

## Project structure

| Path | Purpose |
|---|---|
| `platformio.ini` | Board environment, dependencies, flash, and USB CDC settings |
| `include/config`, `src/config` | Pin assignments, constants, channel tables, and static assets |
| `include/core`, `src/core` | Shared types, application state, analyzer aggregation, policies, and NVS |
| `include/drivers`, `src/drivers` | Buttons, dual-radio lifecycle, RF acquisition, and RF Test task |
| `include/services`, `src/services` | Serial CLI and watchdog |
| `include/ui`, `src/ui` | Display API, theme, menu catalog, controller, and screen modules |
| `src/ui/screens` | Renderers grouped into menu, analyzer, and system domains |
| `src/main.cpp` | Setup, main loop, shutdown, and wake validation |
| `docs/ARCHITECTURE.md` | Dependency rules and guide for adding features |

## Troubleshooting

### One or both nRF24 modules are not detected

- Confirm that VCC is 3.3 V and all grounds are connected.
- Verify the separate CE/CSN pins and the shared SCK/MOSI/MISO lines.
- Install a 10–100 µF capacitor close to each module.
- Use short wires and a supply that does not sag when both radios are active.
- The firmware makes up to five initialization attempts and reboots if both radios are not ready.

### The ESP32 resets or reports a brownout

- Do not power high-current PA/LNA modules from an undersized on-board regulator.
- Use a dedicated 3.3 V regulator with a common ground.
- Add rail decoupling and shorten the power wiring.

### The display is blank, inverted, or has incorrect colors

- Verify every TFT pin and the common ground.
- Confirm that the panel controller is an ST7735.
- Adjust `INITR_BLACKTAB` and `setRotation(3)` for a different panel variant.

### The Serial Monitor does not appear

- Use a USB data cable and select the correct port.
- Set the monitor to 115200 baud.
- This project enables `ARDUINO_USB_MODE=1` and `ARDUINO_USB_CDC_ON_BOOT=1`; pressing reset after opening the monitor may help on some hosts.

### The device does not wake from shutdown

- Hold `RIGHT`/GPIO 9 for at least approximately 1.5 seconds.
- Confirm that the button connects GPIO 9 to GND and has no external pull-down.
- Deep-sleep wake depends on an RTC-capable GPIO, so preserve this pin assignment when changing the wiring.

## Current limitations

- The analyzer measures carrier-detection probability, not absolute RSSI or protocol identity.
- It does not decode Wi-Fi, Bluetooth, BLE, or Zigbee packets.
- Waterfall, survey, and event history are not persisted to flash.
- The ST7735 renderer does not use a full framebuffer; partial rendering is used to save RAM.
- Software shutdown does not physically disconnect board power.
- There are no automated hardware-in-the-loop tests; final verification must be performed on the physical device.

## Contributing

Before submitting changes, run:

```bash
pio run --target clean
pio run
```

Keep pin definitions in `include/config/Config.h`, avoid full-screen redraws for dynamic updates, and document changes to Serial or NVS formats to preserve user compatibility.

This repository currently has no separate license file. Add a `LICENSE` before distributing it under specific license terms.
