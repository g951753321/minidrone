# Protocol Specification (Gen 2)

Note: Bus assignments reference STM32H743VIH6 peripherals. SPI/I2C/UART
signals are routed from the core module to the carrier board via castellated
pads — see [hardware.md](hardware.md#pin-allocation-on-connector).

## SPI1 — IMU Communication (ICM-42688-P)

| Parameter       | Value                   |
|-----------------|-------------------------|
| Bus             | SPI1 (carrier, via core module passthrough) |
| Device          | ICM-42688-P             |
| Mode            | SPI Mode 3 (CPOL=1, CPHA=1) |
| Clock           | 8 MHz (normal), 24 MHz (burst read) |
| CS Pin          | CS_IMU (active low)     |
| Signals         | SPI1_SCK, SPI1_MISO, SPI1_MOSI, CS_IMU |
| INT1            | Data-ready interrupt (active high, push-pull) |

### ICM-42688-P Register Map (Key Registers)

Register Bank 0 (default):

| Register         | Address | R/W | Description                |
|------------------|---------|-----|----------------------------|
| DEVICE_CONFIG    | 0x11    | R/W | SPI mode, soft reset       |
| INT_CONFIG       | 0x14    | R/W | INT1/INT2 configuration    |
| FIFO_CONFIG      | 0x16    | R/W | FIFO mode selection        |
| TEMP_DATA1       | 0x1D    | R   | Temperature data high byte |
| TEMP_DATA0       | 0x1E    | R   | Temperature data low byte  |
| ACCEL_DATA_X1    | 0x1F    | R   | Accel X high byte          |
| ACCEL_DATA_X0    | 0x20    | R   | Accel X low byte           |
| ACCEL_DATA_Y1    | 0x21    | R   | Accel Y high byte          |
| ACCEL_DATA_Y0    | 0x22    | R   | Accel Y low byte           |
| ACCEL_DATA_Z1    | 0x23    | R   | Accel Z high byte          |
| ACCEL_DATA_Z0    | 0x24    | R   | Accel Z low byte           |
| GYRO_DATA_X1     | 0x25    | R   | Gyro X high byte           |
| GYRO_DATA_X0     | 0x26    | R   | Gyro X low byte            |
| GYRO_DATA_Y1     | 0x27    | R   | Gyro Y high byte           |
| GYRO_DATA_Y0     | 0x28    | R   | Gyro Y low byte            |
| GYRO_DATA_Z1     | 0x29    | R   | Gyro Z high byte           |
| GYRO_DATA_Z0     | 0x2A    | R   | Gyro Z low byte            |
| INT_STATUS       | 0x2D    | R   | Interrupt status (clear on read) |
| PWR_MGMT0        | 0x4E    | R/W | Accel/gyro power mode      |
| GYRO_CONFIG0     | 0x4F    | R/W | Gyro FS and ODR            |
| ACCEL_CONFIG0    | 0x50    | R/W | Accel FS and ODR           |
| GYRO_CONFIG1     | 0x51    | R/W | Gyro filter bandwidth      |
| ACCEL_CONFIG1    | 0x53    | R/W | Accel filter bandwidth     |
| WHO_AM_I         | 0x75    | R   | Device ID (expected: 0x47) |
| BANK_SEL         | 0x76    | R/W | Register bank selection    |

### SPI Transaction Format

Read: CS low → [reg_addr | 0x80] → [dummy_byte(s)] → read data → CS high
Write: CS low → [reg_addr & 0x7F] → [data] → CS high

Burst read of 12 bytes (accel + gyro):
CS low → [0x1F | 0x80] → read 12 bytes (ACCEL_X_H through GYRO_Z_L) → CS high

### Recommended Initialization

```
1. Soft reset: DEVICE_CONFIG = 0x01, wait 1 ms
2. Verify WHO_AM_I = 0x47
3. Set gyro: GYRO_CONFIG0 = 0x06 (2000 dps, 1 kHz ODR)
4. Set accel: ACCEL_CONFIG0 = 0x06 (16g, 1 kHz ODR)
5. Set filter: GYRO_CONFIG1 BW = ODR/4 (anti-alias)
6. Enable INT1 data-ready: INT_CONFIG = 0x02
7. Power on: PWR_MGMT0 = 0x0F (gyro LN + accel LN)
8. Wait 30 ms for gyro startup
```

## SPI2 — Shared Bus (Flash + Optical Flow)

| Parameter       | Value                          |
|-----------------|--------------------------------|
| Bus             | SPI2                           |
| Devices         | W25Q128 (flash), PMW3901 (OF) |
| Mode            | SPI Mode 0 (CPOL=0, CPHA=0) for flash; Mode 3 for PMW3901 |
| Clock           | 8 MHz (flash), 2 MHz (PMW3901)|
| CS Pins         | CS_FLASH, CS_OF (active low)  |
| Signals         | SPI2_SCK, SPI2_MISO, SPI2_MOSI |

Note: SPI mode differs between devices. Reconfigure SPI mode before each
device transaction, or use separate SPI peripherals if timing is critical.
Alternative: use SPI3 for PMW3901 if available pins permit.

### W25Q128 Flash Commands

| Command          | Opcode | Description                   |
|------------------|--------|-------------------------------|
| Read Data        | 0x03   | Read bytes from address       |
| Fast Read        | 0x0B   | Read with dummy byte          |
| Page Program     | 0x02   | Write up to 256 bytes         |
| Sector Erase     | 0x20   | Erase 4 KB sector             |
| Block Erase 32K  | 0x52   | Erase 32 KB block             |
| Block Erase 64K  | 0xD8   | Erase 64 KB block             |
| Chip Erase       | 0xC7   | Erase entire chip             |
| Write Enable     | 0x06   | Must precede any write/erase  |
| Read Status Reg  | 0x05   | Check BUSY bit                |
| Read JEDEC ID    | 0x9F   | Manufacturer + device ID      |

### PMW3901 Optical Flow Registers

| Register         | Address | Description                   |
|------------------|---------|-------------------------------|
| Product_ID       | 0x00   | Expected: 0x49                |
| Motion           | 0x02   | Motion detected flag          |
| Delta_X_L        | 0x03   | X motion low byte             |
| Delta_X_H        | 0x04   | X motion high byte            |
| Delta_Y_L        | 0x05   | Y motion low byte             |
| Delta_Y_H        | 0x06   | Y motion high byte            |
| SQUAL            | 0x07   | Surface quality (0-169)       |

Read: [reg_addr & 0x7F] → wait 50 us → read byte
Write: [reg_addr | 0x80] → data byte → wait 50 us

## I2C1 — Shared Bus (Barometer + ToF)

| Parameter       | Value                          |
|-----------------|--------------------------------|
| Bus             | I2C1 (carrier, via core module passthrough) |
| Devices         | BMP390 (0x77), VL53L5CX (0x29)|
| Speed           | 400 kHz (Fast Mode)            |
| Pull-ups        | 4.7K to 3.3V (R1, R2)         |
| Signals         | I2C1_SDA, I2C1_SCL             |


### BMP390 Register Map (Key Registers)

| Register         | Address | Description                   |
|------------------|---------|-------------------------------|
| CHIP_ID          | 0x00   | Device ID (expected: 0x60)    |
| ERR_REG          | 0x02   | Error status                  |
| STATUS           | 0x03   | Sensor status                 |
| DATA_0           | 0x04   | Pressure XLSB                 |
| DATA_1           | 0x05   | Pressure LSB                  |
| DATA_2           | 0x06   | Pressure MSB                  |
| DATA_3           | 0x07   | Temperature XLSB              |
| DATA_4           | 0x08   | Temperature LSB               |
| DATA_5           | 0x09   | Temperature MSB               |
| PWR_CTRL         | 0x1B   | Pressure/temp enable, mode    |
| OSR              | 0x1C   | Oversampling settings         |
| ODR              | 0x1D   | Output data rate              |
| CONFIG           | 0x1F   | IIR filter coefficient        |
| NVM_PAR (21 bytes)| 0x31-0x45 | Calibration coefficients  |

### VL53L5CX Interface

The VL53L5CX uses a proprietary firmware upload + register interface over I2C.

| Operation        | Description                          |
|------------------|--------------------------------------|
| FW Upload        | ~86 KB firmware loaded at boot       |
| Start Ranging    | Configure resolution (4x4/8x8), frequency, integration time |
| Get Data         | Read ranging results (distance per zone + status) |
| INT Pin          | Asserted when new data ready         |

Note: Use ST's VL53L5CX Ultra Lite Driver (ULD) API for all interactions.
The driver handles firmware upload, configuration, and data parsing.

### I2C Bus Recovery

If SDA is stuck low (bus hang), execute recovery sequence:
1. Configure SCL as GPIO output, SDA as GPIO input
2. Clock SCL 9 times (toggle high/low)
3. Check if SDA is released (high)
4. Generate STOP condition (SDA low-to-high while SCL high)
5. Reconfigure pins as I2C alternate function

## USART1 — ELRS Receiver (CRSF Protocol)

| Parameter       | Value                          |
|-----------------|--------------------------------|
| UART            | USART1 (carrier, via core module passthrough) |
| Baud Rate       | 420000                         |
| Data Bits       | 8                              |
| Stop Bits       | 1                              |
| Parity          | None                           |
| Flow Control    | None                           |
| Direction       | Half-duplex (TX for telemetry, RX for RC data) or full-duplex |
| Signals         | USART1_TX, USART1_RX          |

### CRSF Frame Format

```
[SYNC] [LEN] [TYPE] [PAYLOAD...] [CRC]
 0xC8   1B    1B     N bytes      1B
```

| Field   | Size  | Description                                |
|---------|-------|--------------------------------------------|
| SYNC    | 1     | Sync byte: 0xC8 (from TX) or 0xEE (from FC) |
| LEN     | 1     | Length of TYPE + PAYLOAD + CRC             |
| TYPE    | 1     | Frame type                                 |
| PAYLOAD | N     | Type-dependent payload                     |
| CRC     | 1     | CRC8 (DVB-S2 polynomial 0xD5)             |

### CRSF Frame Types (Used)

| Type | Value | Direction    | Description              |
|------|-------|-------------|--------------------------|
| RC Channels Packed | 0x16 | RX → FC | 16 channels, 11-bit each |
| Link Statistics    | 0x14 | RX → FC | RSSI, LQ, SNR, TX power  |
| Battery Sensor     | 0x08 | FC → TX | Voltage, current, mAh, % |
| Attitude           | 0x1E | FC → TX | Pitch, roll, yaw         |
| Flight Mode        | 0x21 | FC → TX | Mode name string         |
| GPS                | 0x02 | FC → TX | Lat, lon, alt, speed     |

### RC Channels Packed (0x16)

16 channels packed as 11-bit values (176 bits = 22 bytes payload).
Channel value range: 172 - 1811 (CRSF standard).
Center: 992. Endpoints: 172 (min), 1811 (max).

```
Byte layout (little-endian bitstream):
  ch0  = bits[0:10]
  ch1  = bits[11:21]
  ch2  = bits[22:32]
  ...
  ch15 = bits[165:175]
```

### Channel Mapping

For channel-to-function assignment and EdgeTX mixer setup,
see [remote_controller.md](remote_controller.md#channel-assignment-edgetx-mixer).

For channel-to-drone_cmd_t mapping,
see [behavioral.md](behavioral.md#crsf-backend-mapping).

### CRSF CRC8 Calculation

Polynomial: 0xD5 (DVB-S2)
Initial value: 0x00
Input: TYPE + PAYLOAD bytes (excludes SYNC and LEN)

```c
uint8_t crsf_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0xD5 : (crc << 1);
        }
    }
    return crc;
}
```

### CRSF Failsafe

For failsafe triggers and system-level actions,
see [safety.md](safety.md#failsafe-conditions).

## USART2 — BLE Module (nRF52832)

| Parameter       | Value                          |
|-----------------|--------------------------------|
| UART            | USART2 (on core module, directly connected to nRF52) |
| Baud Rate       | 115200                         |
| Data Bits       | 8                              |
| Stop Bits       | 1                              |
| Parity          | None                           |
| Flow Control    | None                           |
| Signals         | USART2_TX, USART2_RX          |

### BLE Transport Modes

The BLE link supports two transport modes over the same NUS (Nordic UART Service):

| Mode       | Purpose                          | Protocol           |
|------------|----------------------------------|--------------------|
| **Shell**  | Runtime command line (debug, config) | Zephyr shell (ASCII text, interactive) |
| **Binary** | Structured telemetry and commands    | Length-delimited binary (below)        |

The nRF52 module runs NUS (UUID 6E400001-B5A3-F393-E0A9-E50E24DCCA9E).
Any BLE terminal app (nRF Connect, BLESerial nRF, Wible) connects to NUS
and gets interactive shell access. Structured telemetry uses the binary
protocol for programmatic access (phone app, GATT characteristics).

### BLE Binary Protocol

Messages between STM32 and nRF52 use a simple length-delimited binary protocol:

```
[START] [LEN] [CMD] [PAYLOAD...] [CSUM]
 0xAA    1B    1B    N bytes      1B
```

| Field   | Size  | Description                                |
|---------|-------|--------------------------------------------|
| START   | 1     | Start byte: 0xAA                          |
| LEN     | 1     | Length of CMD + PAYLOAD (1-252)            |
| CMD     | 1     | Command / message type                     |
| PAYLOAD | 0-251 | Command-dependent payload                  |
| CSUM    | 1     | XOR checksum of LEN + CMD + PAYLOAD       |

### BLE Command Types

| CMD  | Name              | Direction    | Payload                    |
|------|-------------------|-------------|----------------------------|
| 0x01 | TELEM_BATTERY     | STM32 → nRF | voltage_mv (uint16 LE)    |
| 0x02 | TELEM_IMU         | STM32 → nRF | ax,ay,az,gx,gy,gz (6x int16 LE) |
| 0x03 | TELEM_MODE        | STM32 → nRF | mode (uint8)              |
| 0x04 | TELEM_ERROR       | STM32 → nRF | error_code (uint8)        |
| 0x05 | TELEM_LOWBATT     | STM32 → nRF | level (uint8: 1=warn, 2=crit) |
| 0x06 | TELEM_CALIB       | STM32 → nRF | result (uint8: 1=ok, 0=fail) |
| 0x07 | TELEM_ALTITUDE    | STM32 → nRF | alt_cm (int32 LE)         |
| 0x08 | TELEM_LINK_STATS  | STM32 → nRF | rssi (int8), lq (uint8)   |
| 0x10 | CMD_SET_PID       | nRF → STM32 | axis(u8), Kp(f32), Ki(f32), Kd(f32) |
| 0x11 | CMD_BLACKBOX_DL   | nRF → STM32 | offset(u32), length(u32)  |
| 0x12 | CMD_BLACKBOX_ERASE| nRF → STM32 | (none)                    |
| 0x13 | CMD_FIND_ME       | nRF → STM32 | enable (uint8: 1=on, 0=off) |
| 0x20 | BLACKBOX_DATA     | STM32 → nRF | raw flash bytes (up to 240B) |
| 0xF0 | DFU_START         | nRF → STM32 | image_size (uint32 LE)    |
| 0xF1 | DFU_DATA          | nRF → STM32 | offset(u32), data(up to 240B) |
| 0xF2 | DFU_COMPLETE      | nRF → STM32 | crc32 (uint32 LE)         |
| 0xFE | ACK               | Both        | cmd_acked (uint8)         |
| 0xFF | NACK              | Both        | cmd_nacked (uint8), error(uint8) |

## USB — DFU and CDC Serial

| Parameter       | Value                          |
|-----------------|--------------------------------|
| Interface       | USB 2.0 Full Speed (12 Mbps)  |
| Classes         | DFU (Device Firmware Upgrade) + CDC ACM (virtual COM port) |
| VID/PID         | TBD (use ST default for DFU)  |
| CDC Baud        | Virtual (any baud, transparent to USB) |

### USB Boot Mode

- Normal boot: USB enumerates as CDC ACM (serial terminal for debug/config/blackbox)
- DFU boot (SW1 held > 3 s at power-on): USB enumerates as DFU class
- DFU uses STM32H7 built-in ROM bootloader or custom DFU bootloader

## DSHOT — Motor Control Protocol (Bidirectional DSHOT300)

Gen 2 uses **bidirectional DSHOT300** (also called BDShot300, DSHOT300_BIDIR).
This is a half-duplex protocol where the FC sends throttle commands and the
ESC responds with eRPM telemetry on the same signal wire.

| Parameter       | Value                          |
|-----------------|--------------------------------|
| Protocol        | DSHOT300 bidirectional          |
| Bit Rate        | 300 kbit/s (TX), ~375 kbit/s (RX, GCR) |
| Frame Size (TX) | 16 bits                        |
| Frame Size (RX) | 21 GCR nibbles → 16-bit eRPM   |
| Bit Period      | 3.33 us                        |
| Inverted        | Yes (idle high, active low — bidirectional spec) |
| Timer           | TIM1 (CH1-CH4) via DMA        |
| Pin Mode        | Open-drain with pull-up (allows both directions) |
| Update Rate     | Matches rate PID loop (4 kHz) — see [performance.md](performance.md) |
| ESC Firmware    | **BlueJay** (recommended) or BLHeli_S with bidirectional patch |

### DSHOT TX Frame Format (FC → ESC)

```
[THROTTLE (11 bit)] [TELEMETRY REQUEST (1 bit)] [CRC (4 bit)]
  bits 15-5              bit 4                     bits 3-0
```

| Field     | Bits  | Description                          |
|-----------|-------|--------------------------------------|
| Throttle  | 15-5  | 0 = disarmed, 48-2047 = throttle range |
| Telemetry | 4     | Always 0 in bidirectional mode (eRPM is automatic) |
| CRC       | 3-0   | XOR checksum (inverted in bidir mode) |

### DSHOT RX Frame Format (ESC → FC)

After each TX frame, the ESC responds with a 16-bit value encoded as 21 GCR
(Gaussian Code Representation) nibbles. The 16-bit value contains:

| Field     | Bits  | Description                          |
|-----------|-------|--------------------------------------|
| eRPM      | 15-4  | 12-bit eRPM exponent + mantissa      |
| CRC       | 3-0   | 4-bit CRC over eRPM bits             |

eRPM encoding: `eRPM = mantissa << exponent` (exponent in upper 3 bits of the 12 bits)

Mechanical RPM: `rpm = (eRPM * 60) / pole_pairs`

For 1404 motors (typically 7 pole pairs / 14 magnets): `rpm = eRPM * 60 / 7`

### Cycle Timing

```
Time → (per motor channel, ~100 us total)

  [TX DSHOT frame: 53.3 us] [Gap: ~30 us] [RX GCR response: ~25 us] [Idle]
        FC → ESC                              ESC → FC
```

Maximum bidirectional update rate: ~10 kHz per motor (limited by TX+RX cycle).
At 4 kHz rate PID loop, there is ~150 us idle margin per cycle.

### DSHOT Special Commands (Throttle 0-47)

| Value | Command              |
|-------|----------------------|
| 0     | Motor stop (disarmed)|
| 1-5   | Beep patterns        |
| 6     | ESC info request     |
| 7     | Spin direction 1     |
| 8     | Spin direction 2     |
| 12    | Save settings        |
| 20    | Spin direction normal|
| 21    | Spin direction reversed |
| 42    | Enable bidirectional eRPM telemetry  |

### Bidirectional DSHOT DMA Implementation (STM32H743)

Each motor channel uses TIM1 CHx in two phases per cycle:

```
Phase 1 (TX): 53 us
  - Pin mode: alternate function (push-pull)
  - TIM1 in PWM output compare mode
  - DMA1 streams 16 timing values from TX buffer to TIM1_CCRx
  - DMA complete → switch to phase 2

Phase 2 (RX): 50 us window
  - Pin mode: alternate function (open-drain input with pull-up)
  - TIM1 in input capture mode (capture on falling edge)
  - DMA2 streams up to 21 timestamp values from TIM1_CCRx to RX buffer
  - Software decodes GCR → eRPM
```

All 4 channels share TIM1 base clock; each channel has its own DMA stream
for both TX (PWM out) and RX (input capture).

### eRPM Decoder (Pure Core)

```c
// Decode 21 GCR timestamps → 16-bit value → eRPM
typedef struct {
    uint16_t erpm;        // 12-bit eRPM
    uint16_t rpm;         // mechanical RPM (with pole_pairs scaling)
    bool valid;           // CRC matched
} dshot_telem_t;

dshot_telem_t dshot_decode(const uint16_t *capture_buf, uint8_t pole_pairs);
```

This function is in the Pure Core (deterministic, host-testable).

### Failure Modes

| Condition                     | Action                                  |
|-------------------------------|-----------------------------------------|
| No GCR response (timeout)     | Mark motor as offline; degrade gracefully |
| GCR CRC mismatch              | Discard reading; use last valid value    |
| eRPM = 0 with throttle > idle | Possible motor stall; counter starts    |
| Stall counter > 100 ms        | Disarm motor; transition to Diag         |
| eRPM far from expected (~30%) | Possible desync; warn but continue       |

## ADC — Battery, Cell, and Current Monitoring

For voltage divider component values and ADC pin assignments,
see [hardware.md](hardware.md). Conversion formulas:

| Measurement    | ADC  | Formula                                                 |
|----------------|------|---------------------------------------------------------|
| Pack voltage   | ADC1 | `pack_mv = (adc_raw * 3300 * 4) / 4095`                |
| Cell 1 voltage | ADC1 | `cell1_mv = (adc_raw * 3300 * 2) / 4095`               |
| Cell 2 voltage | ADC1 | `cell2_mv = (adc_raw * 3300 * 4) / 4095 - cell1_mv`    |
| Cell 3 voltage | --   | `cell3_mv = pack_mv - cell1_mv - cell2_mv` (3S only)   |
| Motor current  | ADC3 | `current_mA = (adc_raw * 3300) / (4095 * 0.01 * 50) * 1000` |

Sample rates:
- Pack + cell ADCs: 10 Hz, 8x hardware oversampling (14-bit effective)
- Motor current ADC: 1 kHz, synchronized with PID loop

For cell voltage thresholds, balance detection, and failsafe actions,
see [safety.md](safety.md).

## SDMMC — MicroSD Card (Blackbox)

| Parameter       | Value                          |
|-----------------|--------------------------------|
| Interface       | SDMMC1 (4-bit SDIO)           |
| Pins            | CK, CMD, D0, D1, D2, D3 + CD (card detect) |
| Bus Width       | 4-bit                          |
| Clock           | 25-50 MHz (High Speed mode)    |
| Throughput      | ~12-25 MB/s                    |
| Filesystem      | FAT32                          |
| Connector       | Push-push MicroSD slot on carrier board |
| DMA             | SDMMC1 IDMA (built-in)        |
| Zephyr Config   | `CONFIG_DISK_DRIVER_SDMMC=y`, `CONFIG_FAT_FILESYSTEM_ELM=y` |

### Write Buffering

PID loop writes to a 32 KB ring buffer in DTCM (DMA-safe, no cache).
A background Zephyr thread drains the ring buffer to SD via SDMMC DMA.
SD garbage collection spikes (~10-100 ms) are absorbed by the ring buffer
(32 KB holds ~2 s of log data at 15 KB/s). PID loop never blocks on SD.

### File Naming Convention

```
/LOG00001.CSV    ← flight 1
/LOG00002.CSV    ← flight 2
...
```

New file created on arm, closed on disarm. File header contains firmware
version, date, battery voltage, and column names. See [features.md](features.md#data-logging-blackbox).

## WS2812B — RGB LED Protocol

| Parameter       | Value                          |
|-----------------|--------------------------------|
| Data Pin        | WS2812_DIN                     |
| Protocol        | Single-wire NRZ                |
| Bit 0           | 0.4 us high + 0.85 us low     |
| Bit 1           | 0.8 us high + 0.45 us low     |
| Reset           | > 50 us low                    |
| Bit Order       | GRB, MSB first                 |
| LEDs            | 1 (single WS2812B on core module) |
| Implementation  | SPI MOSI via DMA (one byte per bit) or TIM PWM + DMA |

Note: Arm LEDs (direction identification) are TBD — pending voltage level
solution for WS2812B on 3.3V system. See quick_note.md.
