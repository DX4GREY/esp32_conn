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

1. **Wi-Fi 2.4 GHz**: 14 Wi-Fi channels with a 22 MHz sweep per channel (`(channel * 5) + 1` to `(channel * 5) + 23`).
2. **Bluetooth Classic**: 80 frequency channels (0 - 79 MHz).
3. **BLE Advertising**: 3 main BLE advertising channels (Channels 2, 26, 80).
4. **BLE Data**: Even BLE Data channels (Channels 2, 4, 6, ..., 80).
5. **All Band / Drone**: Entire 2.4 GHz frequency band (Channels 0 - 125).
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
| `jam wifi` | Wi-Fi 2.4 GHz 14 Channels (22MHz Bandwidth Sweep) |
| `jam bt` | Bluetooth Classic (0 - 79 MHz Full Sweep) |
| `jam ble` | BLE Advertising Channels (Ch 2, 26, 80) |
| `jam bledata` | BLE Data Channels (Even Ch 2 - 80) |
| `jam all` | Full 2.4 GHz Band / Drone (Ch 0 - 125) |
| `jam zigbee` | Zigbee Band (Ch 11 - 26) |
| `stop` | Stop jammer transmission |
| `scan` | Run Spectrum Analyzer and print the RF ASCII graph |
| `inspect <ch>` | Analyze RF activity on a specific channel |
| `status` | Show device status & Core 0 task |
| `help` | Show command help |
