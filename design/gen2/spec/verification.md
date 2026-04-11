# Verification Architecture (Gen 2)

## 1. Provable Properties Catalog

Properties are classified as **formally provable** (P) or **test-only** (T) based
on whether the logic is deterministic and side-effect-free.

### 1.1 Safety Properties (Must Prove)

| ID    | Property                                              | Class | Tool   |
|-------|-------------------------------------------------------|-------|--------|
| VP-01 | State machine never reaches an undefined state        | P     | TLA+   |
| VP-02 | Motors DSHOT = 0 in any state other than Running/armed| P     | TLA+   |
| VP-03 | Calibrate → Diag only on success; never Calibrate → Running | P | TLA+ |
| VP-04 | Disarm from Running always reaches Diag within finite steps | P | TLA+ |
| VP-05 | Battery cutoff (< 3.0V) forces motors off in all states | P  | TLA+   |
| VP-06 | Rate PID integrator |value| <= WINDUP_MAX at all times | P | CBMC  |
| VP-07 | Angle PID integrator |value| <= WINDUP_MAX at all times | P | CBMC |
| VP-08 | Motor DSHOT throttle <= 2047 for all inputs           | P     | CBMC   |
| VP-09 | No arithmetic overflow in PID calculation (int16 inputs, float intermediates) | P | CBMC |
| VP-10 | CRSF stick center (992) produces exactly zero setpoint | P   | CBMC   |
| VP-11 | Voltage divider ADC-to-mV conversion never overflows uint16 (30K/10K divider, up to 12.6V) | P | CBMC |
| VP-11a| Cell count detection returns 1, 2, or 3 for all valid voltage ranges; returns ERROR for dead zone (4.4-6.0V) | P | CBMC |
| VP-12 | Motor stall detection triggers disarm in all states   | P     | TLA+   |
| VP-13 | Error mode is absorbing: only exit is watchdog reset  | P     | TLA+   |

### 1.2 Liveness Properties (Must Prove)

| ID    | Property                                              | Class | Tool   |
|-------|-------------------------------------------------------|-------|--------|
| VP-14 | System always eventually exits Calibrate (success or timeout) | P | TLA+ |
| VP-15 | Motor ramp-down completes within duration_ms          | P     | TLA+   |
| VP-16 | Watchdog is refreshed at least once per 100 ms        | P     | TLA+   |
| VP-17 | CRSF failsafe activates within 500 ms of link loss   | P     | TLA+   |

### 1.3 Protocol Properties (Must Prove)

| ID    | Property                                              | Class | Tool   |
|-------|-------------------------------------------------------|-------|--------|
| VP-18 | CRSF parser always terminates (no infinite loop)      | P     | CBMC   |
| VP-19 | CRSF parser output is empty OR has valid channels     | P     | CBMC   |
| VP-20 | CRSF parser discards frames with bad CRC              | P     | CBMC   |
| VP-21 | CRSF parser buffer never reads/writes out of bounds   | P     | CBMC   |
| VP-22 | BLE protocol parser always terminates                 | P     | CBMC   |
| VP-23 | BLE protocol parser rejects invalid checksum          | P     | CBMC   |
| VP-24 | BLE protocol parser buffer never overflows            | P     | CBMC   |

### 1.4 Additional Provable Properties

| ID    | Property                                              | Class | Tool   |
|-------|-------------------------------------------------------|-------|--------|
| VP-25 | Battery level hysteresis: per-cell warning enter 3.5V, exit 3.6V; critical enter 3.3V, exit 3.4V; applied to 3S pack | P | CBMC |
| VP-26 | Calibration variance check rejects samples with stddev > threshold | P | CBMC |
| VP-27 | Arm debounce rejects re-arm when elapsed_ms < 1000    | P     | CBMC   |
| VP-28 | Motor mixer output clamped to [0, 2047] for all inputs| P     | CBMC   |
| VP-29 | Motor mixer never produces negative duty values        | P     | CBMC   |
| VP-30 | CRSF channel value mapping stays within defined ranges | P    | CBMC   |
| VP-31 | Altitude estimator fusion weight transitions smoothly at crossfade zone | P | CBMC |
| VP-32 | Pre-flight checks: all conditions must pass for arm to succeed | P | TLA+ |
| VP-33 | DSHOT CRC is correct for all throttle values (0-2047) | P     | CBMC   |
| VP-34 | Complementary filter output is bounded for bounded inputs | P | CBMC   |

