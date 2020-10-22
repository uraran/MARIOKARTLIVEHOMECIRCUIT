#include <linux/clocksource.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irqchip.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/memblock.h>
#include <linux/delay.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/system_misc.h>
#include <asm/system_info.h>

#include <mach/cpu.h>
#include <rbus/reg.h>
#include <mach/memory.h>

extern void realtek_1195_cpufreq_voltage_reset(void);
void rtk_sw_pdev_init(void);
extern struct smp_operations rtk119x_smp_ops;

unsigned long realtek_cpu_id;
static void __iomem *wdt_base;

static struct map_desc rtk119x_io_desc[] __initdata = {
	{
		.virtual	= (unsigned long) IOMEM(RBUS_BASE_VIRT),
		.pfn		= __phys_to_pfn(RBUS_BASE_PHYS),
		.length		= RBUS_BASE_SIZE,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= (unsigned long) SYSTEM_GIC_BASE_VIRT,
		.pfn		= __phys_to_pfn(SYSTEM_GIC_BASE_PHYS),
		.length		= 0x10000,
		.type		= MT_DEVICE,
	},
	{   /*
         * RPC ring buffer
         * 0xfc8000000 - 0x4000 = 0xfc7fc000
         */
		.virtual	= (unsigned long) IOMEM(RPC_RINGBUF_VIRT),
		.pfn		= __phys_to_pfn(RPC_RINGBUF_PHYS),
		.length		= RPC_RINGBUF_SIZE,
		.type		= MT_DEVICE,
	},
#if 0
	{   /*
         * FrameBuffer
         * 0xFE000000 - 0x01800000 = 0xfc8000000
         */
		.virtual	= (unsigned long) IOMEM(PLAT_FRAME_BUFFER_VIRT),
		.pfn		= __phys_to_pfn(PLAT_FRAME_BUFFER_BASE_PHYS),
		.length		= PLAT_FRAME_BUFFER_SIZE,
		.type		= MT_DEVICE,
	},
#endif
#if 0
    {
        .virtual    = (unsigned long) __va(VE_BASE_VIRT),
        .pfn        = __phys_to_pfn(VE_BASE_PHYS),
        .length     = VE_BASE_SIZE,
        .type       = MT_DEVICE,
    },
#endif
	{
		.virtual	= (unsigned long) IOMEM(RPC_COMM_VIRT),
		.pfn		= __phys_to_pfn(RPC_COMM_PHYS),
		.length		= RPC_COMM_SIZE,
		.type		= MT_DEVICE,
	},
        {
                .virtual        = (unsigned long) IOMEM(SPI_RBUS_VIRT),
                .pfn            = __phys_to_pfn(SPI_RBUS_PHYS),
                .length         = SPI_RBUS_SIZE,
                .type           = MT_DEVICE,
        }

};

void __init rtk119x_map_io(void)
{
	debug_ll_io_init();

	iotable_init(rtk119x_io_desc, ARRAY_SIZE(rtk119x_io_desc));
}

// ------------------------ COMMON ---------------------------------
#if 0
u32 g_mem_resv[][3] = {
	{SYS_CONFIG_BOOTCODE_MEMBASE, SYS_CONFIG_BOOTCODE_MEMSIZE},
	{SYS_CONFIG_DMEM_MEMBASE, SYS_CONFIG_DMEM_SIZE},
	{SYS_CONFIG_RBUS_MEMBASE, SYS_CONFIG_RBUS_SIZE},
#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
	{ION_CARVEOUT_MEM_BASE, ION_CARVEOUR_MEM_SIZE},		/* reserve for ION */
#endif
};

void __init rtk_reserve(void)
{
	u32	i = 0;

	pr_info("memory reserved(in bytes):\n");
	for (i=0; i<ARRAY_SIZE(g_mem_resv); i++) {
		if (0 != memblock_reserve(g_mem_resv[i][0], g_mem_resv[i][1])) {
			pr_err("%s err, line %d, base 0x%08x, size 0x%08x\n", __func__,
					__LINE__, g_mem_resv[i][0], g_mem_resv[i][1]);
		}
		else {
			pr_info("\t: 0x%08x, 0x%08x\n", g_mem_resv[i][0], g_mem_resv[i][1]);
		}
	}
#ifdef CONFIG_REALTEK_RPC
	memblock_remove(SYS_CONFIG_RPC_MEMBASE, SYS_CONFIG_RPC_SIZE);
#endif
}
#endif

