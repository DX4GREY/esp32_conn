#include "DisplayManager.h"
#include "ButtonManager.h"
#include "RadioManager.h"
#include "SerialCommander.h"

DisplayManager displayManager;

static const char* menuTitles[] = {
    "1. Jammer Control",
    "2. Spectrum Analyzer",
    "3. Channel Inspector",
    "4. RF Power & Dwell",
    "5. Device Status",
    "6. Reboot System"
};
static const int NUM_MENU_ITEMS = 6;

DisplayManager::DisplayManager()
    : tft(TFT_CS, TFT_AO, TFT_SDA, TFT_SCK, TFT_RST) {}

static int16_t centeredTextX(const String& text, uint8_t textSize = 1, int16_t screenWidth = 160) {
    int16_t textWidth = text.length() * 6 * textSize;
    return (screenWidth - textWidth) / 2;
}

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

    // Title "RF24 SUITE" (textSize 1, centered)
    String title = "RF24 SUITE";
    int16_t titleX = centeredTextX(title, 1);
    tft.setCursor(titleX, 20);
    tft.print(title);

    // Draw logo from byte array (centered)
    int logoWidth = 64;
    int logoHeight = 64;
    int logoX = (160 - logoWidth) / 2;
    int logoY = (128 - logoHeight) / 2;
    // splashLogo is an RGB888 bitmap stored as 32-bit unsigned long in PROGMEM
    // (64x64 pixels, one pixel per unsigned long with format 0x00RRGGBB).
    // Adafruit_GFX::drawBitmap() expects 1-bit uint8_t data, and drawRGBBitmap()
    // expects 16-bit RGB565 — neither matches the actual data type. We draw
    // pixel-by-pixel, reading from PROGMEM and converting RGB888 -> RGB565.
    tft.startWrite();
    for (int16_t j = 0; j < logoHeight; j++) {
        for (int16_t i = 0; i < logoWidth; i++) {
            uint32_t pixel = pgm_read_dword(&splashLogo[j * logoWidth + i]);
            uint8_t r = (pixel >> 16) & 0xFF;
            uint8_t g = (pixel >> 8) & 0xFF;
            uint8_t b = pixel & 0xFF;
            tft.writePixel(logoX + i, logoY + j, tft.color565(r, g, b));
        }
    }
    tft.endWrite();

    // Subtitle "by Dx4Grey" (textSize 1, centered below the splashLogo)
    String subtitle = "by Dx4Grey";
    int16_t subtitleX = centeredTextX(subtitle, 1);
    tft.setCursor(subtitleX, logoY + logoHeight + 4);
    tft.print(subtitle);

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
// MENU ITEM HELPER (single row renderer)
// =============================================================================
void DisplayManager::drawMenuItem(int index, bool selected) {
    int y = 16 + (index * 15);

    // Clear any previous state in this row first
    tft.fillRect(6, y - 2, 148, 14, ST77XX_BLACK);

    if (selected) {
        tft.fillRect(6, y - 2, 148, 14, 0x2124); // Highlight box
        tft.setTextColor(ST77XX_YELLOW, 0x2124);
        tft.setCursor(10, y + 1);
        tft.print("> ");
    } else {
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.setCursor(10, y + 1);
        tft.print("  ");
    }
    tft.print(menuTitles[index]);
}

// =============================================================================
// PARTIAL MENU REDRAW (only the two affected items)
// =============================================================================
void DisplayManager::redrawMenuItems(int oldSel, int newSel) {
    if (oldSel != newSel) {
        drawMenuItem(oldSel, false);  // un-highlight previously selected
        drawMenuItem(newSel, true);   // highlight newly selected
    }
}

