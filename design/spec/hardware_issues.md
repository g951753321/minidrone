# Hardware Issues

Known hardware issues, circuit design mistakes, and improvement items
for the miniDrone Rev 1 PCB.

## Circuit Design Mistakes

### HW-01: Missing MOSFET Gate Pull-Down Resistors

| Field    | Detail |
|----------|--------|
| Severity | BLOCKING |
| Components | Q1-Q4 (SI2302) |
| Problem  | No 100K pull-down resistors on MOSFET gates. During MCU reset or power-on glitches, gate pins float and motors may spin unexpectedly. |
| Fix      | Add 100K pull-down resistors from each gate to GND. |
| Reference | safety.md: Motor Safety - Startup Behavior |

### HW-02: No Reverse Polarity Protection

| Field    | Detail |
|----------|--------|
| Severity | High |
| Components | J1, J5 (battery connectors) |
| Problem  | Reversed battery connection will damage LM1117, MCU, and peripherals. |
| Fix      | Add P-channel MOSFET or Schottky diode on battery input. |
| Reference | safety.md: Battery Protection |

### HW-03: No Over-Current / Short Circuit Protection

| Field    | Detail |
|----------|--------|
| Severity | Medium |
| Components | Battery input path |
| Problem  | A motor short or wiring fault draws unlimited current from battery. Relies entirely on battery BMS. |
| Fix      | Add polyfuse (resettable PTC) on battery input. |
| Reference | safety.md: Battery Protection |

### HW-04: Missing Input LC Filter on Battery

| Field    | Detail |
|----------|--------|
| Severity | Medium |
| Components | Battery input |
| Problem  | Motor PWM switching noise couples back into power rail, affecting ADC readings and digital logic. |
| Fix      | Add 10 uH inductor + 100 uF capacitor LC filter on battery input. |
| Reference | safety.md: EMI / Signal Integrity |

### HW-05: Missing Gate Resistors on MOSFETs

| Field    | Detail |
|----------|--------|
| Severity | Low |
| Components | Q1-Q4 gates |
| Problem  | No series gate resistors causes high dI/dt switching transients, contributing to EMI. |
| Fix      | Add 100 ohm series resistors on each MOSFET gate drive line. |
| Reference | safety.md: EMI / Signal Integrity |

### HW-06: No ESD Protection on External Interfaces

| Field    | Detail |
|----------|--------|
| Severity | Low |
| Components | J2 (Bluetooth), J3 (programming), J8 (SWD) |
| Problem  | No TVS diodes on any external-facing connector. ESD events during handling or programming can damage MCU. |
| Fix      | Add TVS diode arrays on UART TX/RX, SWD, and battery lines. |
| Reference | safety.md: ESD Protection |

### HW-07: KiCad DRC Violations

| Field    | Detail |
|----------|--------|
| Severity | Medium |
| Problem  | 6 connection_width warnings, 2 courtyard overlap errors, 4 footprint mismatches, 4 starved thermal reliefs. |
| Fix      | Widen traces to match net class; adjust component placement; update footprints; improve thermal relief geometry. |
| Reference | safety.md: Known DRC Issues |

## Power Sequence Issues

### HW-08: IMU Powers Up Before HM-13 Bluetooth Module

| Field    | Detail |
|----------|--------|
| Severity | Medium |
| Components | MPU6050 (J4), HM-13 (J2) |
| Problem  | Both peripherals share the 3.3V rail and power on simultaneously. The IMU begins outputting data before the HM-13 has completed its own initialization. The HM-13 draws significant inrush current during startup, causing a voltage dip that can produce erratic IMU readings during the critical calibration window. |
| Observed | First ~50 IMU samples after power-on show high variance (axis variance up to 2M), requiring a firmware warm-up discard period. |
| Fix      | Add a load switch or RC delay on the IMU 3.3V supply so it powers on after the HM-13 has settled (~200 ms). Alternatively, add separate LDOs for analog and digital domains. |

## Mechanical / Usability Issues

### HW-09: J11 Jumper (BOOT0) Difficult to Use

| Field    | Detail |
|----------|--------|
| Severity | Medium |
| Components | J11 (BOOT0 jumper) |
| Problem  | Entering UART bootloader mode requires manually inserting/removing a jumper on J11. This is tedious during iterative development and risks bending pins or losing the jumper cap. |
| Fix      | Replace J11 with a tactile push button (active-high with pull-down) or add a DTR/RTS-controlled auto-reset circuit (transistor toggling BOOT0 + NRST from the USB-UART adapter). |

