export IDF_PATH := lib/esp-idf
PROJECT_NAME := hatchtrack-peep-fw

EXTRA_COMPONENT_DIRS := \
	bme680

include $(IDF_PATH)/make/project.mk