// =============================================================================
// RENDER MAIN MENU (COMPACT & FIT)
// =============================================================================
void DisplayManager::renderMainMenu() {
    tft.fillScreen(ST77XX_BLACK);
    
    // Header Banner
    tft.fillRect(0, 0, 160, 14, 0x10A2); // Dark cyan banner
    String headerText = "==== RF24 SUITE ====";
    tft.setTextSize(1);
    int16_t headerWidth = headerText.length() * 6;
    int16_t headerX = (160 - headerWidth) / 2;
    tft.setCursor(headerX, 3);
    tft.setTextColor(ST77XX_CYAN, 0x10A2);
    tft.println(headerText);

    // Menu List (Spacing 18px)
    for (int i = 0; i < NUM_MENU_ITEMS; i++) {
        drawMenuItem(i, i == menuSelection);
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

    // Target Selection Box (Height 34px)
    tft.drawRect(6, 17, 148, 34, ST77XX_CYAN);
    tft.setCursor(10, 20);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("Target: ");
    tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    tft.println(appState.getJammerTargetName());

    tft.setCursor(10, 31);
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.println(appState.getJammerFreqRangeStr());

    // Status Box (Active / Standby, Height 34px)
    int statusY = 54;
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

    // Power & Dwell Info
    tft.setCursor(6, 94);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("RF: ");
    tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    tft.print(appState.getPowerLevelName());
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print(" | Dw: ");
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.print(appState.dwellTimeUs);
    tft.print("us");

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
// =============================================================================
// RENDER RF SETTINGS SCREEN (POWER LEVEL & DWELL TIME)
// =============================================================================
void DisplayManager::renderSettingsScreen() {
    tft.fillScreen(ST77XX_BLACK);

    // Header Banner
    tft.fillRect(0, 0, 160, 14, 0x18F5); // Dark Slate Blue
    tft.setTextSize(1);
    String headerText = "RF POWER & DWELL";
    int16_t headerWidth = headerText.length() * 6;
    int16_t headerX = (160 - headerWidth) / 2;
    tft.setCursor(headerX, 3);
    tft.setTextColor(ST77XX_WHITE, 0x18F5);
    tft.println(headerText);

    // 1. Power Level Box (y = 18 to 56, Height 38px)
    bool pwrSelected = (settingsSelection == 0);
    uint16_t pwrBorder = pwrSelected ? ST77XX_YELLOW : ST77XX_DARKGRAY;
    if (pwrSelected) {
        tft.fillRect(6, 18, 148, 38, 0x2124);
    }
    tft.drawRect(6, 18, 148, 38, pwrBorder);

    tft.setCursor(10, 22);
    tft.setTextColor(pwrSelected ? ST77XX_YELLOW : ST77XX_GRAY, pwrSelected ? 0x2124 : ST77XX_BLACK);
    tft.print(pwrSelected ? "> 1. TX Power Level" : "  1. TX Power Level");

    tft.setCursor(16, 34);
    uint16_t pwrColor = ST77XX_WHITE;
    if (appState.powerLevel == RF24_PA_MAX) pwrColor = ST77XX_RED;
    else if (appState.powerLevel == RF24_PA_HIGH) pwrColor = CUSTOM_ORANGE;
    else if (appState.powerLevel == RF24_PA_LOW) pwrColor = ST77XX_YELLOW;
    else pwrColor = ST77XX_GREEN;

    tft.setTextColor(pwrColor, pwrSelected ? 0x2124 : ST77XX_BLACK);
    tft.print("[ ");
    tft.print(appState.getPowerLevelName());
    tft.print(" ]");

    tft.setCursor(16, 44);
    tft.setTextColor(ST77XX_GRAY, pwrSelected ? 0x2124 : ST77XX_BLACK);
    if (appState.powerLevel == RF24_PA_MAX) {
        tft.print("Full 0dBm + LNA Boost");
    } else if (appState.powerLevel == RF24_PA_HIGH) {
        tft.print("Moderate Jam Range");
    } else if (appState.powerLevel == RF24_PA_LOW) {
        tft.print("Close Range Test");
    } else {
        tft.print("Low Power Safe Test");
    }

    // 2. Dwell Time Box (y = 60 to 98, Height 38px)
    bool dwellSelected = (settingsSelection == 1);
    uint16_t dwellBorder = dwellSelected ? ST77XX_YELLOW : ST77XX_DARKGRAY;
    if (dwellSelected) {
        tft.fillRect(6, 60, 148, 38, 0x2124);
    }
    tft.drawRect(6, 60, 148, 38, dwellBorder);

    tft.setCursor(10, 64);
    tft.setTextColor(dwellSelected ? ST77XX_YELLOW : ST77XX_GRAY, dwellSelected ? 0x2124 : ST77XX_BLACK);
    tft.print(dwellSelected ? "> 2. Channel Dwell" : "  2. Channel Dwell");

    tft.setCursor(16, 76);
    tft.setTextColor(ST77XX_CYAN, dwellSelected ? 0x2124 : ST77XX_BLACK);
    tft.print("[ ");
    tft.print(appState.getDwellTimeName());
    tft.print(" ]");

    tft.setCursor(16, 86);
    tft.setTextColor(ST77XX_GRAY, dwellSelected ? 0x2124 : ST77XX_BLACK);
    if (appState.dwellTimeUs <= 50) {
        tft.print("Hyper-fast RF cycle");
    } else if (appState.dwellTimeUs <= 100) {
        tft.print("Fast sweep repetition");
    } else if (appState.dwellTimeUs <= 200) {
        tft.print("Recommended balanced");
    } else if (appState.dwellTimeUs <= 500) {
        tft.print("Heavy airtime impact");
    } else {
        tft.print("Long dwell blast / ch");
    }

    // Footer
    tft.setCursor(4, 110);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("UP/DN:Sel | RT:Next | B:Menu");
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
        tft.println("CONNECTED");
    } else {
        tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
        tft.println("NOT FOUND");
    }

    tft.setCursor(8, 34);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("Chipset    : ");
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.println("Xtensa");

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
    tft.println(String(appState.getPowerLevelName()));

    tft.setCursor(8, 90);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("Dwell Time : ");
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.println(String(appState.dwellTimeUs) + " us");

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
        String headerText = "SYSTEM REBOOT";
        int16_t headerWidth = headerText.length() * 6;
        int16_t headerX = (160 - headerWidth) / 2;
        tft.setCursor(headerX, 3);
        tft.println(headerText);

        // Main Message
        String mainMessage = "REBOOTING";
        int16_t mainWidth = mainMessage.length() * 6;
        int16_t mainX = (160 - mainWidth) / 2;
        tft.setCursor(mainX, 44);
        tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
        tft.print(mainMessage);

        tft.setCursor(38, 62);
        tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
        tft.print("PLEASE WAIT");

        tft.setCursor(8, 100);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.print("Restarting ESP32...");

        needRedraw = false;
    }
    String dotProgReboot = "REBOOTING";
    int dots = (millis() / 250) % 4;
    for (int d = 0; d < dots; d++) {
        dotProgReboot += ".";
    }
    int16_t dotProgRebootWidth = dotProgReboot.length() * 6;
    int16_t dotX = (160 - dotProgRebootWidth) / 2;
    tft.fillRect(0, 44, 160, 8, ST77XX_BLACK);
    tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    tft.setCursor(dotX, 44);
    tft.print(dotProgReboot);
}

