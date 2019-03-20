#! /bin/sh

PORT="/dev/ttyUSB0"

echo "========================"
echo "ESP32 Firmware Installer"
echo " "

echo "performing dependency check"
echo "checking for python..."
if python -c "" &>/dev/null; then
    echo "python installed"
else
    echo "please install python to run this installer"
    echo "========================"
    exit
fi

echo "checking for pyserial..."
if python -c "import serial" &>/dev/null; then
    echo "pyserial installed"
else
    echo "please install pyserial to run this installer"
    echo "========================"
    exit
fi

echo ""
echo "performing configuration"
echo "press 'enter' at prompt to accept default"
read -p "serial port [$PORT]:" input
echo "========================"

python esptool.py --chip esp32 --port $PORT --baud 115200 \
--before "default_reset" --after "hard_reset" \
write_flash -z --flash_mode "dio" --flash_freq "40m" --flash_size detect \
0x1000 ./bootloader.bin \
0x10000 ./hatchtrack-peep-fw.bin \
0x8000 ./partitions.bin
