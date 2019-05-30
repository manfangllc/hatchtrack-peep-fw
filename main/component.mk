COMPONENT_PRIV_INCLUDEDIRS := \
  . \
  ../ble \
  ../bme680 \
  ../hal \
  ../iot \
  ../peep \
  ../wifi

COMPONENT_EMBED_TXTFILES := \
  uuid.txt \
  root_ca.txt \
  cert.txt \
  key.txt

ifeq ($(PROJECT_NAME),hatchtrack-peep-unit-test-fw)
  # The following line is needed to force the linker to include all the object
  # files into the application, even if the functions in these object files
  # are not referenced from outside (which is usually the case for unit tests).
  COMPONENT_ADD_LDFLAGS = -Wl,--whole-archive -l$(COMPONENT_NAME) -Wl,--no-whole-archive
endif
