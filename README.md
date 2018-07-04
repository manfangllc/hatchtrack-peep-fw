# hatchtrack-peep-fw

ESP32 Firmware for the Hatchtrack Peep product.

## Design

The firmware takes advtange of the ESP32's deep sleep design which performs
a system reset after the sleep is complete. This allows firmware to boot into
a specific state, execute the state's code, sleep, and reset into the next
state.

There are currently three states, where two of them are essentially the same
but leverage different peripherals. The first state is the CONFIG state which
utilizes either BLE or UART to configure WiFi, AWS, and other working
parameters. The other state is the MEASURE state where the configured parameters
are used to take an environmental measurement and then push the data up into the
cloud.

## Requirements

An ESP32 toolchain is required to build the firmware. The project is configured
to find it located in the /opt directory (/opt/xtensa-esp32-elf), but the path
can be altered by running 'make menuconfig'.
