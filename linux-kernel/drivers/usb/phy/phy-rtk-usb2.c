#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/usb/otg.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include "phy-rtk-usb.h"

#define RTK_USB2PHY_NAME "rtk-usb2phy"

static void __iomem *REG_WRAP_VStatusOut2;
static void __iomem *REG_GUSB2PHYACC0;

#define WAIT_VBUSY_RETRY	3

#define REG_USB_PHY_C0	0xC0
#define REG_USB_PHY_C1	0xC1
#define REG_USB_PHY_C2	0xC2
#define REG_USB_PHY_C3	0xC3
#define REG_USB_PHY_C4	0xC4
#define REG_USB_PHY_C5	0xC5
#define REG_USB_PHY_C6	0xC6
#define REG_USB_PHY_C7	0xC7

#define REG_USB_PHY_D0	0xD0
#define REG_USB_PHY_D1	0xD1
#define REG_USB_PHY_D2	0xD2
#define REG_USB_PHY_D3	0xD3
#define REG_USB_PHY_D4	0xD4
#define REG_USB_PHY_D5	0xD5
#define REG_USB_PHY_D6	0xD6
#define REG_USB_PHY_D7	0xD7

#define USB_ST_BUSY		BIT(23)
#define phy_write(addr, val)	do { smp_wmb(); __raw_writel(val, addr); } while(0)
#define phy_read(addr)			__raw_readl(addr)

static struct rtk_usb_phy_data_s phy_page0_default_setting[] = {
	{0xe0, 0xa1},
	{0xe2, 0x9a},
	{0xe4, 0xd6},
	{0xe5, 0x1d},
	{0xe6, 0xc0},
	{0xe7, 0xc1},
	{0xf1, 0x9c},
};

static struct rtk_usb_phy_data_s phy_page1_default_setting[] = {
	{0xe0, 0x35},
	{0xe1, 0xaf},
};

int utmi_wait_register(void __iomem *reg, u32 mask, u32 result);

static char rtk_usb_phy_read(char addr)
{
	volatile unsigned int regVal;

	// polling until VBusy == 0
	utmi_wait_register(REG_GUSB2PHYACC0, USB_ST_BUSY, 0);

	// VCtrl = low nibble of addr, VLoadM = 1
	regVal = BIT(25) | 	// Port num
			 ((addr & 0x0f) << 8);	// vcontrol
	phy_write(REG_GUSB2PHYACC0, regVal);
	utmi_wait_register(REG_GUSB2PHYACC0, USB_ST_BUSY, 0);

	// VCtrl = high nibble of addr, VLoadM = 1
	regVal = BIT(25) | 	// Port num
			 ((addr & 0xf0) << 4);	// vcontrol
	phy_write(REG_GUSB2PHYACC0, regVal);
	utmi_wait_register(REG_GUSB2PHYACC0, USB_ST_BUSY, 0);

	smp_rmb();
	regVal = phy_read(REG_GUSB2PHYACC0);

	return (char) (regVal & 0xff);
}
static int rtk_usb_phy_write(char addr, char data)
{
	volatile unsigned int regVal;

	//write data to VStatusOut2 (data output to phy)
	phy_write(REG_WRAP_VStatusOut2, (u32)data);
	utmi_wait_register(REG_GUSB2PHYACC0, USB_ST_BUSY, 0);

	// VCtrl = low nibble of addr, VLoadM = 1
	regVal = BIT(25) |
			 ((addr & 0x0f) << 8);

	phy_write(REG_GUSB2PHYACC0, regVal);
	utmi_wait_register(REG_GUSB2PHYACC0, USB_ST_BUSY, 0);

	// VCtrl = high nibble of addr, VLoadM = 1
	regVal = BIT(25) |
			 ((addr & 0xf0) << 4);

	phy_write(REG_GUSB2PHYACC0, regVal);
	utmi_wait_register(REG_GUSB2PHYACC0, USB_ST_BUSY, 0);

	return 0;
}

static int rtk_usb_phy_set_page(int page)
{
	return rtk_usb_phy_write(0xf4, page == 0 ? 0x9b : 0xbb);
}

static void rtk_usb2_phy_shutdown(struct usb_phy *phy)
{
	/* Todo */
}

