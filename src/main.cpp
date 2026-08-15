/*
 * =============================================================================
 * ESP32-S3 RF24 SUITE: SIMPLIFIED JAMMER & SPECTRUM ANALYZER
 * =============================================================================
 * Modul Terorganisir:
 *  - Config.h             : Pinout hardware, timing, preset frekuensi, & konstanta
 *  - AppState.h/.cpp      : State global, mode jammer (Wi-Fi/BT/BLE/All), & data analyzer
 *  - ButtonManager.h/.cpp : Debouncing 50ms & edge detection tombol navigasi
 *  - Watchdog.h/.cpp      : Hardware watchdog timer ESP32-S3 (3 detik auto-recovery)
 *  - RadioManager.h/.cpp  : Transmisi jamming agresif & scanning sinyal carrier RF
 *  - DisplayManager.h/.cpp: Menu visual, grafik spektrum real-time, & channel inspector
 *  - SerialCommander.h/.cpp: CLI monitor interaktif & visualisasi grafik ASCII
 * =============================================================================
 */

#include <Arduino.h>
#include "Config.h"
#include "AppState.h"
#include "Watchdog.h"
#include "ButtonManager.h"
#include "RadioManager.h"
#include "DisplayManager.h"
#include "SerialCommander.h"

// Callback untuk menjaga tombol & sistem UI tetap responsif saat jamming atau scanning
void yieldToUI() {
    displayManager.processInput();
    watchdog.feed();
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    // 1. Inisialisasi Serial CLI (115200 Baud)
    serialCommander.init(115200);

    // 2. Inisialisasi Tombol Navigasi (Pull-Up)
    buttonManager.init();

    // 3. Inisialisasi Layar TFT ST7735 1.8" (160x128)
    displayManager.init();

    // 4. Inisialisasi Radio nRF24L01+
    if (!radioManager.init()) {
        while (1) {
            delay(1000);
        }
    }

    // 5. Inisialisasi Hardware Watchdog (3.0s Timeout)
    watchdog.init(WATCHDOG_TIMEOUT_US);
}

// =============================================================================
// MAIN LOOP
// =============================================================================
void loop() {
    // 1. Reset Watchdog Timer (Heartbeat)
    watchdog.feed();

    // 2. Proses Perintah Serial Monitor jika ada
    serialCommander.process();

    // 3. Proses Input Tombol Fisik
    displayManager.processInput();

    // 4. Eksekusi Berdasarkan Mode Aktif
    if (appState.appMode == APP_MODE_JAMMER && appState.jamming) {
        // Mode Jammer: Eksekusi serangan packet storm + carrier hop
        radioManager.stepJammer(yieldToUI);
    } else if (appState.appMode == APP_MODE_ANALYZER_SPECTRUM) {
        // Mode Radio Analyzer: Pindai 126 kanal 2.4 GHz dan update level spektrum
        radioManager.scanSpectrum(yieldToUI);
    } else if (appState.appMode == APP_MODE_ANALYZER_CHANNEL) {
        // Mode Channel Inspector: Monitor sinyal mendalam pada 1 kanal
        radioManager.inspectChannel(appState.inspectedChannel);
        displayManager.requestRedraw();
        delay(25);
    } else {
        // Mode Idle / Menu: Hemat daya
        delay(10);
    }

    // 5. Render Tampilan Layar TFT jika ada pembaruan
    displayManager.updateUI();

    // 6. Auto-recovery jika terjadi Watchdog Timeout
    if (watchdog.isTriggered()) {
        Serial.println("WATCHDOG TRIGGERED! Restarting ESP32...");
        ESP.restart();
    }
}