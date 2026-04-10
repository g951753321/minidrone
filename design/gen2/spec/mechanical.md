# Mechanical Specification (Gen 2)

## Design Constraints

| Parameter          | Value                        |
|--------------------|------------------------------|
| Max Body Size      | 200 mm (W) x 200 mm (L), excluding propellers |
| Target AUW         | 71 g (2S) / 87 g (3S)              |
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

For motor electrical specs (KV, current, thrust) and recommended models per
battery configuration, see [hardware.md](hardware.md#brushless-motors-per-battery-configuration).

Physical dimensions for weight budget:

| Config | Motor Class | Shaft   | Weight/Motor |
|--------|------------|---------|-------------|
| 2S     | 1103       | 1 mm    | ~3.5 g      |
| 3S     | 1204       | 1.5 mm  | ~5 g        |

## Battery

For battery electrical specs (voltage, capacity, connector type, cell count
auto-detection), see [hardware.md](hardware.md#battery-2s---3s-support).

Physical dimensions and weight for budget:

| Config | Capacity | Dimensions (mm)   | Weight |
|--------|----------|-------------------|--------|
| 2S     | 450 mAh | ~55 x 20 x 14    | ~30 g  |
| 3S     | 450 mAh | ~55 x 20 x 20    | ~40 g  |

### Battery Mounting

Battery mounts underneath the PCB (center of gravity aligned with thrust center).
Secured by:
- Silicone rubber band (lightweight, quick-swap)
- Or 3D-printed battery tray integrated into frame
- Battery connector orientation: accessible from rear for easy swap
- 2S/3S batteries are larger; frame battery bay must accommodate ~55 x 20 x 20 mm

## Weight Budget (Per Configuration)

### Common Components (All Configs)

| Component               | Count | Unit Weight (g) | Total (g) |
|-------------------------|-------|-----------------|-----------|
| Core + Carrier PCBs     | 2     | -               | 12        |
| 4-in-1 ESC (20x20 stack)| 1    | 2.5             | 2.5       |
| ELRS Lite RX            | 1     | 0.5             | 0.5       |
| Frame (TPU 3D-print)    | 1     | 6               | 6         |
| Wiring / solder / misc  | --    | --              | 2         |
| **Common subtotal**     |       |                 | **23**    |

### Configuration-Specific Components

| Component               | 2S           | 3S           |
|-------------------------|-------------|--------------|
| Motors (x4)             | 1103, 14 g  | 1204, 20 g   |
| Propellers (x4)         | 3", 4 g     | 3", 4 g      |
| Battery                 | 450mAh, 30 g| 450mAh, 40 g |
| **Config subtotal**     | **48 g**    | **64 g**     |

### Total AUW

| Configuration | Common | Config | **Total AUW** | T/W Ratio |
|---------------|--------|--------|---------------|-----------|
| **2S**        | 23 g   | 48 g   | **71 g**      | 3.9 : 1   |
| **3S**        | 23 g   | 64 g   | **87 g**      | 5.5 : 1   |

### Weight Optimization Paths

| Change                        | Savings | Applies To |
|-------------------------------|---------|------------|
| CF frame (replace TPU)        | ~1-2 g  | All        |
| PCB unibody (no frame)        | ~6 g    | All        |
| Discrete ESC on PCB           | ~1.5 g  | All        |
| Lighter 2S battery (300 mAh) | ~10 g   | 2S         |

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
| J3   | XT30 connector         | --      | Battery main power (2S/3S) |
| J4   | JST-XH 1x04            | 2.5 mm  | Battery balance port (2S/3S) |
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
