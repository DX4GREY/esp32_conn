#include "SerialCommander.h"
#include "AppState.h"
#include "RadioManager.h"
#include "DisplayManager.h"

SerialCommander serialCommander;

void SerialCommander::init(unsigned long baud) {
    Serial.begin(baud);
    delay(500);
    Serial.println("\n==============================================");
    Serial.println("   ESP32-S3 RF24 JAMMER & SPECTRUM ANALYZER   ");
    Serial.println("==============================================");
    Serial.println("Ketik 'help' untuk daftar perintah serial.\n");
}

void SerialCommander::process() {
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        executeCommand(cmd);
    }
}

void SerialCommander::executeCommand(String cmd) {
    cmd.trim();
    String lowerCmd = cmd;
    lowerCmd.toLowerCase();

    if (lowerCmd == "start") {
        radioManager.startJammer(appState.jammerTarget);
    }
    else if (lowerCmd == "stop") {
        radioManager.stopAll();
    }
    else if (lowerCmd.startsWith("jam ")) {
        String targetStr = lowerCmd.substring(4);
        if (appState.setJammerTargetByName(targetStr)) {
            radioManager.startJammer(appState.jammerTarget);
        } else {
            Serial.println("Target tidak valid! Gunakan: jam wifi / jam bt / jam ble / jam all");
        }
    }
    else if (lowerCmd == "scan" || lowerCmd == "spectrum") {
        radioManager.stopJammer();
        printAsciiSpectrum();
    }
    else if (lowerCmd.startsWith("inspect ")) {
        int ch = lowerCmd.substring(8).toInt();
        if (ch >= MIN_CHANNEL && ch <= MAX_CHANNEL) {
            radioManager.stopJammer();
            appState.inspectedChannel = ch;
            uint8_t lvl = radioManager.inspectChannel(ch);
            Serial.printf("🔍 Channel %d (%d MHz): Activity = %d%% [%s]\n",
                          ch, 2400 + ch, lvl, lvl > 20 ? "RF DETECTED" : "CLEAR");
        } else {
            Serial.println("Kanal tidak valid (0 - 125)");
        }
    }
    else if (lowerCmd == "status") {
        printStatus();
    }
    else if (lowerCmd == "help" || lowerCmd == "bantuan") {
        printHelp();
    }
    else if (cmd.length() > 0) {
        Serial.println("Perintah tidak dikenal. Ketik 'help' untuk daftar perintah.");
    }

    displayManager.requestRedraw();
}

void SerialCommander::printAsciiSpectrum() {
    Serial.println("\n📡 Memindai Spektrum RF 2.4 GHz (Kanal 0 - 125)...");
    radioManager.scanSpectrum();

    Serial.println("\n=== GRAFIK SPEKTRUM RF 2.4 GHz ===");
    for (int row = 10; row >= 1; row--) {
        int threshold = row * 10;
        Serial.printf("%3d%% | ", threshold);
        for (int ch = 0; ch < TOTAL_CHANNELS; ch += 2) {
            uint8_t lvl = appState.spectrumLevels[ch];
            uint8_t pk = appState.peakLevels[ch];
            if (lvl >= threshold) {
                Serial.print("█");
            } else if (pk >= threshold) {
                Serial.print("▪");
            } else {
                Serial.print(" ");
            }
        }
        Serial.println();
    }
    Serial.println("     +---------------------------------------------------------------");
    Serial.println(" Ch: | 0   12(W1)   37(W6)   62(W11)   84(W14)   100       125");
    Serial.println(" MHz:| 2400  2412     2437     2462      2484      2500      2525");
    Serial.printf("\n⚡ Peak RF Tertinggi: Kanal %d (%d MHz) dengan Intensitas %d%%\n\n",
                  appState.peakChannel, 2400 + appState.peakChannel, appState.peakLevel);
}

void SerialCommander::printStatus() {
    Serial.println("\n=== STATUS PERANGKAT & RADIO ===");
    Serial.println("nRF24L01+ : " + String(radioManager.isConnected() ? "✅ TERHUBUNG" : "❌ TIDAK TERDETEKSI"));
    Serial.println("Mode Jam  : " + String(appState.jamming ? "🔥 AKTIF" : "🛑 BERHENTI"));
    Serial.println("Target    : " + String(appState.getJammerTargetName()));
    Serial.println("Rentang   : " + String(appState.getJammerFreqRangeStr()));
    Serial.println("Peak RF   : Kanal " + String(appState.peakChannel) + " (" + String(appState.peakLevel) + "%)");
    Serial.println("=== END STATUS ===\n");
}

void SerialCommander::printHelp() {
    Serial.println("\n=== DAFTAR PERINTAH SERIAL ===");
    Serial.println("jam wifi     - Jamming rentang frekuensi Wi-Fi (Ch 1 - 73)");
    Serial.println("jam bt       - Jamming rentang frekuensi Bluetooth (Ch 2 - 80)");
    Serial.println("jam ble      - Jamming 3 kanal utama BLE Advertising (Ch 2/26/80)");
    Serial.println("jam all      - Jamming seluruh pita 2.4 GHz (Ch 0 - 125)");
    Serial.println("stop         - Menghentikan jamming dan mengembalikan ke mode idle");
    Serial.println("scan         - Menjalankan Spectrum Analyzer dan mencetak grafik RF");
    Serial.println("inspect <ch> - Menganalisis aktivitas sinyal pada 1 kanal spesifik");
    Serial.println("status       - Informasi status sistem dan modul radio");
    Serial.println("help         - Menampilkan bantuan ini");
    Serial.println("==============================\n");
}
