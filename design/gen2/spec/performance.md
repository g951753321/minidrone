# Performance Specification (Gen 2)

## Flight Performance (Target)

| Parameter              | Target Value         | Notes                          |
|------------------------|----------------------|--------------------------------|
| Flight Time (hover)    | >= 10 min            | All configs meet this target   |
| Flight Time (mixed)    | >= 7 min             | Hover + mild maneuvering       |
| Max Thrust (2S)        | ~280 g               | 4x 1103 @ 2S, 3" props        |
| Max Thrust (3S)        | ~480 g               | 4x 1204 @ 3S, 3" props        |
| Thrust-to-Weight (2S)  | 3.9 : 1              | At 71g AUW — see [mechanical.md](mechanical.md) |
| Thrust-to-Weight (3S)  | 5.5 : 1              | At 87g AUW                     |
| Max Payload (beyond AUW)| 10 - 50 g           | Config dependent; maintains 2:1 T/W |
| PID Tilt Limit         | 45 deg               | PID output saturates           |
| Emergency Tilt Cutoff  | 60 deg               | Motors off immediately          |
| Max Altitude (indoor)  | 4 m                  | Limited by VL53L5CX ToF range  |
| Max Altitude (outdoor) | ~30 m                | Barometer limited, no GPS      |
| Position Hold Accuracy | +/- 20 cm            | Optical flow, indoor, calm air |
| Altitude Hold Accuracy | +/- 15 cm            | Baro + ToF fusion              |

## Control Loop

| Parameter              | Target Value         | Notes                          |
|------------------------|----------------------|--------------------------------|
| IMU Sample Rate        | 8000 Hz              | ICM-42688 via SPI + INT1       |
| Rate PID Loop          | 4000 Hz              | Inner loop (gyro rate)         |
| Angle PID Loop         | 1000 Hz              | Outer loop (attitude angle)    |
| Altitude Loop          | 50 Hz                | Baro + ToF fusion update       |
| Position Loop          | 50 Hz                | Optical flow update            |
| DSHOT Output Rate      | 4000 Hz              | Matches rate PID loop          |
| CRSF Input Rate        | 50 - 500 Hz          | Depends on ELRS packet rate    |
| BLE Telemetry Rate     | 10 Hz                | Non-critical, background       |
| Blackbox Log Rate      | 1000 Hz (configurable)| Matches angle PID loop        |

### Cascaded PID Architecture

```
ELRS Stick Input
      |
      v
 +----------+    +----------+    +--------+    +-------+
 | Setpoint  |--->| Angle    |--->| Rate   |--->| Motor |
 | Mapping   |    | PID      |    | PID    |    | Mixer |
 |           |    | (1 kHz)  |    | (4 kHz)|    |       |
 +----------+    +----------+    +--------+    +-------+
                      ^               ^             |
                      |               |             v
                 IMU Angle        IMU Rate      DSHOT Out
               (complementary    (gyro raw)    (4x motors)
                filter)
```

## Response Time

| Event                         | Max Latency     |
|-------------------------------|-----------------|
| IMU read to rate PID output   | < 0.25 ms       |
| IMU read to angle PID output  | < 1 ms          |
| CRSF frame to PID setpoint    | < 2 ms          |
| CRSF failsafe detection       | 500 ms (no valid frame) |
| IMU failure detection          | 10 ms (no INT1 pulse) |
| Failsafe ramp-down complete   | 200 ms from detection |
| Battery ADC update             | 100 ms          |
| Motor current ADC update       | 1 ms (per PID loop) |
| Altitude sensor update         | 20 ms           |
| Optical flow update            | 20 ms           |
| BLE command to action          | < 50 ms         |
| USB DFU: full flash (128 KB)  | < 5 s           |

## Power Budget

### Digital Subsystem (3.3V Rail)

| Component              | Current (mA) | Power (mW) |
|------------------------|-------------|------------|
| STM32H743 @ 480 MHz   | 300         | 990        |
| ICM-42688-P (LN mode)  | 0.8         | 2.6        |
| BMP390 (normal mode)   | 0.7         | 2.3        |
| VL53L5CX (ranging)     | 18          | 59         |
| PMW3901 (tracking)     | 8           | 26         |
| nRF52 BLE (TX active)  | 10          | 33         |
| ELRS Lite RX           | 40          | 132        |
| WS2812B LED (white)    | 20          | 66         |
| Buzzer (active)        | 30          | 99         |
| **Subtotal (typical)** | **~398**    | **~1313**  |
| **Subtotal (max)**     | **~428**    | **~1412**  |

### Buck Regulator (TPS63070)

| Parameter                  | 2S               | 3S               |
|----------------------------|-------------------|-------------------|
| Input Voltage              | 6.0 - 8.7V       | 9.0 - 12.6V      |
| Output Voltage             | 3.3V              | 3.3V              |
| Mode                       | Buck              | Buck              |
| Load Current (typical)     | ~398 mA           | ~398 mA           |
| Efficiency                 | ~93%              | ~89%              |
| Power Dissipation          | ~99 mW            | ~163 mW           |
| Input Current from Battery | ~192 mA           | ~126 mA           |

### Motor Subsystem (Direct Battery)

#### 2S Configuration (1103 motors, 11000 KV)

| Condition                   | Current/Motor | Total 4x | Battery Power |
|-----------------------------|---------------|----------|---------------|
| Idle (DSHOT 48)             | 40 mA         | 160 mA   | 1.2 W         |
| Hover (~25% throttle)       | 400 mA        | 1.6 A    | 11.8 W        |
| Sport (~50% throttle)       | 1.2 A         | 4.8 A    | 35.5 W        |
| Full throttle (100%)        | 3.0 A         | 12.0 A   | 88.8 W        |