### 1.5 Testable-Only Properties

| ID    | Property                                              | Class | Tool       |
|-------|-------------------------------------------------------|-------|------------|
| VT-01 | IMU WHO_AM_I returns 0x47 on valid hardware           | T     | Ztest      |
| VT-02 | BMP390 CHIP_ID returns 0x60 on valid hardware         | T     | Ztest      |
| VT-03 | VL53L5CX firmware upload completes within 500 ms      | T     | Ztest+HW   |
| VT-04 | PMW3901 Product_ID returns 0x49                       | T     | Ztest      |
| VT-05 | SPI bus recovery succeeds after MISO stuck            | T     | Ztest+HW   |
| VT-06 | I2C bus recovery succeeds after SDA stuck low         | T     | Ztest+HW   |
| VT-07 | ELRS link established within 5 s of TX power-on       | T     | Ztest+HW   |
| VT-08 | BLE advertising starts within 1 s of boot             | T     | Ztest+HW   |
| VT-09 | LED patterns match mode (visual / GPIO check)         | T     | Ztest      |
| VT-10 | Rate PID loop timing meets 4 kHz +/- 2% on target HW | T     | Ztest+HW   |
| VT-11 | Angle PID loop timing meets 1 kHz +/- 5% on target HW| T     | Ztest+HW   |
| VT-12 | Motor test pulse duration is 500 ms +/- 10%           | T     | Ztest+HW   |
| VT-13 | DSHOT signal timing meets spec (bit 0/1 widths)       | T     | Logic analyzer |
| VT-14 | Telemetry messages arrive at specified rates           | T     | Ztest      |
| VT-15 | Blackbox write does not increase PID loop jitter > 2% | T     | Ztest+HW   |
| VT-16 | USB DFU mode entered successfully via SW1 hold        | T     | Ztest+HW   |
| VT-17 | OTA update completes successfully end-to-end          | T     | Ztest+HW   |
| VT-18 | Altitude hold accuracy +/- 15 cm in static hover      | T     | Flight test|
| VT-19 | Position hold accuracy +/- 20 cm in static hover      | T     | Flight test|
| VT-20 | Motor current measurement accuracy +/- 10%            | T     | Bench test |

---

## 2. Purity Boundary Map

The architecture is split into a **deterministic pure core** (formally verifiable)
and an **effectful shell** (tested only).

