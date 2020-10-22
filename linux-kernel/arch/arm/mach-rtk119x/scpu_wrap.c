#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include "driver/debug.h"

#define DBG_INT			0x230
#define DBG_ADDR		0x234
#define DBG_ADDR_MODE	0x238

static void __iomem *iobase;

irqreturn_t scpu_wrapper_isr(int irq, void *reg_base)
{
	struct pt_regs *regs;
	u32 intr = readl(IOMEM(iobase+DBG_INT));
	u32 addr = readl(IOMEM(iobase+DBG_ADDR));
	u32 cause = readl(IOMEM(iobase+DBG_ADDR_MODE));

	regs = get_irq_regs();

	dbg_err("scpu wrapper get int 0x%08x", intr);
	if (intr & (BIT(4) | BIT(2))) {

		writel((intr & ~(BIT(3)|BIT(1))), IOMEM(iobase+DBG_INT));

		dbg_err("scpu addr:0x%08x mode:%s", addr, (cause & 1) ? "W" : "R");
		//BUG();
		return IRQ_HANDLED;
	}
#if 0
	if (intr & ((1 << 4) | (1 << 10) | (1<<6) | (1<<12))) {
		u32 cause, addr, s_a_cpu;
		char buf[128];

		writel((1 << 9) | (1 << 7) | 1, SB2_DBG_INT);

		s_a_cpu = (intr & (1<<10)) ? 1 : 2;	/* SCPU:1, ACPU:2 */
		addr = (s_a_cpu==1) ? readl(SB2_DBG_ADDR_SYSTEM) : readl(SB2_DBG_ADDR_AUDIO);
		cause = readl(SB2_DBG_ADDR1);
		cause = (s_a_cpu==1) ? (cause >> 2) : (cause >> 4);

		sprintf(buf, "Memory 0x%08x trashed by %sCPU with %s %s\n", addr,
				(s_a_cpu == 1) ? "S" : "A",
				(cause & 1) ? "D" : "I",
				(cause & 2) ? "W" : "R");

		die(buf, regs, 0);

		return IRQ_HANDLED;
	}
#endif

	return IRQ_NONE;
}

static const struct of_device_id rtk_scpu_wrapper_ids[] __initconst = {
	{.compatible = "Realtek,rtk-scpu_wrapper"},
	{},
};

static int __init scpu_wrapper_init(void)
{
	struct device_node *np;
	int irq;

	np = of_find_matching_node(NULL, rtk_scpu_wrapper_ids);
	if (unlikely(np == NULL))
		return -1;

	irq = irq_of_parse_and_map(np, 0);

	iobase = of_iomap(np, 0);
	dbg_info("scpu wrappee irq %d, iobase 0x%08x\n", irq, (u32)iobase);

	of_node_put(np);

	if (request_irq(irq, scpu_wrapper_isr, IRQF_SHARED, "scpu_wrapper", iobase) != 0) {
		dbg_err("Cannot get IRQ\n");
		return -1;
	}
	/* clear dbg trap addr*/
	{
		int i=0;
		for (i=0; i<8; i++)
			writel(0, IOMEM(iobase+0x200+i*4));
		writel(0x18000000, IOMEM(iobase+0x200));	// 1st set start
		writel(0x18000040, IOMEM(iobase+0x210));	// 1st set end

		writel(0x18000040, IOMEM(iobase+0x204));	// 2nd set start
		writel(0x18000080, IOMEM(iobase+0x214));	// 2nd set end

		writel(0x18000080, IOMEM(iobase+0x208));	// 3th set start
		writel(0x180000c0, IOMEM(iobase+0x218));	// 3th set end

		writel(0x180000c0, IOMEM(iobase+0x20c));	// 4th set start
		writel(0x18000100, IOMEM(iobase+0x21c));	// 4th set end

		writel(0x9, IOMEM(iobase+DBG_INT));	// enable interrupt
	//	writel(0x1b, IOMEM(iobase+0x220));	// debug ctrl
	}

	return 0;
}
late_initcall(scpu_wrapper_init);

void scpu_dbg_disable_mem_monitor(int which)
{
	u32 *reg_ctrl = (u32 *) SB2_DBG_CTRL_REG0;

	reg_ctrl += which;
	writel((1 << 13) | (1 << 9) | (1 << 1), reg_ctrl);
}
EXPORT_SYMBOL(scpu_dbg_disable_mem_monitor);

static void scpu_dbg_set_mem_monitor(int which, u32 start, u32 end, u32 flags)
{
	u32 *reg_start, *reg_end, *reg_ctrl;

	/* disable this set first */
	scpu_dbg_disable_mem_monitor(which);

	reg_start = ((u32 *) SB2_DBG_START_REG0) + which;
	reg_end = ((u32 *) SB2_DBG_END_REG0) + which;
	reg_ctrl = ((u32 *) SB2_DBG_CTRL_REG0) + which;

	writel(start, reg_start);
	writel(end, reg_end);

	writel(flags, reg_ctrl);

	/*
	pr_info("sb2 0x%08x=0x%08x\n", (u32)reg_ctrl, readl(reg_ctrl));
	pr_info("sb2 0x%08x=0x%08x\n", (u32)reg_start, readl(reg_start));
	pr_info("sb2 0x%08x=0x%08x\n", (u32)reg_end, readl(reg_end));
	pr_info("sb2 0x%08x=0x%08x\n", (u32)SB2_DBG_INT, readl(SB2_DBG_INT));
	*/
}

/*
 * which: 0~7, which register set
 * d_i: SB2_DBG_MONITOR_DATA|SB2_DBG_MONITOR_INST
 * r_w: SB2_DBG_MONITOR_READ|SB2_DBG_MONITOR_WRITE
 */
void scpu_dbg_scpu_monitor(int which, u32 start, u32 end, u32 d_i, u32 r_w)
{
	scpu_dbg_set_mem_monitor(which, start, end, (1 << 13) | (3 << 8) | d_i | r_w | 3);
}
EXPORT_SYMBOL(scpu_dbg_scpu_monitor);

void scpu_dbg_acpu_monitor(int which, u32 start, u32 end, u32 d_i, u32 r_w)
{
	scpu_dbg_set_mem_monitor(which, start, end, (3 << 12) | (1 << 9) | d_i | r_w | 3);
}
EXPORT_SYMBOL(scpu_dbg_acpu_monitor);
