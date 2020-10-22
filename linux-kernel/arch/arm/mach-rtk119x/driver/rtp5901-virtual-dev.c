/*
 * rtp5901-virtual-dev.c  --  Realsil RTP5901
 *
 * Copyright 2013 Realsil Semiconductor Corp.
 *
 * Author: Wind Han <wind_han@realsil.com.cn>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <mach/irqs.h>
#include <linux/power_supply.h>
#include <linux/mfd/rtp5901-mfd.h>

static struct platform_device virtual_dev[]={
	{
		.name = "rtp5901-cs-ldo2",
		.id = -1,
		.dev = {
			.platform_data = "vdd_12",
		}
	}, {
		.name = "rtp5901-cs-ldo3",
		.id = -1,
		.dev = {
			.platform_data = "vcc_tp",
		}
	}, {
		.name = "rtp5901-cs-ldo4",
		.id = -1,
		.dev = {
			.platform_data = "vcca_33",
		}
	}, {
		.name = "rtp5901-cs-ldo5",
		.id = -1,
		.dev = {
			.platform_data = "vcc_wl",
		}
	}, {
		.name = "rtp5901-cs-buck1",
		.id = -1,
		.dev = {
			.platform_data = "vdd_cpu",
		}
	}, {
		.name = "rtp5901-cs-buck2",
		.id = -1,
		.dev = {
#if 0
			.platform_data = "vdd_core",
#else
			.platform_data = "vddarm",
#endif
		}
	}, {
		.name = "rtp5901-cs-buck3",
		.id = -1,
		.dev = {
			.platform_data = "vcc_ddr",
		}
	}, {
		.name = "rtp5901-cs-buck4",
		.id = -1,
		.dev = {
			.platform_data = "vcc_io",
		}
	},
};

static int __init virtual_init(void)
{
	int j,ret;
	for (j = 0; j < ARRAY_SIZE(virtual_dev); j++) {
		ret = platform_device_register(&virtual_dev[j]);
		if (ret)
			goto creat_devices_failed;
	}

	return ret;

creat_devices_failed:
	while (j--)
		platform_device_register(&virtual_dev[j]);
	return ret;
}
module_init(virtual_init);

static void __exit virtual_exit(void)
{
	int j;
	for (j = ARRAY_SIZE(virtual_dev) - 1; j >= 0; j--)
		platform_device_unregister(&virtual_dev[j]);
}
module_exit(virtual_exit);

//MODULE_LICENSE("GPL");
//MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
//MODULE_DESCRIPTION("Realtek Power Manager Driver");
