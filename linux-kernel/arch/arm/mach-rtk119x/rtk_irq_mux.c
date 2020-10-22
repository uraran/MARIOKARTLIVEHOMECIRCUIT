#include <linux/err.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <asm/exception.h>
#include <asm/mach/irq.h>

#include "drivers/irqchip/irqchip.h"

#include "driver/debug.h"

#define IRQ_INMUX		32

#define rtd_outl(offset, val)		__raw_writel(val, offset)
#define rtd_setbits(offset, Mask)	__raw_writel(((__raw_readl(offset) | Mask)), offset)
#define rtd_clearbits(offset, Mask)	__raw_writel(((__raw_readl(offset) & ~Mask)), offset)

static DEFINE_SPINLOCK(irq_mux_lock);

struct irq_mux_data {
	void __iomem		*base;
	unsigned int		irq;
	unsigned int		irq_offset;
	u32					intr_status;
	u32					intr_en;
};

static struct irq_domain *irq_mux_domain;

static void mux_mask_irq(struct irq_data *data)
{
	struct irq_mux_data *mux_data = irq_data_get_irq_chip_data(data);
	void __iomem *base;
	u32 reg_st;

	mux_data += (data->hwirq / IRQ_INMUX);
	base = mux_data->base;
	reg_st = mux_data->intr_status;

	rtd_setbits(base+reg_st, BIT(data->hwirq % IRQ_INMUX));
}

static void mux_unmask_irq(struct irq_data *data)
{
	struct irq_mux_data *mux_data = irq_data_get_irq_chip_data(data);
	void __iomem *base;
	u32 reg_en;

	mux_data += (data->hwirq / IRQ_INMUX);
	base = mux_data->base;
	reg_en = mux_data->intr_en;

	rtd_setbits(base+reg_en, BIT(data->hwirq % IRQ_INMUX));
}

static void mux_disable_irq(struct irq_data *data)
{
	struct irq_mux_data *mux_data = irq_data_get_irq_chip_data(data);
	void __iomem *base;
	u32 reg_en;

	mux_data += (data->hwirq / IRQ_INMUX);
	base = mux_data->base;
	reg_en = mux_data->intr_en;

	rtd_clearbits(base+reg_en, BIT(data->hwirq % IRQ_INMUX));
}

#ifdef CONFIG_SMP
__maybe_unused static int mux_set_affinity(struct irq_data *d,
			const struct cpumask *mask_val, bool force)
{
	struct irq_mux_data *mux_data = irq_data_get_irq_chip_data(d);
	struct irq_chip *chip = irq_get_chip(mux_data->irq);
	struct irq_data *data = irq_get_irq_data(mux_data->irq);

	if (chip && chip->irq_set_affinity)
		return chip->irq_set_affinity(data, mask_val, force);
	else
		return -EINVAL;
}
#endif

static struct irq_chip mux_chip = {
	.name		= "IRQMUX",
	.irq_mask	= mux_mask_irq,
	.irq_unmask	= mux_unmask_irq,
	.irq_disable= mux_disable_irq,
#ifdef CONFIG_SMP
	.irq_set_affinity	= mux_set_affinity,
#endif
};

static void mux_irq_handle(unsigned int irq, struct irq_desc *desc)
{
	//struct irq_mux_data *mux_data = irq_desc_get_handler_data(desc);
	struct irq_mux_data *mux_data = irq_get_handler_data(irq);
	struct irq_chip *chip = irq_get_chip(irq);
	static u32 count=0;
    int ret;
	unsigned int mux_irq;
	unsigned long status, check_status;
	unsigned long enable;
	u32 reg_st = mux_data->intr_status;
	u32 reg_en = mux_data->intr_en;

	chained_irq_enter(chip, desc);

    spin_lock(&irq_mux_lock);
    enable = __raw_readl(mux_data->base+reg_en);
    status = __raw_readl(mux_data->base+reg_st);
    spin_unlock(&irq_mux_lock);

    /* MISC_TOP : irq_offset is zero.
     *  The I2C3 bit is not linear mapping, adjust the enable bit to match the status.
     *      enable is bit[28]
     *      status is bit[23]
     */
    if (mux_data->irq_offset == 0) {
        if (enable & (0x1U << 28))
            enable |= (0x1U << 23);
        else
            enable &= ~(0x1U << 23);
        enable &= ~(0x1U << 28);
    }

    status &= enable;

    if (status == 0) {
        printk(KERN_ERR "[%s] irq(%u) handle is not found. (st:0x%08lx en:0x%08lx)\n", __func__, irq, status, enable);
        goto out;
    }

    mux_irq = mux_data->irq_offset + __ffs(status);
    ret = generic_handle_irq(irq_find_mapping(irq_mux_domain, mux_irq));

    if (ret != 0) {
        printk(KERN_ERR "[%s] irq(%u) desc is not found. (st:0x%08lx en:0x%08lx)\n", __func__, irq, status, enable);
        goto out;
    }

    spin_lock(&irq_mux_lock);
    check_status = __raw_readl(mux_data->base+reg_st);
    check_status &= enable;
    spin_unlock(&irq_mux_lock);

    if (check_status == status) {
		if(count>1)
			printk(KERN_ERR "[%s] irq(%u) status is not change. clear it! (st:0x%08lx en:0x%08lx), count(%u)\n", __func__, irq, status, enable, count);
		else
			count++;

        spin_lock(&irq_mux_lock);
        rtd_setbits(mux_data->base+reg_st, BIT(__ffs(status)));
        spin_unlock(&irq_mux_lock);
    }
	else
	{
		count = 0;
	}
out:
	chained_irq_exit(chip, desc);
}

