#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/signal.h>
#include <linux/types.h>

#include <asm/mach/pci.h>

#include "rtk_pcie.h"

#define	RBUS1_TIMEOUT(x)	((x & 0x7) << 2)
#define RBUS_TIMEOUT(x)		(x & 0x3)

#define ADDR_TO_DEVICE_NO(addr)         ((addr >> 19) & 0x1F)

void __iomem	*PCIE_CFG_BASE;
void __iomem	*PCIE_DEV_BASE;
void __iomem	*PCIE_CTRL_BASE;
void __iomem	*SB2_BASE;
int irq;

struct resource res[2];

unsigned int rtk_pci_ctrl_write(unsigned long addr, unsigned int val) {
	writel(val, addr + PCIE_CTRL_BASE);
	return 0;
}

unsigned int rtk_pci_ctrl_read(unsigned long addr) {
	unsigned int val = readl(addr + PCIE_CTRL_BASE);
	return val;
}

unsigned int rtk_pci_cfg_write(unsigned long addr, unsigned int val) {
	writel(val, addr + PCIE_CFG_BASE);
	return 0;
}

unsigned int rtk_pci_cfg_read(unsigned long addr) {
	unsigned int val = readl(addr + PCIE_CFG_BASE);
	return val;
}

unsigned int rtk_pci_sb2_write(unsigned long addr, unsigned int val) {
	writel(val, addr + SB2_BASE);
	return 0;
}

unsigned int rtk_pci_sb2_read(unsigned long addr) {
	unsigned int val = readl(addr + SB2_BASE);
	return val;
}

unsigned long _pci_bit_mask(unsigned char byte_mask) {

	int i;
	unsigned long mask = 0;

	for (i = 0; i < 4; i++) {
		if ((byte_mask >> i) & 0x1)
			mask |= (0xFF << (i << 3));
	}

	return mask;
}

unsigned long _pci_bit_shift(unsigned long addr) {

	return ((addr & 0x3) << 3);

}

unsigned long _pci_byte_mask(unsigned long addr, unsigned char size) {

	unsigned char offset = (addr & 0x03);

	switch (size) {
	case 0x01:
		return 0x1 << offset;
	case 0x02:
		if (offset <= 2)
			return 0x3 << offset;

		PCI_CFG_WARNING("compute config mask - data cross dword boundrary (addr=%p, length=2)\n", (void*) addr);

		break;

	case 0x03:
		if (offset <= 1)
			return 0x7 << offset;

		PCI_CFG_WARNING("compute config mask - data cross dword boundrary (addr=%p, length=3)\n", (void*) addr);
		break;

	case 0x04:
		if (offset == 0)
			return 0xF;

		PCI_CFG_WARNING("compute config mask - data cross dword boundrary (addr=%p, length=4)\n", (void*) addr);
		break;

	default:
		PCI_CFG_WARNING("compute config mask failed - size %d should between 1~4\n", size);
	}
	return 0;
}

static unsigned long _pci_address_conversion(struct pci_bus* bus, unsigned int devfn, int reg) {
	int busno = bus->number;
	int dev = PCI_SLOT(devfn);
	int func = PCI_FUNC(devfn);
	return (busno << 24) | (dev << 19) | (func << 16) | reg;
}

