# Mechanical Specification

## Frame

| Parameter          | Value               |
|--------------------|----------------------|
| Configuration      | Quadrotor (X-config) |
| Motor Mount        | 2-pin horizontal headers (M1-M4) |
| Motor Type         | Brushed DC coreless  |
| Propeller          | TBD (matched to motor) |

## Motor Layout

```
        Front
    FL -------- FR
    |            |
    |    PCB     |
    |            |
    BL -------- BR
         Back
```

| Position     | Ref | Rotation   |
|-------------|-----|------------|
| Front Left  | M4  | CCW        |
| Front Right | M3  | CW         |
| Back Left   | M2  | CCW        |
| Back Right  | M1  | CW         |

## Battery

| Parameter       | Value                        |
|-----------------|------------------------------|
| Type            | LiPo                         |
| Configuration   | 2S (series)                  |
| Cell Voltage    | 3.7V nominal / 4.2V max      |
| Capacity        | 560 mAh per cell             |
| Connector       | J1, J5 (2-pin horizontal headers) |

## Weight Budget (Estimated)

| Component          | Weight (g) |
|--------------------|------------|
| PCB assembly       | TBD        |
| 2x LiPo battery   | TBD        |
| 4x Motors          | TBD        |
| 4x Propellers      | TBD        |
| Frame              | TBD        |
| Wiring / connectors| TBD        |
| **Total**          | **TBD**    |

## Connectors

| Ref  | Type                  | Pitch  | Purpose              |
|------|----------------------|--------|----------------------|
| J1, J5 | PinHeader 1x02 horiz | 2.54mm | Battery              |
| J2   | PinSocket 1x04 vert  | 2.54mm | HM-13 Bluetooth      |
| J3   | PinHeader 1x06 vert  | 2.54mm | Programming (UART)   |
| J4   | PinSocket 1x04 vert  | 2.54mm | MPU6050 IMU          |
| J6   | PinHeader 1x02 vert  | 2.54mm | Placeholder          |
| J7   | PinHeader 2x04 vert  | 2.54mm | Expansion            |
| J8   | PinHeader 1x04 horiz | 2.54mm | J-Link SWD           |
| J9, J10 | PinHeader 1x20 vert | 2.54mm | MCU pin breakout   |
| J11  | PinHeader 1x02 vert  | 2.54mm | Jumper               |
| M1-M4 | PinHeader 1x02 horiz | 2.54mm | Motor connectors   |
