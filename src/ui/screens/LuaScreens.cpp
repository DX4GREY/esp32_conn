#include "ui/DisplayManager.h"
#include "ui/DisplaySupport.h"
#include "services/LuaEngine.h"

using namespace DisplayUi;

void DisplayManager::renderLuaScriptsScreen() {
    if (!luaScriptCount && luaRunStatus.length() == 0) {
        luaScriptCount = luaEngine.listScripts(luaScripts, LUA_UI_MAX_SCRIPTS);
        luaScriptSelection = 0;
    }
    drawModernHeader("LUA SCRIPTS", SPECTRUM_ACCENT);
    tft.fillRect(0, 15, 160, 90, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.setCursor(5, 19); tft.print("SD:/RFSuite/scripts");

    if (!luaEngine.isReady()) {
        tft.setTextColor(SPECTRUM_CRITICAL, ST77XX_BLACK);
        tft.setCursor(8, 45); tft.print("SD CARD NOT READY");
    } else if (!luaScriptCount) {
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.setCursor(12, 43); tft.print("NO .LUA FILES");
        tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
        tft.setCursor(7, 58); tft.print("RIGHT TO REFRESH");
    } else {
        const int first = max(0, static_cast<int>(luaScriptSelection) - 2);
        for (int row = 0; row < 5 && first + row < static_cast<int>(luaScriptCount); ++row) {
            const int index = first + row;
            const int y = 32 + row * 13;
            const bool selected = index == static_cast<int>(luaScriptSelection);
            if (selected) tft.fillRoundRect(3, y - 2, 154, 12, 2, SPECTRUM_CARD_BG);
            tft.setTextColor(selected ? SPECTRUM_ACCENT : ST77XX_WHITE,
                             selected ? SPECTRUM_CARD_BG : ST77XX_BLACK);
            tft.setCursor(7, y); tft.print(selected ? "> " : "  ");
            String label = luaScripts[index];
            if (label.length() > 20) label = label.substring(0, 19) + "~";
            tft.print(label);
        }
    }
    if (luaRunStatus.length()) {
        String status = luaRunStatus;
        if (status.length() > 25) status = status.substring(0, 24) + "~";
        tft.setTextColor(status.startsWith("ERROR") ? SPECTRUM_CRITICAL : SPECTRUM_LOW,
                         ST77XX_BLACK);
        tft.setCursor(4, 94); tft.print(status);
    }
    drawModernFooter("B BACK", "UP/DN", "RIGHT RUN");
}
