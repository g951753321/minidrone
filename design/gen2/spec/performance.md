# Performance Specification (Gen 2)

## Flight Performance (Target)

| Parameter              | Target Value         | Notes                          |
|------------------------|----------------------|--------------------------------|
| Flight Time (hover)    | ~26 min              | CNHL 3S 850mAh, 141g AUW      |
| Flight Time (mixed)    | ~18 min              | Hover + mild maneuvering       |
| Max Thrust             | ~800 g               | 4x 1404 @ 3S, 3" props        |
| Thrust-to-Weight       | 5.7 : 1              | At 141g AUW — see [mechanical.md](mechanical.md) |
| Max Payload            | ~100 g               | T/W 3.3:1 with payload        |
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

| Parameter                  | Value              |
|----------------------------|--------------------|
| Input Voltage              | 9.0 - 12.6V (3S)  |
| Output Voltage             | 3.3V               |
| Mode                       | Buck               |
| Load Current (typical)     | ~398 mA            |
| Efficiency                 | ~89%               |
| Power Dissipation          | ~163 mW            |
| Input Current from Battery | ~126 mA            |

### Motor Subsystem — 1404 4500KV on 3S

| Condition                   | Current/Motor | Total 4x | Battery Power |
|-----------------------------|---------------|----------|---------------|
| Idle (DSHOT 48)             | 40 mA         | 160 mA   | 1.8 W         |
| Hover (~15% throttle)       | 350 mA        | 1.4 A    | 15.5 W        |
| Sport (~40% throttle)       | 1.2 A         | 4.8 A    | 53.3 W        |
| Full throttle (100%)        | 5.0 A         | 20.0 A   | 222 W         |

### Total System Power (Hover)

| Motor Hover (W) | Digital (W) | Reg Loss (W) | **Total (W)** |
|-----------------|-------------|---------------|---------------|
| 15.5            | 1.31        | 0.16          | **17.0**      |

### Flight Time Estimates

Battery: CNHL 3S 850 mAh, 11.1V = 9.44 Wh (7.55 Wh usable at 80%), AUW ~141 g

| Flight Mode      | Total Power | Flight Time    |
|------------------|-------------|----------------|
| Gentle hover     | 17.0 W      | **26.6 min**   |
| Mixed flying     | 25 W        | **18.1 min**   |
| Aggressive       | 53 W        | **8.5 min**    |

### Summary

| Parameter             | Value        |
|-----------------------|--------------|
| Battery               | CNHL 3S 850mAh 70C |
| AUW                   | ~141 g       |
| Max Thrust            | ~800 g       |
| Thrust-to-Weight      | 5.7 : 1      |
| Max Payload           | ~100 g (T/W 3.3:1) |
| Hover Time            | 26.6 min     |
| Mixed Flight Time     | 18.1 min     |
| Best For              | Indoor / Outdoor |

For detailed weight breakdown, see [mechanical.md](mechanical.md).

### Alternative Battery Options

| Battery               | Energy   | Weight | AUW    | Hover Time | Mixed Time |
|-----------------------|----------|--------|--------|------------|------------|
| CNHL 3S 450mAh 70C   | 5.0 Wh   | 40 g   | ~101 g | 17.6 min   | 12.0 min   |
| **CNHL 3S 850mAh 70C** | **9.44 Wh** | **80 g** | **~141 g** | **26.6 min** | **18.1 min** |
| CNHL 3S 1300mAh 70C  | 14.4 Wh  | 120 g  | ~181 g | 33 min     | 22 min     |

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