```
+----------------------------------------------------------------------+
|                        Effectful Shell                                |
|                                                                      |
|  +------------------+  +---------------+  +----------------------+   |
|  | CRSF Backend     |  | SPI Driver    |  | DSHOT DMA HAL        |   |
|  | (UART RX → parse |  | (ICM-42688,   |  | (TIM1 CH1-4 DMA     |   |
|  |  → drone_cmd_t)  |  |  Flash, OF)   |  |  motor output)       |   |
|  +--------+---------+  +-------+-------+  +----------+-----------+   |
|           |                     |                     ^              |
|  +------------------+  +---------------+  +----------+-----------+   |
|  | BLE Backend      |  | I2C Driver    |  | WS2812B DMA HAL      |   |
|  | (UART ↔ nRF52    |  | (BMP390,      |  | (LED output)         |   |
|  |  → drone_cmd_t)  |  |  VL53L5CX)    |  |                      |   |
|  +--------+---------+  +-------+-------+  +----------------------+   |
|           |                     |                                    |
|  +------------------+  +---------------+  +----------------------+   |
|  | USB CDC / DFU    |  | ADC HAL       |  | GPIO HAL             |   |
|  | (debug console,  |  | (battery,     |  | (buzzer, button)     |   |
|  |  firmware update) |  |  motor current)|  |                     |   |
|  +--------+---------+  +-------+-------+  +----------------------+   |
|           |                     |                                    |
+----------------------------------------------------------------------+
            |                     |                   |
            v                     v                   |
+----------------------------------------------------------------------+
|                        Pure Core                                     |
|                                                                      |
|  +------------------+  +-------------------+  +------------------+   |
|  | Command Dispatch |  | Rate PID (3-axis) |  | State Machine    |   |
|  | (drone_cmd_t,    |  | (setpoint, rate,  |  | (event, state →  |   |
|  |  mode → event)   |  |  dt → output)     |  |  next_state,     |   |
|  +------------------+  +-------------------+  |  actions[])      |   |
|                        +-------------------+  +------------------+   |
|                        | Angle PID (2-axis)|                         |
|                        | (setpoint, angle, |                         |
|                        |  dt → rate_sp)    |                         |
|                        +-------------------+                         |
|                                                                      |
|  +------------------+  +-------------------+  +------------------+   |
|  | CRSF Parser      |  | Battery Level     |  | Calibration Math |   |
|  | (bytes[] →       |  | (mV → enum        |  | (samples[] →     |   |
|  |  channels[16])   |  |  w/ hysteresis)   |  |  offsets[])      |   |
|  +------------------+  +-------------------+  +------------------+   |
|                                                                      |
|  +------------------+  +-------------------+  +------------------+   |
|  | BLE Protocol     |  | Input Mapper      |  | Motor Mixer      |   |
|  | Parser (bytes[]  |  | (CRSF channels →  |  | (throttle, PID → |   |
|  |  → ble_msg_t)    |  |  stick struct)    |  |  dshot[4])       |   |
|  +------------------+  +-------------------+  +------------------+   |
|                                                                      |
|  +------------------+  +-------------------+  +------------------+   |
|  | Attitude         |  | Altitude          |  | Position         |   |
|  | Estimator        |  | Estimator         |  | Estimator        |   |
|  | (accel, gyro, dt |  | (baro, tof, dt →  |  | (flow, alt, dt → |   |
|  |  → pitch,roll,yaw)|  |  altitude_cm)    |  |  vel_x, vel_y)   |   |
|  +------------------+  +-------------------+  +------------------+   |
|                                                                      |
|  +------------------+  +-------------------+                         |
|  | DSHOT Encoder    |  | Pre-flight Check  |                         |
|  | (throttle →      |  | (sensors, battery,|                         |
|  |  dshot_frame[16])|  |  link → pass/fail)|                         |
|  +------------------+  +-------------------+                         |
|                                                                      |
+----------------------------------------------------------------------+
```

### 2.1 Pure Core Modules

| Module                | Inputs                         | Outputs                      |
|-----------------------|--------------------------------|------------------------------|
| **State Machine**     | current_state, event           | next_state, action_list      |
| **Command Dispatch**  | drone_cmd_t, current_mode      | drone_event_t or IGNORE      |
| **Rate PID** (x3)    | setpoint, gyro_rate, dt, gains | control_output (float)       |
| **Angle PID** (x2)   | setpoint, est_angle, dt, gains | rate_setpoint (float)        |
| **CRSF Parser**       | byte_buffer[], length          | crsf_channels[16] (uint16)  |
| **BLE Protocol Parser**| byte_buffer[], length         | ble_msg_t or EMPTY           |
| **Input Mapper**      | crsf_channels[16]              | drone_cmd_t (stick)          |
| **Battery Level**     | voltage_mv, prev_level         | level_enum (with hysteresis) |
| **Calibration Math**  | raw_samples[N][6]              | offsets[6], variance_ok      |
| **Motor Mixer**       | throttle, pitch, roll, yaw     | dshot[4] (uint16, clamped)   |
| **Attitude Estimator**| accel[3], gyro[3], dt          | pitch, roll, yaw (float)     |
| **Altitude Estimator**| baro_pa, tof_mm, dt            | altitude_cm (int32)          |
| **Position Estimator**| flow_dx, flow_dy, alt, dt      | vel_x, vel_y (float)         |
| **DSHOT Encoder**     | throttle (uint16), telem_req   | dshot_frame (uint16)         |
| **Pre-flight Check**  | sensor_status, batt, link, sticks | pass/fail + reason        |

### 2.2 Effectful Shell Modules

