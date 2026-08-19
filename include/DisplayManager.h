#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "Config.h"
#include "AppState.h"

class DisplayManager {
public:
    DisplayManager();
    void init();
    void processInput();
    void updateUI();
    void requestRedraw();
    void showSplash();

private:
    Adafruit_ST7735 tft;
    int menuSelection = 0;
    int settingsSelection = 0;
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
    uint8_t previousInspectedLevel = 0xFF;
    uint8_t previousInspectedPeak = 0xFF;
    bool previousCarrierDetected = false;
    bool carrierStatusValid = false;
    unsigned long lastSpectrumRenderMs = 0;
    unsigned long lastInspectorRenderMs = 0;
    unsigned long lastJammerRenderMs = 0;

    // Tracks page transitions separately from content changes. A full clear is
    // only needed when a different page replaces the current layout.
    int renderedMode = -1;
    bool jammerLayoutDrawn = false;
    bool settingsLayoutDrawn = false;
    int previousJammerTarget = -1;
    int previousJamChannel = -1;
    int previousPowerLevel = -1;
    int previousDwellTimeUs = -1;
    int previousSettingsSelection = -1;
    bool previousJamming = false;
    bool jammingStatusValid = false;

    // Screen Renderers
    void renderMainMenu();
    void renderJammerScreen();
    void renderSpectrumAnalyzer();
    void renderChannelInspector();
    void renderSettingsScreen();
    void renderStatusScreen();
    void renderRebootScreen();

    // Graphics Helpers
    uint16_t getSignalColor(uint8_t level);
    void drawSpectrumGrid();
    void drawSpectrumBars();
    void drawMenuItem(int index, bool selected);
    void redrawMenuItems(int oldSel, int newSel);
    void resetDynamicCaches();
};

extern DisplayManager displayManager;
