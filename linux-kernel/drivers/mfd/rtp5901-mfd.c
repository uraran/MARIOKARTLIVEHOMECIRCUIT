/*
 * rtp5901-mfd.c  --  Realsil RTP5901
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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/mfd/core.h>
#include <linux/mfd/rtp5901-mfd.h>
#include <linux/delay.h>
#include <linux/mfd/core.h>

struct rtp5901_mfd_chip *g_chip;
static uint8_t rtp5901_reg_addr = 0;

static struct mfd_cell rtp5901s[] = {
	{
		.name = "rtp5901-pmic",
	},
	{
		.name = "rtp5901-rtc",
	},
	{
		.name = "rtp5901-power",
	},
};

// for debug
#ifdef rtp5901_debug
static uint8_t rtp5901_reg_buffer[0xFF];
static int rtp5901_i2c_read(struct rtp5901_mfd_chip *chip, uint8_t reg, int count, uint8_t *val)
{
	int i;

	if (reg < 0 || count <= 0 || reg + count >= 0xFF)
		return -1;

	for (i = 0; i < count; i++) {
		val[i] = rtp5901_reg_buffer[reg + i];
	}

	return 0;
}

static int rtp5901_i2c_write(struct rtp5901_mfd_chip *chip, uint8_t reg, int count, uint8_t *val)
{
	int i;

	if (reg < 0 || count <= 0 || reg + count >= 0xFF)
		return -1;

	for (i = 0; i < count; i++) {
		rtp5901_reg_buffer[reg + i] = val[i];
	}

	return 0;
}
#else
#define MAX_RETRY_CNT 10
//#define RTP5901_SPEED	200 * 1000
static int rtp5901_i2c_read(struct rtp5901_mfd_chip *chip, uint8_t reg, int count, uint8_t *val)
{
	struct i2c_client *i2c = chip->i2c_client;
	struct i2c_msg xfer[2];
	int ret;
    int retry_cnt = MAX_RETRY_CNT;

    while (retry_cnt > 0) {

	/* Write register */
	xfer[0].addr = i2c->addr;
	xfer[0].flags = 0;
	xfer[0].len = 1;
	xfer[0].buf = &reg;
	//xfer[0].scl_rate = RTP5901_SPEED;

	/* Read data */
	xfer[1].addr = i2c->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = count;
	xfer[1].buf = val;
	//xfer[1].scl_rate = RTP5901_SPEED;

	ret = i2c_transfer(i2c->adapter, xfer, 2);

	if (ret == 2)
		ret = 0;
	else if (ret >= 0)
		ret = -EIO;

    if (ret == 0) {
        break;
    } else {
        retry_cnt--;
        mdelay(1);
    }
    }

    if (retry_cnt <= 0)
        printk(KERN_ERR "ERROR! %s reg 0x%x \n", __func__, reg);
    else if (retry_cnt < MAX_RETRY_CNT)
        printk(KERN_WARNING "WARNING! %s reg 0x%x (retry:%d)\n", __func__, reg, (MAX_RETRY_CNT-retry_cnt));

	return ret;
}

static int rtp5901_i2c_write(struct rtp5901_mfd_chip *chip, uint8_t reg, int count, uint8_t *val)
{
	struct i2c_client *i2c = chip->i2c_client;
	uint8_t msg[RTP5901_REG_MAX_REGISTER + 1];
	int ret;
    int retry_cnt = MAX_RETRY_CNT;

    while (retry_cnt > 0) {

	if (count > RTP5901_REG_MAX_REGISTER)
		return -EINVAL;

	msg[0] = reg;
	memcpy(&msg[1], val, count);

#if 0
	ret = i2c_master_normal_send(i2c, msg, count + 1, RTP5901_SPEED);
#else
    ret = i2c_master_send(i2c, msg, count + 1);
#endif
#if 0
	if (ret < 0)
		return ret;
	if (ret != count + 1)
		return -EIO;
#else

    if (ret > 0 && ret == (count + 1)) {
        ret = 0;
        break;
    } else {
        if (ret > 0 && ret != count + 1)
            ret = -EIO;
        retry_cnt--;
        mdelay(1);
    }
#endif
    }

    if (retry_cnt <= 0)
        printk(KERN_ERR "ERROR! %s reg 0x%x \n", __func__, reg);
    else if (retry_cnt < MAX_RETRY_CNT)
        printk(KERN_WARNING "WARNING! %s reg 0x%x (retry:%d)\n", __func__, reg, (MAX_RETRY_CNT-retry_cnt));

#if 0
	return 0;
#else
    return ret;
#endif
}
#endif

