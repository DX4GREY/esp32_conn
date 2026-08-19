# Persistence and Settings Schema

## NVS namespace

Configuration uses ESP32 Preferences namespace:

```text
appstate
```

The current schema version is `2`.

## Stored keys

| Key | Type | Default | Validation / meaning |
|---|---|---|---|
| `schema` | unsigned byte | `2` when first saved | Persistence schema version |
| `power` | integer | `RF24_PA_MAX` | Must be within RF24 PA enum range |
| `dwell` | integer | `200` | Clamped to `10–10000` µs |
| `target` | unsigned byte | Wi-Fi | Must be one of six target enums |
| `profile` | unsigned byte | BALANCED | FAST, BALANCED, DEEP, or CUSTOM |
| `custom` | integer | `40` | Clamped to `10–100` samples |
| `theme` | unsigned byte | CYBER | Must be one of six themes |
| `trace` | unsigned byte | LIVE | LIVE, AVG, MAX, or DELTA; DELTA restores as LIVE |
| `evt_thr` | unsigned byte | `60` | Clamped to `5–100` |
| `evt_hys` | unsigned byte | `10` | Clamped to `0–threshold` |
| `evt_dur` | unsigned byte | `2` | Clamped to `1–20` sweeps |
| `evt_multi` | unsigned byte | `1` | Clamped to `1–16` channels |
| `watch` | byte blob | all false | Exactly the size of the 126-entry watch array |

## Deferred writes

Setters call `markSettingsDirty()` instead of writing immediately. The main loop saves the complete configuration after 1.5 seconds without another settings change. This coalesces rapid UI changes and reduces NVS write frequency.

Explicit `saveSettings()` is used by factory reset. Shutdown currently does not force an early dirty-settings flush, so wait at least 1.5 seconds after a setting change before selecting Shutdown if persistence is important.

## Migration

Schema 0/legacy installations load keys that existed before schema versioning, use defaults for new keys, and schedule one deferred schema-2 save. Every loaded enum and numeric value is validated before use.

Future migrations should:

1. Preserve the ability to read older keys.
2. Validate before casting stored integers to enums.
3. Supply deterministic defaults for new settings.
4. Increment `SETTINGS_SCHEMA_VERSION`.
5. Avoid rewriting NVS on every boot after successful migration.

## DELTA trace exception

The trace selection is persistent, but DELTA requires an environmental baseline stored only in RAM. If NVS contains DELTA at boot, the firmware restores LIVE instead. This avoids presenting an all-zero or stale comparison as a valid measurement.

## Factory reset

Run the exact command:

```text
factory reset confirm
```

It performs these actions:

1. Stops radio activity and the recorder.
2. Clears the `appstate` NVS namespace.
3. Restores default power, dwell, theme, profile, custom depth, trace, event settings, watchlist, and target.
4. Saves schema-2 defaults.
5. Reboots.

Factory reset does not erase the LittleFS session file. Starting a new session replaces that file.

## Runtime-only state

The following is intentionally not stored in NVS:

- current app screen and menu selection;
- analyzer band and radio mode;
- freeze state, zoom, cursor, and peak-follow state;
- baseline values and baseline-valid flag;
- live, average, maximum, and decaying peak arrays;
- waterfall, survey totals, and event history;
- recorder active state and runtime sweep count;
- radio mutex performance counters.

This split prevents high-rate analyzer data from wearing NVS and avoids restoring environmental measurements that are no longer valid.
