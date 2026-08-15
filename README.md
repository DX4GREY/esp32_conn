# ESP32-S3 Dual-Core RF24 Jammer & 2.4 GHz Spectrum Analyzer

Version: 3.0 (Dual-Core FreeRTOS Engine)  
Platform: ESP32-S3  
Display: TFT ST7735 1.8" (160x128 SPI Compact Layout)  
License: MIT

---

## 🌟 Dual-Core Architecture Highlights (FreeRTOS)

1. **Core 0 (Dedicated RF Transmitter Loop - High Priority)**:
   - Runs the nRF24L01+ transmission loop continuously without pausing (`vTaskPrioritySet(NULL, 3)`).
   - Packet storm injection with **5-byte payload** (`writeFast`), **3-byte address width**, **CRC Disabled**, and **LNA Gain MAX**.
   - Not affected by screen rendering, Serial delay, or button reads.

2. **Core 1 (UI, Serial CLI, & Spectrum Analyzer Dispatcher)**:
   - Handles TFT ST7735 1.8" graphics rendering.
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

## 📊 Radio Analyzer Features

- **Live Spectrum Graph (160x128)**: Real-time visualization of all 126 channels in the 2.4 GHz band with intensity color gradients and Peak Hold.
- **Channel Inspector**: In-depth monitoring of a single channel with a signal activity gauge bar and carrier detect status.
- **ASCII Spectrum Scanner**: Print a spectrum graph to the Serial Monitor via the `scan` command.

---

## ⌨️ Serial Command List (115200 Baud)

| Command | Description |
|---|---|
| `jam wifi` | Wi-Fi 2.4 GHz (50 Programmed Channels) |
| `jam bt` | Bluetooth Classic (Ch 1-80 Even+Odd Hop) |
| `jam ble` | BLE Advertising (Ch 1-3, 25-27, 79-81) |
| `jam bledata` | BLE Data (12 Ch from 3 Groups) |
| `jam all` | Full 2.4 GHz Band / Drone (Ch 1 - 100) |
| `jam zigbee` | Zigbee Band (Ch 11 - 26) |
| `stop` | Stop jammer transmission |
| `scan` | Run Spectrum Analyzer and print the RF ASCII graph |
| `inspect <ch>` | Analyze RF activity on a specific channel |
| `status` | Show device status & Core 0 task |
| `help` | Show command help |
