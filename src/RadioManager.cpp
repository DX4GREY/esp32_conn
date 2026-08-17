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
    r.setPALevel(appState.powerLevel, true); // true = LNA Gain Enabled!
    r.setDataRate(RF24_2MBPS);
    r.setCRCLength(RF24_CRC_DISABLED);
    r.disableCRC();
    r.disableAckPayload();
    r.disableDynamicPayloads();
}
// =============================================================================
// FILE-LOCAL MAX-AGGRESSION HELPERS (used by the Core 0 jammer task)
//
// Both nRF24L01+ run in REUSE_TX_PL mode: after a single payload is loaded the
// chip re-broadcasts it back-to-back while CE stays HIGH = 100% airtime, and
// hopping only costs a 2-byte SPI write (setChannel) + a cheap reUseTX()
// re-assertion. The SPI bus is barely touched while the radios blast away.
// =============================================================================
namespace {

// Load the payload once and switch the radio into continuous-reuse mode.
void armContinuousJam(RF24 &r) {
    r.setChannel(0);
    r.writeFast(FAST_JAM_PAYLOAD, FAST_PAYLOAD_SIZE); // primer payload on air
    r.reUseTX();                                      // endless re-transmission
}

// One aggressive hop: retune + re-assert reuse + dwell at 100% duty.
inline void hopAndReuse(RF24 &r, uint8_t ch) {
    r.setChannel(ch);
    r.reUseTX();
    delayMicroseconds(appState.dwellTimeUs);
}

// Split-sweep: radio A covers even indices, radio B odd -> two frequencies are
// pounded simultaneously.
void hopSplit(RF24 &rA, RF24 &rB, const uint8_t *channels, int count,
              const volatile bool &stop) {
    for (int i = 0; i < count && !stop; i++) {
        hopAndReuse(rA, channels[i]);
        i++;
        if (i < count && !stop) {
            hopAndReuse(rB, channels[i]);
        }
    }
}

// Paired-sweep: two channellists swept in lockstep (e.g. BT even/odd).
void hopPaired(RF24 &rA, const uint8_t *listA, RF24 &rB, const uint8_t *listB,
               int count, const volatile bool &stop) {
    for (int i = 0; i < count && !stop; i++) {
        hopAndReuse(rA, listA[i]);
        if (i < count && !stop) {
            hopAndReuse(rB, listB[i]);
        }
    }
}

// Dual-burst: BOTH radios land on the SAME channel consecutively, doubling the
// dwell / energy on critical channels (BLE advertising 37/38/39).
void hopDuo(RF24 &rA, RF24 &rB, const uint8_t *channels, int count,
            const volatile bool &stop) {
    for (int i = 0; i < count && !stop; i++) {
        hopAndReuse(rA, channels[i]);
        if (i < count && !stop) {
            hopAndReuse(rB, channels[i]);
        }
    }
}

// ZigBee: alternate RF offsets inside every ZigBee band across the two radios.
void hopZigbee(RF24 &rA, RF24 &rB, const volatile bool &stop) {
    for (int channel = 11; channel < 27 && !stop; channel++) {
        int startCh = 4 + 5 * (channel - 11);
        int endCh = (5 + 5 * (channel - 11)) + 2;

        int ch = startCh;
        while (ch <= endCh && !stop) {
            if (ch > MAX_CHANNEL) break;

            hopAndReuse(rA, ch);
            ch++;

            if (ch <= endCh && ch <= MAX_CHANNEL && !stop) {
                hopAndReuse(rB, ch);
                ch++;
            }
        }
    }
}

} // namespace

RadioManager::RadioManager() : radio(CE_PIN, CSN_PIN), radio2(CE_PIN_2, CSN_PIN_2) {}

