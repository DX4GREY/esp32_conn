#include "RadioManager.h"

RadioManager radioManager;

// =============================================================================
// FILE-LOCAL: FAST TX CONFIG for a single nRF24L01+ (packet-storm mode)
// =============================================================================
static void applyFastTxConfig(RF24 &r) {
    r.stopListening();
    r.setAutoAck(false);
    r.setRetries(0, 0);
    r.setPayloadSize(FAST_PAYLOAD_SIZE);
    r.setAddressWidth(FAST_ADDRESS_WIDTH);
    r.setPALevel(RF24_PA_MAX, true); // true = LNA Gain Enabled!
    r.setDataRate(RF24_2MBPS);
    r.setCRCLength(RF24_CRC_DISABLED);
    r.disableCRC();
    r.disableAckPayload();
    r.disableDynamicPayloads();
}
// =============================================================================
// FILE-LOCAL SWEEP HELPERS (used by the Core 0 jammer task)
//
// The two nRF24L01+ share the same SPI bus, so their SPI transactions
// serialize with each other, but their RF front-ends radiate in PARALLEL.
// All helpers keep both radios transmitting simultaneously and honour the
// volatile `stop` flag at every step so stopJammer() reacts in microseconds.
// =============================================================================
namespace {

inline void pushPacket(RF24 &r) {
    r.writeFast(FAST_JAM_PAYLOAD, FAST_PAYLOAD_SIZE);
}

// Split-sweep: radio A takes channel indices [0, 2, 4..], radio B takes
// [1, 3, 5..] -> both radios radiate on two DIFFERENT frequencies at once,
// doubling the swept band per full cycle at the same per-radio packet rate.
void sweepSplit(RF24 &rA, RF24 &rB, const uint8_t *channels, int count,
                const volatile bool &stop) {
    for (int i = 0; i < count && !stop; i++) {
        rA.setChannel(channels[i]);
        pushPacket(rA);
        i++;
        if (i < count && !stop) {
            rB.setChannel(channels[i]);
            pushPacket(rB);
        }
    }
}

// Paired-sweep: two channellists swept in lockstep (e.g. BT even/odd), both
// radios transmit at the same time on different frequencies.
void sweepPaired(RF24 &rA, const uint8_t *listA, RF24 &rB, const uint8_t *listB,
                 int count, const volatile bool &stop) {
    for (int i = 0; i < count && !stop; i++) {
        rA.setChannel(listA[i]);
        pushPacket(rA);
        if (i < count && !stop) {
            rB.setChannel(listB[i]);
            pushPacket(rB);
        }
    }
}

// Dual-burst: BOTH radios hit the same channel (used for BLE advertising,
// where packet density on the critical advertising channels matters more
// than widening the sweep).
void sweepDuo(RF24 &rA, RF24 &rB, const uint8_t *channels, int count,
              const volatile bool &stop) {
    for (int i = 0; i < count && !stop; i++) {
        rA.setChannel(channels[i]);
        pushPacket(rA);
        if (i < count && !stop) {
            rB.setChannel(channels[i]);
            pushPacket(rB);
        }
    }
}

// ZigBee uses a 9-RF-channel wide sub-band per ZigBee channel. Alternate the
// RF offsets inside each band across the two radios so they jam in parallel.
void sweepZigbee(RF24 &rA, RF24 &rB, const volatile bool &stop) {
    for (int channel = 11; channel < 27 && !stop; channel++) {
        int startCh = 4 + 5 * (channel - 11);
        int endCh = (5 + 5 * (channel - 11)) + 2;

        int ch = startCh;
        while (ch <= endCh && !stop) {
            if (ch > MAX_CHANNEL) break;

            rA.setChannel(ch);
            pushPacket(rA);
            ch++;

            if (ch <= endCh && ch <= MAX_CHANNEL && !stop) {
                rB.setChannel(ch);
                pushPacket(rB);
                ch++;
            }
        }
    }
}

} // namespace

