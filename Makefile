export IDF_PATH := ./esp-idf
PROJECT_NAME := hatchtrack-peep-fw

EXTRA_COMPONENT_DIRS := \
  bme680 \
  hal \
  iot \
  icm20602 \
  peep \
  protobuf \
  wifi

include $(IDF_PATH)/make/project.mk

CFLAGS += -DPB_BUFFER_ONLY -DPB_FIELD_32BIT=1

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
