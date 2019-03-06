#! /bin/sh

BASE=$(dirname $0)

ESPTOOL=$(realpath $BASE/../esp-idf/components/esptool_py/esptool/esptool.py)
PORT=/dev/ttyUSB0

BOOT_FW=$(realpath $BASE/../build/bootloader/bootloader.bin)
MAIN_FW=$(realpath $BASE/../build/hatchtrack-peep-fw.bin)
PART_FW=$(realpath $BASE/../build/partitions.bin)

python $ESPTOOL --chip esp32 --port $PORT --baud 115200 \
--before "default_reset" --after "hard_reset" \
write_flash -z --flash_mode "dio" --flash_freq "40m" --flash_size detect \
0x1000 $BOOT_FW \
0x10000 $MAIN_FW \
0x8000 $PART_FW
