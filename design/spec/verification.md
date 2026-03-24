# Verification Architecture (VSDD Phase 1b)

## 1. Provable Properties Catalog

Properties are classified as **formally provable** (P) or **test-only** (T) based
on whether the logic is deterministic and side-effect-free.

### 1.1 Safety Properties (Must Prove)

| ID    | Property                                              | Class | Tool   |
|-------|-------------------------------------------------------|-------|--------|
| VP-01 | State machine never reaches an undefined state        | P     | TLA+   |
| VP-02 | Motors PWM = 0 in any state other than Running/armed  | P     | TLA+   |
| VP-03 | Calibrate → Diag only on success; never Calibrate → Running | P | TLA+ |
| VP-04 | Disarm from Running always reaches Diag within finite steps | P | TLA+ |
| VP-05 | Battery cutoff (< 6.0V) forces motors off in all states | P  | TLA+   |
| VP-06 | PID integrator absolute value <= WINDUP_MAX at all times | P | CBMC  |
| VP-07 | Motor PWM duty <= MAX_THROTTLE for all inputs         | P     | CBMC   |
| VP-08 | No arithmetic overflow in PID calculation (int16 inputs, float intermediates) | P | CBMC |
| VP-09 | Joystick center (512,512) produces exactly zero setpoint | P | CBMC |
| VP-10 | Voltage divider ADC-to-mV conversion never overflows uint16 | P | CBMC |

### 1.2 Liveness Properties (Must Prove)

| ID    | Property                                              | Class | Tool   |
|-------|-------------------------------------------------------|-------|--------|
| VP-11 | System always eventually exits Calibrate (success or timeout) | P | TLA+ |
| VP-12 | Motor ramp-down completes within duration_ms          | P     | TLA+   |
| VP-13 | Watchdog is refreshed at least once per PID period    | P     | TLA+   |

### 1.3 Protocol Properties (Must Prove)

| ID    | Property                                              | Class | Tool   |
|-------|-------------------------------------------------------|-------|--------|
| VP-14 | Parser always terminates (no infinite loop on any input) | P  | CBMC   |
| VP-15 | Parser output is empty OR has both valid id and value | P     | CBMC   |
| VP-16 | Parser discards any byte sequence that lacks SOH prefix | P  | CBMC   |
| VP-17 | Parser buffer never reads/writes out of bounds        | P     | CBMC   |

### 1.4 Additional Provable Properties (Reclassified from Test-Only)

| ID    | Property                                              | Class | Tool   |
|-------|-------------------------------------------------------|-------|--------|
| VP-18 | Battery level hysteresis: warning at 7.0V, exit at 7.2V; critical at 6.6V, exit at 6.8V | P | CBMC |
| VP-19 | Calibration variance check rejects samples with stddev > threshold | P | CBMC |
| VP-20 | Arm debounce rejects re-arm when elapsed_ms < 1000 (timestamp is pure input) | P | CBMC |
| VP-21 | Motor mixer output clamped to [0, MAX_THROTTLE] for all inputs | P | CBMC |
| VP-22 | Motor mixer never produces negative duty values       | P     | CBMC   |
| VP-23 | Slider-to-gain mapping stays within defined ranges (Kp: 0-5, Ki: 0-2, Kd: 0-1) | P | CBMC |

### 1.5 Testable-Only Properties

| ID    | Property                                              | Class | Tool       |
|-------|-------------------------------------------------------|-------|------------|
| VT-01 | IMU WHO_AM_I returns 0x68 on valid hardware           | T     | Ztest      |
| VT-02 | I2C bus recovery succeeds after SDA stuck low         | T     | Ztest+HW   |
| VT-03 | Bluetooth disconnect detected within 500 ms           | T     | Ztest      |
| VT-04 | LED patterns match mode (visual / GPIO check)         | T     | Ztest      |
| VT-05 | PID loop timing meets 500 Hz +/- 5% on target HW     | T     | Ztest+HW   |
| VT-06 | Motor test pulse duration is 500 ms +/- 10%           | T     | Ztest+HW   |
| VT-07 | Telemetry messages arrive at specified rates           | T     | Ztest      |

---

## 2. Purity Boundary Map

The architecture is split into a **deterministic pure core** (formally verifiable)
and an **effectful shell** (tested only).

