# Development Guide

## Local workflow

Before editing:

```bash
git status --short
```

After changing dependency-free analyzer logic:

```bash
pio test -e native
```

After firmware, UI, hardware, or configuration changes:

```bash
pio run -e analyzer
pio run -e authorized_rf_lab
git diff --check
```

## Source ownership

- Compile-time pins and constants: `include/config`, `src/config`.
- Shared enums and state contract: `include/core`.
- State algorithms and persistence: `src/core`.
- Hardware and acquisition: `include/drivers`, `src/drivers`.
- Long-running or cross-cutting utilities: `include/services`, `src/services`.
- Input routing and rendering: `include/ui`, `src/ui`.
- Boot and scheduling only: `src/main.cpp`.

See [Architecture](ARCHITECTURE.md) for the full dependency model.

## Adding a menu feature

1. Add an `AppMode` in `include/core/AppTypes.h`.
2. Add focused state and behavior without placing hardware calls in UI code.
3. Declare the renderer in `include/ui/DisplayManager.h`.
4. Implement the renderer in the correct `src/ui/screens` module.
5. Add menu metadata once in `src/ui/MenuCatalog.cpp`.
6. Add dispatch and button behavior in `src/ui/DisplayController.cpp`.
7. Register continuous acquisition in `AppModePolicy` if required.
8. Update [User Guide](USER_GUIDE.md), tests, and release checks.

The two menu pages currently have exactly six slots each. Adding a thirteenth feature requires updating catalog counts, page layout assumptions, and navigation.

## UI rendering rules

- Full-screen clear is for a page transition only.
- Redraw the smallest dirty rectangle, graph column, card, or text field.
- Cache old dynamic values in `DisplayManager`.
- Reset caches on layout or theme changes.
- Keep acquisition and display throttling independent.
- Verify all labels fit a 160 × 128 screen with the built-in 6-pixel text width assumptions.
- Theme-dependent colors belong in `DisplayTheme.cpp` and `DisplaySupport.h`.

## Radio concurrency rules

Both nRF modules share one SPI bus and RF24 objects can be accessed from different cores. All runtime RF24 SPI operations must be protected by the radio bus mutex.

- Use `RadioManager::lockBus()` and `unlockBus()` inside driver members.
- Do not expose mutable RF24 references outside the driver.
- Keep lock scope limited to SPI transactions; avoid delays while holding it when possible.
- Do not call another locking method while already holding a non-recursive mutex.
- Add diagnostic counters for new timeout paths when relevant.
- Display SPI is separate and does not use the radio mutex.

## Single-radio behavior

Every new acquisition path must handle R1-only, R2-only, and no-radio states. A selected but unavailable radio should fall back to the detected receiver where that produces a meaningful result. Never restore a boot loop solely because one module is missing.

## Analyzer algorithm rules

- Keep pure math in `AnalyzerMath.h` so native tests can cover it.
- Store RF acquisition in `RadioAnalyzer.cpp`.
- Store aggregation and events in `AnalyzerState.cpp`.
- Clamp values crossing persistence, parsing, or display boundaries.
- Describe activity as a carrier-hit ratio, never as dBm.
- Document whether new data is live, averaged, cumulative, persistent, or replayed.

## Persistence rules

- Add compact user preferences to `SettingsStore.cpp`.
- Use short NVS key names.
- Validate all loaded data.
- Mark settings dirty and let the 1.5-second service coalesce writes.
- Increment schema version when semantics or required keys change.
- Put high-rate sweep data in LittleFS, not NVS.
- Document file-format changes and increment the session format version.

## Serial compatibility

- Preserve existing command spellings unless a migration path is documented.
- Keep machine-readable lines prefixed (`RFLOG`, `S`).
- Validate and clamp user input.
- Print actionable failure reasons.
- Treat output as user-visible API and update [Serial CLI](SERIAL_CLI.md).

## Build-profile gating

Active RF test code is controlled by `RF_LAB_TX_ENABLED`:

- undefined defaults to `0` in `Config.h`;
- `analyzer` explicitly sets `0`;
- `authorized_rf_lab` explicitly sets `1`.

New transmit functionality must remain inside the same compile-time boundary and must not silently appear in the default environment. Both profiles must compile in CI.

## Documentation checklist

Update documentation whenever a change affects:

- wiring or supported hardware;
- build flags, dependencies, partitions, or upload steps;
- menu controls or display fields;
- analyzer interpretation or calculations;
- Serial commands or output formats;
- NVS keys or LittleFS formats;
- safety boundaries;
- test and release procedures.
