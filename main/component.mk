COMPONENT_PRIV_INCLUDEDIRS := \
	. \
	../bme680 \
	../protobuf \
	../lib/hatchtrack-peep-protobuf/header

CFLAGS += -DPB_BUFFER_ONLY -DPB_FIELD_32BIT=1


