#include "services/LuaEngine.h"
#include "services/StorageManager.h"
#include <dirent.h>
#include "core/AppState.h"
#include "drivers/RadioManager.h"
#include "services/SessionRecorder.h"
#include "services/RfEnvironmentAnalyzer.h"
#include "core/RfEnvironmentState.h"
#include <FS.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

LuaEngine luaEngine;

namespace {
constexpr size_t MAX_SCRIPT_BYTES = 32U * 1024U;
constexpr int MAX_VM_INSTRUCTIONS = 200000;
Stream* activeOutput = nullptr;
int instructionBudget = 0;

int luaPrint(lua_State* state) {
    if (!activeOutput) return 0;
    const int count = lua_gettop(state);
    for (int i = 1; i <= count; ++i) {
        if (i > 1) activeOutput->print('\t');
        const char* value = lua_tostring(state, i);
        activeOutput->print(value ? value : lua_typename(state, lua_type(state, i)));
    }
    activeOutput->println();
    return 0;
}

int rfMillis(lua_State* state) { lua_pushnumber(state, millis()); return 1; }
int rfPeak(lua_State* state) { lua_pushinteger(state, appState.peakChannel); return 1; }
int rfLevel(lua_State* state) {
    const int channel = luaL_checkinteger(state, 1);
    luaL_argcheck(state, channel >= MIN_CHANNEL && channel <= MAX_CHANNEL, 1, "channel must be 0..125");
    lua_pushinteger(state, appState.spectrumLevels[channel]);
    return 1;
}

int rfSpectrum(lua_State* state) {
    lua_createtable(state, TOTAL_CHANNELS, 0);
    for (int channel = 0; channel < TOTAL_CHANNELS; ++channel) {
        lua_pushinteger(state, appState.spectrumLevels[channel]);
        lua_rawseti(state, -2, channel + 1);
    }
    return 1;
}

int rfStatus(lua_State* state) {
    lua_newtable(state);
#define RF_FIELD_INT(name, value) lua_pushinteger(state, value); lua_setfield(state, -2, name)
#define RF_FIELD_BOOL(name, value) lua_pushboolean(state, value); lua_setfield(state, -2, name)
    RF_FIELD_INT("peak_channel", appState.peakChannel);
    RF_FIELD_INT("peak_level", appState.peakLevel);
    RF_FIELD_INT("confidence", appState.analyzerConfidence);
    RF_FIELD_INT("cursor", appState.cursorChannel);
    RF_FIELD_INT("sweeps", appState.surveySweeps);
    RF_FIELD_INT("radios", radioManager.availableRadioCount());
    RF_FIELD_BOOL("frozen", appState.analyzerFrozen);
    RF_FIELD_BOOL("logging", sessionRecorder.isRecording());
    RF_FIELD_BOOL("environment_running", rfEnvironmentState.running);
#undef RF_FIELD_INT
#undef RF_FIELD_BOOL
    return 1;
}

int rfSetCursor(lua_State* state) {
    const int channel = luaL_checkinteger(state, 1);
    luaL_argcheck(state, channel >= MIN_CHANNEL && channel <= MAX_CHANNEL,
                  1, "channel must be 0..125");
    appState.setCursorChannel(channel, false);
    return 0;
}

int rfFreeze(lua_State* state) {
    appState.analyzerFrozen = lua_toboolean(state, 1);
    if (appState.analyzerFrozen) radioManager.requestScanAbort();
    return 0;
}

int rfSetBand(lua_State* state) {
    const char* band = luaL_checkstring(state, 1);
    if (!strcmp(band, "all")) appState.analyzerBand = SCAN_BAND_ALL;
    else if (!strcmp(band, "wifi")) appState.analyzerBand = SCAN_BAND_WIFI;
    else if (!strcmp(band, "bt")) appState.analyzerBand = SCAN_BAND_BT;
    else return luaL_error(state, "band must be all, wifi, or bt");
    radioManager.requestScanAbort(); appState.markSettingsDirty(); return 0;
}

int rfSetTrace(lua_State* state) {
    const char* trace = luaL_checkstring(state, 1);
    if (!strcmp(trace, "live")) appState.analyzerTraceMode = ANALYZER_TRACE_LIVE;
    else if (!strcmp(trace, "avg")) appState.analyzerTraceMode = ANALYZER_TRACE_AVERAGE;
    else if (!strcmp(trace, "max")) appState.analyzerTraceMode = ANALYZER_TRACE_MAX;
    else if (!strcmp(trace, "delta")) appState.analyzerTraceMode = ANALYZER_TRACE_DELTA;
    else return luaL_error(state, "trace must be live, avg, max, or delta");
    appState.markSettingsDirty(); return 0;
}

int rfBaseline(lua_State*) { appState.captureBaseline(); return 0; }
int rfClearMax(lua_State*) { appState.clearAnalyzerMax(); return 0; }
int rfWatch(lua_State* state) {
    const int channel = luaL_checkinteger(state, 1);
    luaL_argcheck(state, channel >= MIN_CHANNEL && channel <= MAX_CHANNEL, 1, "channel must be 0..125");
    appState.toggleWatchChannel(channel); return 0;
}

int rfSession(lua_State* state) {
    const bool start = lua_toboolean(state, 1);
    if (start) appState.loggingEnabled = sessionRecorder.start();
    else { appState.loggingEnabled = false; sessionRecorder.stop(); }
    lua_pushboolean(state, start ? appState.loggingEnabled : true); return 1;
}

int rfEnvironment(lua_State* state) {
    const bool start = lua_toboolean(state, 1);
    bool result = true;
    if (start) result = rfEnvironmentAnalyzer.start(RF_ENV_OCCUPANCY);
    else rfEnvironmentAnalyzer.stop();
    lua_pushboolean(state, result); return 1;
}

int rfOpen(lua_State* state) {
    const char* screen = luaL_checkstring(state, 1);
    if (!strcmp(screen, "spectrum")) appState.appMode = APP_MODE_ANALYZER_SPECTRUM;
    else if (!strcmp(screen, "waterfall")) appState.appMode = APP_MODE_WATERFALL;
    else if (!strcmp(screen, "inspect")) appState.appMode = APP_MODE_ANALYZER_CHANNEL;
    else if (!strcmp(screen, "survey")) appState.appMode = APP_MODE_SURVEY;
    else if (!strcmp(screen, "events")) appState.appMode = APP_MODE_EVENTS;
    else if (!strcmp(screen, "logging")) appState.appMode = APP_MODE_LOGGING;
    else if (!strcmp(screen, "status")) appState.appMode = APP_MODE_STATUS;
    else if (!strcmp(screen, "menu")) appState.appMode = APP_MODE_MENU;
    else return luaL_error(state, "unknown screen");
    return 0;
}

int rfLabStart(lua_State* state) {
#if RF_LAB_TX_ENABLED
    const char* target = luaL_checkstring(state, 1);
    if (!appState.setJammerTargetByName(String(target))) return luaL_error(state, "invalid lab target");
    radioManager.startJammer(appState.jammerTarget); lua_pushboolean(state, appState.jamming); return 1;
#else
    return luaL_error(state, "active RF is disabled in analyzer build");
#endif
}
int rfLabStop(lua_State*) { radioManager.stopAll(); return 0; }

int rfLog(lua_State* state) {
    const char* message = luaL_checkstring(state, 1);
    fs::FS& fs = storageManager.filesystem();
    const char* path = storageManager.usingSd() ? "/RFSuite/log/lua.log" : "/lua.log";
    File file = fs.open(path, FILE_APPEND);
    if (!file) return luaL_error(state, "cannot open Lua log");
    file.printf("%lu,%s\n", static_cast<unsigned long>(millis()), message);
    file.close();
    return 0;
}

void vmHook(lua_State* state, lua_Debug*) {
    instructionBudget -= 1000;
    if (instructionBudget <= 0) luaL_error(state, "instruction limit exceeded");
}

bool safeName(const String& name) {
    if (!name.length() || name.length() > 48 || name.indexOf("..") >= 0 || name.indexOf('/') >= 0) return false;
    for (size_t i = 0; i < name.length(); ++i) {
        const char c = name[i];
        if (!isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-' && c != '.') return false;
    }
    return true;
}
}

bool LuaEngine::begin() {
    ready = storageManager.usingSd();
    error = ready ? "none" : "Lua scripts require an SD card";
    return ready;
}

bool LuaEngine::run(const String& requestedName, Stream& output) {
    if (!ready) { error = "SD card unavailable"; return false; }
    String name = requestedName;
    if (!name.endsWith(".lua")) name += ".lua";
    if (!safeName(name)) { error = "invalid script name"; return false; }
    const String path = String(storageManager.scriptsPath()) + "/" + name;
    File file = storageManager.filesystem().open(path, FILE_READ);
    if (!file) { error = "script not found"; return false; }
    if (file.size() > MAX_SCRIPT_BYTES) { file.close(); error = "script exceeds 32 KiB"; return false; }
    String source;
    source.reserve(file.size() + 1);
    while (file.available()) source += static_cast<char>(file.read());
    file.close();

    lua_State* state = luaL_newstate();
    if (!state) { error = "cannot allocate Lua VM"; return false; }
    luaopen_base(state); luaopen_table(state); luaopen_string(state); luaopen_math(state);
    lua_settop(state, 0);
    const char* blocked[] = {"dofile", "loadfile", "loadstring", "require", "collectgarbage", nullptr};
    for (const char** item = blocked; *item; ++item) { lua_pushnil(state); lua_setglobal(state, *item); }
    lua_register(state, "print", luaPrint);
    lua_newtable(state);
    lua_pushcfunction(state, rfMillis); lua_setfield(state, -2, "millis");
    lua_pushcfunction(state, rfPeak); lua_setfield(state, -2, "peak_channel");
    lua_pushcfunction(state, rfLevel); lua_setfield(state, -2, "level");
    lua_pushcfunction(state, rfLog); lua_setfield(state, -2, "log");
    lua_pushcfunction(state, rfSpectrum); lua_setfield(state, -2, "spectrum");
    lua_pushcfunction(state, rfStatus); lua_setfield(state, -2, "status");
    lua_pushcfunction(state, rfSetCursor); lua_setfield(state, -2, "set_cursor");
    lua_pushcfunction(state, rfFreeze); lua_setfield(state, -2, "freeze");
    lua_pushcfunction(state, rfSetBand); lua_setfield(state, -2, "set_band");
    lua_pushcfunction(state, rfSetTrace); lua_setfield(state, -2, "set_trace");
    lua_pushcfunction(state, rfBaseline); lua_setfield(state, -2, "capture_baseline");
    lua_pushcfunction(state, rfClearMax); lua_setfield(state, -2, "clear_max");
    lua_pushcfunction(state, rfWatch); lua_setfield(state, -2, "toggle_watch");
    lua_pushcfunction(state, rfSession); lua_setfield(state, -2, "recording");
    lua_pushcfunction(state, rfEnvironment); lua_setfield(state, -2, "environment");
    lua_pushcfunction(state, rfOpen); lua_setfield(state, -2, "open_screen");
    lua_pushcfunction(state, rfLabStart); lua_setfield(state, -2, "lab_start");
    lua_pushcfunction(state, rfLabStop); lua_setfield(state, -2, "lab_stop");
    lua_setglobal(state, "rf");
    instructionBudget = MAX_VM_INSTRUCTIONS;
    lua_sethook(state, vmHook, LUA_MASKCOUNT, 1000);
    activeOutput = &output;
    int result = luaL_loadbuffer(state, source.c_str(), source.length(), name.c_str());
    if (result == 0) result = lua_pcall(state, 0, 0, 0);
    if (result != 0) error = lua_tostring(state, -1) ? lua_tostring(state, -1) : "Lua error";
    else error = "none";
    activeOutput = nullptr;
    lua_close(state);
    return result == 0;
}

void LuaEngine::list(Stream& output) const {
    if (!ready) { output.println("Lua scripts require an SD card."); return; }
    DIR* dir = opendir("/sd/RFSuite/scripts");
    if (!dir) { output.println("No scripts directory."); return; }
    bool found = false;
    for (dirent* entry = readdir(dir); entry; entry = readdir(dir)) {
        const String name(entry->d_name);
        if (name.endsWith(".lua")) {
            output.println(name);
            found = true;
        }
    }
    if (!found) output.println("No .lua scripts found.");
    closedir(dir);
}

size_t LuaEngine::listScripts(String* names, size_t capacity) const {
    if (!ready) return 0;
    DIR* dir = opendir("/sd/RFSuite/scripts");
    if (!dir) return 0;
    size_t count = 0;
    for (dirent* entry = readdir(dir); entry && count < capacity; entry = readdir(dir)) {
        const String name(entry->d_name);
        if (name.endsWith(".lua")) names[count++] = name;
    }
    closedir(dir);
    for (size_t i = 0; i < count; ++i)
        for (size_t j = i + 1; j < count; ++j)
            if (names[j] < names[i]) { String swap = names[i]; names[i] = names[j]; names[j] = swap; }
    return count;
}
