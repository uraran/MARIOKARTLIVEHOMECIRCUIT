/*
 * rtp5901-regulator.c  --  Realsil RTP5901
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
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/mfd/rtp5901-mfd.h>
#include <linux/interrupt.h>

struct rtp5901_regulator_info {
	struct regulator_desc desc;
	int min_uV;
	int max_uV;
	int step_uV;
	int vol_reg;
	int vol_shift;
	int vol_nbits;
	int enable_reg;
	int enable_bit;
	int stb_vol_reg;
	int stb_enable_reg;
	uint8_t vol_offset;
};

#define RTP5901_ENABLE_TIME_US	1000

struct rtp5901_regu {
	struct rtp5901_mfd_chip *chip;
	struct regulator_dev **rdev;
	struct rtp5901_regulator_info **info;
	struct mutex mutex;
	int num_regulators;
	int mode;
	int  (*get_ctrl_reg)(int);
	unsigned int *ext_sleep_control;
	unsigned int board_ext_control[RTP5901_ID_REGULATORS_NUM];
};

static inline int check_range(struct rtp5901_regulator_info *info, int min_uV, int max_uV)
{
	if (min_uV < info->min_uV || min_uV > info->max_uV)
		return -EINVAL;

	return 0;
}

// rtp5901 regulator common operations
static int rtp5901_set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV, unsigned *selector)
{
	struct rtp5901_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5901_regulator_info *info = pmic->info[id];
	struct rtp5901_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;

	printk(KERN_DEBUG "Set regulator-%d voltage as %d uV\n", id, min_uV);
	if (check_range(info, min_uV, max_uV)) {
		pr_err("invalid voltage range (%d, %d) uV\n", min_uV, max_uV);
		return -EINVAL;
	}

	val = (min_uV - info->min_uV + info->step_uV - 1) / info->step_uV;
	*selector = val;
	val += info->vol_offset;
	val <<= info->vol_shift;
	mask = ((1 << info->vol_nbits) - 1) << info->vol_shift;

	return rtp5901_update_bits(chip, info->vol_reg, val, mask);
}

static int rtp5901_get_voltage(struct regulator_dev *rdev)
{
	struct rtp5901_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5901_regulator_info *info = pmic->info[id];
	struct rtp5901_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;
	int ret;

	printk(KERN_DEBUG "Get regulator-%d voltage\n", id);
	ret = rtp5901_reg_read(chip, info->vol_reg, &val);
	if (ret)
		return ret;

	mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;
	val = (val & mask) >> info->vol_shift;
	val -= info->vol_offset;
	ret = info->min_uV + info->step_uV * val;

	return ret;
}

static int rtp5901_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	struct rtp5901_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5901_regulator_info *info = pmic->info[id];
	struct rtp5901_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;

	printk(KERN_DEBUG "Set regulator-%d voltage selector as %d\n", id, selector);
	if (selector < 0 || selector >= info->desc.n_voltages) {
		pr_err("invalid selector range (%d, %d) uV, selector = %d\n", 0, info->desc.n_voltages - 1, selector);
		return -EINVAL;
	}

	val = selector;
	val += info->vol_offset;
	val <<= info->vol_shift;
	mask = ((1 << info->vol_nbits) - 1) << info->vol_shift;

	return rtp5901_update_bits(chip, info->vol_reg, val, mask);
}

static int rtp5901_get_voltage_sel(struct regulator_dev *rdev)
{
	struct rtp5901_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5901_regulator_info *info = pmic->info[id];
	struct rtp5901_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;
	int ret;

	printk(KERN_DEBUG "Get regulator-%d voltage selector\n", id);
	ret = rtp5901_reg_read(chip, info->vol_reg, &val);
	if (ret)
		return ret;

	mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;
	val = (val & mask) >> info->vol_shift;
	val -= info->vol_offset;
	if (val < 0 || val >= info->desc.n_voltages) {
		pr_err("invalid selector range (%d, %d) uV, val = %d\n", 0, info->desc.n_voltages - 1, val);
		return -EINVAL;
	}
	ret = val;

	return ret;
}

static int rtp5901_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	struct rtp5901_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5901_regulator_info *info = pmic->info[id];
	int ret;

#if 0 /* Reduce debug messages */
	printk(KERN_DEBUG "List regulator-%d voltage of selector-%d\n", id, selector);
	printk(KERN_DEBUG " |_ min_uV = %d, max_uV = %d, step_uV = %d\n",info->min_uV, info->max_uV, info->step_uV);
#endif
	ret = info->min_uV + info->step_uV * selector;

	if (ret > info->max_uV)
		return -EINVAL;

	return ret;
}

