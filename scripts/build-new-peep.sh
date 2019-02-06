#!/bin/sh

if [ -z "$1" ]; then
  echo "$0 [WIFI-SSID] [WIFI-PASS]"
  exit 1
fi

if [ -z "$2" ]; then
  echo "$0 [WIFI-SSID] [WIFI-PASS]"
  exit 1
fi

BASE=$(dirname $0)
SSID=$1
PASS=$2

cd $(realpath $BASE/..)

printf "%s" "$(uuidgen)" > ./main/uuid.txt
printf "%s" "$SSID" > ./main/wifi_ssid.txt
printf "%s" "$PASS" > ./main/wifi_pass.txt

make
