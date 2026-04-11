# Behavioral Specification (Gen 2)

## 1. Behavioral Contract

### 1.1 System Invariants

These conditions must hold true at all times regardless of mode.

| ID    | Invariant                                                        |
|-------|------------------------------------------------------------------|
| INV-1 | Motors DSHOT output = 0 (disarmed) unless mode is Running AND armed |
| INV-2 | Battery voltage is sampled at least once per 100 ms             |
| INV-3 | Per-cell voltage < 3.0V (cutoff) forces all motors off immediately (scaled by detected cell count) |
| INV-4 | IMU failure detected after 10 ms of no INT1 pulse; motors ramp to 0 over 200 ms, transition to Diag |
| INV-5 | Watchdog timer is refreshed every rate PID loop iteration        |
| INV-6 | PID integrator is clamped to prevent windup (both rate and angle loops) |
| INV-7 | Motor DSHOT throttle value never exceeds 2047                    |
| INV-8 | Motor current never exceeds 5 A per channel (software cutoff)   |
| INV-9 | CRSF failsafe activates if no valid frame received within 500 ms |
| INV-10| Blackbox logging does not block flight-critical loops (async DMA writes) |

### 1.2 Mode Preconditions and Postconditions

#### Calibrate Mode

| Aspect        | Contract                                                      |
|---------------|---------------------------------------------------------------|
| Precondition  | Drone is powered on; motors are disarmed                      |
| Precondition  | IMU responds to WHO_AM_I (0x75 = 0x47)                       |
| Precondition  | Barometer responds to CHIP_ID (0x00 = 0x60)                  |
| Invariant     | Motors remain disarmed for entire duration                    |
| Postcondition (success) | IMU offsets stored; baro ground reference stored; transition to Diag |
| Postcondition (fail)    | Error sent via BLE telemetry; remain in Calibrate; LED red blink |
| Postcondition (timeout) | 5 s timeout; calibration fails; report error |

#### Diag Mode

| Aspect        | Contract                                                      |
|---------------|---------------------------------------------------------------|
| Precondition  | Calibration completed successfully (offsets valid)            |
| Invariant     | Motors disarmed unless single-motor test active               |
| Invariant     | Motor test duration <= 500 ms, one motor at a time            |
| Postcondition (arm)     | All pre-flight checks pass; transition to Running   |
| Postcondition (recalib) | Clear stored offsets; transition to Calibrate        |

##### Pre-Flight Checks (must all pass before arming)

| Check                    | Condition                              |
|--------------------------|----------------------------------------|
| Calibration valid        | IMU offsets stored and non-zero         |
| Battery sufficient       | Per-cell voltage >= 3.3V (above critical)|
| Cells balanced           | max-min cell delta < 100 mV             |
| ELRS link active         | Valid CRSF frame within last 500 ms    |
| IMU healthy              | INT1 pulses within last 10 ms          |
| ESC RPM telemetry alive  | All 4 motors responding to bidir DSHOT (or backup current sense OK) |
| Throttle low             | CRSF CH3 < 200 (near minimum)          |
| Arm switch off then on   | CH5 must transition from disarm to arm |
| Drone level              | Tilt < 10 deg from horizontal          |

#### Running Mode

| Aspect        | Contract                                                      |
|---------------|---------------------------------------------------------------|
| Precondition  | All pre-flight checks passed                                  |
| Precondition  | Battery per-cell voltage >= 3.3V (above critical)             |
| Precondition  | ELRS link active                                              |
| Invariant     | Rate PID loop executes at 4 kHz (+/- 2%)                     |
| Invariant     | Angle PID loop executes at 1 kHz (+/- 5%)                    |
| Invariant     | CRSF stick center produces zero pitch/roll/yaw setpoint       |
| Invariant     | Throttle minimum produces motor idle (DSHOT 48); PID active   |
| Postcondition (disarm)  | Motors ramp to 0 over 200 ms; PID reset; → Diag  |

#### Error Mode

| Aspect        | Contract                                                      |
|---------------|---------------------------------------------------------------|
| Precondition  | Unrecoverable fault detected (IMU absent, flash corrupt, etc.)|
| Invariant     | Motors permanently disarmed                                   |
| Invariant     | LED red fast blink                                            |
| Postcondition | Watchdog timeout → MCU reset → retry startup                  |

