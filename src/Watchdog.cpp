#include "Watchdog.h"

Watchdog watchdog;
volatile bool Watchdog::triggered = false;

void IRAM_ATTR Watchdog::onTimer() {
    triggered = true;
}

void Watchdog::init(unsigned long timeoutUs) {
    triggered = false;
    timer = timerBegin(0, 80, true); // 1 tick = 1 µs (prescaler 80 for an 80MHz clock)
    timerAttachInterrupt(timer, &Watchdog::onTimer, true);
    timerAlarmWrite(timer, timeoutUs, false); // One-shot alarm
    timerAlarmEnable(timer);
}

void Watchdog::feed() {
    if (timer) {
        timerWrite(timer, 0); // Reset timer counter
    }
}

bool Watchdog::isTriggered() const {
    return triggered;
}
