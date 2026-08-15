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

private:
    Adafruit_ST7735 tft;
    int menuSelection = 0;
    bool needRedraw = true;
    unsigned long lastStatusFlash = 0;
    bool flashState = false;

    // Layar Render
    void renderMainMenu();
    void renderJammerScreen();
    void renderSpectrumAnalyzer();
    void renderChannelInspector();
    void renderStatusScreen();

    // Helper Grafis
    uint16_t getSignalColor(uint8_t level);
    void drawSpectrumGrid();
    void drawSpectrumBars();
};

extern DisplayManager displayManager;
