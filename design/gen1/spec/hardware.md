# Hardware Specification

## Microcontroller

| Parameter         | Value                        |
|-------------------|------------------------------|
| MCU               | STM32F103CBTx                |
| Core              | ARM Cortex-M3                |
| Flash             | 128 KB                       |
| SRAM              | 20 KB                        |
| Max Clock         | 72 MHz                       |
| Package           | LQFP-48                      |
| Operating Voltage | 2.0V - 3.6V                  |

## Power Supply

| Parameter            | Value                              |
|----------------------|------------------------------------|
| Battery              | 2S LiPo (2x 3.7V 560mAh series)   |
| Nominal Voltage      | 7.4V                               |
| Full Charge Voltage  | 8.4V                               |
| Cutoff Voltage       | 6.0V (3.0V per cell)               |
| Voltage Regulator    | LM1117MP-3.3 (SOT-223)             |
| Regulated Output     | 3.3V                               |
| Max Regulator Output | 800 mA                             |
| Power Switch         | SW3 (DPDT slide switch, CK JS202011CQN) |

### Power Domains

| Domain  | Voltage | Source          | Loads                        |
|---------|---------|-----------------|------------------------------|
| +BATT   | 7.4V    | Battery pack    | Motors, LM1117 input         |
| +3.3V   | 3.3V    | LM1117 output   | MCU, Bluetooth, IMU, LEDs    |
| GND     | 0V      | Common ground   | All                          |

### Decoupling

| Capacitor     | Value   | Package | Purpose                     |
|---------------|---------|---------|------------------------------|
| C1, C3, C4, C6 | 10 uF | 1210    | Bulk decoupling (power rails) |
| C2, C5, C7, C9-C12 | 0.1 uF | 0603 | Bypass decoupling (MCU VDD pins) |
| C8            | 1 uF    | 0603    | Regulator output stability   |

## Motor Drive

| Parameter         | Value                        |
|-------------------|------------------------------|
| MOSFET            | SI2302 (N-channel)           |
| Quantity          | 4 (Q1-Q4)                   |
| Package           | SOT-23                       |
| V_DS max          | 20V                          |
| I_D max           | 2.6A                         |
| Gate Drive Voltage| 3.3V (from STM32 GPIO)       |
| Gate Pull-Down    | 100K to GND (REQUIRED per safety spec) |
| Flyback Diode     | 1N4007 (D5-D8, SMA package) |
| Motor Connectors  | M1 (BR), M2 (BL), M3 (FR), M4 (FL) |

### Motor PWM Signals

| Motor | Label   | Description      |
|-------|---------|------------------|
| M1    | BR_PWM  | Back Right       |
| M2    | BL_PWM  | Back Left        |
| M3    | FR_PWM  | Front Right      |
| M4    | FL_PWM  | Front Left       |

## Oscillators

| Crystal | Frequency   | Package          | Load Capacitors |
|---------|-------------|------------------|-----------------|
| Y1      | 8 MHz (HSE) | SMD 3225-4Pin    | C13, C15: 20 pF |
| Y2      | 32.768 KHz (LSE) | SMD 3215-2Pin | C14, C16: 12 pF |

## PCB

| Parameter         | Value                        |
|-------------------|------------------------------|
| Layers            | 2 (F.Cu, B.Cu)               |
| Dielectric        | FR4, 1.51 mm                 |
| Copper Thickness  | 35 um (1 oz)                 |
| Min Track Width   | 0.254 mm (10 mil)            |
| Min Clearance     | 0.127 mm (5 mil)             |
| Min Via Drill     | 0.3 mm                       |
| Min Via Diameter  | 0.4 mm                       |
| Solder Mask       | Both sides                   |

### Track Width Classes

| Width (mm) | Typical Use              |
|------------|--------------------------|
| 0.254      | Signal traces            |
| 0.508      | Power traces (3.3V)      |
| 1.27       | Battery / motor power    |
| 2.0        | High current paths       |
| 2.54       | Main power bus           |

### Via Sizes

| Diameter (mm) | Drill (mm) | Use                 |
|---------------|------------|---------------------|
| 0.4           | 0.3        | Signal vias         |
| 0.6           | 0.3        | Standard vias       |
| 1.0           | 0.6        | Power vias          |
