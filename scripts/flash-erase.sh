#! /bin/sh

BASE=$(dirname $0)

ESPTOOL=$(realpath $BASE/../esp-idf/components/esptool_py/esptool/esptool.py)
PORT=/dev/ttyUSB0

python $ESPTOOL --chip esp32 --port $PORT --baud 115200 erase_flash