static int rtp5901_set_voltage_time_sel(struct regulator_dev *rdev,
		unsigned int old_selector, unsigned int new_selector)
{
	int old_volt, new_volt, ret, step;
	struct rtp5901_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5901_mfd_chip *chip = pmic->chip;
	uint8_t val;

	printk(KERN_DEBUG "Set regulator-%d voltage time from selector %u to selector %u\n", id, old_selector, new_selector);
	old_volt = rtp5901_list_voltage(rdev, old_selector);
	if (old_volt < 0)
		return old_volt;

	new_volt = rtp5901_list_voltage(rdev, new_selector);
	if (new_volt < 0)
		return new_volt;

	ret = rtp5901_reg_read(chip, RTP5901_REG_DVS_STEP_CTRL, &val);
	if (ret)
		return ret;

	switch (id) {
		case RTP5901_ID_BUCK1:
			step = 0x20 >> (val & 0x03);
			break;
		case RTP5901_ID_BUCK2:
			step = 0x20 >> ((val >> 2) & 0x03);
			break;
		case RTP5901_ID_BUCK3:
			step = 0x20 >> ((val >> 4) & 0x03);
			break;
		case RTP5901_ID_BUCK4:
			step = 0x20 >> ((val >> 6) & 0x03);
			break;
		default:
			return -EINVAL;
	}
	ret = DIV_ROUND_UP(abs(old_volt - new_volt), RTP5901_DVS_STEP_US) * step;
	return ret;
}

static int rtp5901_enable(struct regulator_dev *rdev)
{
	struct rtp5901_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5901_regulator_info *info = pmic->info[id];
	struct rtp5901_mfd_chip *chip = pmic->chip;

	printk(KERN_DEBUG "Enable regulator-%d\n", id);
	return rtp5901_set_bits(chip, info->enable_reg, 1 << info->enable_bit);
}

static int rtp5901_disable(struct regulator_dev *rdev)
{
	struct rtp5901_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5901_regulator_info *info = pmic->info[id];
	struct rtp5901_mfd_chip *chip = pmic->chip;

	printk(KERN_DEBUG "Disable regulator-%d\n", id);
	return rtp5901_clr_bits(chip, info->enable_reg, 1 << info->enable_bit);
}

static int rtp5901_enable_time(struct regulator_dev *rdev)
{
	int id = rdev_get_id(rdev);

	printk(KERN_DEBUG "Get regulator-%d enable time\n", id);
	return RTP5901_ENABLE_TIME_US;
}

static int rtp5901_is_enabled(struct regulator_dev *rdev)
{
	struct rtp5901_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5901_regulator_info *info = pmic->info[id];
	struct rtp5901_mfd_chip *chip = pmic->chip;
	uint8_t reg_val;
	int ret;

	printk(KERN_DEBUG "Get whether regulator-%d is enabled\n", id);
	ret = rtp5901_reg_read(chip, info->enable_reg, &reg_val);
	if (ret)
		return ret;

	ret = !!(reg_val & (1 << info->enable_bit));

	return ret;
}

static int rtp5901_suspend_enable(struct regulator_dev *rdev)
{
	struct rtp5901_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5901_regulator_info *info = pmic->info[id];
	struct rtp5901_mfd_chip *chip = pmic->chip;

	printk(KERN_DEBUG "Enable regulator-%d on suspend state\n", id);
	return rtp5901_clr_bits(chip, info->stb_enable_reg, 1 << info->enable_bit);
}

static int rtp5901_suspend_disable(struct regulator_dev *rdev)
{
	struct rtp5901_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5901_regulator_info *info = pmic->info[id];
	struct rtp5901_mfd_chip *chip = pmic->chip;

	printk(KERN_DEBUG "Disable regulator-%d on suspend state\n", id);
	return rtp5901_set_bits(chip, info->stb_enable_reg, 1 << info->enable_bit);
}

static int rtp5901_set_suspend_voltage(struct regulator_dev *rdev, int uV)
{
	struct rtp5901_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5901_regulator_info *info = pmic->info[id];
	struct rtp5901_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;

	printk(KERN_DEBUG "Set regulator-%d suspend voltage as %d uV\n", id, uV);
	if (check_range(info, uV, uV)) {
		pr_err("invalid voltage range (%d, %d) uV\n", uV, uV);
		return -EINVAL;
	}

	val = (uV - info->min_uV + info->step_uV - 1) / info->step_uV;
	val += info->vol_offset;
	val <<= info->vol_shift;
	mask = ((1 << info->vol_nbits) - 1) << info->vol_shift;

	return rtp5901_update_bits(chip, info->stb_vol_reg, val, mask);
}