| Module                  | Side Effects                              |
|-------------------------|-------------------------------------------|
| **CRSF Backend**        | UART1 RX DMA → raw bytes → CRSF parser → drone_cmd_t |
| **BLE Backend**         | UART2 RX/TX → BLE protocol parser → drone_cmd_t / telemetry |
| **SPI IMU Driver**      | SPI1 DMA read ICM-42688 registers         |
| **SPI Flash Driver**    | SPI2 read/write/erase W25Q128             |
| **SPI OF Driver**       | SPI2 read PMW3901 motion data             |
| **I2C Baro Driver**     | I2C1 read BMP390 pressure/temperature     |
| **I2C ToF Driver**      | I2C1 configure/read VL53L5CX             |
| **ADC Battery HAL**     | ADC1 DMA read battery voltage             |
| **ADC Current HAL**     | ADC2 read motor current via op-amp        |
| **DSHOT DMA HAL**       | TIM1 DMA output DSHOT frames to 4 motors  |
| **WS2812B DMA HAL**     | SPI/TIM DMA output LED data               |
| **GPIO HAL**            | Buzzer PWM, button read                   |
| **USB CDC/DFU**         | USB peripheral for serial/firmware update |
| **Blackbox Logger**     | Collect pure core outputs → write to flash |
| **Watchdog HAL**        | Refresh IWDG timer                        |
| **Main Loop**           | Orchestrates shell ↔ core data flow       |

### 2.3 Boundary Rules

1. Pure core modules MUST NOT include any Zephyr API calls (no `k_*`, no device bindings)
2. Pure core modules accept C primitive types and structs, never device pointers
3. All hardware interaction flows through the effectful shell
4. The shell calls pure core functions and acts on the returned action_list
5. Pure core modules are compilable and testable on the host (x86) without Zephyr
6. Pure core modules MUST NOT allocate heap memory (all static or stack)
7. Pure core modules MUST be reentrant (no static mutable state except PID integrators, which are passed as struct pointers)

---

## 3. Verification Tooling

| Tool     | Target                  | Purpose                              |
|----------|-------------------------|--------------------------------------|
| **TLA+** | State machine model     | Prove safety & liveness of mode transitions, motor invariants, failsafe |
| **CBMC** | C source (pure core)    | Bounded model checking: overflow, array bounds, termination |
| **Ztest**| Full firmware on target | Zephyr-native unit and integration tests |
| **Ztest (native_sim)** | Pure core on host | Fast iteration without hardware |
| **Fuzz** | CRSF + BLE parsers      | libFuzzer / AFL++ on parsers in isolation |
| **HIL**  | Full system on hardware | Hardware-in-the-loop flight simulation |

### 3.1 TLA+ Scope

Model the state machine covering:
- States: {Calibrate, Diag, Running, Error}
- Events: {calib_ok, calib_fail, calib_timeout, arm, disarm,
           elrs_failsafe, low_batt_critical, low_batt_cutoff, imu_failure,
           excessive_tilt, motor_stall, motor_overcurrent, sw1_press,
           recalibrate, pre_flight_fail}
- Properties: VP-01 through VP-05, VP-12 through VP-17, VP-32

### 3.2 CBMC Scope

Bounded model check the following C functions:

| Function                | Properties Checked           |
|-------------------------|------------------------------|
| crsf_parse()            | VP-18, VP-19, VP-20, VP-21   |
| ble_protocol_parse()    | VP-22, VP-23, VP-24          |
| pid_rate_update()       | VP-06, VP-09                 |
| pid_angle_update()      | VP-07, VP-09                 |
| input_map_channels()    | VP-10, VP-30                 |
| motor_mixer_update()    | VP-08, VP-28, VP-29          |
| adc_to_mv()             | VP-11                        |
| battery_level_update()  | VP-25                        |
| calibration_compute()   | VP-26                        |
| arm_debounce_check()    | VP-27                        |
| dshot_encode()          | VP-33                        |
| attitude_update()       | VP-34                        |
| altitude_fuse()         | VP-31                        |
| pre_flight_check()      | VP-32                        |

### 3.3 Fuzz Testing Scope

| Target              | Input                    | Properties Verified      |
|---------------------|--------------------------|--------------------------|
| CRSF parser         | Arbitrary byte sequences | VP-18, VP-19, VP-21      |
| BLE protocol parser | Arbitrary byte sequences | VP-22, VP-23, VP-24      |
| Input mapper        | Random channel values    | VP-10, VP-30             |

