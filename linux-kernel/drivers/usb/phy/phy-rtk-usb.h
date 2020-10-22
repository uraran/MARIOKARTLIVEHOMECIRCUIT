#ifndef __PHY_RTK_USB_H__
#define __PHY_RTK_USB_H__

struct rtk_usb_phy_s {
	struct usb_phy phy;
	struct device *dev;
};

struct rtk_usb_phy_data_s {
	char addr;
	char data;
};


#endif /* __PHY_RTK_USB_H__ */
