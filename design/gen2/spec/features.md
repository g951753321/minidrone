# Feature Specification (Gen 2)

## Design Constraints

| Parameter       | Value                        |
|-----------------|------------------------------|
| Body Size Limit | 200 mm (W) x 200 mm (L), excluding propellers |
| Target AUW      | 71 g (2S) / 87 g (3S) — see [mechanical.md](mechanical.md) |
| Target Flight Time | >= 10 min hover (all configs) |
| Battery Support  | 2S - 3S LiPo (auto-detect)  |
| Use Environment | Indoor (2S) / Outdoor (3S)  |
| Architecture | Core Module (40x25mm) + Carrier Board (50x50mm) |

## Modular Architecture

The system is split into a reusable **Core Module** and a drone-specific **Carrier Board**.

### Core Module (40 x 25 mm)
- STM32H743VIH6 MCU (Cortex-M7, 480 MHz, 2 MB flash, 1 MB SRAM)
- nRF52832 BLE 5.0 module
- USB-C (DFU, CDC serial, standalone power)
- SWD debug header
- Reset + DFU buttons, power LED, WS2812B status LED
- Can operate standalone (powered from USB) for development

### Carrier Board (50 x 50 mm, drone-specific)
- All flight sensors (IMU, baro, ToF, optical flow)
- ELRS receiver pads
- ESC / motor pads
- Battery management (TPS63070, charger, protection)
- Buzzer, companion computer header
- For hardware details, see [hardware.md](hardware.md)

### Core Module as Dev Board
The core module functions as a standalone development board:
- Plug into USB-C, program via DFU or SWD
- All MCU peripherals exposed on 1.27mm castellated edge pads
- BLE available for wireless debug and configuration
- Design custom carrier boards for other applications (robotics, data loggers, etc.)

## Operating Modes

The drone operates in four distinct modes, transitioned via ELRS command,
BLE command, or hardware button (SW1).

### Mode Overview

| Mode      | Purpose                            | Motors | LED Color              |
|-----------|------------------------------------|--------|------------------------|
| Calibrate | IMU + barometer offset calibration | Off    | Yellow breathe         |
| Diag      | Sensor readout and system check    | Off    | Blue solid             |
| Running   | Active flight with PID control     | Armed  | Green solid            |
| Error     | Unrecoverable fault                | Off    | Red fast blink         |

### Mode: Calibrate

Entered on first boot or by user command. The drone must be placed on a flat,
level surface and remain still during calibration.

- Collect N samples of accelerometer, gyroscope, and barometer data
- Compute and store zero-offset for each IMU axis
- Store barometer ground-level reference pressure
- Send calibration result via telemetry (`"calib"`, `"ok"` / `"fail"`)
- Auto-transition to Diag mode on success

### Mode: Diag

Diagnostic mode for pre-flight system verification. Motors remain disarmed.

- Stream live IMU data via telemetry for inspection
- Report battery voltage and cell count
- Verify ELRS link quality (RSSI, LQ)
- Verify BLE link status
- Verify optical flow sensor status
- Verify barometer / ToF altitude sensor status
- Individual motor test via command (brief DSHOT pulse, one at a time)
- Transition to Running mode via arm command
- Transition to Calibrate mode via calibrate command

### Mode: Running

Active flight mode with cascaded PID stabilization loop.

- Cascaded PID: outer angle loop + inner rate loop, active at 1 kHz (rate) / 500 Hz (angle)
- ELRS stick inputs mapped to throttle / yaw / pitch / roll
- Altitude hold mode via barometer + ToF fusion (toggle via AUX channel)
- Position hold mode via optical flow (toggle via AUX channel)
- PID gains adjustable via BLE configuration interface
- Low battery critical: ramp motors to 0 over 200 ms, transition to Diag
- Disarm command transitions back to Diag mode
- ELRS failsafe triggers: ramp motors to 0 over 200 ms, transition to Diag

### Mode: Error

Entered on unrecoverable hardware fault (IMU not detected, flash corrupt, etc.).

- All motors off
- RGB LED red fast blink
- Error code sent via BLE telemetry
- Watchdog timeout resets MCU to retry startup sequence

### State Transitions

```
                   Power On
                      |
                      v
               +-----------+
               | Calibrate |<-----+
               +-----------+      |
                  |  success      | recalibrate cmd
                  |               |
                  |  fail/timeout |
                  |------+        |
                  v      v        |
               +------+ +-------+ |
          +--->| Diag |  | Error | |
          |    +------+--+-------+ |
          |       | arm cmd       |
          |       v               |
          |  +---------+          |
          +--| Running |----------+
   disarm    +---------+
   low batt
   ELRS failsafe
   IMU failure
   excessive tilt
```

## Flight Control

- 4-channel brushless motor control via DSHOT300
- Cascaded PID attitude stabilization (angle + rate loops)
- 6-axis IMU sensor fusion (accelerometer + gyroscope, ICM-42688-P)
- Barometric altitude hold (BMP390 + VL53L5CX fusion)
- Optical flow position hold (PMW3901)
- Configurable PID gain tuning via BLE
- Motor current sensing via shunt resistor + external op-amp on carrier

