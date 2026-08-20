#pragma once

#include <Arduino.h>

struct AppState;
class RfEnvironmentState;

class SessionRecorder {
public:
    bool begin();
    bool start();
    void stop();
    void service();
    void recordSweep(const AppState& state);
    void recordEnvironmentSummary(const RfEnvironmentState& state, const char* testType);
    void recordProbeSummary(uint8_t channel, uint8_t pa, uint8_t rate,
                            uint8_t size, uint16_t packets, uint16_t intervalMs,
                            uint32_t durationMs);
    bool exportCsv(Stream& output);
    bool replayLatest(AppState& state);
    bool isReady() const { return ready; }
    bool isRecording() const { return recording; }
    size_t fileSize() const;
    uint32_t recordedSweeps() const { return sweepCount; }
    const char* lastError() const { return errorMessage; }

private:
    bool flushPending();
    bool ready = false;
    bool recording = false;
    String pending;
    uint32_t sweepCount = 0;
    unsigned long lastFlushMs = 0;
    const char* errorMessage = "not initialized";
};

extern SessionRecorder sessionRecorder;
