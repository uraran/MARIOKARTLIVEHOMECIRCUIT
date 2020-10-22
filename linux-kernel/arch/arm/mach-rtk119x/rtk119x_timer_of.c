#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk/rtk.h>

#include <asm/localtimer.h>

#define RTK_TIMER_HZ CONFIG_HZ

void rtk_clockevent_init(int index, const char *name,
		void __iomem *base, int irq, unsigned long freq);
void rtk_clocksource_init(void);

static void timer_get_base_and_rate(struct device_node *np,
					void __iomem **base, u32 *rate)
{
	*base = of_iomap(np, 0);

	if (!*base)
		panic("Can't map registers for %s", np->name);

	if (of_property_read_u32(np, "clock-freq", rate) &&
		of_property_read_u32(np, "clock-frequency", rate))
		panic("No clock-frequency property for %s", np->name);
}

static void add_clockevent(struct device_node *event_timer, int index)
{
	void __iomem *iobase;
	int    irq, rate;
	irq = irq_of_parse_and_map(event_timer, 0);
	if (irq <= 0)
		panic("Can't parse IRQ");

	timer_get_base_and_rate(event_timer, &iobase, &rate);

	rtk_clockevent_init(index, event_timer->name, iobase, irq, rate);
}

static const struct of_device_id osctimer_ids[] __initconst = {
	{ .compatible = "Realtek,rtk119x-timer" },
	{/*Sentinel*/},
};

#if 0
void __init rtk119x_timer_init(void)
#else
void rtk119x_timer_init(void)
#endif
{
	struct device_node __maybe_unused *event_timer0;
	struct device_node __maybe_unused *event_timer1;
#ifdef CONFIG_COMMON_CLK_RTK119X
    rtk_init_clocks();
#endif

	event_timer0 = of_find_matching_node(NULL, osctimer_ids);
	if (!event_timer0)
		panic("No timer0 for clockevent");

	add_clockevent(event_timer0, 0);

#ifdef CONFIG_HIGH_RES_TIMERS
	event_timer1 = of_find_matching_node(event_timer0, osctimer_ids);
	if (!event_timer1)
		panic("No timer1 for clockevent");

	add_clockevent(event_timer1, 1);
	of_node_put(event_timer1);
#endif
	rtk_clocksource_init();

#ifdef CONFIG_ARM_ARCH_TIMER
	writel(0x1, IOMEM(0xff018000));
	clocksource_of_init();
#endif
}
