#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include "Config.h"
#include "AppState.h"

typedef void (*YieldCallback)();

class RadioManager {
public:
    RadioManager();
    bool init();

    // Jammer Kontrol
    void startJammer(JammerTarget target);
    void stopJammer();
    void stepJammer(YieldCallback yieldCb = nullptr);

    // Analyzer Kontrol
    void enterRxMode();
    void enterTxMode();
    void scanSpectrum(YieldCallback yieldCb = nullptr);
    uint8_t inspectChannel(int channel);

    // Utility
    void stopAll();
    bool isConnected();
    RF24& getRadio() { return radio; }

private:
    RF24 radio;
    bool rxModeActive = false;

    void applyTxConfig();
    void generateRandomPayload();
};

extern RadioManager radioManager;
