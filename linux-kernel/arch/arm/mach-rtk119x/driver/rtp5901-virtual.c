/*
 * rtp5901-virtual.c  --  Realsil RTP5901
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

#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/mfd/rtp5901-mfd.h>

struct virtual_consumer_data {
	struct mutex lock;
	struct regulator *regulator;
	int enabled;
	int min_uV;
	int max_uV;
	int min_uA;
	int max_uA;
	unsigned int mode;
};

static void update_voltage_constraints(struct virtual_consumer_data *data)
{
	int ret;

	printk(KERN_DEBUG "Update voltage constraints\n");

	if (data->min_uV && data->max_uV && data->min_uV <= data->max_uV) {
		ret = regulator_set_voltage(data->regulator, data->min_uV, data->max_uV);
		if (ret != 0) {
			printk(KERN_ERR "regulator_set_voltage() failed: %d\n", ret);
			return;
		}
	}

	if (data->min_uV && data->max_uV && !data->enabled) {
		ret = regulator_enable(data->regulator);
		if (ret == 0)
			data->enabled = 1;
		else
			printk(KERN_ERR "regulator_enable() failed: %d\n", ret);
	}

	if (!(data->min_uV && data->max_uV) && data->enabled) {
		ret = regulator_disable(data->regulator);
		if (ret == 0)
			data->enabled = 0;
		else
			printk(KERN_ERR "regulator_disable() failed: %d\n", ret);
	}
}

static ssize_t show_cur_uV(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct virtual_consumer_data *data = dev_get_drvdata(dev);
	int ret;

	ret = regulator_get_voltage(data->regulator);

	return sprintf(buf, "%d\n", ret);
}

static ssize_t set_cur_uV(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct virtual_consumer_data *data = dev_get_drvdata(dev);
	long val;

	if (strict_strtol(buf, 10, &val) != 0)
		return count;

	mutex_lock(&data->lock);

	data->min_uV = val;
	data->max_uV = val;
	update_voltage_constraints(data);

	mutex_unlock(&data->lock);

	return count;
}

static ssize_t show_stb_uV(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "regulator standby voltage test\n");
}

static ssize_t set_stb_uV(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct virtual_consumer_data *data = dev_get_drvdata(dev);
	long val;
	int ret;

	if (strict_strtol(buf, 10, &val) != 0)
		return count;

	mutex_lock(&data->lock);

#if 0
	ret = regulator_set_suspend_voltage(data->regulator, val);
#else
    ret = 0;
#endif
	if (ret != 0) {
		printk(KERN_ERR "regulator_set_suspend_voltage() failed: %d\n", ret);
		return count;
	}

	mutex_unlock(&data->lock);

	return count;
}

static DEVICE_ATTR(cur_microvolts, 0644, show_cur_uV, set_cur_uV);
static DEVICE_ATTR(stb_microvolts, 0644, show_stb_uV, set_stb_uV);

struct device_attribute *rtp5901_attributes_virtual[] = {
	&dev_attr_cur_microvolts,
	&dev_attr_stb_microvolts,
};

static int regulator_virtual_consumer_probe(struct platform_device *pdev)
{
	char *reg_id = pdev->dev.platform_data;
	struct virtual_consumer_data *drvdata;
	int ret, i;

	drvdata = kzalloc(sizeof(struct virtual_consumer_data), GFP_KERNEL);
	if (drvdata == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	mutex_init(&drvdata->lock);

	drvdata->regulator = regulator_get(NULL, reg_id);

	if (IS_ERR(drvdata->regulator)) {
		ret = PTR_ERR(drvdata->regulator);
		goto err;
	}

	for (i = 0; i < ARRAY_SIZE(rtp5901_attributes_virtual); i++) {
		ret = device_create_file(&pdev->dev, rtp5901_attributes_virtual[i]);
		if (ret != 0)
			goto err;
	}

	drvdata->mode = regulator_get_mode(drvdata->regulator);
	drvdata->enabled = regulator_is_enabled(drvdata->regulator);

	platform_set_drvdata(pdev, drvdata);

	return 0;

err:
	for (i = 0; i < ARRAY_SIZE(rtp5901_attributes_virtual); i++)
		device_remove_file(&pdev->dev, rtp5901_attributes_virtual[i]);
	kfree(drvdata);
	return ret;
}

static int regulator_virtual_consumer_remove(struct platform_device *pdev)
{
	struct virtual_consumer_data *drvdata = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < ARRAY_SIZE(rtp5901_attributes_virtual); i++)
		device_remove_file(&pdev->dev, rtp5901_attributes_virtual[i]);
	if (drvdata->enabled)
		regulator_disable(drvdata->regulator);
	regulator_put(drvdata->regulator);

	kfree(drvdata);

	return 0;
}

static struct platform_driver regulator_virtual_consumer_driver[] = {
	{
		.probe = regulator_virtual_consumer_probe,
		.remove	= regulator_virtual_consumer_remove,
		.driver	= {
			.name = "rtp5901-cs-ldo2",
		},
	}, {
		.probe = regulator_virtual_consumer_probe,
		.remove = regulator_virtual_consumer_remove,
		.driver = {
			.name = "rtp5901-cs-ldo3",
		},
	}, {
		.probe = regulator_virtual_consumer_probe,
		.remove = regulator_virtual_consumer_remove,
		.driver = {
			.name = "rtp5901-cs-ldo4",
		},
	}, {
		.probe = regulator_virtual_consumer_probe,
		.remove = regulator_virtual_consumer_remove,
		.driver = {
			.name = "rtp5901-cs-ldo5",
		},
	}, {
		.probe = regulator_virtual_consumer_probe,
		.remove = regulator_virtual_consumer_remove,
		.driver = {
			.name = "rtp5901-cs-buck1",
		},
	}, {
		.probe = regulator_virtual_consumer_probe,
		.remove = regulator_virtual_consumer_remove,
		.driver	= {
			.name = "rtp5901-cs-buck2",
		},
	}, {
		.probe = regulator_virtual_consumer_probe,
		.remove = regulator_virtual_consumer_remove,
		.driver	= {
			.name = "rtp5901-cs-buck3",
		},
	}, {
		.probe = regulator_virtual_consumer_probe,
		.remove = regulator_virtual_consumer_remove,
		.driver	= {
			.name = "rtp5901-cs-buck4",
		},
	},
};

static int __init regulator_virtual_consumer_init(void)
{
	int j, ret;
	for (j = 0; j < ARRAY_SIZE(regulator_virtual_consumer_driver); j++) {
		ret =  platform_driver_register(&regulator_virtual_consumer_driver[j]);
		if (ret)
			goto creat_drivers_failed;
	}

	return ret;

creat_drivers_failed:
	while (j--)
		platform_driver_unregister(&regulator_virtual_consumer_driver[j]);
	return ret;
}
module_init(regulator_virtual_consumer_init);

static void __exit regulator_virtual_consumer_exit(void)
{
	int j;
	for (j = ARRAY_SIZE(regulator_virtual_consumer_driver) - 1; j >= 0; j--) {
		platform_driver_unregister(&regulator_virtual_consumer_driver[j]);
	}
}
module_exit(regulator_virtual_consumer_exit);

//MODULE_LICENSE("GPL");
//MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
//MODULE_DESCRIPTION("Realtek Power Manager Driver");
