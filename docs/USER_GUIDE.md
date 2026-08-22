# User Guide

## Display and navigation model

The TFT operates in 160 × 128 landscape orientation. Full-screen clearing occurs only when changing page layouts. Live screens update dirty graph columns, status fields, or cards to minimize flicker and SPI traffic.

The main menu contains four pages: Analyze, Tools, ENV TEST, and ENV MORE.
Analyze, Tools, and ENV TEST contain six items; ENV MORE contains two. Settings
can render them as a 2 × 3 card grid or a four-row scrolling list.

| Main-menu control | Action |
|---|---|
| `UP` / `DOWN` | Move selection between cards |
| `RIGHT` | Open selected card |
| `B` | Advance to the next menu page |
| `UP` at the first item | Open the previous page at its last item |
| `DOWN` at the last item | Open the next page at its first item |

## Analyze page

### Spectrum

Displays the selected trace across the current band. The header reports peak channel and activity. The information row reports cursor channel/frequency, cursor value, optional watch marker `*`, and confidence `Q`.

| Control | Live acquisition | Frozen acquisition |
|---|---|---|
| Tap `UP` | Next scan band | Cursor +1 channel |
| Tap `DOWN` | Next radio mode | Cursor −1 channel |
| Tap `RIGHT` | Freeze | Resume and restore peak-follow cursor |
| Hold `UP` | Cycle `LIVE → AVG → MAX → DELTA` | Same |
| Hold `DOWN` | Cycle zoom `1× → 2× → 4×` | Same |
| Hold `RIGHT` | Capture baseline and select `DELTA` | Same |
| Hold `B` | Toggle watch marker at cursor | Same |
| Tap `B` | Stop radios and return to menu | Same |

Zoom changes only the displayed viewport around the cursor. Acquisition still scans the selected band.

### Waterfall

Shows up to 24 completed sweeps, newest at the top.

| Control | Action |
|---|---|
| `UP` | Next band |
| `DOWN` | Next radio mode |
| `RIGHT` | Clear waterfall history |
| `B` | Return to menu |

### Inspect

Observes one channel with a deeper sample count. It displays RF channel, derived frequency, live activity, peak activity, and a simple activity/clear indicator.

| Control | Action |
|---|---|
| `UP` | Channel +1 |
| `DOWN` | Channel −1 |
| `RIGHT` | Channel +10 with wraparound |
| `B` | Return to menu |

### Survey

Ranks the five channels with the highest average carrier-hit occupancy within the selected band.

| Control | Action |
|---|---|
| `UP` or `DOWN` | Change band and reset survey accumulation |
| `RIGHT` | Reset survey accumulation |
| `B` | Return to menu |

### Events

Shows recent events and the active configuration as `T`, `H`, `D`, and `M`.

- `T`: trigger threshold percentage.
- `H`: hysteresis percentage.
- `D`: required consecutive sweeps.
- `M`: required simultaneous qualifying channels.

| Control | Action |
|---|---|
| Tap `UP` | Threshold +5; wraps from 90 to 30 |
| Tap `DOWN` | Hysteresis +5; wraps from 30 to 0 |
| Hold `UP` | Duration +1; wraps from 5 to 1 in the UI |
| Hold `DOWN` | Minimum channels +1; wraps from 4 to 1 in the UI |
| `RIGHT` | Clear event history |
| `B` | Return to menu |

The Serial CLI supports the wider validated ranges documented in [Serial CLI](SERIAL_CLI.md).

### Logging

Controls the LittleFS session recorder and USB `RFLOG` summaries.

| Control | Action |
|---|---|
| `RIGHT` | Start a new session or stop the current one |
| `B` | Return to menu without automatically stopping an active session |

Starting a session replaces the previous session file. `STOPPED` is the normal inactive state. See [Data and Storage](DATA_AND_STORAGE.md).

## Tools page

### RX Only / RF Test

The default `analyzer` build shows an RX-only information page; active transmit behavior is not compiled. The `authorized_rf_lab` build shows the controlled RF Test screen.

In the lab build only:

| Control | Action |
|---|---|
| `UP` / `DOWN` | Change the predefined laboratory target group |
| `RIGHT` | Start or stop the selected test |
| `B` | Stop and return to menu |

Use only under the restrictions in [Safety and Authorized Use](SAFETY.md).

### Radio Diag

Reports SPI connectivity separately for R1 and R2. Press `RIGHT` to refresh and `B` to return.

### Profiles

| Profile | Spectrum samples/channel | Inspector samples | Use case |
|---|---:|---:|---|
| FAST | 12 | 50 | Highest refresh rate |
| BALANCED | 30 | 100 | Default compromise |
| DEEP | 60 | 200 | More observations, slower sweep |
| CUSTOM | 10–100 | 2× spectrum, capped at 200 | User-selected depth |

Use `UP/DOWN` to select a profile. When CUSTOM is active, `RIGHT` increments the sample count by 10 and wraps after 100.

### Settings

Use `UP/DOWN` to select and `RIGHT` to advance the value.

| Setting | Values | Notes |
|---|---|---|
| TX Power | MIN, LOW, HIGH, MAX | Relevant to the lab transmit profile; RX initialization uses maximum receiver PA/LNA configuration |
| Sample Dwell | Preset microsecond values | Relevant to the lab transmit profile |
| Display Theme | CYBER, OCEAN, AMBER, MATRIX, VIOLET, ICE | Changing theme forces one clean page rebuild, then partial rendering resumes |
| Menu Layout | GRID, LIST | GRID uses cards; LIST shows four scrolling rows |

### Status

Use `UP/DOWN` to change page, `RIGHT` to refresh, and `B` to return.

1. **Device Info**: chip model, revision/cores, CPU, flash size, flash clock, uptime.
2. **Memory Info**: total/free/minimum heap, largest allocation, sketch use, PSRAM.
3. **Radio / Software**: R1/R2 connection, scan mode, build mode, ESP-IDF, build date.
4. **Performance**: average/maximum sweep time, UI average, loop rate, SPI wait, session state.

`SESSION STOPPED` means the SD/LittleFS recorder is not currently active. It does not mean that the analyzer, filesystem, or device has failed. Start it from Analyze → Logging or with `session start`.

### Power

Choose Restart or Shutdown with `UP/DOWN`, confirm with `RIGHT`, or cancel with `B`.

- Restart calls a firmware reboot after showing the restart screen.
- Shutdown enters deep sleep after stopping radios and preparing the display.
- To wake, hold `RIGHT` for about 1.5 seconds. A short or noisy wake press returns to sleep.

## Runtime versus persistent state

Themes, profiles, event configuration, trace choice, watch markers, RF Test configuration, and CUSTOM depth are persisted. Freeze, zoom, cursor, baseline, histories, and recording state are runtime-only. See [Persistence](PERSISTENCE.md) for the exact schema.
## RF Environment pages

The ENV TEST and ENV MORE controls, metrics, limitations, Serial commands, and
record formats are documented in [RF Environment](RF_ENVIRONMENT.md). Pressing
`B` on an active environment screen requests a stop and returns to the menu.
