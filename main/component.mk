COMPONENT_ADD_INCLUDEDIRS = ../../Common ../../../microAkka ../microAkka /home/lieven/workspace/microAkka
COMPONENT_SRCDIRS := . ../../../microAkka
COMPONENT_EXTRA_INCLUDES := ../../../microAkka/src
CXXFLAGS = -fno-rtti -std=gnu++11 -fno-exceptions -I/home/lieven/workspace/microAkka -mlongcalls -fdata-sections -ffunction-sections

COMPONENT_ADD_INCLUDEDIRS := .

COMPONENT_ADD_LDFLAGS += -Wl,--gc-sections 