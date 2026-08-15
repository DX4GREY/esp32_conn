#pragma once
#include <Arduino.h>
#include "Config.h"

class ButtonManager {
public:
    void init();
    int readButton(int pin);
    bool isPressed(int pin);

private:
    unsigned long lastChangeTime[4] = {0, 0, 0, 0};
    int stableState[4] = {HIGH, HIGH, HIGH, HIGH};
    int prevState[4] = {HIGH, HIGH, HIGH, HIGH};

    int getPinIndex(int pin);
};

extern ButtonManager buttonManager;
