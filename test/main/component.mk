COMPONENT_PRIV_INCLUDEDIRS := \
  ../unity/include \
  .

COMPONENT_ADD_LDFLAGS = -Wl,--whole-archive -l$(COMPONENT_NAME) -Wl,--no-whole-archive
