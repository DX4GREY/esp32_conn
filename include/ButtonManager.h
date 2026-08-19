#pragma once
#include <Arduino.h>
#include "Config.h"

class ButtonManager {
public:
    void init();
    int readButton(int pin);
    bool isPressed(int pin);
    bool isLongPressed(int pin, unsigned long holdMs = 700);

private:
    unsigned long lastChangeTime[4] = {0, 0, 0, 0};
    int stableState[4] = {HIGH, HIGH, HIGH, HIGH};
    int prevState[4] = {HIGH, HIGH, HIGH, HIGH};
    unsigned long holdStartTime[4] = {0, 0, 0, 0};
    bool holdReported[4] = {false, false, false, false};

    int getPinIndex(int pin);
};

extern ButtonManager buttonManager;
