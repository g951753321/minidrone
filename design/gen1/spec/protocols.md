# Protocol Specification

## I2C — IMU Communication

| Parameter       | Value                   |
|-----------------|-------------------------|
| Bus             | I2C1                    |
| Device          | MPU6050                 |
| Address         | 0x68                    |
| Speed           | 400 kHz (Fast Mode)     |
| Pull-ups        | 10K to 3.3V (R7, R8)   |
| Signals         | MPU_SDA, MPU_SCL        |
| Connector       | J4 (4-pin: VCC, GND, SDA, SCL) |

### MPU6050 Register Map (Key Registers)

| Register | Address | Description            |
|----------|---------|------------------------|
| PWR_MGMT_1 | 0x6B | Power management       |
| SMPLRT_DIV | 0x19 | Sample rate divider    |
| CONFIG     | 0x1A | DLPF configuration     |
| GYRO_CONFIG | 0x1B | Gyroscope range        |
| ACCEL_CONFIG | 0x1C | Accelerometer range   |
| ACCEL_XOUT_H | 0x3B | Accel data (6 bytes)  |
| GYRO_XOUT_H | 0x43 | Gyro data (6 bytes)   |
| WHO_AM_I   | 0x75 | Device ID (0x68)       |

## UART — Bluetooth Communication (MicroBlue Protocol)

The minidrone is controlled from an iPhone via the **MicroBlue** app by SnappyXO
(Mechanismic Inc.). MicroBlue communicates over BLE through the HM-13 module,
using a simple delimiter-based message protocol.

Reference: https://github.com/snappyxo/microblue-arduino

| Parameter       | Value                        |
|-----------------|------------------------------|
| App             | MicroBlue (iOS / Android)    |
| Module          | HM-13 (Bluetooth 4.0 BLE + SPP) |
| Connector       | J2 (4-pin: VCC, GND, TX, RX) |
| Baud Rate       | 115200                       |
| Data Bits       | 8                            |
| Stop Bits       | 1                            |
| Parity          | None                         |
| Flow Control    | None                         |
| Signals         | BT_TX, BT_RX                |

### MicroBlue Message Protocol

Messages use ASCII control characters as delimiters:

```
[SOH] ID [STX] VALUE [ETX]
 0x01      0x02        0x03
```

| Field | Byte  | Description                                |
|-------|-------|--------------------------------------------|
| SOH   | 0x01  | Start of Header — message start delimiter  |
| ID    | ASCII | Widget identifier string (e.g. "j0", "b0") |
| STX   | 0x02  | Start of Text — separator between ID and value |
| VALUE | ASCII | Value string (e.g. "512,512", "1", "0")    |
| ETX   | 0x03  | End of Text — message end delimiter        |

Example wire bytes for a joystick message:
```
0x01 'j' '0' 0x02 '5' '1' '2' ',' '5' '1' '2' 0x03
```

### MicroBlue Widget IDs

The MicroBlue app provides configurable UI widgets. Each widget sends messages
with a type-prefixed ID.

| Widget Type | ID Format | Value Format       | Value Range     | Description              |
|-------------|-----------|--------------------|-----------------|-----------------------------|
| Button      | `b0`..`bN` | `"1"` / `"0"`   | 1 = pressed, 0 = released | Momentary button        |
| Slider      | `sl0`..`slN` | `"0"`..`"100"` | 0 - 100 (integer)  | Linear slider              |
| Joystick    | `j0`..`jN` | `"X,Y"`          | 0 - 1023 each axis | Two-axis joystick (center = 512,512) |
| D-Pad       | `d0`..`dN` | `"X,Y"`          | 0 - 1023 each axis | Directional pad            |
| Switch      | `sw0`..`swN` | `"1"` / `"0"` | 1 = on, 0 = off    | Toggle switch              |
| Text Input  | `t0`..`tN` | arbitrary string | user text           | Free-form text input (not used) |

Note: Text Input widgets are not used in the drone control layout. Delimiter
bytes (0x01, 0x02, 0x03) appearing inside VALUE fields cause parser corruption.
Only widget types with well-defined numeric values are used.

### Drone Control Mapping

