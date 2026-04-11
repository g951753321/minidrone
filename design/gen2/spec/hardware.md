# Hardware Specification (Gen 2)

## Modular Architecture

The Gen 2 design is split into two boards:

- **Core Module** — MCU, BLE, USB-C, power regulation, flash. Reusable across projects.
- **Carrier Board** — Sensors, ESC, ELRS, battery management. Drone-specific.

Connected via **2x 30-pin castellated pad headers at 1.27 mm pitch**.

```
+-------------------------------+
|       Core Module (40x25mm)   |
|  STM32H743  nRF52  USB-C     |
|  Flash  LDO  SWD  Buttons    |
|  [||||||||||||||||||||||||||] |  ← 2x30 castellated pads (each side)
+-------------------------------+
         ↕ 1.27mm headers (dev) or solder-down (production)
+-------------------------------+
|      Carrier Board (50x50mm)  |
|  IMU  Baro  ToF  OF  ELRS    |
|  ESC pads  Battery  LC filter |
|  LED  Buzzer  Companion hdr   |
+-------------------------------+
```

### Core-to-Carrier Connector

| Parameter           | Value                          |
|---------------------|--------------------------------|
| Type                | Castellated pads (dual-use)    |
| Pitch               | 1.27 mm                        |
| Pins                | 2x 30-pin, dual-row, two edges = **120 pads total** |
| Dev Mode            | Solder 1.27 mm pin headers onto module; plug into sockets on carrier (e.g., Samtec SSQ series) |
| Production Mode     | Solder module directly to carrier via castellated pads |
| Mated Height (dev)  | ~8 mm (with socket)            |
| Mated Height (prod) | 0 mm (flush solder)            |

### Pin Allocation on Connector

| Group             | Pins | Signals (exposed on castellated pads)          |
|-------------------|------|-------------------------------------------------|
| SPI1              | 4    | SCK, MISO, MOSI, CS1                           |
| SPI2              | 4    | SCK, MISO, MOSI, CS2                           |
| SPI4              | 4    | SCK, MISO, MOSI, CS3                           |
| I2C1              | 2    | SDA, SCL                                        |
| I2C2              | 2    | SDA, SCL                                        |
| USART1            | 2    | TX, RX                                          |
| USART3            | 2    | TX, RX                                          |
| UART4             | 2    | TX, RX                                          |
| UART7             | 2    | TX, RX                                          |
| TIM1 CH1-4        | 4    | DSHOT / PWM outputs                             |
| TIM3 CH1-4        | 4    | Aux PWM outputs                                 |
| ADC1 (x4)         | 4    | Battery V, Cell1 V, Cell2 V, spare              |
| ADC3 (x4)         | 4    | Motor current x4 (via op-amps on carrier)       |
| EXTI (x6)         | 6    | Interrupt-capable GPIO                          |
| GPIO (x10)        | 10   | General purpose I/O                             |
| BLE USART2 TX/RX  | 2    | nRF52 passthrough (from core module)            |
| nRF52 Reset       | 1    | BLE module reset                                |
| USB D+/D-         | 2    | USB passthrough (optional, if carrier needs connector) |
| +3V3 OUT          | 4    | Regulated 3.3V from core module LDO             |
| +VBAT IN          | 4    | Battery voltage input to core module             |
| GND               | 16   | Ground (distributed for return current)          |
| NRST              | 1    | MCU reset                                       |
| BOOT0             | 1    | Boot mode select                                |
| **Total**         | **120** |                                              |

## Core Module

### Microcontroller — STM32H743VIH6

| Parameter         | Value                        |
|-------------------|------------------------------|
| MCU               | STM32H743VIH6                |
| Core              | ARM Cortex-M7 (dual-issue, 6-stage pipeline) |
| FPU               | **Double-precision** hardware FPU |
| Flash             | **2 MB** (dual-bank)         |
| SRAM              | **1 MB** (DTCM 128K + AXI 512K + SRAM1-4) |
| I-Cache / D-Cache | 16 KB / 16 KB                |
| Max Clock         | **480 MHz**                  |
| Package           | LQFP-100 (14 x 14 mm)       |
| Operating Voltage | 1.62V - 3.6V                |
| USB               | USB 2.0 FS + **HS** (with internal PHY) |
| ADC               | 3x **16-bit** 3.6 Msps      |
| DAC               | 2x 12-bit                    |
| Timers            | Advanced (TIM1, TIM8), GP (TIM2-5, TIM12-17) |
| DMA               | 2x DMA controller + MDMA     |
| RNG               | Hardware random number generator |