int _indirect_cfg_write(unsigned long addr, unsigned long data, unsigned char size) {

	unsigned long status;
	unsigned char mask;
	int try_count = 1000;

	if (ADDR_TO_DEVICE_NO(addr) != 0) // rtd1192 only supports one device per slot,
		return PCIBIOS_DEVICE_NOT_FOUND; // it's uses bit0 to select enable device's configuration space

	mask = _pci_byte_mask(addr, size);

	if (!mask)
		return PCIBIOS_SET_FAILED;

	data = (data << _pci_bit_shift(addr)) & _pci_bit_mask(mask);

	rtk_pci_ctrl_write(PCIE_INDIR_CTR, 0x12);
	rtk_pci_ctrl_write(PCIE_CFG_ST, CFG_ST_ERROR|CFG_ST_DONE);
	rtk_pci_ctrl_write(PCIE_CFG_ADDR, addr);
	rtk_pci_ctrl_write(PCIE_CFG_WDATA, data);

	if (size == 4)
		rtk_pci_ctrl_write(PCIE_CFG_EN, 0x1);
	else
		rtk_pci_ctrl_write(PCIE_CFG_EN, BYTE_CNT(mask) | BYTE_EN | WRRD_EN(1));

	rtk_pci_ctrl_write(PCIE_CFG_CT, GO_CT);

	do {
		udelay(50);
		status = rtk_pci_ctrl_read(PCIE_CFG_ST);
	} while (!(status & CFG_ST_DONE) && try_count--);

	if (try_count < 0) {
		PCI_CFG_WARNING("Write config data (%p) failed - timeout\n",
				(void*) addr);
		goto error_occur;
	}

	if (rtk_pci_ctrl_read(PCIE_CFG_ST) & CFG_ST_ERROR){
		status = rtk_pci_cfg_read(0x04);
		if (status & CFG_ST_DETEC_PAR_ERROR)
			PCI_CFG_WARNING("Write config data failed - PAR error detected\n");
		if (status & CFG_ST_SIGNAL_SYS_ERROR)
			PCI_CFG_WARNING("Write config data failed - system error\n");
		if (status & CFG_ST_REC_MASTER_ABORT)
			PCI_CFG_WARNING("Write config data failed - master abort\n");
		if (status & CFG_ST_REC_TARGET_ABORT)
			PCI_CFG_WARNING("Write config data failed - target abort\n");
		if (status & CFG_ST_SIG_TAR_ABORT)
			PCI_CFG_WARNING("Write config data failed - tar abort\n");

		rtk_pci_cfg_write(0x04, status);

		goto error_occur;
	}

	rtk_pci_ctrl_write(PCIE_CFG_ST, CFG_ST_ERROR|CFG_ST_DONE);

	return PCIBIOS_SUCCESSFUL;

error_occur:

	rtk_pci_ctrl_write(PCIE_CFG_ST, CFG_ST_ERROR|CFG_ST_DONE);

	return PCIBIOS_SET_FAILED;
}

int _indirect_cfg_read(unsigned long addr, unsigned long* pdata, unsigned char size) {

	unsigned long status;
	unsigned char mask;
	int try_count = 1000;

	if (ADDR_TO_DEVICE_NO(addr) != 0) // rtd1192 only supports one device per slot,
		return PCIBIOS_DEVICE_NOT_FOUND; // it's uses bit0 to select enable device's configuration space

	mask = _pci_byte_mask(addr, size);

	if (!mask)
		return PCIBIOS_SET_FAILED;

	rtk_pci_ctrl_write(PCIE_INDIR_CTR, 0x10);
	rtk_pci_ctrl_write(PCIE_CFG_ST, 0x3);
	rtk_pci_ctrl_write(PCIE_CFG_ADDR, (addr & ~0x3));
	rtk_pci_ctrl_write(PCIE_CFG_EN, BYTE_CNT(mask) | BYTE_EN | WRRD_EN(0));
	rtk_pci_ctrl_write(PCIE_CFG_CT, GO_CT);

	do {
		udelay(50);
		status = rtk_pci_ctrl_read(PCIE_CFG_ST);//status = GET_PCIE_CFG_ST();
	} while (!(status & CFG_ST_DONE) && try_count--);

	if (try_count < 0) {
		PCI_CFG_WARNING("Read config data (%p) failed - timeout\n", (void*) addr);
		goto error_occur;
	}

	if (rtk_pci_ctrl_read(PCIE_CFG_ST) & CFG_ST_ERROR) {
		status = rtk_pci_cfg_read(0x04);

		if (status & CFG_ST_DETEC_PAR_ERROR)
			PCI_CFG_WARNING("Read config data failed - PAR error detected\n");
		if (status & CFG_ST_SIGNAL_SYS_ERROR)
			PCI_CFG_WARNING("Read config data failed - system error\n");
		if (status & CFG_ST_REC_MASTER_ABORT)
			PCI_CFG_WARNING("Read config data failed - master abort\n");
		if (status & CFG_ST_REC_TARGET_ABORT)
			PCI_CFG_WARNING("Read config data failed - target abort\n");
		if (status & CFG_ST_SIG_TAR_ABORT)
			PCI_CFG_WARNING("Read config data failed - tar abort\n");

		rtk_pci_cfg_write(0x04, status); //clear status bits

		goto error_occur;
	}

	rtk_pci_ctrl_write(PCIE_CFG_ST, 0x3);

	*pdata = (rtk_pci_ctrl_read(PCIE_CFG_RDATA) & _pci_bit_mask(mask)) >> _pci_bit_shift(addr);

	return PCIBIOS_SUCCESSFUL;

error_occur:
	rtk_pci_ctrl_write(PCIE_CFG_ST,0x3);
	return PCIBIOS_SET_FAILED;
}

