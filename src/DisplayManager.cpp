#include "DisplayManager.h"
#include "ButtonManager.h"
#include "RadioManager.h"
#include "SerialCommander.h"

DisplayManager displayManager;

static const char* menuTitles[] = {
    "1. Jammer Frequency",
    "2. Spectrum Analyzer",
    "3. Channel Inspector",
    "4. Device Status",
    "5. Reboot System"
};
static const int NUM_MENU_ITEMS = 5;

DisplayManager::DisplayManager()
    : tft(TFT_CS, TFT_AO, TFT_SDA, TFT_SCK, TFT_RST) {}

void DisplayManager::init() {
    tft.initR(INITR_BLACKTAB);   // ST7735 128x160
    tft.setRotation(3);          // Landscape 160 x 128
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextSize(1);
    needRedraw = true;
}

void DisplayManager::requestRedraw() {
    needRedraw = true;
}

// =============================================================================
// SPLASH SCREEN (SHOWN BEFORE THE MAIN MENU)
// =============================================================================
void DisplayManager::showSplash() {
    tft.fillScreen(ST77XX_BLACK);

    // Decorative border frame
    tft.drawRect(4, 4, 152, 120, ST77XX_DARKGRAY);

    // Title "NRF24 SUITE" (textSize 2, centered)
    String titleText = "NRF24 SUITE";
    tft.setTextSize(2);
    int16_t titleWidth = titleText.length() * 12;  // 6px per char * 2 (textSize)
    int16_t titleX = (160 - titleWidth) / 2;
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.setCursor(titleX, 50);
    tft.print(titleText);

    // Subtitle "by Dx4Grey" (textSize 1, centered below the title)
    String subtitleText = "by Dx4Grey";
    tft.setTextSize(1);
    int16_t subtitleWidth = subtitleText.length() * 6;  // 6px per char * 1 (textSize)
    int16_t subtitleX = (160 - subtitleWidth) / 2;
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.setCursor(subtitleX, 72);
    tft.print(subtitleText);

    // Keep the splash visible for a moment before the menu appears
    delay(2500);
}

uint16_t DisplayManager::getSignalColor(uint8_t level) {
    if (level < 30) return ST77XX_GREEN;
    if (level < 65) return ST77XX_YELLOW;
    if (level < 85) return CUSTOM_ORANGE;
    return ST77XX_RED;
}

// =============================================================================
// RENDER MAIN MENU (COMPACT & FIT)
// =============================================================================
void DisplayManager::renderMainMenu() {
    tft.fillScreen(ST77XX_BLACK);
    
    // Header Banner
    tft.fillRect(0, 0, 160, 14, 0x10A2); // Dark cyan banner
    String headerText = "=== RF24 SUITE ===";
    tft.setTextSize(1);
    int16_t headerWidth = headerText.length() * 6;
    int16_t headerX = (160 - headerWidth) / 2;
    tft.setCursor(headerX, 3);
    tft.setTextColor(ST77XX_CYAN, 0x10A2);
    tft.println(headerText);

    // Menu List (Spacing 18px)
    for (int i = 0; i < NUM_MENU_ITEMS; i++) {
        int y = 20 + (i * 18);
        if (i == menuSelection) {
            tft.fillRect(6, y - 2, 148, 15, 0x2124); // Highlight box
            tft.setTextColor(ST77XX_YELLOW, 0x2124);
            tft.setCursor(10, y + 1);
            tft.print("> ");
        } else {
            tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
            tft.setCursor(10, y + 1);
            tft.print("  ");
        }
        tft.print(menuTitles[i]);
    }

    // Footer
    tft.setCursor(8, 110);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("UP/DN:Select | RT:Enter");
}

