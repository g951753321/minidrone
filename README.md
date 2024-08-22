# Minidrone project

This project is a complex project that involves the firmware development, the schematic, and PCB design for a mini drone.

## Firmware development

The firmware development is done using the Zephyr RTOS. The firmware includes the control algorithms for the drone, and the communication with the MicroBlue application.

### Setup Zephyr RTOS

To setup the Zephyr RTOS, follow the instructions in the [Zephyr RTOS documentation](https://docs.zephyrproject.org/latest/getting_started/index.html).

### Project Initialization

To initial the project, clone the repository and run the following commands:

```bash
west init -m https://github.com/g951753321/minidrone.git --mr main minidrone
cd minidrone
west update
```

### Build the project

To build the project, run the following commands:

```bash
west build -b stm32_dev_min minidrone/apps/PID_control
```

The firmware is built for the `stm32_dev_min` board, and the application is `PID_control`.
The output files are located in the `build/zephyr` directory.

## Schematic and PCB design

The schematic/PCB design is done using KiCad. The schematic includes the design of the motor drivers, the microcontroller, and the sensors.
