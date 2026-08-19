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

// Shared modern UI palette (RGB565).
static constexpr uint16_t SPECTRUM_HEADER_BG = 0x0862;
static constexpr uint16_t SPECTRUM_CARD_BG   = 0x0021;
static constexpr uint16_t SPECTRUM_BORDER    = 0x1ACB;
static constexpr uint16_t SPECTRUM_GRID      = 0x10E4;
static constexpr uint16_t SPECTRUM_ACCENT    = 0x05FF;
static constexpr uint16_t SPECTRUM_LOW       = 0x05F4;
static constexpr uint16_t SPECTRUM_MID       = 0xBFE0;
static constexpr uint16_t SPECTRUM_HIGH      = 0xFD20;
static constexpr uint16_t SPECTRUM_CRITICAL  = 0xF94F;

static const char* compactBandName(AnalyzerBand band) {
    switch (band) {
        case SCAN_BAND_WIFI: return "WIFI";
        case SCAN_BAND_BT:   return "BT";
        case SCAN_BAND_ALL:
        default:             return "ALL";
    }
}

DisplayManager::DisplayManager()
    : tft(TFT_CS, TFT_AO, TFT_SDA, TFT_SCK, TFT_RST) {
    resetDynamicCaches();
}

void DisplayManager::resetDynamicCaches() {
    memset(previousSpectrumLevels, 0xFF, sizeof(previousSpectrumLevels));
    memset(previousPeakLevels, 0xFF, sizeof(previousPeakLevels));
    previousHeaderPeakLevel = 0xFF;
    previousHeaderPeakChannel = -1;
    previousHeaderRadio1Level = 0xFF;
    previousHeaderRadio2Level = 0xFF;
    previousInspectedLevel = 0xFF;
    previousInspectedPeak = 0xFF;
    carrierStatusValid = false;
    lastSpectrumRenderMs = 0;
    lastInspectorRenderMs = 0;
    lastJammerRenderMs = 0;
    jammerLayoutDrawn = false;
    settingsLayoutDrawn = false;
    previousJammerTarget = -1;
    previousJamChannel = -1;
    previousPowerLevel = -1;
    previousDwellTimeUs = -1;
    previousSettingsSelection = -1;
    jammingStatusValid = false;
}

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

    tft.fillRoundRect(45, 28, 70, 70, 6, SPECTRUM_CARD_BG);
    tft.drawRoundRect(45, 28, 70, 70, 6, SPECTRUM_BORDER);

    String title = "RF24 SUITE";
    int16_t titleX = centeredTextX(title, 1);
    tft.setCursor(titleX, 10);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.print(title);
    tft.drawFastHLine(54, 21, 52, SPECTRUM_ACCENT);

    // Draw logo from byte array (centered)
    int logoWidth = 64;
    int logoHeight = 64;
    int logoX = (160 - logoWidth) / 2;
    int logoY = 31;
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
    tft.setTextColor(SPECTRUM_ACCENT, ST77XX_BLACK);
    tft.setCursor(subtitleX, 106);
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

void DisplayManager::drawModernHeader(const char* title, uint16_t accent) {
    tft.fillRect(0, 0, 160, 14, SPECTRUM_HEADER_BG);
    tft.drawFastHLine(0, 13, 160, accent);
    tft.fillCircle(7, 7, 3, accent);
    tft.drawCircle(7, 7, 5, SPECTRUM_BORDER);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE, SPECTRUM_HEADER_BG);
    tft.setCursor(centeredTextX(String(title)), 3);
    tft.print(title);
}

void DisplayManager::drawFooterChip(int x, int width, const char* label) {
    tft.fillRoundRect(x, 107, width, 16, 3, 0x10A2);
    int textX = x + (width - static_cast<int>(strlen(label)) * 6) / 2;
    tft.setCursor(textX, 111);
    tft.setTextColor(SPECTRUM_ACCENT, 0x10A2);
    tft.print(label);
}

