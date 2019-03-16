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

test-measure:
	make -f Makefile.test-measure

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
