# Feature Specification

## Operating Modes

The drone operates in three distinct modes, transitioned via Bluetooth command
or hardware button (SW2).

### Mode Overview

| Mode      | Purpose                          | Motors | LED Pattern              |
|-----------|----------------------------------|--------|--------------------------|
| Calibrate | IMU offset calibration           | Off    | STAT_0 slow blink        |
| Diag      | Sensor readout and system check  | Off    | STAT_0 solid on          |
| Running   | Active flight with PID control   | Armed  | STAT_0 solid, STAT_1 on  |

### Mode: Calibrate

Entered on first boot or by user command. The drone must be placed on a flat,
level surface and remain still during calibration.

- Collect N samples of accelerometer and gyroscope data
- Compute and store zero-offset for each axis
- Send calibration result back to MicroBlue (`"calib"`, `"ok"` / `"fail"`)
- Auto-transition to Diag mode on success

### Mode: Diag

Diagnostic mode for pre-flight system verification. Motors remain disarmed.

- Stream live IMU data to MicroBlue for inspection
- Report battery voltage
- Verify Bluetooth link quality
- Individual motor test via MicroBlue buttons (brief pulse, one at a time)
- Transition to Running mode via arm command (`b0` = `"1"`)
- Transition to Calibrate mode via calibrate command (`b1` = `"1"`)

### Mode: Running

Active flight mode with PID stabilization loop.

- PID control loop active at 500 Hz
- Joystick inputs mapped to throttle / yaw / pitch / roll
- PID gains adjustable via sliders in real-time
- Low battery critical: ramp motors to 0 over 200 ms, transition to Diag
- Disarm command (`b0` = `"0"`) transitions back to Diag mode
- Bluetooth disconnect triggers failsafe: ramp motors to 0 over 200 ms, transition to Diag

### State Transitions

```
                   Power On
                      |
                      v
               +-----------+
               | Calibrate |<-----+
               +-----------+      |
                  |  success      | b1 (recalibrate)
                  v               |
               +------+          |
          +--->| Diag |----------+
          |    +------+
          |       | arm (b0="1")
          |       v
          |  +---------+
          +--| Running |
   disarm    +---------+
   low batt
   BT disconnect
```

## Flight Control

- 4-channel brushed DC motor control via PWM
- PID-based attitude stabilization
- 6-axis IMU sensor fusion (accelerometer + gyroscope)
- Configurable PID gain tuning via Bluetooth

## Sensor

| Sensor   | Module  | Interface | Parameters                    |
|----------|---------|-----------|-------------------------------|
| IMU      | MPU6050 | I2C       | 3-axis accel + 3-axis gyro    |
| Battery  | Voltage divider (R1: 30K top, R6: 20K bottom) | ADC | Battery voltage monitoring |

### IMU Specifications (MPU6050)

| Parameter          | Value                |
|--------------------|----------------------|
| Accelerometer Range | +/- 2g, 4g, 8g, 16g |
| Gyroscope Range    | +/- 250, 500, 1000, 2000 deg/s |
| I2C Address        | 0x68 (AD0 = LOW)    |
| Data Rate          | Up to 1 kHz         |
| Pull-ups           | R7, R8: 10K on SDA/SCL |

## Communication

- Bluetooth wireless control (HM-13 module)
- UART serial programming interface
- SWD debug interface (J-Link compatible)

## User Interface

| Component       | Ref        | Purpose                    |
|-----------------|------------|----------------------------|
| Status LED 0    | D1/D9/D10  | System status indicators   |
| Status LED 1    | D2         | Status feedback            |
| Status LED 2    | D3         | Status feedback            |
| Status LED 3    | D4         | Status feedback            |
| DIP Switch      | SW1        | Configuration (2-position) |
| Push Button     | SW2        | User input / reset         |
| Power Switch    | SW3        | Main power on/off          |

### Status LED Mapping

| Label   | Function                   |
|---------|----------------------------|
| STAT_0  | IMU / calibration status   |
| STAT_1  | Mode indicator             |
| STAT_2  | Low battery warning        |
| STAT_3  | Reserved                   |

Note: Power on is indicated by the PWR LED (hardware).
Bluetooth connection status is indicated by the HM-13 module's built-in LED.

### LED Behavior per Mode

| LED    | Calibrate       | Diag              | Running              |
|--------|-----------------|--------------------|-----------------------|
| STAT_0 | Slow blink      | Solid on (cal OK)  | Solid on              |
| STAT_1 | Off             | Off                | Solid on (armed)      |
| STAT_2 | (low batt blink)| (low batt blink)   | (low batt blink)      |
| STAT_3 | —               | —                  | —                     |

## Power Management

- Battery voltage monitoring via ADC (voltage divider R1: 30K top, R6: 20K bottom, ratio = 0.4)
- Low battery warning (LED + Bluetooth notification)
- Hardware power switch (SW3)
- Regulated 3.3V supply for digital logic

## Programming & Debug

| Interface    | Connector | Signals                      |
|--------------|-----------|------------------------------|
| UART Program | J3 (6-pin) | PRG_TX, PRG_RX, PRG_RTS, PRG_DTR, VCC, GND |
| J-Link SWD   | J8 (4-pin) | SWDIO, SWCLK, GND, 3.3V    |
| Expansion     | J7 (2x4)  | General purpose I/O         |
| Dev Headers   | J9, J10 (1x20) | Full MCU pin breakout  |

## Expansion

- Jumper header J11 for configuration
- Placeholder connector J6 for future modules
- 2x20 pin breakout headers (J9, J10) exposing all MCU pins