// =============================================================================
// RENDER JAMMER SCREEN (COMPACT & FIT)
// =============================================================================
void DisplayManager::renderJammerScreen() {
    tft.fillScreen(ST77XX_BLACK);

    // Header Banner
    tft.fillRect(0, 0, 160, 14, 0x6000); // Dark red banner
    String headerText = "Jammer Control";
    tft.setTextSize(1);
    int16_t headerWidth = headerText.length() * 6;
    int16_t headerX = (160 - headerWidth) / 2;
    tft.setCursor(headerX, 3);
    tft.setTextColor(ST77XX_WHITE, 0x6000);
    tft.println(headerText);

    // Target Selection Box (Height 36px)
    tft.drawRect(6, 18, 148, 36, ST77XX_CYAN);
    tft.setCursor(10, 21);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("Target: ");
    tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    tft.println(appState.getJammerTargetName());

    tft.setCursor(10, 33);
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.println(appState.getJammerFreqRangeStr());

    // Status Box (Active / Standby)
    int statusY = 58;
    if (appState.jamming) {
        tft.fillRect(6, statusY, 148, 34, ST77XX_RED);
        tft.drawRect(6, statusY, 148, 34, ST77XX_YELLOW);
        tft.setCursor(18, statusY + 5);
        tft.setTextColor(ST77XX_WHITE, ST77XX_RED);
        tft.print("[ JAMMING ACTIVE! ]");
        
        tft.setCursor(14, statusY + 19);
        tft.print("Ch: ");
        tft.print(appState.currentJamChannel);
        tft.print(" (");
        tft.print(2400 + appState.currentJamChannel);
        tft.print(" MHz)");
    } else {
        tft.fillRect(6, statusY, 148, 34, 0x2104);
        tft.drawRect(6, statusY, 148, 34, ST77XX_GRAY);
        tft.setCursor(44, statusY + 12);
        tft.setTextColor(ST77XX_GRAY, 0x2104);
        tft.print("[ STANDBY ]");
    }

    // Footer Controls
    tft.setCursor(4, 110);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("UP/DN:Tgt | RT:Jam | B:Back");
}

// =============================================================================
// RENDER RADIO SPECTRUM ANALYZER (LIVE RF GRAPH - COMPACT)
// =============================================================================
void DisplayManager::drawSpectrumGrid() {
    tft.fillScreen(ST77XX_BLACK);

    // Top Header Status
    tft.setCursor(2, 2);
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.print("BAND:");
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.print(appState.getAnalyzerBandName());

    tft.setCursor(78, 2);
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.print("PK:");
    tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    tft.print(appState.peakChannel);
    tft.print("(");
    tft.print(appState.peakLevel);
    tft.print("%)");

    // Y-axis Grid Level Lines (100%, 50%, Baseline 0%)
    tft.drawFastHLine(GRAPH_X_START, GRAPH_Y_TOP, GRAPH_WIDTH, ST77XX_DARKGRAY);
    tft.drawFastHLine(GRAPH_X_START, GRAPH_Y_TOP + (GRAPH_HEIGHT / 2), GRAPH_WIDTH, ST77XX_DARKGRAY);
    tft.drawFastHLine(GRAPH_X_START, GRAPH_Y_BASELINE, GRAPH_WIDTH, ST77XX_WHITE); // X baseline

    // Y-axis Labels (0, 50, 100)
    tft.setCursor(0, GRAPH_Y_TOP);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("100");
    tft.setCursor(2, GRAPH_Y_TOP + (GRAPH_HEIGHT / 2) - 3);
    tft.print("50");
    tft.setCursor(6, GRAPH_Y_BASELINE - 4);
    tft.print(" 0");

    // X-axis: Popular Wi-Fi Channel Markers (1, 6, 11, 14)
    const int wifiMarkers[4] = {12, 37, 62, 84};
    const char* wifiLabels[4] = {"1", "6", "11", "14"};

    for (int i = 0; i < 4; i++) {
        int markerX = GRAPH_X_START + wifiMarkers[i];
        tft.drawFastVLine(markerX, GRAPH_Y_BASELINE + 1, 2, ST77XX_CYAN);
        tft.setCursor(markerX - 2, GRAPH_Y_BASELINE + 4);
        tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
        tft.print(wifiLabels[i]);
    }

    tft.setCursor(114, GRAPH_Y_BASELINE + 4);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("2.4G");

    // Footer
    tft.setCursor(4, 110);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("UP:Band  RT:Rst  B:Menu");
}

