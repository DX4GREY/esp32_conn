# Firmware Architecture

This document describes the module boundaries used by RF24 Suite and the expected workflow for extending it. The main rule is that hardware access, application state, UI rendering, and user input must remain separate.

## Directory layout

```text
include/
├── config/       Hardware pins and compile-time constants
├── core/         Application types, analyzer math/state, and mode policies
├── drivers/      Buttons and dual nRF24 interfaces
├── services/     Serial CLI, recorder, performance metrics, and watchdog
└── ui/           Display API, shared theme, and menu metadata

src/
├── config/       Channel tables and static assets
├── core/         RF settings, analyzer state, and NVS persistence
├── drivers/      Radio lifecycle, RF task, and analyzer acquisition
├── services/     Serial command and watchdog implementations
├── ui/           Display core, controller, and feature catalog
│   └── screens/  Renderers grouped by feature domain
└── main.cpp      Boot, main-loop scheduling, shutdown, and wake-up
```

## Dependency direction

```text
config ───────────────┐
                     ▼
core types/state ◄── drivers
       ▲              ▲
       │              │
services          UI controller/screens
       ▲              ▲
       └────── main ──┘
```

- `config` must not depend on application or UI modules.
- `core/AppTypes.h` stays lightweight because menus and policies use it.
- Drivers may update application state but must not render the display.
- Screen renderers may read state and driver status, but RF acquisition remains in drivers.
- `main.cpp` schedules modules; feature-specific rendering does not belong there.

## Module responsibilities

### Core

- `AppTypes.h`: shared enums and small data structures.
- `AppState.h`: the public state contract used by the application.
- `AppState.cpp`: RF Test target, power, and dwell configuration.
- `AnalyzerMath.h`: dependency-free averaging, delta, confidence, and event-run primitives used by native tests.
- `AnalyzerState.cpp`: analyzer traces, baseline, confidence, watchlist, configurable events, survey, and peaks.
- `SettingsStore.cpp`: schema-versioned NVS validation, migration, delayed writes, and factory reset.
- `AppModePolicy.cpp`: decides which screens require continuous spectrum sweeps.

### Drivers

- `RadioManager.cpp`: one/two-radio discovery, shared-SPI mutex and metrics, RX/TX lifecycle, and the optional Core 0 RF Test task.
- `RadioAnalyzer.cpp`: spectrum and single-channel acquisition.
- `ButtonManager.cpp`: debounced edge and long-press detection.

### UI

- `DisplayManager.cpp`: TFT initialization, shared primitives, splash, and caches.
- `DisplayTheme.cpp`: selectable RGB565 palettes resolved from the active theme.
- `DisplayController.cpp`: mode dispatch and physical-button input routing.
- `MenuCatalog.cpp`: labels, icons, destination modes, and open actions for every menu card.
- `DisplaySupport.h`: shared theme colors and formatting helpers.
- `screens/MenuScreens.cpp`: main menu and RF Test renderers.
- `screens/AnalyzerScreens.cpp`: spectrum, waterfall, survey, event, logging, diagnostics, profile, and inspector renderers.
- `screens/SystemScreens.cpp`: settings, status, restart, shutdown, and power renderers.

### Services

- `SessionRecorder.cpp`: buffered LittleFS sessions, size limiting, CSV export, and last-sweep replay.
- `PerformanceMonitor.cpp`: smoothed scan/UI duration, maxima, and loop-rate tracking.
- `SerialCommander.cpp`: CLI routing and machine-readable/diagnostic output.
- `Watchdog.cpp`: main-loop liveness monitoring.

## Adding a new display feature

1. Add a new `AppMode` to `include/core/AppTypes.h`.
2. Add any runtime data and focused helper methods to `AppState`.
3. Implement state behavior in the closest core source file. Create a new focused file if the feature does not belong to an existing domain.
4. Declare the renderer in `include/ui/DisplayManager.h`.
5. Implement it in the matching `src/ui/screens` file or create a new screen group.
6. Register its label, mode, icon, and open flags in `src/ui/MenuCatalog.cpp`.
7. Add the render dispatch and button handling to `DisplayController.cpp`.
8. If it needs continuous scanning, register the mode in `AppModePolicy.cpp`.
9. Update the README controls table and run a clean build.

Menu labels and mode routing must not be duplicated inside the renderer. `MenuCatalog` is the single source of truth for menu metadata.

## Adding analyzer data processing

- Put RF sampling in `RadioAnalyzer.cpp`.
- Put aggregation, event generation, or history management in `AnalyzerState.cpp`.
- Keep screen code read-only with respect to raw acquisition whenever possible.
- Document whether the result is instantaneous, averaged, or persistent.
- Avoid presenting carrier-detection ratios as calibrated RSSI/dBm.

## Rendering rules

- A full-screen clear is allowed only when switching to a different page layout.
- Dynamic screens should redraw dirty values, columns, cards, or graph regions only.
- Keep common colors and formatting in `DisplaySupport.h`.
- Add new palettes to `DisplayTheme.cpp`, then extend `DisplayThemeId` and its display name together.
- Cache previously rendered values in `DisplayManager` when updates are frequent.
- Throttle high-frequency rendering independently from RF acquisition.

## Persistence rules

- Only compact user configuration belongs in NVS; writes must be marked dirty and deferred.
- Full sweep sessions belong in LittleFS, while waterfall rows, events, and temporary UI state stay in RAM.
- Add all new NVS keys in `SettingsStore.cpp` and provide a safe default.
- Validate stored enum and numeric values before using them.
- Keep keys short because ESP32 Preferences/NVS key length is limited.
- Increment the schema when persisted structure semantics change and preserve migration from earlier keys.

## Verification

Run a clean build after structural or dependency changes:

```bash
pio test -e native
pio run -e analyzer
pio run -e authorized_rf_lab
```

Before testing on hardware, also verify:

- Both radios are detected independently.
- Spectrum, waterfall, survey, and event screens continue updating.
- Button navigation returns safely to the menu.
- RF Test stops before entering receive-based tools.
- NVS settings survive restart.
- Shutdown returns to deep sleep after a short wake press and boots after a long press.
