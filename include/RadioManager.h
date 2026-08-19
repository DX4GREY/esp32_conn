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

    // Jammer Control (Dual-Core FreeRTOS)
    void startJammer(JammerTarget target);
    void stopJammer();

    // Analyzer Control
    void enterRxMode();
    void enterTxMode();
    void scanSpectrum(void (*yieldCb)() = nullptr);
    uint8_t inspectChannel(int channel);

    // Utility
    void stopAll();
    void updatePALevel(rf24_pa_dbm_e pwr);
    bool isConnected();
    bool isRadio1Connected();
    bool isRadio2Connected();
    RF24& getRadio() { return radio; }

private:
    RF24 radio;
    RF24 radio2;
    bool rxModeActive = false;
    TaskHandle_t jammerTaskHandle = NULL;
    volatile bool stopJam = false;

    void applyTxConfig();
    static void jammerTaskCode(void *param);
};

extern RadioManager radioManager;
