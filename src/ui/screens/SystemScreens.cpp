#include "ui/DisplayManager.h"
#include "ui/DisplaySupport.h"
#include "drivers/RadioManager.h"

using namespace DisplayUi;

void DisplayManager::renderSettingsScreen() {
    if (!settingsLayoutDrawn) {
        drawModernHeader("RF SETTINGS", SPECTRUM_HIGH);
        drawModernFooter("U/D SEL", "R NEXT", "B BACK");
        settingsLayoutDrawn = true;
    }

    auto drawSettingCard = [&](int item, int y, const char* label) {
        const bool selected = settingsSelection == item;
        const uint16_t background = selected ?
                                    SPECTRUM_HEADER_BG : SPECTRUM_CARD_BG;
        tft.fillRect(5, y, 150, 26, ST77XX_BLACK);
        tft.fillRoundRect(5, y, 150, 26, 4, background);
        tft.drawRoundRect(5, y, 150, 26, 4,
                          selected ? SPECTRUM_ACCENT : SPECTRUM_BORDER);
        if (selected) {
            tft.fillRoundRect(8, y + 4, 3, 18, 1, SPECTRUM_ACCENT);
        }
        tft.setCursor(15, y + 4);
        tft.setTextColor(selected ? SPECTRUM_ACCENT : ST77XX_GRAY,
                         background);
        tft.print(label);
        return background;
    };

    const uint16_t pwrBg = drawSettingCard(0, 17, "TX POWER");
    tft.setCursor(15, 31);
    uint16_t pwrColor = ST77XX_WHITE;
    if (appState.powerLevel == RF24_PA_MAX) pwrColor = SPECTRUM_CRITICAL;
    else if (appState.powerLevel == RF24_PA_HIGH) pwrColor = SPECTRUM_HIGH;
    else if (appState.powerLevel == RF24_PA_LOW) pwrColor = SPECTRUM_MID;
    else pwrColor = SPECTRUM_LOW;
    tft.setTextColor(pwrColor, pwrBg);
    tft.print(appState.getPowerLevelName());

    const uint16_t dwellBg = drawSettingCard(1, 45, "SAMPLE DWELL");
    tft.setCursor(15, 59);
    tft.setTextColor(SPECTRUM_ACCENT, dwellBg);
    tft.print(appState.getDwellTimeName());

    const uint16_t themeBg = drawSettingCard(2, 73, "DISPLAY THEME");
    tft.setCursor(15, 87);
    tft.setTextColor(SPECTRUM_ACCENT, themeBg);
    tft.print(appState.getDisplayThemeName());

    // Compact palette preview for the active theme.
    const uint16_t swatches[5] = {
        SPECTRUM_LOW, SPECTRUM_MID, SPECTRUM_HIGH,
        SPECTRUM_CRITICAL, SPECTRUM_ACCENT
    };
    for (int i = 0; i < 5; i++) {
        tft.fillRoundRect(98 + i * 10, 86, 8, 8, 2, swatches[i]);
    }

    previousSettingsSelection = settingsSelection;
    previousPowerLevel = static_cast<int>(appState.powerLevel);
    previousDwellTimeUs = appState.dwellTimeUs;
}