## Sensors

| Sensor         | IC / Module   | Interface | Purpose                            |
|----------------|---------------|-----------|-------------------------------------|
| IMU            | ICM-42688-P   | SPI1      | Attitude (accel + gyro)             |
| Barometer      | BMP390        | I2C1      | Altitude hold (> 2 m)              |
| Time-of-Flight | VL53L5CX      | I2C1      | Altitude hold (< 4 m)              |
| Optical Flow   | PMW3901       | SPI2      | Position hold                       |
| Battery ADC    | STM32H7 ADC1  | Internal  | Voltage via divider (30K/10K)       |
| Motor Current  | Shunt + op-amp| ADC2      | Stall / overcurrent detect          |

For IC specifications and placement requirements, see [hardware.md](hardware.md).
For bus protocols and register maps, see [protocols.md](protocols.md).

## Communication

| Link               | Purpose                  | Module / Interface        |
|--------------------|--------------------------|---------------------------|
| ELRS 2.4 GHz      | Primary flight control   | ELRS Lite RX, UART (CRSF)|
| BLE 5.0            | Config, telemetry, OTA   | nRF52832 module (MDBT42Q) |
| USB-C              | Programming, debug, charge| STM32H7 native USB 2.0 FS|
| SWD                | Debug interface           | 4-pin header (SWDIO, SWCLK, GND, 3V3) |

## User Interface

| Component           | Type              | Purpose                    |
|---------------------|-------------------|----------------------------|
| RGB LED             | WS2812B (x1)     | Mode / status / battery indicator |
| Buzzer              | Passive piezo     | Arm beep, low battery alarm, lost model beep |
| Push Button         | SW1 (tactile)     | Mode cycle / bind / DFU boot |
| Power Switch        | SW2 (slide)       | Main power on/off           |

### RGB LED Behavior per Mode

| Mode      | Color         | Pattern              |
|-----------|---------------|----------------------|
| Calibrate | Yellow        | Breathe (fade in/out)|
| Diag      | Blue          | Solid on             |
| Running   | Green         | Solid on             |
| Error     | Red           | Fast blink (5 Hz)    |
| Low Batt Warning  | Orange | Blink (2 Hz)        |
| Low Batt Critical | Red   | Blink (2 Hz)        |
| ELRS not linked   | Purple| Slow blink (1 Hz)   |
| BLE connected     | Cyan  | Single flash on connect |

### Buzzer Behavior

| Event               | Pattern                    |
|---------------------|----------------------------|
| Arm                 | Rising two-tone beep       |
| Disarm              | Falling two-tone beep      |
| Low battery warning | Single beep every 2 s      |
| Low battery critical| Continuous rapid beep      |
| Lost model (BLE cmd)| Loud continuous tone       |
| ELRS bind mode      | Repeating short beep       |

## Power Management

- **2S - 3S LiPo support** with automatic cell count detection at boot
- **Per-cell voltage monitoring** via JST-XH balance connector
- **Cell imbalance detection**: refuse arm if max-min cell delta > 100 mV
- Per-cell threshold scaling (warning, critical, cutoff) — see [safety.md](safety.md)
- Per-motor RPM monitoring (bidirectional DSHOT) + current backup — see [safety.md](safety.md)
- Low battery warning (LED + buzzer + telemetry notification)
- **No onboard charger** — use external LiPo balance charger via JST-XH connector
- For power system components, see [hardware.md](hardware.md#power-supply)

## Programming and Debug

| Interface    | Connector   | Signals                          |
|--------------|-------------|----------------------------------|
| USB-C        | On core module | USB D+, D-, VBUS, CC1, CC2, GND |
| SWD          | On core module (4-pin, 1.27mm pitch) | SWDIO, SWCLK, GND, 3V3         |

### Boot Modes

| SW1 Press at Power-On | Mode                    |
|------------------------|-------------------------|
| Not pressed            | Normal boot (flash)     |
| Held > 3 s            | USB DFU bootloader mode |
| Double-press           | ELRS bind mode          |

## Data Logging (Blackbox)

- 1 MB internal flash (STM32H743 bank 2); optional 16 MB external SPI flash on carrier
- Logs: IMU raw data, PID outputs, motor duty, battery voltage, ELRS inputs
- Log rate: configurable 100 Hz - 1 kHz
- Download via BLE or USB serial
- Erase via BLE command or USB command
- Circular buffer: oldest data overwritten when full

## OTA Firmware Update

- BLE DFU via nRF52 bootloader (MCU firmware image transfer)
- Fallback: USB-C DFU mode via hardware button
- For detailed OTA flow, see [remote_controller.md](remote_controller.md#ota-firmware-update-via-ble)

## Expansion

- I2C bus exposed on SWD connector (shared pins) for external sensors
- 2 spare GPIO pads on PCB edge for future use
- SPI flash can store user configuration and waypoint data
