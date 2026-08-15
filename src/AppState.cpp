#include "AppState.h"

AppState appState;

void AppState::setJammerTarget(JammerTarget target) {
    jammerTarget = target;
    switch (jammerTarget) {
        case JAM_TARGET_WIFI:
            jammerMinCh = WIFI_MIN_CH;
            jammerMaxCh = WIFI_MAX_CH;
            currentJamChannel = (WIFI_MIN_CH + WIFI_MAX_CH) / 2;
            break;
        case JAM_TARGET_BT:
            jammerMinCh = BT_MIN_CH;
            jammerMaxCh = BT_MAX_CH;
            currentJamChannel = (BT_MIN_CH + BT_MAX_CH) / 2;
            break;
        case JAM_TARGET_BLE_ADV:
            jammerMinCh = BT_MIN_CH;
            jammerMaxCh = BT_MAX_CH;
            currentJamChannel = BLE_ADV_CHANNELS[0];
            break;
        case JAM_TARGET_ALL:
        default:
            jammerMinCh = MIN_CHANNEL;
            jammerMaxCh = MAX_CHANNEL;
            currentJamChannel = (MIN_CHANNEL + MAX_CHANNEL) / 2;
            break;
    }
}

bool AppState::setJammerTargetByName(const String& name) {
    String n = name;
    n.trim();
    n.toLowerCase();

    if (n == "wifi" || n == "wi-fi") {
        setJammerTarget(JAM_TARGET_WIFI);
        return true;
    } else if (n == "bt" || n == "bluetooth") {
        setJammerTarget(JAM_TARGET_BT);
        return true;
    } else if (n == "ble" || n == "bleadv" || n == "ble-adv") {
        setJammerTarget(JAM_TARGET_BLE_ADV);
        return true;
    } else if (n == "all" || n == "full" || n == "allband") {
        setJammerTarget(JAM_TARGET_ALL);
        return true;
    }
    return false;
}

const char* AppState::getJammerTargetName() const {
    switch (jammerTarget) {
        case JAM_TARGET_WIFI:    return "Wi-Fi (2.4 GHz)";
        case JAM_TARGET_BT:      return "Bluetooth (BT)";
        case JAM_TARGET_BLE_ADV: return "BLE Advertising";
        case JAM_TARGET_ALL:     return "All 2.4GHz Band";
        default:                 return "Unknown";
    }
}

const char* AppState::getJammerFreqRangeStr() const {
    switch (jammerTarget) {
        case JAM_TARGET_WIFI:    return "2401-2473 MHz (Ch 1-73)";
        case JAM_TARGET_BT:      return "2402-2480 MHz (Ch 2-80)";
        case JAM_TARGET_BLE_ADV: return "2402/2426/2480 MHz";
        case JAM_TARGET_ALL:     return "2400-2525 MHz (Ch 0-125)";
        default:                 return "";
    }
}

void AppState::cycleJammerTarget(int direction) {
    int current = (int)jammerTarget;
    int total = 4;
    current = (current + direction + total) % total;
    setJammerTarget((JammerTarget)current);
}

void AppState::cycleAnalyzerBand(int direction) {
    int current = (int)analyzerBand;
    int total = 3;
    current = (current + direction + total) % total;
    analyzerBand = (AnalyzerBand)current;
    resetPeaks();
}

const char* AppState::getAnalyzerBandName() const {
    switch (analyzerBand) {
        case SCAN_BAND_ALL:  return "ALL (0-125)";
        case SCAN_BAND_WIFI: return "Wi-Fi (1-73)";
        case SCAN_BAND_BT:   return "Bluetooth (2-80)";
        default:             return "ALL";
    }
}

void AppState::getAnalyzerChannelRange(int &minCh, int &maxCh) const {
    switch (analyzerBand) {
        case SCAN_BAND_WIFI:
            minCh = WIFI_MIN_CH;
            maxCh = WIFI_MAX_CH;
            break;
        case SCAN_BAND_BT:
            minCh = BT_MIN_CH;
            maxCh = BT_MAX_CH;
            break;
        case SCAN_BAND_ALL:
        default:
            minCh = MIN_CHANNEL;
            maxCh = MAX_CHANNEL;
            break;
    }
}

void AppState::resetPeaks() {
    for (int i = 0; i < TOTAL_CHANNELS; i++) {
        spectrumLevels[i] = 0;
        peakLevels[i] = 0;
    }
    peakChannel = 0;
    peakLevel = 0;
    inspectedPeak = 0;
}

void AppState::decayPeaks() {
    for (int i = 0; i < TOTAL_CHANNELS; i++) {
        if (peakLevels[i] > spectrumLevels[i]) {
            if (peakLevels[i] >= PEAK_DECAY_RATE) {
                peakLevels[i] -= PEAK_DECAY_RATE;
            } else {
                peakLevels[i] = 0;
            }
        }
    }
}
