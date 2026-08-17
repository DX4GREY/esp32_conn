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
            currentJamChannel = bluetooth_even_channels[0];
            break;
        case JAM_TARGET_BLE_ADV:
            jammerMinCh = BT_MIN_CH;
            jammerMaxCh = BT_MAX_CH;
            currentJamChannel = BLE_ADV_CHANNELS[0];
            break;
        case JAM_TARGET_BLE_DATA:
            jammerMinCh = BT_MIN_CH;
            jammerMaxCh = BT_MAX_CH;
            currentJamChannel = BLE_DATA_CHANNELS[0];
            break;
        case JAM_TARGET_ALL:
            jammerMinCh = MIN_CHANNEL;
            jammerMaxCh = MAX_CHANNEL;
            currentJamChannel = (MIN_CHANNEL + MAX_CHANNEL) / 2;
            break;
        case JAM_TARGET_ZIGBEE:
            jammerMinCh = 11;
            jammerMaxCh = 26;
            currentJamChannel = 18;
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
    } else if (n == "bledata" || n == "ble-data") {
        setJammerTarget(JAM_TARGET_BLE_DATA);
        return true;
    } else if (n == "all" || n == "full" || n == "drone" || n == "allband") {
        setJammerTarget(JAM_TARGET_ALL);
        return true;
    } else if (n == "zigbee") {
        setJammerTarget(JAM_TARGET_ZIGBEE);
        return true;
    }
    return false;
}

const char* AppState::getJammerTargetName() const {
    switch (jammerTarget) {
        case JAM_TARGET_WIFI:     return "Wi-Fi (50 Hop)";
        case JAM_TARGET_BT:       return "Bluetooth (1-80)";
        case JAM_TARGET_BLE_ADV:  return "BLE Advertising";
        case JAM_TARGET_BLE_DATA: return "BLE Data (2..80)";
        case JAM_TARGET_ALL:      return "All Band / Drone";
        case JAM_TARGET_ZIGBEE:   return "Zigbee (11-26)";
        default:                  return "Unknown";
    }
}

const char* AppState::getJammerFreqRangeStr() const {
    switch (jammerTarget) {
        case JAM_TARGET_WIFI:     return "2406-2468MHz (50 Ch)";
        case JAM_TARGET_BT:       return "2401-2480MHz (80 Hop)";
        case JAM_TARGET_BLE_ADV:  return "2401-2481MHz (9 Ch)";
        case JAM_TARGET_BLE_DATA: return "12 Ch (3 Groups)";
        case JAM_TARGET_ALL:      return "2401-2500MHz (1-100)";
        case JAM_TARGET_ZIGBEE:   return "2405-2480MHz (16 Ch)";
        default:                  return "";
    }
}

void AppState::cycleJammerTarget(int direction) {
    int current = (int)jammerTarget;
    int total = 6;
    current = (current + direction + total) % total;
    setJammerTarget((JammerTarget)current);
}

void AppState::cyclePowerLevel(int direction) {
    int cur = (int)powerLevel;
    // RF24 PA levels: RF24_PA_MIN(0), RF24_PA_LOW(1), RF24_PA_HIGH(2), RF24_PA_MAX(3)
    cur = (cur + direction + 4) % 4;
    powerLevel = (rf24_pa_dbm_e)cur;
}

bool AppState::setPowerLevelByName(const String& name) {
    String n = name;
    n.trim();
    n.toLowerCase();

    if (n == "min" || n == "-18" || n == "-18dbm") {
        powerLevel = RF24_PA_MIN;
        return true;
    } else if (n == "low" || n == "-12" || n == "-12dbm") {
        powerLevel = RF24_PA_LOW;
        return true;
    } else if (n == "high" || n == "-6" || n == "-6dbm") {
        powerLevel = RF24_PA_HIGH;
        return true;
    } else if (n == "max" || n == "0" || n == "0dbm" || n == "full") {
        powerLevel = RF24_PA_MAX;
        return true;
    }
    return false;
}

const char* AppState::getPowerLevelName() const {
    switch (powerLevel) {
        case RF24_PA_MIN:  return "MIN (-18dBm)";
        case RF24_PA_LOW:  return "LOW (-12dBm)";
        case RF24_PA_HIGH: return "HIGH (-6dBm)";
        case RF24_PA_MAX:  return "MAX (0dBm)";
        default:           return "MAX (0dBm)";
    }
}

const char* AppState::getPowerLevelDbmStr() const {
    switch (powerLevel) {
        case RF24_PA_MIN:  return "-18 dBm";
        case RF24_PA_LOW:  return "-12 dBm";
        case RF24_PA_HIGH: return "-6 dBm";
        case RF24_PA_MAX:  return "0 dBm / MAX";
        default:           return "0 dBm";
    }
}

void AppState::cycleDwellTime(int direction) {
    int idx = 2; // default is 200us (index 2)
    for (int i = 0; i < DWELL_PRESETS_COUNT; i++) {
        if (dwellTimeUs == DWELL_PRESETS[i]) {
            idx = i;
            break;
        }
    }
    idx = (idx + direction + DWELL_PRESETS_COUNT) % DWELL_PRESETS_COUNT;
    dwellTimeUs = DWELL_PRESETS[idx];
}

bool AppState::setDwellTime(int us) {
    if (us >= 10 && us <= 10000) {
        dwellTimeUs = us;
        return true;
    }
    return false;
}

const char* AppState::getDwellTimeName() const {
    switch (dwellTimeUs) {
        case 50:   return "50 us (Ultra Fast)";
        case 100:  return "100 us (Fast)";
        case 200:  return "200 us (Balanced)";
        case 500:  return "500 us (Heavy)";
        case 1000: return "1000 us (1 ms)";
        default:   return "Custom";
    }
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
