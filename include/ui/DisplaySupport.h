#pragma once

#include <Arduino.h>
#include "core/AppTypes.h"

namespace DisplayUi {

// Shared RGB565 theme. All screen modules use the same palette so visual
// changes remain centralized.
constexpr uint16_t SPECTRUM_HEADER_BG = 0x0862;
constexpr uint16_t SPECTRUM_CARD_BG   = 0x0021;
constexpr uint16_t SPECTRUM_BORDER    = 0x1ACB;
constexpr uint16_t SPECTRUM_GRID      = 0x10E4;
constexpr uint16_t SPECTRUM_ACCENT    = 0x05FF;
constexpr uint16_t SPECTRUM_LOW       = 0x05F4;
constexpr uint16_t SPECTRUM_MID       = 0xBFE0;
constexpr uint16_t SPECTRUM_HIGH      = 0xFD20;
constexpr uint16_t SPECTRUM_CRITICAL  = 0xF94F;

inline const char* compactBandName(AnalyzerBand band) {
    switch (band) {
        case SCAN_BAND_WIFI: return "WIFI";
        case SCAN_BAND_BT:   return "BT";
        case SCAN_BAND_ALL:
        default:             return "ALL";
    }
}

inline String formatBytesShort(uint32_t bytes) {
    if (bytes >= 1024UL * 1024UL) {
        const uint32_t whole = bytes / (1024UL * 1024UL);
        const uint32_t decimal = ((bytes % (1024UL * 1024UL)) * 10UL) /
                                 (1024UL * 1024UL);
        return String(whole) + "." + String(decimal) + " MB";
    }
    if (bytes >= 1024UL) return String(bytes / 1024UL) + " KB";
    return String(bytes) + " B";
}

inline String formatUptime(unsigned long uptimeMs) {
    unsigned long seconds = uptimeMs / 1000UL;
    const unsigned long days = seconds / 86400UL;
    seconds %= 86400UL;
    const unsigned long hours = seconds / 3600UL;
    seconds %= 3600UL;
    const unsigned long minutes = seconds / 60UL;
    seconds %= 60UL;

    char text[20];
    if (days > 0) {
        snprintf(text, sizeof(text), "%lud %02lu:%02lu:%02lu",
                 days, hours, minutes, seconds);
    } else {
        snprintf(text, sizeof(text), "%02lu:%02lu:%02lu",
                 hours, minutes, seconds);
    }
    return String(text);
}

inline int16_t centeredTextX(const String& text, uint8_t textSize = 1,
                             int16_t screenWidth = 160) {
    return (screenWidth - text.length() * 6 * textSize) / 2;
}

}  // namespace DisplayUi
