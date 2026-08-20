#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "config/Config.h"
#include "core/AppState.h"

class DisplayManager {
public:
    DisplayManager();
    void init();
    void processInput();
    void updateUI();
    void requestRedraw();
    void showSplash();
    void prepareForShutdown();

private:
    Adafruit_ST7735 tft;
    int menuSelection = 0;
    int menuPage = 0;
    int settingsSelection = 0;
    int statusPage = 0;
    int powerSelection = 0;
    uint8_t envEventScroll = 0;
    uint8_t envBandChannel = 42;
    bool needRedraw = true;
    unsigned long lastStatusFlash = 0;
    bool flashState = false;

    // Partial-redraw support for main menu navigation
    bool menuNeedsPartialRedraw = false;
    int prevMenuSelection = 0;

    // Dynamic-screen caches. Only changed pixels/regions are sent over SPI,
    // which avoids visible clearing and keeps the UI responsive.
    uint8_t previousSpectrumLevels[TOTAL_CHANNELS];
    uint8_t previousPeakLevels[TOTAL_CHANNELS];
    uint8_t previousHeaderPeakLevel = 0xFF;
    int previousHeaderPeakChannel = -1;
    uint8_t previousHeaderRadio1Level = 0xFF;
    uint8_t previousHeaderRadio2Level = 0xFF;
    uint8_t previousInspectedLevel = 0xFF;
    uint8_t previousInspectedPeak = 0xFF;
    bool previousCarrierDetected = false;
    bool carrierStatusValid = false;
    unsigned long lastSpectrumRenderMs = 0;
    unsigned long lastInspectorRenderMs = 0;
    unsigned long lastJammerRenderMs = 0;
    unsigned long lastStatusRenderMs = 0;
    unsigned long lastEnvRenderMs = 0;

    // Tracks page transitions separately from content changes. A full clear is
    // only needed when a different page replaces the current layout.
    int renderedMode = -1;
    bool jammerLayoutDrawn = false;
    bool settingsLayoutDrawn = false;
    int previousJammerTarget = -1;
    int previousJamChannel = -1;
    int previousJamChannel2 = -1;
    int previousPowerLevel = -1;
    int previousDwellTimeUs = -1;
    int previousSettingsSelection = -1;
    bool previousJamming = false;
    bool jammingStatusValid = false;
    int renderedStatusPage = -1;
    String previousStatusValues[6];
    uint16_t previousStatusColors[6] = {0, 0, 0, 0, 0, 0};

    // Screen Renderers
    void renderMainMenu();
    void renderJammerScreen();
    void renderSpectrumAnalyzer();
    void renderWaterfallScreen();
    void renderChannelInspector();
    void renderSurveyScreen();
    void renderEventsScreen();
    void renderLoggingScreen();
    void renderRadioDiagScreen();
    void renderProfilesScreen();
    void renderSettingsScreen();
    void renderStatusScreen();
    void renderPowerScreen();
    void renderRfEnvironmentScreen();
    void renderRebootScreen();
    void renderShutdownScreen();

    // Graphics Helpers
    uint16_t getSignalColor(uint8_t level);
    void drawSpectrumGrid();
    void drawSpectrumBars();
    void drawMenuItem(int index, bool selected);
    void drawMenuIcon(int index, int centerX, int centerY, uint16_t color, uint16_t background);
    void redrawMenuItems(int oldSel, int newSel);
    void resetDynamicCaches();
    void drawModernHeader(const char* title, uint16_t accent);
    void drawModernFooter(const char* left, const char* middle, const char* right);
    void drawFooterChip(int x, int width, const char* label);
};

extern DisplayManager displayManager;
