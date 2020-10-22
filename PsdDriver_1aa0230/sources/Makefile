CROSS_COMPILE ?= ../../software_phoenix/phoenix/toolchain/usr/local/arm-2013.11/bin/arm-linux-gnueabihf-
KPATH ?= ../../software_phoenix/linux-kernel
FUJI_BOARD_TYPE ?= FUJI_BOARD_TYPE_EP
FUJI_CONFIG_TYPE ?= FUJI_CONFIG_TYPE_SYSDEV

ARCH := arm
MAKE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CFILES = psd.c \
psd_battery.c \
psd_deviation.c \
psd_drive.c \
psd_gpio.c \
psd_heart_beat.c \
psd_log.c \
psd_mcu_update.c \
psd_param.c \
psd_park.c \
psd_physics.c \
psd_pwm.c \
psd_six_axis.c \
psd_spi.c \
psd_spi_param.c \
psd_stall.c \
psd_state.c \
psd_syscall.c \
psd_temperature.c \
psd_timer.c \
psd_util.c 

obj-m := PsdDeviceModule.o
PsdDeviceModule-objs := $(CFILES:.c=.o)
ccflags-y += -std=gnu99 -Wall -Wno-declaration-after-statement -I$(MAKE_DIR)../include -D$(FUJI_BOARD_TYPE) -D$(FUJI_CONFIG_TYPE)

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KPATH) M=$(CURDIR) modules
clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KPATH) M=$(CURDIR) clean
