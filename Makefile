export IDF_PATH := ./esp-idf
export PROJECT_NAME := hatchtrack-peep-fw

EXTRA_COMPONENT_DIRS := \
  ble \
  bme680 \
  hal \
  iot \
  icm20602 \
  peep \
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