RadioManager::RadioManager() : radio(CE_PIN, CSN_PIN), radio2(CE_PIN_2, CSN_PIN_2) {}

bool RadioManager::init() {
    // Initialize the custom SPI bus (shared by both radios, 16 MHz)
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);
    SPI.setFrequency(16000000);
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);

    // Initialize both nRF24L01+ modules (different CSN/CE, same SPI bus)
    if (!radio.begin(&SPI) || !radio2.begin(&SPI)) {
        Serial.println("FATAL: nRF24L01+ not detected!");
        Serial.println("   Check wiring and SPI pinout. System HANG.");
        return false;
    }

    applyTxConfig();
    Serial.println("nRF24L01+ Dual Radio Ready!");
    Serial.println("   Use the display menu or type 'help' in Serial.\n");
    return true;
}

void RadioManager::applyTxConfig() {
    applyFastTxConfig(radio);
    applyFastTxConfig(radio2);
    rxModeActive = false;
}

// Enter transmission mode: force a clean TX state on both radios, power them
// up and, crucial for a streaming jammer, drive CE HIGH so the TX FIFO keeps
// draining on-air. WITHOUT CE HIGH, writeFast() would just stuff the 3-slot
// FIFO and then block forever waiting for a free slot.
void RadioManager::enterTxMode() {
    applyTxConfig();      // stopListening + all TX patches on both radios
    radio.powerUp();
    radio2.powerUp();
    radio.ce(HIGH);
    radio2.ce(HIGH);
}

void RadioManager::enterRxMode() {
    // Fresh RX state on both radios (no unresolved carrier/fifo leftovers)
    radio.stopConstCarrier();
    radio.flush_tx();
    radio.flush_rx();
    radio.setAutoAck(false);
    radio.setPALevel(RF24_PA_MAX, true);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.startListening();

    radio2.stopConstCarrier();
    radio2.flush_tx();
    radio2.flush_rx();
    radio2.setAutoAck(false);
    radio2.setPALevel(RF24_PA_MAX, true);
    radio2.setDataRate(RF24_2MBPS);
    radio2.setCRCLength(RF24_CRC_DISABLED);
    radio2.startListening();

    rxModeActive = true;
}

void RadioManager::startJammer(JammerTarget target) {
    // If the task is already sweeping, just hot-swap the target
    // (the running loop re-reads appState.jammerTarget each round).
    if (appState.jamming && jammerTaskHandle != NULL) {
        appState.setJammerTarget(target);
        return;
    }

    appState.setJammerTarget(target);
    stopJam = false;

    // Full deterministic TX-handler state, including CE HIGH
    enterTxMode();

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

    Serial.println("JAMMER ACTIVE (Core 0 Background Task): " + String(appState.getJammerTargetName()));
    Serial.println("   Range: " + String(appState.getJammerFreqRangeStr()) + "\n");
}

void RadioManager::stopJammer() {
    if (!appState.jamming && jammerTaskHandle == NULL) return;

    stopJam = true;

    // Keep CE high while the task unwinds so the TX FIFO keeps draining and
    // writeFast() can never block forever. The loop checks stopJam between
    // every payload, so the task exits in microseconds.
    if (jammerTaskHandle != NULL) {
        unsigned long startWait = millis();
        while (eTaskGetState(jammerTaskHandle) != eDeleted && (millis() - startWait < 500)) {
            delay(2);
        }
        jammerTaskHandle = NULL;
    }

    // Now stop both radios cleanly and drop any residual FIFO payloads
    radio.ce(LOW);
    radio.flush_tx();
    radio.powerDown();
    radio2.ce(LOW);
    radio2.flush_tx();
    radio2.powerDown();
    appState.jamming = false;
    Serial.println("Jammer Stopped.");
}

void RadioManager::stopAll() {
    stopJammer();
    radio.stopListening();
    radio2.stopListening();
    rxModeActive = false;
}

bool RadioManager::isConnected() {
    return radio.isChipConnected() && radio2.isChipConnected();
}

