#include "ui/DisplayManager.h"
#include "ui/DisplaySupport.h"
#include "ui/MenuCatalog.h"

using namespace DisplayUi;

void DisplayManager::drawMenuIcon(int index, int centerX, int centerY,
                                  uint16_t color, uint16_t background) {
    switch (index) {
        case 0: // Spectrum bars
            tft.drawFastVLine(centerX - 7, centerY + 1, 5, color);
            tft.drawFastVLine(centerX - 3, centerY - 3, 9, color);
            tft.drawFastVLine(centerX + 1, centerY - 6, 12, color);
            tft.drawFastVLine(centerX + 5, centerY - 1, 7, color);
            tft.drawFastHLine(centerX - 9, centerY + 6, 18, color);
            break;
        case 1: // Waterfall/history
            for (int row = 0; row < 4; row++) {
                tft.drawFastHLine(centerX - 8 + row, centerY - 6 + row * 4,
                                  16 - row * 2, color);
            }
            break;
        case 2: // Magnifier
            tft.drawCircle(centerX - 2, centerY - 2, 6, color);
            tft.drawLine(centerX + 3, centerY + 3, centerX + 8, centerY + 8, color);
            tft.fillCircle(centerX - 2, centerY - 2, 1, color);
            break;
        case 3: // Survey chart
            tft.drawFastHLine(centerX - 9, centerY + 6, 18, color);
            tft.fillRect(centerX - 7, centerY, 3, 6, color);
            tft.fillRect(centerX - 2, centerY - 4, 3, 10, color);
            tft.fillRect(centerX + 3, centerY - 1, 3, 7, color);
            break;
        case 4: // Event marker
            tft.drawCircle(centerX, centerY, 7, color);
            tft.drawLine(centerX, centerY - 5, centerX - 2, centerY + 1, color);
            tft.drawLine(centerX - 2, centerY + 1, centerX + 3, centerY + 1, color);
            tft.drawFastVLine(centerX + 3, centerY + 1, 4, color);
            break;
        case 5: // Recording/logging
            tft.drawRoundRect(centerX - 9, centerY - 7, 18, 14, 3, color);
            tft.fillCircle(centerX, centerY, 4, color);
            break;
        case 6: // RF antenna
            tft.drawFastVLine(centerX, centerY - 4, 10, color);
            tft.fillCircle(centerX, centerY - 5, 2, color);
            tft.drawLine(centerX - 3, centerY + 5, centerX + 3, centerY + 5, color);
            tft.drawLine(centerX - 5, centerY - 3, centerX - 8, centerY, color);
            tft.drawLine(centerX + 5, centerY - 3, centerX + 8, centerY, color);
            break;
        case 7: // Dual-radio diagnostics
            tft.drawRoundRect(centerX - 9, centerY - 6, 7, 12, 2, color);
            tft.drawRoundRect(centerX + 2, centerY - 6, 7, 12, 2, color);
            tft.fillCircle(centerX - 6, centerY + 3, 1, color);
            tft.fillCircle(centerX + 5, centerY + 3, 1, color);
            break;
        case 8: // Profiles
            tft.drawRoundRect(centerX - 9, centerY - 7, 18, 5, 2, color);
            tft.drawRoundRect(centerX - 7, centerY, 14, 5, 2, color);
            tft.drawRoundRect(centerX - 5, centerY + 7, 10, 3, 1, color);
            break;
        case 9: // Settings sliders
            tft.drawFastHLine(centerX - 9, centerY - 5, 18, color);
            tft.drawFastHLine(centerX - 9, centerY, 18, color);
            tft.drawFastHLine(centerX - 9, centerY + 5, 18, color);
            tft.fillCircle(centerX - 3, centerY - 5, 2, background);
            tft.drawCircle(centerX - 3, centerY - 5, 2, color);
            tft.fillCircle(centerX + 4, centerY, 2, background);
            tft.drawCircle(centerX + 4, centerY, 2, color);
            tft.fillCircle(centerX, centerY + 5, 2, background);
            tft.drawCircle(centerX, centerY + 5, 2, color);
            break;
        case 10: // Device status
            tft.drawRoundRect(centerX - 8, centerY - 7, 16, 14, 3, color);
            tft.fillCircle(centerX, centerY - 3, 1, color);
            tft.drawFastVLine(centerX, centerY, 4, color);
            break;
        case 11: // Power/reboot
            tft.drawCircle(centerX, centerY, 7, color);
            tft.fillRect(centerX - 2, centerY - 8, 5, 7, background);
            tft.drawFastVLine(centerX, centerY - 8, 9, color);
            break;
    }
}

