# ESP32-S3 nRF24L01+ Jammer & 2.4 GHz Spectrum Analyzer

Versi: 2.0  
Platform: ESP32-S3  
Display: TFT ST7735 1.8" (160x128 SPI)  
License: MIT

---

## ⚠️ PERINGATAN & DISCLAIMER
**Proyek ini ditujukan untuk tujuan edukasi, pengujian sinyal RF, dan analisis spektrum di lingkungan laboratorium tertutup.** Penggunaan pemancar RF tanpa izin untuk mengganggu komunikasi wireless dapat melanggar hukum telekomunikasi di banyak negara.

---

## 🌟 Fitur Utama

### 1. 🔥 Simplified RF Jammer (Praktis & Terfokus)
Pengguna tidak perlu lagi mengatur dwell, sweep, dan parameter rumit secara manual. Cukup pilih target target:
- **Wi-Fi 2.4 GHz**: Kanal 1 - 73 (2401 - 2473 MHz)
- **Bluetooth Classic & BLE**: Kanal 2 - 80 (2402 - 2480 MHz)
- **BLE Advertising Only**: Kanal 2, 26, dan 80 (2402, 2426, 2480 MHz)
- **All 2.4 GHz Band**: Kanal 0 - 125 (2400 - 2525 MHz)
*Sistem secara otomatis mengaktifkan parameter jamming paling agresif (Maximum PA Level, 2 Mbps rate, packet storm injection, dan ultra-fast carrier dwell).*

### 2. 📊 Live 2.4 GHz Spectrum Analyzer
- **Visual Spectrum Graph Real-time** pada layar TFT ST7735 (160x128).
- **126 Kanal (0 - 125)** divisualisasikan dengan gradien warna intensitas sinyal:
  - 🟢 Hijau: Sinyal Rendah (< 30%)
  - 🟡 Kuning: Sinyal Sedang (30% - 65%)
  - 🟠 Oranye: Sinyal Tinggi (65% - 85%)
  - 🔴 Merah: Sinyal Sangat Kuat (> 85%)
- **Peak Hold Indicator** (titik cyan penahan nilai puncak dengan decay otomatis).
- **Penanda Kanal Wi-Fi 1, 6, 11, 14** pada sumbu X.
- **Filter Band Scan**: ALL / Wi-Fi / Bluetooth.

### 3. 🔍 Channel Inspector
- Monitor mendalam pada satu kanal spesifik.
- Menampilkan frekuensi MHz, mapping protokol (Wi-Fi / Bluetooth), progress bar intensitas sinyal, dan status carrier detection.

### 4. 💻 Serial CLI & ASCII Spectrum Monitor
- Kontrol penuh melalui Serial Monitor (115200 baud).
- Perintah `scan` menampilkan grafik spektrum ASCII langsung di terminal!

---

## 🛠️ Pinout Hardware

| Komponen | ESP32-S3 Pin | Fungsi / Pin Modul |
|---|---|---|
| **nRF24L01+** | 3.3V / GND | VCC / GND (Gunakan elco 100uF) |
| | GPIO 7 | CE |
| | GPIO 6 | CSN (SS) |
| | GPIO 12 | SCK |
| | GPIO 11 | MOSI |
| | GPIO 13 | MISO |
| **TFT ST7735 1.8"** | GPIO 14 | CS |
| | GPIO 15 | RESET |
| | GPIO 16 | A0 / DC |
| | GPIO 17 | SDA / MOSI |
| | GPIO 18 | SCK |
| **Tombol Navigasi** | GPIO 10 | UP (Pull-Up) |
| | GPIO 9 | RIGHT / ENTER (Pull-Up) |
| | GPIO 8 | DOWN (Pull-Up) |
| | GPIO 5 | BACK / B (Pull-Up) |

---

## 🎮 Cara Penggunaan Navigasi Layar

- **Menu Utama**:
  - `UP` / `DOWN`: Pindah menu
  - `RIGHT`: Masuk ke fitur yang dipilih
- **Di Layar Jammer**:
  - `UP` / `DOWN`: Ganti Target (Wi-Fi / BT / BLE / All Band)
  - `RIGHT`: Start / Stop Jamming
  - `B`: Kembali ke Menu
- **Di Layar Spectrum Analyzer**:
  - `UP` / `DOWN`: Ganti rentang band scan (ALL / Wi-Fi / BT)
  - `RIGHT`: Reset Peak Hold
  - `B`: Keluar ke Menu
- **Di Layar Channel Inspector**:
  - `UP` / `DOWN`: Ganti kanal (+/- 1)
  - `RIGHT`: Lompat kanal (+/- 10)
  - `B`: Keluar ke Menu

---

## ⌨️ Daftar Perintah Serial (115200 Baud)

| Perintah | Deskripsi |
|---|---|
| `jam wifi` | Mengaktifkan jamming target Wi-Fi (Ch 1 - 73) |
| `jam bt` | Mengaktifkan jamming target Bluetooth (Ch 2 - 80) |
| `jam ble` | Mengaktifkan jamming target BLE Advertising (Ch 2/26/80) |
| `jam all` | Mengaktifkan jamming seluruh pita 2.4 GHz (Ch 0 - 125) |
| `stop` | Menghentikan jamming / scanning radio |
| `scan` | Memindai 126 kanal dan mencetak visual ASCII Spectrum Graph di Serial |
| `inspect <ch>` | Menganalisis aktivitas RF mendalam pada satu kanal |
| `status` | Menampilkan ringkasan status perangkat dan radio |
| `help` | Menampilkan daftar perintah |