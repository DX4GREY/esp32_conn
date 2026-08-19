#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <freertos/FreeRTOS.h>
#include "config/Config.h"
#include "core/AppState.h"

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
    bool hasAnyRadio() const;
    uint8_t availableRadioCount() const;
    uint32_t getBusContentions() const;
    uint32_t getBusTimeouts() const;
    uint32_t getMaxBusWaitUs() const;
    uint32_t getAverageBusWaitUs() const;
    static constexpr bool transmitFeaturesEnabled() {
#if RF_LAB_TX_ENABLED
        return true;
#else
        return false;
#endif
    }
private:
    RF24 radio;
    RF24 radio2;
    volatile bool radio1Available = false;
    volatile bool radio2Available = false;
    bool rxModeActive = false;
    TaskHandle_t jammerTaskHandle = NULL;
    volatile bool stopJam = false;

    bool lockBus(TickType_t timeout = pdMS_TO_TICKS(100));
    void unlockBus();
    void applyTxConfig();
    static void jammerTaskCode(void *param);
};

extern RadioManager radioManager;
