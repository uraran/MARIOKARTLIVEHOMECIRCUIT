/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2015 by Wilma Wu <wilmawu@realtek.com>
 *
 * Time initialization.
 */
#include <common.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/platform_lib/board/gpio.h>
#include <rtk_i2c-lib.h>

DECLARE_GLOBAL_DATA_PTR;

const struct rtd1195_sysinfo sysinfo = {
    "Board: Realtek Neodymium Board\n"
};

/**
 * @brief checkboard
 *
 * @return 0
 */
int checkboard(void)
{
	printf(sysinfo.board_string);
    return 0;
}

/**
 * @brief board_init
 *
 * @return 0
 */
int board_init(void)
{
	gd->bd->bi_arch_number = MACH_TYPE_RTK_RTD1195;
	gd->bd->bi_boot_params = CONFIG_BOOT_PARAM_BASE; /* boot param addr */

	return 0;
}

/**
 * @brief dram_init_banksize
 *
 * @return 0
 */
void dram_init_banksize(void)
{
	// Bank 1
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = get_accessible_ddr_size(UNIT_BYTE);

#if (CONFIG_NR_DRAM_BANKS > 1)
	// Bank 2
	gd->bd->bi_dram[1].start = CONFIG_SYS_SDRAM_DCU2_BASE;
	gd->bd->bi_dram[1].size = CONFIG_SYS_RAM_DCU2_SIZE;
#endif

#if (CONFIG_NR_DRAM_BANKS > 2)
	// Bank 3
	#if defined(CONFIG_SYS_SDRAM_DCU_OPT_BASE) && defined(CONFIG_SYS_RAM_DCU_OPT_SIZE)
		gd->bd->bi_dram[2].start = CONFIG_SYS_SDRAM_DCU_OPT_BASE;
		gd->bd->bi_dram[2].size = CONFIG_SYS_RAM_DCU_OPT_SIZE;
	#endif
#endif

}

int board_eth_init(bd_t *bis)
{
	return 0;
}

#define FUEL_GAUGE_ADDRESS 0x36
#define SOC_SUB_ADDRESS    0x4 //State of charge
#define SOC_LIMIT          0x4

int board_power_control(void)
{
	unsigned char  SubAddr = SOC_SUB_ADDRESS;
	unsigned short nSubAddrByte = 1;
	unsigned short nDataByte = 1;
	unsigned char  soc_data = 0xff;
	int ret=0;

	I2CN_Init(0);
	
	//Read RT9428 one byteDevice address 7-bit 0x36 ;Subaddress 0x04
	ret = I2C_Read_EX(0,FUEL_GAUGE_ADDRESS,nSubAddrByte,&SubAddr, nDataByte, &soc_data, 0);
	
	I2CN_UnInit(0);
	//i2c
	if(soc_data < SOC_LIMIT && !ret) 
	{
		printf("Low battery =%x ;shutting down!!\n",soc_data);
		setGPIO(50,1); // power_off
		mdelay(1);
		setISOGPIO(20,1); //latch enable
		return -1;		
	}
	else
	{
		setGPIO(44,0); //  LCD_EN#
		setGPIO(45,1); // LCD_STB#
		setISOGPIO(17,1); //BL_ADJ (PWM0 for backlight adjust)				
	}
	
	printf("battery =%x \n",soc_data);
	
	return 0;
}

/**
 * @brief misc_init_r - Configure Panda board specific configurations
 * such as power configurations, ethernet initialization as phase2 of
 * boot sequence
 *
 * @return 0
 */
int misc_init_r(void)
{
	if(!board_power_control())
		RTK_power_saving();
	
	return 0;
}

/*
 * get_board_rev() - get board revision
 */
u32 get_board_rev(void)
{
	uint revision = 0;

	revision = (uint)simple_strtoul(CONFIG_VERSION, NULL, 16);

	return revision;
}

int dram_init(void)
{
	gd->ram_size = get_accessible_ddr_size(UNIT_BYTE);

	return 0;
}