```
+------------------------------------------------------------------+
|                        Effectful Shell                            |
|                                                                  |
|  +------------------+  +---------------+  +------------------+   |
|  | Input Backend    |  | I2C Driver    |  | GPIO / PWM HAL   |   |
|  | (MicroBlue, RC,  |  | (MPU6050)     |  | (Motors, LEDs)   |   |
|  |  USB, etc.)      |  |               |  |                  |   |
|  +--------+---------+  +-------+-------+  +--------+---------+   |
|           |                     |                   |             |
|           v                     v                   v             |
|  +--------+---------+  +-------+-------+  +--------+---------+   |
|  | Backend Parser   |  | IMU Reader    |  | Motor Writer     |   |
|  | (transport →     |  | (effectful)   |  | (effectful)      |   |
|  |  drone_cmd_t)    |  |               |  |                  |   |
|  +--------+---------+  +-------+-------+  +--------+---------+   |
|           |                     |                   ^             |
+-----------+---------------------+-------------------+-------------+
            |                     |                   |
            v                     v                   |
+------------------------------------------------------------------+
|                        Pure Core                                 |
|                                                                  |
|  +------------------+  +---------------+  +------------------+   |
|  | Command Dispatch |  | PID Controller|  | State Machine    |   |
|  | (drone_cmd_t,    |  | (setpoint,    |  | (event,state →   |   |
|  |  mode → event)   |  |  measured,dt  |  |  next_state,     |   |
|  |                  |  |  → output)    |  |  actions[])      |   |
|  +------------------+  +---------------+  +------------------+   |
|                                                                  |
|  +------------------+  +---------------+  +------------------+   |
|  | Input Mapper     |  | Battery Level |  | Calibration Math |   |
|  | (joystick raw →  |  | (mV → enum   |  | (samples[] →     |   |
|  |  setpoints)      |  |  w/ hysteresis)|  |  offsets[])      |   |
|  +------------------+  +---------------+  +------------------+   |
|                                                                  |
|  +------------------+  +------------------+                      |
|  | Motor Mixer      |  | Gain Mapper      |                      |
|  | (throttle,PID →  |  | (value 0-100 →   |                      |
|  |  duty[4])        |  |  float gain)     |                      |
|  +------------------+  +------------------+                      |
|                                                                  |
+------------------------------------------------------------------+
```

Note: Protocol-specific parsers (MicroBlue, etc.) sit in the effectful shell.
They translate transport bytes into `drone_cmd_t` (defined in `command.h`).
The pure core only sees `drone_cmd_t` — it never touches raw bytes or I/O.

### 2.1 Pure Core Modules

These modules are deterministic, take inputs, return outputs, and have no
side effects. They are the primary targets for formal verification and
property-based testing.

| Module              | Inputs                        | Outputs                      |
|---------------------|-------------------------------|------------------------------|
| **State Machine**   | current_state, event          | next_state, action_list      |
| **Command Dispatch**| drone_cmd_t, current_mode     | drone_event_t or IGNORE      |
| **PID Controller**  | setpoint, measured, dt, gains | control_output (float)       |
| **Input Mapper**    | joystick x, y (int16)         | signed setpoint (-512..+511) |
| **Battery Level**   | voltage_mv, prev_level        | level_enum (with hysteresis) |
| **Calibration Math**| raw_samples[N][6]             | offsets[6], variance_ok (bool) |
| **Motor Mixer**     | throttle, pitch, roll, yaw    | duty[4] (uint16, clamped)    |
| **Gain Mapper**     | value (uint8, 0-100)          | gain (float, bounded)        |

### 2.2 Effectful Shell Modules

These modules perform I/O and are tested via integration tests on target hardware.

| Module                  | Side Effects                              |
|-------------------------|-------------------------------------------|
| **Input Backend**       | Read from transport (UART, SPI, USB, etc); convert to drone_cmd_t |
| **MicroBlue Backend**   | Input backend: UART → mb_parse → drone_cmd_t |
| **Telemetry Backend**   | Serialize telemetry structs to transport format |
| **I2C Driver**          | Read/write registers on MPU6050            |
| **PWM HAL**             | Set timer compare registers for motors     |
| **GPIO HAL**            | Set LED pin states                         |
| **ADC HAL**             | Read battery voltage analog input          |
| **Watchdog HAL**        | Refresh hardware watchdog timer            |
| **Main Loop**           | Orchestrates shell ↔ core data flow        |

### 2.3 Boundary Rules

1. Pure core modules MUST NOT include any Zephyr API calls (no `k_*`, no device bindings)
2. Pure core modules accept C primitive types and structs, never device pointers
3. All hardware interaction flows through the effectful shell
4. The shell calls pure core functions and acts on the returned action_list
5. Pure core modules are compilable and testable on the host (x86) without Zephyr

---

## 3. Verification Tooling