void DisplayManager::drawModernFooter(const char* left, const char* middle, const char* right) {
    tft.fillRect(0, 105, 160, 23, ST77XX_BLACK);
    if (left && left[0]) drawFooterChip(3, 48, left);
    if (middle && middle[0]) drawFooterChip(56, 49, middle);
    if (right && right[0]) drawFooterChip(110, 47, right);
}

// =============================================================================
// MENU ITEM HELPER (single row renderer)
// =============================================================================
void DisplayManager::drawMenuItem(int index, bool selected) {
    int y = 16 + (index * 15);

    tft.fillRect(4, y - 1, 152, 13, ST77XX_BLACK);

    if (selected) {
        tft.fillRoundRect(4, y - 1, 152, 13, 3, SPECTRUM_CARD_BG);
        tft.drawRoundRect(4, y - 1, 152, 13, 3, SPECTRUM_BORDER);
        tft.fillRoundRect(7, y + 2, 3, 7, 1, SPECTRUM_ACCENT);
        tft.setTextColor(ST77XX_WHITE, SPECTRUM_CARD_BG);
        tft.setCursor(14, y + 2);
    } else {
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.setCursor(14, y + 2);
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
    drawModernHeader("RF24 CONTROL", SPECTRUM_ACCENT);

    // Menu List (Spacing 18px)
    for (int i = 0; i < NUM_MENU_ITEMS; i++) {
        drawMenuItem(i, i == menuSelection);
    }

    drawModernFooter("U/D SEL", "", "R OPEN");
}

// =============================================================================
// RENDER JAMMER SCREEN (COMPACT & FIT)
// =============================================================================
void DisplayManager::renderJammerScreen() {
    if (!jammerLayoutDrawn) {
        drawModernHeader("RF CONTROL", SPECTRUM_HIGH);
        tft.fillRoundRect(5, 17, 150, 34, 4, SPECTRUM_CARD_BG);
        tft.drawRoundRect(5, 17, 150, 34, 4, SPECTRUM_BORDER);
        tft.fillRoundRect(5, 54, 150, 34, 4, SPECTRUM_CARD_BG);
        tft.drawRoundRect(5, 54, 150, 34, 4, SPECTRUM_BORDER);
        tft.fillRoundRect(5, 92, 150, 11, 3, SPECTRUM_CARD_BG);
        drawModernFooter("U/D TGT", "R START", "B BACK");
        jammerLayoutDrawn = true;
    }

    if (previousJammerTarget != static_cast<int>(appState.jammerTarget)) {
        tft.fillRect(8, 20, 144, 27, SPECTRUM_CARD_BG);
        tft.setCursor(10, 21);
        tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
        tft.print("TARGET  ");
        tft.setTextColor(ST77XX_WHITE, SPECTRUM_CARD_BG);
        tft.println(appState.getJammerTargetName());
        tft.setCursor(10, 35);
        tft.setTextColor(SPECTRUM_ACCENT, SPECTRUM_CARD_BG);
        tft.println(appState.getJammerFreqRangeStr());
        previousJammerTarget = static_cast<int>(appState.jammerTarget);
    }

    // Status Box (Active / Standby, Height 34px)
    int statusY = 54;
    if (!jammingStatusValid || previousJamming != appState.jamming ||
        (appState.jamming && previousJamChannel != appState.currentJamChannel)) {
        if (appState.jamming) {
            const uint16_t activeBg = 0x4004;
            tft.fillRoundRect(5, statusY, 150, 34, 4, activeBg);
            tft.drawRoundRect(5, statusY, 150, 34, 4, SPECTRUM_CRITICAL);
            tft.fillCircle(13, statusY + 9, 3, SPECTRUM_CRITICAL);
            tft.setCursor(20, statusY + 6);
            tft.setTextColor(ST77XX_WHITE, activeBg);
            tft.print("TRANSMIT ACTIVE");
            tft.setCursor(20, statusY + 20);
            tft.print("Ch: ");
            tft.print(appState.currentJamChannel);
            tft.print(" (");
            tft.print(2400 + appState.currentJamChannel);
            tft.print(" MHz)   ");
        } else {
            tft.fillRoundRect(5, statusY, 150, 34, 4, SPECTRUM_CARD_BG);
            tft.drawRoundRect(5, statusY, 150, 34, 4, SPECTRUM_BORDER);
            tft.fillCircle(13, statusY + 17, 3, SPECTRUM_LOW);
            tft.setCursor(22, statusY + 14);
            tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
            tft.print("READY / STANDBY");
        }
        previousJamming = appState.jamming;
        previousJamChannel = appState.currentJamChannel;
        jammingStatusValid = true;
    }

    // Power & Dwell Info
    if (previousPowerLevel != static_cast<int>(appState.powerLevel) ||
        previousDwellTimeUs != appState.dwellTimeUs) {
        tft.fillRoundRect(5, 92, 150, 11, 3, SPECTRUM_CARD_BG);
        tft.setCursor(9, 94);
        tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
        tft.print("PWR ");
        tft.setTextColor(SPECTRUM_HIGH, SPECTRUM_CARD_BG);
        tft.print(appState.getPowerLevelName());
        tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
        tft.print("  DW ");
        tft.setTextColor(SPECTRUM_ACCENT, SPECTRUM_CARD_BG);
        tft.print(appState.dwellTimeUs);
        tft.print("us");
        previousPowerLevel = static_cast<int>(appState.powerLevel);
        previousDwellTimeUs = appState.dwellTimeUs;
    }
}

// =============================================================================
// RENDER RADIO SPECTRUM ANALYZER (LIVE RF GRAPH - COMPACT)
// =============================================================================
void DisplayManager::drawSpectrumGrid() {
    // Compact status header with a small signal glyph and band badge.
    tft.fillRect(0, 0, 160, 14, SPECTRUM_HEADER_BG);
    tft.drawFastHLine(0, 13, 160, SPECTRUM_ACCENT);
    tft.drawFastVLine(4, 8, 3, SPECTRUM_ACCENT);
    tft.drawFastVLine(7, 6, 5, SPECTRUM_ACCENT);
    tft.drawFastVLine(10, 3, 8, SPECTRUM_ACCENT);
    tft.setCursor(15, 3);
    tft.setTextColor(ST77XX_WHITE, SPECTRUM_HEADER_BG);
    tft.print("SPECTRUM");

    tft.fillRoundRect(66, 2, 31, 10, 3, SPECTRUM_BORDER);
    const char* band = compactBandName(appState.analyzerBand);
    int bandX = 66 + (31 - static_cast<int>(strlen(band)) * 6) / 2;
    tft.setCursor(bandX, 3);
    tft.setTextColor(SPECTRUM_ACCENT, SPECTRUM_BORDER);
    tft.print(band);

    // Chart card and dotted horizontal guides.
    tft.fillRect(16, 15, 130, 67, SPECTRUM_CARD_BG);
    tft.drawRect(16, 15, 130, 67, SPECTRUM_BORDER);
    for (int x = GRAPH_X_START; x < GRAPH_X_START + GRAPH_WIDTH; x += 4) {
        tft.drawPixel(x, GRAPH_Y_TOP, SPECTRUM_GRID);
        tft.drawPixel(x, GRAPH_Y_TOP + (GRAPH_HEIGHT / 2), SPECTRUM_GRID);
    }
    tft.drawFastHLine(GRAPH_X_START, GRAPH_Y_BASELINE, GRAPH_WIDTH, SPECTRUM_BORDER);

    // Minimal Y-axis labels leave more room for the actual signal plot.
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.setCursor(0, GRAPH_Y_TOP - 2);
    tft.print("100");
    tft.setCursor(3, GRAPH_Y_TOP + (GRAPH_HEIGHT / 2) - 3);
    tft.print("50");
    tft.setCursor(9, GRAPH_Y_BASELINE - 5);
    tft.print("0");

    // X-axis: Popular Wi-Fi Channel Markers (1, 6, 11, 14)
    const int wifiMarkers[4] = {12, 37, 62, 84};
    const char* wifiLabels[4] = {"1", "6", "11", "14"};

    for (int i = 0; i < 4; i++) {
        int markerX = GRAPH_X_START + wifiMarkers[i];
        tft.drawFastVLine(markerX, GRAPH_Y_BASELINE + 1, 3, SPECTRUM_ACCENT);
        tft.setCursor(markerX - 2, GRAPH_Y_BASELINE + 4);
        tft.setTextColor(SPECTRUM_ACCENT, ST77XX_BLACK);
        tft.print(wifiLabels[i]);
    }

    tft.setCursor(135, GRAPH_Y_BASELINE + 4);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print("GHz");

    tft.fillRect(0, 105, 160, 23, ST77XX_BLACK);
    drawFooterChip(2, 37, "U BAND");
    String modeChip = "D ";
    modeChip += appState.getAnalyzerRadioModeName();
    drawFooterChip(41, 37, modeChip.c_str());
    drawFooterChip(80, 37, "R RST");
    drawFooterChip(119, 39, "B BACK");
}

void DisplayManager::drawSpectrumBars() {
    const uint8_t peakRadio1 = appState.radio1Levels[appState.peakChannel];
    const uint8_t peakRadio2 = appState.radio2Levels[appState.peakChannel];
    // Header is redrawn only when its displayed value changes.
    if (previousHeaderPeakChannel != appState.peakChannel ||
        previousHeaderPeakLevel != appState.peakLevel ||
        previousHeaderRadio1Level != peakRadio1 ||
        previousHeaderRadio2Level != peakRadio2) {
        tft.fillRect(99, 1, 61, 11, SPECTRUM_HEADER_BG);
        tft.setCursor(99, 3);
        tft.setTextColor(ST77XX_GRAY, SPECTRUM_HEADER_BG);
        tft.print("P ");
        tft.setTextColor(getSignalColor(appState.peakLevel), SPECTRUM_HEADER_BG);
        tft.print(appState.peakChannel);
        tft.print(" ");
        tft.print(appState.peakLevel);
        tft.print("%");
        previousHeaderPeakChannel = appState.peakChannel;
        previousHeaderPeakLevel = appState.peakLevel;

        tft.fillRect(18, 95, 126, 8, ST77XX_BLACK);
        String radioSummary;
        if (appState.analyzerRadioMode == ANALYZER_RADIO_FAST) {
            radioSummary = "FAST  R1/R2 SPLIT";
        } else if (appState.analyzerRadioMode == ANALYZER_RADIO_DIVERSITY) {
            radioSummary = "R1:" + String(peakRadio1) + "% R2:" + String(peakRadio2) + "%";
        } else if (appState.analyzerRadioMode == ANALYZER_RADIO_1) {
            radioSummary = "RADIO 1  " + String(peakRadio1) + "%";
        } else {
            radioSummary = "RADIO 2  " + String(peakRadio2) + "%";
        }
        tft.setCursor(centeredTextX(radioSummary), 95);
        tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
        tft.print(radioSummary);
        previousHeaderRadio1Level = peakRadio1;
        previousHeaderRadio2Level = peakRadio2;
    }

    tft.startWrite();
    for (int ch = 0; ch < TOTAL_CHANNELS; ch++) {
        uint8_t lvl = appState.spectrumLevels[ch];
        uint8_t peak = appState.peakLevels[ch];
        if (previousSpectrumLevels[ch] == lvl && previousPeakLevels[ch] == peak) {
            continue;
        }

        int x = GRAPH_X_START + ch;

        int barHeight = (lvl * GRAPH_HEIGHT) / 100;
        int peakHeight = (peak * GRAPH_HEIGHT) / 100;

        int peakTop = GRAPH_Y_BASELINE - peakHeight;

        // Clear only this dirty column and restore the card's dotted guides.
        tft.writeFastVLine(x, GRAPH_Y_TOP, GRAPH_HEIGHT + 1, SPECTRUM_CARD_BG);
        if ((ch % 4) == 0) {
            tft.writePixel(x, GRAPH_Y_TOP, SPECTRUM_GRID);
            tft.writePixel(x, GRAPH_Y_TOP + (GRAPH_HEIGHT / 2), SPECTRUM_GRID);
        }
        tft.writePixel(x, GRAPH_Y_BASELINE, SPECTRUM_BORDER);

        if (barHeight > 0) {
            // A four-zone vertical gradient makes intensity readable without
            // requiring wider bars or a costly full-frame canvas.
            int remaining = barHeight;
            int cursorY = GRAPH_Y_BASELINE;
            int segment = min(remaining, (GRAPH_HEIGHT * 30) / 100);
            cursorY -= segment;
            tft.writeFastVLine(x, cursorY, segment, SPECTRUM_LOW);
            remaining -= segment;

            segment = min(remaining, (GRAPH_HEIGHT * 35) / 100);
            cursorY -= segment;
            if (segment > 0) tft.writeFastVLine(x, cursorY, segment, SPECTRUM_MID);
            remaining -= segment;

            segment = min(remaining, (GRAPH_HEIGHT * 20) / 100);
            cursorY -= segment;
            if (segment > 0) tft.writeFastVLine(x, cursorY, segment, SPECTRUM_HIGH);
            remaining -= segment;

            if (remaining > 0) {
                cursorY -= remaining;
                tft.writeFastVLine(x, cursorY, remaining, SPECTRUM_CRITICAL);
            }
        }

        // Peak marker is drawn last, so it stays visible over the gradient.
        if (peakHeight > 0 && peakTop >= GRAPH_Y_TOP) {
            tft.writePixel(x, peakTop, ST77XX_WHITE);
        }

        previousSpectrumLevels[ch] = lvl;
        previousPeakLevels[ch] = peak;
    }
    tft.endWrite();
}

void DisplayManager::renderSpectrumAnalyzer() {
    if (needRedraw) {
        drawSpectrumGrid();
        memset(previousSpectrumLevels, 0xFF, sizeof(previousSpectrumLevels));
        memset(previousPeakLevels, 0xFF, sizeof(previousPeakLevels));
        previousHeaderPeakChannel = -1;
        previousHeaderPeakLevel = 0xFF;
        previousHeaderRadio1Level = 0xFF;
        previousHeaderRadio2Level = 0xFF;
        needRedraw = false;
    }

    unsigned long now = millis();
    if (lastSpectrumRenderMs != 0 && now - lastSpectrumRenderMs < 50) return;
    lastSpectrumRenderMs = now;
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
        drawModernHeader("CHANNEL INSPECT", SPECTRUM_ACCENT);
        tft.fillRoundRect(5, 17, 150, 27, 4, SPECTRUM_CARD_BG);
        tft.drawRoundRect(5, 17, 150, 27, 4, SPECTRUM_BORDER);
        tft.fillRoundRect(5, 47, 150, 48, 4, SPECTRUM_CARD_BG);
        tft.drawRoundRect(5, 47, 150, 48, 4, SPECTRUM_BORDER);

        int ch = appState.inspectedChannel;
        int freq = 2400 + ch;

        // Channel & Frequency
        tft.setCursor(10, 20);
        tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
        tft.print("CH ");
        tft.setTextColor(SPECTRUM_HIGH, SPECTRUM_CARD_BG);
        tft.print(ch);
        tft.setTextColor(ST77XX_WHITE, SPECTRUM_CARD_BG);
        tft.print("  ");
        tft.print(freq);
        tft.print(" MHz");

        // Protocol Information Mapping
        tft.setCursor(10, 32);
        tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
        tft.print("BAND  ");
        tft.setTextColor(SPECTRUM_ACCENT, SPECTRUM_CARD_BG);
        if (ch >= 1 && ch <= 73) {
            tft.print("Wi-Fi Band");
        } else if (ch >= 74 && ch <= 80) {
            tft.print("BT High Band");
        } else {
            tft.print("RF Extended");
        }

        tft.setCursor(10, 51);
        tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
        tft.print("LIVE SIGNAL");
        tft.drawRoundRect(10, 62, 140, 13, 3, SPECTRUM_BORDER);
        drawModernFooter("U/D +/-", "R +10", "B BACK");

        needRedraw = false;
        previousInspectedLevel = 0xFF;
        previousInspectedPeak = 0xFF;
        carrierStatusValid = false;
    }

    unsigned long now = millis();
    if (lastInspectorRenderMs != 0 && now - lastInspectorRenderMs < 50) return;
    lastInspectorRenderMs = now;

    const bool valuesChanged = previousInspectedLevel != appState.inspectedLevel ||
                               previousInspectedPeak != appState.inspectedPeak;
    const bool carrierDetected = appState.inspectedLevel > 20;

    // ---------------------------------------------------------------
    // DYNAMIC part: per-frame update without a full-screen clear (anti-flicker)
    // ---------------------------------------------------------------
    // Clear the gauge area (incl. border), then redraw border + fill
    if (valuesChanged) {
        // Keep the static border intact; update only its interior.
        tft.fillRect(12, 64, 136, 9, SPECTRUM_CARD_BG);

        int barW = map(appState.inspectedLevel, 0, 100, 0, 136);
        if (barW > 0) {
            tft.fillRect(12, 64, barW, 9, getSignalColor(appState.inspectedLevel));
        }

        int peakX = 12 + map(appState.inspectedPeak, 0, 100, 0, 135);
        tft.drawFastVLine(peakX, 63, 11, ST77XX_WHITE);

        tft.fillRect(10, 79, 140, 9, SPECTRUM_CARD_BG);
        tft.setCursor(10, 80);
        tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
        tft.print("NOW ");
        tft.setTextColor(getSignalColor(appState.inspectedLevel), SPECTRUM_CARD_BG);
        tft.print(appState.inspectedLevel);
        tft.print("%   ");

        tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
        tft.print("PEAK ");
        tft.setTextColor(SPECTRUM_ACCENT, SPECTRUM_CARD_BG);
        tft.print(appState.inspectedPeak);
        tft.print("%");

        previousInspectedLevel = appState.inspectedLevel;
        previousInspectedPeak = appState.inspectedPeak;
    }

    if (!carrierStatusValid || previousCarrierDetected != carrierDetected) {
        tft.fillRect(10, 88, 140, 6, SPECTRUM_CARD_BG);
        tft.setCursor(10, 88);
        tft.setTextColor(carrierDetected ? SPECTRUM_CRITICAL : SPECTRUM_LOW,
                         SPECTRUM_CARD_BG);
        tft.print(carrierDetected ? "> RF ACTIVITY" : "> CHANNEL CLEAR");
        previousCarrierDetected = carrierDetected;
        carrierStatusValid = true;
    }
}

