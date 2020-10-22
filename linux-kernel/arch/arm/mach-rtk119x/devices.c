#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include <rbus/reg.h>
#include <mach/irqs.h>

#include "driver/rtk_drivers.h"

static struct plat_rtk_drivers driver_platform_data[] = {
	{
		.membase		= MCP_IO_ADDR(0),
		.mapbase		= (resource_size_t)RBUS_MCP_PHYS(0),
		.irq			= 34,				// swc:34 nwc:69
	},
	{
		.membase		= _MISC_IO_ADDR(RTC_OFFSET),
		.mapbase		= (resource_size_t)RBUS_MISC_PHYS(RTC_OFFSET),
		.irq			= 71,
	},
};

static struct platform_device rtk_mcp = {
	.name	= "rtk_mcp",
	.id		= -1,
	.dev	= {
		.platform_data	= &driver_platform_data[0],
	},
};
#if 0 
static struct platform_device rtk_rtc = {
	.name	= "rtk_rtc",
	.id		= -1,
	.dev	= {
		.platform_data	= &driver_platform_data[1],
	},
};
#endif 
/* RBUS */
static struct resource rtk_rbus_resource[] = {
	[0] = {
		.start = 0x18000000,
		.end   = 0x18000000 + 0x60000 - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device rtk_device_rbus = {
	.name = "rtk_rbus",
	.id = -1,
	.num_resources = ARRAY_SIZE(rtk_rbus_resource),
	.resource = rtk_rbus_resource,
	.dev = {},
};

/* PU WRAPPER */
static struct resource rtk_puwrap_resource[] = {
	[0] = {
		.start = 0x18047000,
		.end   = 0x18047000 + 0xc00 - 1,
		.flags = IORESOURCE_MEM,
	},

	[1] = {
		.start = IRQ_VE,
		.end   = IRQ_VE,
		.flags = IORESOURCE_IRQ,
	},
#if 0
	[2] = {
		.start = VE_BASE_PHYS,
		.end   = VE_BASE_PHYS + VE_BASE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
#endif
};

static struct platform_device rtk_device_puwrap = {
	.name = "rtk_puwrap",
	.id = -1,
	.num_resources = ARRAY_SIZE(rtk_puwrap_resource),
	.resource = rtk_puwrap_resource,
	.dev = {},
};

/* VPU */
static struct resource rtk_vpu_resource[] = {
	[0] = {
		.start = 0x18040000,
		.end   = 0x18040000 + 0xc000 - 1,
		.flags = IORESOURCE_MEM,
	},

	[1] = {
		.start = IRQ_VE,
		.end   = IRQ_VE,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device rtk_device_vpu = {
	.name = "rtk_vpu",
	.id = -1,
	.num_resources = ARRAY_SIZE(rtk_vpu_resource),
	.resource = rtk_vpu_resource,
	.dev = {},
};

/* JPU */
static struct resource rtk_jpu_resource[] = {
	[0] = {
		.start = 0x18044000,
		.end   = 0x18044000 + 0x4000 - 1,
		.flags = IORESOURCE_MEM,
	},

	[1] = {
		.start = IRQ_VE,
		.end   = IRQ_VE,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device rtk_device_jpu = {
	.name = "rtk_jpu",
	.id = -1,
	.num_resources = ARRAY_SIZE(rtk_jpu_resource),
	.resource = rtk_jpu_resource,
	.dev = {},
};

/* V1 Status */
static struct resource rtk_v1_st_resource[] = {
	[0] = {
		.start = 0x18005000,
		.end   = 0x18006000,
		.flags = IORESOURCE_MEM,
	},

};

static struct platform_device rtk_device_v1_st = {
	.name = "rtk_v1",
	.id = -1,
	.num_resources = ARRAY_SIZE(rtk_v1_st_resource),
	.resource = rtk_v1_st_resource,
	.dev = {},
};

static struct platform_device *sw_pdevs[] __initdata = {
	&rtk_mcp,
/*	&rtk_rtc,*/
    &rtk_device_rbus,
	&rtk_device_puwrap,
	&rtk_device_jpu,
	&rtk_device_vpu,
	&rtk_device_v1_st,
};

void rtk_sw_pdev_init(void)
{
	platform_add_devices(sw_pdevs, ARRAY_SIZE(sw_pdevs));
}