static int __init rtk_memblock_remove(phys_addr_t base, phys_addr_t size)
{
    int ret = memblock_remove(base, size);

    if (ret)
        printk(KERN_ERR "Failed to remove memory (%ld bytes at 0x%08lx)\n",
                size, (unsigned long)base);
    else
        printk(KERN_INFO "remove memory (%ld bytes at 0x%08lx)\n",
                size, (unsigned long)base);

    return ret;
}

void __init rtk_reserve(void)
{
    rtk_memblock_remove(RBUS_BASE_PHYS, 0x00100000);
    rtk_memblock_remove(PLAT_NOR_BASE_PHYS, PLAT_NOR_SIZE);
    rtk_memblock_remove(PLAT_SECURE_BASE_PHYS, PLAT_SECURE_SIZE);
}

void rtk_bus_sync(void)
{
	writel_relaxed(0x1234, IOMEM(0xfe01a020));
	dmb();
}
EXPORT_SYMBOL(rtk_bus_sync);

void rtk_machine_restart(char mode, const char *cmd)
{
#if 0
#include <linux/syscore_ops.h>
    syscore_shutdown();
#endif

#ifdef CONFIG_ARM_REALTEK_CPUFREQ
    realtek_1195_cpufreq_voltage_reset();
#endif 

    #define WDT_CLR			4
#define WDT_CTL			0
#define WDT_OVERFLOW	0xC
#define WDT_NMI         8
    printk(" [%s] !!!!!!!!!!!!!!!!!!!\n",__func__);

	writel(BIT(0),   wdt_base + WDT_CLR);
	writel(0x800000, wdt_base + WDT_OVERFLOW);
    //writel(0x40000, wdt_base + WDT_NMI);
	writel(0xff,     wdt_base + WDT_CTL);
#if 1
    {
        while (1)
            mdelay(100);
    }
#else
    {
        int count = 500;
        while (count > 0) {
            mdelay(10);
            printk(" WDT_CTL : 0x%08x\n",readl(wdt_base + WDT_CTL));
            count--;
        }
        if (count <= 0)
            printk("TIMEOUT ERROR!!!\n");
    }
#endif
}

static struct of_device_id rtk_restart_ids[] = {
	{	.compatible = "Realtek,rtk119x-wdt",
		.data		= rtk_machine_restart },
	{}
};
static void rtk_setup_restart(void)
{
	const struct of_device_id *of_id;
	struct device_node *np;

	np = of_find_matching_node(NULL, rtk_restart_ids);
	if (WARN(!np, "Unable to setup watchdog restart"))
		return;

	wdt_base = of_iomap(np, 0);
	WARN(!wdt_base, "failed to map IO base for watchdog");

	of_id = of_match_node(rtk_restart_ids, np);
	WARN(!of_id, "restart not available");

	arm_pm_restart = of_id->data;
}
// ------------------------ ~COMMON ---------------------------------
unsigned int realtek_rev(void)
{
	return (system_rev>>16);
}

static void __init rtk119x_dt_init(void)
{
	system_rev = get_cpu_revision();
	realtek_cpu_id = get_cpu_id();

	rtk_setup_restart();

	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);

	rtk_sw_pdev_init();

	/* setting SCPU wrapper to force as buffable */
	writel(((readl(IOMEM(0xfe01d000)) & ~(0x3<<12)) | (0x1 <<12)), IOMEM(0xfe01d000));
	writel(0x1234, IOMEM(0xfe01a020));	/* latch */
}

static const char * const rtk119x_board_dt_compat[] = {
	"Realtek,rtd-119x",
	"Realtek,rtd-1192",
	NULL,
};

extern void rtk119x_timer_init(void);


#ifdef CONFIG_MACH_RTK1192
DT_MACHINE_START(RTK119X_DT, "unicorn")
	.init_machine	= rtk119x_dt_init,
	.reserve		= rtk_reserve,
	.map_io			= rtk119x_map_io,
	.init_time		= rtk119x_timer_init,
	.init_irq		= irqchip_init,
	.dt_compat		= rtk119x_board_dt_compat,
	.smp			= smp_ops(rtk119x_smp_ops),
MACHINE_END
#else
DT_MACHINE_START(RTK119X_DT, "phoenix")
	.init_machine	= rtk119x_dt_init,
	.reserve		= rtk_reserve,
	.map_io			= rtk119x_map_io,
	.init_time		= rtk119x_timer_init,
	.init_irq		= irqchip_init,
	.dt_compat		= rtk119x_board_dt_compat,
	.smp			= smp_ops(rtk119x_smp_ops),
MACHINE_END
#endif