static int rtk_usb2_phy_init(struct usb_phy *phy)
{
	int i;

	/* Set page 0 */
	rtk_usb_phy_set_page(0);

	for (i=0; i<ARRAY_SIZE(phy_page0_default_setting); i++) {
		if (rtk_usb_phy_write(phy_page0_default_setting[i].addr, phy_page0_default_setting[i].data)) {
			pr_err("[%s:%d], Error : addr = 0x%x, value = 0x%x\n",
				__FUNCTION__, __LINE__,
				phy_page0_default_setting[i].addr,
				phy_page0_default_setting[i].data);
			return -1;
		}
	}

	/* Set page 1 */
	rtk_usb_phy_set_page(1);

	for (i=0; i<ARRAY_SIZE(phy_page1_default_setting); i++) {
		if (rtk_usb_phy_write(phy_page1_default_setting[i].addr, phy_page1_default_setting[i].data)) {
			pr_err("[%s:%d], Error : addr = 0x%x, value = 0x%x\n",
				__FUNCTION__, __LINE__,
				phy_page1_default_setting[i].addr,
				phy_page1_default_setting[i].data);
			return -1;
		}
	}

	/* show setting values */
	rtk_usb_phy_set_page(0);
	pr_info("[USB2_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_D4, rtk_usb_phy_read(REG_USB_PHY_D4));

	pr_info("[USB2_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C0, rtk_usb_phy_read(REG_USB_PHY_C0));
	pr_info("[USB2_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C1, rtk_usb_phy_read(REG_USB_PHY_C1));
	pr_info("[USB2_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C2, rtk_usb_phy_read(REG_USB_PHY_C2));
	pr_info("[USB2_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C4, rtk_usb_phy_read(REG_USB_PHY_C4));
	pr_info("[USB2_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C5, rtk_usb_phy_read(REG_USB_PHY_C5));
	pr_info("[USB2_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C6, rtk_usb_phy_read(REG_USB_PHY_C6));

	rtk_usb_phy_set_page(1);
	pr_info("[USB2_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_D4, rtk_usb_phy_read(REG_USB_PHY_D4));

	pr_info("[USB2_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C0, rtk_usb_phy_read(REG_USB_PHY_C0));
	pr_info("[USB2_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C1, rtk_usb_phy_read(REG_USB_PHY_C1));
	pr_info("[USB2_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C5, rtk_usb_phy_read(REG_USB_PHY_C5));
	pr_info("[USB2_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C6, rtk_usb_phy_read(REG_USB_PHY_C6));
	pr_info("[USB2_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C7, rtk_usb_phy_read(REG_USB_PHY_C7));

	return 0;
}

static int pre_state=0;
void rtk_usb2_phy_state(u32 state)
{
	if (state == 1) {
		pr_err("prepare to U0\n");
		rtk_usb_phy_write(0xF0, 0xbc);
		rtk_usb_phy_write(0xF2, 0xBE);
		pre_state = 1;
	}
	else if (pre_state == 1){
		pr_err("U0\n");
		rtk_usb_phy_write(0xF2, 0x00);
		rtk_usb_phy_write(0xF0, 0xfc);
		pre_state = 0;
	}
}
EXPORT_SYMBOL(rtk_usb2_phy_state);

static int rtk_usb2phy_probe(struct platform_device *pdev)
{
	struct rtk_usb_phy_s *rtk_usb_phy;
	struct device *dev = &pdev->dev;
	int	ret;

	rtk_usb_phy = devm_kzalloc(dev, sizeof(*rtk_usb_phy), GFP_KERNEL);
	if (!rtk_usb_phy)
		return -ENOMEM;

	rtk_usb_phy->dev			= &pdev->dev;
	rtk_usb_phy->phy.dev		= rtk_usb_phy->dev;
	rtk_usb_phy->phy.label		= RTK_USB2PHY_NAME;
	rtk_usb_phy->phy.init		= rtk_usb2_phy_init;
	rtk_usb_phy->phy.shutdown	= rtk_usb2_phy_shutdown;

	REG_GUSB2PHYACC0     = of_iomap(dev->of_node, 0);
	REG_WRAP_VStatusOut2 = of_iomap(dev->of_node, 1);

	platform_set_drvdata(pdev, rtk_usb_phy);

	ret = usb_add_phy(&rtk_usb_phy->phy, USB_PHY_TYPE_USB2);
	if (ret)
		goto err;

	dev_info(&pdev->dev, "%s Initialized RTK USB 2.0 PHY\n", __FILE__);
err:
	return ret;
}

static int rtk_usb2phy_remove(struct platform_device *pdev)
{
	struct rtk_usb_phy_s *rtk_usb_phy = platform_get_drvdata(pdev);

	usb_remove_phy(&rtk_usb_phy->phy);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id usbphy_rtk_dt_match[] = {
	{ .compatible = "Realtek,rtk119x-usb2phy", },
	{},
};
MODULE_DEVICE_TABLE(of, usbphy_rtk_dt_match);
#endif

static struct platform_driver rtk_usb2phy_driver = {
	.probe		= rtk_usb2phy_probe,
	.remove		= rtk_usb2phy_remove,
	.driver		= {
		.name	= RTK_USB2PHY_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(usbphy_rtk_dt_match),
	},
};

module_platform_driver(rtk_usb2phy_driver);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" RTK_USB2PHY_NAME);