### 1.3 Startup Sequence Contract

```
1.  Power on
2.  Start watchdog timer (IWDG, 100 ms timeout)
3.  Initialize GPIO (all DSHOT pins LOW)
4.  Initialize system clock (HSE 25 MHz → PLL1 → 480 MHz); configure D-Cache and I-Cache; configure MPU for DMA regions
5.  Initialize USB (CDC + DFU composite device)
6.  Check SW1 button state:
    - Held > 3 s → enter DFU mode (no further init)
    - Double-press → enter ELRS bind mode (set flag, continue)
7.  Initialize USART1 (CRSF @ 420000 baud)
8.  Initialize USART2 (BLE @ 115200 baud)
9.  Initialize SPI1 (IMU)
10. Initialize SPI2 (Flash + Optical Flow)
11. Initialize I2C1 (Baro + ToF + Charger)
12. Start battery ADC sampling (DMA, 10 Hz)
12a. Validate 3S battery voltage:
     - 9.0-13.1V → 3S valid, continue
     - < 9.0V → no battery or depleted; USB-only power; refuse arm
13. Verify IMU (WHO_AM_I check, up to 3 retries with 100 ms delay)
    - PASS → continue
    - FAIL → enter Error mode
14. Verify barometer (CHIP_ID check)
    - PASS → continue
    - FAIL → enter Error mode (baro required for safe flight)
15. Initialize VL53L5CX (firmware upload, ~300 ms)
    - FAIL → log warning; continue without ToF (altitude hold degraded)
16. Initialize PMW3901 (Product_ID check)
    - FAIL → log warning; continue without OF (position hold disabled)
17. Initialize flash (JEDEC ID check)
    - FAIL → log warning; continue without blackbox
18. Enter Calibrate mode
```

Note: Steps 15-17 are non-critical sensors. Failure degrades capability but
does not prevent flight. Steps 13-14 are mandatory.

### 1.4 Shutdown Contract

| Trigger              | Behavior                                          |
|----------------------|---------------------------------------------------|
| SW2 power off        | P-FET cuts power; no software action              |
| Watchdog timeout     | MCU reset → full startup sequence                 |
| Cell voltage < 3.0V  | Motors off immediately, remain in Diag            |
| USB DFU mode         | Flight disabled; USB active for firmware update   |

---

## 2. Interface Definition

### 2.1 Abstract Input Interface

The drone consumes input through an abstract interface, decoupled from any
specific transport. The primary backend is CRSF over ELRS; the secondary is
BLE for configuration commands.

#### Input Commands (Transport-Agnostic)

| Command            | Parameters                              | Valid In       |
|--------------------|-----------------------------------------|----------------|
| CMD_ARM            | --                                      | Diag           |
| CMD_DISARM         | --                                      | Running        |
| CMD_CALIBRATE      | --                                      | Diag           |
| CMD_STICK          | throttle, yaw, pitch, roll (int16 each) | Running        |
| CMD_SET_FLIGHT_MODE| mode (ACRO / ANGLE / ALT_HOLD)         | Running        |
| CMD_POSITION_HOLD  | enable (bool)                           | Running        |
| CMD_SET_PID_GAIN   | loop (RATE/ANGLE), axis (P/R/Y), gain (KP/KI/KD), value (float) | Any |
| CMD_MOTOR_TEST     | motor_id (0-3)                          | Diag           |
| CMD_FIND_ME        | enable (bool)                           | Diag           |
| CMD_BLACKBOX_DL    | offset, length                          | Diag           |
| CMD_BLACKBOX_ERASE | --                                      | Diag           |

#### Input Command Struct