bool RadioManager::init() {
    // Ensure both radios start de-asserted. If a module was left in TX (e.g.
    // after a brownout reset during MAX-AGGRESSION jamming), CE HIGH holds it
    // transmitting and can stall the next begin(). Pull CE LOW first.
    radio.ce(LOW);
    radio2.ce(LOW);

    // Initialize the custom SPI bus (shared by both radios, 16 MHz)
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);
    SPI.setFrequency(16000000);
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);

    // nRF24L01+ modules are famously unresponsive for a few ms right after
    // power-up or after the 3.3V rail sags (brownout reset under heavy RF load).
    // A single begin() attempt followed by a hard hang is fragile -- retry with a
    // settle delay and power-down cleanup between attempts instead.
    const int MAX_ATTEMPTS = 5;
    bool okA = false, okB = false;
    for (int attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt) {
        delay(50); // let the 3.3V rail fully rise / module settle

        // Initialize both nRF24L01+ modules (different CSN/CE, same SPI bus)
        okA = radio.begin(&SPI);
        okB = radio2.begin(&SPI);
        if (okA && okB) {
            applyTxConfig();
            Serial.println("nRF24L01+ Dual Radio Ready!");
            Serial.println("   Use the display menu or type 'help' in Serial.\n");
            return true;
        }

        Serial.printf("[init] attempt %d/%d: radio1=%s radio2=%s\n",
                      attempt, MAX_ATTEMPTS,
                      okA ? "OK" : "FAIL", okB ? "OK" : "FAIL");

        // Clean up before retrying (begin() may have left the module powered up).
        radio.powerDown();
        radio2.powerDown();
    }

    Serial.println("FATAL: nRF24L01+ not detected after 5 attempts!");
    Serial.println("   Check wiring, 3.3V supply and decoupling caps.\n");
    return false;
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

    // Max-aggression arming: both radios enter continuous REUSE_TX_PL mode and
    // stream at 100% airtime; the task loop only hops channels afterwards.
    armContinuousJam(radio);
    armContinuousJam(radio2);

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

void RadioManager::updatePALevel(rf24_pa_dbm_e pwr) {
    appState.powerLevel = pwr;
    radio.setPALevel(pwr, true);
    radio2.setPALevel(pwr, true);
}

bool RadioManager::isConnected() {
    return radio.isChipConnected() && radio2.isChipConnected();
}

// =============================================================================
// FREERTOS TASK: CORE 0 MAX-AGGRESSION DUAL-RADIO TRANSMISSION LOOP
//
// Both radios were armed in REUSE_TX_PL mode (see armContinuousJam) and CE is
// HIGH, so they continuously blast the last payload at 100% airtime. This loop
// only retunes channels - each channel is hammered for JAMMER_DWELL_US before
// hopping, and stopJam is re-checked at every hop boundary.
// =============================================================================
void RadioManager::jammerTaskCode(void *param) {
    RadioManager *self = static_cast<RadioManager*>(param);
    vTaskPrioritySet(NULL, 3);

    const volatile bool &stop = self->stopJam;

    while (!stop) {
        switch (appState.jammerTarget) {
            // -----------------------------------------------------------------
            // 1. TARGET WI-FI (50 Programmed Channels via wifi_channels[])
            //    Both radios hammer DIFFERENT halves simultaneously.
            // -----------------------------------------------------------------
            case JAM_TARGET_WIFI:
                hopSplit(self->radio, self->radio2, wifi_channels,
                         WIFI_CHANNELS_COUNT, stop);
                break;

            // -----------------------------------------------------------------
            // 2. TARGET BLUETOOTH (Full 1-80 hop)
            //    Radio1 pounds even channels, radio2 odd channels IN LOCKSTEP,
            //    so both transceivers radiate at the same time.
            // -----------------------------------------------------------------
            case JAM_TARGET_BT:
                hopPaired(self->radio, bluetooth_even_channels,
                          self->radio2, bluetooth_odd_channels,
                          BLUETOOTH_EVEN_CHANNELS_COUNT, stop);
                break;

            // -----------------------------------------------------------------
            // 3. TARGET BLE ADVERTISING (Ch 37/38/39 = RF 2/26/80 + hops)
            //    Both radios land on EVERY channel together = 2x dwell/energy
            //    on the critical advertising frequencies.
            // -----------------------------------------------------------------
            case JAM_TARGET_BLE_ADV:
                hopDuo(self->radio, self->radio2, ble_channels,
                       BLE_CHANNELS_COUNT, stop);
                break;

            // -----------------------------------------------------------------
            // 4. TARGET BLE DATA (12 Channels from 3 Channel Groups)
            //    Split hop across both radios for 2x coverage.
            // -----------------------------------------------------------------
            case JAM_TARGET_BLE_DATA:
                hopSplit(self->radio, self->radio2, BLE_DATA_CHANNELS,
                         BLE_DATA_CHANNELS_COUNT, stop);
                break;

            // -----------------------------------------------------------------
            // 5. TARGET ALL BAND / DRONE (Channels 1 - 100 via full_channels[])
            //    Split across both radios: each sweep cycle covers 2x the band.
            // -----------------------------------------------------------------
            case JAM_TARGET_ALL:
                hopSplit(self->radio, self->radio2, full_channels,
                         FULL_CHANNELS_COUNT, stop);
                break;

            // -----------------------------------------------------------------
            // 6. TARGET ZIGBEE (Channels 11 - 26)
            //    RF offsets inside every ZigBee band are alternated between
            //    the two radios for parallel band coverage.
            // -----------------------------------------------------------------
            case JAM_TARGET_ZIGBEE:
                hopZigbee(self->radio, self->radio2, stop);
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