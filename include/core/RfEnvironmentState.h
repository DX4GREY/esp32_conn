#pragma once
#include <Arduino.h>
#include "config/Config.h"

constexpr uint8_t RF_ENV_HISTORY_BUCKETS = 32;
constexpr uint8_t RF_ENV_BURST_EVENTS = 32;
constexpr uint8_t RF_ENV_COMPARE_MAX = 4;

enum RfEnvMode : uint8_t { RF_ENV_IDLE, RF_ENV_OCCUPANCY, RF_ENV_COMPARE };
enum RfBurstSeverity : uint8_t { RF_BURST_LOW, RF_BURST_MEDIUM, RF_BURST_HIGH };

struct RfChannelStats {
    uint32_t sampleCount = 0;
    uint32_t carrierHits = 0;
    uint32_t lastActivityMs = 0;
    uint16_t consecutiveActive = 0;
    uint16_t burstCount = 0;
    uint8_t occupancy = 0;
    uint8_t movingAverage = 0;
    uint8_t peak = 0;
    uint8_t minimum = 100;
    uint8_t maximum = 0;
    uint8_t persistence = 0;
    uint8_t score = 0;
};

struct RfBurstEvent {
    uint32_t id = 0, timestampMs = 0, durationMs = 0;
    uint16_t frequencyMHz = 2400;
    uint8_t channel = 0, peak = 0, baseline = 0, delta = 0;
    RfBurstSeverity severity = RF_BURST_LOW;
};

struct RfEnvironmentSnapshot {
    bool valid = false;
    uint32_t capturedMs = 0;
    uint16_t burstCount = 0;
    uint8_t average = 0, peak = 0, peakChannel = 0, score = 0;
    uint8_t channelOccupancy[TOTAL_CHANNELS] = {};
};

struct RfEnvironmentConfig {
    uint8_t minChannel = 0, maxChannel = 125;
    uint8_t emaAlpha = 25, burstThreshold = 25;
    uint8_t compareCount = 2, historyDepth = RF_ENV_HISTORY_BUCKETS;
    uint8_t compareChannels[RF_ENV_COMPARE_MAX] = {6, 11, 42, 80};
    uint16_t sampleWindowSeconds = 10;
#if RF_LAB_TX_ENABLED
    uint8_t probeChannel = 42, probePayloadSize = 8, probePa = RF24_PA_LOW, probeDataRate = RF24_1MBPS;
    uint16_t probeIntervalMs = 100, probePacketCount = 100;
    uint16_t probeMaxDurationSeconds = 10;
#endif
};

class RfEnvironmentState {
public:
    RfEnvironmentConfig config;
    RfChannelStats channels[TOTAL_CHANNELS];
    uint8_t history[RF_ENV_HISTORY_BUCKETS][TOTAL_CHANNELS] = {};
    uint8_t historyHead = 0, historyCount = 0;
    RfBurstEvent events[RF_ENV_BURST_EVENTS];
    uint8_t eventHead = 0, eventCount = 0;
    RfEnvironmentSnapshot before, after;
    volatile bool running = false;
    volatile RfEnvMode mode = RF_ENV_IDLE;
    uint32_t startedMs = 0, completedCycles = 0, samplesPerSecond = 0;
    uint32_t lastCycleUs = 0, maxCycleUs = 0;
    void resetRuntime();
    void captureSnapshot(RfEnvironmentSnapshot& target);
    void topChannels(uint8_t* output, uint8_t count) const;
    uint8_t averageOccupancy() const;
    uint8_t overallScore() const;
    const char* scoreLabel(uint8_t score) const;
};
extern RfEnvironmentState rfEnvironmentState;
