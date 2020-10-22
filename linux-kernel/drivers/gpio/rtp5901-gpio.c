/*
 * rtp5901-gpio.c  --  Realsil RTP5901
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/mfd/rtp5901-mfd.h>

/*
*  gpio 0: gpio0
*	1: gpio1
*	2: gpio3
*	3: rtc_clk
*	4: ldo1
*	5: exten
*/

static int rtp5901_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	uint8_t val;
	struct rtp5901_mfd_chip *chip = dev_get_drvdata(gc->dev);

	printk(KERN_DEBUG "Get rtp5901 gpio-%d value\n", offset);
	switch (offset) {
		case 0:
		case 1:
			rtp5901_reg_read(chip, RTP5901_REG_GPIO0_CTRL + offset, &val);
			val &= 0x07;
			if (val == 0x01)
				return 1;
			else
				return 0;
		case 2:
			rtp5901_reg_read(chip, RTP5901_REG_GPIO3_CTRL, &val);
			val &= 0x07;
			if (val == 0x01)
				return 1;
			else
				return 0;
		case 3:
			rtp5901_reg_read(chip, RTP5901_REG_XTAL_CLK_SET, &val);
			val &= 0x18;
			if (val == 0x10)
				return 1;
			else
				return 0;
		case 4:
			rtp5901_reg_read(chip, RTP5901_REG_LDO1_CTL, &val);
			val &= 0x10;
			if (val == 0x10)
				return 1;
			else
				return 0;
		case 5:
			rtp5901_reg_read(chip, RTP5901_REG_EXTEN_EN, &val);
			val &= 0x01;
			if (val == 0x01)
				return 1;
			else
				return 0;
		default:
			dev_warn(gc->dev, "rtp5901 gpio 0-5, offset = %d\n", offset);
			return -EINVAL;
	}

	return 0;
}

static void rtp5901_gpio_set(struct gpio_chip *gc, unsigned offset, int value)
{
	uint8_t val;
	struct rtp5901_mfd_chip *chip = dev_get_drvdata(gc->dev);

	printk(KERN_DEBUG "Set rtp5901 gpio-%d value, value = %d\n", offset, value);
	if (value) {
		switch (offset) {
			case 0:
			case 1:
				rtp5901_reg_read(chip, RTP5901_REG_GPIO0_CTRL + offset, &val);
				val &= 0x07;
				if (val == 0x02)
					return;
				rtp5901_update_bits(chip, RTP5901_REG_GPIO0_CTRL + offset, 0x01, 0x07);
				break;
			case 2:
				rtp5901_reg_read(chip, RTP5901_REG_GPIO3_CTRL, &val);
				val &= 0x07;
				if (val == 0x02)
					return;
				rtp5901_update_bits(chip, RTP5901_REG_GPIO3_CTRL, 0x01, 0x07);
				break;
			case 3:
				rtp5901_update_bits(chip, RTP5901_REG_XTAL_CLK_SET, 0x10, 0x18);
				break;
			case 4:
				rtp5901_set_bits(chip, RTP5901_REG_LDO1_CTL, 0x10);
				break;
			case 5:
				rtp5901_set_bits(chip, RTP5901_REG_EXTEN_EN, 0x01);
				rtp5901_clr_bits(chip, RTP5901_REG_EXTEN_EN, 0x04);
				break;
			default:
				dev_warn(gc->dev, "rtp5901 gpio 0-5, offset = %d\n", offset);
				return;
		}
	} else {
		switch (offset) {
			case 0:
			case 1:
				rtp5901_reg_read(chip, RTP5901_REG_GPIO0_CTRL + offset, &val);
				val &= 0x07;
				if (val == 0x02)
					return;
				rtp5901_clr_bits(chip, RTP5901_REG_GPIO0_CTRL + offset, 0x07);
				break;
			case 2:
				rtp5901_reg_read(chip, RTP5901_REG_GPIO3_CTRL, &val);
				val &= 0x07;
				if (val == 0x02)
					return;
				rtp5901_clr_bits(chip, RTP5901_REG_GPIO3_CTRL, 0x07);
				break;
			case 3:
				rtp5901_update_bits(chip, RTP5901_REG_XTAL_CLK_SET, 0x08, 0x18);
				break;
			case 4:
				rtp5901_clr_bits(chip, RTP5901_REG_LDO1_CTL, 0x10);
				break;
			case 5:
				rtp5901_clr_bits(chip, RTP5901_REG_EXTEN_EN, 0x01);
				rtp5901_set_bits(chip, RTP5901_REG_EXTEN_EN, 0x04);
				break;
			default:
				dev_warn(gc->dev, "rtp5901 gpio 0-5, offset = %d\n", offset);
				return;
		}
	}
}

