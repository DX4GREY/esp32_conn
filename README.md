# ESP32-S3 Dual-Core RF24 Jammer & 2.4 GHz Spectrum Analyzer

Versi: 3.0 (Dual-Core FreeRTOS Engine)  
Platform: ESP32-S3  
Display: TFT ST7735 1.8" (160x128 SPI Compact Layout)  
License: MIT

---

## 🌟 Keunggulan Arsitektur Dual-Core (FreeRTOS)

1. **Core 0 (Dedicated RF Transmitter Loop - High Priority)**:
   - Menjalankan loop transmisi nRF24L01+ secara terus-menerus tanpa jeda (`vTaskPrioritySet(NULL, 3)`).
   - Injeksi packet storm dengan **5-byte payload** (`writeFast`), **3-byte address width**, **CRC Disabled**, dan **LNA Gain MAX**.
   - Tidak terganggu oleh rendering layar, delay Serial, atau pembacaan tombol.

2. **Core 1 (UI, Serial CLI, & Spectrum Analyzer Dispatcher)**:
   - Menangani rendering grafis TFT ST7735 1.8".
   - Memproses tombol navigasi fisik dengan debouncing 50ms.
   - Menjalankan Hardware Watchdog 3.0s untuk auto-recovery.

---

## 🎯 6 Preset Target Jammer

1. **Wi-Fi 2.4 GHz**: 14 kanal Wi-Fi dengan sweep 22 MHz per kanal (`(channel * 5) + 1` s/d `(channel * 5) + 23`).
2. **Bluetooth Classic**: 80 kanal frekuensi (0 - 79 MHz).
3. **BLE Advertising**: 3 kanal utama advertising BLE (Kanal 2, 26, 80).
4. **BLE Data**: Kanal genap BLE Data (Kanal 2, 4, 6, ..., 80).
5. **All Band / Drone**: Seluruh pita frekuensi 2.4 GHz (Kanal 0 - 125).
6. **Zigbee**: Kanal 11 - 26 (2405 - 2480 MHz).

---

## 📊 Radio Analyzer Features

- **Live Spectrum Graph (160x128)**: Visualisasi sinyal 126 kanal 2.4 GHz real-time dengan gradien warna intensitas dan Peak Hold.
- **Channel Inspector**: Monitor mendalam pada satu kanal dengan bar gauge aktivitas sinyal dan status carrier detect.
- **ASCII Spectrum Scanner**: Cetak grafik spektrum di Serial Monitor lewat perintah `scan`.

---

## ⌨️ Daftar Perintah Serial (115200 Baud)

| Perintah | Deskripsi |
|---|---|
| `jam wifi` | Wi-Fi 2.4 GHz 14 Channels (22MHz Bandwidth Sweep) |
| `jam bt` | Bluetooth Classic (0 - 79 MHz Full Sweep) |
| `jam ble` | BLE Advertising Channels (Ch 2, 26, 80) |
| `jam bledata` | BLE Data Channels (Even Ch 2 - 80) |
| `jam all` | Full 2.4 GHz Band / Drone (Ch 0 - 125) |
| `jam zigbee` | Zigbee Band (Ch 11 - 26) |
| `stop` | Menghentikan transmisi jammer |
| `scan` | Menjalankan Spectrum Analyzer dan mencetak grafik RF ASCII |
| `inspect <ch>` | Menganalisis aktivitas sinyal pada 1 kanal spesifik |
| `status` | Menampilkan status perangkat & task Core 0 |
| `help` | Menampilkan bantuan perintah |