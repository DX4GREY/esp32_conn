#pragma once
#include <Arduino.h>
#include <RF24.h>
#include "Config.h"

// =============================================================================
// ENUMERASI STATE APLIKASI
// =============================================================================

enum AppMode {
    APP_MODE_MENU,               // Menu Utama
    APP_MODE_JAMMER,             // Mode Jammer Cepat Multi-Target
    APP_MODE_ANALYZER_SPECTRUM,  // Mode Radio Analyzer: Live RF Spectrum Graph
    APP_MODE_ANALYZER_CHANNEL,   // Mode Radio Analyzer: Detail Inspeksi 1 Channel
    APP_MODE_STATUS              // Informasi Status Perangkat & Hardware
};

enum JammerTarget {
    JAM_TARGET_WIFI = 0,         // Target Wi-Fi 2.4 GHz (14 Channels 22MHz Sweep)
    JAM_TARGET_BT = 1,           // Target Bluetooth Classic (0 - 79 MHz Sweep)
    JAM_TARGET_BLE_ADV = 2,      // Target BLE Advertising Only (Ch 2, 26, 80)
    JAM_TARGET_BLE_DATA = 3,     // Target BLE Data Only (Even Ch 2 - 80)
    JAM_TARGET_ALL = 4,          // Target Seluruh Band 2.4 GHz / Drone (Ch 0 - 125)
    JAM_TARGET_ZIGBEE = 5        // Target Zigbee (Ch 11 - 26)
};

enum AnalyzerBand {
    SCAN_BAND_ALL = 0,           // Scan seluruh 126 kanal (0 - 125)
    SCAN_BAND_WIFI = 1,          // Scan kanal rentang Wi-Fi (1 - 73)
    SCAN_BAND_BT = 2             // Scan kanal rentang Bluetooth (2 - 80)
};

// =============================================================================
// STRUKTUR STATE GLOBAL
// =============================================================================

struct AppState {
    // Mode UI & Operasi
    AppMode appMode = APP_MODE_MENU;

    // ----- JAMMER STATE -----
    volatile bool jamming = false;
    JammerTarget jammerTarget = JAM_TARGET_WIFI;
    int jammerMinCh = WIFI_MIN_CH;
    int jammerMaxCh = WIFI_MAX_CH;
    int currentJamChannel = 37;

    // Parameter Radio Transmisi (Auto Aggressive)
    rf24_pa_dbm_e powerLevel = DEFAULT_POWER;
    rf24_datarate_e dataRate = DEFAULT_RATE;

    // ----- ANALYZER STATE -----
    AnalyzerBand analyzerBand = SCAN_BAND_ALL;
    uint8_t spectrumLevels[TOTAL_CHANNELS];  // Aktivitas RF saat ini (0 - 100%)
    uint8_t peakLevels[TOTAL_CHANNELS];      // Nilai Peak Hold (0 - 100%)
    int peakChannel = 0;                     // Kanal dengan RF tertinggi
    uint8_t peakLevel = 0;                   // Nilai RF tertinggi saat ini (%)

    // Inspeksi Kanal Tunggal
    int inspectedChannel = 36;               // Default Wi-Fi Ch 6
    uint8_t inspectedLevel = 0;              // Aktivitas real-time %
    uint8_t inspectedPeak = 0;               // Peak aktivitas %

    // ----- HELPER METHODS -----
    void setJammerTarget(JammerTarget target);
    bool setJammerTargetByName(const String& name);
    const char* getJammerTargetName() const;
    const char* getJammerFreqRangeStr() const;
    void cycleJammerTarget(int direction = 1);

    void cycleAnalyzerBand(int direction = 1);
    const char* getAnalyzerBandName() const;
    void getAnalyzerChannelRange(int &minCh, int &maxCh) const;
    void resetPeaks();
    void decayPeaks();
};

extern AppState appState;