void DisplayManager::drawSpectrumBars() {
    // Update Header Peak Information
    tft.fillRect(76, 0, 84, 12, ST77XX_BLACK);
    tft.setCursor(78, 2);
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.print("PK:");
    tft.setTextColor(getSignalColor(appState.peakLevel), ST77XX_BLACK);
    tft.print("Ch");
    tft.print(appState.peakChannel);
    tft.print(" ");
    tft.print(appState.peakLevel);
    tft.print("%");

    // Render spectrum bar for each channel
    for (int ch = 0; ch < TOTAL_CHANNELS; ch++) {
        int x = GRAPH_X_START + ch;
        uint8_t lvl = appState.spectrumLevels[ch];
        uint8_t peak = appState.peakLevels[ch];

        int barHeight = (lvl * GRAPH_HEIGHT) / 100;
        int peakHeight = (peak * GRAPH_HEIGHT) / 100;

        int barTop = GRAPH_Y_BASELINE - barHeight;
        int peakTop = GRAPH_Y_BASELINE - peakHeight;

        // 1. Clear empty area above the bar and peak
        if (peakTop > GRAPH_Y_TOP) {
            tft.drawFastVLine(x, GRAPH_Y_TOP, peakTop - GRAPH_Y_TOP, ST77XX_BLACK);
        }

        // 2. Draw Peak Hold Indicator
        if (peakHeight > 0 && peakTop >= GRAPH_Y_TOP) {
            tft.drawPixel(x, peakTop, ST77XX_CYAN);
        }

        // 3. Clear space between peak and active bar
        if (peakTop < barTop - 1) {
            tft.drawFastVLine(x, peakTop + 1, barTop - peakTop - 1, ST77XX_BLACK);
        }

        // 4. Draw Active Signal Bar
        if (barHeight > 0) {
            uint16_t color = getSignalColor(lvl);
            tft.drawFastVLine(x, barTop, barHeight, color);
        }
    }
}

void DisplayManager::renderSpectrumAnalyzer() {
    if (needRedraw) {
        drawSpectrumGrid();
        needRedraw = false;
    }
    drawSpectrumBars();
}

// =============================================================================
// RENDER CHANNEL INSPECTOR (COMPACT & FIT)
// =============================================================================
void DisplayManager::renderChannelInspector() {
    // ---------------------------------------------------------------
    // STATIC part: drawn only once (when entering the mode / changing channel)
    // ---------------------------------------------------------------
    if (needRedraw) {
        tft.fillScreen(ST77XX_BLACK);

        // Header Banner
        tft.fillRect(0, 0, 160, 14, 0x0810);
        tft.setCursor(24, 3);
        tft.setTextColor(ST77XX_CYAN, 0x0810);
        tft.setTextSize(1);
        String headerText = "CHANNEL INSPECTOR";
        int16_t headerWidth = headerText.length() * 6;
        int16_t headerX = (160 - headerWidth) / 2;
        tft.setCursor(headerX, 3);
        tft.println(headerText);

        int ch = appState.inspectedChannel;
        int freq = 2400 + ch;

        // Channel & Frequency
        tft.setCursor(8, 18);
        tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
        tft.print("RF Ch: ");
        tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
        tft.print(ch);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.print(" (");
        tft.print(freq);
        tft.print(" MHz)");

        // Protocol Information Mapping
        tft.setCursor(8, 30);
        tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
        tft.print("Protocol : ");
        tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
        if (ch >= 1 && ch <= 73) {
            tft.print("Wi-Fi Band");
        } else if (ch >= 74 && ch <= 80) {
            tft.print("BT High Band");
        } else {
            tft.print("RF Extended");
        }

        // Activity Gauge (Progress Bar - Width 144px)
        tft.drawRect(8, 44, 144, 16, ST77XX_WHITE);

        // Footer
        tft.setCursor(4, 110);
        tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
        tft.print("UP/DN:+1 | RT:+10 | B:Menu");

        needRedraw = false;
    }

    // ---------------------------------------------------------------
    // DYNAMIC part: per-frame update without a full-screen clear (anti-flicker)
    // ---------------------------------------------------------------
    // Clear the gauge area (incl. border), then redraw border + fill
    tft.fillRect(8, 44, 144, 16, ST77XX_BLACK);
    tft.drawRect(8, 44, 144, 16, ST77XX_WHITE);

    int barW = map(appState.inspectedLevel, 0, 100, 0, 140);
    if (barW > 0) {
        tft.fillRect(10, 46, barW, 12, getSignalColor(appState.inspectedLevel));
    }
    if (barW < 140) {
        tft.fillRect(10 + barW, 46, 140 - barW, 12, ST77XX_BLACK);
    }

    // Peak Marker pada Bar
    int peakX = 10 + map(appState.inspectedPeak, 0, 100, 0, 140);
    if (peakX >= 10 && peakX <= 150) {
        tft.drawFastVLine(peakX, 44, 16, ST77XX_CYAN);
    }

    // Activity Percentage Text (clear old row, then redraw)
    tft.fillRect(8, 64, 150, 8, ST77XX_BLACK);
    tft.setCursor(8, 64);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.print("Signal: ");
    tft.setTextColor(getSignalColor(appState.inspectedLevel), ST77XX_BLACK);
    tft.print(appState.inspectedLevel);
    tft.print("%  ");

    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("Peak: ");
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.print(appState.inspectedPeak);
    tft.print("%");

    // Carrier Status (clear old row, then redraw)
    tft.fillRect(8, 76, 150, 8, ST77XX_BLACK);
    tft.setCursor(8, 76);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("Status: ");
    if (appState.inspectedLevel > 20) {
        tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
        tft.print("[ RF DETECTED ]");
    } else {
        tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
        tft.print("[ SIGNAL CLEAR ]");
    }
}