```c
typedef enum {
    CMD_ARM,
    CMD_DISARM,
    CMD_CALIBRATE,
    CMD_STICK,
    CMD_SET_FLIGHT_MODE,
    CMD_POSITION_HOLD,
    CMD_SET_PID_GAIN,
    CMD_MOTOR_TEST,
    CMD_FIND_ME,
    CMD_BLACKBOX_DL,
    CMD_BLACKBOX_ERASE,
} cmd_type_t;

typedef enum {
    FLIGHT_MODE_ACRO,
    FLIGHT_MODE_ANGLE,
    FLIGHT_MODE_ALT_HOLD,
} flight_mode_t;

typedef enum {
    PID_LOOP_RATE,
    PID_LOOP_ANGLE,
} pid_loop_t;

typedef enum {
    PID_AXIS_PITCH,
    PID_AXIS_ROLL,
    PID_AXIS_YAW,
} pid_axis_t;

typedef enum {
    GAIN_KP,
    GAIN_KI,
    GAIN_KD,
} gain_type_t;

typedef struct {
    cmd_type_t type;
    union {
        struct { int16_t throttle; int16_t yaw;
                 int16_t pitch; int16_t roll; }          stick;
        struct { flight_mode_t mode; }                    flight_mode;
        struct { bool enable; }                           position_hold;
        struct { pid_loop_t loop; pid_axis_t axis;
                 gain_type_t gain; float value; }         pid_gain;
        struct { uint8_t motor_id; }                      motor_test;
        struct { bool enable; }                           find_me;
        struct { uint32_t offset; uint32_t length; }      blackbox_dl;
    };
} drone_cmd_t;
```

#### CRSF Backend Mapping

