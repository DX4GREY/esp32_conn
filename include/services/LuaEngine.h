#pragma once

#include <Arduino.h>

class LuaEngine {
public:
    bool begin();
    bool run(const String& scriptName, Stream& output);
    void list(Stream& output) const;
    size_t listScripts(String* names, size_t capacity) const;
    bool isReady() const { return ready; }
    const char* lastError() const { return error.c_str(); }

private:
    bool ready = false;
    String error = "not initialized";
};

extern LuaEngine luaEngine;