| Tool     | Target                  | Purpose                              |
|----------|-------------------------|--------------------------------------|
| **TLA+** | State machine model     | Prove safety & liveness of mode transitions, motor invariants |
| **CBMC** | C source (pure core)    | Bounded model checking: overflow, array bounds, termination |
| **Ztest**| Full firmware on target | Zephyr-native unit and integration tests |
| **Ztest (native_sim)** | Pure core on host | Fast iteration without hardware |
| **Fuzz** | MicroBlue parser        | libFuzzer / AFL++ on parser in isolation |

### 3.1 TLA+ Scope

Model the state machine as a TLA+ specification covering:
- States: {Calibrate, Diag, Running}
  - Note: Startup boot sequence is modeled as precondition to Calibrate.
    IMU failure at boot causes watchdog reset (no stable Error state in firmware).
- Events: {calib_ok, calib_fail, calib_timeout, arm, disarm,
           bt_disconnect, low_batt_critical, low_batt_cutoff, imu_failure,
           excessive_tilt, sw2_press, recalibrate}
- Properties to check: VP-01 through VP-05, VP-11 through VP-13

### 3.2 CBMC Scope

Bounded model check the following C functions:
- `microblue_parse()` — VP-14 through VP-17
- `pid_update()` — VP-06, VP-08
- `input_map_joystick()` — VP-09
- `motor_set_pwm()` — VP-07
- `adc_to_mv()` — VP-10

CBMC harnesses will use `__CPROVER_assume()` for preconditions and
`__CPROVER_assert()` for postconditions on all inputs within type range.

### 3.3 Fuzz Testing Scope

The MicroBlue parser is the primary external input boundary.
Fuzz with arbitrary byte sequences to verify:
- No buffer overflows (VP-17)
- Always terminates (VP-14)
- Output is well-formed or empty (VP-15)

Build parser as a standalone library (pure core) and link with libFuzzer.

---

## 4. Traceability Matrix

Every spec requirement traces to a verification property and test.

| Spec (behavioral.md)       | Verification Property | Test Type     |
|----------------------------|-----------------------|---------------|
| INV-1 (motors off)         | VP-02                 | TLA+ proof    |
| INV-2 (battery sampling)   | VT-07                 | Ztest timing  |
| INV-3 (cutoff)             | VP-05                 | TLA+ proof    |
| INV-4 (IMU failure)        | VP-04                 | TLA+ proof    |
| INV-5 (watchdog)           | VP-13                 | TLA+ proof    |
| INV-6 (PID windup)         | VP-06                 | CBMC proof    |
| INV-7 (max throttle)       | VP-07                 | CBMC proof    |
| EC-01 (malformed packet)   | VP-14, VP-16          | Fuzz          |
| EC-02 (partial packet)     | VP-15                 | Fuzz          |
| EC-05 (bad joystick value) | VP-09                 | CBMC + Ztest  |
| EC-06 (BT disconnect)      | VP-04                 | TLA+ proof    |
| EC-08 (rapid arm/disarm)   | VT-10                 | Ztest         |
| EC-09 (buffer overflow)    | VP-17                 | Fuzz          |
| EC-16 (calibration moving) | VT-09                 | Ztest         |
| EC-18 (critical battery)   | VP-05                 | TLA+ proof    |
| EC-21 (ADC hysteresis)     | VT-08                 | Ztest         |
| EC-26 (throttle ramp)      | VP-07                 | CBMC + Ztest  |
| EC-28 (excessive tilt)     | VP-02                 | TLA+ proof    |
| EC-31 (arm invalid offsets) | VP-03                | TLA+ proof    |
| EC-36 (calibration timeout)| VP-11                 | TLA+ proof    |
| EC-03 (unknown widget ID)  | VP-15                 | Fuzz + CBMC   |
| EC-04 (empty value)        | VP-15                 | CBMC          |
| EC-07 (BT reconnect)      | VP-01                 | TLA+ proof    |
| EC-10 (slider out of range)| VP-23                 | CBMC          |
| EC-13 (stale IMU data)     | VT-03                 | Ztest         |
| EC-15 (calibration not level)| Accepted risk       | —             |
| EC-20 (battery recovery)   | VP-18                 | CBMC          |
| EC-22 (battery disconnected)| Accepted risk        | — (hardware)  |
| EC-24 (motor test held)    | VT-06                 | Ztest         |
| EC-25 (multiple motor tests)| VP-01                | TLA+ proof    |
| EC-29 (single motor failure)| Accepted risk        | — (no current sensing) |
| EC-33 (power on low batt)  | VP-05                 | TLA+ proof    |
| EC-34 (SW2 press Running)  | VP-04                 | TLA+ proof    |
| Mixer output clamping      | VP-21, VP-22          | CBMC          |
| Gain mapping bounds        | VP-23                 | CBMC          |
