#include "RadioManager.h"

RadioManager radioManager;

RadioManager::RadioManager() : radio(CE_PIN, CSN_PIN) {}

bool RadioManager::init() {
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);
    if (!radio.begin()) {
        Serial.println("❌ FATAL: nRF24L01+ tidak terdeteksi!");
        Serial.println("   Periksa kabel dan pinout SPI. Sistem HANG.");
        return false;
    }
    applyTxConfig();
    generateRandomPayload();
    Serial.println("✅ Radio nRF24L01+ Siap!");
    Serial.println("   Gunakan Menu Layar atau ketik 'help' di Serial.\n");
    return true;
}

void RadioManager::applyTxConfig() {
    radio.stopListening();
    radio.setPALevel((rf24_pa_dbm_e)appState.powerLevel);
    radio.setDataRate(appState.dataRate);
    radio.setAutoAck(false);
    radio.setRetries(0, 0);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.setPayloadSize(PAYLOAD_SIZE);
    radio.openWritingPipe(DEFAULT_PIPE_ADDRESS);
    rxModeActive = false;
}

void RadioManager::enterTxMode() {
    if (rxModeActive) {
        applyTxConfig();
    }
}

void RadioManager::enterRxMode() {
    radio.stopConstCarrier();          // Hentikan carrier jika aktif
    radio.setAutoAck(false);
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.startListening();
    rxModeActive = true;
}

void RadioManager::generateRandomPayload() {
    for (int i = 0; i < PAYLOAD_SIZE; i++) {
        appState.randomPayload[i] = random(0, 256);
    }
}

void RadioManager::startJammer(JammerTarget target) {
    appState.setJammerTarget(target);
    enterTxMode();
    radio.powerUp();
    applyTxConfig();   // Pastikan konfigurasi TX
    appState.jamming = true;
    Serial.println("🔥 JAMMER AKTIF: " + String(appState.getJammerTargetName()));
    Serial.println("   Frekuensi: " + String(appState.getJammerFreqRangeStr()));
    Serial.println("   Mode Serangan: Packet Storm + Carrier Dwell (Aggressive)\n");
}

void RadioManager::stopJammer() {
    if (!appState.jamming) return;
    radio.stopConstCarrier();
    appState.jamming = false;
    Serial.println("🛑 Jammer Dimatikan.");
}

void RadioManager::stopAll() {
    stopJammer();
    radio.stopConstCarrier();
    radio.stopListening();
    rxModeActive = false;
}

bool RadioManager::isConnected() {
    return radio.isChipConnected();
}

// ===================== THE ONLY CORRECT stepJammer() =====================
void RadioManager::stepJammer(YieldCallback yieldCb) {
    if (!appState.jamming) return;

    // Pastikan TX mode
    if (rxModeActive) {
        enterTxMode();
    }

    // 1. Pilih kanal
    uint8_t ch;
    if (appState.jammerTarget == JAM_TARGET_BLE_ADV) {
        ch = (uint8_t)BLE_ADV_CHANNELS[random(0, 3)];
    } else {
        ch = (uint8_t)random(appState.jammerMinCh, appState.jammerMaxCh + 1);
    }
    appState.currentJamChannel = ch;
    radio.setChannel(ch);
    radio.setPALevel((rf24_pa_dbm_e)appState.powerLevel);  // Pastikan PA level sesuai

    // 2. Packet Storm – dengan flush untuk mencegah TX FIFO macet
    radio.stopConstCarrier();  // Matikan carrier sebelum packet burst
    for (int i = 0; i < 5; i++) {
        generateRandomPayload();
        bool success = radio.write(appState.randomPayload, PAYLOAD_SIZE);
        if (!success) {
            radio.flush_tx();
        }
        if (i % 2 == 0 && yieldCb) yieldCb();
    }

    // 3. Carrier Wave – PARAMETER YANG BENAR: (channel, power)
    //    Channel = ch, Power = appState.powerLevel
    // startConstCarrier expects (power, channel) in this RF24 implementation
    radio.startConstCarrier((rf24_pa_dbm_e)appState.powerLevel, ch);

    // 4. Dwell time dengan safe delay (handle overflow > 16383 us)
    uint32_t dwell = appState.hopDwellUs;
    if (dwell > 16383UL) {
        delay(dwell / 1000);
        delayMicroseconds(dwell % 1000);
    } else {
        delayMicroseconds(dwell);
    }
    radio.stopConstCarrier();
}
// ========================================================================

void RadioManager::scanSpectrum(YieldCallback yieldCb) {
    if (!rxModeActive) {
        enterRxMode();
    }

    int minCh, maxCh;
    appState.getAnalyzerChannelRange(minCh, maxCh);

    int highestCh = minCh;
    uint8_t highestLvl = 0;

    for (int ch = minCh; ch <= maxCh; ch++) {
        radio.setChannel(ch);
        delayMicroseconds(35);

        int hits = 0;
        for (int s = 0; s < SPECTRUM_SAMPLES_PER_CH; s++) {
            if (radio.testRPD() || radio.testCarrier()) {
                hits++;
            }
            delayMicroseconds(8);
        }

        uint8_t lvl = (hits * 100) / SPECTRUM_SAMPLES_PER_CH;
        appState.spectrumLevels[ch] = lvl;

        if (lvl > appState.peakLevels[ch]) {
            appState.peakLevels[ch] = lvl;
        }

        if (lvl > highestLvl) {
            highestLvl = lvl;
            highestCh = ch;
        }

        if ((ch % 16 == 0) && yieldCb) {
            yieldCb();
        }
    }

    appState.peakChannel = highestCh;
    appState.peakLevel = highestLvl;
    appState.decayPeaks();
}

uint8_t RadioManager::inspectChannel(int channel) {
    if (!rxModeActive) {
        enterRxMode();
    }

    channel = constrain(channel, MIN_CHANNEL, MAX_CHANNEL);
    radio.setChannel(channel);
    delayMicroseconds(40);

    int hits = 0;
    for (int s = 0; s < INSPECT_SAMPLES; s++) {
        if (radio.testRPD() || radio.testCarrier()) {
            hits++;
        }
        delayMicroseconds(12);
    }

    uint8_t lvl = (hits * 100) / INSPECT_SAMPLES;
    appState.inspectedLevel = lvl;

    if (lvl > appState.inspectedPeak) {
        appState.inspectedPeak = lvl;
    }

    return lvl;
}