#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include "Config.h"
#include "AppState.h"

class RadioManager {
public:
    RadioManager();
    bool init();

    // Jammer Kontrol (Dual-Core FreeRTOS)
    void startJammer(JammerTarget target);
    void stopJammer();

    // Analyzer Kontrol
    void enterRxMode();
    void enterTxMode();
    void scanSpectrum(void (*yieldCb)() = nullptr);
    uint8_t inspectChannel(int channel);

    // Utility
    void stopAll();
    bool isConnected();
    RF24& getRadio() { return radio; }

private:
    RF24 radio;
    bool rxModeActive = false;
    TaskHandle_t jammerTaskHandle = NULL;
    volatile bool stopJam = false;

    void applyTxConfig();
    static void jammerTaskCode(void *param);
};

extern RadioManager radioManager;
