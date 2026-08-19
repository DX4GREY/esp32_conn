#pragma once
#include <Arduino.h>
#include "config/Config.h"

class Watchdog {
public:
    void init(unsigned long timeoutUs = WATCHDOG_TIMEOUT_US);
    void feed();
    bool isTriggered() const;

private:
    hw_timer_t *timer = nullptr;
    static volatile bool triggered;
    static void IRAM_ATTR onTimer();
};

extern Watchdog watchdog;
