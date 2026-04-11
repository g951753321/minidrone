# Mechanical Specification (Gen 2)

## Design Constraints

| Parameter          | Value                        |
|--------------------|------------------------------|
| Max Body Size      | 200 mm (W) x 200 mm (L), excluding propellers |
| Target AUW         | 141 g                              |
| Configuration      | Quadrotor (X-config)         |
| Use Environment    | Indoor / calm outdoor        |

## Frame

| Parameter          | Value                        |
|--------------------|------------------------------|
| Configuration      | Quadrotor X-frame             |
| Material           | 3D-printed (TPU/PLA) or carbon fiber plate |
| Motor-to-motor     | 130 - 150 mm (diagonal)      |
| Arm Width          | 8 - 10 mm                    |
| Arm Thickness      | 3 - 4 mm                     |
| Center Stack       | 20 x 20 mm M2 mounting pattern (standard FC/ESC stack) |
| Prop Guard         | Optional snap-on TPU guards   |

### Frame Layout (Top View)

```
             200 mm max (body)
        +-------------------------+
        |                         |
        |    FL            FR     |     Prop circle (76 mm / 3")
        |     \   ______   /     |     extends beyond body
        |      \ | PCB  | /      |
        |       \| 50x50|/       |
        |        |______|        |
        |       /        \       |
        |      /          \      |
        |    BL            BR    |
        |                         |
        +-------------------------+
```

### Frame Options

| Option | Material        | Weight | Pros                        | Cons                      |
|--------|----------------|--------|-----------------------------|---------------------------|
| A      | 3D-printed PLA | ~8 g   | Easy to iterate, cheap      | Brittle, heavier          |
| B      | 3D-printed TPU | ~6 g   | Flexible, crash resistant   | Less rigid, slight vibration |
| C      | CF plate (1 mm)| ~5 g   | Lightest, stiffest          | Harder to manufacture, RF blocking |
| D      | PCB-integrated | 0 g    | No separate frame needed    | PCB must be large enough for arms |

Recommended: **Option B (TPU)** for development, **Option C (CF)** for final version.

If PCB is designed as a full unibody (Option D), arms extend from PCB edges
with motor pads at tips. This eliminates frame weight entirely but limits
PCB shape to the airframe outline.

## Motor Layout

```
        Front
    FL -------- FR
    |            |
    |    PCB     |
    |   (stack)  |
    BL -------- BR
         Back
```

