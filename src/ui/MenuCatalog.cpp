#include "ui/MenuCatalog.h"

namespace {

constexpr MenuFeature FEATURES[MenuCatalog::FEATURE_COUNT] = {
    {"SPECTRUM",  APP_MODE_ANALYZER_SPECTRUM, 0,
     MENU_OPEN_STOP_RADIOS | MENU_OPEN_RESET_PEAKS},
    {"WATERFALL", APP_MODE_WATERFALL,          1, MENU_OPEN_STOP_RADIOS},
    {"INSPECT",   APP_MODE_ANALYZER_CHANNEL,   2,
     MENU_OPEN_STOP_RADIOS | MENU_OPEN_RESET_INSPECTOR},
    {"SURVEY",    APP_MODE_SURVEY,             3, MENU_OPEN_STOP_RADIOS},
    {"EVENTS",    APP_MODE_EVENTS,             4, MENU_OPEN_STOP_RADIOS},
    {"LOGGING",   APP_MODE_LOGGING,            5, MENU_OPEN_STOP_RADIOS},
#if RF_LAB_TX_ENABLED
    {"RF TEST",   APP_MODE_JAMMER,             6, MENU_OPEN_NONE},
#else
    {"RX ONLY",   APP_MODE_JAMMER,             6, MENU_OPEN_NONE},
#endif
    {"RADIO DIAG",APP_MODE_RADIO_DIAG,         7, MENU_OPEN_NONE},
    {"PROFILES",  APP_MODE_PROFILES,           8, MENU_OPEN_NONE},
    {"SETTINGS",  APP_MODE_SETTINGS,           9, MENU_OPEN_NONE},
    {"STATUS",    APP_MODE_STATUS,             10, MENU_OPEN_NONE},
    {"POWER",     APP_MODE_POWER,              11, MENU_OPEN_STOP_RADIOS}
};

constexpr const char* PAGE_TITLES[MenuCatalog::PAGE_COUNT] = {
    "ANALYZE",
    "TOOLS"
};

}  // namespace

namespace MenuCatalog {

int featureIndex(int page, int slot) {
    return page * ITEMS_PER_PAGE + slot;
}

const MenuFeature& featureAt(int page, int slot) {
    return FEATURES[featureIndex(page, slot)];
}

const char* pageTitle(int page) {
    return PAGE_TITLES[page];
}

}  // namespace MenuCatalog
