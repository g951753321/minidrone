# Remote Controller Specification (Gen 2)

## Architecture Overview

Gen 2 uses a dual-link communication architecture:

| Link       | Protocol   | Purpose                     | Latency  | Rate      |
|------------|-----------|------------------------------|----------|-----------|
| **Primary**  | ELRS (CRSF) | Real-time flight control   | 2 - 5 ms | 50-500 Hz |
| **Secondary**| BLE 5.0    | Config, telemetry, OTA, blackbox | 15-30 ms | 10 Hz |

ELRS handles all time-critical stick inputs and flight mode switching.
BLE handles configuration, PID tuning, blackbox download, and OTA updates.
If BLE disconnects during flight, there is no impact on flight control.

## Primary Link: ExpressLRS (ELRS)

### Transmitter (TX) — Recommended Hardware

| Option               | Price   | Form Factor    | ELRS Built-in | Notes           |
|----------------------|---------|----------------|----------------|-----------------|
| RadioMaster Pocket   | ~$60    | Compact gamepad| Yes (2.4 GHz)  | Best value, portable |
| RadioMaster Zorro    | ~$90    | Full gimbals   | Yes (2.4 GHz)  | Better gimbals   |
| RadioMaster Boxer    | ~$120   | Full-size TX   | Yes (2.4 GHz)  | Most ergonomic   |
| BetaFPV LiteRadio 3  | ~$40   | Compact gamepad| Yes (2.4 GHz)  | Budget option    |

All recommended TXs run **EdgeTX** open-source firmware with native ELRS support.

### Receiver (RX) — On Drone

| Option                  | Weight | Size       | Antenna    | Notes           |
|-------------------------|--------|------------|------------|-----------------|
| **BetaFPV ELRS Lite RX** | 0.5 g | 10x10 mm  | Ceramic    | Lightest, recommended |
| HappyModel EP2          | 0.9 g  | 12x12 mm  | Wire dipole| Better range     |
| HappyModel PP           | 0.4 g  | 10x10 mm  | Ceramic    | Ultra-light      |

Recommended: **BetaFPV ELRS Lite RX** for best weight/performance balance.

### ELRS Configuration

| Parameter          | Value                          |
|--------------------|--------------------------------|
| Frequency          | 2.4 GHz                        |
| TX Power           | 25 mW (indoor) / 100 mW (outdoor) |
| Packet Rate        | 250 Hz (recommended default)   |
| Switch Mode        | Hybrid 8-channel (8 proportional channels) |
| Telemetry Ratio    | 1:8 (one telemetry packet per 8 RC packets) |
| Bind Phrase        | Configured via ExpressLRS Configurator (no physical bind button needed) |

### Packet Rate vs Latency Trade-off

| Packet Rate | Latency | Range (25 mW) | Channels | Best For           |
|-------------|---------|----------------|----------|--------------------|
| 50 Hz       | 20 ms   | 30+ km         | 12       | Long range (unused)|
| 150 Hz      | 6.6 ms  | 10+ km         | 12       | Outdoor cruising   |
| 250 Hz      | 4 ms    | 5+ km          | 8        | **Default indoor** |
| 500 Hz      | 2 ms    | 2+ km          | 4        | Racing (aggressive)|

Recommended: **250 Hz** for indoor use (4 ms latency, 8 channels sufficient).

### Channel Assignment (EdgeTX Mixer)

| Channel | EdgeTX Input | Function         | Endpoints     |
|---------|-------------|------------------|---------------|
| CH1     | Aileron (A)  | Roll             | 172 - 1811    |
| CH2     | Elevator (E) | Pitch            | 172 - 1811    |
| CH3     | Throttle (T) | Throttle         | 172 - 1811    |
| CH4     | Rudder (R)   | Yaw              | 172 - 1811    |
| CH5     | SA (2-pos)   | Arm / Disarm     | 172 / 1811    |
| CH6     | SB (3-pos)   | Flight Mode      | 172/992/1811  |
| CH7     | SC (2-pos)   | Position Hold    | 172 / 1811    |
| CH8     | SD (2-pos)   | Buzzer / Beeper  | 172 / 1811    |

### ELRS Failsafe Configuration

| Setting                      | Value                          |
|------------------------------|--------------------------------|
| Failsafe Mode               | No Pulses (CRSF stops sending) |

