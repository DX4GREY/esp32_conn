#pragma once
#include <Arduino.h>

class SerialCommander {
public:
    void init(unsigned long baud = 115200);
    void process();
    void executeCommand(String cmd);
    void printHelp();
    void printStatus();
    void printConfig();
    void printAsciiSpectrum();
};

extern SerialCommander serialCommander;