__maybe_unused static int mux_irq_domain_xlate(struct irq_domain *d,
				struct device_node *controller,
				const u32 *intspec, unsigned int intsize,
				unsigned long *out_hwirq,
				unsigned int *out_type)
{
	if (d->of_node != controller)
		return -EINVAL;

	if (intsize < 2)
		return -EINVAL;

	*out_hwirq = intspec[0]*IRQ_INMUX + intspec[1];
	*out_type = 0;

	return 0;
}

static int mux_irq_domain_map(struct irq_domain *d,
				unsigned int irq, irq_hw_number_t hw)
{
	struct irq_mux_data *data = d->host_data;

	irq_set_chip_and_handler(irq, &mux_chip, handle_level_irq);
	irq_set_chip_data(irq, data);
	set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);

	return 0;
}

static struct irq_domain_ops mux_irq_domain_ops = {
	.xlate	= mux_irq_domain_xlate,
	.map	= mux_irq_domain_map,
};

static void __init mux_init_each(struct irq_mux_data *mux_data, void __iomem *base,
								u32 irq, u32 status, u32 enable, int nr)
{
	mux_data->base = base;
	mux_data->irq = irq;
	mux_data->irq_offset = nr*IRQ_INMUX;
	mux_data->intr_status = status;
	mux_data->intr_en = enable;

	rtd_clearbits(base+enable, BIT(2));
	rtd_outl(base+status, BIT(2));

	if (irq_set_handler_data(irq, mux_data) != 0)
		BUG();
	irq_set_chained_handler(irq, mux_irq_handle);
}

#ifdef CONFIG_OF
static int __init mux_of_init(struct device_node *np,
					struct device_node *parent)
{
	int i;
	u32 nr_irq=1;
	struct irq_mux_data *mux_data;
	void __iomem *base;
	u32 irq;
	u32 status, enable;

	if (WARN_ON(!np))
		return -ENODEV;

	if (of_property_read_u32(np, "Realtek,mux-nr", &nr_irq)) {
		dbg_err("%s can not specified mux number.", __func__);
	}

	mux_data = kcalloc(nr_irq, sizeof(*mux_data), GFP_KERNEL);
	WARN(!mux_data,"could not allocate MUX IRQ data");

/*TODO : tempary define the first irq number in this mux is 160, end in 160+64*/
	irq_mux_domain = irq_domain_add_simple(np, (nr_irq*IRQ_INMUX), 160,
			&mux_irq_domain_ops, mux_data);
	WARN(!irq_mux_domain, "IRQ domain init failed\n");

	for (i=0; i < nr_irq; i++) {
		base = of_iomap(np, i);
		WARN(!(base), "unable to map IRQ base registers\n");

		irq = irq_of_parse_and_map(np, i);
		WARN(!(irq), "can not map IRQ.\n");

		of_property_read_u32_index(np, "intr-status", i, &status);
		of_property_read_u32_index(np, "intr-en", i, &enable);

		mux_init_each(mux_data, base, irq, status, enable, i);

		mux_data++;
	}

	return 0;
}

IRQCHIP_DECLARE(rtk_irq_mux, "Realtek,rtk-irq-mux", mux_of_init);
#endif
