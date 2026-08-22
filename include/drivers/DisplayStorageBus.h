#pragma once

#include <SPI.h>

// TFT and microSD share SCK/MOSI on the carrier board. Returning a function-
// local static avoids cross-translation-unit construction-order problems.
inline SPIClass& displayStorageSpi() {
    static SPIClass bus(HSPI);
    return bus;
}