static struct regulator_ops rtp5901_ops_dcdc = {
	.set_voltage_sel = rtp5901_set_voltage_sel,
	.get_voltage_sel = rtp5901_get_voltage_sel,
	.list_voltage = rtp5901_list_voltage,
	.set_voltage_time_sel = rtp5901_set_voltage_time_sel,
	.enable	= rtp5901_enable,
	.disable = rtp5901_disable,
	.enable_time = rtp5901_enable_time,
	.is_enabled = rtp5901_is_enabled,
	.set_suspend_enable = rtp5901_suspend_enable,
	.set_suspend_disable = rtp5901_suspend_disable,
	.set_suspend_voltage = rtp5901_set_suspend_voltage,
};

static struct regulator_ops rtp5901_ops = {
	.set_voltage = rtp5901_set_voltage,
	.get_voltage = rtp5901_get_voltage,
	.list_voltage = rtp5901_list_voltage,
	.enable	= rtp5901_enable,
	.disable = rtp5901_disable,
	.enable_time = rtp5901_enable_time,
	.is_enabled = rtp5901_is_enabled,
	.set_suspend_enable = rtp5901_suspend_enable,
	.set_suspend_disable = rtp5901_suspend_disable,
	.set_suspend_voltage = rtp5901_set_suspend_voltage,
};

static struct rtp5901_regulator_info rtp5901_regulator_infos[] = {
	RTP5901_LDO("RTP5901_LDO1", RTP5901_ID_LDO1, RTP5901_LDO1_VOL, RTP5901_LDO1_VOL, 0, RTP5901_REG_NONE_WORTH, 0, 0, RTP5901_REG_NONE_WORTH, 0, RTP5901_REG_NONE_WORTH, RTP5901_REG_NONE_WORTH, 0),
	RTP5901_LDO("RTP5901_LDO2", RTP5901_ID_LDO2, 1000, 2500, 100, RTP5901_REG_LDO2VOUT_SET, 0, 7, RTP5901_REG_POWER_EN, 4, RTP5901_REG_STB_LDO2VOUT, RTP5901_REG_POWER_STB_EN, 0),
	RTP5901_LDO("RTP5901_LDO3", RTP5901_ID_LDO3, 1800, 3300, 100, RTP5901_REG_LDO3VOUT_SET, 0, 7, RTP5901_REG_POWER_EN, 5, RTP5901_REG_STB_LDO3VOUT, RTP5901_REG_POWER_STB_EN, 0),
	RTP5901_LDO("RTP5901_LDO4", RTP5901_ID_LDO4, 1800, 3300, 100, RTP5901_REG_LDO4VOUT_SET, 0, 7, RTP5901_REG_POWER_EN, 6, RTP5901_REG_STB_LDO4VOUT, RTP5901_REG_POWER_STB_EN, 0),
	RTP5901_LDO("RTP5901_LDO5", RTP5901_ID_LDO5, 1800, 3300, 50, RTP5901_REG_LDO5VOUT_SET, 0, 7, RTP5901_REG_POWER_EN, 7, RTP5901_REG_STB_LDO5VOUT, RTP5901_REG_POWER_STB_EN, 0),
	RTP5901_BUCK("RTP5901_BUCK1", RTP5901_ID_BUCK1, 700, 2275, 25, RTP5901_REG_DC1VOUT_SET, 0, 6, RTP5901_REG_POWER_EN, 0, RTP5901_REG_STB_DC1VOUT, RTP5901_REG_POWER_STB_EN, 0),
	RTP5901_BUCK("RTP5901_BUCK2", RTP5901_ID_BUCK2, 700, 2275, 25, RTP5901_REG_DC2VOUT_SET, 0, 6, RTP5901_REG_POWER_EN, 1, RTP5901_REG_STB_DC2VOUT, RTP5901_REG_POWER_STB_EN, 0),
	RTP5901_BUCK("RTP5901_BUCK3", RTP5901_ID_BUCK3, 700, 2275, 25, RTP5901_REG_DC3VOUT_SET, 0, 6, RTP5901_REG_POWER_EN, 2, RTP5901_REG_STB_DC3VOUT, RTP5901_REG_POWER_STB_EN, 0),
	RTP5901_BUCK("RTP5901_BUCK4", RTP5901_ID_BUCK4, 1700, 3500, 100, RTP5901_REG_DC4VOUT_SET, 0, 6, RTP5901_REG_POWER_EN, 3, RTP5901_REG_STB_DC4VOUT, RTP5901_REG_POWER_STB_EN, 0x0A),
};