For motor labels (M1-M4), rotation directions, and DSHOT channel assignments,
see [hardware.md](hardware.md#motor-layout).

## Propellers

| Parameter       | Value                        |
|-----------------|------------------------------|
| Size            | 76 mm (3 inch)               |
| Blade Count     | 2-blade or 3-blade           |
| Material        | Polycarbonate                |
| Mounting        | Press-fit on 1 mm shaft      |
| Weight          | ~1 g per prop (2-blade)      |
| Recommended     | HQProp 3x1.5x3 or Gemfan 3018 |

Note: 2.5" props (65 mm) are an alternative for tighter body constraint.
Trade-off: less thrust but fits more easily within 200 mm body limit.

## Motor Specifications

For motor electrical specs (KV, current, thrust), see
[hardware.md](hardware.md#brushless-motors--1404-4500kv).

| Motor Class | Shaft   | Weight/Motor |
|------------|---------|-------------|
| 1404       | 1.5 mm  | ~8.5 g      |

## Battery

For battery electrical specs, see [hardware.md](hardware.md#battery--3s-lipo).

| Model              | Capacity | Dimensions (mm) | Weight |
|--------------------|----------|-----------------|--------|
| CNHL MiniStar 3S   | 850 mAh | 62 x 25 x 30   | ~80 g  |

### Battery Mounting

Battery mounts underneath the PCB (center of gravity aligned with thrust center).
Secured by:
- Silicone rubber band (lightweight, quick-swap)
- Or 3D-printed battery tray integrated into frame
- XT60 connector accessible from rear for easy swap
- Battery bay must accommodate 62 x 25 x 30 mm

## Weight Budget

### Common Components

| Component               | Count | Unit Weight (g) | Total (g) |
|-------------------------|-------|-----------------|-----------|
| Core + Carrier PCBs     | 2     | -               | 12        |
| 4-in-1 ESC (20x20 stack)| 1    | 2.5             | 2.5       |
| ELRS Lite RX            | 1     | 0.5             | 0.5       |
| Frame (TPU 3D-print)    | 1     | 6               | 6         |
| Wiring / solder / misc  | --    | --              | 2         |
| **Common subtotal**     |       |                 | **23**    |

### Drone-Specific Components

| Component               | Weight      |
|-------------------------|-------------|
| 1404 motors (x4)        | 34 g        |
| 3" propellers (x4)      | 4 g         |
| CNHL 3S 850mAh battery  | 80 g        |
| **Subtotal**            | **118 g**   |

### Total AUW

| Common | Drone | **Total AUW** | T/W Ratio | T/W with 100g payload |
|--------|-------|---------------|-----------|----------------------|
| 23 g   | 118 g | **141 g**     | 5.7 : 1   | 3.3 : 1              |

### Weight Optimization Paths

| Change                        | Savings |
|-------------------------------|---------|
| CF frame (replace TPU)        | ~1-2 g  |
| Discrete ESC on carrier PCB   | ~1.5 g  |
| Lighter battery (450 mAh)    | ~40 g (AUW ~101g, T/W 7.9:1) |

## Center of Gravity

| Requirement              | Specification                  |
|--------------------------|--------------------------------|
| CG horizontal            | Within 5 mm of geometric center|
| CG vertical              | Below motor plane (battery underneath helps) |
| IMU placement            | At CG or within 3 mm          |

Battery position is the primary CG adjustment tool. Slide battery forward/back
to balance.

## Vibration Management

| Source                   | Mitigation                     |
|--------------------------|--------------------------------|
| Motor vibration          | Soft-mount motors (silicone O-rings on motor screws) |
| Prop imbalance           | Use factory-balanced props; replace damaged props |
| IMU vibration isolation  | Gyro notch filter in firmware (ICM-42688 provides); optional foam pad under IMU if on breakout board |
| Frame resonance          | TPU frame absorbs vibration; CF frame may need motor soft-mounting |

## Connectors

### Core Module
| Ref  | Type                    | Pitch   | Purpose              |
|------|------------------------|---------|----------------------|
| J1   | USB-C receptacle       | --      | Programming, USB CDC, standalone power |
| J2   | PinHeader 1x04 vert    | 1.27 mm | SWD debug            |
| EDGE | Castellated pads x120  | 1.27 mm | Carrier interface (2x 30-pin, both edges) |

### Carrier Board
| Ref  | Type                    | Pitch   | Purpose              |
|------|------------------------|---------|----------------------|
| J3   | XT60 connector         | --      | Battery main power (3S)    |
| J4   | JST-XH 1x04            | 2.5 mm  | Battery balance port (3S)  |
| J7   | JST-GH 1x04            | 1.25 mm | GPS module (removable)     |
| J8   | JST-GH 1x03 + VBAT pads| 1.25 mm | Camera / VTX (removable)   |
| J9   | Push-push MicroSD slot  | --      | Blackbox logging (FAT32)   |
| J5   | PinHeader 1x04 vert    | 2.54 mm | Companion computer UART |
| J6   | Module socket pads     | 1.27 mm | Core module mounting (2x 30-pin) |
| M1-4 | Motor pads (solder)    | --      | Brushless motors (3 pads each) |
| ELRS | Solder pads (4-pin)    | 1.27 mm | ELRS receiver        |

Note: JST-XH balance port serves both per-cell ADC monitoring (always active)
and external balance charging (when plugged into LiPo charger).

## Assembly Stack-Up

```
Side view (not to scale):

    Propellers
    -----------      <- Props press-fit on motor shafts
    |  Motors  |     <- Motors mounted on frame arms
    ===========      <- Frame (TPU/CF arms)
    |   PCB    |     <- Main flight controller PCB
    |  (ESC)   |     <- 4-in-1 ESC (20x20 stack under PCB, or integrated)
    -----------
    | Battery  |     <- Battery strapped underneath
    -----------
    [ToF] [OF]       <- Bottom-facing sensors (VL53L5CX + PMW3901)
```

### Stack Height Budget

| Layer                | Height (mm) |
|----------------------|-------------|
| Propeller clearance  | 3           |
| Motor height         | 6           |
| Frame arm            | 4           |
| PCB + ESC stack      | 8           |
| Battery              | 7           |
| Bottom sensors       | 3           |
| **Total**            | **~31 mm**  |
