#include "core/AppState.h"

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

void AppState::cycleAnalyzerRadioMode(int direction) {
    int current = static_cast<int>(analyzerRadioMode);
    constexpr int total = 4;
    current = (current + direction + total) % total;
    analyzerRadioMode = static_cast<AnalyzerRadioMode>(current);
    resetPeaks();
}

const char* AppState::getAnalyzerRadioModeName() const {
    switch (analyzerRadioMode) {
        case ANALYZER_RADIO_FAST:      return "FAST";
        case ANALYZER_RADIO_DIVERSITY: return "DIV";
        case ANALYZER_RADIO_1:         return "R1";
        case ANALYZER_RADIO_2:         return "R2";
        default:                       return "FAST";
    }
}

void AppState::cycleScanProfile(int direction) {
    int current = static_cast<int>(scanProfile);
    constexpr int total = 4;
    current = (current + direction + total) % total;
    scanProfile = static_cast<ScanProfile>(current);
    resetPeaks();
    saveSettings();
}

const char* AppState::getScanProfileName() const {
    switch (scanProfile) {
        case SCAN_PROFILE_FAST:     return "FAST";
        case SCAN_PROFILE_DEEP:     return "DEEP";
        case SCAN_PROFILE_CUSTOM:   return "CUSTOM";
        case SCAN_PROFILE_BALANCED:
        default:                    return "BALANCED";
    }
}

int AppState::getSpectrumSampleCount() const {
    switch (scanProfile) {
        case SCAN_PROFILE_FAST: return 12;
        case SCAN_PROFILE_DEEP: return 60;
        case SCAN_PROFILE_CUSTOM: return customSpectrumSamples;
        default:                return SPECTRUM_SAMPLES_PER_CH;
    }
}

int AppState::getInspectSampleCount() const {
    switch (scanProfile) {
        case SCAN_PROFILE_FAST: return 50;
        case SCAN_PROFILE_DEEP: return 200;
        case SCAN_PROFILE_CUSTOM: return constrain(customSpectrumSamples * 2, 20, 200);
        default:                return INSPECT_SAMPLES;
    }
}

void AppState::cycleCustomSampleCount() {
    customSpectrumSamples += 10;
    if (customSpectrumSamples > 100) customSpectrumSamples = 10;
    resetPeaks();
    saveSettings();
}

void AppState::recordCompletedSweep() {
    for (int ch = 0; ch < TOTAL_CHANNELS; ch++) {
        waterfall[waterfallHead][ch] = spectrumLevels[ch];
        occupancyTotal[ch] += spectrumLevels[ch];
    }
    waterfallHead = (waterfallHead + 1) % WATERFALL_ROWS;
    if (waterfallCount < WATERFALL_ROWS) waterfallCount++;
    surveySweeps++;

    const unsigned long now = millis();
    if (peakLevel >= 60 && now - lastEventMs >= 750) {
        rfEvents[eventHead].timestampMs = now;
        rfEvents[eventHead].channel = static_cast<uint8_t>(peakChannel);
        rfEvents[eventHead].level = peakLevel;
        eventHead = (eventHead + 1) % RF_EVENT_COUNT;
        if (eventCount < RF_EVENT_COUNT) eventCount++;
        lastEventMs = now;
    }

    if (loggingEnabled) {
        Serial.printf("RFLOG,%lu,%lu,%d,%u,%s,%s\n", now,
                      static_cast<unsigned long>(surveySweeps), peakChannel,
                      peakLevel, getAnalyzerBandName(), getAnalyzerRadioModeName());
    }
}

void AppState::resetSurvey() {
    memset(occupancyTotal, 0, sizeof(occupancyTotal));
    surveySweeps = 0;
}

void AppState::clearEvents() {
    memset(rfEvents, 0, sizeof(rfEvents));
    eventHead = 0;
    eventCount = 0;
    lastEventMs = 0;
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
        radio1Levels[i] = 0;
        radio2Levels[i] = 0;
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

// =============================================================================
// NVS PERSISTENCE
// =============================================================================
