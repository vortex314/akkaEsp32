#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#
# ex. : touch main.cpp
#       make flash term DEFINE="-DPROGRAMMER=1 -DHOSTNAME=prog"
#
DEFINE ?= -DAAAAA=BBBBBB
PROJECT_NAME := akkaEsp32
TTY ?= USB0
DEFINE ?= -DAAAAA=BBBBBB
SERIAL_PORT ?= /dev/tty$(TTY)
ESPPORT = $(SERIAL_PORT)
SERIAL_BAUD = 115200
ESPBAUD = 921600
IDF_PATH ?= /home/lieven/esp/esp-idf
WORKSPACE := /home/lieven/workspace
DEFINES := -DWIFI_SSID=${SSID} -DWIFI_PASS=${PSWD}  -DESP32_IDF=1 $(DEFINE) 
CPPFLAGS +=  $(DEFINES)  -I../Common -I../microAkka 
CPPFLAGS +=  -I$(WORKSPACE)/ArduinoJson/src -I $(IDF_PATH)/components/freertos/include/freertos 
CXXFLAGS +=  $(DEFINES)  -I../Common -I../microAkka 

CXXFLAGS +=  -I$(WORKSPACE)/ArduinoJson/src -I $(IDF_PATH)/components/freertos/include/freertos 
CXXFLAGS +=  -fno-rtti -ffunction-sections -fdata-sections 
EXTRA_COMPONENT_DIRS = 
# LDFLAGS += -Wl,--gc-sections

include $(IDF_PATH)/make/project.mk

REMOTE :
	touch main/main.cpp
	make DEFINE="-DREMOTE=1 -DHOSTNAME=remote -DMQTT_HOST=limero.ddns.net" 
	
DRIVE_WIFI :
	touch main/main.cpp
	make DEFINE="-DMOTORSPEED=1 -DHOSTNAME=drive"
	
DRIVE :
	touch main/main.cpp
	make DEFINE="-DMOTORSPEED=1 -DMOTORSERVO=2 -DHOSTNAME=drive -DMQTT_SERIAL" 
	
TAG :
	touch main/main.cpp
	make DEFINE="-DDWM1000_TAG=1 -DMQTT_SERIAL"
	
COMPASS :
	touch main/main.cpp
	make DEFINE="-DDIGITAL_COMPASS=1"
	
SERIAL :
	touch main/main.cpp
	make DEFINE="-DMQTT_SERIAL" 

term:
	rm -f $(TTY)_minicom.log
	minicom -D $(SERIAL_PORT) -b $(SERIAL_BAUD) -C $(TTY)_minicom.log
