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

#define RTK_USB_RLE0599_PHY_NAME "rtk-usb-phy-rle0599"

#define REG_WRAP_VStatusOut2	IOMEM(0xFE013824)
#define REG_EHCI_INSNREG05		IOMEM(0xFE0130A4)
#define REG_EFUSE71cc		IOMEM(0xFE0171CC)		
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

#define USB_ST_BUSY		BIT(17)

static DEFINE_SPINLOCK(rtk_phy_lock);

#define phy_read(addr)			__raw_readl(addr)
#define phy_write(addr, val)	do { smp_wmb(); __raw_writel(val, addr); } while(0)
#define PHY_IO_TIMEOUT_MSEC		(50)

static struct rtk_usb_phy_data_s phy_page0_default_setting[] = {
	{0xe0, 0xa0},
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

char  efuse_mapping[16] = {

	 0xa1, 0x85, 0x89, 0x8d, 0x91, 0x95, 0x99, 0x9d, 0xa1, 0xa5, 0xa9, 0xad, 0xb1, 0xb5, 0xb9, 0xbd, 

};


int utmi_wait_register(void __iomem *reg, u32 mask, u32 result)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(PHY_IO_TIMEOUT_MSEC);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if ((phy_read(reg) & mask) == result)
			return 0;
		udelay(100);
	}
	pr_err("\033[0;32;31m can't program USB phy \033[m\n");
	return -1;
}
EXPORT_SYMBOL_GPL(utmi_wait_register);

static char rtk_usb_phy_read(char addr)
{
	volatile unsigned int regVal;

	utmi_wait_register(REG_EHCI_INSNREG05, USB_ST_BUSY, 0);

	// VCtrl = low nibble of addr, VLoadM = 1
	regVal = (1 << 13) | 	// Port num
			 (1 << 12) |	// vload
			 ((addr & 0x0f) << 8);	// vcontrol
	phy_write(REG_EHCI_INSNREG05, regVal);
	utmi_wait_register(REG_EHCI_INSNREG05, USB_ST_BUSY, 0);

	// VCtrl = low nibble of addr, VLoadM = 0
	regVal &= ~(1 << 12);
	phy_write(REG_EHCI_INSNREG05, regVal);
	utmi_wait_register(REG_EHCI_INSNREG05, USB_ST_BUSY, 0);

	// VCtrl = high nibble of addr, VLoadM = 1
	regVal = (1 << 13) | 	// Port num
			 (1 << 12) |	// vload
			 ((addr & 0xf0) << 4);	// vcontrol
	phy_write(REG_EHCI_INSNREG05, regVal);
	utmi_wait_register(REG_EHCI_INSNREG05, USB_ST_BUSY, 0);

	// VCtrl = high nibble of addr, VLoadM = 0
	regVal &= ~(1 << 12);
	phy_write(REG_EHCI_INSNREG05, regVal);
	utmi_wait_register(REG_EHCI_INSNREG05, USB_ST_BUSY, 0);

	smp_rmb();
	regVal = phy_read(REG_EHCI_INSNREG05);

	return (char) (regVal & 0xff);
}

