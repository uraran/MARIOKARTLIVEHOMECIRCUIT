

//#ifndef	__RTK119X_GPIO_H
//#define	__RTK119X_GPIO_H

//#include <asm-generic/gpio.h>

#define __ARM_GPIOLIB_COMPLEX

/* The inline versions use the static inlines in the driver header */
#include "gpio_rtk119x.h"

#if 0

/*
 * The get/set/clear functions will inline when called with constant
 * parameters referencing built-in GPIOs, for low-overhead bitbanging.
 *
 * gpio_set_value() will inline only on traditional rtk119x style controllers
 * with distinct set/clear registers.
 *
 * Otherwise, calls with variable parameters or referencing external
 * GPIOs (e.g. on GPIO expander chips) use outlined functions.
 */
static inline void gpio_set_value(unsigned gpio, int value)
{
#if 0
	if (__builtin_constant_p(value) && gpio < rtk119x_soc_info.gpio_num) {
		struct rtk119x_gpio_controller *ctlr;
		u32				mask;

		ctlr = __gpio_to_controller(gpio);

		if (ctlr->set_data != ctlr->clr_data) {
			mask = __gpio_mask(gpio);
			if (value)
				__raw_writel(mask, ctlr->set_data);
			else
				__raw_writel(mask, ctlr->clr_data);
			return;
		}
	}

	__gpio_set_value(gpio, value);
#endif
}

/* Returns zero or nonzero; works for gpios configured as inputs OR
 * as outputs, at least for built-in GPIOs.
 *
 * NOTE: for built-in GPIOs, changes in reported values are synchronized
 * to the GPIO clock.  This is easily seen after calling gpio_set_value()
 * and then immediately gpio_get_value(), where the gpio_get_value() will
 * return the old value until the GPIO clock ticks and the new value gets
 * latched.
 */
static inline int gpio_get_value(unsigned gpio)
{
#if 0
	struct rtk119x_gpio_controller *ctlr;

	if (!__builtin_constant_p(gpio) || gpio >= rtk119x_soc_info.gpio_num)
		return __gpio_get_value(gpio);

	ctlr = __gpio_to_controller(gpio);
	return __gpio_mask(gpio) & __raw_readl(ctlr->in_data);
#else
	return 1;
#endif
}

static inline int gpio_cansleep(unsigned gpio)
{
#if 0
	if (__builtin_constant_p(gpio) && gpio < rtk119x_soc_info.gpio_num)
		return 0;
	else
		return __gpio_cansleep(gpio);
#else
	return 1;
#endif

}

static inline int irq_to_gpio(unsigned irq)
{
#if 0
	/* don't support the reverse mapping */
	return -ENOSYS;
#else
	return 1;
#endif

}
#endif
//#endif				/* __RTK119X_GPIO_H */
