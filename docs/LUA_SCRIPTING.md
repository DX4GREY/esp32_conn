# Lua Scripting Guide

## Preparing the SD card

Format the card as FAT16 or FAT32. At boot the firmware creates these folders:

```text
/RFSuite/
├── log/
│   ├── rf_session.csv
│   └── lua.log
└── scripts/
    └── example.lua
```

Copy plain-text Lua 5.1 files into `/RFSuite/scripts/`. A script is limited to
32 KiB. The TFT browser displays up to 16 scripts, sorted by filename.

## Running a script from the TFT

1. Open the `SCRIPTING` menu page.
2. Select `LUA SCRIPTS`.
3. Use `UP` and `DOWN` to select a `.lua` file.
4. Press `RIGHT` to run it. Press `B` to return to the menu.

Removing or inserting a card while powered is not supported. Reboot after
changing the card. Script `print()` output is sent to USB Serial at 115200 baud;
completion or error status also appears on the TFT.

The same scripts can be managed through Serial:

```text
lua list
lua run channel_report
```

## Built-in Lua libraries

The VM provides Lua's base, `string`, `table`, and `math` libraries. For safety,
filesystem, operating-system, dynamic package loading, `dofile`, `loadfile`,
`loadstring`, `require`, and direct garbage-collector control are unavailable.
Scripts have a 200,000-instruction limit so an accidental endless loop returns
an error instead of permanently blocking the UI.

## RFSuite `rf` library

### Reading analyzer data

| Function | Result |
|---|---|
| `rf.millis()` | Milliseconds since boot |
| `rf.peak_channel()` | Current peak RF channel, `0..125` |
| `rf.level(channel)` | Latest relative activity percentage |
| `rf.spectrum()` | Table containing all 126 activity values; Lua index 1 is RF channel 0 |
| `rf.status()` | Table of current firmware status fields |

`rf.status()` contains `peak_channel`, `peak_level`, `confidence`, `cursor`,
`sweeps`, `radios`, `frozen`, `logging`, and `environment_running`.

### Analyzer control

| Function | Behavior |
|---|---|
| `rf.set_cursor(channel)` | Move cursor to channel `0..125` |
| `rf.freeze(boolean)` | Freeze or resume acquisition |
| `rf.set_band("all"|"wifi"|"bt")` | Select analyzer band |
| `rf.set_trace("live"|"avg"|"max"|"delta")` | Select trace |
| `rf.capture_baseline()` | Capture current analyzer baseline |
| `rf.clear_max()` | Clear maximum trace history |
| `rf.toggle_watch(channel)` | Toggle a watched-channel marker |

### Storage and environment test

| Function | Behavior |
|---|---|
| `rf.log(message)` | Append `milliseconds,message` to `/RFSuite/log/lua.log` |
| `rf.recording(true|false)` | Start a new session or stop recording; returns success |
| `rf.environment(true|false)` | Start or stop passive occupancy analysis; returns success |

### TFT navigation

`rf.open_screen(name)` changes to one of `spectrum`, `waterfall`, `inspect`,
`survey`, `events`, `logging`, `status`, or `menu` after the script finishes.

### Controlled-lab RF functions

`rf.lab_start(target)` and `rf.lab_stop()` mirror the firmware's controlled-lab
feature. `target` can use the same target names accepted by the firmware CLI.
`rf.lab_start()` is compiled out of the normal `analyzer` profile and returns a
Lua error there. It is available only in `authorized_rf_lab`, preserving the
same build-time safety boundary as the native firmware feature.

## Examples

Read a full spectrum and log channels above 70%:

```lua
local levels = rf.spectrum()
for index, level in ipairs(levels) do
    local channel = index - 1
    if level >= 70 then
        rf.log(string.format("high activity ch=%d level=%d", channel, level))
    end
end
```

Configure the analyzer and open its TFT screen:

```lua
rf.set_band("wifi")
rf.set_trace("avg")
rf.freeze(false)
rf.recording(true)
rf.open_screen("spectrum")
```

Check status before starting a passive environment test:

```lua
local status = rf.status()
print("radios", status.radios, "peak", status.peak_channel)
if status.radios > 0 then
    assert(rf.environment(true), "environment analyzer could not start")
end
```

## Error handling

Use `assert`, `pcall`, and explicit validation like normal Lua:

```lua
local ok, message = pcall(function()
    rf.set_band("invalid")
end)
if not ok then
    rf.log("script error: " .. tostring(message))
end
```

Keep scripts short and event-oriented. A script runs synchronously, so regular
button processing and scanning resume after it exits.
