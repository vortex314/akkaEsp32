COMPONENT_ADD_INCLUDEDIRS = ../../Common ../../../microAkka  
COMPONENT_SRCDIRS := . ../../../microAkka
COMPONENT_EXTRA_INCLUDES := ../../../microAkka/src  ../../../ArduinoJson
CXXFLAGS =  -fno-rtti -std=gnu++11 -I/home/lieven/workspace/microAkka -mlongcalls -fdata-sections -ffunction-sections

COMPONENT_ADD_INCLUDEDIRS := .

COMPONENT_ADD_LDFLAGS += -Wl,--gc-sections 

COMMENT := -fno-rtti