// =============================================================================
// RENDER STATUS SCREEN (COMPACT & FIT)
// =============================================================================
void DisplayManager::renderStatusScreen() {
    static const char* pageTitles[] = {"DEVICE INFO", "MEMORY INFO", "RADIO / SW"};
    const char* labels[6];
    String values[6];
    uint16_t colors[6] = {
        ST77XX_WHITE, ST77XX_WHITE, ST77XX_WHITE,
        ST77XX_WHITE, ST77XX_WHITE, ST77XX_WHITE
    };

    if (statusPage == 0) {
        labels[0] = "CHIP";      values[0] = String(ESP.getChipModel());
        labels[1] = "REV/CORE";  values[1] = "r" + String(ESP.getChipRevision()) + " / " +
                                              String(ESP.getChipCores()) + " cores";
        labels[2] = "CPU";       values[2] = String(ESP.getCpuFreqMHz()) + " MHz";
        labels[3] = "FLASH";     values[3] = formatBytesShort(ESP.getFlashChipSize());
        labels[4] = "FLASH CLK"; values[4] = String(ESP.getFlashChipSpeed() / 1000000UL) + " MHz";
        labels[5] = "UPTIME";    values[5] = formatUptime(millis());
        colors[0] = SPECTRUM_ACCENT;
        colors[5] = SPECTRUM_LOW;
    } else if (statusPage == 1) {
        labels[0] = "HEAP TOTAL"; values[0] = formatBytesShort(ESP.getHeapSize());
        labels[1] = "HEAP FREE";  values[1] = formatBytesShort(ESP.getFreeHeap());
        labels[2] = "HEAP MIN";   values[2] = formatBytesShort(ESP.getMinFreeHeap());
        labels[3] = "MAX BLOCK";  values[3] = formatBytesShort(ESP.getMaxAllocHeap());
        labels[4] = "SKETCH";     values[4] = formatBytesShort(ESP.getSketchSize()) + " used";
        const uint32_t psramTotal = ESP.getPsramSize();
        labels[5] = "PSRAM";
        values[5] = psramTotal == 0 ? String("NOT PRESENT") :
                    formatBytesShort(ESP.getFreePsram()) + " free";
        colors[1] = SPECTRUM_LOW;
        colors[2] = SPECTRUM_HIGH;
        colors[5] = psramTotal == 0 ? ST77XX_GRAY : SPECTRUM_LOW;
    } else {
        const bool radio1Ok = radioManager.isRadio1Connected();
        const bool radio2Ok = radioManager.isRadio2Connected();
        labels[0] = "RADIO 1";   values[0] = radio1Ok ? "CONNECTED" : "NOT FOUND";
        labels[1] = "RADIO 2";   values[1] = radio2Ok ? "CONNECTED" : "NOT FOUND";
        labels[2] = "SCAN MODE"; values[2] = appState.getAnalyzerRadioModeName();
        labels[3] = "RF POWER";  values[3] = appState.getPowerLevelName();
        labels[4] = "ESP-IDF";   values[4] = ESP.getSdkVersion();
        labels[5] = "BUILD";     values[5] = __DATE__;
        colors[0] = radio1Ok ? SPECTRUM_LOW : SPECTRUM_CRITICAL;
        colors[1] = radio2Ok ? SPECTRUM_LOW : SPECTRUM_CRITICAL;
        colors[2] = SPECTRUM_ACCENT;
        colors[3] = SPECTRUM_HIGH;
    }

    const bool layoutChanged = renderedStatusPage != statusPage;
    if (layoutChanged) {
        drawModernHeader(pageTitles[statusPage], SPECTRUM_LOW);
        tft.fillRoundRect(137, 2, 20, 10, 3, SPECTRUM_BORDER);
        tft.setCursor(139, 3);
        tft.setTextColor(SPECTRUM_ACCENT, SPECTRUM_BORDER);
        tft.print(statusPage + 1);
        tft.print("/3");
        tft.fillRoundRect(5, 17, 150, 86, 4, SPECTRUM_CARD_BG);
        tft.drawRoundRect(5, 17, 150, 86, 4, SPECTRUM_BORDER);
        drawModernFooter("U/D PAGE", "R REF", "B BACK");
        for (int row = 0; row < 6; row++) previousStatusValues[row] = "";
    }

    for (int row = 0; row < 6; row++) {
        const int y = 21 + row * 13;
        if (layoutChanged) {
            if (row > 0) tft.drawFastHLine(11, y - 3, 138, SPECTRUM_GRID);
            tft.setCursor(11, y);
            tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
            tft.print(labels[row]);
        }
        if (layoutChanged || previousStatusValues[row] != values[row] ||
            previousStatusColors[row] != colors[row]) {
            tft.fillRect(76, y, 73, 8, SPECTRUM_CARD_BG);
            const int valueX = max(77, 149 - static_cast<int>(values[row].length()) * 6);
            tft.setCursor(valueX, y);
            tft.setTextColor(colors[row], SPECTRUM_CARD_BG);
            tft.print(values[row]);
            previousStatusValues[row] = values[row];
            previousStatusColors[row] = colors[row];
        }
    }

    renderedStatusPage = statusPage;
    lastStatusRenderMs = millis();
}

// =============================================================================
// POWER MENU
// =============================================================================
void DisplayManager::renderPowerScreen() {
    drawModernHeader("POWER OPTIONS", SPECTRUM_HIGH);

    const char* titles[2] = {"RESTART", "SHUTDOWN"};
    const char* subtitles[2] = {"Reboot firmware", "Enter deep sleep"};
    for (int item = 0; item < 2; item++) {
        const int y = 22 + item * 39;
        const bool selected = powerSelection == item;
        const uint16_t background = selected ? SPECTRUM_HEADER_BG : SPECTRUM_CARD_BG;
        const uint16_t accent = item == 0 ? SPECTRUM_ACCENT : SPECTRUM_CRITICAL;
        tft.fillRect(10, y, 140, 32, ST77XX_BLACK);
        tft.fillRoundRect(10, y, 140, 32, 5, background);
        tft.drawRoundRect(10, y, 140, 32, 5,
                          selected ? accent : SPECTRUM_BORDER);
        if (selected) tft.fillRoundRect(13, y + 5, 3, 22, 1, accent);
        tft.setCursor(22, y + 6);
        tft.setTextColor(selected ? ST77XX_WHITE : ST77XX_GRAY, background);
        tft.print(titles[item]);
        tft.setCursor(22, y + 18);
        tft.setTextColor(selected ? accent : ST77XX_GRAY, background);
        tft.print(subtitles[item]);
    }

    drawModernFooter("U/D SEL", "R OK", "B BACK");
}

void DisplayManager::renderShutdownScreen() {
    if (!needRedraw) return;

    drawModernHeader("POWER OFF", SPECTRUM_CRITICAL);
    tft.fillRoundRect(14, 23, 132, 75, 7, SPECTRUM_CARD_BG);
    tft.drawRoundRect(14, 23, 132, 75, 7, SPECTRUM_BORDER);
    tft.drawCircle(80, 44, 10, SPECTRUM_CRITICAL);
    tft.fillRect(78, 30, 5, 14, SPECTRUM_CARD_BG);
    tft.drawFastVLine(80, 29, 14, SPECTRUM_CRITICAL);
    tft.setCursor(47, 62);
    tft.setTextColor(ST77XX_WHITE, SPECTRUM_CARD_BG);
    tft.print("SHUTTING DOWN");
    tft.setCursor(29, 78);
    tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
    tft.print("Hold RIGHT to wake");
    tft.setCursor(45, 110);
    tft.setTextColor(SPECTRUM_ACCENT, ST77XX_BLACK);
    tft.print("DEEP SLEEP");
    needRedraw = false;
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