For FC-side failsafe detection and actions, see [safety.md](safety.md#failsafe-conditions).

### ELRS Bind Procedure

1. Power on drone while double-pressing SW1 → drone enters bind mode (LED purple blink)
2. On TX: enter ELRS Lua script → select "Bind"
3. TX and RX exchange bind phrase
4. LED changes to blue (bound, in Diag mode)
5. Bind phrase stored in ELRS RX flash; persists across power cycles

Alternative: use ExpressLRS Configurator to pre-flash a bind phrase to both
TX module and RX. No runtime bind procedure needed.

## Secondary Link: BLE 5.0

### Phone App

| Platform   | Framework       | Features                           |
|------------|----------------|-------------------------------------|
| iOS        | Swift / SwiftUI | Config, telemetry, blackbox viewer  |
| Android    | Kotlin / Compose| Same features as iOS                |
| Cross-platform | Flutter     | Single codebase for both            |

Recommended: **Flutter** for cross-platform development.

### BLE GATT Service Layout

| Service                  | UUID (16-bit) | Description                     |
|--------------------------|---------------|---------------------------------|
| Drone Control Service    | 0xDC01        | Configuration and commands       |
| Telemetry Service        | 0xDC02        | Real-time sensor data streaming  |
| Blackbox Service         | 0xDC03        | Flight log download              |
| DFU Service              | 0xFE59        | Nordic DFU (standard UUID)       |

#### Drone Control Service (0xDC01)

| Characteristic     | UUID   | Properties | Description                    |
|--------------------|--------|------------|--------------------------------|
| PID Gains          | 0xDC11 | R/W        | Read/write PID gains (36 bytes: 3 axes x 2 loops x 3 gains x float) |
| Flight Config      | 0xDC12 | R/W        | Max tilt angle, throttle curve, etc. |
| Device Info        | 0xDC13 | R          | FW version, HW revision, serial |
| Command            | 0xDC14 | W          | Send commands (calibrate, find-me, erase blackbox) |

#### Telemetry Service (0xDC02)

| Characteristic     | UUID   | Properties | Description                    |
|--------------------|--------|------------|--------------------------------|
| Battery            | 0xDC21 | N          | Voltage (mV), notify 1 Hz     |
| IMU Raw            | 0xDC22 | N          | 6x int16, notify 10 Hz        |
| Attitude           | 0xDC23 | N          | Pitch, roll, yaw (float), notify 10 Hz |
| Altitude           | 0xDC24 | N          | Altitude cm (int32), notify 10 Hz |
| Motor Current      | 0xDC25 | N          | 4x uint16 mA, notify 10 Hz    |
| Link Stats         | 0xDC26 | N          | RSSI, LQ from ELRS, notify 1 Hz |
| Mode               | 0xDC27 | N          | Current mode enum, notify on change |

#### Blackbox Service (0xDC03)

| Characteristic     | UUID   | Properties | Description                    |
|--------------------|--------|------------|--------------------------------|
| Log Info           | 0xDC31 | R          | Total logs, bytes used, bytes free |
| Log Data           | 0xDC32 | N          | Stream log data chunks (240 bytes per notify) |
| Log Control        | 0xDC33 | W          | Start download, stop, erase    |

### BLE Connection Parameters

| Parameter                  | Value              |
|----------------------------|--------------------|
| Advertising Interval       | 100 ms (fast) / 1000 ms (slow, after 30 s) |
| Connection Interval        | 15 ms (min) / 30 ms (max) |
| Slave Latency              | 0                  |
| Supervision Timeout        | 4 s                |
| MTU                        | 247 bytes (negotiated) |
| PHY                        | 2 Mbps (BLE 5.0)  |
| TX Power                   | 0 dBm (indoor sufficient) |

### BLE Security

| Parameter                  | Value              |
|----------------------------|--------------------|
| Pairing                    | Just Works (no PIN for simplicity) |
| Encryption                 | AES-128 (BLE link layer) |
| Bonding                    | Yes (store pairing keys for reconnection) |

## OTA Firmware Update via BLE

### Update Flow

```
1. Phone app: select firmware image file (.bin)
2. App connects to DFU Service (0xFE59)
3. App transfers image to nRF52 via Nordic DFU protocol
4. nRF52 validates image CRC
5. nRF52 enters STM32 update mode:
   a. Assert STM32 BOOT0 pin (via GPIO)
   b. Assert STM32 NRST pin (reset)
   c. Release NRST (STM32 boots into USART bootloader)
   d. Transfer image to STM32 via USART1 (115200 baud, STM32 bootloader protocol)
   e. Verify written flash (read-back CRC)
   f. Release BOOT0 pin
   g. Reset STM32 (normal boot)
6. nRF52 reports success/failure to app
```

### Update Safety

| Concern                    | Mitigation                     |
|----------------------------|--------------------------------|
| Power loss during update   | nRF52 retains image; retry on next boot |
| Corrupt image flashed      | STM32 CRC check at boot; if fail, enter DFU mode |
| OTA during flight          | DFU command rejected if mode = Running |

## Telemetry Display on TX

ELRS supports sending telemetry from FC back to the TX via CRSF uplink.
The TX (EdgeTX) can display telemetry on its screen or speak it via audio.

### Telemetry Sensors Sent to TX

| Sensor       | CRSF Frame Type | Data                        | Rate |
|-------------|----------------|-----------------------------|------|
| Battery      | 0x08           | Voltage, current, mAh, %   | 1 Hz |
| Attitude     | 0x1E           | Pitch, roll, yaw            | 5 Hz |
| Flight Mode  | 0x21           | Mode name string            | On change |

### EdgeTX Configuration

| Setting                  | Value                          |
|--------------------------|--------------------------------|
| Telemetry protocol       | CRSF (auto-detected)          |
| Battery sensor           | "RxBt" (receiver battery)      |
| Low battery alarm        | Set to 3.5V (per cell)        |
| Critical battery alarm   | Set to 3.3V (per cell)        |
| Voice alerts             | "Battery low" on warning       |

## Controller Ergonomics

### Recommended TX Settings (EdgeTX)

| Parameter          | Value                          |
|--------------------|--------------------------------|
| Stick Mode         | Mode 2 (left: throttle/yaw, right: pitch/roll) |
| Expo (all sticks)  | 30% (soften center feel)       |
| Throttle Curve     | Linear (no expo on throttle)   |
| Deadband           | 2% (reduce center jitter)      |
| Channel Range      | CRSF default (172-1811)        |

### Model Setup Checklist (EdgeTX)

1. Create new model
2. Set Internal RF: CRSF, 250 Hz, 25 mW
3. Mixer: CH1=Ail, CH2=Ele, CH3=Thr, CH4=Rud (standard)
4. Mixer: CH5=SA (arm), CH6=SB (flight mode), CH7=SC (pos hold), CH8=SD (beep)
5. Set failsafe: No Pulses
6. Telemetry: discover sensors (auto from CRSF)
7. Set battery alerts: warning 3.5V, critical 3.3V
8. Bind (or use bind phrase)
