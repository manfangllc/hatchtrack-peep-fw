export IDF_PATH :=  $(realpath ../..)/lib/esp-idf

PROJECT_NAME := hatchtrack-peep-unit-test-fw

EXTRA_COMPONENT_DIRS := \
  $(realpath ../..)/bme680 \
  $(realpath ../..)/icm20602 \
  $(realpath ../..)/hal \
  $(realpath ../..)/peep \
  $(realpath ../..)/protobuf

include $(IDF_PATH)/make/project.mk

CFLAGS += -DPB_BUFFER_ONLY -DPB_FIELD_32BIT=1
CFLAGS += -DPEEP_UNIT_TEST_BUILD

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
