# Lua Scripting Guide

## Editor setup and generated API library

The repository includes a generator for a LuaLS/EmmyLua-compatible definition
of the global `rf` API. Generate it from the repository root:

```bash
python3 tools/generate_lua_library.py
```

The default output is `tools/generated/rfsuite.lua`. Add that directory as a
Lua language-server library (for example, in VS Code workspace settings):

```json
{
  "Lua.workspace.library": [
    "${workspaceFolder}/tools/generated"
  ],
  "Lua.runtime.version": "Lua 5.1",
  "Lua.diagnostics.globals": ["rf"]
}
```

The generated file provides completion, parameter hints, string-value choices,
return types, and the fields of `rf.status()`. It is an editor-only stub: do not
copy or execute it on the device. Scripts themselves still go in
`/RFSuite/scripts/` on the SD card.

The generator reads the registered function names from
`src/services/LuaEngine.cpp` and rejects missing or obsolete metadata. After
changing the firmware API, update the signatures/descriptions in
`tools/generate_lua_library.py`, regenerate the file, and verify it with:

```bash
python3 tools/generate_lua_library.py --check
```

Use `--output PATH` when another editor or project needs the stub elsewhere.

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
32 KiB. The TFT browser displays up to 32 scripts, sorted by filename.

## Running a script from the TFT

1. Open the `SCRIPTING` menu page.
2. Select `LUA SCRIPTS`.
3. Use `UP` and `DOWN` to select a `.lua` file.
4. Press `A` to run it. Press `B` to return to the menu.

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

This is a synchronous, one-shot runtime: a fresh VM is created for every run,
top-level statements execute once, and all Lua variables disappear afterward.
Long loops delay normal UI and analyzer processing even when they remain under
the instruction limit. Prefer short tasks; for interactive GUI scripts, keep
delays small and leave the loop when the user requests an exit.

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

`rf.spectrum()` is deliberately 1-based like a normal Lua array. Convert an
array index to an RF24 channel with `channel = index - 1`. Activity values and
status fields are snapshots; call the function again to obtain newer data.

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

### Custom TFT GUI

Lua can draw inside a protected 152×86 pixel canvas. The firmware retains its
header, border, and button footer. Canvas coordinates start at `(0, 0)` in the
top-left content area and are clipped so scripts cannot overwrite the firmware
frame.

| Function | Behavior |
|---|---|
| `rf.gui_begin(title)` | Open and clear a firmware-framed Lua GUI |
| `rf.gui_footer(left,middle,right)` | Set three firmware footer labels |
| `rf.gui_clear()` | Clear only the Lua canvas |
| `rf.gui_text(x,y,text,color)` | Draw clipped single-line text |
| `rf.gui_pixel(x,y,color)` | Draw one pixel |
| `rf.gui_line(x0,y0,x1,y1,color)` | Draw a clipped line |
| `rf.gui_rect(x,y,w,h,color,filled)` | Draw an outline or filled rectangle |
| `rf.gui_circle(x,y,r,color,filled)` | Draw an outline or filled circle |
| `rf.button("up"|"down"|"a"|"b")` | Read an active-low button during a script |
| `rf.delay(milliseconds)` | Yield for `0..1000` ms and feed the watchdog |

Colors are `white`, `black`, `gray`, `accent`/`cyan`, `green`, `yellow`,
`orange`, and `red`. Color arguments are optional and default to white. On a
Lua GUI, press `A` to rerun the script and `B` to return to the script list.

Optional parameters are positional. To fill a rectangle with the default white
color, pass the default color explicitly: `rf.gui_rect(0, 0, 20, 10, "white",
true)`. Canvas drawing is not automatically refreshed by a callback; the script
must draw each new frame itself.

### Controlled-lab RF functions

`rf.lab_start(target)` and `rf.lab_stop()` mirror the firmware's controlled-lab
feature. `target` can use the same target names accepted by the firmware CLI.
`rf.lab_start()` is compiled out of the normal `analyzer` profile and returns a
Lua error there. It is available only in `authorized_rf_lab`, preserving the
same build-time safety boundary as the native firmware feature.

## Examples

Ready-to-copy scripts are available in `examples/lua/`:

| Script | Purpose |
|---|---|
| `01_status_report.lua` | Print radio, sweep, peak, and recorder status |
| `02_top_channels.lua` | Rank the ten busiest channels |
| `03_wifi_report.lua` | Summarize and log activity in channels 1–73 |
| `04_threshold_alert.lua` | Print and log channels above a threshold |
| `05_watch_busy.lua` | Mark the five busiest channels for watching |
| `06_delta_setup.lua` | Capture a baseline and open the DELTA trace |
| `07_start_recording.lua` | Start CSV recording and open Logging |
| `08_stop_recording.lua` | Stop CSV recording |
| `09_environment_start.lua` | Start passive environment sampling |
| `10_channel_inspect.lua` | Freeze and inspect the current peak channel |
| `30_custom_gui.lua` | Render a custom RF dashboard inside the firmware frame |
| `40_snake_game.lua` | Interactive Snake using all four hardware buttons |
| `90_api_self_test.lua` | Validate status, spectrum, levels, and range checks |
| `91_control_self_test.lua` | Test cursor/freeze controls and invalid arguments |
| `92_integration_self_test.lua` | Test SD logging and environment start/stop |
| `93_rf_lab_self_test.lua` | Test authorized RF start/stop and target validation |

Copy the desired files to `/RFSuite/scripts/` on the SD card.

### Running the Lua self-tests

Copy the three `9x_*_self_test.lua` files to `/RFSuite/scripts/`, then run them
from the TFT or Serial:

```text
lua run 90_api_self_test
lua run 91_control_self_test
lua run 92_integration_self_test
```

Each test prints individual `PASS`/`FAIL` lines followed by a summary. The API
test is read-only. The control test temporarily changes cursor/freeze state and
restores it. The integration test appends a marker to `lua.log` and briefly
starts/stops passive environment sampling when it was not already running.

The RF lab self-test must only be run in a shielded, explicitly authorized RF
setup with the `authorized_rf_lab` firmware. It starts the Wi-Fi lab target and
stops it immediately, then verifies that an invalid target is rejected. On the
normal `analyzer` build it prints `SKIP`; active RF remains compiled out.

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

## Script design checklist

- Keep the file below 32 KiB and use a safe filename containing letters,
  numbers, `_`, `-`, or `.`.
- Validate channels before calling channel functions; valid RF24 channels are
  `0..125`.
- Remember that displayed activity is a relative carrier-hit percentage, not
  calibrated RSSI or dBm.
- Check boolean results from `rf.recording()`, `rf.environment()`, and
  `rf.lab_start()` instead of assuming the operation started.
- Wrap expected argument failures with `pcall`; an unhandled Lua error stops the
  whole script and is reported on Serial/TFT.
- Use `rf.delay()` rather than a busy-wait loop. Each call is limited to 1000 ms.
- Stop controlled-lab activity with `rf.lab_stop()` before leaving a lab script,
  including its error path. Active RF remains unavailable in normal builds.
