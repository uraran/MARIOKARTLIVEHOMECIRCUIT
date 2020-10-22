/*
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/gpio.h>
#include <linux/ioport.h>


#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include "mach/gpio.h"

static struct rfkill *bt_rfk;
static const char bt_name[] = "bluetooth";

static  int bt_reset;
/*
static void getIoResource(struct platform_device  *pdev)
{
	struct resource *res;

	printk("rfkill get gpio\n");

	res = platform_get_resource_byname(pdev, IORESOURCE_IO,"bt_reset");
	if (!res) {
		printk("couldn't find bt_reset gpio\n");
	}

	bt_reset = res->start;

	printk("bt_reset = %ld\n", bt_reset);
}
*/

static int bluetooth_set_power(void *data, bool blocked)
{
	printk("%s: block=%d\n",__func__, blocked);

	if (!blocked) {
		//mdelay(750);	//mask mdelay here because user bluedroid will do it again	victor 20140612
		gpio_direction_output(bt_reset, 1);
		//mdelay(10);
	} else {
		gpio_direction_output(bt_reset, 0);
		//mdelay(750);
	}

	return 0;
}

static struct rfkill_ops rfkill_bluetooth_ops = {
	.set_block = bluetooth_set_power,
};

static void rfkill_gpio_init(struct device_node * rtk119x_bt_node)
{
/*
	if(gpio_request(bt_reset,"bt_reset")){
		printk("request bt reset fail\n");
	}
*/
/*initial gpios*/

	bt_reset = of_get_gpio_flags(rtk119x_bt_node, 0, NULL); 	// get gpio number from device tree

	if (bt_reset < 0) {
		printk("[%s ] could not get gpio from of \n",__FUNCTION__);
	}

	if (!gpio_is_valid(bt_reset))						 //check whether gpio number is valid
	{
		printk("[%s ] gpio %d is not valid\n",__FUNCTION__, bt_reset);
	}

	if(gpio_request(bt_reset, rtk119x_bt_node->name))			 //request gpio
	{
		printk("[%s ] could not request gpio, %d\n",__FUNCTION__, bt_reset);
	}
}

static void rfkill_gpio_deinit(void)
{
	gpio_free(bt_reset);
}
static int rfkill_bluetooth_probe(struct platform_device *pdev)
{

	int rc = 0;
	bool default_state = true;
	struct device_node *rtk119x_bt_node;
	rtk119x_bt_node = pdev->dev.of_node;
	
	printk(KERN_INFO "-->%s\n", __func__);
//	getIoResource(pdev);

	bt_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
	   &rfkill_bluetooth_ops, NULL);
	if (!bt_rfk) {
	 rc = -ENOMEM;
	 goto err_rfkill_alloc;
	}
        rfkill_gpio_init(rtk119x_bt_node);
	/* userspace cannot take exclusive control */
	rfkill_init_sw_state(bt_rfk,true);//default blocked
	rc = rfkill_register(bt_rfk);
	if (rc)
		goto err_rfkill_reg;

	rfkill_set_sw_state(bt_rfk,true);
	//bluetooth_set_power(NULL, default_state);	//mask this if device tree has set gpio default output low   victor20140612

	printk(KERN_INFO "<--%s\n", __func__);
	return 0;

err_rfkill_reg:
	rfkill_destroy(bt_rfk);
err_rfkill_alloc:
	return rc;
}

static int rfkill_bluetooth_remove(struct platform_device *dev)
{
	printk(KERN_INFO "-->%s\n", __func__);
	rfkill_gpio_deinit();
	rfkill_unregister(bt_rfk);
	rfkill_destroy(bt_rfk);
	printk(KERN_INFO "<--%s\n", __func__);
	return 0;
}
static struct of_device_id rtk119x_bt_ids[] = {
	{ .compatible = "Realtek,bt" },
	{ /* Sentinel */ },
};
static struct platform_driver rfkill_bluetooth_driver = {
	.probe  = rfkill_bluetooth_probe,
	.remove = rfkill_bluetooth_remove,
	.driver = {
		.name = "rfkill",
		.owner = THIS_MODULE,
		.of_match_table = rtk119x_bt_ids,
	},
};

static int __init rfkill_bluetooth_init(void)
{
	printk(KERN_INFO "-->%s\n", __func__);
	return platform_driver_register(&rfkill_bluetooth_driver);
}

static void __exit rfkill_bluetooth_exit(void)
{
	printk(KERN_INFO "-->%s\n", __func__);
	platform_driver_unregister(&rfkill_bluetooth_driver);
}

late_initcall(rfkill_bluetooth_init);
module_exit(rfkill_bluetooth_exit);
MODULE_DESCRIPTION("bluetooth rfkill");
MODULE_AUTHOR("rs <wn@realsil.com.cn>");
MODULE_LICENSE("GPL");

