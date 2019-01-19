export IDF_PATH := lib/esp-idf
PROJECT_NAME := hatchtrack-peep-fw

EXTRA_COMPONENT_DIRS := \
	bme680 \
	protobuf

include $(IDF_PATH)/make/project.mk

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
