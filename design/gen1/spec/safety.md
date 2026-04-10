# Safety Specification

## Power Safety

### Battery Protection

| Requirement                  | Status      | Notes                          |
|------------------------------|-------------|--------------------------------|
| Reverse polarity protection  | NOT present | Add Schottky diode on input    |
| Over-voltage protection      | NOT present | 2S LiPo max 8.4V within LM1117 limits |
| Under-voltage cutoff         | TBD         | Implement in firmware (6.0V threshold) |
| Over-current protection      | NOT present | Consider polyfuse on battery input |
| Short circuit protection     | NOT present | Rely on battery BMS            |

### Low Battery Thresholds

| Level    | Cell Voltage | Pack Voltage | Enter At | Exit At | Action                      |
|----------|-------------|--------------|----------|---------|-----------------------------|
| Normal   | > 3.5V      | > 7.0V       | —        | —       | Normal operation             |
| Warning  | 3.3V - 3.5V | 6.6V - 7.0V | 7.0V     | 7.2V    | STAT_2 blink, BT notification |
| Critical | < 3.3V      | < 6.6V       | 6.6V     | 6.8V    | Ramp motors to 0, transition to Diag |
| Cutoff   | < 3.0V      | < 6.0V       | 6.0V     | —       | Motors off immediately       |

Note: Hysteresis prevents oscillation near thresholds. Enter/Exit voltages define
the transition boundaries.

## Motor Safety

### Startup Behavior

| Requirement                         | Status      | Notes                          |
|--------------------------------------|-------------|--------------------------------|
| MOSFET gate pull-down resistors      | REQUIRED    | 100K pull-downs on Q1-Q4 gates; BLOCKING defect until present |
| Motors off on power-up               | REQUIRED    | GPIO init to LOW before any other init; pull-downs ensure safety during reset |
| Arm/disarm command required          | REQUIRED    | Arm via `b0`=`"1"` from Diag only; disarm via `b0`=`"0"` |
| Motor ramp-up limit                  | REQUIRED    | Max 33 duty counts per PID tick (50% of MAX_THROTTLE per second) |

### Failsafe Conditions

| Condition                  | Action                                             |
|----------------------------|-----------------------------------------------------|
| Bluetooth disconnect       | Ramp motors to 0 over 200 ms; transition to Diag   |
| IMU failure (no data > 50 ms) | Ramp motors to 0 over 200 ms; transition to Diag |
| Low battery critical (< 6.6V) | Ramp motors to 0 over 200 ms; transition to Diag |
| Low battery cutoff (< 6.0V)  | Motors off immediately (no ramp); remain in Diag  |
| Watchdog timeout           | MCU reset → full startup sequence                   |
| Excessive tilt (> 60 deg)  | Motors off immediately (no ramp); transition to Diag |
| Tilt software limit (45 deg) | PID output saturated; no further tilt commanded   |

## EMI / Signal Integrity

| Requirement                          | Status      | Notes                          |
|---------------------------------------|-------------|--------------------------------|
| Input LC filter (battery)            | NOT present | Add 10uH + 100uF              |
| Gate resistors on MOSFETs            | NOT present | Add 100 ohm series             |
| Ferrite beads on Bluetooth lines     | NOT present | Add 100 ohm @ 100MHz          |
| Decoupling on all VDD pins           | Present     | 0.1uF on each VDD pin         |

## ESD Protection

| Interface        | Protection | Notes                         |
|------------------|------------|-------------------------------|
| Bluetooth (J2)   | None       | Consider TVS on TX/RX         |
| Programming (J3) | None       | Consider TVS on UART lines    |
| SWD (J8)         | None       | Consider TVS on SWDIO/SWCLK   |
| Battery (J1, J5) | None       | Consider TVS on power input   |

## Known DRC Issues (from KiCad)

| Type                  | Count | Severity | Action Needed              |
|-----------------------|-------|----------|----------------------------|
| connection_width      | 6     | Warning  | Widen traces to match net class |
| courtyards_overlap    | 2     | Error    | Adjust component placement |
| lib_footprint_mismatch| 4     | Warning  | Update footprints from library |
| starved_thermal       | 4     | Error    | Improve thermal relief geometry |
