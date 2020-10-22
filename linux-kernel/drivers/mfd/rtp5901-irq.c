/*
 * rtp5901-irq.c  --  Realsil RTP5901
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
#include <linux/bug.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/mfd/rtp5901-mfd.h>
#include <linux/wakelock.h>
#include <linux/kthread.h>

static inline int irq_to_rtp5901_irq(struct rtp5901_mfd_chip *chip, int irq)
{
	return (irq - chip->irq_base);
}

static irqreturn_t rtp5901_irq(int irq, void *irq_data)
{
	struct rtp5901_mfd_chip *chip = irq_data;
	uint64_t irq_sts, irq_sts_en;
	uint8_t val;
	int i;

	wake_lock(&chip->irq_wake);
	rtp5901_reg_read(chip, RTP5901_REG_ALDOIN_IRQ_EN, &val);
	val &= 0x80;
	if (!val) {
		wake_unlock(&chip->irq_wake);
		return IRQ_NONE;
	}

	chip->read_irqs(chip, &irq_sts);
	irq_sts_en = irq_sts & chip->irq_enable;
	if (!irq_sts_en) {
		chip->write_irqs(chip, irq_sts);
		wake_unlock(&chip->irq_wake);
		return IRQ_NONE;
	}

	for (i = 0; i < chip->irq_num; i++) {

		if (!(irq_sts_en & (((uint64_t) 1) << i)))
			continue;

		handle_nested_irq(chip->irq_base + i);
	}

	chip->write_irqs(chip, irq_sts);
	wake_unlock(&chip->irq_wake);
	return IRQ_HANDLED;
}

static void rtp5901_irq_lock(struct irq_data *data)
{
	struct rtp5901_mfd_chip *chip = irq_data_get_irq_chip_data(data);

	mutex_lock(&chip->irq_lock);
}

static void rtp5901_irq_sync_unlock(struct irq_data *data)
{
	struct rtp5901_mfd_chip *chip = irq_data_get_irq_chip_data(data);

	chip->update_irqs_en(chip);

	mutex_unlock(&chip->irq_lock);
}

static void rtp5901_irq_enable(struct irq_data *data)
{
	struct rtp5901_mfd_chip *chip = irq_data_get_irq_chip_data(data);

	chip->irq_enable |= (((uint64_t) 1) << irq_to_rtp5901_irq(chip, data->irq));
}

static void rtp5901_irq_disable(struct irq_data *data)
{
	struct rtp5901_mfd_chip *chip = irq_data_get_irq_chip_data(data);

	chip->irq_enable &= ~(((uint64_t) 1) << irq_to_rtp5901_irq(chip, data->irq));
}

#ifdef CONFIG_PM_SLEEP
static int rtp5901_irq_set_wake(struct irq_data *data, unsigned int enable)
{
	struct rtp5901_mfd_chip *chip = irq_data_get_irq_chip_data(data);
	return irq_set_irq_wake(chip->chip_irq, enable);
}
#else
#define rtp5901_irq_set_wake NULL
#endif

static struct irq_chip rtp5901_irq_chip = {
	.name = "rtp5901_irq",
	.irq_bus_lock = rtp5901_irq_lock,
	.irq_bus_sync_unlock = rtp5901_irq_sync_unlock,
	.irq_disable = rtp5901_irq_disable,
	.irq_enable = rtp5901_irq_enable,
	.irq_set_wake = rtp5901_irq_set_wake,
};

int rtp5901_irq_init(struct rtp5901_mfd_chip *chip, int irq, struct rtp5901_platform_data *pdata)
{
	int ret, cur_irq;
	int flags = IRQF_ONESHOT;

	printk(KERN_DEBUG "Init rtp5901 irq, irq = %d, irq base = %d\n", irq, pdata->irq_base);

	if (!irq) {
		dev_warn(chip->dev, "No interrupt support, no core IRQ\n");
		return 0;
	}

	if (!pdata || !pdata->irq_base) {
		dev_warn(chip->dev, "No interrupt support, no IRQ base\n");
		return 0;
	}

	// Clear unattended interrupts
	chip->write_irqs(chip, 0xffffffff | (uint64_t) 0xffffffff << 32);

	mutex_init(&chip->irq_lock);
	wake_lock_init(&chip->irq_wake, WAKE_LOCK_SUSPEND, "rtp5901_irq_wake");
	chip->chip_irq = irq;
	chip->irq_base = pdata->irq_base;
	chip->irq_num = RTP5901_IRQ_NUM;

	/* Register with genirq */
	for (cur_irq = chip->irq_base;
	     cur_irq < chip->irq_num + chip->irq_base;
	     cur_irq++) {
		irq_set_chip_data(cur_irq, chip);
		irq_set_chip_and_handler(cur_irq, &rtp5901_irq_chip, handle_edge_irq);
		irq_set_nested_thread(cur_irq, 1);

#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}

	ret = request_threaded_irq(irq, NULL, rtp5901_irq, flags, "rtp5901", chip);

	irq_set_irq_type(irq, IRQ_TYPE_LEVEL_LOW);

	if (ret != 0)
		dev_err(chip->dev, "Failed to request IRQ: %d\n", ret);

	return ret;
}

int rtp5901_irq_exit(struct rtp5901_mfd_chip *chip)
{
	printk(KERN_DEBUG "Exit rtp5901 irq\n");

	if (chip->chip_irq)
		free_irq(chip->chip_irq, chip);
	return 0;
}
