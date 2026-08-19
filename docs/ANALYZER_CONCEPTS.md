# Analyzer Concepts

## What the analyzer measures

The nRF24L01+ does not expose a continuous calibrated RSSI value. The firmware repeatedly tests carrier/RPD activity during a channel observation window and reports:

```text
activity_percent = carrier_hits × 100 / samples_per_channel
```

The displayed `0–100%` value is therefore an observation ratio. It is useful for relative occupancy, comparisons, trends, and change detection. It is not dBm, protocol identity, packet count, or proof that a particular network produced the signal.

## RF channel and frequency

The UI uses nRF24 RF channel numbers:

```text
frequency_MHz = 2400 + RF_channel
```

Examples:

| RF channel | Frequency | Common reference |
|---:|---:|---|
| 2 | 2402 MHz | BLE advertising channel 37 center |
| 12 | 2412 MHz | Wi-Fi channel 1 center |
| 26 | 2426 MHz | BLE advertising channel 38 center |
| 37 | 2437 MHz | Wi-Fi channel 6 center |
| 62 | 2462 MHz | Wi-Fi channel 11 center |
| 80 | 2480 MHz | BLE advertising channel 39 center |

RF channel numbers are not Wi-Fi or BLE logical channel numbers. The ALL range includes RF channels 0–125 because that is the chip register range. Channels above 83 are above the nominal 2400–2483.5 MHz ISM band and should not be treated as standard 2.4 GHz ISM coverage.

## Scan bands

| Band | RF channel range | Derived frequency range |
|---|---:|---:|
| ALL | 0–125 | 2400–2525 MHz |
| Wi-Fi | 1–73 | 2401–2473 MHz |
| Bluetooth | 2–80 | 2402–2480 MHz |

These are acquisition ranges, not protocol decoders.

## Radio modes

| Mode | Two-radio behavior | One-radio fallback |
|---|---|---|
| FAST | R1 observes one channel while R2 observes the adjacent channel in the same sampling window | Available radio scans sequentially |
| DIV | Both receivers observe the same channel; a sample is active if either detects carrier | Available radio provides the result |
| R1 | R1 only | Falls back to R2 if R1 is unavailable |
| R2 | R2 only | Falls back to R1 if R2 is unavailable |

FAST reduces the number of observation windows for a full sweep when both modules are available. DIV can reduce the impact of module-to-module sensitivity and antenna-placement differences, but does not create calibrated diversity gain.

## Sampling profiles

Increasing samples per channel generally makes the ratio less sensitive to single observations but increases sweep duration.

| Profile | Spectrum | Inspector |
|---|---:|---:|
| FAST | 12 | 50 |
| BALANCED | 30 | 100 |
| DEEP | 60 | 200 |
| CUSTOM | 10–100 | `2 × CUSTOM`, limited to 20–200 |

Each channel includes a short settling delay before sampling. Individual samples also include a microsecond-scale delay. Actual sweep time depends on band width, profile, radio mode, UI callbacks, and SPI contention; use Status → Performance or `perf` instead of assuming a fixed rate.

## Traces

### LIVE

The latest completed sweep.

### AVG

An exponential moving average:

```text
average = (3 × previous_average + live_sample) / 4
```

The first survey sweep initializes the average directly from the live value.

### MAX

The highest live activity seen on each channel since boot or an explicit maximum clear. This is separate from the decaying white peak marker.

### DELTA

Positive change above the captured baseline:

```text
delta = max(live - baseline, 0)
```

Baseline capture copies the current moving average when survey data exists, otherwise it copies the current live sweep. Baseline data remains in RAM and is intentionally discarded on reboot. If `DELTA` is requested before baseline capture, its values are zero.

## Peak behavior

- `peakChannel` and `peakLevel` describe the strongest channel in the latest completed sweep.
- Per-channel white peak markers decay by one percentage point per completed sweep when above the live value.
- MAX trace values do not decay.

## Cursor, watch markers, and zoom

While live, the cursor follows the current peak. Freezing stops acquisition and lets `UP/DOWN` move the cursor. Resuming restores peak-follow.

Watch markers are persistent booleans for RF channels 0–125. A watched cursor is shown with `*`. Watch markers do not filter acquisition or generate events by themselves.

Zoom values 1×, 2×, and 4× change graph mapping around the cursor. Zoom does not narrow the acquisition band and therefore does not increase sweep rate.

## Observation confidence

`Q` is a bounded heuristic for acquisition depth:

```text
confidence = clamp(35 + samples_per_channel / 2
                   + 15 when two receivers are available
                   + 5 when a baseline exists,
                   0, 100)
```

Confidence is not measurement accuracy, signal quality, probability that a protocol is present, or calibration uncertainty.

## Waterfall and survey

- The waterfall is a circular RAM buffer of 24 completed sweeps.
- Survey accumulation adds each channel's activity percentage to a 32-bit total.
- Displayed survey occupancy is `total / surveySweeps`, capped at 100.
- Changing band from the Survey screen resets survey accumulation.

## Event engine

For each channel, the engine maintains a consecutive-run counter:

1. At or above threshold `T`, the counter increments up to 255.
2. At or below `T - H`, it resets to zero.
3. Between those levels, hysteresis retains the current counter.
4. A channel qualifies after at least `D` sweeps.
5. An event fires when at least `M` channels qualify.

The event record stores time since boot, current peak channel/level, qualifying-channel count, and longest run length. The event system uses an eight-entry circular buffer and a latch so sustained activity creates one event until all qualifying runs release. A 250 ms minimum interval also prevents immediate retriggering.

## Interpretation limits

- No Wi-Fi, Bluetooth, BLE, Zigbee, or proprietary packets are decoded.
- Wideband signals may appear across multiple adjacent RF channels.
- Different modules, antennas, supply noise, and enclosure placement can change results.
- Carrier/RPD thresholds are properties of the nRF24 receiver, not a calibrated laboratory instrument.
- Use a calibrated spectrum analyzer when absolute power, emissions compliance, bandwidth, spurious output, or precise frequency response matters.