// =============================================================================
// RENDER STATUS SCREEN (COMPACT & FIT)
// =============================================================================
// =============================================================================
// RENDER RF SETTINGS SCREEN (POWER LEVEL & DWELL TIME)
// =============================================================================
void DisplayManager::renderSettingsScreen() {
    if (!settingsLayoutDrawn) {
        drawModernHeader("RF SETTINGS", SPECTRUM_HIGH);
        drawModernFooter("U/D SEL", "R NEXT", "B BACK");
        settingsLayoutDrawn = true;
    }

    bool pwrSelected = (settingsSelection == 0);
    uint16_t pwrBg = pwrSelected ? SPECTRUM_HEADER_BG : SPECTRUM_CARD_BG;
    tft.fillRect(5, 17, 150, 39, ST77XX_BLACK);
    tft.fillRoundRect(5, 17, 150, 39, 4, pwrBg);
    tft.drawRoundRect(5, 17, 150, 39, 4,
                      pwrSelected ? SPECTRUM_ACCENT : SPECTRUM_BORDER);
    if (pwrSelected) tft.fillRoundRect(8, 21, 3, 31, 1, SPECTRUM_ACCENT);

    tft.setCursor(15, 21);
    tft.setTextColor(pwrSelected ? SPECTRUM_ACCENT : ST77XX_GRAY, pwrBg);
    tft.print("TX POWER");

    tft.setCursor(15, 33);
    uint16_t pwrColor = ST77XX_WHITE;
    if (appState.powerLevel == RF24_PA_MAX) pwrColor = SPECTRUM_CRITICAL;
    else if (appState.powerLevel == RF24_PA_HIGH) pwrColor = SPECTRUM_HIGH;
    else if (appState.powerLevel == RF24_PA_LOW) pwrColor = SPECTRUM_MID;
    else pwrColor = SPECTRUM_LOW;
    tft.setTextColor(pwrColor, pwrBg);
    tft.print(appState.getPowerLevelName());
    tft.setCursor(15, 45);
    tft.setTextColor(ST77XX_GRAY, pwrBg);
    tft.print("Radio output level");

    bool dwellSelected = (settingsSelection == 1);
    uint16_t dwellBg = dwellSelected ? SPECTRUM_HEADER_BG : SPECTRUM_CARD_BG;
    tft.fillRect(5, 60, 150, 39, ST77XX_BLACK);
    tft.fillRoundRect(5, 60, 150, 39, 4, dwellBg);
    tft.drawRoundRect(5, 60, 150, 39, 4,
                      dwellSelected ? SPECTRUM_ACCENT : SPECTRUM_BORDER);
    if (dwellSelected) tft.fillRoundRect(8, 64, 3, 31, 1, SPECTRUM_ACCENT);

    tft.setCursor(15, 64);
    tft.setTextColor(dwellSelected ? SPECTRUM_ACCENT : ST77XX_GRAY, dwellBg);
    tft.print("SAMPLE DWELL");
    tft.setCursor(15, 76);
    tft.setTextColor(SPECTRUM_ACCENT, dwellBg);
    tft.print(appState.getDwellTimeName());
    tft.setCursor(15, 88);
    tft.setTextColor(ST77XX_GRAY, dwellBg);
    tft.print("Channel timing: ");
    tft.print(appState.dwellTimeUs);
    tft.print(" us");

    previousSettingsSelection = settingsSelection;
    previousPowerLevel = static_cast<int>(appState.powerLevel);
    previousDwellTimeUs = appState.dwellTimeUs;
}