// =============================================================================
// RENDER STATUS SCREEN (COMPACT & FIT)
// =============================================================================
void DisplayManager::renderStatusScreen() {
    tft.fillScreen(ST77XX_BLACK);

    // Header Banner
    tft.fillRect(0, 0, 160, 14, 0x2124);
    tft.setCursor(34, 3);
    tft.setTextColor(ST77XX_WHITE, 0x2124);
    tft.setTextSize(1);
    tft.println("DEVICE STATUS");

    tft.setCursor(8, 20);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("Radio nRF24: ");
    if (radioManager.isConnected()) {
        tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
        tft.println("CONNECTED (OK)");
    } else {
        tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
        tft.println("NOT FOUND (ERR)");
    }

    tft.setCursor(8, 34);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("Chipset    : ");
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.println("ESP32-S3 DualCore");

    tft.setCursor(8, 48);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("Clock / RAM: ");
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.println("240MHz / 320KB");

    tft.setCursor(8, 62);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("Watchdog   : ");
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    tft.println("3.0s Hardware");

    tft.setCursor(8, 76);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("Jammer Pwr : ");
    tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    tft.println("MAX (0 dBm)");

    tft.setCursor(14, 110);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("Press B to go back");
}

// =============================================================================
// RENDER REBOOT SCREEN (SYSTEM RESTART)
// =============================================================================
void DisplayManager::renderRebootScreen() {
    if (needRedraw) {
        tft.fillScreen(ST77XX_BLACK);

        // Header Banner
        tft.fillRect(0, 0, 160, 14, 0x7800); // Maroon (dark red)
        tft.setCursor(24, 3);
        tft.setTextColor(ST77XX_WHITE, 0x7800);
        tft.setTextSize(1);
        tft.println("SYSTEM REBOOT");

        // Main Message
        tft.setCursor(18, 44);
        tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
        tft.print("REBOOTING");

        tft.setCursor(38, 62);
        tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
        tft.print("PLEASE WAIT");

        tft.setCursor(8, 100);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.print("Restarting ESP32...");

        needRedraw = false;
    }

    // Running dot animation (REBOOTING. / REBOOTING.. / REBOOTING...)
    int dots = (millis() / 250) % 4;
    tft.fillRect(18, 44, 90, 8, ST77XX_BLACK);
    tft.setCursor(18, 44);
    tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    tft.print("REBOOTING");
    for (int d = 0; d < dots; d++) {
        tft.print(".");
    }
}

