/**
 * dwc3-rtk.c - Realtek DWC3 Specific Glue layer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/usb/otg.h>
#include <linux/usb/nop-usb-xceiv.h>
#include <linux/of.h>
#include <linux/of_platform.h>

#include "../host/xhci.h" //hcy test added
#include "core.h" //hcy test added

struct dwc3_rtk {
	struct platform_device	*usb2_phy;
	struct platform_device	*usb3_phy;
	struct device		*dev;

	struct clk		*clk;
	struct platform_device  *dwc; //hjcy added
};

static int dwc3_rtk_register_phys(struct dwc3_rtk *rtk)
{
	struct nop_usb_xceiv_platform_data pdata;
	struct platform_device	*pdev;
	int			ret;

	memset(&pdata, 0x00, sizeof(pdata));

	pdev = platform_device_alloc("nop_usb_xceiv", PLATFORM_DEVID_AUTO);
	if (!pdev)
		return -ENOMEM;

	rtk->usb2_phy = pdev;
	pdata.type = USB_PHY_TYPE_USB2;

	ret = platform_device_add_data(rtk->usb2_phy, &pdata, sizeof(pdata));
	if (ret)
		goto err1;

	pdev = platform_device_alloc("nop_usb_xceiv", PLATFORM_DEVID_AUTO);
	if (!pdev) {
		ret = -ENOMEM;
		goto err1;
	}

	rtk->usb3_phy = pdev;
	pdata.type = USB_PHY_TYPE_USB3;

	ret = platform_device_add_data(rtk->usb3_phy, &pdata, sizeof(pdata));
	if (ret)
		goto err2;

	ret = platform_device_add(rtk->usb2_phy);
	if (ret)
		goto err2;

	ret = platform_device_add(rtk->usb3_phy);
	if (ret)
		goto err3;

	return 0;

err3:
	platform_device_del(rtk->usb2_phy);

err2:
	platform_device_put(rtk->usb3_phy);

err1:
	platform_device_put(rtk->usb2_phy);

	return ret;
}

static int dwc3_rtk_remove_child(struct device *dev, void *unused)
{
	struct platform_device *pdev = to_platform_device(dev);

	platform_device_unregister(pdev);

	return 0;
}

static int dwc3_rtk_probe(struct platform_device *pdev)
{
	struct dwc3_rtk	*rtk;
//	struct clk		*clk;
	struct device		*dev = &pdev->dev;
	struct device_node	*node = dev->of_node;

	int			ret = -ENOMEM;

	rtk = devm_kzalloc(dev, sizeof(*rtk), GFP_KERNEL);
	if (!rtk) {
		dev_err(dev, "not enough memory\n");
		goto err1;
	}

	/*
	 * Right now device-tree probed devices don't get dma_mask set.
	 * Since shared usb code relies on it, set it here for now.
	 * Once we move to full device tree support this will vanish off.
	 */
	if (!dev->dma_mask)
		dev->dma_mask = &dev->coherent_dma_mask;
	if (!dev->coherent_dma_mask)
		dev->coherent_dma_mask = DMA_BIT_MASK(32);

	platform_set_drvdata(pdev, rtk);

	ret = dwc3_rtk_register_phys(rtk);
	if (ret) {
		dev_err(dev, "couldn't register PHYs\n");
		goto err1;
	}

#if 0
	clk = devm_clk_get(dev, "usbdrd30");
	if (IS_ERR(clk)) {
		dev_err(dev, "couldn't get clock\n");
		ret = -EINVAL;
		goto err1;
	}
#endif
	rtk->dev	= dev;
#if 0
	rtk->clk	= clk;

	clk_prepare_enable(rtk->clk);
#endif
	if (node) {
		ret = of_platform_populate(node, NULL, NULL, dev);
		if (ret) {
			dev_err(dev, "failed to add dwc3 core\n");
			goto err2;
		}
		/* hcy adde below */
		node =  of_find_compatible_node(NULL, NULL, "synopsys,dwc3");
		if (node != NULL)
			rtk->dwc = of_find_device_by_node(node);			
		/* hcy added above */
	} else {
		dev_err(dev, "no device node, failed to add dwc3 core\n");
		ret = -ENODEV;
		goto err2;
	}

	return 0;

err2:
#if 0
	clk_disable_unprepare(clk);
#endif
err1:
	return ret;
}

static int dwc3_rtk_remove(struct platform_device *pdev)
{
	struct dwc3_rtk	*rtk = platform_get_drvdata(pdev);

	device_for_each_child(&pdev->dev, NULL, dwc3_rtk_remove_child);
	platform_device_unregister(rtk->usb2_phy);
	platform_device_unregister(rtk->usb3_phy);

	clk_disable_unprepare(rtk->clk);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id rtk_dwc3_match[] = {
	{ .compatible = "Realtek,rtk119x-dwc3" },
	{},
};
MODULE_DEVICE_TABLE(of, rtk_dwc3_match);
#endif


#ifdef CONFIG_PM_SLEEP
extern struct dwc3 *dwc_substance ; //hcy added

static int dwc3_rtk_suspend(struct device *dev)
{
//hcy removed	clk_disable(rtk->clk);

#if 0
	/* reset usb3 */
	writel(readl(IOMEM(0xfe000000)) & ~(BIT(2)|BIT(4)), IOMEM(0xfe000000));
#endif
	return 0;
}

static int dwc3_rtk_resume(struct device *dev)
{
	struct dwc3_rtk *rtk = dev_get_drvdata(dev);
       

	
#if 0
	/* release rst of usb3 */
	writel(readl(IOMEM(0xfe000000)) | (BIT(2)|BIT(4)), IOMEM(0xfe000000));
	mdelay(5);
#endif
//hcy removed	clk_enable(rtk->clk);



	switch (((struct dwc3 *)platform_get_drvdata(rtk->dwc))->mode) {
        case USB_DR_MODE_PERIPHERAL:
                writel(0x0, IOMEM(0xfe013258));         // in wrapper 
                writel(0x0, IOMEM(0xfe013270));         // set Dpm 1'b0
                break;
        case USB_DR_MODE_HOST:
                writel(0x7, IOMEM(0xfe013258));         // in wrapper
                writel(0x606, IOMEM(0xfe013270));       // set Dpm 1'b1
                break;
        case USB_DR_MODE_OTG:
		break;
	}	





	/* runtime set active to reflect active state. */
	pm_runtime_disable(dev);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	return 0;
}

static const struct dev_pm_ops dwc3_rtk_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dwc3_rtk_suspend, dwc3_rtk_resume)
};

#define DEV_PM_OPS	(&dwc3_rtk_dev_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static struct platform_driver dwc3_rtk_driver = {
	.probe		= dwc3_rtk_probe,
	.remove		= dwc3_rtk_remove,
	.driver		= {
		.name	= "rtk-dwc3",
		.of_match_table = of_match_ptr(rtk_dwc3_match),
		.pm	= DEV_PM_OPS,
	},
};

module_platform_driver(dwc3_rtk_driver);

MODULE_ALIAS("platform:rtk-dwc3");
MODULE_LICENSE("GPL");