| Widget         | ID   | Mapping                              |
|----------------|------|--------------------------------------|
| Left Joystick  | `j0` | Throttle (Y) + Yaw (X)              |
| Right Joystick | `j1` | Pitch (Y) + Roll (X)                |
| Arm Button     | `b0` | Arm / disarm motors                  |
| Calib Button   | `b1` | IMU calibration                      |
| PID Kp Slider  | `sl0`| Proportional gain tuning             |
| PID Ki Slider  | `sl1`| Integral gain tuning                 |
| PID Kd Slider  | `sl2`| Derivative gain tuning               |

### Joystick Value Parsing

Joystick and D-Pad values arrive as `"X,Y"` strings with range 0-1023.
Center position is `512,512`.

```
Received: [0x01] "j0" [0x02] "700,300" [0x03]
Parsed:   id = "j0", value = "700,300"
          X = 700 (right of center)
          Y = 300 (above center)

Conversion to signed range:
  steering = X - 512  →  +188 (right)
  throttle = Y - 512  →  -212 (forward/up)
```

### Device-to-App Messages (Telemetry)

The minidrone can send data back to MicroBlue using the same frame format:

```
[0x01] ID [0x02] VALUE [0x03]
```

| ID         | Value                        | Format   | Description                  |
|------------|------------------------------|----------|------------------------------|
| `battery`  | voltage string               | "7.20"   | Pack voltage in V, 2 decimal places |
| `status`   | mode string                  | "diag"   | One of: "calibrate", "diag", "running", "error" |
| `imu`      | `"ax,ay,az,gx,gy,gz"`       | raw int16| Raw register values, comma-separated |
| `lowbatt`  | level string                 | "warning"| One of: "warning", "critical" |
| `calib`    | result string                | "ok"     | One of: "ok", "fail"         |
| `error`    | description string           | text     | Human-readable error message  |

## UART — Serial Programming

| Parameter       | Value                          |
|-----------------|--------------------------------|
| Connector       | J3 (6-pin header)              |
| Baud Rate       | 115200                         |
| Signals         | PRG_TX, PRG_RX, PRG_RTS, PRG_DTR |
| Protocol        | STM32 bootloader (USART)       |

### Boot Mode Selection

| BOOT0 | BOOT1 | Mode                    |
|-------|-------|-------------------------|
| 0     | X     | Main Flash (normal run) |
| 1     | 0     | System Memory (bootloader) |
| 1     | 1     | Embedded SRAM           |

## SWD — Debug Interface

| Parameter       | Value                   |
|-----------------|-------------------------|
| Connector       | J8 (4-pin header)       |
| Protocol        | Serial Wire Debug (SWD) |
| Signals         | SWDIO, SWCLK, GND, 3.3V |
| Compatible      | J-Link, ST-Link         |

## PWM — Motor Control

| Parameter       | Value                   |
|-----------------|-------------------------|
| Channels        | 4                       |
| Frequency       | 20 kHz                  |
| Timer Clock     | 72 MHz (APB2)           |
| Prescaler       | 0 (no prescale)         |
| Auto-Reload     | 3599 (72 MHz / 20 kHz)  |
| Effective Resolution | ~11.8 bits (3600 steps) |
| MAX_THROTTLE    | 3599                    |
| MOTOR_IDLE_DUTY | 50                      |
| Drive           | Low-side N-MOSFET (SI2302) |

| Channel | Signal  | Motor Position |
|---------|---------|----------------|
| CH1     | FR_PWM  | Front Right    |
| CH2     | FL_PWM  | Front Left     |
| CH3     | BR_PWM  | Back Right     |
| CH4     | BL_PWM  | Back Left      |

## ADC — Battery Monitoring

| Parameter       | Value                        |
|-----------------|------------------------------|
| Input           | VBAT_a (voltage divider)     |
| R_top (R1)      | 30K                          |
| R_bottom (R6)   | 20K                          |
| Divider Ratio   | 20K / (30K + 20K) = 0.4     |
| Max Input Voltage | 8.4V (full charge) → 3.36V at ADC (within 3.3V + margin) |
| ADC Resolution  | 12-bit (0-4095)              |
| Reference       | 3.3V (VDDA)                  |
| Conversion      | voltage_mv = (adc_raw * 3300 * 5) / (4095 * 2) |
| Sample Rate     | 5 Hz                        |
