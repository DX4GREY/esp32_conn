/*
 * =============================================================================
 * ESP32-S3 RF24 SUITE: DUAL-CORE JAMMER & SPECTRUM ANALYZER
 * =============================================================================
 * Organized Modules:
 *  - Config.h             : Hardware pinout, timing, frequency presets, & constants
 *  - AppState.h/.cpp      : Global state, 6 jammer target presets, & analyzer data
 *  - ButtonManager.h/.cpp : 50ms debouncing & navigation button edge detection
 *  - Watchdog.h/.cpp      : ESP32-S3 hardware watchdog timer (3s auto-recovery)
 *  - RadioManager.h/.cpp  : Dual-Core FreeRTOS Task (Core 0 RF Jamming) & Spectrum Scanning
 *  - DisplayManager.h/.cpp: Visual menu, real-time spectrum graph, & channel inspector
 *  - SerialCommander.h/.cpp: Interactive CLI monitor & ASCII graph visualization
 * =============================================================================
 */

#include <Arduino.h>
#include "Config.h"
#include "AppState.h"
#include "Watchdog.h"
#include "ButtonManager.h"
#include "RadioManager.h"
#include "DisplayManager.h"
#include "SerialCommander.h"

// Callback to keep buttons & UI responsive during scanning
void yieldToUI() {
    displayManager.processInput();
    watchdog.feed();
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    // 1. Initialize Serial CLI (115200 Baud)
    serialCommander.init(115200);

    // 2. Initialize Navigation Buttons (Pull-Up)
    buttonManager.init();

    // 3. Initialize TFT ST7735 1.8" Display (160x128 Compact)
    displayManager.init();

    // 4. Initialize nRF24L01+ Radio
    if (!radioManager.init()) {
        while (1) {
            delay(1000);
        }
    }

    // 5. Initialize Hardware Watchdog (3.0s Timeout)
    watchdog.init(WATCHDOG_TIMEOUT_US);
}

// =============================================================================
// MAIN LOOP (CORE 1: UI, SERIAL, & SPECTRUM DISPATCHER)
// =============================================================================
void loop() {
    // 1. Reset Watchdog Timer (Heartbeat)
    watchdog.feed();

    // 2. Process Serial Monitor commands if any
    serialCommander.process();

    // 3. Process Physical Button Input
    displayManager.processInput();

    // 4. Execute Based on Active Mode
    if (appState.appMode == APP_MODE_ANALYZER_SPECTRUM) {
        // Radio Analyzer Mode: Scan 126 channels and update spectrum levels
        radioManager.scanSpectrum(yieldToUI);
    } else if (appState.appMode == APP_MODE_ANALYZER_CHANNEL) {
        // Channel Inspector Mode: Deep RF monitoring on a single channel
        // (no per-loop requestRedraw => only dynamic areas are updated,
        //  eliminating flicker from repeated fillScreen)
        radioManager.inspectChannel(appState.inspectedChannel);
        delay(25);
    } else if (appState.appMode == APP_MODE_REBOOT) {
        // Reboot Mode: screen is rendered in updateUI, restart is briefly delayed
        delay(10);
    } else {
        // Jammer Mode runs on Core 0 background task, Core 1 idle
        delay(10);
    }

    // 5. Render TFT screen if there are updates
    displayManager.updateUI();

    // 5b. Reboot System: show message then restart ESP32
    if (appState.appMode == APP_MODE_REBOOT) {
        delay(1200); // give the reboot message time to be visible on screen
        Serial.println("REBOOTING SYSTEM...");
        ESP.restart();
    }

    // 6. Auto-recovery on Watchdog Timeout
    if (watchdog.isTriggered()) {
        Serial.println("WATCHDOG TRIGGERED! Restarting ESP32...");
        ESP.restart();
    }
}