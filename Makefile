export IDF_PATH := ./esp-idf
export PROJECT_NAME := hatchtrack-peep-fw
export CERT_BIN_OFFSET := 0x10000
export CERT_BIN := ./build/nvs_certs.bin
export CERT_CSV := certificates.csv
export CERT_SIZE := 16384
export NVS_PART_GEN_PATH=$(IDF_PATH)/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py

EXTRA_COMPONENT_DIRS := \
  ble \
  bme680 \
  hal \
  iot \
  icm20602 \
  peep \
  OSAL \
  wifi

include $(IDF_PATH)/make/project.mk

test-deep-sleep:
	make -f Makefile.test-deep-sleep

test-measure:
	make -f Makefile.test-measure

test-measure-config:
	make -f Makefile.test-measure-config

test-ble-config:
	make -f Makefile.test-ble-config

cert_flash:
	$(PYTHON) $(NVS_PART_GEN_PATH) --input $(CERT_CSV) --output $(CERT_BIN) --size $(CERT_SIZE)
	$(ESPTOOLPY_WRITE_FLASH) $(CERT_BIN_OFFSET) $(CERT_BIN)

distclean: clean
	rm -f sdkconfig.old
	rm -f sdkconfig
	rm -rf build

gitclean:
	git clean -xfdf
	git submodule foreach --recursive git clean -xfdf
	git reset --hard
	git submodule foreach --recursive git reset --hard
	git submodule update --init --recursive