// =============================================================================
// RENDER STATUS SCREEN (COMPACT & FIT)
// =============================================================================
void DisplayManager::renderStatusScreen() {
    drawModernHeader("SYSTEM STATUS", SPECTRUM_LOW);
    tft.fillRoundRect(5, 17, 150, 86, 4, SPECTRUM_CARD_BG);
    tft.drawRoundRect(5, 17, 150, 86, 4, SPECTRUM_BORDER);

    tft.setCursor(12, 22);
    tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
    tft.print("RADIO");
    if (radioManager.isConnected()) {
        tft.fillCircle(91, 25, 3, SPECTRUM_LOW);
        tft.setTextColor(SPECTRUM_LOW, SPECTRUM_CARD_BG);
        tft.setCursor(99, 22);
        tft.println("CONNECTED");
    } else {
        tft.fillCircle(91, 25, 3, SPECTRUM_CRITICAL);
        tft.setTextColor(SPECTRUM_CRITICAL, SPECTRUM_CARD_BG);
        tft.setCursor(99, 22);
        tft.println("NOT FOUND");
    }
    tft.drawFastHLine(11, 33, 138, SPECTRUM_GRID);

    tft.setCursor(12, 39);
    tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
    tft.print("MCU / CLOCK");
    tft.setTextColor(ST77XX_WHITE, SPECTRUM_CARD_BG);
    tft.setCursor(96, 39);
    tft.print("S3 240M");

    tft.setCursor(12, 55);
    tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
    tft.print("MEMORY        ");
    tft.setTextColor(ST77XX_WHITE, SPECTRUM_CARD_BG);
    tft.print("320 KB");

    tft.setCursor(12, 71);
    tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
    tft.print("WATCHDOG      ");
    tft.setTextColor(SPECTRUM_LOW, SPECTRUM_CARD_BG);
    tft.print("3.0s READY");

    tft.setCursor(12, 87);
    tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
    tft.print("RF CONFIG     ");
    tft.setTextColor(SPECTRUM_ACCENT, SPECTRUM_CARD_BG);
    tft.print(appState.dwellTimeUs);
    tft.print(" us");

    drawModernFooter("", "", "B BACK");
}

