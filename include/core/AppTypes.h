#pragma once

#include <Arduino.h>

// Application screens. Keeping the modes in a dependency-light header lets
// navigation and menu metadata evolve without pulling the full global state
// into every module.
enum AppMode {
    APP_MODE_MENU,
    APP_MODE_JAMMER,
    APP_MODE_ANALYZER_SPECTRUM,
    APP_MODE_WATERFALL,
    APP_MODE_ANALYZER_CHANNEL,
    APP_MODE_SURVEY,
    APP_MODE_EVENTS,
    APP_MODE_LOGGING,
    APP_MODE_RADIO_DIAG,
    APP_MODE_PROFILES,
    APP_MODE_SETTINGS,
    APP_MODE_STATUS,
    APP_MODE_POWER,
    APP_MODE_REBOOT,
    APP_MODE_SHUTDOWN
};

enum JammerTarget {
    JAM_TARGET_WIFI = 0,
    JAM_TARGET_BT,
    JAM_TARGET_BLE_ADV,
    JAM_TARGET_BLE_DATA,
    JAM_TARGET_ALL,
    JAM_TARGET_ZIGBEE
};

enum AnalyzerBand {
    SCAN_BAND_ALL = 0,
    SCAN_BAND_WIFI,
    SCAN_BAND_BT
};

enum AnalyzerRadioMode {
    ANALYZER_RADIO_FAST = 0,
    ANALYZER_RADIO_DIVERSITY,
    ANALYZER_RADIO_1,
    ANALYZER_RADIO_2
};

enum ScanProfile {
    SCAN_PROFILE_FAST = 0,
    SCAN_PROFILE_BALANCED,
    SCAN_PROFILE_DEEP,
    SCAN_PROFILE_CUSTOM
};

enum DisplayThemeId {
    DISPLAY_THEME_CYBER = 0,
    DISPLAY_THEME_OCEAN,
    DISPLAY_THEME_AMBER,
    DISPLAY_THEME_MATRIX,
    DISPLAY_THEME_VIOLET,
    DISPLAY_THEME_ICE,
    DISPLAY_THEME_COUNT
};

constexpr int WATERFALL_ROWS = 24;
constexpr int RF_EVENT_COUNT = 8;

struct RfEvent {
    unsigned long timestampMs = 0;
    uint8_t channel = 0;
    uint8_t level = 0;
};
