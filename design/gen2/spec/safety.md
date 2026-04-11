# Safety Specification (Gen 2)

## Gen 1 Issue Resolution

Every Gen 1 hardware issue is explicitly addressed in this design.

| Gen 1 Issue | Resolution in Gen 2                           |
|-------------|------------------------------------------------|
| HW-01 Missing gate pull-downs | Eliminated: using 4-in-1 ESC (built-in gate control) |
| HW-02 No reverse polarity    | Added: P-FET reverse polarity protection       |
| HW-03 No overcurrent         | Added: PTC resettable fuse (3A hold / 6A trip) |
| HW-04 Missing LC filter      | Added: LC filter on battery input (4.7 uH + 100 uF) |
| HW-05 Missing gate resistors | Eliminated: ESC handles gate drive internally  |
| HW-06 No ESD protection      | Added: TVS on USB-C (USBLC6-2SC6), TVS on SWD |
| HW-07 DRC violations         | Addressed: 4-layer PCB resolves trace width and thermal issues |
| HW-08 IMU/BT power sequence  | Addressed: separate enable control; IMU powers on after 200 ms delay |
| HW-09 BOOT0 jumper usability | Replaced: USB-C DFU via software button hold   |
| HW-10 Cannot power off       | Fixed: high-side P-FET switch with clean cutoff |
| HW-11 Overweight (74g)       | Fixed: 141g AUW with 800g thrust (T/W 5.7:1) |
| HW-12 BT/IMU interference    | Fixed: IMU at board center, BLE at edge, ground plane shielding |
| HW-13 LDO inefficient        | Replaced: TPS63070 buck-boost (93% vs 45% efficiency) |

## Power Safety

### Battery Protection

| Requirement                  | Status      | Implementation                 |
|------------------------------|-------------|--------------------------------|
| Reverse polarity protection  | Present     | P-FET (DMP2035U) on battery input |
| Over-voltage protection      | Present     | 3S LiPo max 12.6V within TPS63070 input range |
| Under-voltage cutoff         | Present     | Firmware: 3.0V/cell cutoff, motors off immediately |
| Over-current protection      | Present     | PTC fuse: 5A hold / 10A trip    |
| Short circuit protection     | Present     | PTC fuse + battery BMS          |
| Charge protection            | N/A         | External balance charger handles CV/CC/termination |

### Low Battery Thresholds (Per-Cell, 3S)

All thresholds are defined per-cell and applied to the 3S pack.

| Level    | Per-Cell V  | Enter At | Exit At | 3S Pack    | Action |
|----------|------------|----------|---------|------------|--------|
| Normal   | > 3.5V     | --       | --      | > 10.5V    | Normal operation |
| Warning  | 3.3-3.5V   | 3.5V     | 3.6V    | 10.5V/10.8V| LED orange, buzzer/2s, telemetry |
| Critical | 3.0-3.3V   | 3.3V     | 3.4V    | 9.9V/10.2V | Ramp motors to 0, → Diag, LED red, buzzer rapid |
| Cutoff   | < 3.0V     | 3.0V     | --      | 9.0V       | Motors off immediately, remain in Diag |

Note: Hysteresis (100 mV per cell) prevents oscillation near thresholds.

