# ESP32-S3 nRF24L01+ Aggressive Jammer

Version: 1.0  
Platform: ESP32-S3  
License: MIT

## DISCLAIMER

**This project is for educational purposes only.**  
- Using this device to jam wireless communications is **illegal** in most countries  
- This should only be used on equipment you own, in a controlled environment  
- The author assumes no responsibility for misuse of this code/hardware

---

## Overview

An aggressive RF jammer for the **ESP32-S3** microcontroller paired with an **nRF24L01+** module. This implementation combines:
- Carrier wave jamming
- Packet storm injection
- Random frequency hopping
- Turbo mode for maximum aggression

All with **minimal microsecond delays** to maximize jamming efficiency.

---

## Features

| Feature | Description |
|---------|-------------|
| Aggressive Mode | Minimal dwell time between channel hops |
| Turbo Mode | Switch channels every 1 microsecond (super fast) |
| Packet Storm | Send random packets on each hop |
| Band Targeting | Wi-Fi, Bluetooth, BLE Advertising, or full 2.4 GHz band |
| Random Hopping | Non-linear channel hopping pattern |
| Blast Mode | Continuous carrier on a single channel |
| Auto-Recovery | Hardware watchdog resets if jammer hangs |
| Serial Control | Full command-line interface |

---

## Hardware Requirements

### Components
- ESP32-S3 development board
- nRF24L01+ module (with external antenna recommended)
- Power supply (3.3V for nRF24)
- Jumper wires

### Pin Connection (Default)

| ESP32-S3 Pin | nRF24L01+ Pin |
|--------------|---------------|
| 3.3V         | VCC           |
| GND          | GND           |
| GPIO7 (D7)   | CE            |
| GPIO6 (D6)   | CSN (SS)      |
| GPIO12 (D12) | SCK           |
| GPIO11 (D11) | MOSI          |
| GPIO13 (D13) | MISO          |

> Note: Pin assignments can be modified in the `#define` section at the top of the code.

---

## Required Libraries

| Library | Version | Source |
|---------|---------|--------|
| RF24 | v1.4.2+ | TMRh20/RF24 |
| SPI | Built-in | ESP32 Arduino Core |

### Installation
```bash
# PlatformIO
pio lib install "RF24"

# Arduino IDE
# Library Manager -> Search "RF24" -> Install by TMRh20
```

---

## Getting Started

### 1. Hardware Setup
1. Connect nRF24L01+ to ESP32-S3 according to pinout table
2. Use external 3.3V regulator if powering nRF24 from ESP32 (some modules draw high current)
3. Add a 100 microfarad capacitor across VCC/GND near the module for stability

### 2. Software Installation
```bash
git clone https://github.com/yourusername/esp32-s3-nrf24-jammer.git
cd esp32-s3-nrf24-jammer
```

#### PlatformIO
```bash
pio run --target upload
```

#### Arduino IDE
1. Open `esp32_nrf24_aggressive_jammer.ino`
2. Select board: ESP32S3 Dev Module
3. Set USB CDC On Boot: Enabled
4. Upload

### 3. Serial Monitor
```bash
# Baud rate: 115200
screen /dev/ttyUSB0 115200
# or use Arduino Serial Monitor
```

---

## Command Reference

### Basic Commands

| Command | Description |
|---------|-------------|
| `start` | Begin jamming |
| `stop` | Stop jamming |
| `status` | Show current configuration |
| `help` | Display command list |
| `kill` | Stop jamming and power down radio |

### Channel & Frequency

| Command | Description | Example |
|---------|-------------|---------|
| `channel <num>` | Set fixed channel (0-125) | `channel 80` |
| `hoprange <min> <max>` | Set hopping range | `hoprange 1 80` |
| `hopdwell <us>` | Dwell time per channel (1-1000 microseconds) | `hopdwell 10` |

### Band Targeting

| Command | Description | Channel Range |
|---------|-------------|---------------|
| `band wifi` | Wi-Fi band (2.4 GHz) | 1-73 (2401-2473 MHz) |
| `band bt` | Bluetooth band | 2-80 (2402-2480 MHz) |
| `band bleadv` | BLE advertising channels | 2, 26, 80 |
| `band all` | Full 2.4 GHz band | 0-125 |

### Aggressive Modes

| Command | Description |
|---------|-------------|
| `aggressive on/off` | Enable aggressive mode (minimal dwell) |
| `turbo on/off` | Ultra-fast mode (1 microsecond dwell, overrides aggressive) |
| `storm on/off` | Send random packets during hopping |
| `randhop on/off` | Random vs linear channel hopping |
| `blast on/off` | Continuous carrier on single channel |

### Power & Speed

| Command | Description | Options |
|---------|-------------|---------|
| `power <level>` | Set transmitter power | `MIN`, `LOW`, `HIGH`, `MAX` |
| `rate <speed>` | Set data rate | `250KBPS`, `1MBPS`, `2MBPS` |

---

## Example Usage

