#include "core/AppState.h"
#include <Preferences.h>

namespace {
constexpr uint8_t SETTINGS_SCHEMA_VERSION = 2;
constexpr unsigned long SETTINGS_SAVE_DELAY_MS = 1500;
}

void AppState::loadSettings() {
    Preferences prefs;
    prefs.begin("appstate", false);
    const uint8_t storedSchema = prefs.getUChar("schema", 0);

    const int storedPower = prefs.getInt("power", DEFAULT_POWER);
    powerLevel = storedPower >= RF24_PA_MIN && storedPower <= RF24_PA_MAX ?
                 static_cast<rf24_pa_dbm_e>(storedPower) : DEFAULT_POWER;
    dwellTimeUs = constrain(prefs.getInt("dwell", JAMMER_DWELL_US), 10, 10000);
    const uint8_t storedTarget = prefs.getUChar("target", JAM_TARGET_WIFI);
    jammerTarget = storedTarget <= JAM_TARGET_ZIGBEE ?
                   static_cast<JammerTarget>(storedTarget) : JAM_TARGET_WIFI;

    const uint8_t storedProfile = prefs.getUChar("profile", SCAN_PROFILE_BALANCED);
    scanProfile = storedProfile <= SCAN_PROFILE_CUSTOM ?
                  static_cast<ScanProfile>(storedProfile) : SCAN_PROFILE_BALANCED;
    customSpectrumSamples = constrain(prefs.getInt("custom", 40), 10, 100);

    const uint8_t storedTheme = prefs.getUChar("theme", DISPLAY_THEME_CYBER);
    displayTheme = storedTheme < DISPLAY_THEME_COUNT ?
                   static_cast<DisplayThemeId>(storedTheme) : DISPLAY_THEME_CYBER;

    if (storedSchema >= 2) {
        const uint8_t storedTrace = prefs.getUChar("trace", ANALYZER_TRACE_LIVE);
        analyzerTraceMode = storedTrace < ANALYZER_TRACE_COUNT ?
                            static_cast<AnalyzerTraceMode>(storedTrace) :
                            ANALYZER_TRACE_LIVE;
        // A baseline describes the current RF environment and intentionally
        // lives in RAM. Never restore DELTA without a matching baseline.
        if (analyzerTraceMode == ANALYZER_TRACE_DELTA) {
            analyzerTraceMode = ANALYZER_TRACE_LIVE;
        }
        eventThreshold = constrain(prefs.getUChar("evt_thr", 60), 5, 100);
        eventHysteresis = constrain(prefs.getUChar("evt_hys", 10), 0, eventThreshold);
        eventMinSweeps = constrain(prefs.getUChar("evt_dur", 2), 1, 20);
        eventMinChannels = constrain(prefs.getUChar("evt_multi", 1), 1, 16);
        if (prefs.getBytesLength("watch") == sizeof(watchedChannels)) {
            prefs.getBytes("watch", watchedChannels, sizeof(watchedChannels));
        }
    }
    prefs.end();

    setJammerTarget(jammerTarget);
    settingsDirty = storedSchema != SETTINGS_SCHEMA_VERSION;
    settingsDirtySinceMs = millis();
}

void AppState::saveSettings() {
    Preferences prefs;
    if (!prefs.begin("appstate", false)) return;
    prefs.putUChar("schema", SETTINGS_SCHEMA_VERSION);
    prefs.putInt("power", static_cast<int>(powerLevel));
    prefs.putInt("dwell", dwellTimeUs);
    prefs.putUChar("target", static_cast<uint8_t>(jammerTarget));
    prefs.putUChar("profile", static_cast<uint8_t>(scanProfile));
    prefs.putInt("custom", customSpectrumSamples);
    prefs.putUChar("theme", static_cast<uint8_t>(displayTheme));
    prefs.putUChar("trace", static_cast<uint8_t>(analyzerTraceMode));
    prefs.putUChar("evt_thr", eventThreshold);
    prefs.putUChar("evt_hys", eventHysteresis);
    prefs.putUChar("evt_dur", eventMinSweeps);
    prefs.putUChar("evt_multi", eventMinChannels);
    prefs.putBytes("watch", watchedChannels, sizeof(watchedChannels));
    prefs.end();
    settingsDirty = false;
}

void AppState::markSettingsDirty() {
    settingsDirty = true;
    settingsDirtySinceMs = millis();
}

void AppState::serviceSettingsPersistence() {
    if (settingsDirty && millis() - settingsDirtySinceMs >= SETTINGS_SAVE_DELAY_MS) {
        saveSettings();
    }
}

void AppState::factoryResetSettings() {
    Preferences prefs;
    if (prefs.begin("appstate", false)) {
        prefs.clear();
        prefs.end();
    }
    powerLevel = DEFAULT_POWER;
    dwellTimeUs = JAMMER_DWELL_US;
    displayTheme = DISPLAY_THEME_CYBER;
    scanProfile = SCAN_PROFILE_BALANCED;
    customSpectrumSamples = 40;
    analyzerTraceMode = ANALYZER_TRACE_LIVE;
    configureEventEngine(60, 10, 2, 1);
    memset(watchedChannels, 0, sizeof(watchedChannels));
    setJammerTarget(JAM_TARGET_WIFI);
    saveSettings();
}
