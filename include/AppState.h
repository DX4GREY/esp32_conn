#pragma once
#include <Arduino.h>
#include <RF24.h>
#include "Config.h"

// =============================================================================
// ENUMERASI STATE APLIKASI
// =============================================================================

enum AppMode {
    APP_MODE_MENU,               // Main Menu
    APP_MODE_JAMMER,             // Fast Multi-Target Jammer Mode
    APP_MODE_ANALYZER_SPECTRUM,  // Radio Analyzer Mode: Live RF Spectrum Graph
    APP_MODE_ANALYZER_CHANNEL,   // Radio Analyzer Mode: Deep Inspection of 1 Channel
    APP_MODE_STATUS,             // Device Status Information
    APP_MODE_REBOOT              // System Reboot / Restart
};

enum JammerTarget {
    JAM_TARGET_WIFI = 0,         // Wi-Fi 2.4 GHz Target (50 Programmed Channels)
    JAM_TARGET_BT = 1,           // Bluetooth Classic Target (Ch 1 - 80 Even+Odd Hop)
    JAM_TARGET_BLE_ADV = 2,      // BLE Advertising Target (Ch 1-3, 25-27, 79-81)
    JAM_TARGET_BLE_DATA = 3,     // BLE Data Target (12 Ch from 3 Groups)
    JAM_TARGET_ALL = 4,          // Full 2.4 GHz Band / Drone Target (Ch 1 - 100)
    JAM_TARGET_ZIGBEE = 5        // Zigbee Target (Ch 11 - 26)
};

enum AnalyzerBand {
    SCAN_BAND_ALL = 0,           // Scan all 126 channels (0 - 125)
    SCAN_BAND_WIFI = 1,          // Scan Wi-Fi channel range (1 - 73)
    SCAN_BAND_BT = 2             // Scan Bluetooth channel range (2 - 80)
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

    // Radio Transmission Parameters (Auto Aggressive)
    rf24_pa_dbm_e powerLevel = DEFAULT_POWER;
    rf24_datarate_e dataRate = DEFAULT_RATE;

    // ----- ANALYZER STATE -----
    AnalyzerBand analyzerBand = SCAN_BAND_ALL;
    uint8_t spectrumLevels[TOTAL_CHANNELS];  // Current RF activity (0 - 100%)
    uint8_t peakLevels[TOTAL_CHANNELS];      // Peak Hold Value (0 - 100%)
    int peakChannel = 0;                     // Channel with the highest RF
    uint8_t peakLevel = 0;                   // Current highest RF value (%)

    // Single Channel Inspection
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
