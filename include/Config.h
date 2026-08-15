#pragma once
#include <Arduino.h>
#include <RF24.h>

// =============================================================================
// HARDWARE PIN DEFINITIONS
// =============================================================================

// Radio nRF24L01+ (SPI kustom ESP32-S3)
#define CE_PIN   7
#define CSN_PIN  6
#define SCK_PIN  12
#define MOSI_PIN 11
#define MISO_PIN 13

// Display TFT ST7735 1.8" (128x160 SPI)
#define TFT_SCK   18
#define TFT_SDA   17
#define TFT_AO    16
#define TFT_RST   15
#define TFT_CS    14

// Navigation Buttons (Active LOW / Internal Pull-Up)
#define BTN_UP    10
#define BTN_RIGHT 9
#define BTN_DOWN  8
#define BTN_B     5

// =============================================================================
// TFT DISPLAY COLORS (RGB565)
// =============================================================================
#ifndef ST77XX_GRAY
#define ST77XX_GRAY 0x8410
#endif
#ifndef ST77XX_DARKGRAY
#define ST77XX_DARKGRAY 0x39E7
#endif
#ifndef CUSTOM_ORANGE
#define CUSTOM_ORANGE 0xFD20
#endif

// =============================================================================
// BASIC FREQUENCY & RADIO CONFIGURATION
// =============================================================================
#define MIN_CHANNEL         0
#define MAX_CHANNEL         125
#define TOTAL_CHANNELS      126           // 0 - 125
#define DEFAULT_POWER       RF24_PA_MAX
#define DEFAULT_RATE        RF24_2MBPS    // 2Mbps = performa agresif

// Target Band Presets (Frequency = 2400 + RF24 Channel in MHz)
#define WIFI_MIN_CH         1             // 2401 MHz - awal band Wi-Fi (ch 1-13)
#define WIFI_MAX_CH         73            // 2473 MHz - akhir band Wi-Fi
#define BT_MIN_CH           2             // 2402 MHz - awal band Bluetooth
#define BT_MAX_CH           80            // 2480 MHz - akhir band Bluetooth (BR/EDR + BLE)

// 3 Main BLE Advertising Channels (2402, 2426, 2480 MHz)
const int BLE_ADV_CHANNELS[3] = {2, 26, 80};

// 21 Main Bluetooth AFH Channels
const int BT_AFH_CHANNELS[21] = {1, 6, 11, 16, 21, 26, 31, 36, 41, 46, 51, 56, 61, 66, 71, 76, 3, 23, 43, 63, 79};

// Fast payload for Packet Storm (5-byte ultra low overhead)
#define FAST_PAYLOAD_SIZE   5
#define FAST_ADDRESS_WIDTH  3
const uint8_t FAST_JAM_PAYLOAD[5] = {0xAA, 0x55, 0xAA, 0x55, 0xAA};

// =============================================================================
// ANALYZER & GRAPH CONFIGURATION
// =============================================================================
#define SPECTRUM_SAMPLES_PER_CH 30        // Carrier hit sample count per channel
#define INSPECT_SAMPLES         100       // Deep channel inspector sample count
#define PEAK_DECAY_RATE         1         // Kecepatan penurunan peak hold

// Spectrum Graph Area Dimensions (160x128 Compact Safe Layout Screen)
#define GRAPH_X_START       18            // Left margin for Y labels
#define GRAPH_WIDTH         126           // 1 pixel per channel (0..125)
#define GRAPH_Y_TOP         16
#define GRAPH_HEIGHT        64
#define GRAPH_Y_BASELINE    80

// =============================================================================
// TIMING & BUFFER
// =============================================================================
#define BUTTON_DEBOUNCE_MS  50
#define WATCHDOG_TIMEOUT_US 3000000       // 3 seconds (3,000,000 µs)
#define PAYLOAD_SIZE        32

// Dummy nRF24 pipe address
const uint8_t DEFAULT_PIPE_ADDRESS[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
