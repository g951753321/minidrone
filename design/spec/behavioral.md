# Behavioral Specification

## 1. Behavioral Contract

### 1.1 System Invariants

These conditions must hold true at all times regardless of mode.

| ID   | Invariant                                                        |
|------|------------------------------------------------------------------|
| INV-1 | Motors PWM output = 0 unless mode is Running AND armed          |
| INV-2 | Battery voltage is sampled at least once per 100 ms             |
| INV-3 | Pack voltage < 6.0V (cutoff) forces all motors off immediately |
| INV-4 | IMU read failure detected after 50 ms of no valid data; once detected, ramp motors to 0 over 200 ms and transition to Diag |
| INV-5 | Watchdog timer is refreshed every PID loop iteration            |
| INV-6 | PID integrator is clamped to prevent windup                     |
| INV-7 | Motor PWM duty cycle never exceeds configured max throttle      |

### 1.2 Mode Preconditions and Postconditions

#### Calibrate Mode

| Aspect        | Contract                                                      |
|---------------|---------------------------------------------------------------|
| Precondition  | Drone is powered on; motors are off                           |
| Precondition  | IMU responds to WHO_AM_I (0x75 = 0x68)                       |
| Invariant     | Motors remain off for entire duration                         |
| Postcondition (success) | Gyro and accel offsets stored; transition to Diag   |
| Postcondition (fail)    | Error sent via BT; remain in Calibrate; STAT_0 fast blink |

#### Diag Mode

| Aspect        | Contract                                                      |
|---------------|---------------------------------------------------------------|
| Precondition  | Calibration completed successfully (offsets valid)            |
| Invariant     | Motors off unless single-motor test pulse active              |
| Invariant     | Motor test pulse duration <= 500 ms, one motor at a time      |
| Postcondition (arm)     | All pre-flight checks pass; transition to Running   |
| Postcondition (recalib) | Clear stored offsets; transition to Calibrate        |

#### Running Mode

| Aspect        | Contract                                                      |
|---------------|---------------------------------------------------------------|
| Precondition  | Calibration offsets valid                                     |
| Precondition  | Battery voltage >= 6.6V (above critical threshold)           |
| Precondition  | Bluetooth connected                                           |
| Invariant     | PID loop executes at 500 Hz (+/- 5%)                         |
| Invariant     | Joystick center (512,512) produces zero pitch/roll/yaw       |
| Invariant     | Throttle = 0 produces MOTOR_IDLE_DUTY (50 out of 65535); PID stabilization remains active at idle |
| Postcondition (disarm)  | Motors ramp to 0 over 200 ms; PID reset; transition to Diag |

### 1.3 Startup Sequence Contract

```
1. Power on
2. Start watchdog timer (first action, ensures recovery from any hang)
3. Initialize GPIO (all motor pins LOW)
4. Initialize UART (BT @ 115200)
5. Initialize I2C
6. Start battery ADC sampling
7. Verify IMU (WHO_AM_I check, up to 3 retries with 100 ms delay)
   - PASS → enter Calibrate mode
   - FAIL → enter Error state: STAT_0 fast blink, queue error for BT
           (error sent when BT connects; watchdog resets after timeout)
```

Note: Watchdog is started at step 2 so that any hang (including IMU failure)
will result in an automatic MCU reset rather than an unrecoverable halt.

### 1.4 Shutdown Contract

| Trigger              | Behavior                                          |
|----------------------|---------------------------------------------------|
| SW3 power off        | Hardware cutoff, no software action               |
| Watchdog timeout     | MCU reset → full startup sequence                 |
| Pack voltage < 6.0V  | Motors off immediately, remain in Diag            |

---

## 2. Interface Definition

### 2.1 Abstract Input Interface

The drone consumes input through an abstract interface, decoupled from any
specific transport (Bluetooth, UART, USB, RC receiver, etc.). The current
default backend is MicroBlue over HM-13 BLE; see `protocols.md` for
transport-specific framing details.

#### Input Commands (Transport-Agnostic)

Any input source must be able to produce the following commands:

| Command            | Parameters                       | Valid In       |
|--------------------|----------------------------------|----------------|
| CMD_ARM            | —                                | Diag           |
| CMD_DISARM         | —                                | Running        |
| CMD_CALIBRATE      | —                                | Diag           |
| CMD_JOYSTICK       | axis (0=throttle/yaw, 1=pitch/roll), x (int16), y (int16) | Running |
| CMD_SET_PID_GAIN   | gain_type (KP/KI/KD), value (uint8, 0-100) | Running |
| CMD_MOTOR_TEST     | motor_id (0-3)                   | Diag           |