static int rtk_pcie_rd_conf(struct pci_bus* bus, unsigned int devfn, int reg, int size, u32* pval) {

	unsigned long address;

	int ret = PCIBIOS_DEVICE_NOT_FOUND;
	rtk_pci_ctrl_write(PCIE_SYS_CTR, (rtk_pci_ctrl_read(PCIE_SYS_CTR) | INDOR_CFG_EN) & ~TRANS_EN);

	if (bus->number == 0) {
		address = _pci_address_conversion(bus, devfn, reg);
		ret = _indirect_cfg_read(address, (unsigned long*) pval, size);
	}

	rtk_pci_ctrl_write(PCIE_SYS_CTR, (rtk_pci_ctrl_read(PCIE_SYS_CTR) & ~INDOR_CFG_EN) | TRANS_EN);
	return ret;
}

static int rtk_pcie_wr_conf(struct pci_bus* bus, unsigned int devfn, int reg, int size, u32 val) {

	unsigned long address;

	int ret = PCIBIOS_DEVICE_NOT_FOUND;
	rtk_pci_ctrl_write(PCIE_SYS_CTR, (rtk_pci_ctrl_read(PCIE_SYS_CTR) | INDOR_CFG_EN) & ~TRANS_EN);

	if (bus->number == 0) {
		address = _pci_address_conversion(bus, devfn, reg);
		ret = _indirect_cfg_write(address, val, size);
	}

	rtk_pci_ctrl_write(PCIE_SYS_CTR, (rtk_pci_ctrl_read(PCIE_SYS_CTR) & ~INDOR_CFG_EN) | TRANS_EN);
	return ret;
}

static struct pci_ops rtk_pcie_ops = {
	.read = rtk_pcie_rd_conf,
	.write = rtk_pcie_wr_conf,
};

static int __init rtk_pcie_setup(int nr, struct pci_sys_data *sys){

	res[0].start = 0x40000000UL;
	res[0].end = 0x40ffffffUL;
	res[0].name = "pci mem space";
	res[0].flags = IORESOURCE_MEM;

	if (request_resource(&iomem_resource, &res[0])) {
		PCI_CFG_WARNING("Failed to reserve iomem resource\n");
		return -1;
	}

	res[1].start = 0x00030000UL;
	res[1].end = 0x0003ffffUL;
	res[1].name = "pci io space";
	res[1].flags = IORESOURCE_IO;

	if (request_resource(&ioport_resource, &res[1])) {
		PCI_CFG_WARNING("Failed to reserve io resource\n");
		return -1;
	}

	pci_add_resource_offset(&sys->resources, &res[0], sys->mem_offset);
	pci_add_resource_offset(&sys->resources, &res[1], sys->mem_offset);

	return 1;
}