// =============================================================================
// FREERTOS TASK: CORE 0 ULTRA-FAST DUAL-RADIO TRANSMISSION LOOP
//
// EnterTxMode() already left CE HIGH, so writeFast() below fills the TX FIFO
// and the radio slips packets onto the air automatically while the FIFO
// drains - the loop is never blocked and both radios can stream in parallel.
// =============================================================================
void RadioManager::jammerTaskCode(void *param) {
    RadioManager *self = static_cast<RadioManager*>(param);
    vTaskPrioritySet(NULL, 3);

    const volatile bool &stop = self->stopJam;

    while (!stop) {
        switch (appState.jammerTarget) {
            // -----------------------------------------------------------------
            // 1. TARGET WI-FI (50 Programmed Channels via wifi_channels[])
            //    Both radios sweep DIFFERENT halves simultaneously.
            // -----------------------------------------------------------------
            case JAM_TARGET_WIFI:
                sweepSplit(self->radio, self->radio2, wifi_channels,
                           WIFI_CHANNELS_COUNT, stop);
                break;

            // -----------------------------------------------------------------
            // 2. TARGET BLUETOOTH (Full 1-80 hop)
            //    Radio1 sweeps even channels, radio2 odd channels IN LOCKSTEP,
            //    so both transceivers radiate at the same time.
            // -----------------------------------------------------------------
            case JAM_TARGET_BT:
                sweepPaired(self->radio, bluetooth_even_channels,
                            self->radio2, bluetooth_odd_channels,
                            BLUETOOTH_EVEN_CHANNELS_COUNT, stop);
                break;

            // -----------------------------------------------------------------
            // 3. TARGET BLE ADVERTISING (Ch 37/38/39 = RF 2/26/80 + hops)
            //    Both radios blast the SAME channels = 2x packet density on
            //    the critical advertising frequencies.
            // -----------------------------------------------------------------
            case JAM_TARGET_BLE_ADV:
                sweepDuo(self->radio, self->radio2, ble_channels,
                         BLE_CHANNELS_COUNT, stop);
                break;

            // -----------------------------------------------------------------
            // 4. TARGET BLE DATA (12 Channels from 3 Channel Groups)
            //    Split sweep across both radios for 2x coverage.
            // -----------------------------------------------------------------
            case JAM_TARGET_BLE_DATA:
                sweepSplit(self->radio, self->radio2, BLE_DATA_CHANNELS,
                           BLE_DATA_CHANNELS_COUNT, stop);
                break;

            // -----------------------------------------------------------------
            // 5. TARGET ALL BAND / DRONE (Channels 1 - 100 via full_channels[])
            //    Split across both radios: each sweep cycle covers 2x the band.
            // -----------------------------------------------------------------
            case JAM_TARGET_ALL:
                sweepSplit(self->radio, self->radio2, full_channels,
                           FULL_CHANNELS_COUNT, stop);
                break;

            // -----------------------------------------------------------------
            // 6. TARGET ZIGBEE (Channels 11 - 26)
            //    RF offsets inside every ZigBee band are alternated between
            //    the two radios for parallel band coverage.
            // -----------------------------------------------------------------
            case JAM_TARGET_ZIGBEE:
                sweepZigbee(self->radio, self->radio2, stop);
                break;
        }
        vTaskDelay(1); // Yield 1 tick so Core 0 IDLE0 can reset the Task Watchdog
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
        radio2.setChannel(ch);
        delayMicroseconds(35); // PLL Synthesizer stabilization time

        int hits = 0;
        for (int s = 0; s < SPECTRUM_SAMPLES_PER_CH; s++) {
            if (radio.testRPD() || radio.testCarrier() || radio2.testRPD() || radio2.testCarrier()) {
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
    radio2.setChannel(channel);
    delayMicroseconds(40);

    int hits = 0;
    for (int s = 0; s < INSPECT_SAMPLES; s++) {
        if (radio.testRPD() || radio.testCarrier() || radio2.testRPD() || radio2.testCarrier()) {
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