#### Input Command Struct

```c
typedef enum {
    CMD_ARM,
    CMD_DISARM,
    CMD_CALIBRATE,
    CMD_JOYSTICK,
    CMD_SET_PID_GAIN,
    CMD_MOTOR_TEST,
} cmd_type_t;

typedef enum {
    GAIN_KP,
    GAIN_KI,
    GAIN_KD,
} gain_type_t;

typedef struct {
    cmd_type_t type;
    union {
        struct { uint8_t axis; int16_t x; int16_t y; } joystick;
        struct { gain_type_t gain; uint8_t value; }     pid_gain;
        struct { uint8_t motor_id; }                    motor_test;
    };
} drone_cmd_t;
```

#### Input Backend Interface

Each input backend (MicroBlue, future RC, etc.) implements a translator that
converts transport-specific messages into `drone_cmd_t`:

| Function              | Input                    | Output        |
|-----------------------|--------------------------|---------------|
| backend_init()        | config (baud, pins, etc) | status        |
| backend_poll()        | —                        | drone_cmd_t or NONE |

The MicroBlue backend maps widget IDs to commands as defined in `protocols.md`:

| MicroBlue Widget | drone_cmd_t                                      |
|------------------|--------------------------------------------------|
| `b0` = `"1"`     | CMD_ARM                                          |
| `b0` = `"0"`     | CMD_DISARM                                       |
| `b1` = `"1"`     | CMD_CALIBRATE                                    |
| `j0` = `"X,Y"`   | CMD_JOYSTICK { axis=0, x=X-512, y=Y-512 }       |
| `j1` = `"X,Y"`   | CMD_JOYSTICK { axis=1, x=X-512, y=Y-512 }       |
| `sl0` = `"V"`    | CMD_SET_PID_GAIN { gain=KP, value=V }            |
| `sl1` = `"V"`    | CMD_SET_PID_GAIN { gain=KI, value=V }            |
| `sl2` = `"V"`    | CMD_SET_PID_GAIN { gain=KD, value=V }            |
| `b2` = `"1"`     | CMD_MOTOR_TEST { motor_id=0 (FR) }               |
| `b3` = `"1"`     | CMD_MOTOR_TEST { motor_id=1 (FL) }               |
| `b4` = `"1"`     | CMD_MOTOR_TEST { motor_id=2 (BR) }               |
| `b5` = `"1"`     | CMD_MOTOR_TEST { motor_id=3 (BL) }               |

### 2.2 Abstract Output Interface (Telemetry)

Telemetry is sent through a backend-agnostic output interface. Each backend
serializes the telemetry struct into its transport format.

| Telemetry            | Parameters                      | Rate       | Mode      |
|----------------------|---------------------------------|------------|-----------|
| TELEM_BATTERY        | voltage_mv (uint16)             | 1 Hz       | All       |
| TELEM_CALIB_RESULT   | ok (bool)                       | Once       | Calibrate |
| TELEM_IMU            | ax,ay,az,gx,gy,gz (int16)      | 10 Hz      | Diag      |
| TELEM_MODE           | mode (enum)                     | On change  | All       |
| TELEM_ERROR          | error_code (enum)               | On event   | All       |
| TELEM_LOW_BATT       | level (WARNING/CRITICAL)        | On change  | All       |

#### Telemetry Backend Interface

| Function              | Input            | Output  |
|-----------------------|------------------|---------|
| telem_send_battery()  | voltage_mv       | —       |
| telem_send_calib()    | ok               | —       |
| telem_send_imu()      | imu_data[6]      | —       |
| telem_send_mode()     | mode             | —       |
| telem_send_error()    | error_code       | —       |
| telem_send_lowbatt()  | level            | —       |

The MicroBlue backend serializes these using `[SOH] ID [STX] VALUE [ETX]`
framing as defined in `protocols.md`.

### 2.3 Internal Interfaces

#### IMU Driver

| Function         | Input             | Output                       | Rate     |
|------------------|-------------------|------------------------------|----------|
| imu_init()       | —                 | status (OK / FAIL)           | Once     |
| imu_read()       | —                 | ax, ay, az, gx, gy, gz (int16) | 1 kHz |
| imu_calibrate()  | sample_count      | offsets[6] (int16)           | Once     |

#### Motor Driver

