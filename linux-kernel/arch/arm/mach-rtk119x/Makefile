ccflags-$(CONFIG_ARCH_RTK119X)	:= -I$(srctree)

obj-$(CONFIG_ARCH_RTK119X)	+= rtk119x.o devices.o rtk_irq_mux.o
obj-$(CONFIG_RTK_SB2_MEM_TRASH_MONITOR)		+= sb2_dbg.o scpu_wrap.o
obj-$(CONFIG_RTK_TIMER)		+= rtk119x_timer_of.o rtk119x_timer.o
ifneq ($(CONFIG_COMMON_CLK_RTK119X),y)
obj-$(CONFIG_RTK_TIMER)                += clock.o
endif
obj-$(CONFIG_ION)			+= driver/rtk_ion_of.o
obj-$(CONFIG_RTK_RPC)		+= driver/RPCkern.o driver/RPCDriver.o driver/RPCintr.o driver/RPCpoll.o
obj-y		+= driver/rtk119x_clock_4K.o
obj-$(CONFIG_RTK_MCP)		+= driver/rtk_mcp.o
obj-$(CONFIG_RTK_MCP)		+= driver/rtk_sha1.o
obj-$(CONFIG_RTK_SE)		+= driver/rtk_se_of.o driver/rtk_se_drv.o
obj-$(CONFIG_RTK_MD)		+= driver/rtk_md_drv.o
obj-$(CONFIG_RTK_KMD)		+= driver/rtk_kmd.o
obj-$(CONFIG_RTK_RTC)		+= driver/rtk_rtc_drv.o
obj-$(CONFIG_RTK_FB)		+= driver/dc2vo/
#obj-$(CONFIG_RTK_FB)		+= driver/rtk_fb.o
obj-$(CONFIG_RTK_BT_RESET)	+= driver/rtk_rfkill.o
obj-$(CONFIG_RTK_FB)		+= driver/android_fb.o

obj-$(CONFIG_REALTEK_AVCPU)		+= driver/avcpu.o
obj-$(CONFIG_REALTEK_IR)	+= driver/irda/venus_ir.o driver/irda/ir_rp.o driver/irda/ir_input.o driver/irda/ir_tx.o
obj-$(CONFIG_RTK_CODEC)		+= driver/rtk_ve/
obj-$(CONFIG_REALTEK_GPIO)	+= driver/gpio_rtk119x.o
obj-$(CONFIG_RTK_VFD)		+= driver/rtk_vfd.o
obj-$(CONFIG_MFD_RTP5901)		+= driver/rtk_rtp5901_of.o
#obj-$(CONFIG_MFD_RTP5901)		+= driver/rtp5901-virtual-dev.o
#obj-$(CONFIG_MFD_RTP5901)		+= driver/rtp5901-virtual.o
obj-$(CONFIG_SMP)		+= platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU)	+= hotplug.o
obj-$(CONFIG_REALTEK_PCIE)	+= driver/rtk_pcie.o

# PM objects
obj-$(CONFIG_SUSPEND)       += suspend.o resume.o rtk119x_ip_power.o

obj-$(CONFIG_RTK_MON_GPIO)	+= vir_input.o

obj-$(CONFIG_RTK_THERMAL)	+= driver/rtk_thermal.o
obj-$(CONFIG_RTK_WATCHDOG)		+= driver/rtk_watchdog.o
obj-$(CONFIG_RTK_V1_ST) 	+= driver/rtk_v1_st.o
