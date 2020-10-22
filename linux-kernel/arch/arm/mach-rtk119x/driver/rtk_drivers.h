#ifndef _RTK_DRIVERS_H_
#define _RTK_DRIVERS_H_

#include <linux/platform_device.h>

/*
 * This is the platform device platform_data structure
 */
struct plat_rtk_drivers {
	void __iomem	*membase;	/* ioremap cookie or NULL */
	resource_size_t	mapbase;	/* resource base */
	unsigned int	irq;		/* interrupt number */
	unsigned long	irqflags;	/* request_irq flags */
	void			*private_data;
//	int		(*handle_irq)(struct uart_port *);
};

#endif //_RTK_DRIVERS_H_