Thresholds are evaluated **per individual cell** (via balance connector
monitoring), not just the pack average. Any single cell falling below
threshold triggers the corresponding action. See
[hardware.md](hardware.md#per-cell-voltage-monitoring).

### Cell Imbalance Detection

| Condition                              | Action                                |
|-----------------------------------------|---------------------------------------|
| Max cell - min cell > 50 mV (warning)  | LED flash, telemetry warning          |
| Max cell - min cell > 100 mV (error)   | Refuse arm; require recharge / balance|
| Max cell - min cell > 200 mV in flight | Critical: ramp motors to 0, → Diag    |

Imbalance indicates a damaged cell or improper charging. Refusing arm
prevents flight on a battery that may suddenly sag below cutoff under load.

### Input Power Filtering

| Component   | Value        | Purpose                         |
|-------------|-------------|----------------------------------|
| L1 (inductor)| 4.7 uH     | Filter motor PWM noise from battery rail |
| C_FILT      | 100 uF MLCC | Low-impedance bulk filter       |
| Location    | Between PTC fuse and ESC/regulator input | |

## Motor Safety

### ESC Safety Features

| Requirement                         | Status      | Implementation                 |
|--------------------------------------|-------------|--------------------------------|
| Motors off on power-up               | Present     | ESC requires DSHOT arm sequence; no output until armed |
| Arm/disarm required                  | Present     | AUX1 channel (CH5) arm switch  |
| Bidirectional DSHOT300               | Present     | BlueJay firmware on ESC; eRPM telemetry per motor |
| ESC desync protection                | Present     | BlueJay firmware handles desync |
| Motor stall detection (RPM)          | Present     | Bidirectional DSHOT eRPM (primary) |
| Motor stall detection (current)      | Present     | Shunt + op-amp (secondary, redundancy) |

### Motor RPM Monitoring (Primary Stall Detection)

| Parameter              | Value                          |
|------------------------|--------------------------------|
| Source                 | Bidirectional DSHOT300 eRPM (per motor) |
| Update Rate            | 4 kHz (every rate PID cycle)   |
| Resolution             | 12-bit eRPM (~0.1% at typical RPMs) |
| Pole Pairs             | 7 (for 1404 motors with 14 magnets) |
| RPM Range              | 0 - 50,000 RPM (3S 1404)            |
| Stall Detection        | eRPM = 0 with DSHOT throttle > 200 for > 100 ms |
| Desync Detection       | eRPM deviation > 30% from expected for > 200 ms |
| Action on Stall        | Disarm all motors, → Diag, report error      |
| Action on Desync       | Warn but continue; log event                   |

### Motor Current Monitoring (Secondary, Backup)

| Parameter              | Value                          |
|------------------------|--------------------------------|
| Sense Resistor         | 10 mohm per motor (on ESC output) |
| Op-Amp Gain            | 50x (external op-amp on carrier) |
| Over-current Threshold | 5 A per motor (software limit) |
| Use                    | Backup stall detection if RPM telemetry fails |
| Action on Overcurrent  | Affected motor DSHOT = 0; → Diag |

### Failsafe Conditions

| Condition                      | Detection               | Action                                     |
|--------------------------------|-------------------------|---------------------------------------------|
| ELRS link loss                 | No valid CRSF > 500 ms | Ramp motors to 0 over 200 ms; → Diag       |
| IMU failure (no INT1)          | No INT1 pulse > 10 ms  | Ramp motors to 0 over 200 ms; → Diag       |
| Low battery critical (< 3.3V) | ADC threshold           | Ramp motors to 0 over 200 ms; → Diag       |
| Low battery cutoff (< 3.0V)   | ADC threshold           | Motors off immediately; remain in Diag      |
| Watchdog timeout               | IWDG hardware           | MCU reset → full startup sequence           |
| Excessive tilt (> 60 deg)     | IMU angle calculation   | Motors off immediately; → Diag              |
| PID tilt limit (45 deg)       | PID saturation          | PID output clamped; no further tilt         |
| Motor stall detected           | DSHOT eRPM = 0 with thr > 200 for 100 ms | Motors off immediately; → Diag |
| Motor overcurrent (> 5 A)     | Current sensing (backup)| Affected motor DSHOT = 0; → Diag           |
| Motor desync detected          | DSHOT eRPM > 30% off expected | Warn; continue flight                |
| ESC bidirectional DSHOT failure| No GCR response for > 1 s | Fall back to current-only stall detection |
| Cell imbalance (> 100 mV)     | Per-cell ADC monitoring | Refuse arm; warn user                       |
| Cell critical imbalance (> 200 mV in flight) | Per-cell ADC | Ramp motors to 0; → Diag        |
| BLE disconnect                 | UART timeout            | No flight action (BLE is not flight-critical)|

## EMI / Signal Integrity

| Requirement                          | Status      | Implementation                 |
|---------------------------------------|-------------|--------------------------------|
| Input LC filter (battery)            | Present     | 4.7 uH + 100 uF on battery input |
| Continuous ground plane               | Present     | 4-layer: L2 = unbroken GND plane |
| IMU placement isolation               | Present     | Board center, local GND pour, away from power traces |
| BLE antenna separation                | Present     | BLE module at board edge, >= 10 mm from IMU |
| BLE ground keepout                    | Present     | No copper under antenna per module datasheet |
| SPI signal integrity                  | Present     | Short traces (< 20 mm); series termination 33 ohm on SCK if needed |
| Decoupling on all VDD pins            | Present     | 100 nF on each VDD pin (0402)  |
| Shielded inductor (buck-boost)        | Present     | Shielded 1 uH to minimize radiated EMI |
| Motor power routing (bottom layer)    | Present     | High-current motor traces on L4, away from analog on L1 |

## ESD Protection

| Interface          | Protection           | Device / Method            |
|--------------------|---------------------|----------------------------|
| USB-C (D+/D-)     | TVS diode array     | USBLC6-2SC6 (SOT-23-6)    |
| USB-C (VBUS)       | TVS + PTC fuse      | Charger IC handles OVP     |
| SWD (SWDIO/SWCLK) | TVS diode array     | PRTR5V0U2X (SOT-363)      |
| Battery connector  | PTC fuse + P-FET    | No direct ESD path to ICs  |
| ELRS RX pads       | Internal to module   | Module handles ESD          |
| BLE module pads    | Internal to module   | Module handles ESD          |

## Thermal Safety

| Component            | Max Temp | Estimated Rise | Margin  |
|----------------------|----------|---------------|---------|
| STM32H743 (480 MHz)  | 105 C    | ~15 C above ambient | 50+ C |
| TPS63070 (400 mA)    | 150 C    | ~8 C above ambient | 90+ C |
| ESC MOSFETs (hover)  | 150 C    | ~20 C above ambient | 80+ C |

No active cooling required. All components operate well within thermal limits
at maximum load in 40 C ambient.

## Software Safety

### Watchdog

| Parameter              | Value                          |
|------------------------|--------------------------------|
| Type                   | Independent Watchdog (IWDG)    |
| Clock                  | LSI (~32 kHz)                  |
| Timeout                | 100 ms                         |
| Refresh                | Every rate PID loop iteration  |
| On Timeout             | MCU hardware reset → full startup |

### Stack Overflow Protection

| Parameter              | Value                          |
|------------------------|--------------------------------|
| MPU Region             | Stack guard (4 bytes, no access)|
| Detection              | MemManage fault → MCU reset    |
| Stack Size             | Main: 2 KB, ISR: 1 KB (Zephyr configured) |

### D-Cache Coherency (H743 Specific)

| Concern                | Mitigation                     |
|------------------------|--------------------------------|
| DMA + D-cache stale data | Use DTCM (no cache) for all DMA buffers |
| Alternative            | Cache-aligned buffers + SCB_CleanDCache_by_Addr() before DMA TX, SCB_InvalidateDCache_by_Addr() after DMA RX |
| MPU configuration      | Mark DMA buffer regions as non-cacheable via MPU |

### DMA Safety

| Concern                | Mitigation                     |
|------------------------|--------------------------------|
| DMA overrun            | Double-buffering on IMU SPI DMA |
| DMA error              | DMA error interrupt → flag fault, disable motors |
| Buffer corruption      | DMA buffers in dedicated SRAM section, not shared |