### Core Module Components

| Component               | Part                    | Location    |
|-------------------------|-------------------------|-------------|
| MCU                     | STM32H743VIH6           | Center      |
| BLE Module              | MDBT42Q (nRF52832)      | Edge (antenna outward) |
| USB-C Connector         | USB 2.0 receptacle      | Edge        |
| ESD Protection          | USBLC6-2SC6             | Near USB-C  |
| LDO (module power)      | AP2112K-3.3 (600mA)    | Near MCU    |
| HSE Crystal             | 25 MHz (SMD 2520)       | Near MCU    |
| LSE Crystal             | 32.768 kHz (SMD 3215)   | Near MCU    |
| SWD Header              | 1x4 pin, 1.27mm pitch   | Edge        |
| Reset Button            | Tactile (SW1)           | Edge        |
| Boot/DFU Button         | Tactile (SW2)           | Edge        |
| Power LED               | Green, 0402             | Edge        |
| Status LED              | WS2812B (x1)           | Edge        |
| Decoupling              | Per STM32H743 datasheet | Near MCU    |

### Core Module Power

| Parameter            | Value                              |
|----------------------|------------------------------------|
| Input Voltage        | +3V3 from carrier **or** USB VBUS (5V) |
| Module LDO           | AP2112K-3.3 (from USB VBUS)       |
| Power OR             | Schottky diode OR: carrier 3.3V or LDO output |
| Module Consumption   | ~350 mA max (MCU 300mA + BLE 10mA + LED 20mA) |
| Standalone Operation | Yes — powered from USB-C alone (for dev without carrier) |

Note: When on carrier, carrier supplies 3.3V via TPS63070. Module LDO is
bypassed (diode OR, carrier voltage wins). When standalone, USB VBUS powers
LDO → 3.3V for module only.

### Core Module PCB

| Parameter         | Value                              |
|-------------------|------------------------------------|
| Dimensions        | 40 x 25 mm                         |
| Layers            | 4 (Signal / GND / Power / Signal)  |
| Edge Pads         | 2x 30-pin castellated, 1.27mm pitch, both long edges |
| Solder Mask       | Both sides, matte black             |

## Carrier Board (Drone Application)

## IMU — ICM-42688-P

| Parameter           | Value                      |
|---------------------|----------------------------|
| Interface           | SPI (up to 24 MHz)         |
| Accel Range         | +/- 16g (configurable)     |
| Gyro Range          | +/- 2000 deg/s (configurable) |
| Gyro Noise Density  | 2.8 mdps/rtHz              |
| Accel Noise Density | 70 ug/rtHz                 |
| ODR (max)           | 32 kHz (gyro), 32 kHz (accel) |
| On-chip FIFO        | 2 KB                       |
| Package             | LGA-14 (2.5 x 3.0 mm)     |
| Supply              | 1.71V - 3.6V               |
| Current             | 0.8 mA (gyro + accel, low-noise mode) |
| INT1 Pin            | Data-ready interrupt to MCU EXTI |
| Decoupling          | 100 nF on VDD, 1 uF on VDDIO |

### Placement Requirements

- Mount at PCB center of gravity
- Minimum 5 mm clearance from motor traces and power planes
- Local ground pour stitched with vias beneath IC
- No copper pour on layer directly beneath sensor (reduce stress)

## Barometer — BMP390

| Parameter           | Value                      |
|---------------------|----------------------------|
| Interface           | I2C (up to 3.4 MHz)       |
| I2C Address         | 0x77 (SDO = HIGH)         |
| Pressure Range      | 300 - 1250 hPa            |
| Relative Accuracy   | +/- 0.03 hPa              |
| Absolute Accuracy   | +/- 0.5 hPa               |
| Temperature Accuracy| +/- 0.5 C                 |
| ODR                 | Up to 200 Hz              |
| Package             | LGA-10 (2.0 x 2.0 mm)    |
| Supply              | 1.71V - 3.6V              |
| Current             | 0.7 mA (normal mode)      |
| Decoupling          | 100 nF on VDD             |

### Placement Requirements

