#include "core/AppState.h"
#include <Preferences.h>

void AppState::loadSettings() {
    Preferences prefs;
    prefs.begin("appstate", false);
    powerLevel = (rf24_pa_dbm_e)prefs.getInt("power", DEFAULT_POWER);
    dwellTimeUs = prefs.getInt("dwell", JAMMER_DWELL_US);
    jammerTarget = (JammerTarget)prefs.getUChar("target", JAM_TARGET_WIFI);
    uint8_t storedProfile = prefs.getUChar("profile", SCAN_PROFILE_BALANCED);
    scanProfile = storedProfile <= SCAN_PROFILE_CUSTOM ?
                  static_cast<ScanProfile>(storedProfile) : SCAN_PROFILE_BALANCED;
    customSpectrumSamples = constrain(prefs.getInt("custom", 40), 10, 100);
    prefs.end();
    // Recompute jammer channel range & starting channel for the loaded target
    setJammerTarget(jammerTarget);
}

void AppState::saveSettings() {
    Preferences prefs;
    prefs.begin("appstate", false);
    prefs.putInt("power", (int)powerLevel);
    prefs.putInt("dwell", dwellTimeUs);
    prefs.putUChar("target", (uint8_t)jammerTarget);
    prefs.putUChar("profile", static_cast<uint8_t>(scanProfile));
    prefs.putInt("custom", customSpectrumSamples);
    prefs.end();
}
