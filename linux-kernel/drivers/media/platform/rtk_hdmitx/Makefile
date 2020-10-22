#
# Makefile for HDMITX linux kernel module.
#
ccflags-y += -I$(srctree)/arch/arm/mach-rtk119x/driver
ccflags-y += -I$(srctree)/drivers/media/platform/rtk_mhltx

#ccflags-y += -DAO_ON_SCPU

hdmitx-$(CONFIG_RTK_HDMITX):= hdmitx_core.o switch_hdmitx.o hdmitx_api.o rtk_edid.o
obj-$(CONFIG_RTK_HDMITX)+= hdmitx.o

