#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>


static irqreturn_t input_detect_irq(int irq, void *dev_id)
{
	return IRQ_HANDLED;
}

static const struct of_device_id rtk_input_match[] __initconst = {
	{.compatible = "Realtek,virt-input"},
	{},
};

static int __init virt_input_init(void)
{
	struct device_node *np;
	int irq;
	int gpio;
	int ret;

	np = of_find_matching_node(NULL, rtk_input_match);
	if (unlikely(np == NULL))
		return -ENODEV;
	gpio = of_get_named_gpio(np, "realtek,virt-input-gpio", 0);
	if (!gpio_is_valid(gpio))
		return -ENODEV;

	irq = irq_of_parse_and_map(np, 0);

	of_node_put(np);

	irq_set_irq_type(irq, IRQ_TYPE_LEVEL_HIGH);
	ret = request_irq(irq, input_detect_irq,
//				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				IRQF_SHARED,
				"virt_input", NULL);
	if (ret) {
		pr_err("%s : Failed to register handler\n", __func__);
		return ret;
	}

	return 0;
}
late_initcall_sync(virt_input_init);
