# Performance Specification

## Flight Performance (Target)

| Parameter              | Target Value       | Notes                     |
|------------------------|--------------------|---------------------------|
| Flight Time            | TBD                | Depends on AUW and motor efficiency |
| Max Payload            | TBD                | Beyond self-weight        |
| PID Tilt Limit         | 45 deg             | PID output saturates; no further tilt commanded |
| Emergency Tilt Cutoff  | 60 deg             | Motors off immediately; transition to Diag |
| Max Altitude           | Indoor use only    | No barometer              |

## Control Loop

| Parameter              | Target Value       | Notes                     |
|------------------------|--------------------|---------------------------|
| IMU Sample Rate        | 1000 Hz            | MPU6050 max rate          |
| PID Loop Rate          | 500 Hz             | 2 ms period               |
| PWM Frequency          | 20 kHz             | Above audible range       |
| PWM Resolution         | ~11.8 bits (3600 steps) | 72 MHz / 20 kHz; MAX_THROTTLE = 3599 |
| Bluetooth Command Rate | 50 Hz              | 20 ms update interval     |

## Response Time

| Event                       | Max Latency     |
|-----------------------------|-----------------|
| IMU read to PID output      | < 1 ms          |
| Bluetooth command to action | < 20 ms         |
| IMU failure detection        | 50 ms (no valid data timeout) |
| Failsafe ramp-down complete  | 200 ms from detection |
| Battery ADC update          | 100 ms          |

## Power Budget (Estimated)

| Component         | Current (mA) | Voltage | Power (mW) |
|-------------------|-------------|---------|------------|
| STM32F103 @ 72MHz | 40          | 3.3V    | 132        |
| MPU6050           | 4           | 3.3V    | 13         |
| HM-13 Bluetooth   | 30          | 3.3V    | 99         |
| 4x Status LEDs    | 20          | 3.3V    | 66         |
| **Digital subtotal** | **94**    | **3.3V** | **310**   |
| 4x Motors (hover)  | TBD        | 7.4V    | TBD        |
| **Total**          | **TBD**    |         | **TBD**    |

## Regulator Thermal

| Parameter                  | Value              |
|----------------------------|--------------------|
| Input Voltage              | 7.4V (nominal)     |
| Output Voltage             | 3.3V               |
| Dropout                    | 4.1V               |
| Load Current (digital)     | ~94 mA             |
| Power Dissipation          | ~385 mW            |
| Thermal Resistance (SOT-223) | ~50 C/W (to PCB) |
| Temperature Rise           | ~19 C              |