static int __init rtk_pcie_map_irq(const struct pci_dev *dev, u8 slot, u8 pin){
	return irq;
}

static struct hw_pci rtk_pci_hw __initdata = {
	.nr_controllers	= 1,
	.setup = rtk_pcie_setup,
	.map_irq = rtk_pcie_map_irq,
	.ops = &rtk_pcie_ops,
};

static int __init rtk_pcie_probe(struct platform_device *pdev){

	int ret = 0;

	PCIE_CFG_BASE = of_iomap(pdev->dev.of_node, 0);
	if (!PCIE_CFG_BASE) {

		dev_err(&pdev->dev, "pcie no mmio space\n");
		return -EINVAL;
	}

	PCIE_DEV_BASE = of_iomap(pdev->dev.of_node, 1);
	if (!PCIE_DEV_BASE) {
		dev_err(&pdev->dev, "pcie no mmio space\n");
		return -EINVAL;
	}

	PCIE_CTRL_BASE = of_iomap(pdev->dev.of_node, 3);
	if (!PCIE_CTRL_BASE) {
		dev_err(&pdev->dev, "pcie no mmio space\n");
		return -EINVAL;
	}

	SB2_BASE = of_iomap(pdev->dev.of_node, 4);
	if (!SB2_BASE) {
		dev_err(&pdev->dev, "pcie no mmio space\n");
		return -EINVAL;
	}

	irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no irq\n");
		return -EINVAL;
	}

	rtk_pci_sb2_write(0x010, RBUS1_TIMEOUT(0x7) | RBUS_TIMEOUT(0x3));

	rtk_pci_ctrl_write(0xCFC, 0x1804F000);
	rtk_pci_ctrl_write(0xD00, 0xFFFFF000);
	rtk_pci_ctrl_write(0xD04, 0x40004000);

	/*-------------------------------------------
	 * PCI-E RC configuration space initialization
	 *-------------------------------------------*/
	rtk_pci_cfg_write(0x04, 0x00000007);
	rtk_pci_cfg_write(0x78, 0x00100010);

	/*-------------------------------------------
	 * PCI-E RC Initialization
	 *-------------------------------------------*/
	rtk_pci_ctrl_write(PCIE_SYS_CTR, rtk_pci_ctrl_read(PCIE_SYS_CTR) | APP_LTSSM_EN | TRANS_EN);

	rtk_pci_ctrl_write(PCIE_SYS_CTR, rtk_pci_ctrl_read(PCIE_SYS_CTR) | APP_INIT_RST);
	udelay(100);
	rtk_pci_ctrl_write(PCIE_SYS_CTR, rtk_pci_ctrl_read(PCIE_SYS_CTR) & ~APP_INIT_RST);
	udelay(100);

	pci_common_init(&rtk_pci_hw);

	return ret;
}

static int __exit rtk_pcie_remove(struct platform_device *pdev){

	return 0;
}

static const struct of_device_id rtk_pcie_of_match[] = {
	{ .compatible = "Realtek,rtk119x-pcie", },
	{},
};
MODULE_DEVICE_TABLE(of, rtk_pcie_of_match);

static struct platform_driver rtk_pcie_driver = {
	.remove		= __exit_p(rtk_pcie_remove),
	.driver = {
		.name	= "realtek-pcie",
		.owner	= THIS_MODULE,
		.of_match_table = rtk_pcie_of_match,
	},
};

static int __init rtk_pcie_init(void){
	return platform_driver_probe(&rtk_pcie_driver, rtk_pcie_probe);
}

subsys_initcall(rtk_pcie_init);

MODULE_AUTHOR("James Tai <james.tai@realtek.com>");
MODULE_DESCRIPTION("Realtek PCIe host controller driver");
MODULE_LICENSE("GPL v2");