static int rtk_usb_phy_write(char addr, char data)
{
	volatile unsigned int regVal;

	//printk("[%s:%d], addr = 0x%x, data = 0x%x\n", __FUNCTION__, __LINE__, addr, data);

	//write data to VStatusOut2 (data output to phy)
	phy_write(REG_WRAP_VStatusOut2, (u32) data);
	utmi_wait_register(REG_EHCI_INSNREG05, USB_ST_BUSY, 0);

	// VCtrl = low nibble of addr, VLoadM = 1
	regVal = (1 << 13) | 	// Port num
			 (1 << 12) |	// vload
			 ((addr & 0x0f) << 8);	// vcontrol
	phy_write(REG_EHCI_INSNREG05, regVal);
	utmi_wait_register(REG_EHCI_INSNREG05, USB_ST_BUSY, 0);

	// VCtrl = low nibble of addr, VLoadM = 0
	regVal &= ~(1 << 12);
	phy_write(REG_EHCI_INSNREG05, regVal);
	utmi_wait_register(REG_EHCI_INSNREG05, USB_ST_BUSY, 0);

	// VCtrl = high nibble of addr, VLoadM = 1
	regVal = (1 << 13) | 	// Port num
			 (1 << 12) |	// vload
			 ((addr & 0xf0) << 4);	// vcontrol
	phy_write(REG_EHCI_INSNREG05, regVal);
	utmi_wait_register(REG_EHCI_INSNREG05, USB_ST_BUSY, 0);

	// VCtrl = high nibble of addr, VLoadM = 0
	regVal &= ~(1 << 12);
	phy_write(REG_EHCI_INSNREG05, regVal);
	utmi_wait_register(REG_EHCI_INSNREG05, USB_ST_BUSY, 0);

	return 0;
}

static int rtk_usb_phy_set_page(int page)
{
	return rtk_usb_phy_write(0xf4, page == 0 ? 0x9b : 0xbb);
}

static int initialized=0;
void rtk_usb_phy_shutdown(struct usb_phy *phy)
{
	/* Todo */
	initialized = 0;
}
EXPORT_SYMBOL_GPL(rtk_usb_phy_shutdown);