static int rtp5901_gpio_doutput(struct gpio_chip *gc, unsigned offset, int value)
{
	struct rtp5901_mfd_chip *chip = dev_get_drvdata(gc->dev);

	printk(KERN_DEBUG "Set rtp5901 gpio-%d as output, value %d\n", offset, value);
	if (value) {
		switch (offset) {
			case 0:
			case 1:
				return rtp5901_update_bits(chip, RTP5901_REG_GPIO0_CTRL + offset, 0x01, 0x07);
			case 2:
				return rtp5901_update_bits(chip, RTP5901_REG_GPIO3_CTRL, 0x01, 0x07);
			case 3:
				return rtp5901_update_bits(chip, RTP5901_REG_XTAL_CLK_SET, 0x10, 0x18);
			case 4:
				return rtp5901_set_bits(chip, RTP5901_REG_LDO1_CTL, 0x10);
			case 5:
				rtp5901_set_bits(chip, RTP5901_REG_EXTEN_EN, 0x01);
				return rtp5901_clr_bits(chip, RTP5901_REG_EXTEN_EN, 0x04);
			default:
				dev_warn(gc->dev, "rtp5901 gpio 0-5, offset = %d\n", offset);
				return -EINVAL;
		}
	} else {
		switch (offset) {
			case 0:
			case 1:
				return rtp5901_clr_bits(chip, RTP5901_REG_GPIO0_CTRL + offset, 0x07);
			case 2:
				return rtp5901_clr_bits(chip, RTP5901_REG_GPIO3_CTRL, 0x07);
			case 3:
				return rtp5901_update_bits(chip, RTP5901_REG_XTAL_CLK_SET, 0x08, 0x18);
			case 4:
				return rtp5901_clr_bits(chip, RTP5901_REG_LDO1_CTL, 0x10);
			case 5:
				rtp5901_clr_bits(chip, RTP5901_REG_EXTEN_EN, 0x01);
				return rtp5901_set_bits(chip, RTP5901_REG_EXTEN_EN, 0x04);
			default:
				dev_warn(gc->dev, "rtp5901 gpio 0-5, offset = %d\n", offset);
				return -EINVAL;
		}
	}

	return 0;
}

static int rtp5901_gpio_dinput(struct gpio_chip *gc, unsigned offset)
{
	struct rtp5901_mfd_chip *chip = dev_get_drvdata(gc->dev);

	printk(KERN_DEBUG "Set rtp5901 gpio-%d as input\n", offset);
	switch (offset) {
		case 0:
		case 1:
			return rtp5901_update_bits(chip, RTP5901_REG_GPIO0_CTRL + offset, 0x02, 0x07);
		case 2:
			return rtp5901_update_bits(chip, RTP5901_REG_GPIO3_CTRL, 0x02, 0x07);
		case 3:
		case 4:
		case 5:
			dev_warn(gc->dev, "rtp5901 gpio-3/4/5 only output\n");
			return 0;
		default:
			dev_warn(gc->dev, "rtp5901 gpio 0-5, offset = %d\n", offset);
			return -EINVAL;
	}

	return 0;
}

static ssize_t rtp5901_gpio_direction_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Show rtp5901 gpio direction\n");
}

