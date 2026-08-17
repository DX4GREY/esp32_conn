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
};

extern DisplayManager displayManager;
