# Serial CLI Reference

## Connection

The command interface uses USB Serial at 115200 baud. Commands are line-oriented, trimmed, and case-insensitive.

```bash
pio device monitor --baud 115200
```

## General commands

| Command | Result |
|---|---|
| `help` | Print the built-in command summary |
| `status` | Print radio count, test state, target, power, dwell, peak, and confidence |
| `config` or `settings` | Print stored configuration and compile-time build mode |
| `stop` | Stop all radio activity |
| `perf` | Print sweep/UI/loop timing and SPI mutex metrics |
| `factory reset confirm` | Clear NVS settings, restore defaults, and reboot |

Factory reset does not delete `/rf_session.csv` from LittleFS.

## Analyzer commands

| Command | Valid values | Behavior |
|---|---|---|
| `scan` or `spectrum` | — | Stop RF Test, execute one sweep, and print an ASCII spectrum |
| `inspect <ch>` | `0–125` | Stop RF Test and inspect one RF channel |
| `trace <mode>` | `live`, `avg`, `average`, `max`, `delta` | Select displayed trace |
| `freeze` or `hold` | — | Pause continuous acquisition |
| `resume` | — | Resume acquisition |
| `zoom <value>` | `1`, `2`, `4` | Set display zoom |
| `cursor <ch>` | Any integer, clamped to `0–125` | Move cursor and disable peak-follow |
| `watch <ch>` | Any integer, clamped to `0–125` | Toggle persistent watch marker |
| `baseline` | — | Capture baseline and select DELTA |
| `max clear` | — | Clear MAX and decaying peak state |

Select `baseline` before `trace delta`; DELTA has no useful values without an in-RAM baseline.

## Event commands

| Command | Validated range | Meaning |
|---|---:|---|
| `event threshold <n>` | `5–100` | Activity percentage required to increment a run |
| `event hysteresis <n>` | `0–threshold` | Release margin below threshold |
| `event duration <n>` | `1–20` | Consecutive qualifying sweeps |
| `event channels <n>` | `1–16` | Simultaneous qualifying channels required |

Changing any event parameter clears per-channel run counters and releases the event latch. Existing event-history entries remain until cleared on the Events screen.

## Session commands

| Command | Behavior |
|---|---|
| `session` or `session info` | Print recorder state, sweep count, file size, and last error |
| `session start` | Replace the previous file and start recording |
| `session stop` | Flush pending data and stop |
| `session export` | Flush and stream the complete CSV between marker lines |
| `session replay` | Stop recording/radios and freeze the last stored sweep on Spectrum |

Unknown `session` actions currently fall back to the information output.

## RF-lab configuration commands

These settings exist in both builds, but active transmission is compile-time disabled in `analyzer`.

| Command | Values | Behavior |
|---|---|---|
| `power` or `pwr` | No argument | Print current configured transmit level |
| `power <level>` | `min`, `low`, `high`, `max`; dBm aliases are also accepted | Store and apply configured transmit level |
| `dwell` | No argument | Print current dwell |
| `dwell <us>` | `10–10000` | Store dwell microseconds |
| `start` | — | Start selected RF Test in lab build; unavailable message in analyzer build |
| `jam <target>` | `wifi`, `bt`, `ble`, `bledata`, `all`, `zigbee` | Select target and start in lab build |

Use transmit commands only within the scope defined by [Safety and Authorized Use](SAFETY.md).

## Output examples

### Performance

```text
PERF sweep last/avg/max=... us, UI avg/max=... us, loop=... Hz
SPI lock contention=... timeout=... avg/max wait=... us
```

Contention means a caller found the mutex busy and waited. A timeout means it could not acquire the bus within its allowed wait and deserves investigation.

### Session information

```text
Session: STOPPED, 0 sweeps, 12345 bytes, error=none
```

The sweep counter is for the current runtime recording. A nonzero file size can exist while the current recorder state is stopped.

## Automation notes

- Human-readable output includes spaces and, in some lines, Unicode symbols.
- Use `RFLOG` and session `S` rows for machine processing.
- Command responses are not a versioned RPC protocol; parsers should tolerate additional informational lines.
- Do not send status commands at extremely high rates. Radio status checks share the protected SPI bus.