#if 0
static __devinit int rtp5901_regulator_probe(struct platform_device *pdev)
#else
static int rtp5901_regulator_probe(struct platform_device *pdev)
#endif
{
	struct rtp5901_mfd_chip *chip = dev_get_drvdata(pdev->dev.parent);
	struct rtp5901_regulator_info *info;
	struct regulator_init_data *reg_data;
	struct regulator_dev *rdev;
	struct rtp5901_regu *pmic;
	struct rtp5901_board *pmic_plat_data;
	int i, err;

	pmic_plat_data = dev_get_platdata(chip->dev);
	if (!pmic_plat_data)
		return -EINVAL;

	pmic = kzalloc(sizeof(*pmic), GFP_KERNEL);
	if (!pmic)
		return -ENOMEM;

	mutex_init(&pmic->mutex);
	pmic->chip = chip;
	platform_set_drvdata(pdev, pmic);

//	Give control of all register to control port
//	tps65910_set_bits(pmic->mfd, TPS65910_DEVCTRL,
//				DEVCTRL_SR_CTL_I2C_SEL_MASK);

	switch(rtp5901_chip_id(chip)) {
		case RTP5901_ID:
			pmic->num_regulators = ARRAY_SIZE(rtp5901_regulator_infos);
			info = rtp5901_regulator_infos;
			break;
		default:
			pr_err("Invalid rtp chip version\n");
			kfree(pmic);
			return -ENODEV;
	}

//		pmic->get_ctrl_reg = &tps65910_get_ctrl_register;
//		pmic->num_regulators = ARRAY_SIZE(tps65910_regs);
//		pmic->ext_sleep_control = tps65910_ext_sleep_control;
//		info = tps65910_regs;

	pmic->info = kcalloc(pmic->num_regulators, sizeof(struct rtp5901_regulator_info *), GFP_KERNEL);
	if (!pmic->info) {
		err = -ENOMEM;
		goto err_free_pmic;
	}

	pmic->rdev = kcalloc(pmic->num_regulators, sizeof(struct regulator_dev *), GFP_KERNEL);
	if (!pmic->rdev) {
		err = -ENOMEM;
		goto err_free_info;
	}

	for (i = 0; i < pmic->num_regulators && i < RTP5901_ID_REGULATORS_NUM; i++, info++) {
		reg_data = pmic_plat_data->rtp5901_pmic_init_data[i];

		if (!reg_data)
			continue;

		if (i < RTP5901_ID_BUCK1)
			info->desc.ops = &rtp5901_ops;
		else
			info->desc.ops = &rtp5901_ops_dcdc;

		pmic->info[i] = info;

#if 0
		rdev = regulator_register(&pmic->info[i]->desc, chip->dev, reg_data, pmic);
        struct regulator_dev *regulator_register(struct regulator_desc *regulator_desc,
                struct device *dev,
                const struct regulator_init_data *init_data,
                void *driver_data);
#else
        {
            struct regulator_config config = {
                .dev = chip->dev,
                .init_data = reg_data,
                .regmap = chip->regmap,
                .driver_data = pmic,
            };
            rdev = regulator_register(&pmic->info[i]->desc, &config);
        }
#endif
		if (IS_ERR(rdev)) {
			dev_err(chip->dev, "failed to register %s regulator\n", pdev->name);
			err = PTR_ERR(rdev);
			goto err_unregister_regulator;
		}

		pmic->rdev[i] = rdev;
	}


	return 0;

err_unregister_regulator:
	while (--i >= 0)
		regulator_unregister(pmic->rdev[i]);
	kfree(pmic->rdev);
err_free_info:
	kfree(pmic->info);
err_free_pmic:
	kfree(pmic);
	return err;
}

#if 0
static int __devexit rtp5901_regulator_remove(struct platform_device *pdev)
#else
static int rtp5901_regulator_remove(struct platform_device *pdev)
#endif
{
	struct rtp5901_regu *pmic = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < pmic->num_regulators; i++)
		regulator_unregister(pmic->rdev[i]);


	kfree(pmic->rdev);
	kfree(pmic->info);
	kfree(pmic);
	return 0;
}


static struct platform_driver rtp5901_regulator_driver = {
	.driver	= {
		.name = "rtp5901-pmic",
		.owner = THIS_MODULE,
	},
	.probe = rtp5901_regulator_probe,
#if 0
	.remove	= __devexit_p(rtp5901_regulator_remove),
#else
	.remove	= rtp5901_regulator_remove,
#endif
};

static int __init rtp5901_regulator_init(void)
{
	return platform_driver_register(&rtp5901_regulator_driver);
}
#if 0
subsys_initcall(rtp5901_regulator_init);
#else
#if 0
late_initcall(rtp5901_regulator_init);
#else
fs_initcall(rtp5901_regulator_init);
#endif
#endif

static void __exit rtp5901_regulator_exit(void)
{
	platform_driver_unregister(&rtp5901_regulator_driver);
}
module_exit(rtp5901_regulator_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek Power Manager Driver");