static ssize_t rtp5901_gpio_direction_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int gpio_dir, gpio_idx, gpio_val = -1;
	struct rtp5901_mfd_chip *chip = dev_get_drvdata(dev);
	struct gpio_chip *gpio_chip = &chip->gpio;
	char buftemp[2];

	if (count != 3 && count != 4)
		return count;

	// index
	buftemp[0] = buf[0];
	buftemp[1] = '\0';
	gpio_idx = simple_strtoul(buftemp, NULL, 10);

	// direction
	buftemp[0] = buf[1];
	buftemp[1] = '\0';
	gpio_dir = simple_strtoul(buftemp, NULL, 10);

	if (count == 4) {
		// val
		buftemp[0] = buf[2];
		buftemp[1] = '\0';
		gpio_val = simple_strtoul(buftemp, NULL, 10);
	}

	printk(KERN_DEBUG "Set rtp5901 gpio-%d direction, dir = %d\n", gpio_idx, gpio_dir);

	if (gpio_idx < 0 || gpio_idx >= gpio_chip->ngpio || gpio_dir < 0 || gpio_dir > 1)
		return count;

	if (gpio_dir == 0)
		gpio_direction_input(gpio_chip->base + gpio_idx);
	else {
		if (gpio_val < 0 || gpio_val > 1)
			return count;
		gpio_direction_output(gpio_chip->base + gpio_idx, gpio_val);
	}

	return count;
}

static int rtp5901_gpio_offset = 0;
static ssize_t rtp5901_gpio_value_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct rtp5901_mfd_chip *chip = dev_get_drvdata(dev);
	struct gpio_chip *gpio_chip = &chip->gpio;

	return sprintf(buf, "%d\n", gpio_get_value(gpio_chip->base + rtp5901_gpio_offset));
}

static ssize_t rtp5901_gpio_value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct rtp5901_mfd_chip *chip = dev_get_drvdata(dev);
	struct gpio_chip *gpio_chip = &chip->gpio;

	char buftemp[2];
	int val;

	if (count != 3)
		return count;

	// offset
	buftemp[0] = buf[0];
	buftemp[1] = '\0';
	rtp5901_gpio_offset = simple_strtoul(buftemp, NULL, 10);

	// value
	buftemp[0] = buf[1];
	buftemp[1] = '\0';
	val = simple_strtoul(buftemp, NULL, 10);

	gpio_set_value(gpio_chip->base + rtp5901_gpio_offset, val);

	return count;
}

static struct device_attribute rtp5901_gpio_attrs[] = {
	RTP_MFD_ATTR(rtp5901_gpio_direction),
	RTP_MFD_ATTR(rtp5901_gpio_value),
};

int rtp5901_gpio_init(struct rtp5901_mfd_chip *chip, int gpio_base)
{
	int ret, err, j;

	printk(KERN_DEBUG "Init rtp5901 gpio, gpio base = %d\n", gpio_base);

	if (!gpio_base)
		return -EINVAL;

	chip->gpio.owner = THIS_MODULE;
	chip->gpio.label = chip->i2c_client->name;
	chip->gpio.dev = chip->dev;
	chip->gpio.base	= gpio_base;

	switch(rtp5901_chip_id(chip)) {
		case RTP5901_ID:
			chip->gpio.ngpio = RTP5901_GPIO_NUM;
			break;
		default:
			return -EINVAL;
	}

	chip->gpio.direction_input = rtp5901_gpio_dinput;
	chip->gpio.direction_output = rtp5901_gpio_doutput;
	chip->gpio.set = rtp5901_gpio_set;
	chip->gpio.get = rtp5901_gpio_get;

	ret = gpiochip_add(&chip->gpio);
	if (ret) {
		dev_warn(chip->dev, "GPIO registration failed: %d\n", ret);
		return ret;
	}

	for (j = 0; j < ARRAY_SIZE(rtp5901_gpio_attrs); j++) {
		ret = device_create_file(chip->dev, &rtp5901_gpio_attrs[j]);
		if (ret)
			goto sysfs_failed;
	}

	return 0;

sysfs_failed:
	while (j--)
		device_remove_file(chip->dev, &rtp5901_gpio_attrs[j]);
	err = gpiochip_remove(&chip->gpio);
	if (err) {
		dev_warn(chip->dev, "GPIO unregistration failed: %d\n", err);
		return err;
	}
	return ret;
}

int rtp5901_gpio_exit(struct rtp5901_mfd_chip *chip)
{
	int j, ret;

	printk(KERN_DEBUG "Exit rtp5901 gpio\n");

	for (j = 0; j < ARRAY_SIZE(rtp5901_gpio_attrs); j++)
		device_remove_file(chip->dev, &rtp5901_gpio_attrs[j]);
	ret = gpiochip_remove(&chip->gpio);
	if (ret) {
		dev_warn(chip->dev, "GPIO unregistration failed: %d\n", ret);
		return ret;
	}
	return 0;
}
