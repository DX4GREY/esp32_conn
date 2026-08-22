# RF Environment Reference

## Scope and measurement meaning

The RF Environment pages add longer-running, passive views of the 2.4 GHz band.
They use the nRF24L01+ RPD/carrier detector: occupancy is the percentage of
observations that reported activity, not RSSI, dBm, transmitter identity, or a
decoded protocol. Results are best used for relative comparisons made with the
same hardware, placement, range, and sample window.

The displayed interference score is a `0–100` heuristic made from occupancy
(40%), persistence (20%), burst activity (20%), and neighboring-channel
activity (20%). Its labels are CLEAR, LOW, MODERATE, HIGH, and SEVERE. It is not
a regulatory limit, link-quality guarantee, or calibrated RF-power result.

## Menu pages and controls

Press `B` on the main menu until ENV TEST or ENV MORE is shown. On every active
environment page, `B` requests analyzer/probe stop and returns to the menu.

| Screen | What it shows | Controls |
|---|---|---|
| Occupancy | Per-channel moving occupancy, average, peak, and top channels | `RIGHT`: start/stop |
| Heatmap | Up to 32 circular time buckets across grouped RF channels | `RIGHT`: start/stop |
| Bursts | Up to 32 increases above the moving baseline, with LOW/MEDIUM/HIGH severity | `RIGHT`: start/stop; `UP/DOWN`: browse |
| Compare | Moving activity for the configured 2–4 RF channels | `RIGHT`: start/stop |
| RF Status | Running state, cycle/rate data, average, strongest channel, and relative score | `RIGHT`: start/stop |
| BEF/AFT | Two snapshots and signed per-channel change | `RIGHT`: capture Before, then After; `UP/DOWN`: channel |
| Band Info | Frequency and possible protocol-region overlap | `UP/DOWN`: RF channel |
| RX Only / Auth Probe | Receive-only notice, or bounded probe editor in the lab profile | See Authorized probe below |

Occupancy, Heatmap, Bursts, and RF Status start the same occupancy acquisition.
Compare starts comparison mode. Starting a run resets runtime environment
statistics. The default inclusive range is `0–125`, the default window is 10
seconds, the EMA alpha is 25%, and the burst threshold is 25 percentage points.

Heatmap history is circular: each completed sample window advances one column,
up to the configured depth. A burst is retained when activity exceeds an
established nonzero moving baseline by the configured threshold. The event list
holds the latest 32 events.

## Before/After snapshots

The first `RIGHT` press captures Before; the next captures After. Each snapshot
contains average and peak activity, peak channel, relative score, burst count,
and all 126 channel occupancies. Snapshots remain in RAM and disappear after a
restart or factory reset. If session recording is active, a compact `E` summary
is also appended to the active session CSV (SD `/RFSuite/log/rf_session.csv`
or LittleFS fallback `/rf_session.csv`).

## Band Info limitations

Band Info maps `RF_CH` to `2400 + RF_CH` MHz and reports whether that frequency
falls in broad Wi-Fi, Bluetooth/BLE, or Zigbee regions. Exact BLE advertising
centers and exact Wi-Fi/Zigbee channel centers may also be shown. A label such
as `possible`, `overlap`, or `band region` never means a protocol was detected
or decoded.

## Serial configuration

The complete command table is in [Serial CLI](SERIAL_CLI.md). Typical setup:

```text
env range 0 125
env window 10
env compare 12 37 62 80
env start occupancy
env status
env top
env stop
```

Window, range, comparison channels, burst threshold, EMA alpha, and history
depth are NVS-backed, although only window, range, and comparison channels are
currently editable through the Serial CLI. See [Persistence](PERSISTENCE.md).

## Session recording

Start Logging before an environment test to preserve its compact summary.
Environment runs and snapshots append `E` rows; they do not store every raw
observation or the complete heatmap. See [Data and Storage](DATA_AND_STORAGE.md)
for the exact format and storage limit.

## Authorized bounded probe

The default `analyzer` build displays RX ONLY and contains no active probe path.
The `authorized_rf_lab` profile exposes Auth Probe for shielded, explicitly
authorized testing. Its configurable bounds are:

| Setting | Range | Default |
|---|---:|---:|
| RF channel | `0–125` | `42` |
| Packet interval | `20–5000 ms` | `100 ms` |
| Packet limit | `1–1000` | `100` |
| Maximum duration | `1–60 s` | `10 s` |
| Payload size (Serial only) | `1–32 bytes` | `8 bytes` |
| Data rate (Serial only) | 250 kbps, 1 Mbps, 2 Mbps | 1 Mbps |

On-device, use `UP/DOWN` to select Channel, Interval, Packets, Duration, or
Start/Stop. `RIGHT` advances the selected value or explicitly starts/stops the
probe. The run stops at the first of packet limit, duration limit, or operator
stop, then returns the radio to RX. Pressing `B` is the immediate operator stop
and exit path. Read [Safety and Authorized Use](SAFETY.md) before using this
build profile.