Build parsers as standalone libraries (pure core) and link with libFuzzer.
Fuzz corpus seeded with valid CRSF and BLE frames, then mutated.

### 3.4 Hardware-in-the-Loop (HIL) Testing

| Test                   | Setup                         | Validates              |
|------------------------|-------------------------------|------------------------|
| IMU → PID → Motor loop | IMU on vibration table        | VT-10, VT-11 (timing) |
| ELRS end-to-end        | TX + RX in RF shielded box    | VT-07 (link timing)   |
| Failsafe               | TX power off during armed     | VP-17 (500 ms detect) |
| Battery simulation     | Programmable power supply     | VP-05, VP-25           |
| Motor current          | Resistive motor load          | VT-20 (accuracy)      |
| Full flight sim        | Drone on tethered rig         | VT-18, VT-19          |

---

## 4. Traceability Matrix

| Spec (behavioral.md)         | Verification Property | Test Type       |
|------------------------------|-----------------------|-----------------|
| INV-1 (motors off)           | VP-02                 | TLA+ proof      |
| INV-2 (battery sampling)     | VT-14                 | Ztest timing    |
| INV-3 (cutoff)               | VP-05                 | TLA+ proof      |
| INV-4 (IMU failure)          | VP-04                 | TLA+ proof      |
| INV-5 (watchdog)             | VP-16                 | TLA+ proof      |
| INV-6 (PID windup)           | VP-06, VP-07          | CBMC proof      |
| INV-7 (max DSHOT)            | VP-08                 | CBMC proof      |
| INV-8 (motor current limit)  | VP-12                 | TLA+ proof      |
| INV-9 (CRSF failsafe)       | VP-17                 | TLA+ proof      |
| INV-10 (blackbox non-blocking)| VT-15                | Ztest+HW        |
| EC-01 (CRSF CRC bad)        | VP-20                 | CBMC + Fuzz     |
| EC-02 (CRSF too short)       | VP-19                 | Fuzz            |
| EC-03 (CRSF sync wrong)      | VP-18                 | Fuzz            |
| EC-04 (ELRS link lost)       | VP-17                 | TLA+ proof      |
| EC-05 (ELRS link restored)   | VP-01                 | TLA+ proof      |
| EC-10 (rapid arm/disarm)     | VP-27                 | CBMC            |
| EC-11 (BLE checksum bad)     | VP-23                 | CBMC + Fuzz     |
| EC-12 (IMU wrong ID)         | VT-01                 | Ztest           |
| EC-13 (SPI bus error)        | VT-05                 | Ztest+HW        |
| EC-16 (calibration moving)   | VP-26                 | CBMC            |
| EC-17 (IMU failure Running)  | VP-04                 | TLA+ proof      |
| EC-18 (baro fail flight)     | VP-01                 | TLA+ proof      |
| EC-22 (critical battery)     | VP-05                 | TLA+ proof      |
| EC-25 (ADC hysteresis)       | VP-25                 | CBMC            |
| EC-31 (throttle spike)       | VP-08                 | CBMC            |
| EC-33 (excessive tilt)       | VP-02                 | TLA+ proof      |
| EC-34 (motor stall)          | VP-12                 | TLA+ proof      |
| EC-35 (motor overcurrent)    | VP-12                 | TLA+ proof      |
| EC-38 (arm invalid calib)    | VP-03, VP-32          | TLA+ proof      |
| EC-39 (arm low batt)         | VP-32                 | TLA+ proof      |
| EC-40 (arm no ELRS)          | VP-32                 | TLA+ proof      |
| EC-41 (arm throttle high)    | VP-32                 | TLA+ proof      |
| EC-42 (arm tilted)           | VP-32                 | TLA+ proof      |
| EC-45 (boot low batt)        | VP-05                 | TLA+ proof      |
| EC-47 (flash full)           | VT-15                 | Ztest           |
| Mixer output clamping        | VP-28, VP-29          | CBMC            |
| Channel mapping bounds       | VP-30                 | CBMC            |
| DSHOT encoding               | VP-33                 | CBMC            |
| Attitude filter bounded      | VP-34                 | CBMC            |
| Altitude fusion crossfade    | VP-31                 | CBMC            |
| Pre-flight check completeness| VP-32                 | TLA+ proof      |
