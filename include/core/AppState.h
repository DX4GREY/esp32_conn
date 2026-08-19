#pragma once
#include <Arduino.h>
#include <RF24.h>
#include "config/Config.h"
#include "core/AppTypes.h"

// =============================================================================
// GLOBAL APPLICATION STATE
// =============================================================================

struct AppState {
    // Mode UI & Operasi
    AppMode appMode = APP_MODE_MENU;

    // ----- JAMMER STATE -----
    volatile bool jamming = false;
    volatile JammerTarget jammerTarget = JAM_TARGET_WIFI;
    int jammerMinCh = WIFI_MIN_CH;
    int jammerMaxCh = WIFI_MAX_CH;
    int currentJamChannel = 37;

    // Radio Transmission Parameters (Dynamic RF Settings)
    rf24_pa_dbm_e powerLevel = DEFAULT_POWER;
    rf24_datarate_e dataRate = DEFAULT_RATE;
    volatile int dwellTimeUs = JAMMER_DWELL_US;

    // ----- ANALYZER STATE -----
    AnalyzerBand analyzerBand = SCAN_BAND_ALL;
    AnalyzerRadioMode analyzerRadioMode = ANALYZER_RADIO_FAST;
    ScanProfile scanProfile = SCAN_PROFILE_BALANCED;
    int customSpectrumSamples = 40;
    uint8_t spectrumLevels[TOTAL_CHANNELS];  // Current RF activity (0 - 100%)
    uint8_t peakLevels[TOTAL_CHANNELS];      // Peak Hold Value (0 - 100%)
    uint8_t radio1Levels[TOTAL_CHANNELS];    // Per-radio activity for diagnostics
    uint8_t radio2Levels[TOTAL_CHANNELS];
    int peakChannel = 0;                     // Channel with the highest RF
    uint8_t peakLevel = 0;                   // Current highest RF value (%)
    uint8_t waterfall[WATERFALL_ROWS][TOTAL_CHANNELS];
    uint8_t waterfallHead = 0;
    uint8_t waterfallCount = 0;
    uint32_t occupancyTotal[TOTAL_CHANNELS];
    uint32_t surveySweeps = 0;
    RfEvent rfEvents[RF_EVENT_COUNT];
    uint8_t eventHead = 0;
    uint8_t eventCount = 0;
    unsigned long lastEventMs = 0;
    bool loggingEnabled = false;

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

    // RF Settings Helpers
    void cyclePowerLevel(int direction = 1);
    bool setPowerLevelByName(const String& name);
    const char* getPowerLevelName() const;
    const char* getPowerLevelDbmStr() const;

    void cycleDwellTime(int direction = 1);
    bool setDwellTime(int us);
    const char* getDwellTimeName() const;

    void cycleAnalyzerBand(int direction = 1);
    const char* getAnalyzerBandName() const;
    void cycleAnalyzerRadioMode(int direction = 1);
    const char* getAnalyzerRadioModeName() const;
    void cycleScanProfile(int direction = 1);
    const char* getScanProfileName() const;
    int getSpectrumSampleCount() const;
    int getInspectSampleCount() const;
    void cycleCustomSampleCount();
    void recordCompletedSweep();
    void resetSurvey();
    void clearEvents();
    void getAnalyzerChannelRange(int &minCh, int &maxCh) const;
    void resetPeaks();
    void decayPeaks();

    // NVS Persistence
    void loadSettings();
    void saveSettings();
};

extern AppState appState;