- Shield from direct airflow (place under foam or cover with tape)
- Keep away from heat sources (regulator, motor drivers)
- Vent hole in enclosure for pressure equalization

## Time-of-Flight — VL53L5CX

| Parameter           | Value                      |
|---------------------|----------------------------|
| Interface           | I2C (up to 1 MHz)         |
| I2C Address         | 0x29 (default)             |
| Range               | 20 mm - 4000 mm           |
| Zones               | 4x4 or 8x8 multi-zone    |
| Accuracy            | +/- 5 mm (typical)        |
| Ranging Rate        | Up to 60 Hz               |
| FoV                 | 63 deg (diagonal)         |
| Package             | Module 6.4 x 3.0 x 1.5 mm|
| Supply              | 2.8V (from 3.3V via LDO) or 3.3V direct |
| Current             | 18 mA (ranging active)    |
| INT Pin             | Ranging complete interrupt |
| Decoupling          | 100 nF + 4.7 uF on AVDD  |

### Placement Requirements

- Mount on bottom side of PCB, facing down
- Clear optical path (no obstructions within 63 deg FoV cone)
- Cover glass must be clean and flush with PCB bottom surface

## Optical Flow — PMW3901

| Parameter           | Value                      |
|---------------------|----------------------------|
| Interface           | SPI (up to 2 MHz)         |
| Resolution          | 30 x 30 pixel array       |
| Frame Rate          | Up to 121 fps              |
| Optimal Height      | 80 mm - 1 m               |
| Motion Output       | Delta-X, Delta-Y (int16)  |
| Package             | Module ~15 x 20 mm        |
| Supply              | 1.8V - 3.6V               |
| Current             | 8 mA (tracking active)    |
| Motion Pin          | Motion detected interrupt  |
| Decoupling          | 100 nF on VDD             |

### Placement Requirements

- Mount on bottom side of PCB, facing down
- Lens must have clear view of ground
- Minimum 80 mm altitude for valid readings
- Co-locate with VL53L5CX for fused altitude + position

## BLE Module — MDBT42Q (nRF52832)

| Parameter           | Value                      |
|---------------------|----------------------------|
| Chipset             | nRF52832 (Cortex-M4F)     |
| BLE Version         | 5.0                        |
| Interface to STM32  | UART (up to 1 Mbps)       |
| Antenna             | On-module PCB antenna      |
| TX Power            | -20 to +4 dBm             |
| Range               | ~30 m (indoor)             |
| Package             | Module 16 x 10 x 2.2 mm   |
| Supply              | 1.7V - 3.6V               |
| Current             | 5 mA (TX), 5.4 mA (RX)   |
| Additional Pins     | Reset (from STM32 GPIO)   |
| Decoupling          | 100 nF + 10 uF on VDD     |

### Functions

- BLE GATT server for telemetry streaming
- BLE DFU bootloader for OTA firmware updates (both nRF52 and STM32)
- Configuration interface (PID gains, flight parameters)
- Blackbox data download

### Placement Requirements

- Place at PCB edge with antenna facing outward
- Minimum 10 mm from IMU (reduce RF interference on analog sensors)
- Ground plane keepout under antenna area per module datasheet

## ELRS Receiver

| Parameter           | Value                      |
|---------------------|----------------------------|
| Module              | BetaFPV ELRS Lite RX or HappyModel EP2 |
| Protocol            | ExpressLRS 3.x              |
| Frequency           | 2.4 GHz                    |
| Packet Rate         | 50 / 150 / 250 / 500 Hz   |
| Latency             | 2 - 5 ms                   |
| Interface           | UART (CRSF protocol, 420000 baud) |
| Supply              | 3.3V (or 5V, module dependent) |
| Current             | ~40 mA                     |
| Weight              | 0.5 - 0.9 g               |
| Antenna             | Ceramic chip or wire dipole|
| Size                | ~10 x 10 mm (Lite RX)     |

### Mounting

- Solder pads: 3V3, GND, TX, RX
- Antenna should be away from motors and carbon fiber frame
- Secure with double-sided tape or solder directly to FC pads

## Motor System

### Brushless Motors — 1404 4500KV