// =============================================================================
// UPDATE UI DISPATCHER
// =============================================================================
void DisplayManager::updateUI() {
    switch (appState.appMode) {
        case APP_MODE_MENU:
            if (menuNeedsPartialRedraw) {
                redrawMenuItems(prevMenuSelection, menuSelection);
                menuNeedsPartialRedraw = false;
            } else if (needRedraw) {
                renderMainMenu();
                needRedraw = false;
                menuNeedsPartialRedraw = false;
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
        case APP_MODE_SETTINGS:
            if (needRedraw) {
                renderSettingsScreen();
                needRedraw = false;
            }
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
            prevMenuSelection = menuSelection;
            menuSelection = (menuSelection - 1 + NUM_MENU_ITEMS) % NUM_MENU_ITEMS;
            menuNeedsPartialRedraw = true;
        } else if (buttonManager.isPressed(BTN_DOWN)) {
            prevMenuSelection = menuSelection;
            menuSelection = (menuSelection + 1) % NUM_MENU_ITEMS;
            menuNeedsPartialRedraw = true;
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
                appState.appMode = APP_MODE_SETTINGS;
            } else if (menuSelection == 4) {
                appState.appMode = APP_MODE_STATUS;
            } else if (menuSelection == 5) {
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
    // CONDITION 5: RF SETTINGS
    // -------------------------------------------------------------------------
    else if (appState.appMode == APP_MODE_SETTINGS) {
        if (buttonManager.isPressed(BTN_UP) || buttonManager.isPressed(BTN_DOWN)) {
            settingsSelection = (settingsSelection + 1) % 2;
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_RIGHT)) {
            if (settingsSelection == 0) {
                appState.cyclePowerLevel(1);
                radioManager.updatePALevel(appState.powerLevel);
            } else {
                appState.cycleDwellTime(1);
            }
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_B)) {
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
    // -------------------------------------------------------------------------
    // CONDITION 6: STATUS
    // -------------------------------------------------------------------------
    else if (appState.appMode == APP_MODE_STATUS) {
        if (buttonManager.isPressed(BTN_B) || buttonManager.isPressed(BTN_RIGHT)) {
            appState.appMode = APP_MODE_MENU;
            needRedraw = true;
        }
    }
}
