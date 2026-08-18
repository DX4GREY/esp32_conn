# ESP32-S3 Dual-Core RF24 Jammer & 2.4 GHz Spectrum Analyzer

Version: 3.2 (NVS Persistence + Memory-Optimized Config)    
Platform: ESP32-S3  
Display: TFT ST7735 1.8" (160x128 SPI Compact Layout)  
License: MIT

---

## 🌟 Dual-Core Architecture Highlights (FreeRTOS)

1. **Core 0 (Dedicated RF Transmitter Loop - High Priority)**:
   - Runs the nRF24L01+ transmission loop continuously without pausing (`vTaskPrioritySet(NULL, 3)`).
   - Packet storm injection with **5-byte payload** (`writeFast`), **3-byte address width**, **CRC Disabled**, and dynamic **PA Level** (MIN, LOW, HIGH, MAX + LNA Gain).
   - Dynamic **Dwell Time** (`50 µs`, `100 µs`, `200 µs`, `500 µs`, `1000 µs`) per hop.
   - Not affected by screen rendering, Serial delay, or button reads.

2. **Core 1 (UI, Serial CLI, & Spectrum Analyzer Dispatcher)**:
   - Handles TFT ST7735 1.8" graphics rendering with compact, safe UI layouts.
   - Dedicated **RF Power & Dwell Config** interactive menu screen.
   - Processes physical navigation buttons with 50ms debouncing.
   - Runs the 3.0s Hardware Watchdog for auto-recovery.

---

## 🎯 6 Jammer Target Presets

1. **Wi-Fi 2.4 GHz**: 50 programmed channels (`wifi_channels[]`) covering the 22 MHz Wi-Fi band.
2. **Bluetooth Classic**: Full hop over all channels 1 - 80 (`bluetooth_odd_channels[]` + `bluetooth_even_channels[]`).
3. **BLE Advertising**: Channel groups 1-3, 25-27, 79-81 (advertising channels 2/26/80 + adjacent hops, `ble_channels[]`).
4. **BLE Data**: 12 channels from 3 groups (`BLE_DATA_CHANNELS[]`).
5. **All Band / Drone**: Full 2.4 GHz band, channels 1 - 100 (`full_channels[]`).
6. **Zigbee**: Channels 11 - 26 (2405 - 2480 MHz).

---

## ⚙️ Dynamic RF Power & Dwell Controls

- **TX Power Levels**: `MIN (-18 dBm)`, `LOW (-12 dBm)`, `HIGH (-6 dBm)`, `MAX (0 dBm + LNA Boost)`.
- **Hopping Dwell Presets**: `50 µs` (Ultra-fast), `100 µs` (Fast), `200 µs` (Balanced default), `500 µs` (Heavy airtime), `1000 µs` (Deep blast).
- Configurable directly from the **TFT Menu (4. RF Power & Dwell)** or dynamically via **Serial CLI** commands (`power` / `dwell`).

---

## 📊 Radio Analyzer Features

- **Live Spectrum Graph (160x128)**: Real-time visualization of all 126 channels in the 2.4 GHz band with intensity color gradients and Peak Hold.
- **Channel Inspector**: In-depth monitoring of a single channel with a signal activity gauge bar and carrier detect status.
- **ASCII Spectrum Scanner**: Print a spectrum graph to the Serial Monitor via the `scan` command.

---

## 💾 NVS Settings Persistence
Settings are stored in ESP32 flash (NVS) and survive power cycles:
- **TX Power Level** (min/low/high/max)
- **Dwell Time** (50–1000 µs)
- **Active Jammer Target** (wifi, bt, ble, bledata, all, zigbee)

Settings are automatically saved on every change (via TFT menu or Serial CLI) and loaded on boot.

---

## 📁 Project Structure

| File | Description |
|---|---|
| `include/Config.h` | Hardware pins, frequency presets, constants (extern declarations) |
| `src/Config.cpp` | Channel/lookup table definitions (single-definition to save ~112 KB flash) |
| `include/AppState.h` | Global state struct, enums, NVS method declarations |
| `src/AppState.cpp` | State logic + NVS load/save persistence |
| `include/RadioManager.h/.cpp` | Core 0 FreeRTOS jammer task + spectrum scanning |
| `include/DisplayManager.h/.cpp` | TFT rendering, menu navigation, input processing |
| `include/ButtonManager.h/.cpp` | 50 ms debounced button edge detection |
| `include/SerialCommander.h/.cpp` | Interactive CLI monitor & ASCII graph |
| `include/Watchdog.h/.cpp` | 3 s hardware watchdog for auto-recovery |

---

## ⌨️ Serial Command List (115200 Baud)

| Command | Description |
|---|---|
| `jam <target>` | Jam target: `wifi`, `bt`, `ble`, `bledata`, `all`, `zigbee` |
| `power [lvl]` | Set / display TX power: `min`, `low`, `high`, `max` (e.g. `power max`) |
| `dwell [us]` | Set / display hop dwell time: `50`, `100`, `200`, `500`, `1000` |
| `config` | Display complete RF configuration parameters |
| `start` | Start jammer on currently selected target |
| `stop` | Stop jammer transmission |
| `scan` | Run Spectrum Analyzer and print the RF ASCII graph |
| `inspect <ch>` | Analyze RF activity on a specific channel (0 - 125) |
| `status` | Show device status, radio connectivity, and Core 0 task |
| `help` | Show command help list |