int rtk_usb_phy_init(void)
{
	int i;
	int ret=0;
	unsigned long flags;

	spin_lock_irqsave(&rtk_phy_lock, flags);

	if (initialized) goto out;

	/* Set page 0 */
	//printk("[%s:%d], Set page 0\n", __FUNCTION__, __LINE__);
	rtk_usb_phy_set_page(0);

	for (i=0; i<ARRAY_SIZE(phy_page0_default_setting); i++) {
		if (rtk_usb_phy_write(phy_page0_default_setting[i].addr, phy_page0_default_setting[i].data)) {
			pr_err("[%s:%d], Error : addr = 0x%x, value = 0x%x\n",
				__FUNCTION__, __LINE__,
				phy_page0_default_setting[i].addr,
				phy_page0_default_setting[i].data);
			return -1;
		}
		pr_info("[%s:%d], Good : addr = 0x%x, value = 0x%x\n",
				__FUNCTION__, __LINE__,
				phy_page0_default_setting[i].addr,
				phy_page0_default_setting[i].data);

		if (phy_page0_default_setting[i].addr == 0xe0) {
			volatile unsigned int jj = 0;
			jj = (phy_read(REG_EFUSE71cc) >> 6) & 0xf ;
			if (rtk_usb_phy_write(phy_page0_default_setting[i].addr, efuse_mapping[jj])){
                        	pr_err("[%s:%d], Error : addr = 0x%x, value = 0x%x\n",
                                __FUNCTION__, __LINE__,
                                phy_page0_default_setting[i].addr,
                                efuse_mapping[jj]);
                        	return -1;
			}

			pr_info("[%s:%d], Good : addr = 0x%x, value = 0x%x\n",
                                __FUNCTION__, __LINE__,
                                phy_page0_default_setting[i].addr,
                                efuse_mapping[jj]);


		}	
		
	}

	/* Set page 1 */
	//printk("[%s:%d], Set page 1\n", __FUNCTION__, __LINE__);
	rtk_usb_phy_set_page(1);

	for (i=0; i<ARRAY_SIZE(phy_page1_default_setting); i++) {
		if (rtk_usb_phy_write(phy_page1_default_setting[i].addr, phy_page1_default_setting[i].data)) {
			pr_err("[%s:%d], Error : addr = 0x%x, value = 0x%x\n",
				__FUNCTION__, __LINE__,
				phy_page1_default_setting[i].addr,
				phy_page1_default_setting[i].data);
			ret = -1;
			goto out;
		}
	}

	/* show setting values */
	rtk_usb_phy_set_page(0);
	pr_info("[USB_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_D4, rtk_usb_phy_read(REG_USB_PHY_D4));

	pr_info("[USB_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C0, rtk_usb_phy_read(REG_USB_PHY_C0));
	pr_info("[USB_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C1, rtk_usb_phy_read(REG_USB_PHY_C1));
	pr_info("[USB_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C2, rtk_usb_phy_read(REG_USB_PHY_C2));
	pr_info("[USB_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C4, rtk_usb_phy_read(REG_USB_PHY_C4));
	pr_info("[USB_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C5, rtk_usb_phy_read(REG_USB_PHY_C5));
	pr_info("[USB_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C6, rtk_usb_phy_read(REG_USB_PHY_C6));

	rtk_usb_phy_set_page(1);
	pr_info("[USB_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_D4, rtk_usb_phy_read(REG_USB_PHY_D4));

	pr_info("[USB_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C0, rtk_usb_phy_read(REG_USB_PHY_C0));
	pr_info("[USB_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C1, rtk_usb_phy_read(REG_USB_PHY_C1));
	pr_info("[USB_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C5, rtk_usb_phy_read(REG_USB_PHY_C5));
	pr_info("[USB_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C6, rtk_usb_phy_read(REG_USB_PHY_C6));
	pr_info("[USB_PHY], addr = 0x%x, data = 0x%x\n", REG_USB_PHY_C7, rtk_usb_phy_read(REG_USB_PHY_C7));

	initialized = 1;

out:
	spin_unlock_irqrestore(&rtk_phy_lock, flags);
	return ret;
}
EXPORT_SYMBOL_GPL(rtk_usb_phy_init);

#if 0
static int rtk_usb_rle0599_phy_probe(struct platform_device *pdev)
{
	struct rtk_usb_phy_s *rtk_usb_phy;
	struct device *dev = &pdev->dev;
	int	ret;

	rtk_usb_phy = devm_kzalloc(dev, sizeof(*rtk_usb_phy), GFP_KERNEL);
	if (!rtk_usb_phy)
		return -ENOMEM;

	rtk_usb_phy->dev			= &pdev->dev;
	rtk_usb_phy->phy.dev		= rtk_usb_phy->dev;
	rtk_usb_phy->phy.label		= RTK_USB_RLE0599_PHY_NAME;
	rtk_usb_phy->phy.init		= rtk_usb_phy_init;
	rtk_usb_phy->phy.shutdown	= rtk_usb_phy_shutdown;

	REG_WRAP_VStatusOut2 = of_iomap(dev->of_node, 0);
	REG_EHCI_INSNREG05   = of_iomap(dev->of_node, 1);

	ret = usb_add_phy(&rtk_usb_phy->phy, USB_PHY_TYPE_USB2);
	if (ret)
		goto err;

	platform_set_drvdata(pdev, rtk_usb_phy);

	dev_info(&pdev->dev, "Initialized RTK USB 2.0 RLE0599 PHY\n");
err:
	return ret;
}

static int rtk_usb_rle0599_phy_remove(struct platform_device *pdev)
{
	struct rtk_usb_phy_s *rtk_usb_phy = platform_get_drvdata(pdev);

	usb_remove_phy(&rtk_usb_phy->phy);

	return 0;
}

static const struct of_device_id usb_phy_rle0599_rtk_dt_ids[] = {
	{ .compatible = "Realtek,rtk119x-usb_phy_rle0599", },
	{},
};
MODULE_DEVICE_TABLE(of, usb_phy_rle0599_rtk_dt_ids);

static struct platform_driver rtk_usb_rle0599_phy_driver = {
	.probe		= rtk_usb_rle0599_phy_probe,
	.remove		= rtk_usb_rle0599_phy_remove,
	.driver		= {
		.name	= RTK_USB_RLE0599_PHY_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(usb_phy_rle0599_rtk_dt_ids),
	},
};

module_platform_driver(rtk_usb_rle0599_phy_driver);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" RTK_USB_RLE0599_PHY_NAME);
#endif
