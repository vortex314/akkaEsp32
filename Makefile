#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := akkaEsp32
TTY ?= USB0
SERIAL_PORT ?= /dev/tty$(TTY)
ESPPORT = $(SERIAL_PORT)
SERIAL_BAUD = 115200
ESPBAUD = 921600
IDF_PATH ?= /home/lieven/esp/esp-idf
CPPFLAGS += -DWIFI_SSID=${SSID} -DWIFI_PASS=${PSWD} -DESP32_IDF=1 -I../Common -I../microAkka
EXTRA_COMPONENT_DIRS = 

include $(IDF_PATH)/make/project.mk

term:
	rm -f $(TTY)_minicom.log
	minicom -D $(SERIAL_PORT) -b $(SERIAL_BAUD) -C $(TTY)_minicom.log