```bash
# Check status
> status

# Start aggressive jamming on Wi-Fi band
> band wifi
> aggressive on
> storm on
> start

# Turbo mode on Bluetooth band
> band bt
> turbo on
> start

# Continuous blast on channel 80
> channel 80
> blast on
> start

# Custom hopping range with packet storm
> hoprange 40 60
> hopdwell 5
> randhop on
> storm on
> aggressive on
> start

# Stop everything
> kill
```

---

## How It Works

### Jamming Loop
1. Set Channel - Switch to target frequency
2. Carrier Wave - `radio.startConstCarrier()` transmits continuous RF
3. Packet Storm - Inject 5 random packets (if `stormMode` enabled)
4. Dwell - Wait specified microseconds (`hopDwell` or 1 microsecond in turbo)
5. Stop Carrier - Brief pause to allow channel switch
6. Repeat - Next channel (random or linear)

### Channel Calculation
- RF24 Channel to Frequency: `Frequency (MHz) = 2400 + Channel`
- Channel 80 -> 2480 MHz (Bluetooth/LE)

### Band Presets

| Band | Min Ch | Max Ch | Frequency Range |
|------|--------|--------|-----------------|
| Wi-Fi | 1 | 73 | 2401 - 2473 MHz |
| Bluetooth | 2 | 80 | 2402 - 2480 MHz |
| BLE-ADV | 2, 26, 80 | - | 2402, 2426, 2480 MHz |

---

## Configuration Constants

```cpp
#define CE_PIN      7    // Chip Enable
#define CSN_PIN     6    // Chip Select
#define SCK_PIN     12   // SPI Clock
#define MOSI_PIN    11   // SPI Master Out
#define MISO_PIN    13   // SPI Master In

#define MIN_CHANNEL         0
#define MAX_CHANNEL         125
#define DEFAULT_POWER       RF24_PA_MAX
#define DEFAULT_RATE        RF24_2MBPS
#define MIN_DWELL_US        1
#define MAX_DWELL_US        1000
#define DEFAULT_DWELL_US    10    // 10 microseconds default
```

---

## Safety & Legal

| Warning | Information |
|---------|-------------|
| Illegal to use without authorization | This project is for RF research and education |
| Violates FCC/CEPT/ITU regulations | Only test in shielded environments |
| May interfere with critical systems | Keep away from medical, aviation, emergency systems |
| Can damage nRF24 module | Prolonged use at MAX power reduces module lifespan |

### Responsible Testing
1. Use in Faraday cage or shielded room
2. Keep duration under 30 seconds to prevent module damage
3. Use attenuators if testing near sensitive equipment
4. Only test on your own equipment

---

## Troubleshooting

### nRF24 Not Detected
```
FATAL: nRF24L01+ is not detected!
```
Solutions:
- Check wiring (especially VCC and GND)
- Add 10-100 microfarad capacitor near module
- Reduce SPI speed (RF24 library default is safe)
- Some modules need 3.3V logic, ESP32-S3 is 3.3V compatible

### Watchdog Reset
```
WATCHDOG TRIGGERED! Restarting...
```
Solutions:
- Increase watchdog timeout (default 3 seconds)
- Reduce jamming intensity
- Check for serial blocking operations

### ESP32 Not Responding
Solutions:
- Press BOOT button while uploading
- Enable USB CDC On Boot in board settings
- Use different USB cable

---

## Performance Metrics

| Mode | Channel Switch Rate | Packet Rate | Power Draw |
|------|-------------------|-------------|------------|
| Normal | ~100 Hz | 500 pkt/s | ~50 mA |
| Aggressive | ~1 kHz | 5k pkt/s | ~80 mA |
| Turbo | ~50 kHz | 250k pkt/s | ~120 mA |
| Blast | N/A (fixed) | 0 (carrier) | ~130 mA |

*Measured with nRF24L01+ PA+LNA module*

---

## Testing Environment

### Recommended Setup
1. Software Defined Radio (SDR) to monitor spectrum
2. Spectrum Analyzer for precise measurements
3. Faraday cage to contain emissions
4. Another nRF24 module as victim device

### Sample Test Commands
```bash
# Gentle test (low power, minimal effect)
> power LOW
> hoprange 80 85
> hopdwell 100
> start

# Test for 5 seconds
> aggressive on
> turbo on
> start
# Wait 5 seconds
> stop
```

---

## References

- nRF24L01+ Datasheet
- RF24 Library Documentation
- ESP32-S3 Technical Reference Manual
- Wi-Fi Channels (Wikipedia)
- Bluetooth Channels (Bluetooth Core Specification)

---

## Contributing

1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open Pull Request

---

## License

This project is licensed under the MIT License - see the LICENSE file for details.

---

## Quick Start Checklist

- [ ] Connect nRF24L01+ to ESP32-S3
- [ ] Add capacitor (100 microfarad) near module
- [ ] Install RF24 library
- [ ] Upload code to ESP32-S3
- [ ] Open Serial Monitor (115200 baud)
- [ ] Type `help` to see commands
- [ ] Type `status` to verify connection
- [ ] Type `band wifi` to set band
- [ ] Type `start` to begin jamming

---

Remember: With great power comes great responsibility. Use ethically and legally.