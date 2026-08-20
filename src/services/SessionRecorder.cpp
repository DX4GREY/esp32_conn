#include "services/SessionRecorder.h"
#include "core/AppState.h"
#include "core/RfEnvironmentState.h"
#include "drivers/RadioManager.h"
#include <LittleFS.h>

SessionRecorder sessionRecorder;

namespace {
constexpr const char* SESSION_PATH = "/rf_session.csv";
constexpr size_t MAX_SESSION_BYTES = 256U * 1024U;
constexpr size_t FLUSH_THRESHOLD = 3072;
constexpr unsigned long FLUSH_INTERVAL_MS = 2000;
}

bool SessionRecorder::begin() {
    ready = LittleFS.begin(true);
    errorMessage = ready ? "none" : "LittleFS mount failed";
    return ready;
}

bool SessionRecorder::start() {
    if (!ready) {
        errorMessage = "LittleFS unavailable";
        return false;
    }
    LittleFS.remove(SESSION_PATH);
    File file = LittleFS.open(SESSION_PATH, FILE_WRITE);
    if (!file) {
        errorMessage = "cannot create session";
        return false;
    }
    file.println("# RF24 analyzer session v1");
    file.println("# ACTIVITY values are carrier-hit percentages, not dBm");
    file.printf("# firmware_build=%s compiled=%s %s\n", RF_LAB_TX_ENABLED ? "authorized_rf_lab" : "analyzer", __DATE__, __TIME__);
    file.println("# E: type,ms,test,start_ms,duration_ms,radios,min_ch,max_ch,window_s,avg,peak_ch,peak_pct,score,bursts,top5");
    file.println("# P: type,ms,channel,pa,data_rate,payload_size,packets,interval_ms,duration_ms");
    file.println("type,ms,sweep,peak_ch,peak_pct,confidence,band,mode,trace,ch0..ch125");
    file.close();
    pending.reserve(4096);
    pending = "";
    sweepCount = 0;
    lastFlushMs = millis();
    recording = true;
    errorMessage = "none";
    return true;
}

void SessionRecorder::stop() {
    flushPending();
    recording = false;
}

bool SessionRecorder::flushPending() {
    if (!ready || pending.length() == 0) return ready;
    File file = LittleFS.open(SESSION_PATH, FILE_APPEND);
    if (!file) {
        errorMessage = "session append failed";
        recording = false;
        return false;
    }
    const size_t written = file.print(pending);
    file.close();
    if (written != pending.length()) {
        errorMessage = "short filesystem write";
        recording = false;
        return false;
    }
    pending = "";
    lastFlushMs = millis();
    return true;
}

void SessionRecorder::service() {
    if (recording && pending.length() > 0 &&
        millis() - lastFlushMs >= FLUSH_INTERVAL_MS) flushPending();
}

void SessionRecorder::recordSweep(const AppState& state) {
    if (!recording) return;
    if (fileSize() + pending.length() >= MAX_SESSION_BYTES) {
        errorMessage = "session size limit reached";
        stop();
        return;
    }

    String line;
    line.reserve(560);
    line = "S," + String(millis()) + "," + String(state.surveySweeps) + "," +
           String(state.peakChannel) + "," + String(state.peakLevel) + "," +
           String(state.analyzerConfidence) + "," + String(state.analyzerBand) + "," +
           String(state.analyzerRadioMode) + "," + String(state.analyzerTraceMode);
    for (int ch = 0; ch < TOTAL_CHANNELS; ch++) {
        line += ',';
        line += String(state.spectrumLevels[ch]);
    }
    line += '\n';
    pending += line;
    sweepCount++;
    if (pending.length() >= FLUSH_THRESHOLD) flushPending();
}

void SessionRecorder::recordEnvironmentSummary(const RfEnvironmentState& state, const char* testType) {
    if (!recording) return;
    uint8_t top[5]; state.topChannels(top, 5);
    uint32_t bursts=0; for(int ch=0;ch<TOTAL_CHANNELS;ch++) bursts+=state.channels[ch].burstCount;
    String line; line.reserve(240); line="E,"+String(millis())+","+testType+","+
        String(state.startedMs)+","+String(millis()-state.startedMs)+","+
        String(radioManager.availableRadioCount())+","+String(state.config.minChannel)+","+
        String(state.config.maxChannel)+","+String(state.config.sampleWindowSeconds)+","+
        String(state.averageOccupancy())+","+String(top[0])+","+
        String(state.channels[top[0]].peak)+","+String(state.overallScore())+","+String(bursts);
    for(int i=0;i<5;i++){line+=',';line+=String(top[i]);line+=':';line+=String(state.channels[top[i]].movingAverage);} line+='\n';
    pending += line; if(pending.length()>=FLUSH_THRESHOLD) flushPending();
}
void SessionRecorder::recordProbeSummary(uint8_t channel,uint8_t pa,uint8_t rate,uint8_t size,uint16_t packets,uint16_t intervalMs,uint32_t durationMs){
    if(!recording)return;pending += "P,"+String(millis())+","+String(channel)+","+String(pa)+","+String(rate)+","+String(size)+","+String(packets)+","+String(intervalMs)+","+String(durationMs)+"\n";
}

bool SessionRecorder::exportCsv(Stream& output) {
    flushPending();
    if (!ready || !LittleFS.exists(SESSION_PATH)) return false;
    File file = LittleFS.open(SESSION_PATH, FILE_READ);
    if (!file) return false;
    while (file.available()) output.write(file.read());
    file.close();
    return true;
}

bool SessionRecorder::replayLatest(AppState& state) {
    flushPending();
    if (!ready || !LittleFS.exists(SESSION_PATH)) return false;
    File file = LittleFS.open(SESSION_PATH, FILE_READ);
    if (!file) return false;
    String lastData;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (line.startsWith("S,")) lastData = line;
    }
    file.close();
    if (lastData.length() == 0) return false;

    int start = 0;
    int field = 0;
    int channel = 0;
    while (start <= static_cast<int>(lastData.length())) {
        int comma = lastData.indexOf(',', start);
        if (comma < 0) comma = lastData.length();
        const String value = lastData.substring(start, comma);
        if (field == 3) state.peakChannel = value.toInt();
        else if (field == 4) state.peakLevel = constrain(value.toInt(), 0, 100);
        else if (field == 5) state.analyzerConfidence = constrain(value.toInt(), 0, 100);
        else if (field >= 9 && channel < TOTAL_CHANNELS) {
            state.spectrumLevels[channel++] = constrain(value.toInt(), 0, 100);
        }
        field++;
        start = comma + 1;
        if (comma >= static_cast<int>(lastData.length())) break;
    }
    state.analyzerFrozen = true;
    state.cursorChannel = state.peakChannel;
    return channel == TOTAL_CHANNELS;
}

size_t SessionRecorder::fileSize() const {
    if (!ready || !LittleFS.exists(SESSION_PATH)) return 0;
    File file = LittleFS.open(SESSION_PATH, FILE_READ);
    if (!file) return 0;
    const size_t size = file.size();
    file.close();
    return size;
}
