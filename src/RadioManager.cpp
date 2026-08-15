#include "RadioManager.h"

RadioManager radioManager;

RadioManager::RadioManager() : radio(CE_PIN, CSN_PIN) {}

bool RadioManager::init() {
    // Inisialisasi SPI kustom
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);

    // Inisialisasi nRF24L01+
    if (!radio.begin()) {
        Serial.println("❌ FATAL: nRF24L01+ tidak terdeteksi!");
        Serial.println("   Periksa kabel dan pinout SPI. Sistem HANG.");
        return false;
    }

    // Default ke TX configuration
    applyTxConfig();
    generateRandomPayload();

    Serial.println("✅ Radio nRF24L01+ Siap!");
    Serial.println("   Gunakan Menu Layar atau ketik 'help' di Serial.\n");
    return true;
}

void RadioManager::applyTxConfig() {
    radio.stopListening();
    radio.setPALevel(appState.powerLevel);
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
    radio.stopConstCarrier();
    radio.setAutoAck(false);
    radio.setPALevel(RF24_PA_MAX); // Sensitivitas maksimal untuk scan
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
    applyTxConfig();
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

void RadioManager::stepJammer(YieldCallback yieldCb) {
    if (!appState.jamming) return;

    // Pastikan berada di mode TX
    if (rxModeActive) {
        enterTxMode();
    }

    // 1. Pemilihan Kanal Berdasarkan Target
    int ch;
    if (appState.jammerTarget == JAM_TARGET_BLE_ADV) {
        ch = BLE_ADV_CHANNELS[random(0, 3)];
    } else {
        ch = random(appState.jammerMinCh, appState.jammerMaxCh + 1);
    }
    appState.currentJamChannel = ch;
    radio.setChannel(ch);

    // 2. Serangan Packet Storm (Inject dummy frames)
    radio.stopConstCarrier();
    for (int i = 0; i < 5; i++) {
        generateRandomPayload();
        radio.write(appState.randomPayload, PAYLOAD_SIZE);
        if (i % 2 == 0 && yieldCb) yieldCb();
    }

    // 3. Serangan Carrier Wave (Interferensi Sinyal Berkelanjutan)
    radio.startConstCarrier(appState.powerLevel, ch);
    delayMicroseconds(appState.hopDwellUs);
    radio.stopConstCarrier();
}

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
        delayMicroseconds(35); // Waktu stabilisasi PLL Synthesizer

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

        // Beri kesempatan tombol/UI merespons setiap 16 kanal
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