| Function           | Input                    | Output  | Constraint           |
|--------------------|--------------------------|---------|----------------------|
| motor_set_pwm()    | channel (0-3), duty (uint16) | —   | duty <= MAX_THROTTLE |
| motor_all_off()    | —                        | —       | Immediate, no ramp   |
| motor_ramp_down()  | duration_ms              | —       | Async, non-blocking  |

#### Battery Monitor

| Function           | Input  | Output              | Rate    |
|--------------------|--------|----------------------|---------|
| battery_read_mv()  | —      | voltage in mV (uint16) | 5 Hz |
| battery_level()    | —      | NORMAL / WARNING / CRITICAL / CUTOFF | 5 Hz |

#### PID Controller

Three independent PID instances: pitch, roll, yaw. All share the same gains
(tuned via sliders) since the airframe is symmetric.

| Function           | Input                          | Output              |
|--------------------|--------------------------------|----------------------|
| pid_update()       | setpoint, measured, dt         | control output (float) |
| pid_set_gains()    | Kp, Ki, Kd (float)            | —                    |
| pid_reset()        | —                              | Clear integrator + derivative state |

PID instances: `pid_pitch`, `pid_roll`, `pid_yaw`.

Gain mapping (from CMD_SET_PID_GAIN value, 0-100, linear):

| gain_type | Formula               | Range       |
|-----------|-----------------------|-------------|
| KP        | Kp = value * 0.05     | 0.0 - 5.0   |
| KI        | Ki = value * 0.02     | 0.0 - 2.0   |
| KD        | Kd = value * 0.01     | 0.0 - 1.0   |

#### Motor Mixer

Converts throttle + PID outputs into four motor duty values.
Motor rotation directions follow standard X-quad convention.

```
        Front
    FL(CCW) --- FR(CW)
    |               |
    BL(CW)  --- BR(CCW)
         Back
```

Mixing equations (all values clamped to [0, MAX_THROTTLE]):

| Motor | Ref | Rotation | Equation                                    |
|-------|-----|----------|---------------------------------------------|
| FR    | M3  | CW       | throttle + pitch_out + roll_out - yaw_out   |
| FL    | M4  | CCW      | throttle + pitch_out - roll_out + yaw_out   |
| BR    | M1  | CCW      | throttle - pitch_out + roll_out + yaw_out   |
| BL    | M2  | CW       | throttle - pitch_out - roll_out - yaw_out   |

Where:
- `throttle` = joystick j0 Y axis mapped from [0,1023] to [MOTOR_IDLE_DUTY, MAX_THROTTLE]
- `pitch_out` = pid_pitch.update(pitch_setpoint, imu_pitch, dt)
- `roll_out` = pid_roll.update(roll_setpoint, imu_roll, dt)
- `yaw_out` = pid_yaw.update(yaw_setpoint, imu_yaw_rate, dt)

Output clamping: each motor value is clamped to [0, MAX_THROTTLE].
If any value goes negative after mixing, it is set to 0.

#### Motor Mixer Driver

| Function           | Input                              | Output              |
|--------------------|------------------------------------|----------------------|
| mixer_update()     | throttle, pitch, roll, yaw (float) | duty[4] (uint16)    |

This is a **Pure Core** module: deterministic, no side effects.

### 2.4 Command Acceptance Matrix

Commands received in an invalid mode are silently ignored (no error response).
Commands are evaluated as `drone_cmd_t`, independent of transport.

| Command          | Calibrate | Diag | Running |
|------------------|-----------|------|---------|
| CMD_ARM          | ignore    | accept | ignore (already armed) |
| CMD_DISARM       | ignore    | ignore | accept  |
| CMD_CALIBRATE    | ignore (in progress) | accept | ignore |
| CMD_JOYSTICK     | ignore    | ignore | accept  |
| CMD_SET_PID_GAIN | ignore    | ignore | accept  |
| CMD_MOTOR_TEST   | ignore    | accept | ignore  |

---

## 3. Edge Case Catalog

### 3.1 Communication Edge Cases

| ID    | Scenario                                    | Expected Behavior                                    |
|-------|---------------------------------------------|------------------------------------------------------|
| EC-01 | Malformed MicroBlue packet (missing ETX)    | UART shell times out via readBytesUntil; pure parser receives incomplete buffer and returns empty message |
| EC-02 | Partial packet received (buffer < 3 bytes)  | SOH check fails; message discarded                   |
| EC-03 | Unknown widget ID received                  | Message ignored silently                             |
| EC-04 | Valid ID but empty value string             | hasValue() returns false; message ignored            |
| EC-05 | Joystick value not parseable as "X,Y"       | sscanf returns < 2; use previous valid input         |
| EC-06 | Bluetooth disconnect during Running mode    | Failsafe: ramp motors down, transition to Diag      |
| EC-07 | Bluetooth reconnect after disconnect        | Remain in Diag; user must re-arm explicitly          |
| EC-08 | Rapid repeated arm/disarm commands          | Debounce: ignore arm within 1 s of last disarm      |
| EC-09 | UART receive buffer overflow                | Oldest bytes lost; parser re-syncs on next SOH      |
| EC-10 | Slider value out of range (< 0 or > 100)   | Clamp to 0-100 before applying                      |