| Parameter           | Value                      |
|---------------------|----------------------------|
| Motor Class         | 1404                        |
| KV Rating           | 4500 KV                    |
| Stator              | 14 mm dia x 4 mm height   |
| Max Thrust (3S, 3") | ~200 g per motor           |
| Weight              | ~8.5 g per motor           |
| Shaft               | 1.5 mm                     |
| Max Current          | 5 A per motor             |
| Connector           | JST-PH 1.25 mm 3-pin or solder pads |
| Recommended         | BetaFPV 1404 4500KV, FlyFishRC Flash 1404 4500KV |

For propeller specs, see [mechanical.md](mechanical.md).

### 4-in-1 ESC

| Parameter           | Value                      |
|---------------------|----------------------------|
| Type                | 4-in-1 integrated ESC board |
| Firmware            | **BlueJay** (required for bidirectional DSHOT300) |
| Protocol            | Bidirectional DSHOT300      |
| Continuous Current  | 6 A per channel            |
| Burst Current       | 10 A per channel           |
| MOSFET              | Integrated on ESC board    |
| Supply              | **3S LiPo (9.0 - 12.6V)** |
| Size                | ~20 x 20 mm mounting       |
| Weight              | ~2-3 g                     |
| Voltage Rating      | >= 13V                     |

Alternative: discrete motor driver ICs (4x DRV8323 or equivalent, up to 60V)
integrated on main PCB if a custom ESC is preferred.

### Motor Layout

| Motor | Label | Position     | Rotation | DSHOT Channel |
|-------|-------|-------------|----------|---------------|
| M1    | FR    | Front Right | CW       | TIM1_CH1      |
| M2    | FL    | Front Left  | CCW      | TIM1_CH2      |
| M3    | BR    | Back Right  | CCW      | TIM1_CH3      |
| M4    | BL    | Back Left   | CW       | TIM1_CH4      |

## Power Supply

### Battery — 3S LiPo

| Parameter            | Value                              |
|----------------------|------------------------------------|
| Model                | CNHL MiniStar 850mAh 3S 70C       |
| Configuration        | 3S (3 cells series)                |
| Nominal Voltage      | 11.1V                              |
| Full Charge Voltage  | 12.6V                              |
| Cutoff Voltage       | 9.0V (3.0V per cell)              |
| Capacity             | 850 mAh                           |
| Discharge Rate       | 70C continuous / 140C burst        |
| Energy               | 9.44 Wh                           |
| Main Connector       | XT60                               |
| Balance Connector    | JST-XH 4-pin                      |
| Dimensions           | 62 x 25 x 30 mm                   |
| Weight               | ~80 g (with connector)             |

### Balance Connector and Cell Monitoring

A JST-XH balance connector exposes individual cell voltages for both
**external balance charging** and **per-cell monitoring**.

| Parameter        | Value                                       |
|------------------|---------------------------------------------|
| Connector        | JST-XH 4-pin (3S standard)                 |
| Pinout           | GND, CELL1+, CELL2+, CELL3+ (= +VBAT)      |
| Position         | Carrier board edge, near XT60 connector     |
| Standard         | Industry-standard LiPo balance port         |
| Use 1            | Plug into external balance charger (primary charging method) |
| Use 2            | Per-cell ADC monitoring (always active)    |

### Per-Cell Voltage Monitoring

Cell voltages are read via resistor dividers connected to STM32H743 ADC channels.
Only 2 ADC channels are needed: cell 1 directly, cell 2 cumulative (cell 1 + cell 2).
For 3S, cell 3 is computed as `pack_voltage - cell1 - cell2`.

| Cell    | Source              | ADC Channel       | Divider          | Max Input | ADC Voltage |
|---------|---------------------|-------------------|------------------|-----------|-------------|
| CELL1   | JST-XH pin 2 (cell1+)| ADC1_INx (CELL1) | 10K / 10K (0.5)  | 4.4V      | 2.2V        |
| CELL2   | JST-XH pin 3 (cell1+cell2 sum)| ADC1_INy (CELL2) | 30K / 10K (0.25) | 8.8V | 2.2V    |
| CELL3   | (computed for 3S)    | --                | --               | --        | --          |
| Pack    | +VBAT                | ADC1_INz (VBAT)  | 30K / 10K (0.25) | 13.1V     | 3.28V       |

Conversion:
```
cell1_mv  = (adc1 * 3300 * 2) / 4095
cell12_mv = (adc2 * 3300 * 4) / 4095          // cumulative cell1 + cell2 voltage
cell2_mv  = cell12_mv - cell1_mv
cell3_mv  = pack_mv - cell12_mv               // 3S only
```

Cell1 and cell2 measured directly via ADC; cell3 computed from pack voltage.

### Cell Monitoring Features

The MCU uses cell voltages for:

| Feature              | Description                                      |
|----------------------|--------------------------------------------------|
| Imbalance detection  | Refuse arm if max-min cell delta > 100 mV       |
| Per-cell low voltage | Critical/cutoff thresholds applied per-cell, not just pack |
| Telemetry            | Per-cell voltage reported via BLE and CRSF      |
| Health logging       | Blackbox logs per-cell voltage at 10 Hz         |
| Battery age estimate | Track cycle count and voltage sag patterns       |

For threshold values, see [safety.md](safety.md#low-battery-thresholds-per-cell-applies-to-1s--2s--3s).

### Battery Voltage Validation

The firmware validates 3S battery voltage at boot:

| Measured Voltage  | State           | Action                    |
|-------------------|-----------------|---------------------------|
| 9.0V - 13.1V     | 3S OK           | Normal operation          |
| < 9.0V           | No battery / depleted | USB-only power; refuse arm |

Validation runs once at boot before calibration. All battery thresholds
are expressed as per-cell voltages (3.0V cutoff, 3.3V critical, 3.5V warning)
and applied to the 3-cell pack.

### Power Domains

| Domain  | Voltage          | Source              | Loads                                |
|---------|------------------|---------------------|--------------------------------------|
| +VBAT   | 9.0 - 12.6V     | Battery direct (via XT60)| ESC / Motors, VTX power         |
| +3V3    | 3.3V             | TPS63070 output     | MCU, IMU, Baro, ToF, OF, BLE, ELRS, Flash, LED, Buzzer |
| +VUSB   | 5.0V             | USB-C VBUS          | Core module LDO (standalone power), USB peripheral |
| GND     | 0V               | Common ground       | All                                  |

Note: No onboard battery charging. Use an external balance charger via the
JST-XH balance connector for 3S batteries.

### Buck-Boost Regulator — TPS63070

| Parameter            | Value                              |
|----------------------|------------------------------------|
| Topology             | Buck-boost (step up/down)          |
| Input Range          | **2.0V - 16V**                     |
| Output               | 3.3V (set by resistor divider)     |
| Max Output Current   | 2 A                               |
| Efficiency           | ~93% (typical, varies with Vin)   |
| Switching Frequency  | 2.4 MHz                            |
| Package              | VQFN-14 (3.5 x 3.5 mm)           |
| Inductor             | 1.5 uH (shielded, low DCR, 3A rated) |
| Input Capacitor      | 10 uF x2 MLCC (X5R/X7R, **25V rated**) |
| Output Capacitor     | 22 uF x2 MLCC (X5R/X7R)          |
| Feedback Divider     | R_TOP: 1M, R_BOT: 470K (3.3V out) |

Note: TPS63070 operates in buck mode for 3S (input 9.0-12.6V, always > 3.3V output).

### Battery Charging — External Only

No onboard battery charger. Charging is performed via an external LiPo
balance charger plugged into the JST-XH balance connector. This is the
standard practice for 2S/3S drones and avoids the weight and complexity of
onboard multi-cell charging.

USB-C is still used for:
- Programming (DFU and SWD via separate header)
- Debug serial (USB CDC)
- Powering the core module standalone (without battery)

### USB-C Connector

| Parameter            | Value                              |
|----------------------|------------------------------------|
| Connector            | USB Type-C 2.0 receptacle (mid-mount or SMD) |
| ESD Protection       | USBLC6-2SC6 (SOT-23-6)            |
| CC Resistors         | 2x 5.1K to GND (UFP, device mode) |
| Functions            | USB DFU, USB CDC serial, battery charging |

### Power Switch — High-Side P-FET

| Parameter            | Value                              |
|----------------------|------------------------------------|
| Switch Type          | P-channel MOSFET (e.g., **DMP3010LK3**) |
| Vds Rating           | **-30V** (sufficient for 3S: 12.6V) |
| Control              | SW2 slide switch (gate control)    |
| Rds(on)              | < 35 mohm                          |
| Features             | Clean power cutoff, no contact resistance issues |
| Status LED           | Green LED downstream of switch (power indicator) |

### Reverse Polarity Protection

| Parameter            | Value                              |
|----------------------|------------------------------------|
| Method               | P-channel MOSFET body diode        |
| MOSFET               | Same DMP3010LK3 (**-30V rated**)   |
| Voltage Drop         | < 30 mV at load (Rds(on) path)    |

### Overcurrent Protection

| Parameter            | Value                              |
|----------------------|------------------------------------|
| Device               | Resettable PTC fuse                |
| Rating               | **5A hold / 10A trip**             |
| Package              | 1210 SMD                           |
| Location             | Between battery connector and power switch |

Note: PTC rated for worst-case 3S full-throttle current. 4 motors can draw
~12 A peak; the 10 A trip protects against short circuits while allowing
brief burst peaks (PTC has thermal time constant).

### Decoupling — Core Module

| Capacitor       | Value    | Package | Purpose                        |
|-----------------|----------|---------|--------------------------------|
| C_VDD (x8)      | 100 nF  | 0402    | MCU VDD bypass (each VDD pin)  |
| C_VDDA          | 1 uF + 100 nF | 0402 | MCU analog supply            |
| C_VCAP (x2)     | 2.2 uF  | 0402    | H743 internal LDO (VCAP pins) |
| C_LDO_IN        | 10 uF   | 0603    | AP2112K input                  |
| C_LDO_OUT       | 10 uF   | 0603    | AP2112K output                 |
| C_BLE           | 100 nF + 10 uF | 0402/0603 | nRF52 module VDD          |
| C_USB           | 100 nF  | 0402    | USB VBUS decoupling            |

### Decoupling — Carrier Board

| Capacitor       | Value    | Package | Purpose                        |
|-----------------|----------|---------|--------------------------------|
| C_IN_BUCK (x2)  | 10 uF   | 0805 25V | Buck-boost input             |
| C_OUT_BUCK (x2) | 22 uF   | 0805    | Buck-boost output              |
| C_IMU           | 100 nF + 1 uF | 0402 | ICM-42688 VDD + VDDIO       |
| C_BARO          | 100 nF  | 0402    | BMP390 VDD                     |
| C_TOF           | 100 nF + 4.7 uF | 0402/0603 | VL53L5CX AVDD           |

## Data Storage

STM32H743 has 2 MB dual-bank internal flash. Bank 2 (1 MB) is used for
blackbox logging, eliminating the need for external SPI flash.

| Parameter           | Value                      |
|---------------------|----------------------------|
| Blackbox Storage    | 1 MB (internal flash bank 2)|
| Page Size           | 256 bytes (flash word = 32 bytes) |
| Sector Size         | 128 KB                     |
| Endurance           | 10K erase cycles per sector |
| Write Speed         | ~3.5 MB/s (via internal bus)|

Note: If more blackbox capacity is needed, an external W25Q128 SPI flash
(16 MB) can be added on the carrier board via SPI4. The core module exposes
SPI4 on the castellated pads for this purpose.

## Oscillators (on Core Module)

| Crystal | Frequency       | Package       | Load Capacitors    | Purpose          |
|---------|----------------|---------------|--------------------|------------------|
| Y1      | 25 MHz (HSE)   | SMD 2520-4Pin | 2x 10 pF (0402)   | PLL → 480 MHz    |
| Y2      | 32.768 kHz (LSE)| SMD 3215-2Pin | 2x 6.8 pF (0402) | USB SOF, RTC     |

Note: H743 requires HSE for USB HS PHY. 25 MHz chosen for clean PLL
multiplication to 480 MHz system clock and 48 MHz USB clock.

## PCB — Core Module

| Parameter         | Value                              |
|-------------------|------------------------------------|
| Dimensions        | 40 x 25 mm                         |
| Layers            | 4 (Signal / GND / Power / Signal)  |
| Dielectric        | FR4, 1.0 mm total                  |
| Copper Thickness  | 35 um (1 oz) outer, 17.5 um (0.5 oz) inner |
| Min Track Width   | 0.15 mm (6 mil)                    |
| Min Clearance     | 0.15 mm (6 mil)                    |
| Edge Pads         | Castellated half-holes, 1.27 mm pitch |
| Solder Mask       | Both sides, matte black            |

### Core Module Design Rules

- H743 LQFP-100 at board center; decoupling caps within 2 mm of VDD pins
- VCAP caps immediately adjacent to VCAP1/VCAP2 pins (critical for internal LDO)
- BLE module antenna at board edge with ground keepout
- USB-C at short edge for easy access
- SWD and buttons at opposite short edge
- 25 MHz HSE crystal within 10 mm of OSC_IN/OSC_OUT
- Continuous GND plane on L2

## PCB — Carrier Board

| Parameter         | Value                              |
|-------------------|------------------------------------|
| Dimensions        | 50 x 50 mm (max)                  |
| Layers            | 4 (Signal / GND / Power / Signal)  |
| Dielectric        | FR4, 1.6 mm total                  |
| Copper Thickness  | 35 um (1 oz) outer, 17.5 um (0.5 oz) inner |
| Min Track Width   | 0.15 mm (6 mil)                    |
| Mounting Holes    | 4x M2 at 20 x 20 mm pattern (ESC stack) |
| Module Socket     | 2x 30-pin SMD socket pads, 1.27mm pitch |

### Carrier Board Track Width Classes

| Width        | Typical Use                             |
|-------------|------------------------------------------|
| 0.15 mm     | Fine signal (SPI, I2C, GPIO)             |
| 0.25 mm     | Standard signal                          |
| 0.50 mm     | 3.3V rail (up to 1 A)                   |
| 1.50 mm     | Per-motor power trace (up to 5 A)        |
| **Copper pour** | **Battery main +VBAT / GND (>5 A)**  |

Battery rail (+VBAT, GND) must use **copper pours on L1 + L4** stitched with
power vias every 2-3mm. Do not use single traces for battery or ESC input —
3S at full throttle draws up to 20 A peak (4 × 5 A).

### Carrier Board Design Rules

- Core module socket at board center-top
- IMU (ICM-42688-P) at board center (near CG)
- Minimum 10 mm between module BLE antenna and IMU
- Battery/ESC power: copper pour on L1 + L4, **not single traces**
- TPS63070 + LC filter near XT60 connector
- Barometer covered with foam; isolated from motor airflow
- GPS + camera connectors at board edge (JST-GH)
- Companion computer header at board edge
- ToF + Optical Flow on bottom side, facing down

### Carrier GPS Connector (Removable Module)

| Parameter         | Value                              |
|-------------------|------------------------------------|
| Connector         | JST-GH 4-pin, 1.25 mm pitch       |
| Signals           | 3V3, GND, TX (UART4), RX (UART4)  |
| Baud Rate         | 38400 (default, configurable)      |
| Protocol          | NMEA / UBX (auto-detect)           |
| Recommended Module| HGLRC M100 Mini (2.8g, u-blox M10, 15x15mm) |
| Alternative       | Flywoo GM10 Mini V3 (3.5g, M10 + compass, 20x20mm) |
| Mounting          | Velcro or 3D-printed pylon on top of frame |

When GPS is not installed, the connector adds <0.3g. The firmware auto-detects
GPS presence (NMEA sentence received within 2s of boot). If no GPS is detected,
GPS-dependent features (position hold outdoor, RTH) are disabled gracefully.

### Carrier Camera / VTX Connector (Removable Module)

| Parameter         | Value                              |
|-------------------|------------------------------------|
| Power Pads        | +VBAT (direct battery, for VTX), GND |
| Signal Connector  | JST-GH 3-pin: 3V3, TX (UART3), RX (UART3) |
| UART Purpose      | VTX control (SmartAudio / MSP DisplayPort) |
| Recommended       | HDZero Whoop Lite VTX (4.5g) + Nano90 camera (2.5g) = 7g total |
| Alternative       | Analog AIO (Caddx Ant Lite, ~3g total) |
| Alternative       | ESP32-CAM (WiFi streaming to phone, ~5g) |
| Mounting          | VTX on frame top plate; camera on front tilt mount |

The VTX draws power directly from +VBAT (9-12.6V, within HDZero/analog VTX input range).
The UART provides SmartAudio control (change channel, power) or MSP DisplayPort
for OSD overlay. When no VTX is installed, the pads add <0.2g.

### Carrier Companion Computer Header

| Parameter         | Value                              |
|-------------------|------------------------------------|
| Connector         | 1x4 pin header, 2.54 mm pitch     |
| Signals           | 3V3, GND, TX (UART7), RX (UART7)  |
| Baud Rate         | Configurable (default 921600)      |
| Purpose           | Future RPi Zero / ESP32 companion  |
| Protocol          | MAVLink or custom binary           |
