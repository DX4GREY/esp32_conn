#pragma once

#include <Arduino.h>
#include <FS.h>

class StorageManager {
public:
    bool begin();
    fs::FS& filesystem();
    const char* sessionPath() const;
    const char* scriptsPath() const { return "/RFSuite/scripts"; }
    bool usingSd() const { return sdMounted; }
    const char* backendName() const { return sdMounted ? "SD" : "LittleFS"; }

private:
    bool ensureDirectory(fs::FS& fs, const char* path);
    bool sdMounted = false;
    bool flashMounted = false;
};

extern StorageManager storageManager;