// reg r/w
int rtp5901_reg_read(struct rtp5901_mfd_chip *chip, uint8_t reg, uint8_t *val)
{
	int ret;

	printk(KERN_DEBUG "Read from reg 0x%x\n", reg);
	mutex_lock(&chip->io_mutex);

	ret = chip->read(chip, reg, 1, val);
	if (ret < 0)
		dev_err(chip->dev, "Read from reg 0x%x failed\n", reg);

	mutex_unlock(&chip->io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(rtp5901_reg_read);

int rtp5901_reg_write(struct rtp5901_mfd_chip *chip, uint8_t reg, uint8_t *val)
{
	int ret;

	printk(KERN_DEBUG "Write for reg 0x%x\n", reg);
	mutex_lock(&chip->io_mutex);

	ret = chip->write(chip, reg, 1, val);
	if (ret < 0)
		dev_err(chip->dev, "Write for reg 0x%x failed\n", reg);

	mutex_unlock(&chip->io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(rtp5901_reg_write);

int rtp5901_bulk_read(struct rtp5901_mfd_chip *chip, uint8_t reg, int count, uint8_t *val)
{
	int ret;

	printk(KERN_DEBUG "Read bulk from reg 0x%x\n", reg);
	mutex_lock(&chip->io_mutex);

	ret = chip->read(chip, reg, count, val);
	if (ret < 0)
		dev_err(chip->dev, "Read bulk from reg 0x%x failed\n", reg);

	mutex_unlock(&chip->io_mutex);

	return ret;
}
EXPORT_SYMBOL_GPL(rtp5901_bulk_read);

int rtp5901_bulk_write(struct rtp5901_mfd_chip *chip, uint8_t reg, int count, uint8_t *val)
{
	int ret;

	printk(KERN_DEBUG "Write bulk for reg 0x%x\n", reg);
	mutex_lock(&chip->io_mutex);

	ret = chip->write(chip, reg, count, val);
	if (ret < 0)
		dev_err(chip->dev, "Write bulk for reg 0x%x failed\n", reg);

	mutex_unlock(&chip->io_mutex);

	return ret;
}
EXPORT_SYMBOL_GPL(rtp5901_bulk_write);

int rtp5901_set_bits(struct rtp5901_mfd_chip *chip, int reg, uint8_t bit_mask)
{
	uint8_t val;
	int ret = 0;

	printk(KERN_DEBUG "Set reg 0x%x with bit mask 0x%x\n", reg, bit_mask);
	mutex_lock(&chip->io_mutex);

	ret = chip->read(chip, reg, 1, &val);
	if (ret < 0) {
		dev_err(chip->dev, "Read from reg 0x%x failed\n", reg);
		goto out;
	}

	if ((val & bit_mask) != bit_mask) {
		val |= bit_mask;
		ret = chip->write(chip, reg, 1, &val);
		if (ret < 0)
			dev_err(chip->dev, "Write for reg 0x%x failed\n", reg);
	}
out:
	mutex_unlock(&chip->io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(rtp5901_set_bits);

int rtp5901_clr_bits(struct rtp5901_mfd_chip *chip, int reg, uint8_t bit_mask)
{
	uint8_t val;
	int ret = 0;

	printk(KERN_DEBUG "Clear reg 0x%x with bit mask 0x%x\n", reg, bit_mask);
	mutex_lock(&chip->io_mutex);

	ret = chip->read(chip, reg, 1, &val);
	if (ret < 0) {
		dev_err(chip->dev, "Read from reg 0x%x failed\n", reg);
		goto out;
	}

	if (val & bit_mask) {
		val &= ~bit_mask;
		ret = chip->write(chip, reg, 1, &val);
		if (ret < 0)
			dev_err(chip->dev, "Write for reg 0x%x failed\n", reg);
	}
out:
	mutex_unlock(&chip->io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(rtp5901_clr_bits);

int rtp5901_update_bits(struct rtp5901_mfd_chip *chip, int reg, uint8_t reg_val, uint8_t mask)
{
	uint8_t val;
	int ret = 0;

	printk(KERN_DEBUG "Update reg 0x%x with val 0x%x & bit mask 0x%x\n", reg, reg_val, mask);
	mutex_lock(&chip->io_mutex);

	ret = chip->read(chip, reg, 1, &val);
	if (ret < 0) {
		dev_err(chip->dev, "Read from reg 0x%x failed\n", reg);
		goto out;
	}

	if ((val & mask) != reg_val) {
		val = (val & ~mask) | reg_val;
		ret = chip->write(chip, reg, 1, &val);
		if (ret < 0)
			dev_err(chip->dev, "Write for reg 0x%x failed\n", reg);
	}
out:
	mutex_unlock(&chip->io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(rtp5901_update_bits);

// irqs ops
static int rtp5901_init_irqs(struct rtp5901_mfd_chip *chip)
{
        uint8_t v1[7] = {0x00, 0xff, 0x00, 0x00, 0xc0, 0x0f, 0x37};
        uint8_t v2[7] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        int err;

	printk(KERN_DEBUG "Init rtp5901 irqs\n");
        err =  rtp5901_bulk_write(chip, RTP5901_REG_ALDOIN_IRQ_EN, 7, v1);
        if (err) {
                printk("[RTP5901-MFD] try to init irq_en failed!\n");
                return err;
        }

        err =  rtp5901_bulk_write(chip, RTP5901_REG_AVCC_IRQ_STS2, 7, v2);
        if (err) {
                printk("[RTP5901-MFD] try to init irq_sts failed!\n");
                return err;
        }

        chip->irq_enable = 0x00000000 | (uint64_t) 0x00000000 << 32;
        chip->update_irqs_en(chip);

        return 0;
}

static int rtp5901_update_irqs_enable(struct rtp5901_mfd_chip *chip)
{
	int ret = 0;
	uint64_t irqs;
	uint8_t v[7] = {0, 0, 0, 0, 0, 0, 0};

	printk(KERN_DEBUG "Update rtp5901 irqs enable\n");
	ret =  rtp5901_bulk_read(chip, RTP5901_REG_ALDOIN_IRQ_EN, 7, v);
	if (ret < 0)
		return ret;

	irqs = (((uint64_t) v[6]) << 48) | (((uint64_t) v[5]) << 40) |
	       (((uint64_t) v[4]) << 32) | (((uint64_t) v[3]) << 24) |
	       (((uint64_t) v[2]) << 16) | (((uint64_t) v[1]) << 8) |
	       ((uint64_t) v[0]);

	if (chip->irq_enable != irqs) {
		v[0] = ((chip->irq_enable) & 0xff);
		v[1] = ((chip->irq_enable) >> 8) & 0xff;
		v[2] = ((chip->irq_enable) >> 16) & 0xff;
		v[3] = ((chip->irq_enable) >> 24) & 0xff;
		v[4] = ((chip->irq_enable) >> 32) & 0xff;
		v[5] = ((chip->irq_enable) >> 40) & 0xff;
		v[6] = ((chip->irq_enable) >> 48) & 0xff;
		ret =  rtp5901_bulk_write(chip, RTP5901_REG_ALDOIN_IRQ_EN, 7, v);
	}

	return ret;
}

static int rtp5901_read_irqs(struct rtp5901_mfd_chip *chip, uint64_t *irqs)
{
	uint8_t v[7] = {0, 0, 0, 0, 0, 0, 0};
	int ret;

	printk(KERN_DEBUG "Read rtp5901 irqs status\n");
	ret =  rtp5901_bulk_read(chip, RTP5901_REG_AVCC_IRQ_STS2, 7, v);
	if (ret < 0)
		return ret;

	*irqs = (((uint64_t) v[0]) << 48) | (((uint64_t) v[6]) << 40) |
		(((uint64_t) v[2]) << 32) | (((uint64_t) v[4]) << 24) |
		(((uint64_t) v[3]) << 16) | (((uint64_t) v[5]) << 8) |
		((uint64_t) v[1]);

	return 0;
}

static int rtp5901_write_irqs(struct rtp5901_mfd_chip *chip, uint64_t irqs)
{
	uint8_t v[7];
	int ret;

	printk(KERN_DEBUG "Write rtp5901 irqs status\n");
	v[0] = (irqs >> 48) & 0xff;
        v[1] = (irqs >> 0) & 0xff;
        v[2] = (irqs >> 32) & 0xff;
        v[3] = (irqs >> 16) & 0xff;
        v[4] = (irqs >> 24) & 0xff;
        v[5] = (irqs >> 8) & 0xff;
        v[6] = (irqs >> 40) & 0xff;

	ret = rtp5901_bulk_write(chip, RTP5901_REG_AVCC_IRQ_STS2, 7, v);
	if (ret < 0)
		return ret;

	return 0;
}

// attrs
static ssize_t rtp5901_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t val;
	struct rtp5901_mfd_chip *chip = i2c_get_clientdata(to_i2c_client(dev));

	rtp5901_reg_read(chip, rtp5901_reg_addr, &val);

	return sprintf(buf, "REG[%x]=%x\n", rtp5901_reg_addr, val);
}

static ssize_t rtp5901_reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	uint8_t val;
	struct rtp5901_mfd_chip *chip = i2c_get_clientdata(to_i2c_client(dev));

	tmp = simple_strtoul(buf, NULL, 16);
	if(tmp < 256)
		rtp5901_reg_addr = tmp;
	else {
		val = tmp & 0x00FF;
		rtp5901_reg_addr= (tmp >> 8) & 0x00FF;
		rtp5901_reg_write(chip, rtp5901_reg_addr, &val);
	}

	return count;
}

static uint8_t rtp5901_regs_addr = 0;
static int rtp5901_regs_len = 0;
static int rtp5901_regs_rw = 0;
static uint8_t rtp5901_regs_datas[256];
static ssize_t rtp5901_regs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        int count = 0;
	int i;
        struct rtp5901_mfd_chip *chip = i2c_get_clientdata(to_i2c_client(dev));

	if (rtp5901_regs_rw == 0) {
		rtp5901_bulk_read(chip, rtp5901_regs_addr, rtp5901_regs_len, rtp5901_regs_datas);
		for (i = 0; i < rtp5901_regs_len; i++) {
			count += sprintf(buf + count, "REG[%x]=%x\n", rtp5901_regs_addr + i, rtp5901_regs_datas[i]);
		}
	} else if (rtp5901_regs_rw == 1) {
		rtp5901_bulk_write(chip, rtp5901_regs_addr, rtp5901_regs_len, rtp5901_regs_datas);
		sprintf(buf, "bulk write ok\n");
	}

        return count;
}

static ssize_t rtp5901_regs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        int tmp;
	char buftemp[3];

	if (count < 6)
		return count;

	// rw flag
	buftemp[0] = buf[0];
	buftemp[1] = '\0';
	rtp5901_regs_rw = simple_strtoul(buftemp, NULL, 16);
	printk("<0>" "rtp5901_regs_store, rtp5901_regs_rw = %d\n", rtp5901_regs_rw);
	// addr
	buftemp[0] = buf[1];
	buftemp[1] = buf[2];
        buftemp[2] = '\0';
        rtp5901_regs_addr = simple_strtoul(buftemp, NULL, 16);
	printk("<0>" "rtp5901_regs_store, rtp5901_regs_addr = 0x%x\n", rtp5901_regs_addr);
	// addr
        buftemp[0] = buf[3];
        buftemp[1] = buf[4];
        buftemp[2] = '\0';
        rtp5901_regs_len = simple_strtoul(buftemp, NULL, 16);
	printk("<0>" "rtp5901_regs_store, rtp5901_regs_len = %d\n", rtp5901_regs_len);
	if (rtp5901_regs_rw == 1) {
		if (count != 5 + rtp5901_regs_len * 2 + 1) {
			printk("<0>" "rtp5901_regs_store error, count = %d\n", count);
		}
		for (tmp = 0; tmp < rtp5901_regs_len; tmp++) {
			buftemp[0] = buf[tmp * 2 + 5];
        		buftemp[1] = buf[tmp * 2 + 6];
        		buftemp[2] = '\0';
        		rtp5901_regs_datas[tmp] = simple_strtoul(buftemp, NULL, 16);
			printk("<0>" "rtp5901_regs_store, val[%x] = 0x%x\n", tmp + rtp5901_regs_addr, rtp5901_regs_datas[tmp]);
		}
	}

        return count;
}

static struct device_attribute rtp5901_mfd_attrs[] = {
	RTP_MFD_ATTR(rtp5901_reg),
	RTP_MFD_ATTR(rtp5901_regs),
};

int rtp5901_mfd_create_attrs(struct rtp5901_mfd_chip *chip)
{
	int j, ret;

	for (j = 0; j < ARRAY_SIZE(rtp5901_mfd_attrs); j++) {
		ret = device_create_file(chip->dev, &rtp5901_mfd_attrs[j]);
		if (ret)
			goto sysfs_failed;
	}
	goto succeed;

sysfs_failed:
	while (j--)
		device_remove_file(chip->dev, &rtp5901_mfd_attrs[j]);
succeed:
	return ret;
}

#if 0
static int rtp5901_mfd_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
#else
int rtp5901_mfd_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
#endif
{
	struct rtp5901_mfd_chip *chip;
	struct rtp5901_board *pmic_plat_data = dev_get_platdata(&i2c->dev);
	struct rtp5901_platform_data *init_data;
	int ret = 0;
	if (!pmic_plat_data)
		return -EINVAL;

	init_data = kzalloc(sizeof(struct rtp5901_platform_data), GFP_KERNEL);
	if (init_data == NULL)
		return -ENOMEM;

	init_data->irq = pmic_plat_data->irq;
	init_data->irq_base = pmic_plat_data->irq_base;

	chip = kzalloc(sizeof(struct rtp5901_mfd_chip), GFP_KERNEL);
	if (chip == NULL) {
		kfree(init_data);
		return -ENOMEM;
	}

	i2c_set_clientdata(i2c, chip);
	chip->dev = &i2c->dev;
	chip->i2c_client = i2c;
	chip->id = id->driver_data;
	chip->read = rtp5901_i2c_read;		// read
	chip->write = rtp5901_i2c_write;	// write
	chip->init_irqs = rtp5901_init_irqs;
	chip->update_irqs_en = rtp5901_update_irqs_enable;
	chip->read_irqs = rtp5901_read_irqs;
	chip->write_irqs = rtp5901_write_irqs;
	chip->pdata = init_data;
	mutex_init(&chip->io_mutex);

	/* verify the ic
	ret = rtp5901_reg_read(chip, 0x22);
	if ((ret < 0) || (ret == 0xff)){
		printk("The device is not rtp5901\n");
		goto err;
	} */

	g_chip = chip;

	if (pmic_plat_data && pmic_plat_data->pre_init) {
		ret = pmic_plat_data->pre_init(chip);			// do pre_init
		if (ret != 0) {
			dev_err(chip->dev, "pre_init() failed: %d\n", ret);
			goto err;
		}
	}

#ifdef CONFIG_GPIO_RTP5901
	ret = rtp5901_gpio_init(chip, pmic_plat_data->gpio_base);	// initial gpio
	if (ret < 0)
		goto err;
#endif

	ret = rtp5901_irq_init(chip, init_data->irq, init_data);	// initial irq
	if (ret < 0)
		goto irq_err;

#if 0
	ret = mfd_add_devices(chip->dev, -1, rtp5901s, ARRAY_SIZE(rtp5901s), NULL, 0);
#else
	ret = mfd_add_devices(chip->dev, -1, rtp5901s, ARRAY_SIZE(rtp5901s), NULL, 0, NULL);
#endif
	if (ret < 0)
		goto mfd_err;

	if (pmic_plat_data && pmic_plat_data->post_init) {
		ret = pmic_plat_data->post_init(chip);			// do post_init
		if (ret != 0) {
			dev_err(chip->dev, "post_init() failed: %d\n", ret);
			goto mfd_err;
		}
	}

	ret = rtp5901_mfd_create_attrs(chip);
	if (ret)
		goto mfd_err;

	printk("%s:irq=%d,irq_base=%d,gpio_base=%d\n",__func__,init_data->irq,init_data->irq_base,pmic_plat_data->gpio_base);
	return ret;

mfd_err:
	mfd_remove_devices(chip->dev);
	rtp5901_irq_exit(chip);
irq_err:
#ifdef CONFIG_GPIO_RTP5901
	rtp5901_gpio_exit(chip);
#endif
err:
	kfree(init_data);
	kfree(chip);
	return ret;
}

// power off
int rtp5901_power_off(void)
{
	int ret;
	struct rtp5901_mfd_chip *chip = g_chip;
	struct rtp5901_board *pmic_plat_data = dev_get_platdata(chip->dev);

	printk("%s\n", __func__);

	if (pmic_plat_data && pmic_plat_data->late_exit) {
		ret = pmic_plat_data->late_exit(chip);	// do late_exit
		if (ret != 0)
			dev_err(chip->dev, "late_exit() failed: %d\n", ret);
	}

	mdelay(20);
	rtp5901_set_bits(chip, RTP5901_REG_PMU_STATE_CTL, 0x80);
	mdelay(20);
	printk("warning!!! rtp can't power-off, maybe some error happend!\n");

	return 0;
}
EXPORT_SYMBOL_GPL(rtp5901_power_off);

#if 0
static int __devexit rtp5901_mfd_remove(struct i2c_client *client)
#else
static int rtp5901_mfd_remove(struct i2c_client *client)
#endif
{
	struct rtp5901_mfd_chip *chip = i2c_get_clientdata(client);
	int j;

	printk("<0>""rtp5901_mfd_remove\n");
	mfd_remove_devices(chip->dev);
	rtp5901_irq_exit(chip);
#ifdef CONFIG_GPIO_RTP5901
	rtp5901_gpio_exit(chip);
#endif
	i2c_set_clientdata(client, NULL);
	for (j = 0; j < ARRAY_SIZE(rtp5901_mfd_attrs); j++)
		device_remove_file(chip->dev, &rtp5901_mfd_attrs[j]);
	if (chip->pdata != NULL)
		kfree(chip->pdata);
	kfree(chip);
	g_chip = NULL;

	return 0;
}

static const struct i2c_device_id rtp5901_mfd_id_table[] = {
	{ "rtp5901_mfd", RTP5901_ID },
	{},
};

MODULE_DEVICE_TABLE(i2c, rtp5901_mfd_id_table);

static struct i2c_driver rtp5901_mfd_driver = {
	.driver = {
		.name = "rtp5901_mfd",
		.owner = THIS_MODULE,
	},
	.probe = rtp5901_mfd_probe,
#if 0
	.remove = __devexit_p(rtp5901_mfd_remove),
#else
	.remove = rtp5901_mfd_remove,
#endif
	.id_table = rtp5901_mfd_id_table,
};

static int __init rtp5901_mfd_init(void)
{
	return i2c_add_driver(&rtp5901_mfd_driver);
}
#if 0
subsys_initcall_sync(rtp5901_mfd_init);
#else
#if 0
late_initcall(rtp5901_mfd_init);
#else
fs_initcall(rtp5901_mfd_init);
#endif
#endif

static void __exit rtp5901_mfd_exit(void)
{
	i2c_del_driver(&rtp5901_mfd_driver);
}
module_exit(rtp5901_mfd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek Power Manager Driver");
