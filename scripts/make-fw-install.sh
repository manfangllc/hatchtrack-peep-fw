#! /bin/sh

# Absolute path to this script, e.g. /home/user/bin/foo.sh
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in, thus /home/user/bin
DIR=$(dirname "$SCRIPT")

COMMIT=$(git rev-parse --short HEAD)

TMP=$DIR/.tmp
NAME=$DIR/../"hatchtrack-peep-fw-install-$COMMIT"
SCRIPT="install.sh"

mkdir $TMP
cp -v $DIR/$SCRIPT $TMP/.
cp -v $DIR/../esp-idf/components/esptool_py/esptool/esptool.py $TMP/.
cp -v $DIR/../build/bootloader/bootloader.bin $TMP/.
cp -v $DIR/../build/partitions.bin $TMP/.
cp -v $DIR/../build/hatchtrack-peep-fw.bin $TMP/.

makeself $TMP $NAME "installer for Hatchtrack Peep firmware" sh ./$SCRIPT
rm -rf $TMP
