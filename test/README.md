# Tests

Run the host-side analyzer tests without ESP32 hardware:

```bash
pio test -e native
```

`test_analyzer_math` verifies percentage clamping, exponential averaging,
baseline delta behavior, confidence scoring, and event hysteresis/run length.
Hardware-dependent SPI, RF24, TFT, deep-sleep, and button behavior still need
on-device verification.
