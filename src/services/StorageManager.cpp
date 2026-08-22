#include "services/StorageManager.h"
#include "config/Config.h"
#include <LittleFS.h>
#include <SD.h>
#include <SPI.h>

StorageManager storageManager;

namespace {
// Keep the SD card off the RF24 controller (the global SPI object uses FSPI).
SPIClass sdSpi(HSPI);
}

bool StorageManager::ensureDirectory(fs::FS& fs, const char* path) {
    if (fs.exists(path)) return true;
    String current;
    const String full(path);
    for (size_t i = 1; i <= full.length(); ++i) {
        if (i == full.length() || full[i] == '/') {
            current = full.substring(0, i);
            if (current.length() && !fs.exists(current) && !fs.mkdir(current)) return false;
        }
    }
    return fs.exists(path);
}

bool StorageManager::begin() {
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    sdSpi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    sdMounted = SD.begin(SD_CS_PIN, sdSpi, 10000000U) && SD.cardType() != CARD_NONE;
    if (sdMounted) {
        if (!ensureDirectory(SD, "/RFSuite/log") ||
            !ensureDirectory(SD, "/RFSuite/scripts")) {
            SD.end();
            sdMounted = false;
        }
    }

    // Keep flash available as a transparent recorder fallback.
    flashMounted = LittleFS.begin(true);
    Serial.printf("Storage: %s%s\n", backendName(),
                  sdMounted ? " mounted at /RFSuite" : " fallback");
    return sdMounted || flashMounted;
}

fs::FS& StorageManager::filesystem() {
    return sdMounted ? static_cast<fs::FS&>(SD) : static_cast<fs::FS&>(LittleFS);
}

const char* StorageManager::sessionPath() const {
    return sdMounted ? "/RFSuite/log/rf_session.csv" : "/rf_session.csv";
}