// =============================================================================
// RENDER REBOOT SCREEN (SYSTEM RESTART)
// =============================================================================
void DisplayManager::renderRebootScreen() {
    if (needRedraw) {
        drawModernHeader("SYSTEM RESTART", SPECTRUM_CRITICAL);
        tft.fillRoundRect(18, 25, 124, 70, 7, SPECTRUM_CARD_BG);
        tft.drawRoundRect(18, 25, 124, 70, 7, SPECTRUM_BORDER);
        tft.drawCircle(80, 45, 10, SPECTRUM_CRITICAL);
        tft.drawFastVLine(80, 32, 12, SPECTRUM_CRITICAL);
        tft.setCursor(47, 62);
        tft.setTextColor(ST77XX_WHITE, SPECTRUM_CARD_BG);
        tft.print("RESTARTING");
        tft.setCursor(44, 78);
        tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
        tft.print("PLEASE WAIT");
        tft.setCursor(42, 109);
        tft.setTextColor(SPECTRUM_ACCENT, ST77XX_BLACK);
        tft.print("RF24 SYSTEM");

        needRedraw = false;
    }
    String dotProgReboot = "REBOOTING";
    int dots = (millis() / 250) % 4;
    for (int d = 0; d < dots; d++) {
        dotProgReboot += ".";
    }
    int16_t dotProgRebootWidth = dotProgReboot.length() * 6;
    int16_t dotX = (160 - dotProgRebootWidth) / 2;
    tft.fillRect(30, 62, 100, 8, SPECTRUM_CARD_BG);
    tft.setTextColor(ST77XX_WHITE, SPECTRUM_CARD_BG);
    tft.setCursor(dotX, 62);
    tft.print(dotProgReboot);
}

// =============================================================================
// UPDATE UI DISPATCHER
// =============================================================================
void DisplayManager::updateUI() {
    const int currentMode = static_cast<int>(appState.appMode);
    if (renderedMode != currentMode) {
        // A page transition is the only time the complete framebuffer area is
        // cleared. Updates within a page use the dirty regions below.
        tft.fillScreen(ST77XX_BLACK);
        renderedMode = currentMode;
        resetDynamicCaches();
        needRedraw = true;
        menuNeedsPartialRedraw = false;
    }

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
            if (needRedraw || lastJammerRenderMs == 0 ||
                millis() - lastJammerRenderMs >= 100) {
                renderJammerScreen();
                lastJammerRenderMs = millis();
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
        if (buttonManager.isPressed(BTN_UP)) {
            appState.cycleAnalyzerBand(1);
            needRedraw = true;
        } else if (buttonManager.isPressed(BTN_DOWN)) {
            appState.cycleAnalyzerRadioMode(1);
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