void DisplayManager::drawMenuItem(int index, bool selected) {
    constexpr int cardWidth = 74;
    constexpr int cardHeight = 27;
    const int column = index % 2;
    const int row = index / 2;
    const int featureIndex = MenuCatalog::featureIndex(menuPage, index);
    const MenuFeature& feature = MenuCatalog::featureAt(menuPage, index);
    const int x = 4 + column * 78;
    const int y = 16 + row * 29;
    const uint16_t background = selected ? SPECTRUM_HEADER_BG : SPECTRUM_CARD_BG;
    const uint16_t border = selected ? SPECTRUM_ACCENT : SPECTRUM_BORDER;
    const uint16_t iconColor = selected ? SPECTRUM_ACCENT : ST77XX_GRAY;

    // Clear only this card's dirty rectangle before rebuilding it.
    tft.fillRect(x, y, cardWidth, cardHeight, ST77XX_BLACK);
    tft.fillRoundRect(x, y, cardWidth, cardHeight, 4, background);
    tft.drawRoundRect(x, y, cardWidth, cardHeight, 4, border);
    if (selected) tft.fillRoundRect(x + 2, y + 5, 3, 17, 1, SPECTRUM_ACCENT);

    drawMenuIcon(feature.iconId, x + cardWidth / 2, y + 8,
                 iconColor, background);

    const int labelX = x +
        (cardWidth - static_cast<int>(strlen(feature.label)) * 6) / 2;
    tft.setCursor(labelX, y + 17);
    tft.setTextColor(selected ? ST77XX_WHITE : ST77XX_GRAY, background);
    tft.print(feature.label);

    if (featureIndex == 4 && appState.eventCount > 0) {
        tft.fillCircle(x + cardWidth - 8, y + 7, 5, SPECTRUM_HIGH);
        tft.setCursor(x + cardWidth - 11, y + 4);
        tft.setTextColor(ST77XX_BLACK, SPECTRUM_HIGH);
        tft.print(appState.eventCount);
    } else if (featureIndex == 5 && appState.loggingEnabled) {
        tft.fillCircle(x + cardWidth - 8, y + 7, 4, SPECTRUM_CRITICAL);
    }
}

// =============================================================================
// PARTIAL MENU REDRAW (only the two affected items)
// =============================================================================
void DisplayManager::redrawMenuItems(int oldSel, int newSel) {
    if (oldSel != newSel) {
        drawMenuItem(oldSel, false);
        drawMenuItem(newSel, true);
    }
}

// =============================================================================
// RENDER MAIN MENU (COMPACT & FIT)
// =============================================================================
void DisplayManager::renderMainMenu() {
    drawModernHeader(MenuCatalog::pageTitle(menuPage), SPECTRUM_ACCENT);
    tft.fillRoundRect(137, 2, 20, 10, 3, SPECTRUM_BORDER);
    tft.setCursor(139, 3);
    tft.setTextColor(SPECTRUM_ACCENT, SPECTRUM_BORDER);
    tft.print(menuPage + 1);
    tft.print("/");
    tft.print(MenuCatalog::PAGE_COUNT);

    // Six compact feature cards in a 2 x 3 grid.
    for (int i = 0; i < MenuCatalog::ITEMS_PER_PAGE; i++) {
        drawMenuItem(i, i == menuSelection);
    }

    drawModernFooter("U/D MOVE", "B PAGE", "R OPEN");
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
    const int radio1Channel = appState.currentJamChannel;
    const int radio2Channel = appState.currentJamChannel2;
    if (!jammingStatusValid || previousJamming != appState.jamming) {
        if (appState.jamming) {
            const uint16_t activeBg = 0x4004;
            tft.fillRoundRect(5, statusY, 150, 34, 4, activeBg);
            tft.drawRoundRect(5, statusY, 150, 34, 4, SPECTRUM_CRITICAL);
            tft.fillCircle(13, statusY + 9, 3, SPECTRUM_CRITICAL);
            tft.setCursor(20, statusY + 6);
            tft.setTextColor(ST77XX_WHITE, activeBg);
            tft.print("ACTIVE  CH / MHz");
        } else {
            tft.fillRoundRect(5, statusY, 150, 34, 4, SPECTRUM_CARD_BG);
            tft.drawRoundRect(5, statusY, 150, 34, 4, SPECTRUM_BORDER);
            tft.fillCircle(13, statusY + 17, 3, SPECTRUM_LOW);
            tft.setCursor(22, statusY + 14);
            tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
            tft.print("READY / STANDBY");
        }
        previousJamming = appState.jamming;
        previousJamChannel = -1;
        previousJamChannel2 = -1;
        jammingStatusValid = true;
    }

    // Only update the changing value row. Each frequency is derived from the
    // same channel snapshot so a Core 0 hop cannot produce a mismatched pair.
    if (appState.jamming &&
        (previousJamChannel != radio1Channel ||
         previousJamChannel2 != radio2Channel)) {
        const uint16_t activeBg = 0x4004;
        tft.fillRect(9, statusY + 18, 142, 11, activeBg);
        tft.setCursor(10, statusY + 20);
        tft.setTextColor(ST77XX_WHITE, activeBg);
        tft.print("R1 ");
        tft.print(radio1Channel);
        tft.print("/");
        tft.print(2400 + radio1Channel);
        tft.print(" R2 ");
        tft.print(radio2Channel);
        tft.print("/");
        tft.print(2400 + radio2Channel);
        previousJamChannel = radio1Channel;
        previousJamChannel2 = radio2Channel;
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
