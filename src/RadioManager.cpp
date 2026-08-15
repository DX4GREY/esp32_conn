#include "RadioManager.h"

RadioManager radioManager;

RadioManager::RadioManager() : radio(CE_PIN, CSN_PIN) {}

bool RadioManager::init() {
    // Initialize custom SPI pins (16 MHz)
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);
    SPI.setFrequency(16000000);
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);

    // Initialize nRF24L01+
    if (!radio.begin(&SPI)) {
        Serial.println("❌ FATAL: nRF24L01+ not detected!");
        Serial.println("   Check wiring and SPI pinout. System HANG.");
        return false;
    }

    applyTxConfig();
    Serial.println("✅ nRF24L01+ Radio Ready!");
    Serial.println("   Use the display menu or type 'help' in Serial.\n");
    return true;
}

void RadioManager::applyTxConfig() {
    radio.stopListening();
    radio.setAutoAck(false);
    radio.setRetries(0, 0);
    radio.setPayloadSize(FAST_PAYLOAD_SIZE);
    radio.setAddressWidth(FAST_ADDRESS_WIDTH);
    radio.setPALevel(RF24_PA_MAX, true); // true = LNA Gain Enabled!
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.disableCRC();
    radio.disableAckPayload();
    radio.disableDynamicPayloads();
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
    radio.setPALevel(RF24_PA_MAX, true);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.startListening();
    rxModeActive = true;
}

void RadioManager::startJammer(JammerTarget target) {
    if (appState.jamming && jammerTaskHandle != NULL) {
        // If the target changes while jamming, switch directly
        appState.setJammerTarget(target);
        return;
    }

    appState.setJammerTarget(target);
    stopJam = false;

    enterTxMode();
    radio.powerUp();
    applyTxConfig();

    appState.jamming = true;

    // Launch the ultra-fast aggressive transmission task on Core 0
    xTaskCreatePinnedToCore(
        RadioManager::jammerTaskCode,
        "RFJammer",
        4096,
        this,
        3,
        &jammerTaskHandle,
        0
    );

    Serial.println("🔥 JAMMER ACTIVE (Core 0 Background Task): " + String(appState.getJammerTargetName()));
    Serial.println("   Range: " + String(appState.getJammerFreqRangeStr()) + "\n");
}

void RadioManager::stopJammer() {
    if (!appState.jamming && jammerTaskHandle == NULL) return;

    stopJam = true;
    if (jammerTaskHandle != NULL) {
        unsigned long startWait = millis();
        while (eTaskGetState(jammerTaskHandle) != eDeleted && (millis() - startWait < 500)) {
            delay(2);
        }
        jammerTaskHandle = NULL;
    }

    radio.stopConstCarrier();
    radio.powerDown();
    appState.jamming = false;
    Serial.println("🛑 Jammer Stopped.");
}

void RadioManager::stopAll() {
    stopJammer();
    radio.stopListening();
    rxModeActive = false;
}

bool RadioManager::isConnected() {
    return radio.isChipConnected();
}

// =============================================================================
// FREERTOS TASK: CORE 0 ULTRA-FAST TRANSMISSION LOOP
// =============================================================================
void RadioManager::jammerTaskCode(void *param) {
    RadioManager *self = static_cast<RadioManager*>(param);
    vTaskPrioritySet(NULL, 3);

    while (!self->stopJam) {
        switch (appState.jammerTarget) {
            // -----------------------------------------------------------------
            // 1. TARGET WI-FI (14 Channels 22MHz Full Bandwidth Sweep)
            // -----------------------------------------------------------------
            case JAM_TARGET_WIFI:
                for (int channel = 0; channel < 14 && !self->stopJam; channel++) {
                    int startCh = (channel * 5) + 1;
                    int endCh = (channel * 5) + 23;
                    for (int ch = startCh; ch <= endCh && !self->stopJam; ch++) {
                        if (ch <= MAX_CHANNEL) {
                            self->radio.setChannel(ch);
                            self->radio.writeFast(FAST_JAM_PAYLOAD, FAST_PAYLOAD_SIZE);
                        }
                    }
                }
                break;

            // -----------------------------------------------------------------
            // 2. TARGET BLUETOOTH (0 - 79 MHz Full Hopping Sweep)
            // -----------------------------------------------------------------
            case JAM_TARGET_BT:
                for (int ch = 0; ch < 80 && !self->stopJam; ch++) {
                    self->radio.setChannel(ch);
                    self->radio.writeFast(FAST_JAM_PAYLOAD, FAST_PAYLOAD_SIZE);
                }
                break;

            // -----------------------------------------------------------------
            // 3. TARGET BLE ADVERTISING (Channels 2, 26, 80)
            // -----------------------------------------------------------------
            case JAM_TARGET_BLE_ADV:
                for (int ch = 0; ch < 3 && !self->stopJam; ch++) {
                    self->radio.setChannel(BLE_ADV_CHANNELS[ch]);
                    self->radio.writeFast(FAST_JAM_PAYLOAD, FAST_PAYLOAD_SIZE);
                }
                break;

            // -----------------------------------------------------------------
            // 4. TARGET BLE DATA (Even Channels 2 - 80)
            // -----------------------------------------------------------------
            case JAM_TARGET_BLE_DATA:
                for (int ch = 2; ch <= 80 && !self->stopJam; ch += 2) {
                    self->radio.setChannel(ch);
                    self->radio.writeFast(FAST_JAM_PAYLOAD, FAST_PAYLOAD_SIZE);
                }
                break;

            // -----------------------------------------------------------------
            // 5. TARGET ALL BAND / DRONE (Channels 0 - 125)
            // -----------------------------------------------------------------
            case JAM_TARGET_ALL:
                for (int ch = 0; ch < 125 && !self->stopJam; ch++) {
                    self->radio.setChannel(ch);
                    self->radio.writeFast(FAST_JAM_PAYLOAD, FAST_PAYLOAD_SIZE);
                }
                break;

            // -----------------------------------------------------------------
            // 6. TARGET ZIGBEE (Channels 11 - 26)
            // -----------------------------------------------------------------
            case JAM_TARGET_ZIGBEE:
                for (int channel = 11; channel < 27 && !self->stopJam; channel++) {
                    int startCh = 4 + 5 * (channel - 11);
                    int endCh = (5 + 5 * (channel - 11)) + 2;
                    for (int ch = startCh; ch <= endCh && !self->stopJam; ch++) {
                        if (ch <= MAX_CHANNEL) {
                            self->radio.setChannel(ch);
                            self->radio.writeFast(FAST_JAM_PAYLOAD, FAST_PAYLOAD_SIZE);
                        }
                    }
                }
                break;
        }
        vTaskDelay(1); // Give a 1-tick delay so Core 0 IDLE0 can reset the Task Watchdog
    }
    vTaskDelete(NULL);
}

// =============================================================================
// SPECTRUM ANALYZER & CHANNEL INSPECTOR
// =============================================================================
void RadioManager::scanSpectrum(void (*yieldCb)()) {
    if (!rxModeActive) {
        enterRxMode();
    }

    int minCh, maxCh;
    appState.getAnalyzerChannelRange(minCh, maxCh);

    int highestCh = minCh;
    uint8_t highestLvl = 0;

    for (int ch = minCh; ch <= maxCh; ch++) {
        radio.setChannel(ch);
        delayMicroseconds(35); // PLL Synthesizer stabilization time

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