// =============================================================================
// UPDATE UI DISPATCHER
// =============================================================================
void DisplayManager::updateUI() {
    switch (appState.appMode) {
        case APP_MODE_MENU:
            if (needRedraw) {
                renderMainMenu();
                needRedraw = false;
            }
            break;
        case APP_MODE_JAMMER:
            if (needRedraw) {
                renderJammerScreen();
                needRedraw = false;
            }
            break;
        case APP_MODE_ANALYZER_SPECTRUM:
            renderSpectrumAnalyzer();
            break;
        case APP_MODE_ANALYZER_CHANNEL:
            renderChannelInspector();   // per-frame dynamic update (static part redrawn internally)
            break;
        case APP_MODE_STATUS:
            if (needRedraw) {
                renderStatusScreen();
                needRedraw = false;
            }
            break;
        case APP_MODE_REBOOT:
            renderRebootScreen();   // di-render tiap frame agar animasi titik hidup
            break;
    }
}

// =============================================================================
// INPUT NAVIGATION
// =============================================================================
void DisplayManager::processInput() {
    // -------------------------------------------------------------------------
    // CONDITION 1: MAIN MENU
    // -------------------------------------------------------------------------
    if (appState.appMode == APP_MODE_MENU) {
        if (buttonManager.isPressed(BTN_UP)) {
            menuSelection = (menuSelection - 1 + NUM_MENU_ITEMS) % NUM_MENU_ITEMS;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_DOWN)) {
            menuSelection = (menuSelection + 1) % NUM_MENU_ITEMS;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            if (menuSelection == 0) {
                appState.appMode = APP_MODE_JAMMER;
            } else if (menuSelection == 1) {
                radioManager.stopAll();
                appState.resetPeaks();
                appState.appMode = APP_MODE_ANALYZER_SPECTRUM;
            } else if (menuSelection == 2) {
                radioManager.stopAll();
                appState.inspectedPeak = 0;
                appState.appMode = APP_MODE_ANALYZER_CHANNEL;
            } else if (menuSelection == 3) {
                appState.appMode = APP_MODE_STATUS;
            } else if (menuSelection == 4) {
                // Reboot System: stop radio then enter reboot mode
                radioManager.stopAll();
                appState.appMode = APP_MODE_REBOOT;
            }
            needRedraw = true;
        }
    }
    // -------------------------------------------------------------------------
    // CONDITION 2: JAMMER MODE
    // -------------------------------------------------------------------------
    else if (appState.appMode == APP_MODE_JAMMER) {
        if (buttonManager.isPressed(BTN_UP)) {
            appState.cycleJammerTarget(-1);
            if (appState.jamming) {
                radioManager.startJammer(appState.jammerTarget);
            }
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_DOWN)) {
            appState.cycleJammerTarget(1);
            if (appState.jamming) {
                radioManager.startJammer(appState.jammerTarget);
            }
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            if (appState.jamming) {
                radioManager.stopJammer();
            } else {
                radioManager.startJammer(appState.jammerTarget);
            }
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            radioManager.stopJammer();
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    // -------------------------------------------------------------------------
    // CONDITION 3: SPECTRUM ANALYZER
    // -------------------------------------------------------------------------
    else if (appState.appMode == APP_MODE_ANALYZER_SPECTRUM) {
        if (buttonManager.isPressed(BTN_UP) || buttonManager.isPressed(BTN_DOWN)) {
            appState.cycleAnalyzerBand(1);
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            appState.resetPeaks();
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            radioManager.stopAll();
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    // -------------------------------------------------------------------------
    // CONDITION 4: CHANNEL INSPECTOR
    // -------------------------------------------------------------------------
    else if (appState.appMode == APP_MODE_ANALYZER_CHANNEL) {
        if (buttonManager.isPressed(BTN_UP)) {
            appState.inspectedChannel = constrain(appState.inspectedChannel + 1, MIN_CHANNEL, MAX_CHANNEL);
            appState.inspectedPeak = 0;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_DOWN)) {
            appState.inspectedChannel = constrain(appState.inspectedChannel - 1, MIN_CHANNEL, MAX_CHANNEL);
            appState.inspectedPeak = 0;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            appState.inspectedChannel = (appState.inspectedChannel + 10) % (MAX_CHANNEL + 1);
            appState.inspectedPeak = 0;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            radioManager.stopAll();
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    // -------------------------------------------------------------------------
    // CONDITION 5: STATUS
    // -------------------------------------------------------------------------
    else if (appState.appMode == APP_MODE_STATUS) {
        if (buttonManager.isPressed(BTN_B) || buttonManager.isPressed(BTN_RIGHT)) {
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
}
