#
# Makefile for ALSA
#
ccflags-y += -I$(srctree)/arch/arm/mach-rtk119x/driver

#ccflags-y += -DAO_ON_SCPU

obj-$(CONFIG_SND_ARMAACI)	+= snd-aaci.o
snd-aaci-objs			:= aaci.o

obj-$(CONFIG_SND_PXA2XX_PCM)	+= snd-pxa2xx-pcm.o
snd-pxa2xx-pcm-objs		:= pxa2xx-pcm.o

obj-$(CONFIG_SND_PXA2XX_LIB)	+= snd-pxa2xx-lib.o
snd-pxa2xx-lib-y		:= pxa2xx-pcm-lib.o
snd-pxa2xx-lib-$(CONFIG_SND_PXA2XX_LIB_AC97)	+= pxa2xx-ac97-lib.o

obj-$(CONFIG_SND_PXA2XX_AC97)	+= snd-pxa2xx-ac97.o
snd-pxa2xx-ac97-objs		:= pxa2xx-ac97.o

obj-$(CONFIG_SND_REALTEK)      += snd-realtek-driver.o
snd-realtek-driver-objs     := snd-realtek.o snd-realtek-compress.o snd-realtek_RPC.o snd-realtek-notify.o
