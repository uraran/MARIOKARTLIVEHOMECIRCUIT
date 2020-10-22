
#obj-m += v4l2_hdmi_dev.o

#OBJS = phoenix_hdmiFun.o  phoenix_hdmiHdmi.o phoenix_hdmiMs.o phoenix_hdmiMux.o phoenix_hdmiOnChipEDIDRx.o phoenix_hdmiPhy.o phoenix_hdmiPlatform.o

#OBJNAME := v4l2_hdmi_dev  
#$(OBJNAME)-objs := v4l2_hdmi_dev.o #phoenix_hdmiFun.o  phoenix_hdmiHdmi.o phoenix_hdmiMs.o phoenix_hdmiMux.o phoenix_hdmiOnChipEDIDRx.o phoenix_hdmiPhy.o phoenix_hdmiPlatform.o
#obj-m := $(OBJNAME).o

#obj-y += v4l2_hdmi_rx.o
#v4l2_hdmi_rx-y := v4l2_hdmi_dev.o #phoenix_hdmiFun.o 

OBJNAME := v4l2_hdmi_rx
$(OBJNAME)-objs := v4l2_hdmi_dev.o phoenix_hdmiFun.o  phoenix_hdmiHdmi.o phoenix_hdmiMs.o phoenix_hdmiMux.o phoenix_hdmiOnChipEDIDRx.o phoenix_hdmiPhy.o phoenix_hdmiPlatform.o phoenix_mipi_wrapper.o phoenix_hdmi_wrapper.o hdmirx_rpc.o phoenix_hdmi_repeater.o phoenix_MHL_CBUS.o phoenix_mipi_i2c_ctl.o camera_ov5645.o
$(OBJNAME)-objs += camera_rle0551c.o camera_ov5647.o camera_ov9782.o
#obj-m	+= v4l2_hdmi_rx.o
obj-$(CONFIG_HDMI_RX)	+= v4l2_hdmi_rx.o

#obj-m += v4l2_hdmi_rx.o
