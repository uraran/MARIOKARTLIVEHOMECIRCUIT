#include <linux/device.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include <linux/miscdevice.h>

#include "rtk_se_drv.h"

struct se_dev		*se_drv_data = NULL;
struct se_device	*se_device = NULL;

static const struct file_operations se_fops = {
	.owner			= THIS_MODULE,
	.open			= se_open,
	.release		= se_release,
	.unlocked_ioctl	= se_ioctl,
	.write			= se_write,
	.mmap			= se_mmap,
};

static int rtk_se_probe(struct platform_device *pdev)
{
	int ret;

	dbg_info("%s %s",__FILE__, __func__);
	se_drv_data = kzalloc(sizeof(struct se_dev), GFP_KERNEL);
	if (unlikely(!se_drv_data))
		return -ENOMEM;

	se_device->dev.minor = MISC_DYNAMIC_MINOR;
	se_device->dev.name = "rtk-se";
	se_device->dev.fops = &se_fops;
	se_device->dev.parent = NULL;
	ret = misc_register(&se_device->dev);
	if (ret) {
		dbg_err("rtk-se: failed to register misc device.");
		return ret;
	}
	se_drv_init();

	return 0;
}

static int rtk_se_remove(struct platform_device *pdev)
{
	dbg_info("%s %s",__FILE__, __func__);

	se_drv_uninit();
	kfree(se_drv_data);

	return 0;
}

static struct platform_driver rtk_se_driver = {
	.probe		= rtk_se_probe,
	.remove		= rtk_se_remove,
	.driver		= {
		.name	= "rtk-se",
		.owner	= THIS_MODULE,
	},
};

static const struct of_device_id rtk_se_ids[] __initconst = {
	{ .compatible = "Realtek,rtk119x-se" },
	{ /* Sentinel */ },		
};

static struct platform_device *rtk_se_device;

static int __init rtk_se_init(void)
{
	int ret=0;
	struct device_node *np;

	dbg_info("%s %s",__FILE__, __func__);

	se_device = kzalloc(sizeof(struct se_device), GFP_KERNEL);
	if (!se_device)
		return -ENOMEM;

	np = of_find_matching_node(NULL, rtk_se_ids);
	if (!np)
		panic("No SE device node");

	se_device->base = of_iomap(np, 0);
	se_device->irq = irq_of_parse_and_map(np, 0);
	dbg_info("base:0x%x irq:%d", (unsigned int)se_device->base, se_device->irq);

	ret = platform_driver_register(&rtk_se_driver);
	if (!ret) {
		rtk_se_device = platform_device_alloc("rtk-se", 0);
		if (rtk_se_device) {
			ret = platform_device_add(rtk_se_device);
		}
		else {
			ret = -ENOMEM;
		}
		if (ret) {
			platform_device_put(rtk_se_device);
			platform_driver_unregister(&rtk_se_driver);
		}
	}

	return ret;
}

static void __exit rtk_se_exit(void)
{
	dbg_info("%s %s",__FILE__, __func__);

	platform_driver_unregister(&rtk_se_driver);
	misc_deregister(&se_device->dev);
	kfree(se_device);
}

module_init(rtk_se_init);
module_exit(rtk_se_exit);
