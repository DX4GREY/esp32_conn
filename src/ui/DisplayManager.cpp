#include "ui/DisplayManager.h"
#include "ui/DisplaySupport.h"

using namespace DisplayUi;

DisplayManager displayManager;

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
    lastStatusRenderMs = 0;
    lastEnvRenderMs = 0;
    envRunningStatusValid = false;
    jammerLayoutDrawn = false;
    settingsLayoutDrawn = false;
    previousJammerTarget = -1;
    previousJamChannel = -1;
    previousJamChannel2 = -1;
    previousPowerLevel = -1;
    previousDwellTimeUs = -1;
    previousSettingsSelection = -1;
    jammingStatusValid = false;
    renderedStatusPage = -1;
    for (int i = 0; i < 6; i++) {
        previousStatusValues[i] = "";
        previousStatusColors[i] = 0;
    }
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

void DisplayManager::prepareForShutdown() {
    tft.enableDisplay(false);
    delay(20);
    tft.enableSleep(true);
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
    if (level < 30) return SPECTRUM_LOW;
    if (level < 65) return SPECTRUM_MID;
    if (level < 85) return SPECTRUM_HIGH;
    return SPECTRUM_CRITICAL;
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
    tft.fillRoundRect(x, 107, width, 16, 3, DISPLAY_FOOTER_BG);
    int textX = x + (width - static_cast<int>(strlen(label)) * 6) / 2;
    tft.setCursor(textX, 111);
    tft.setTextColor(SPECTRUM_ACCENT, DISPLAY_FOOTER_BG);
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