#### 3S Configuration (1204 motors, 5000 KV)

| Condition                   | Current/Motor | Total 4x | Battery Power |
|-----------------------------|---------------|----------|---------------|
| Idle (DSHOT 48)             | 30 mA         | 120 mA   | 1.3 W         |
| Hover (~20% throttle)       | 300 mA        | 1.2 A    | 13.3 W        |
| Sport (~45% throttle)       | 1.0 A         | 4.0 A    | 44.4 W        |
| Full throttle (100%)        | 3.5 A         | 14.0 A   | 155.4 W       |

### Total System Power (Hover)

| Config | Motor Hover (W) | Digital (W) | Reg Loss (W) | Total (W) |
|--------|-----------------|-------------|---------------|-----------|
| **2S** | 11.8            | 1.31        | 0.10          | **13.2**  |
| **3S** | 13.3            | 1.31        | 0.16          | **14.8**  |

Note: 3S hover power is higher due to heavier AUW (heavier battery + motors).
However, thrust margin is much greater, enabling outdoor flight in wind.

### Flight Time Estimates (All Configurations)

#### 2S: 450 mAh, 7.4V = 3.33 Wh (2.66 Wh usable), AUW ~71 g

| Flight Mode      | Total Power | Flight Time    |
|------------------|-------------|----------------|
| Gentle hover     | 12.3 W      | **13.0 min**   |
| Mixed flying     | 18.0 W      | **8.9 min**    |
| Aggressive       | 36.0 W      | **4.4 min**    |

#### 3S: 450 mAh, 11.1V = 5.0 Wh (4.0 Wh usable), AUW ~87 g

| Flight Mode      | Total Power | Flight Time    |
|------------------|-------------|----------------|
| Gentle hover     | 14.8 W      | **16.2 min**   |
| Mixed flying     | 22.0 W      | **10.9 min**   |
| Aggressive       | 45.0 W      | **5.3 min**    |

### Configuration Comparison Summary

| Parameter             | 2S          | 3S           |
|-----------------------|-------------|--------------|
| AUW                   | ~71 g       | ~87 g        |
| Thrust-to-Weight      | 3.9 : 1     | 5.5 : 1      |
| Hover Time            | 13.0 min    | 16.2 min     |
| Mixed Time            | 8.9 min     | 10.9 min     |
| Wind Resistance       | Moderate    | Good         |
| Best For              | In/Outdoor  | Outdoor      |
| Charging              | External balance charger | External balance charger |
| Recommended           | **Default** | Power users  |

For detailed weight breakdown, see [mechanical.md](mechanical.md#weight-budget-per-configuration).
For battery options and capacities, see [hardware.md](hardware.md#battery-2s---3s-support).

### Battery Options (Per Configuration)

#### 2S Options

| Battery               | Energy   | Weight | Hover Time | Mixed Time |
|-----------------------|----------|--------|------------|------------|
| 2S 300 mAh 75C       | 2.22 Wh  | 20 g   | 10.8 min   | 7.4 min    |
| **2S 450 mAh 75C**   | 2.66 Wh  | 30 g   | 13.0 min   | 8.9 min    |
| 2S 650 mAh 75C       | 3.85 Wh  | 42 g   | 15.2 min   | 10.2 min   |

#### 3S Options

| Battery               | Energy   | Weight | Hover Time | Mixed Time |
|-----------------------|----------|--------|------------|------------|
| 3S 300 mAh 75C       | 3.33 Wh  | 30 g   | 13.5 min   | 8.5 min    |
| **3S 450 mAh 75C**   | 4.00 Wh  | 40 g   | 17.4 min   | 10.9 min   |
| 3S 650 mAh 75C       | 5.77 Wh  | 55 g   | 19.0 min   | 11.8 min   |

## Regulator Thermal

| Parameter                  | Value              |
|----------------------------|--------------------|
| Regulator                  | TPS63070           |
| Input Voltage              | 2.0 - 16V         |
| Output Voltage             | 3.3V               |
| Efficiency (worst case 3S) | ~88%               |
| Load Current               | ~398 mA            |
| Power Dissipation (worst)  | ~180 mW (3S input, ~88% eff) |
| Thermal Resistance         | ~45 C/W (VQFN-14 to PCB) |
| Temperature Rise           | ~8 C               |

Note: Acceptable thermal. Higher than G431 design but well within TPS63070's 2A capability.

## Processing Budget

<!-- STM32H743 @ 480 MHz, Cortex-M7 dual-issue superscalar -->

| Task                    | CPU Cycles (est.) | Period  | CPU Load |
|-------------------------|-------------------|---------|----------|
| Rate PID (3-axis)       | 1000              | 0.25 ms | 0.83%    |
| Angle PID (3-axis)      | 1500              | 1 ms    | 0.31%    |
| IMU SPI read + process  | 500               | 0.125 ms| 0.83%    |
| Motor mixer + DSHOT DMA | 300               | 0.25 ms | 0.25%    |
| CRSF parse              | 200               | 2 ms    | 0.02%    |
| Baro read + altitude    | 800               | 20 ms   | 0.008%   |
| Optical flow read       | 400               | 20 ms   | 0.004%   |
| BLE UART TX             | 200               | 100 ms  | 0.0004%  |
| Blackbox flash write    | 500               | 1 ms    | 0.10%    |
| Battery ADC             | 100               | 100 ms  | 0.0002%  |
| LED update              | 200               | 50 ms   | 0.0008%  |
| **Total**               |                   |         | **~3%**  |

Headroom: ~97% CPU idle at 480 MHz. Enables future EKF sensor fusion, computer vision preprocessing, and companion computer protocol handling.
