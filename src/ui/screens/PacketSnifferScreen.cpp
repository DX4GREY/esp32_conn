#include "ui/DisplayManager.h"
#include "ui/DisplaySupport.h"
#include "services/PacketSniffer.h"

using namespace DisplayUi;

void DisplayManager::renderPacketSnifferScreen() {
    drawModernHeader("NRF24 PACKET SNIFFER", SPECTRUM_ACCENT);
    tft.fillRoundRect(5, 17, 150, 86, 4, SPECTRUM_CARD_BG);
    tft.drawRoundRect(5, 17, 150, 86, 4, SPECTRUM_BORDER);

    tft.setCursor(10, 22);
    tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
    tft.print("STATE");
    tft.setCursor(63, 22);
    tft.setTextColor(packetSniffer.isRunning() ? SPECTRUM_LOW : SPECTRUM_CRITICAL,
                     SPECTRUM_CARD_BG);
    tft.print(packetSniffer.isRunning() ? "CAPTURING" : "ERROR");

    tft.setCursor(10, 34);
    tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
    tft.print("CH / RATE");
    tft.setCursor(76, 34);
    tft.setTextColor(ST77XX_WHITE, SPECTRUM_CARD_BG);
    tft.print(packetSniffer.channel());
    tft.print(" / ");
    tft.print(packetSniffer.dataRate() == SnifferDataRate::RATE_1_MBPS ? "1M" : "2M");

    tft.setCursor(10, 46);
    tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
    tft.print("PACKETS");
    tft.setCursor(76, 46);
    tft.setTextColor(SPECTRUM_ACCENT, SPECTRUM_CARD_BG);
    tft.print(packetSniffer.packetCount());

    tft.drawFastHLine(10, 58, 140, SPECTRUM_GRID);
    tft.setCursor(10, 63);
    tft.setTextColor(ST77XX_GRAY, SPECTRUM_CARD_BG);
    tft.print(packetSniffer.isRunning() ? "LATEST RAW HEX" : "LAST ERROR");

    String text = packetSniffer.isRunning() ? packetSniffer.lastHex()
                                             : packetSniffer.lastError();
    if (!text.length()) text = packetSniffer.isRunning() ? "WAITING FOR PACKET..." : "NO RADIO DATA";
    for (uint8_t line = 0; line < 3; ++line) {
        const int offset = line * 23;
        if (offset >= static_cast<int>(text.length())) break;
        tft.setCursor(10, 75 + line * 9);
        tft.setTextColor(ST77XX_WHITE, SPECTRUM_CARD_BG);
        tft.print(text.substring(offset, min<int>(offset + 23, text.length())));
    }

    drawModernFooter("U/D CH", "A 1M/2M", "B STOP");
}