### 3.2 Sensor Edge Cases

| ID    | Scenario                                    | Expected Behavior                                    |
|-------|---------------------------------------------|------------------------------------------------------|
| EC-11 | IMU WHO_AM_I returns wrong ID at boot       | Halt in error state; STAT_0 fast blink; send error via BT |
| EC-12 | IMU I2C bus hangs (SDA stuck low)           | I2C timeout (10 ms); attempt bus recovery (9 clocks on SCL); if fail: in Calibrate → fail calibration and remain; in Running → transition to Diag; in Diag → report error and remain |
| EC-13 | IMU returns stale/identical data            | Detect via timestamp; if stale > 20 ms → treat as IMU failure |
| EC-14 | IMU data spike (single-sample outlier)      | DLPF handles filtering; no additional software filtering needed |
| EC-15 | Calibration with drone not level            | Offsets will be wrong; user must recalibrate; no auto-detect |
| EC-16 | Calibration with drone moving               | Variance check on samples; if stddev > threshold → fail |
| EC-17 | IMU failure during Running mode             | Detected after 50 ms no data (INV-4); ramp motors to 0 over 200 ms; transition to Diag |

### 3.3 Power Edge Cases

| ID    | Scenario                                    | Expected Behavior                                    |
|-------|---------------------------------------------|------------------------------------------------------|
| EC-18 | Battery voltage drops below 6.6V (critical) | STAT_2 blink; send `lowbatt`/`critical`; auto-disarm |
| EC-19 | Battery voltage drops below 6.0V (cutoff)   | Motors off immediately (no ramp); remain in Diag     |
| EC-20 | Battery voltage recovers above threshold     | No auto-transition; user must re-arm manually        |
| EC-21 | ADC reading noise / fluctuation near threshold | Hysteresis: enter warning at 7.0V, exit at 7.2V   |
| EC-22 | Battery disconnected while running           | Hardware power loss; no software action possible     |
| EC-23 | Voltage regulator thermal shutdown           | 3.3V drops; MCU brownout reset → full startup sequence |

### 3.4 Motor Edge Cases

| ID    | Scenario                                    | Expected Behavior                                    |
|-------|---------------------------------------------|------------------------------------------------------|
| EC-24 | Motor test button held down in Diag         | Single pulse only (500 ms max); ignore until released |
| EC-25 | Multiple motor test buttons pressed at once  | Only first received is honored; others ignored       |
| EC-26 | Joystick full deflection immediately after arm | Throttle ramp limiter: max 33 duty counts per PID tick (50% of MAX_THROTTLE per second at 500 Hz) |
| EC-27 | Throttle at zero while armed                | Motors run at MOTOR_IDLE_DUTY (50/65535); PID stabilization remains active; mixer outputs clamped to [0, MAX_THROTTLE] |
| EC-28 | Excessive tilt angle (> 60 deg)             | Motors off immediately; transition to Diag           |
| EC-29 | Single motor failure (stall / disconnect)    | Not detectable without current sensing; PID will saturate; user must disarm |
| EC-30 | PWM timer overflow                          | STM32 auto-reload register handles wrap; no action needed |

### 3.5 Mode Transition Edge Cases

| ID    | Scenario                                    | Expected Behavior                                    |
|-------|---------------------------------------------|------------------------------------------------------|
| EC-31 | Arm command while calibration offsets invalid | Reject arm; remain in Diag; send error via BT      |
| EC-32 | Calibrate command while already calibrating  | Ignore; current calibration continues                |
| EC-33 | Power on with low battery (< 6.6V)         | Complete calibration; enter Diag; refuse arm         |
| EC-34 | SW2 button press during Running mode        | Disarm motors; transition to Diag                    |
| EC-35 | Rapid mode cycling (arm → disarm → arm)     | Enforce 1 s cooldown between arm attempts            |
| EC-36 | Calibration takes too long (> 5 s)          | Timeout; calibration fails; remain in Calibrate; report error |
