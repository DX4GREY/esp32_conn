#include "services/SerialCommander.h"
#include "core/AppState.h"
#include "drivers/RadioManager.h"
#include "ui/DisplayManager.h"

SerialCommander serialCommander;

void SerialCommander::init(unsigned long baud) {
    Serial.begin(baud);
    delay(500);
    Serial.println("\n==============================================");
    Serial.println("   ESP32-S3 DUAL-CORE RF24 SUITE & ANALYZER   ");
    Serial.println("==============================================");
    Serial.println("Type 'help' for the list of serial commands.\n");
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
            Serial.println("Invalid target! Use: wifi, bt, ble, bledata, all, zigbee");
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
            Serial.println("Invalid channel (0 - 125)");
        }
    }
    else if (lowerCmd.startsWith("power") || lowerCmd.startsWith("pwr")) {
        String pwrArg = lowerCmd.startsWith("power") ? lowerCmd.substring(5) : lowerCmd.substring(3);
        pwrArg.trim();
        if (pwrArg.length() > 0) {
            if (appState.setPowerLevelByName(pwrArg)) {
                radioManager.updatePALevel(appState.powerLevel);
                Serial.printf("⚡ TX Power Level set to: %s (%s)\n",
                              appState.getPowerLevelName(), appState.getPowerLevelDbmStr());
            } else {
                Serial.println("Invalid power level! Options: min (-18dBm), low (-12dBm), high (-6dBm), max (0dBm)");
            }
        } else {
            Serial.printf("⚡ Current TX Power Level: %s (%s)\n",
                          appState.getPowerLevelName(), appState.getPowerLevelDbmStr());
        }
    }
    else if (lowerCmd.startsWith("dwell")) {
        String dwellArg = lowerCmd.length() > 5 ? lowerCmd.substring(5) : "";
        dwellArg.trim();
        if (dwellArg.length() > 0) {
            int us = dwellArg.toInt();
            if (appState.setDwellTime(us)) {
                Serial.printf("⏱️ Dwell Time set to: %d µs (%s)\n",
                              appState.dwellTimeUs, appState.getDwellTimeName());
            } else {
                Serial.println("Invalid dwell time! Presets: 50, 100, 200, 500, 1000 (Range: 10-10000 µs)");
            }
        } else {
            Serial.printf("⏱️ Current Dwell Time: %d µs (%s)\n",
                          appState.dwellTimeUs, appState.getDwellTimeName());
        }
    }
    else if (lowerCmd == "config" || lowerCmd == "settings") {
        printConfig();
    }
    else if (lowerCmd == "status") {
        printStatus();
    }
    else if (lowerCmd == "help") {
        printHelp();
    }
    else if (cmd.length() > 0) {
        Serial.println("Unknown command. Type 'help' for the list of commands.");
    }

    displayManager.requestRedraw();
}

void SerialCommander::printAsciiSpectrum() {
    Serial.println("\n📡 Scanning RF 2.4 GHz Spectrum (Channel 0 - 125)...");
    radioManager.scanSpectrum();

    Serial.println("\n=== RF 2.4 GHz SPECTRUM GRAPH ===");
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
    Serial.printf("\n⚡ Highest RF Peak: Channel %d (%d MHz) with Intensity %d%%\n\n",
                  appState.peakChannel, 2400 + appState.peakChannel, appState.peakLevel);
}

void SerialCommander::printConfig() {
    Serial.println("\n=== RF CONFIGURATION & PARAMETERS ===");
    Serial.println("TX Power Level : " + String(appState.getPowerLevelName()) + " [" + String(appState.getPowerLevelDbmStr()) + "]");
    Serial.println("Dwell Time     : " + String(appState.dwellTimeUs) + " µs (" + String(appState.getDwellTimeName()) + ")");
    Serial.println("Display Theme  : " + String(appState.getDisplayThemeName()));
    Serial.println("Active Target  : " + String(appState.getJammerTargetName()) + " (" + String(appState.getJammerFreqRangeStr()) + ")");
    Serial.println("Radio PA Mode  : LNA Gain MAX Enabled");
    Serial.println("Data Rate      : 2 Mbps (RF24_2MBPS)");
    Serial.println("Packet Size    : " + String(FAST_PAYLOAD_SIZE) + " Byte Payload (FAST_JAM_PAYLOAD)");
    Serial.println("Address Width  : " + String(FAST_ADDRESS_WIDTH) + " Bytes");
    Serial.println("CRC Status     : Disabled (Maximum Raw Aggression)");
    Serial.println("======================================\n");
}

void SerialCommander::printStatus() {
    Serial.println("\n=== DEVICE & RADIO STATUS ===");
    Serial.println("nRF24L01+ : " + String(radioManager.isConnected() ? "✅ CONNECTED" : "❌ NOT DETECTED"));
    Serial.println("Jam Mode  : " + String(appState.jamming ? "🔥 ACTIVE (Core 0 Task)" : "🛑 STOPPED"));
    Serial.println("Target    : " + String(appState.getJammerTargetName()));
    Serial.println("Range     : " + String(appState.getJammerFreqRangeStr()));
    Serial.println("TX Power  : " + String(appState.getPowerLevelName()) + " (" + String(appState.getPowerLevelDbmStr()) + ")");
    Serial.println("Dwell Time: " + String(appState.dwellTimeUs) + " µs (" + String(appState.getDwellTimeName()) + ")");
    Serial.println("Peak RF   : Channel " + String(appState.peakChannel) + " (" + String(appState.peakLevel) + "%)");
    Serial.println("=== END STATUS ===\n");
}

void SerialCommander::printHelp() {
    Serial.println("\n=== SERIAL COMMAND LIST ===");
    Serial.println("jam <target> - Targets: wifi, bt, ble, bledata, all, zigbee");
    Serial.println("power <lvl>  - Set TX Power: min, low, high, max (e.g. 'power max')");
    Serial.println("dwell <us>   - Set Dwell Time in µs: 50, 100, 200, 500, 1000");
    Serial.println("config       - Show current RF configuration & parameters");
    Serial.println("start        - Start jammer on active target");
    Serial.println("stop         - Stop jammer transmission");
    Serial.println("scan         - Run Spectrum Analyzer and print RF graph");
    Serial.println("inspect <ch> - Analyze RF activity on a specific channel");
    Serial.println("status       - Show system status and radio module");
    Serial.println("help         - Show this help");
    Serial.println("==============================\n");
}
