#!/bin/sh

if [ "$#" -ne 6 ]; then
  echo "$0 [$UUID] [WIFI-SSID] [WIFI-PASS] [ROOT-CA] [CERT] [KEY]"
  exit 1
fi

BASE=$(dirname $0)
UUID=$1
SSID=$2
PASS=$3
ROOTCA=$4
CERT=$5
KEY=$6

cd $(realpath $BASE/..)

printf "%s" "$UUID" > ./main/uuid.txt
printf "%s" "$SSID" > ./main/wifi_ssid.txt
printf "%s" "$PASS" > ./main/wifi_pass.txt
cat $ROOTCA > ./main/root_ca.txt
cat $CERT > ./main/cert.txt
cat $KEY > ./main/key.txt

make
