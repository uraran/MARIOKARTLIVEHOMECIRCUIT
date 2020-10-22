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

#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/uio_driver.h>

static int rtk_refclk_probe(struct platform_device *pdev)
{
	int ret;
	struct resource res;
	struct uio_info *info;

	printk("%s:%d\n", __func__, __LINE__);

    if(pdev == NULL)
        return -ENODEV;

	info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
    if(info == NULL)
        return -ENOMEM;

	ret = of_address_to_resource(pdev->dev.of_node, 0, &res);
    info->mem[0].addr = res.start;
    info->mem[0].size = ALIGN(res.end - res.start + 1, PAGE_SIZE);
    //info->mem[0].internal_addr = ioremap(info->mem[0].addr, info->mem[0].size);
    info->mem[0].memtype = UIO_MEM_PHYS;
    info->version = "0.0.1";
    info->name = "rtk_refclk";
#ifdef CONFIG_UIO_ASSIGN_MINOR
    info->minor = 253;
#endif

    ret = uio_register_device(&pdev->dev, info);
    if(ret != 0){
        //iounmap(info->mem[0].internal_addr);
        printk("%s: failed to register uio device\n", __func__);
        return ret;
    }

    platform_set_drvdata(pdev, info);
    return 0;
}

static int rtk_refclk_remove(struct platform_device *pdev)
{
	struct uio_info *info = platform_get_drvdata(pdev);

	printk("%s:%d\n", __func__, __LINE__);

	uio_unregister_device(info);
	platform_set_drvdata(pdev, NULL);
	//iounmap(info->mem[0].internal_addr);
	kfree(info);

	return 0;
}

static const struct of_device_id rtk_refclk_ids[] = {
	{ .compatible = "Realtek,rtk119x-refclk" },
	{ /* Sentinel */ },
};

static struct platform_driver rtk_refclk_driver = {
	.probe		= rtk_refclk_probe,
	.remove		= rtk_refclk_remove,
	.driver		= {
		.name	= "rtk119x-refclk",
		.owner	= THIS_MODULE,
        .of_match_table = rtk_refclk_ids,
	},
};

static int __init rtk_refclk_init(void)
{
	printk("%s:%d\n", __func__, __LINE__);
	return platform_driver_register(&rtk_refclk_driver);
}

static void __exit rtk_refclk_exit(void)
{
	printk("%s:%d\n", __func__, __LINE__);
	platform_driver_unregister(&rtk_refclk_driver);
}

module_init(rtk_refclk_init);
module_exit(rtk_refclk_exit);