CRSF channels are mapped to `drone_cmd_t` as follows. For channel assignment
details, see [remote_controller.md](remote_controller.md#channel-assignment-edgetx-mixer).
For CRSF frame format and CRC, see [protocols.md](protocols.md).

| CRSF Input               | drone_cmd_t                                    |
|--------------------------|------------------------------------------------|
| CH1-CH4 (sticks)         | CMD_STICK { map(172..1811 → signed range) }    |
| CH5 (AUX1) low→high     | CMD_ARM                                        |
| CH5 (AUX1) high→low     | CMD_DISARM                                     |
| CH6 (AUX2) 3-pos        | CMD_SET_FLIGHT_MODE { ACRO / ANGLE / ALT_HOLD }|
| CH7 (AUX3)              | CMD_POSITION_HOLD { on/off }                   |
| CH8 (AUX4)              | CMD_FIND_ME { on/off }                         |

#### BLE Backend Mapping

BLE commands use the binary protocol defined in [protocols.md](protocols.md#ble-communication-protocol).
BLE handles configuration commands only (not flight-critical):

| BLE CMD  | drone_cmd_t                                        |
|----------|----------------------------------------------------|
| 0x10     | CMD_SET_PID_GAIN { loop, axis, gain, value }       |
| 0x11     | CMD_BLACKBOX_DL { offset, length }                 |
| 0x12     | CMD_BLACKBOX_ERASE                                 |
| 0x13     | CMD_FIND_ME { enable }                             |

### 2.2 Abstract Output Interface (Telemetry)

| Telemetry            | Parameters                      | Rate       | Mode      |
|----------------------|---------------------------------|------------|-----------|
| TELEM_BATTERY        | voltage_mv (uint16)             | 1 Hz       | All       |
| TELEM_CALIB_RESULT   | ok (bool)                       | Once       | Calibrate |
| TELEM_IMU            | ax,ay,az,gx,gy,gz (int16)      | 10 Hz      | Diag      |
| TELEM_MODE           | mode (enum)                     | On change  | All       |
| TELEM_ERROR          | error_code (enum)               | On event   | All       |
| TELEM_LOW_BATT       | level (WARNING/CRITICAL)        | On change  | All       |
| TELEM_ALTITUDE       | altitude_cm (int32)             | 10 Hz      | Running   |
| TELEM_LINK_STATS     | rssi (int8), lq (uint8)        | 1 Hz       | All       |
| TELEM_MOTOR_CURRENT  | current_mA[4] (uint16)         | 10 Hz      | Running   |

Telemetry is sent via two backends simultaneously:
- **BLE**: all telemetry types (for phone app / configurator)
- **CRSF uplink**: battery voltage, attitude, flight mode (for TX OSD display)

### 2.3 Internal Interfaces

#### IMU Driver

| Function         | Input             | Output                           | Rate     |
|------------------|-------------------|----------------------------------|----------|
| imu_init()       | --                | status (OK / FAIL)               | Once     |
| imu_read()       | --                | ax,ay,az,gx,gy,gz (int16, raw)  | 8 kHz    |
| imu_calibrate()  | sample_count      | offsets[6] (int16)               | Once     |
| imu_health()     | --                | bool (INT1 received within 10 ms)| On demand|

#### Motor Driver

| Function           | Input                    | Output  | Constraint               |
|--------------------|--------------------------|---------|--------------------------|
| motor_set_dshot()  | channel (0-3), throttle (uint16) | -- | throttle <= 2047        |
| motor_all_stop()   | --                       | --      | Send DSHOT 0 to all      |
| motor_ramp_down()  | duration_ms              | --      | Async, non-blocking      |
| motor_arm_sequence()| --                      | --      | DSHOT arm handshake      |
| motor_get_rpm()    | ch(0-3)                  | uint16 RPM | From bidir DSHOT eRPM |

#### Battery Monitor (Pack + Per-Cell)

| Function              | Input  | Output                              | Rate    |
|-----------------------|--------|-------------------------------------|---------|
| battery_read_pack_mv()| --     | pack voltage in mV (uint16)        | 10 Hz   |
| battery_read_cell_mv()| cell(0-2)| cell voltage in mV (uint16)      | 10 Hz   |
| battery_level()       | --     | NORMAL/WARNING/CRITICAL/CUTOFF     | 10 Hz   |
| battery_imbalance_mv()| --     | max_cell - min_cell delta (uint16) | 10 Hz   |
| battery_cell_count()  | --     | 1, 2, or 3 (detected at boot)      | Once    |

#### Motor Health Monitor

| Function             | Input  | Output                  | Rate    |
|----------------------|--------|-------------------------|---------|
| motor_current_mA()   | ch(0-3)| current in mA (uint16) | 1 kHz   |
| motor_stall_check()  | --     | stall_detected (bool)   | 100 Hz (RPM-based primary, current backup) |

#### PID Controller (Cascaded)

Rate loop (inner): 3 independent instances (pitch_rate, roll_rate, yaw_rate)
Angle loop (outer): 2 independent instances (pitch_angle, roll_angle)

| Function              | Input                          | Output              |
|-----------------------|--------------------------------|----------------------|
| pid_rate_update()     | setpoint, gyro_rate, dt        | control output (float) |
| pid_angle_update()    | setpoint, estimated_angle, dt  | rate setpoint (float) |
| pid_set_gains()       | loop, axis, Kp, Ki, Kd        | --                   |
| pid_reset()           | --                             | Clear integrator + derivative state |

#### Attitude Estimator

| Function              | Input                          | Output              |
|-----------------------|--------------------------------|----------------------|
| attitude_update()     | accel[3], gyro[3], dt          | pitch, roll, yaw (float, deg) |

Uses complementary filter (alpha = 0.98 gyro, 0.02 accel).
Future: upgrade to Madgwick or Mahony AHRS.

#### Altitude Estimator

| Function              | Input                          | Output              |
|-----------------------|--------------------------------|----------------------|
| altitude_update()     | baro_pa, tof_mm, dt            | altitude_cm (int32)  |

Fusion: ToF used below 2 m (high accuracy); barometer used above 2 m.
Crossfade between 1.5 m and 2.5 m.

#### Position Estimator

| Function              | Input                          | Output              |
|-----------------------|--------------------------------|----------------------|
| position_update()     | flow_dx, flow_dy, altitude, dt | velocity_x, velocity_y (float, cm/s) |

Optical flow delta scaled by altitude for metric velocity.

#### Motor Mixer

Converts throttle + PID outputs into four DSHOT throttle values.

```
        Front
    FL(CCW) --- FR(CW)
    |               |
    BL(CW)  --- BR(CCW)
         Back
```

| Motor | Ref | Rotation | Equation                                    |
|-------|-----|----------|---------------------------------------------|
| FR    | M1  | CW       | throttle + pitch_out + roll_out - yaw_out   |
| FL    | M2  | CCW      | throttle + pitch_out - roll_out + yaw_out   |
| BR    | M3  | CCW      | throttle - pitch_out + roll_out + yaw_out   |
| BL    | M4  | CW       | throttle - pitch_out - roll_out - yaw_out   |

Output clamping: each motor value clamped to [0, 2047].
Negative values after mixing are set to 0.

### 2.4 Command Acceptance Matrix

Commands received in an invalid mode are silently ignored.

| Command              | Calibrate | Diag | Running | Error |
|----------------------|-----------|------|---------|-------|
| CMD_ARM              | ignore    | accept | ignore | ignore |
| CMD_DISARM           | ignore    | ignore | accept | ignore |
| CMD_CALIBRATE        | ignore    | accept | ignore | ignore |
| CMD_STICK            | ignore    | ignore | accept | ignore |
| CMD_SET_FLIGHT_MODE  | ignore    | ignore | accept | ignore |
| CMD_POSITION_HOLD    | ignore    | ignore | accept | ignore |
| CMD_SET_PID_GAIN     | accept    | accept | accept | ignore |
| CMD_MOTOR_TEST       | ignore    | accept | ignore | ignore |
| CMD_FIND_ME          | ignore    | accept | ignore | ignore |
| CMD_BLACKBOX_DL      | ignore    | accept | ignore | ignore |
| CMD_BLACKBOX_ERASE   | ignore    | accept | ignore | ignore |

---

## 3. Edge Case Catalog

### 3.1 Communication Edge Cases

| ID    | Scenario                                    | Expected Behavior                                    |
|-------|---------------------------------------------|------------------------------------------------------|
| EC-01 | CRSF frame CRC mismatch                    | Frame discarded; use last valid channel data          |
| EC-02 | CRSF frame too short (< 4 bytes)           | Frame discarded                                       |
| EC-03 | CRSF sync byte wrong                       | Resync: scan for next 0xC8                           |
| EC-04 | CRSF link lost during Running              | Failsafe after 500 ms; ramp motors down; → Diag      |
| EC-05 | CRSF link restored after failsafe          | Remain in Diag; user must re-arm                     |
| EC-06 | ELRS packet rate change mid-flight         | Transparent: CRSF parser handles variable rates      |
| EC-07 | BLE disconnect during Running              | No flight action (BLE not flight-critical)           |
| EC-08 | BLE and CRSF send conflicting commands     | CRSF has priority for flight commands; BLE for config only |
| EC-09 | UART receive buffer overflow (CRSF)        | DMA circular buffer; oldest data overwritten; parser resyncs |
| EC-10 | Rapid arm/disarm toggle                    | 1 s cooldown between arm attempts                    |
| EC-11 | BLE protocol checksum mismatch             | Frame discarded; NACK sent                           |

### 3.2 Sensor Edge Cases

| ID    | Scenario                                    | Expected Behavior                                    |
|-------|---------------------------------------------|------------------------------------------------------|
| EC-12 | IMU WHO_AM_I returns wrong ID at boot       | Enter Error mode; LED red blink                      |
| EC-13 | IMU SPI bus error (MISO stuck)              | SPI timeout (5 ms); retry 3x; if fail: failsafe     |
| EC-14 | IMU data spike (single-sample outlier)      | Handled by digital filter (ICM-42688 AAF + firmware LPF) |
| EC-15 | Calibration with drone not level            | Offsets will be wrong; user must recalibrate         |
| EC-16 | Calibration with drone moving               | Variance check; stddev > threshold → fail            |
| EC-17 | IMU failure during Running                  | Detected after 10 ms no INT1; ramp motors down; → Diag |
| EC-18 | Barometer fails during flight               | Altitude hold disabled; degrade to angle/acro mode; warn user |
| EC-19 | ToF out of range (> 4 m)                   | ToF data marked invalid; altitude from baro only     |
| EC-20 | Optical flow over featureless surface        | SQUAL < 20; position hold disabled; warn user        |
| EC-21 | Optical flow fails completely                | Position hold disabled; altitude hold still works    |

### 3.3 Power Edge Cases

| ID    | Scenario                                    | Expected Behavior                                    |
|-------|---------------------------------------------|------------------------------------------------------|
| EC-22 | Battery voltage drops below 3.3V (critical) | LED red blink; buzzer rapid; auto-disarm             |
| EC-23 | Battery voltage drops below 3.0V (cutoff)   | Motors off immediately; remain in Diag               |
| EC-24 | Battery voltage recovers above threshold     | No auto-transition; user must re-arm manually        |
| EC-25 | ADC noise near threshold                     | Hysteresis: 100 mV gap between enter and exit       |
| EC-26 | Battery disconnected while running           | Hardware power loss; no software action possible     |
| EC-27 | USB connected during flight                  | USB powers core module only; flight unaffected      |
| EC-28 | USB VBUS without battery                    | Core module powered from USB; motors disabled (no battery = cannot arm) |
| EC-28a| Battery voltage < 6.0V at boot              | Treated as no battery or critically depleted; refuse arm |
| EC-28b| Cell imbalance > 100 mV at boot              | Refuse arm; warn user; recommend rebalance via external charger |
| EC-28c| Cell imbalance > 200 mV in flight            | Critical: ramp motors to 0; → Diag; log event |
| EC-28d| Single cell drops below 3.0V (cutoff)        | Motors off immediately; → Diag (even if pack avg > cutoff) |
| EC-28e| Balance connector unplugged (only main connected) | Cell readings = 0 or noise; firmware detects and disables cell-based protection; falls back to pack-only monitoring with warning |

### 3.4 Motor Edge Cases

| ID    | Scenario                                    | Expected Behavior                                    |
|-------|---------------------------------------------|------------------------------------------------------|
| EC-29 | Motor test held in Diag                     | Single pulse 500 ms max; ignore until command released |
| EC-30 | Multiple motor test simultaneous             | Only first received honored; others ignored          |
| EC-31 | Full throttle immediately after arm          | Throttle must be low to arm (pre-flight check); after arm, rate limiter: max 100 DSHOT/tick |
| EC-32 | Throttle zero while armed                   | Motors at idle (DSHOT 48); PID stabilization active  |
| EC-33 | Excessive tilt (> 60 deg)                   | Motors off immediately; → Diag                       |
| EC-34 | Motor stall (eRPM = 0 with throttle > 200) | Disarm all motors; → Diag; report error              |
| EC-35 | Motor overcurrent (> 5 A)                   | Affected motor DSHOT = 0; disarm all; → Diag         |
| EC-36 | ESC desync (eRPM > 30% off expected)       | BlueJay handles internally; FC logs event; if persistent > 200 ms, warn user |
| EC-37 | Prop strike / sudden load                    | Detected via RPM drop + current spike; if persistent, disarm |
| EC-37a| Bidirectional DSHOT response timeout        | Mark motor as no-telemetry; fall back to current sensing for stall detection |
| EC-37b| GCR decode CRC error                        | Discard reading; use last valid eRPM; counter triggers fallback if persistent |
| EC-37c| All 4 motors lose RPM telemetry             | ESC firmware fault: disarm; → Diag; report error    |

### 3.5 Mode Transition Edge Cases

| ID    | Scenario                                    | Expected Behavior                                    |
|-------|---------------------------------------------|------------------------------------------------------|
| EC-38 | Arm with invalid calibration                 | Reject arm; remain in Diag; send error               |
| EC-39 | Arm with low battery (< 3.3V per cell)     | Reject arm; remain in Diag                           |
| EC-39a| Arm with cell imbalance > 100 mV            | Reject arm; remain in Diag; suggest recharge        |
| EC-40 | Arm with no ELRS link                        | Reject arm; remain in Diag                           |
| EC-41 | Arm with throttle not at minimum             | Reject arm; remain in Diag                           |
| EC-42 | Arm with drone tilted > 10 deg               | Reject arm; remain in Diag                           |
| EC-43 | Calibrate during Running                     | Ignore command                                       |
| EC-44 | DFU mode requested during Running            | Ignore (DFU only at boot via SW1)                    |
| EC-45 | Power on with critical battery (< 3.3V)     | Complete startup; enter Diag; refuse arm             |
| EC-46 | Flight mode switch during aggressive maneuver| Smooth transition: angle/rate setpoints interpolated over 100 ms |

### 3.6 Blackbox / Flash Edge Cases

| ID    | Scenario                                    | Expected Behavior                                    |
|-------|---------------------------------------------|------------------------------------------------------|
| EC-47 | Flash full during flight                     | Circular buffer: overwrite oldest sector             |
| EC-48 | Flash write error                            | Retry once; if fail, disable logging; flight continues |
| EC-49 | Flash erase during flight                    | Reject: erase only allowed in Diag mode             |
| EC-50 | BLE blackbox download during flight          | Reject: download only in Diag mode                  |
