#include "ButtonManager.h"

ButtonManager buttonManager;

void ButtonManager::init() {
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_B, INPUT_PULLUP);
}

int ButtonManager::getPinIndex(int pin) {
    if (pin == BTN_UP) return 0;
    if (pin == BTN_RIGHT) return 1;
    if (pin == BTN_DOWN) return 2;
    if (pin == BTN_B) return 3;
    return -1;
}

int ButtonManager::readButton(int pin) {
    int idx = getPinIndex(pin);
    if (idx < 0) return HIGH;

    int raw = digitalRead(pin);
    unsigned long now = millis();

    if (raw != stableState[idx]) {
        // Ada perubahan — mulai atau lanjutkan timer debounce
        if (lastChangeTime[idx] == 0) {
            lastChangeTime[idx] = now;  // Catat awal transisi
        }
        if (now - lastChangeTime[idx] >= BUTTON_DEBOUNCE_MS) {
            stableState[idx] = raw;     // Terima state baru setelah stabil
            lastChangeTime[idx] = 0;    // Reset untuk transisi berikutnya
        }
    } else {
        lastChangeTime[idx] = 0;        // Kembali ke state stabil — reset timer
    }
    return stableState[idx];
}

bool ButtonManager::isPressed(int pin) {
    int idx = getPinIndex(pin);
    if (idx < 0) return false;

    int state = readButton(pin);
    bool pressed = (prevState[idx] == HIGH && state == LOW);
    prevState[idx] = state;
    return pressed;
}