### HW-10: No Power Off When Batteries Connected

| Field    | Detail |
|----------|--------|
| Severity | High |
| Components | SW3 (power switch), J1, J5 |
| Problem  | The power switch SW3 does not fully disconnect the battery from the circuit, or the switch position is ambiguous. When batteries are plugged in, the system cannot be fully powered off, leading to continuous current draw and potential deep-discharge of the LiPo cells. |
| Fix      | Verify SW3 DPDT wiring cuts both +BATT and GND paths. If the switch is correctly wired, consider adding a high-side P-FET power switch for clean cutoff with lower contact resistance. Add a power indicator LED downstream of SW3 to confirm on/off state. |

### HW-11: Total Weight Too Heavy (74g)

| Field    | Detail |
|----------|--------|
| Severity | High |
| Components | Entire assembly |
| Problem  | Measured all-up weight is 74g. For the brushed coreless motors and propellers used, this is too heavy for stable hover and adequate flight time. Target should be under 50g. |
| Breakdown | See mechanical.md weight budget (TBD entries need actual measurements). |
| Fix      | Reduce PCB size (remove unused breakout headers J9, J10). Use lighter battery (smaller capacity or single-cell with boost converter). Minimize connector weight (solder motors directly). Remove unnecessary headers (J6 placeholder, J7 expansion). Consider 4-layer PCB to reduce board area. |

## Power Architecture Issues

### HW-13: LM1117 Regulator Unnecessary — Use Battery Voltage Directly

| Field    | Detail |
|----------|--------|
| Severity | Medium |
| Components | U2 (LM1117MP-3.3), battery pack |
| Problem  | The current design uses an LM1117 LDO to regulate 7.4V (2S) down to 3.3V. This wastes ~385 mW as heat (4.1V dropout × 94 mA), adds weight and board space, and introduces a single point of failure. The motors already run directly from +BATT; only the MCU, IMU, and BT module need 3.3V. |
| Proposal | Use a 2S (7.4V) or 3S (11.1V) LiPo battery to power motors directly, and replace the LM1117 with a small buck converter (e.g., TPS562200 or MP2359) for the 3.3V rail. Benefits: higher efficiency (~90% vs ~45%), less heat, supports wider battery voltage range, and enables 3S for more motor thrust without redesigning the digital rail. Alternatively, for minimum weight, use a single-cell LiPo (3.7V) with a buck-boost to 3.3V and drive motors directly from the cell. |

## EMI / Interference Issues

### HW-12: HM-13 and MPU6050 Placement Interference

| Field    | Detail |
|----------|--------|
| Severity | Medium |
| Components | HM-13 (J2), MPU6050 (J4) |
| Problem  | The Bluetooth module and IMU are mounted in close proximity on the PCB. The HM-13 2.4 GHz RF transmission introduces high-frequency noise that couples into the IMU's analog front-end, potentially affecting accelerometer and gyroscope readings during active BT communication. |
| Observed | Possible correlation between BT telemetry bursts and IMU noise spikes (needs further characterization). |
| Fix      | Increase physical separation between HM-13 and MPU6050. Add ground pour / shielding between the two modules. Add ferrite beads (100 ohm @ 100 MHz) on I2C lines to the IMU. In Rev 2, place IMU at board center and BT module at board edge with antenna facing outward. |

## Summary

| ID    | Issue                          | Severity | Category  |
|-------|--------------------------------|----------|-----------|
| HW-01 | Missing MOSFET gate pull-downs | BLOCKING | Circuit   |
| HW-02 | No reverse polarity protection | High     | Circuit   |
| HW-03 | No over-current protection     | Medium   | Circuit   |
| HW-04 | Missing battery input LC filter | Medium  | Circuit   |
| HW-05 | Missing MOSFET gate resistors  | Low      | Circuit   |
| HW-06 | No ESD protection              | Low      | Circuit   |
| HW-07 | KiCad DRC violations           | Medium   | Circuit   |
| HW-08 | IMU/HM-13 power sequence       | Medium   | Power     |
| HW-09 | BOOT0 jumper usability         | Medium   | Usability |
| HW-10 | Cannot power off with battery  | High     | Usability |
| HW-11 | Total weight 74g (too heavy)   | High     | Mechanical|
| HW-12 | BT/IMU placement interference  | Medium   | EMI       |
| HW-13 | LDO inefficient, use battery direct | Medium | Power |
