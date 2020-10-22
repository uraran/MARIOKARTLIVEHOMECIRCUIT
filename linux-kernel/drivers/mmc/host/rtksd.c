/*
 * Realtek MMC/SD/SDIO driver
 *
 * Authors:
 * Copyright (C) 2008-2009 Realtek Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/mbus.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/mmc/host.h>
#include <asm/unaligned.h>

#include <linux/sched.h>                //liao
#include <linux/wait.h>                 //liao
#include <linux/slab.h>                 //liao
#include <linux/semaphore.h>            //liao
#include <linux/mmc/card.h>             //liao
#include <linux/mmc/mmc.h>              //liao
#include <linux/mmc/sd.h>               //liao
#include <linux/mmc/sdio.h>               //liao
#include <linux/workqueue.h>            //liao
#include <linux/completion.h>           //liao
#include <rbus/reg_mmc.h>               //liao
#include <rbus/reg_iso.h>               //liao
#include <rbus/reg_sys.h>               //liao
#include "rtksd.h"                  //liao
#include "rtk_crsd_ops.h"                  //liao
#include "../mmc_debug.h"               //liao

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#ifdef CONFIG_REALTEK_PCBMGR
#include <mach/pcbMgr.h>
#ifdef CONFIG_REALTEK_GPIO
#include <mach/venus_gpio.h>
#define EMMC_SHOUTDOWN_PROTECT
#endif
#endif

#ifdef CONFIG_MMC_RTKSD_INTERRUPT
#define ENABLE_SD_INT_MODE
#endif

#if 0 // JINN enable debug message
	#define MMC_DEBUG_PRINT(fmt,args...)	printk(fmt,## args)
#else
	#define MMC_DEBUG_PRINT(fmt,args...)
#endif

#define DRIVER_NAME "rtksd"
#define BANNER      "Realtek SD Card Driver"
#define VERSION     "$Id: rtksd.c Phoenix 2015-10-08 10:30 $"

#ifdef MONI_MEM_TRASH
#define MAX_BLK_NUM 0x01
#endif

struct device_node *rtk119x_crsd_node = NULL;
static int maxfreq = RTKSD_CLOCKRATE_MAX;
static int nodma;
struct mmc_host * mmc_sd_host_local = NULL;
static u32 rtk_crsd_bus_wid = 0;
static u8  g_cmd[6];
static unsigned char* pRSP=NULL;
static unsigned char* pRSP_org=NULL;
static unsigned int  g_crinit=0;
static struct rw_semaphore cr_rw_sem;
static unsigned int  crsd_rca = 0;

static unsigned int MMCDetected = 0;

static unsigned int force_check_previous_xfer_done; // CMD25_USE_SD_AUTOWRITE2
static unsigned int sd_in_receive_data_state; // CMD25_WO_STOP_COMMAND
static unsigned int sd_current_blk_address; // CMD25_WO_STOP_COMMAND
//static unsigned int sd_previous_blk_section; // CMD25_WO_STOP_COMMAND
static struct timer_list rtksd_stop_cmd_timer; // CMD25_WO_STOP_COMMAND
static struct mmc_command *rtksd_mmc_cmd; // CMD25_WO_STOP_COMMAND
static struct rtk_crsd_host *rtksd_sdport; // CMD25_WO_STOP_COMMAND

#define MAX_PHASE		31
#define TUNING_CNT		3


static void rtk_crsd_request(struct mmc_host *host, struct mmc_request *mrq);
static void rtk_crsd_hw_reset(struct mmc_host *host);
static int rtk_crsd_get_ro(struct mmc_host *mmc);
static int rtk_crsd_get_cd(struct mmc_host *host);
static void rtk_crsd_set_ios(struct mmc_host *host, struct mmc_ios *ios);
static int rtk_crsd_switch_voltage(struct mmc_host *mmc, struct mmc_ios *ios);
static int rtk_crsd_execute_tuning(struct mmc_host *mmc, u32 opcode);
static void rtk_crsd_set_speed(struct rtk_crsd_host *sdport,u8 level);
static void rtk_crsd_speed(struct rtk_crsd_host *sdport,enum crsd_clock_speed sd_speed);
static void rtk_crsd_dump_reg(struct rtk_crsd_host *sdport);
static u8 rtk_crsd_get_rsp_type(struct mmc_command* cmd);

typedef void (*set_gpio_func_t)(u32 gpio_num,u8 dir,u8 level);

static const struct mmc_host_ops rtk_crsd_ops = {
    .request        = rtk_crsd_request,
    .get_ro         = rtk_crsd_get_ro,
    .get_cd         = rtk_crsd_get_cd,
    .set_ios        = rtk_crsd_set_ios,
    .hw_reset		= rtk_crsd_hw_reset,
    .start_signal_voltage_switch	= rtk_crsd_switch_voltage,
	.execute_tuning = rtk_crsd_execute_tuning,
};

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const int __size = size;				\
		const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int __off = 3 - ((start) / 32);			\
		const int __shft = (start) & 31;			\
		u32 __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if (__size + __shft > 32)				\
			__res |= resp[__off-1] << ((32 - __shft) % 32);	\
		__res & __mask;						\
	})

u32 rtk_crsd_swap_endian(u32 input)
{
        u32 output;
        output = (input & 0xff000000)>>24|
                         (input & 0x00ff0000)>>8|
                         (input & 0x0000ff00)<<8|
                         (input & 0x000000ff)<<24;
        return output;
}


static u8 crsd_power_status = 0;
static void rtk_crsd_card_power(struct rtk_crsd_host *sdport,u8 status)
{
	int res;
	u32 tmp;

	if(status == crsd_power_status)
		return;
	if(status) //power on
	{
		res = gpio_direction_output(sdport->gpio_card_power, 0);

		/* Workaround :
		 * 1. OS reboot SD card initial fail, jamestai20150129
		 * 2. Enable/disable SD card write protection, jamestai20150615
		 */
		tmp = crsd_readl(sdport->base_pll_io + CR_PFUNC_CR );
		crsd_writel(0x33333223 | (tmp & 0x00000F00), sdport->base_pll_io + CR_PFUNC_CR );

		if (res < 0) {
			printk(KERN_ERR "can't gpio output crsd card power on %d\n", sdport->gpio_card_power);
		}else{
			crsd_power_status = 1; //card is power on
			mdelay(10); //delay 10 ms after power on
		}
	}
	else //power off
	{
		res = gpio_direction_input(sdport->gpio_card_power);

		/* Workaround :
		 * 1. OS reboot SD card initial fail, jamestai20150129
		 * 2. Enable/disable SD card write protection, jamestai20150615
		 */
		tmp = crsd_readl(sdport->base_pll_io + CR_PFUNC_CR );
		crsd_writel(0x22223222 | (tmp & 0x00000F00), sdport->base_pll_io + CR_PFUNC_CR );

		if (res < 0) {
			printk(KERN_ERR "can't gpio input crsd card power off %d\n", sdport->gpio_card_power);
		}else{
			crsd_power_status = 0; //card is power off
			mdelay(10); //delay 10 ms after power off
		}
	}
}

static void rtk_crsd_read_rsp(struct rtk_crsd_host *sdport,u32 *rsp, int reg_count)
{
    u32 iobase = sdport->base_io;


    if ( reg_count==6 ){
        rsp[0] = crsd_readb(iobase+SD_CMD1) << 24 |
                 crsd_readb(iobase+SD_CMD2) << 16 |
                 crsd_readb(iobase+SD_CMD3) << 8 |
                 crsd_readb(iobase+SD_CMD4);
    }else if(reg_count==16){
        rsp[0] = rtk_crsd_swap_endian(rsp[0]);
        rsp[1] = rtk_crsd_swap_endian(rsp[1]);
        rsp[2] = rtk_crsd_swap_endian(rsp[2]);
        rsp[3] = rtk_crsd_swap_endian(rsp[3]);
    }

    RTKSDPRINTF( "*rsp[0]=0x%08x; reg_count=%u\n", rsp[0], reg_count);

}

static void rtk_crsd_reset(struct rtk_crsd_host *sdport)
{
    crsd_writel(0x00, sdport->base_io + CR_DMA_CTL3);
    crsd_writeb(0x00, sdport->base_io + SD_TRANSFER);
    crsd_writeb(0xff, sdport->base_io + CR_CARD_STOP);    // SD Card module transfer stop and idle state.
    crsd_writeb(0x00, sdport->base_io + CR_CARD_STOP);    // SD Card module transfer start.
}

static void rtk_crsd_hw_reset(struct mmc_host *host)
{

    struct rtk_crsd_host *sdport;
    u32 iobase;
//  u32 tmp_clock;
//  unsigned long flags;

    sdport = mmc_priv(host);
    iobase = sdport->base_io;
// jinn printk(KERN_ERR "[%s][%04d][base_io 0x%08x]\n",__FUNCTION__, __LINE__, iobase);
    RedSDPRINTF( "regCARD_EXIST = %x\n",crsd_readb(iobase+CARD_EXIST));
    
    force_check_previous_xfer_done = 0; // CMD25_USE_SD_AUTOWRITE2
    sd_in_receive_data_state = 0; // CMD25_WO_STOP_COMMAND
    sd_current_blk_address = 0; // CMD25_WO_STOP_COMMAND
//    sd_previous_blk_section = 0; // CMD25_WO_STOP_COMMAND
    rtksd_mmc_cmd = NULL; // CMD25_WO_STOP_COMMAND
    rtksd_sdport = sdport; // CMD25_WO_STOP_COMMAND

////////////////////////////////////////////////////////////////////
//  sdport->ins_event = EVENT_REMOV;
//  sdport->rtflags &= ~RTKCR_FCARD_DETECTED;
//  rtkcr_set_clk_muxpad(sdport,OFF);
//	det_time = 1;
	crsd_writeb( 0xff, sdport->base_io+CR_CARD_STOP );     	// SD Card module transfer stop and idle state
//  if(sdport->mmc->card)
//		mmc_card_set_removed(sdport->mmc->card);

	sdport->ops->card_power(sdport, 0);	//power off
	sdport->ops->card_power(sdport, 1);	//power on

//  sdport->ins_event = EVENT_INSER;
//  sdport->rtflags |= RTKCR_FCARD_DETECTED;
//	det_time = 1;


    down_write(&cr_rw_sem);
	sdport->rtflags &= ~RTKCR_FCARD_SELECTED;
	crsd_rca = 0;
//	spin_lock_irqsave(&sdport->lock,flags);
	crsd_writeb( 0x0, sdport->base_io+CR_CARD_STOP );            // SD Card module transfer no stop
    crsd_writeb( 0x2, sdport->base_io+CARD_SELECT );            	//Specify the current active card module for the coming data transfer, bit 2:0 = 010
    crsd_writeb( 0x4, sdport->base_io+CR_CARD_OE );            	//arget module is SD/MMC card module, bit 2 =1
//	crsd_writeb( 0x4, sdport->base_io+CR_IP_CARD_INT_EN );     	//SD/MMC Interrupt Enable, bit 2 = 1
    crsd_writeb( 0x4, sdport->base_io+CARD_CLOCK_EN_CTL );     	// SD Card Module Clock Enable, bit 2 = 1

    crsd_writeb( 0xD0, sdport->base_io+SD_CONFIGURE1 );
	rtk_crsd_speed(sdport, CRSD_CLOCK_400KHZ);

	//# SD CMD Response Timeout Error disable	//#x /b 0x18010584
    crsd_writeb( 0x0, sdport->base_io+SD_STATUS2 );

	crsd_writel( 0x003E0003,  sdport->base_pll_io+CR_PLL_SD1);	//PLL_SD1
	udelay(100);
	crsd_writel( 0x078D1893,  sdport->base_pll_io+CR_PLL_SD2);	//Reduce the impact of spectrum by Hsin-yin, jamestai20150302
	udelay(100);
	crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) | 0x00070000, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to 4MHz by Hsin-yin
	udelay(100);
	crsd_writel( 0x00564388,  sdport->base_pll_io+CR_PLL_SD3);	//PLL_SD3
	udelay(100);
	crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) & 0xFFF8FFFF, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to normal clock source by Hsin-yin
	udelay(100);
	crsd_writeb( crsd_readb(sdport->base_io+SD_CONFIGURE1) & 0xEF, sdport->base_io+SD_CONFIGURE1 ); //Reset FIFO pointer by Hsin-yin
	crsd_writel( 0x00000007,  sdport->base_pll_io+CR_PLL_SD4);	//PLL_SD4
	udelay(100);
	crsd_writeb(0x0, iobase+CARD_SD_CLK_PAD_DRIVE);
	crsd_writeb(0x0, iobase+CARD_SD_CMD_PAD_DRIVE);
	crsd_writeb(0x0, iobase+CARD_SD_DAT_PAD_DRIVE);
	crsd_writel(0x0, sdport->base_io+CR_SD_PAD_CTL);	//change to 3.3v

	MMCDetected = 0;
//	spin_unlock_irqrestore(&sdport->lock, flags);
    rtk_crsd_sync(sdport);
    up_write(&cr_rw_sem);

	mdelay(10);

#ifdef ENABLE_SD_INT_MODE
	writel(0x16 ,sdport->base_crsd + 0x24);
	writel(0x17 ,sdport->base_crsd + 0x28);
#endif

}

static void rtk_crsd_set_div(struct rtk_crsd_host *sdport,u32 set_div)
{
    u32 iobase = sdport->base_io;
    u32 tmp_div;
    unsigned long flags;
    spin_lock_irqsave(&sdport->lock,flags);
    tmp_div = crsd_readb(iobase+SD_CONFIGURE1) & ~MASK_CLOCK_DIV;
    if (set_div != CLOCK_DIV_NON)
    {
        crsd_writeb(tmp_div|set_div|SDCLK_DIV, iobase+SD_CONFIGURE1);
    }
    else
        crsd_writeb(tmp_div, iobase+SD_CONFIGURE1);
    rtk_crsd_sync(sdport);
    spin_unlock_irqrestore(&sdport->lock, flags);
    RTKSDPRINTF( "%s: set div to 0x%02x, 0x%x=%08x\n",
              DRIVER_NAME, tmp_div|set_div,iobase+SD_CONFIGURE1, crsd_readb(iobase+SD_CONFIGURE1));
}

static void rtk_crsd_set_bits(struct rtk_crsd_host *sdport,u8 set_bit)
{
    u32 iobase = sdport->base_io;
    u32 tmp_bits;
    unsigned long flags;

    spin_lock_irqsave(&sdport->lock,flags);
    tmp_bits = crsd_readb(iobase+SD_CONFIGURE1);
    if((tmp_bits & MASK_BUS_WIDTH) != set_bit ){
        tmp_bits &= ~MASK_BUS_WIDTH;
        crsd_writeb(tmp_bits|set_bit,iobase+SD_CONFIGURE1);
        spin_unlock_irqrestore(&sdport->lock, flags);
        RTKSDPRINTF( "%s: change to %s mode, 0x%x =%08x\n",
                DRIVER_NAME,bit_tlb[set_bit],iobase+SD_CONFIGURE1,crsd_readb(iobase+SD_CONFIGURE1));
    }
    else
    {
        RTKSDPRINTF( "%s: already in  %s mode\n", DRIVER_NAME,bit_tlb[set_bit]);
        spin_unlock_irqrestore(&sdport->lock, flags);
	}
}

static void rtk_crsd_set_access_mode(struct rtk_crsd_host *sdport,u8 level)
{
    u32 iobase = sdport->base_io;
    u32 tmp_bits;
    unsigned long flags;

    RTKSDPRINTF( "\n");
    spin_lock_irqsave(&sdport->lock,flags);
    tmp_bits = crsd_readb(iobase+SD_CONFIGURE1) & ~MODE_SEL_MASK;

    if(level == ACCESS_MODE_SD20)
	{
        tmp_bits |= MODE_SEL_SD20;
    }
	else if(level == ACCESS_MODE_DDR)
	{
        tmp_bits |= MODE_SEL_DDR;
	}
	else if(level == ACCESS_MODE_SD30)
	{
        tmp_bits |= (MODE_SEL_SD30 | SD30_ASYNC_FIFO_RST);
    }
	else
	{
        tmp_bits |= MODE_SEL_SD20;
    }

    crsd_writeb(tmp_bits,iobase+SD_CONFIGURE1);
    spin_unlock_irqrestore(&sdport->lock, flags);
}



static void rtk_crsd_speed(struct rtk_crsd_host *sdport,enum crsd_clock_speed sd_speed)
{
    switch(sd_speed){
        case CRSD_CLOCK_200KHZ:  
		    RTKSDPRINTF( "speed CRSD_CLOCK_200KHZ\n");
            rtk_crsd_set_div(sdport,CLOCK_DIV_256);	//0x580 = 0xd0
            rtk_crsd_set_speed(sdport, 1);			//0x478 = 0x2101
            break;
        case CRSD_CLOCK_400KHZ:  
		    RTKSDPRINTF( "speed CRSD_CLOCK_400KHZ\n");
            rtk_crsd_set_div(sdport,CLOCK_DIV_256);	//0x580 = 0xd0
            rtk_crsd_set_speed(sdport, 0);			//0x478 = 0x2100
            break;
        case CRSD_CLOCK_6200KHZ:
		    RTKSDPRINTF( "speed CRSD_CLOCK_6200KHZ\n");
	        rtk_crsd_set_div(sdport,CLOCK_DIV_NON);	//0x580 = 0x10
			rtk_crsd_set_speed(sdport, 3);			//0x478 = 0x2103
            break;
        case CRSD_CLOCK_20000KHZ:
			RTKSDPRINTF( "speed CRSD_CLOCK_20000KHZ\n");
			crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) | 0x00070000, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to 4MHz by Hsin-yin
			crsd_writel( 0x00204388,  sdport->base_pll_io+CR_PLL_SD3);
			udelay(2);
			crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) & 0xFFF8FFFF, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to normal clock source by Hsin-yin
			crsd_writeb( crsd_readb(sdport->base_io+SD_CONFIGURE1) & 0xEF, sdport->base_io+SD_CONFIGURE1 ); //Reset FIFO pointer by Hsin-yin

			rtk_crsd_set_div(sdport,CLOCK_DIV_NON); //0x580 = 0x10
			rtk_crsd_set_speed(sdport, 0);			//0x478 = 0x2100
            break;
        case CRSD_CLOCK_25000KHZ:
		    RTKSDPRINTF( "speed CRSD_CLOCK_25000KHZ\n");
	        rtk_crsd_set_div(sdport,CLOCK_DIV_NON); //0x580 = 0x10
			rtk_crsd_set_speed(sdport, 1);			//0x478 = 0x2101
            break;
        case CRSD_CLOCK_50000KHZ:
		    RTKSDPRINTF( "speed CRSD_CLOCK_50000KHZ\n");
		    if(sdport->mmc->ios.timing == MMC_TIMING_UHS_DDR50){
				crsd_writeb(0X3F, sdport->base_io+CARD_SD_CLK_PAD_DRIVE);
				crsd_writeb(0X3F, sdport->base_io+CARD_SD_CMD_PAD_DRIVE);
				crsd_writeb(0X3F, sdport->base_io+CARD_SD_DAT_PAD_DRIVE);

				crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) | 0x00070000, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to 4MHz by Hsin-yin
				crsd_writel( 0x00324388,  sdport->base_pll_io+CR_PLL_SD3);
				udelay(2);
				crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) & 0xFFF8FFFF, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to normal clock source by Hsin-yin
				crsd_writeb( crsd_readb(sdport->base_io+SD_CONFIGURE1) & 0xEF, sdport->base_io+SD_CONFIGURE1 ); //Reset FIFO pointer by Hsin-yin
		    }
	        rtk_crsd_set_div(sdport,CLOCK_DIV_NON); //0x580 = 0x10
			rtk_crsd_set_speed(sdport, 0);			//0x478 = 0x2100
            break;
        case CRSD_CLOCK_100000KHZ:
			RTKSDPRINTF( "speed CRSD_CLOCK_100000KHZ\n");
			crsd_writeb(0X3F, sdport->base_io+CARD_SD_CLK_PAD_DRIVE);
			crsd_writeb(0X3F, sdport->base_io+CARD_SD_CMD_PAD_DRIVE);
			crsd_writeb(0X3F, sdport->base_io+CARD_SD_DAT_PAD_DRIVE);

			rtk_crsd_set_div(sdport,CLOCK_DIV_NON); //0x580 = 0x10
			rtk_crsd_set_speed(sdport, 0);			//0x478 = 0x2100
			break;
        case CRSD_CLOCK_208000KHZ:
		    RTKSDPRINTF( "speed CRSD_CLOCK_208000KHZ\n");

			/*
			 * SD clock rate formula:
			 * Target clock = (ssc_div_n +3) *4.5/4, For example: (172+3)*4.5/4 = 196MHz
			 * jamestai20141222
			 */
        	if(sdport->magic_num == 0x0 ||
        	   sdport->magic_num == 0x1){

        		crsd_writeb(0X3F, sdport->base_io+CARD_SD_CLK_PAD_DRIVE);
				crsd_writeb(0X3F, sdport->base_io+CARD_SD_CMD_PAD_DRIVE);
				crsd_writeb(0X3F, sdport->base_io+CARD_SD_DAT_PAD_DRIVE);

				rtk_crsd_set_div(sdport,CLOCK_DIV_NON); //0x580 = 0x10
				rtk_crsd_set_speed(sdport, 0);			//0x478 = 0x2100

        	}else{

        		crsd_writeb(0XEE, sdport->base_io+CARD_SD_CLK_PAD_DRIVE);
        		crsd_writeb(0XEE, sdport->base_io+CARD_SD_CMD_PAD_DRIVE);
        		crsd_writeb(0XEE, sdport->base_io+CARD_SD_DAT_PAD_DRIVE);

				crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) | 0x00070000, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to 4MHz by Hsin-yin
				crsd_writel( 0x00b64388,  sdport->base_pll_io+CR_PLL_SD3);	//bus clock to 208MHz jamestai20150319
        	}

			udelay(2);
			crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) & 0xFFF8FFFF, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to normal clock source by Hsin-yin
			crsd_writeb( crsd_readb(sdport->base_io+SD_CONFIGURE1) & 0xEF, sdport->base_io+SD_CONFIGURE1 ); //Reset FIFO pointer by Hsin-yin

			rtk_crsd_set_div(sdport,CLOCK_DIV_NON); //0x580 = 0x10
			rtk_crsd_set_speed(sdport, 0);			//0x478 = 0x2100
            break;
        default :
		    RTKSDPRINTF( "default speed CRSD_CLOCK_400KHZ\n");
            rtk_crsd_set_div(sdport,CLOCK_DIV_256); //0x580 = 0xd0
            rtk_crsd_set_speed(sdport, 0);			//0x478 = 0x2100
            break;
    }
    rtk_crsd_sync(sdport);
}


static void rtk_crsd_set_speed(struct rtk_crsd_host *sdport,u8 level)
{
    u32 iobase = sdport->base_io;
//  u32 tmp_para;
    unsigned long flags;


    spin_lock_irqsave(&sdport->lock,flags);
    switch(level)
    {
        case 0:  //ddr50 , highest speed
		    RTKSDPRINTF( "speed 2100\n");
            crsd_writel(0x2100,iobase+CR_SD_CKGEN_CTL);
            break;
        case 1:
		    RTKSDPRINTF( "speed 2101\n");
            crsd_writel(0x2101,iobase+CR_SD_CKGEN_CTL);
            break;
        case 2:
		    RTKSDPRINTF( "speed 2102\n");
            crsd_writel(0x2102,iobase+CR_SD_CKGEN_CTL);
            break;
        case 3:
		    RTKSDPRINTF( "speed 2103\n");
            crsd_writel(0x2103,iobase+CR_SD_CKGEN_CTL);
            break;

        default :
		    RTKSDPRINTF( "default speed 2102\n");
            crsd_writel(0x2102,iobase+CR_SD_CKGEN_CTL);
            break;
    }
    rtk_crsd_sync(sdport);
    spin_unlock_irqrestore(&sdport->lock, flags);
}


static u8 rtk_crsd_get_rsp_len(u8 rsp_para)
{
    RTKSDPRINTF( "\n");
    switch (rsp_para & 0x3) {
    case 0:
        return 0;
    case 1:
        return 6;
    case 2:
        return 16;
    default:
        return 0;
    }
}

static u32 rtk_crsd_get_cmd_timeout(struct crsd_cmd_pkt *cmd_info)
{
    struct rtk_crsd_host *sdport   = cmd_info->sdport;
//  u16 block_count             = cmd_info->block_count;
    u32 tmout = 0;

    RTKSDPRINTF( "\n");
/*
   if(cmd_info->cmd->data)
    {
        tmout = msecs_to_jiffies(200);
        if(block_count>0x100)
        {
            tmout = tmout + msecs_to_jiffies(block_count>>1);
        }
    }
    else
        tmout = msecs_to_jiffies(80);

#ifdef CONFIG_ANDROID
    tmout += msecs_to_jiffies(100);
#endif
*/
    tmout += msecs_to_jiffies(1);
    cmd_info->timeout = sdport->tmout = tmout;
    return 0;
}

#define SD_ALLOC_LENGTH     2048
static int rtk_crsd_allocate_dma_buf(struct rtk_crsd_host *sdport, struct mmc_command *cmd)
{
//  extern unsigned char* pPSP;
    if (!pRSP)
        pRSP_org = pRSP = dma_alloc_coherent(sdport->dev, SD_ALLOC_LENGTH, &sdport->paddr ,GFP_KERNEL);
    else
        return 0;

    if(!pRSP){
        WARN_ON(1);
	    printk(KERN_ERR "allocate rtk sd dma buf FAIL!!!!!!!!!!!!!!!!!!!\n");
        cmd->error = -ENOMEM;
        return 0;
    }

    RTKSDPRINTF( "allocate rtk dma buf : dma addr=0x%08x, phy addr=0x%08x\n", (unsigned int)pRSP, (unsigned int)sdport->paddr);
    return 1;
}
static int rtk_crsd_free_dma_buf(struct rtk_crsd_host *sdport)
{
    if (pRSP_org)
        dma_free_coherent(sdport->dev, SD_ALLOC_LENGTH, pRSP_org ,sdport->paddr);
    else
        return 0;

    RTKSDPRINTF( "free rtk dma buf \n");
    return 1;
}

static void rtk_crsd_get_next_block(void)
{
    RTKSDPRINTF( "\n");
    if (pRSP_org)
    {
        if ((pRSP+0x200) >= (pRSP_org+SD_ALLOC_LENGTH))
            pRSP = pRSP_org;
        else
            pRSP+=0x200;
    }
}

static int rtk_crsd_set_rspparam(struct crsd_cmd_pkt *cmd_info)
{
    //correct emmc setting for rt1195

    switch(cmd_info->cmd->opcode)
    {
        case MMC_GO_IDLE_STATE:			//cmd 0	/* bc                          */
		 	RTKSDPRINTF_WARM("is cmd CMD0 MMC_GO_IDLE_STATE\n");
            cmd_info->rsp_para1 = 0xD0;			//SD1_R0;
            cmd_info->rsp_para2 = 0x74;			//0x50|SD_R0;
            cmd_info->rsp_para3 = 0;
            break;

        case MMC_SEND_OP_COND:
        	if(MMCDetected == 1){
        		cmd_info->rsp_para1 = SD1_R0;
        		cmd_info->rsp_para2 = 0x45;
        		cmd_info->rsp_para3 = 0x1;
        		cmd_info->cmd->arg = MMC_VDD_30_31|MMC_VDD_31_32|MMC_VDD_32_33|MMC_VDD_33_34|MMC_SECTOR_ADDR;
        	}
        	break;

        case MMC_ALL_SEND_CID:			//cmd 2	/* bcr                     R2  */
		 	RTKSDPRINTF_WARM("is cmd CMD2 MMC_ALL_SEND_CID\n");
            cmd_info->rsp_para1 = -1;//SD1_R0;
            cmd_info->rsp_para2 = 0x42;//SD_R2;
            cmd_info->rsp_para3 = 0x5;//SD2_R0;
            break;

//		case SD_SEND_RELATIVE_ADDR:		//cmd 3	/* bcr        R6  */
        case MMC_SET_RELATIVE_ADDR:		//cmd 3	/* ac   [31:16] RCA        R1  */
		 	RTKSDPRINTF_WARM("is cmd CMD3 MMC_SET_RELATIVE_ADDR\n");
            cmd_info->rsp_para1 = -1;//SD1_R0;
            cmd_info->rsp_para2 = 0x41;//SD_R1|CRC16_CAL_DIS;
            cmd_info->rsp_para3 = 0x5;//;
            //cmd_info->cmd->arg = 0x10000;
            break;

		case MMC_SET_DSR:				//cmd 4 /* bc [31:16] RCA */
		 	RTKSDPRINTF_WARM("is cmd CMD4 MMC_SET_DSR \n");
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
            break;

///		case SD_SWITCH:					//cmd 6	/* adtc [31:0] See below   R1  */
//      case MMC_SWITCH:				//cmd 6	/* ac   [31:0] See below   R1b */
		case SD_APP_SET_BUS_WIDTH:     	//cmd 6	/* ac   [1:0] bus width    R1  */
		 	RTKSDPRINTF_WARM("cmd->flags = %x \n",cmd_info->cmd->flags);

			if((cmd_info->cmd->flags & (0x3<<5)) == MMC_CMD_ADTC)	//SD_SWITCH
			{
			 	RTKSDPRINTF_WARM("is cmd CMD6 SD_SWITCH\n");
	            cmd_info->rsp_para1 = -1;
	            cmd_info->rsp_para2 = 0x1;//SD_R1b|CRC16_CAL_DIS;
	            cmd_info->rsp_para3 = 0x5;//-1;
			}else{
			 	RTKSDPRINTF_WARM("is cmd ACMD6 SD_APP_SET_BUS_WIDTH\n");
	            cmd_info->rsp_para1 = -1;
	            if(MMCDetected == 1){
	            	cmd_info->rsp_para2 = 0x09;//SD_R1b|CRC16_CAL_DIS;
	            	cmd_info->rsp_para3 = 0x05;//-1;
	            }else{
	            	cmd_info->rsp_para2 = 0x61;//SD_R1b|CRC16_CAL_DIS;
	            	cmd_info->rsp_para3 = 0xd;//-1;
	            }
			}
            break;

        case MMC_SELECT_CARD:			//cmd 7	/* ac   [31:16] RCA        R1  */
		 	RTKSDPRINTF_WARM("is cmd CMD7 MMC_SELECT_CARD\n");
            cmd_info->rsp_para1 = -1;//0;
            cmd_info->rsp_para2 = 0x41;//SD_R1b|CRC16_CAL_DIS;
            cmd_info->rsp_para3 = 0x5;//-1;
//          cmd_info->cmd->arg = 0x10000;
            break;

//		case MMC_SEND_EXT_CSD:			//cmd 8 // SD_SEND_IF_COND
		case SD_SEND_IF_COND:			//cmd 8   /* bcr  [11:0] See below   R7  */
		 	RTKSDPRINTF_WARM("is cmd CMD8 SD_SEND_IF_COND\n");
            cmd_info->rsp_para1 = 0xD0;			//-1;
            cmd_info->rsp_para2 = 0x41;			//SD_R1;
            cmd_info->rsp_para3 = 0x0;			//-1;
            if (cmd_info->sdport->rtflags & RTKCR_FCARD_SELECTED && MMCDetected == 1) {
            	cmd_info->rsp_para1 = 0x50;
            	cmd_info->rsp_para2 = SD_R1;
            	cmd_info->rsp_para3 = 0x05;
            }
            break;

        case MMC_SEND_CSD:				//cmd 9	/* ac   [31:16] RCA        R2  */
		 	RTKSDPRINTF_WARM("is cmd CMD9 MMC_SEND_CSD\n");
            cmd_info->rsp_para1 = -1;//0;
            cmd_info->rsp_para2 = 0x42;//SD_R2;
            cmd_info->rsp_para3 = 0x5;//0x0;
//          cmd_info->cmd->arg = 0x10000;
            break;

        case MMC_SEND_CID:				//cmd 10	/* ac   [31:16] RCA        R2  */
		 	RTKSDPRINTF_WARM("is cmd CMD10 MMC_SEND_CID\n");
            cmd_info->rsp_para1 = -1;//0;
            cmd_info->rsp_para2 = 0x42;//SD_R2;
            cmd_info->rsp_para3 = 0x5;//0x0;
//          cmd_info->cmd->arg = 0x10000;
            break;



//		case MMC_READ_DAT_UNTIL_STOP:	//cmd 11 /* adtc [31:0] dadr R1 */
		case SD_SWITCH_VOLTAGE:			//cmd 11  /* ac                      R1  */
		 	RTKSDPRINTF_WARM("is cmd CMD11 SD_SWITCH_VOLTAGE\n");
            cmd_info->rsp_para1 = -1;//0;
            cmd_info->rsp_para2 = 0x41;     //don't update
            cmd_info->rsp_para3 = 0x0;     //don't update
            break;


        case MMC_STOP_TRANSMISSION:  	//cmd 12	/* ac                      R1b */
		 	RTKSDPRINTF_WARM("is cmd CMD12 MMC_STOP_TRANSMISSION\n");
            cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x7c;// SD_R1|CRC16_CAL_DIS;
            cmd_info->rsp_para3 = 0x0;//-1;
            break;

		case SD_APP_SD_STATUS:         	//cmd 13   	/* adtc                    R1  */
//      case MMC_SEND_STATUS: 			//cmd 13	/* ac   [31:16] RCA        R1  */
			if((cmd_info->cmd->flags & (0x3<<5)) == MMC_CMD_ADTC)	//SD_APP_SD_STATUS
			{
			 	RTKSDPRINTF_WARM("is cmd ACMD13 SD_APP_SD_STATUS\n");
	            cmd_info->rsp_para1 = -1;
	            cmd_info->rsp_para2 = 0x1;//SD_R1|CRC16_CAL_DIS;
	            cmd_info->rsp_para3 = 0x5;
//				cmd_info->cmd->arg = 0x10000;
			}else{
			 	RTKSDPRINTF_WARM("is cmd CMD13 MMC_SEND_STATUS\n");
	            cmd_info->rsp_para1 = -1;
	            cmd_info->rsp_para2 = 0x41;//SD_R1|CRC16_CAL_DIS;
	            cmd_info->rsp_para3 = -1;
//	            cmd_info->rsp_para3 = 0;
//				cmd_info->cmd->arg = 0x10000;
			}
            break;

		case MMC_GO_INACTIVE_STATE:		//cmd 15 /* ac [31:16] RCA */
		 	RTKSDPRINTF_WARM("is cmd CMD15 MMC_GO_INACTIVE_STATE\n");
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
            break;

		case MMC_SET_BLOCKLEN:			//cmd 16 /* ac [31:0] block len R1 */
		 	RTKSDPRINTF_WARM("is cmd CMD16 MMC_SET_BLOCKLEN\n");
            cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x41;
            cmd_info->rsp_para3 = 0x5;
//			cmd_info->cmd->arg = 0x10000;
            break;


		case MMC_READ_SINGLE_BLOCK:		//cmd 17 /* adtc [31:0] data addr R1 */
		 	RTKSDPRINTF_WARM("is cmd CMD17 MMC_READ_SINGLE_BLOCK\n");
            cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x1;
            cmd_info->rsp_para3 = -1;
            break;


        case MMC_READ_MULTIPLE_BLOCK:	//cmd 18	/* adtc [31:0] data addr   R1  */
		 	RTKSDPRINTF_WARM("is cmd CMD18 MMC_READ_MULTIPLE_BLOCK\n");
            cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x1;
            if(MMCDetected == 1){
            	cmd_info->rsp_para3 = 0x4;//0x04;
            }else{
            	cmd_info->rsp_para3 = 0x2;
            }
            break;


//		case MMC_BUS_TEST_W:			//cmd 19 /* adtc R1 */
		case MMC_SEND_TUNING_BLOCK:		//cmd 19 /* adtc R1 */
		 	RTKSDPRINTF_WARM("is cmd CMD19 MMC_SEND_TUNING_BLOCK\n");
            cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x1;
            cmd_info->rsp_para3 = -1;//0x04;
            break;

		

		case MMC_WRITE_DAT_UNTIL_STOP:	//cmd 20 /* adtc [31:0] data addr R1 */
		 	RTKSDPRINTF_WARM("is cmd CMD20 SPEED_CLASS_CONTROL \n");
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
            break;

//host not support ACMD22
		case SD_APP_SEND_NUM_WR_BLKS:  	//cmd 22   /* adtc                    R1  */
		 	RTKSDPRINTF_WARM("is cmd ACMD22 SD_APP_SEND_NUM_WR_BLKS \n");
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
            break;

		case MMC_SET_BLOCK_COUNT:		//cmd 23 /* adtc [31:0] data addr R1 */
		 	RTKSDPRINTF_WARM("is cmd CMD23 ACMD23 MMC_SET_BLOCK_COUNT\n");
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
            break;

		case MMC_WRITE_BLOCK:			//cmd 24 /* adtc [31:0] data addr R1 */
		 	RTKSDPRINTF_WARM("is cmd CMD24 MMC_WRITE_BLOCK\n");
            cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x1;
            cmd_info->rsp_para3 = -1;
            break;

        case MMC_WRITE_MULTIPLE_BLOCK:	//cmd 25	/* adtc                    R1  */
		 	RTKSDPRINTF_WARM("is cmd CMD25 MMC_WRITE_MULTIPLE_BLOCK\n");
            cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x1;
            cmd_info->rsp_para3 = 0x2;
            break;

//host not support CMD27
		case MMC_PROGRAM_CSD:			//cmd 27 /* adtc R1 */
		 	RTKSDPRINTF_WARM("is cmd CMD27 MMC_PROGRAM_CSD\n");
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
			break;

		case MMC_SET_WRITE_PROT:		//cmd 28 /* ac [31:0] data addr R1b */
		 	RTKSDPRINTF_WARM("is cmd CMD28 MMC_SET_WRITE_PROT \n");
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
            break;

		case MMC_CLR_WRITE_PROT:		//cmd 29 /* ac [31:0] data addr R1b */
		 	RTKSDPRINTF_WARM("is cmd CMD29 MMC_CLR_WRITE_PROT \n");
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
            break;

		case MMC_SEND_WRITE_PROT:		//cmd 30 /* adtc [31:0] wpdata addr R1 */
		 	RTKSDPRINTF_WARM("is cmd CMD30 MMC_SEND_WRITE_PROT \n");
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
            break;
		
		case SD_ERASE_WR_BLK_START:    	//cmd 32   /* ac   [31:0] data addr   R1  */
		 	RTKSDPRINTF_WARM("is cmd CMD32 SD_ERASE_WR_BLK_START \n");
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
            break;

		case SD_ERASE_WR_BLK_END:      	//cmd 33   /* ac   [31:0] data addr   R1  */
		 	RTKSDPRINTF_WARM("is cmd CMD33 SD_ERASE_WR_BLK_END \n");
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
            break;

		case MMC_ERASE:					//cmd 38 /* ac R1b */
		 	RTKSDPRINTF_WARM("is cmd CMD38 SD_ERASE_WR_BLK_END \n");
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
            break;

		case SD_APP_OP_COND:           	//cmd 41   /* bcr  [31:0] OCR         R3  */
		 	RTKSDPRINTF_WARM("is cmd ACMD41 SD_APP_OP_COND\n");
        	cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x45;
            cmd_info->rsp_para3 = 0x0;
            break;

//host not support CMD42
		case MMC_LOCK_UNLOCK:			//cmd 42 /* adtc R1b */
			if((cmd_info->cmd->flags & (0x3<<5)) == MMC_CMD_ADTC)	//SD_APP_SD_STATUS
			{
			 	RTKSDPRINTF_WARM("is cmd CMD42 MMC_LOCK_UNLOCK\n");
	            cmd_info->rsp_para1 = -1;     //don't update
	            cmd_info->rsp_para3 = -1;     //don't update
			}else{
			 	RTKSDPRINTF_WARM("is cmd ACMD42 SET_CLR_CARD_DETECT\n");
	            cmd_info->rsp_para1 = -1;     //don't update
	            cmd_info->rsp_para3 = -1;     //don't update
			}
            break;

//host not support ACMD51
		case SD_APP_SEND_SCR:          	//cmd 51   /* adtc                    R1  */
		 	RTKSDPRINTF_WARM("is cmd ACMD51 SD_APP_SEND_SCR\n");
        	cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x41;
            cmd_info->rsp_para3 = 0x5;
            break;

		case MMC_APP_CMD:				//cmd 55 /* ac [31:16] RCA R1 */
		 	RTKSDPRINTF_WARM("is cmd CMD55 MMC_APP_CMD\n");
        	cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x41;
            cmd_info->rsp_para3 = 0x5;
            break;

		case MMC_GEN_CMD:				//cmd 56 /* adtc [0] RD/WR R1 */
		 	RTKSDPRINTF_WARM("is cmd CMD56 MMC_GEN_CMD\n");
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
            break;

		case MMC_EXT_READ_SINGLE:		//cmd 48
		 	RTKSDPRINTF_WARM("is cmd CMD48 MMC_GEN_CMD\n");
            cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x1;
            cmd_info->rsp_para3 = -1;
            break;

		case MMC_EXT_WRITE_SINGLE:		//cmd 49
		 	RTKSDPRINTF_WARM("is cmd CMD49 MMC_GEN_CMD\n");
            cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x1;
            cmd_info->rsp_para3 = -1;
            break;

		case MMC_EXT_READ_MULTIPLE:		//cmd 58
		 	RTKSDPRINTF_WARM("is cmd CMD58 MMC_GEN_CMD\n");
            cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x1;
            if(MMCDetected == 1){
            	cmd_info->rsp_para3 = 0x4;//0x04;
            }else{
            	cmd_info->rsp_para3 = 0x2;//0x04;
            }
            break;

		case MMC_EXT_WRITE_MULTIPLE:	//cmd 59
		 	RTKSDPRINTF_WARM("is cmd CMD59 MMC_GEN_CMD\n");
            cmd_info->rsp_para1 = -1;
            cmd_info->rsp_para2 = 0x1;
            cmd_info->rsp_para3 = 0x2;
            break;

        default:
		 	RTKSDPRINTF_WARM("is cmd default, opcode = %d \n",cmd_info->cmd->opcode);
            cmd_info->rsp_para1 = -1;     //don't update
            cmd_info->rsp_para3 = -1;     //don't update
            break;
    }



    return 0;
}

static int rtk_crsd_SendCMDGetRSP_Cmd(struct crsd_cmd_pkt *cmd_info)
{
    u8 cmd_idx              = cmd_info->cmd->opcode;
    u32 sd_arg;
    s8 rsp_para1;
    s8 rsp_para2;
    s8 rsp_para3;
    u8 rsp_len              = cmd_info->rsp_len;
    u32 *rsp                = (u32 *)&cmd_info->cmd->resp;
    struct rtk_crsd_host *sdport = cmd_info->sdport;
    u32 iobase = sdport->base_io;
    int err;
//  unsigned long flags;
    u32 dma_val=0;
//  u32 byte_count = 0x200, block_count = 1, cpu_mode=0, sa=0;
    u32 byte_count = 0x200, block_count = 1, sa=0;
//  u8 tmp9_buf[1024]={0};
    u32 buf_ptr=0;


    rtk_crsd_set_rspparam(cmd_info);   //for 119x
    sd_arg              = cmd_info->cmd->arg;
    rsp_para1           = cmd_info->rsp_para1;
    rsp_para2           = cmd_info->rsp_para2;
    rsp_para3           = cmd_info->rsp_para3;

    RTKSDPRINTF( "%s: cmd_idx=%u, rsp_addr=0x%08x \n",
            DRIVER_NAME,cmd_idx,(unsigned int)rsp);
    RTKSDPRINTF( "%s: sd_arg=0x%08x; rsp_para1=0x%x \n",
            DRIVER_NAME,sd_arg,rsp_para1);
    RTKSDPRINTF( "%s: rsp_para2=0x%x rsp_para3=0x%x rsp_len=0x%x \n",
            DRIVER_NAME,rsp_para2,rsp_para3,rsp_len);


    if(rsp == NULL) {
        BUG_ON(1);
    }



/* SDIO commands                         type  argument     response */
//#define SD_IO_SEND_OP_COND          5 /* bcr  [23:0] OCR         R4  */
//#define SD_IO_RW_DIRECT            52 /* ac   [31:0] See below   R5  */
//#define SD_IO_RW_EXTENDED          53 /* adtc [31:0] See below   R5  */

    if ((cmd_idx == SD_IO_SEND_OP_COND)|(cmd_idx == SD_IO_RW_DIRECT)|(cmd_idx == SD_IO_RW_EXTENDED))
    {
        RTKSDPRINTF( "%s : reject SDIO commands cmd:0x%02x \n",DRIVER_NAME,cmd_idx);
        return CR_TRANSFER_FAIL;
    }


    if (rsp_para1 != -1)
        crsd_writeb(rsp_para1, iobase+SD_CONFIGURE1);
    crsd_writeb(rsp_para2,     iobase+SD_CONFIGURE2);
    if (rsp_para3 != -1)
        crsd_writeb(rsp_para3, iobase+SD_CONFIGURE3);
    g_cmd[0] = (0x40|cmd_idx);
    g_cmd[1] = (sd_arg>>24)&0xff;
    g_cmd[2] = (sd_arg>>16)&0xff;
    g_cmd[3] = (sd_arg>>8)&0xff;
    g_cmd[4] = sd_arg&0xff;
    g_cmd[5] = 0x00;

    crsd_writeb(g_cmd[0],    iobase+SD_CMD0);
    crsd_writeb(g_cmd[1],    iobase+SD_CMD1);
    crsd_writeb(g_cmd[2],    iobase+SD_CMD2);
    crsd_writeb(g_cmd[3],    iobase+SD_CMD3);
    crsd_writeb(g_cmd[4],    iobase+SD_CMD4);
    crsd_writeb(g_cmd[5],    iobase+SD_CMD5);
    sdport->cmd_opcode = cmd_idx;
    rtk_crsd_get_cmd_timeout(cmd_info);

    if (RESP_TYPE_17B & rsp_para2)
    {
        //remap the resp dst buffer to un-cache
        #if 1
        RTKSDPRINTF( "pRSP : 0x%08x\n", (unsigned int)pRSP);
        buf_ptr = ((u32)pRSP&~0xff);
        sa = buf_ptr/8;
        RTKSDPRINTF( "final buf addr : 0x%08x  (buf_ptr = 0x%x )\n", sa,buf_ptr);
        #else
        sa = (u32)rsp/8;
        #endif

        dma_val = RSP17_SEL|DDR_WR|DMA_XFER;
        RTKSDPRINTF( "-----rsp 17B-----\n DMA_sa=0x%08x DMA_len=0x%08x DMA_setting=0x%08x\n", sa,1,dma_val);
        crsd_writeb(byte_count,       iobase+SD_BYTE_CNT_L);     //0x24
        crsd_writeb(byte_count>>8,    iobase+SD_BYTE_CNT_H);     //0x28
        crsd_writeb(block_count,      iobase+SD_BLOCK_CNT_L);    //0x2C
        crsd_writeb(block_count>>8,   iobase+SD_BLOCK_CNT_H);    //0x30
//        if (cpu_mode && (iobase==CR_BASE_ADDR))
//            crsd_writel( CPU_MODE_EN, iobase+EMMC_CPU_ACC); //enable cpu mode
//        else
//            crsd_writel( 0, iobase+EMMC_CPU_ACC);
        crsd_writel(sa, iobase+CR_DMA_CTL1);   //espeical for R2
        crsd_writel(1, iobase+CR_DMA_CTL2);   //espeical for R2
        crsd_writel(dma_val, iobase+CR_DMA_CTL3);   //espeical for R2
        rtk_crsd_get_next_block();
    }
    else if (RESP_TYPE_6B & rsp_para2)
    {
        RTKSDPRINTF( "-----rsp 6B-----\n");
        RTKSDPRINTF( "do nothings\n");
		//cr_writel( 0x00, iobase+EMMC_CPU_ACC);
    }

    RTKSDPRINTF( "cmd0:0x%02x,cmd1:0x%02x,cmd2:0x%02x,cmd3:0x%02x,cmd4:0x%02x,cmd5:0x%02x\n", g_cmd[0],g_cmd[1],g_cmd[2],g_cmd[3],g_cmd[4],g_cmd[5]);

#ifdef ENABLE_SD_INT_MODE
    err = rtk_crsd_int_wait_opt_end(DRIVER_NAME,sdport,SD_SENDCMDGETRSP,0);
#else
    err = rtk_crsd_wait_opt_end(DRIVER_NAME,sdport,SD_SENDCMDGETRSP,0);
#endif

    if(err == CR_TRANS_OK){
      //if(buf_ptr != NULL)
        if(buf_ptr != 0)
        {
            //ignore start pattern
			int i;
			*(((unsigned int *)buf_ptr)+4) = crsd_readb(iobase+SD_CMD5);	//the last byte of 17 byte is from SD_CMD5, not from dma		
			for(i=0;i<5;i++)
			{
				RTKSDPRINTF_DBG( "befor 0x%x = 0x%x\n",(unsigned int)(((unsigned int *)buf_ptr)+i),*(((unsigned int *)buf_ptr)+i) );
			}
			
            buf_ptr++;
            rtk_crsd_read_rsp(sdport,(u32*)buf_ptr, rsp_len);
//			if (cmd_idx == SD_SWITCH)
//				memcpy(rsp, (u32*)buf_ptr, 64);
//			else
			for(i=0;i<5;i++)
			{
				RTKSDPRINTF_DBG( "after 0x%x = 0x%x\n",(unsigned int)(((unsigned int *)buf_ptr)+i),*(((unsigned int *)buf_ptr)+i) );
			}
			memcpy(rsp, (u32*)buf_ptr, 16);
			for(i=0;i<5;i++)
			{
				RTKSDPRINTF_DBG( "rsp[%d](0x%x) = 0x%x\n",i,((unsigned int)&(rsp[i])),rsp[i]);
			}
        }
        else
            rtk_crsd_read_rsp(sdport,rsp, rsp_len);
        rtk_crsd_sync(sdport);
        if (cmd_idx == MMC_SET_RELATIVE_ADDR)
        {
            g_crinit = 1;
            RTKSDPRINTF( "emmc init done ...\n");
        }
    }
    else{
        RTKSDPRINTF("%s: cmd trans fail, err=%d \n",DRIVER_NAME, err);
    }
    return err;
}

static int rtk_crsd_SendStopCMD(struct mmc_command *cmd, struct rtk_crsd_host *sdport)
{
    u8 cmd_idx;
    u32 sd_arg;
    s8 rsp_para1;
    s8 rsp_para2;
    s8 rsp_para3;
    u8 rsp_len;
    u32 iobase;
    int err;    
	struct mmc_command  stop_cmd;
	struct crsd_cmd_pkt stop_cmd_info;
	struct crsd_cmd_pkt * cmd_info;
				
	cmd_info = &stop_cmd_info;    
	err = 0;
	memset(&stop_cmd, 0, sizeof(struct mmc_command));
	memset(&stop_cmd_info, 0, sizeof(struct crsd_cmd_pkt));
	stop_cmd.opcode         = MMC_STOP_TRANSMISSION;
	stop_cmd.arg            = cmd->arg; // card's RCA, not necessary for cmd12
	stop_cmd_info.cmd       = &stop_cmd;
    stop_cmd_info.sdport    = sdport;
    stop_cmd_info.rsp_para2 = rtk_crsd_get_rsp_type(stop_cmd_info.cmd);
    stop_cmd_info.rsp_len   = rtk_crsd_get_rsp_len(stop_cmd_info.rsp_para2);
	cmd_idx = cmd_info->cmd->opcode;
	rsp_len = cmd_info->rsp_len;
	sdport = cmd_info->sdport;
	iobase = sdport->base_io;

    rtk_crsd_set_rspparam(cmd_info);   //for 119x

	//cmd_info->rsp_para2 &= 0xF7;// wait busy end disable
	//cmd_info->rsp_para3 = 0xE0;

    sd_arg              = cmd_info->cmd->arg;
    rsp_para1           = cmd_info->rsp_para1;
    rsp_para2           = cmd_info->rsp_para2;
    rsp_para3           = cmd_info->rsp_para3;

    if (rsp_para1 != -1) {
        crsd_writeb(rsp_para1, iobase+SD_CONFIGURE1);
    }

    crsd_writeb(rsp_para2,     iobase+SD_CONFIGURE2);

    if (rsp_para3 != -1) {
        crsd_writeb(rsp_para3, iobase+SD_CONFIGURE3);
    }

    g_cmd[0] = (0x40|cmd_idx);
    g_cmd[1] = (sd_arg>>24)&0xff;
    g_cmd[2] = (sd_arg>>16)&0xff;
    g_cmd[3] = (sd_arg>>8)&0xff;
    g_cmd[4] = sd_arg&0xff;
    g_cmd[5] = 0x00;

	//printk("*** cmd12, argu 0x%08x ***\n", sd_arg);
	//printk("*** rsp_para2 = 0x%02x, CFG2 = 0x%02x\n", rsp_para2, crsd_readb(iobase+SD_CONFIGURE2));
	//printk("*** rsp_para3 = 0x%02x, CFG2 = 0x%02x\n", rsp_para2, crsd_readb(iobase+SD_CONFIGURE3));

    crsd_writeb(g_cmd[0],    iobase+SD_CMD0);
    crsd_writeb(g_cmd[1],    iobase+SD_CMD1);
    crsd_writeb(g_cmd[2],    iobase+SD_CMD2);
    crsd_writeb(g_cmd[3],    iobase+SD_CMD3);
    crsd_writeb(g_cmd[4],    iobase+SD_CMD4);
    crsd_writeb(g_cmd[5],    iobase+SD_CMD5);
    sdport->cmd_opcode = cmd_idx;
    rtk_crsd_get_cmd_timeout(cmd_info);

    err = rtk_crsd_wait_opt_end(DRIVER_NAME,sdport,SD_SENDCMDGETRSP,0);

	//printk("--- rsp_para2 = 0x%02x, CFG2 = 0x%02x\n", rsp_para2, crsd_readb(iobase+SD_CONFIGURE2));
	//printk("--- rsp_para3 = 0x%02x, CFG3 = 0x%02x\n", rsp_para2, crsd_readb(iobase+SD_CONFIGURE3));
	//printk("--- XFER = 0x%02x\n", crsd_readb(iobase+SD_TRANSFER));

    if(err == CR_TRANS_OK){
        rtk_crsd_sync(sdport);
    }
    else{
        RTKSDPRINTF("%s: cmd trans fail, err=%d \n",DRIVER_NAME, err);
    }
    return err;
}
#ifdef CMD25_USE_SD_AUTOWRITE2
static int rtk_crsd_SendStopCMD_and_not_check_done(struct mmc_command *cmd, struct rtk_crsd_host *sdport)
{
    u8 cmd_idx;
    u32 sd_arg;
    s8 rsp_para1;
    s8 rsp_para2;
    s8 rsp_para3;
    u8 rsp_len;
    u32 iobase;
    int err;    
	struct mmc_command  stop_cmd;
	struct crsd_cmd_pkt stop_cmd_info;
	struct crsd_cmd_pkt * cmd_info;
				
	cmd_info = &stop_cmd_info;    
	err = 0;
	memset(&stop_cmd, 0, sizeof(struct mmc_command));
	memset(&stop_cmd_info, 0, sizeof(struct crsd_cmd_pkt));
	stop_cmd.opcode         = MMC_STOP_TRANSMISSION;
	stop_cmd.arg            = cmd->arg; // card's RCA
	stop_cmd_info.cmd       = &stop_cmd;
    stop_cmd_info.sdport    = sdport;
    stop_cmd_info.rsp_para2 = rtk_crsd_get_rsp_type(stop_cmd_info.cmd);
    stop_cmd_info.rsp_len   = rtk_crsd_get_rsp_len(stop_cmd_info.rsp_para2);
	cmd_idx = cmd_info->cmd->opcode;
	rsp_len = cmd_info->rsp_len;
	sdport = cmd_info->sdport;
	iobase = sdport->base_io;

    rtk_crsd_set_rspparam(cmd_info);   //for 119x

	//cmd_info->rsp_para2 &= 0xF7;// wait busy end disable
	//cmd_info->rsp_para3 = 0xE0;

    sd_arg              = cmd_info->cmd->arg;
    rsp_para1           = cmd_info->rsp_para1;
    rsp_para2           = cmd_info->rsp_para2;
    rsp_para3           = cmd_info->rsp_para3;

    if (rsp_para1 != -1) {
        crsd_writeb(rsp_para1, iobase+SD_CONFIGURE1);
    }

    crsd_writeb(rsp_para2,     iobase+SD_CONFIGURE2);

    if (rsp_para3 != -1) {
        crsd_writeb(rsp_para3, iobase+SD_CONFIGURE3);
    }

    g_cmd[0] = (0x40|cmd_idx);
    g_cmd[1] = (sd_arg>>24)&0xff;
    g_cmd[2] = (sd_arg>>16)&0xff;
    g_cmd[3] = (sd_arg>>8)&0xff;
    g_cmd[4] = sd_arg&0xff;
    g_cmd[5] = 0x00;

	//printk("*** rsp_para2 = 0x%02x, CFG2 = 0x%02x\n", rsp_para2, crsd_readb(iobase+SD_CONFIGURE2));
	//printk("*** rsp_para3 = 0x%02x, CFG2 = 0x%02x\n", rsp_para2, crsd_readb(iobase+SD_CONFIGURE3));

    crsd_writeb(g_cmd[0],    iobase+SD_CMD0);
    crsd_writeb(g_cmd[1],    iobase+SD_CMD1);
    crsd_writeb(g_cmd[2],    iobase+SD_CMD2);
    crsd_writeb(g_cmd[3],    iobase+SD_CMD3);
    crsd_writeb(g_cmd[4],    iobase+SD_CMD4);
    crsd_writeb(g_cmd[5],    iobase+SD_CMD5);
    sdport->cmd_opcode = cmd_idx;
    rtk_crsd_get_cmd_timeout(cmd_info);

#if 1 // jinn
	rtk_crsd_sync(sdport);
    crsd_writeb((u8) (SD_SENDCMDGETRSP|START_EN), sdport->base_io+SD_TRANSFER );
	force_check_previous_xfer_done = 1;
	return 0; // always return 0
#else
    err = rtk_crsd_wait_opt_end(DRIVER_NAME,sdport,SD_SENDCMDGETRSP,0);

	//printk("--- rsp_para2 = 0x%02x, CFG2 = 0x%02x\n", rsp_para2, crsd_readb(iobase+SD_CONFIGURE2));
	//printk("--- rsp_para3 = 0x%02x, CFG3 = 0x%02x\n", rsp_para2, crsd_readb(iobase+SD_CONFIGURE3));
	//printk("--- XFER = 0x%02x\n", crsd_readb(iobase+SD_TRANSFER));

    if(err == CR_TRANS_OK){
        rtk_crsd_sync(sdport);
    }
    else{
        RTKSDPRINTF("%s: cmd trans fail, err=%d \n",DRIVER_NAME, err);
    }
    return err;
#endif
}
#endif

static int rtk_crsd_SendCMDGetRSP(struct crsd_cmd_pkt * cmd_info)
{
    int rc;
    rc = rtk_crsd_SendCMDGetRSP_Cmd(cmd_info);

    return rc;
}

static int rtk_crsd_Stream_Cmd(u16 cmdcode,struct crsd_cmd_pkt *cmd_info,u8 data_len)
{
    u8 cmd_idx              = cmd_info->cmd->opcode;
    u32 sd_arg              = cmd_info->cmd->arg;
    s8 rsp_para1             = cmd_info->rsp_para1;
    s8 rsp_para2             = cmd_info->rsp_para2;
    s8 rsp_para3             = cmd_info->rsp_para3;
    int rsp_len             = cmd_info->rsp_len;
    u32 *rsp                = (u32 *)&cmd_info->cmd->resp;
    u16 byte_count          = cmd_info->byte_count;
    u16 block_count         = cmd_info->block_count;
    struct rtk_crsd_host *sdport = cmd_info->sdport;
    u32 iobase = sdport->base_io;
    int err;
    u8 *data              = cmd_info->dma_buffer;
//  unsigned long flags;
    u32 cpu_mode=0;
    u32 sa=0;

    RTKSDPRINTF( "%s: cmd_idx=%u, rsp_addr=0x%08x \n",
            DRIVER_NAME,cmd_idx,(unsigned int)rsp);
    RTKSDPRINTF( "%s: sd_arg=0x%08x; rsp_para1=0x%x \n",
            DRIVER_NAME,sd_arg,rsp_para1);
    RTKSDPRINTF( "%s: rsp_para2=0x%x rsp_para3=0x%x rsp_len=0x%x \n",
            DRIVER_NAME,rsp_para2,rsp_para3,rsp_len);


    if(data == NULL || rsp == NULL) {
        BUG_ON(1);
    }

    rtk_crsd_set_rspparam(cmd_info);   //for 119x
    sd_arg              = cmd_info->cmd->arg;
    if (rsp_para1 != -1)
        rsp_para1           = cmd_info->rsp_para1;

    /* Fix CRC error for SD_AUTOWRITE3, jamestai20150325 */
    if(cmdcode == SD_AUTOWRITE3){
    	rsp_para2 = 0;
    }else{
    	rsp_para2 = cmd_info->rsp_para2;
    }

    if (rsp_para3 != -1)
        rsp_para3           = cmd_info->rsp_para3;
    sa = (u32)data/8;

    if((cmdcode==SD_NORMALWRITE)){
        byte_count = 512;
    }else if(cmdcode==SD_NORMALREAD && MMCDetected == 1){
    	byte_count = 512;
    }

    g_cmd[0] = (0x40|cmd_idx);
    g_cmd[1] = (sd_arg>>24)&0xff;
    g_cmd[2] = (sd_arg>>16)&0xff;
    g_cmd[3] = (sd_arg>>8)&0xff;
    g_cmd[4] = sd_arg&0xff;
    g_cmd[5] = 0x00;

    crsd_writeb(g_cmd[0],    iobase+SD_CMD0);           //0x10
    crsd_writeb(g_cmd[1],    iobase+SD_CMD1);           //0x14
    crsd_writeb(g_cmd[2],    iobase+SD_CMD2);           //0x18
    crsd_writeb(g_cmd[3],    iobase+SD_CMD3);           //0x1C
    crsd_writeb(g_cmd[4],    iobase+SD_CMD4);           //0x20
    crsd_writeb(g_cmd[5],    iobase+SD_CMD5);           //0x20

    if (rsp_para1 != -1)
    {
        crsd_writeb(rsp_para1,         iobase+SD_CONFIGURE1);     //0x0C
        RTKSDPRINTF( "read configure1 : 0x%02x\n", crsd_readb(iobase+SD_CONFIGURE1));
    }
    crsd_writeb(rsp_para2,         iobase+SD_CONFIGURE2);     //0x0C
    if (rsp_para3 != -1)
    {
        crsd_writeb(rsp_para3,         iobase+SD_CONFIGURE3);     //0x0C
        RTKSDPRINTF( "read configure3 : 0x%02x\n", crsd_readb(iobase+SD_CONFIGURE3));
    }
    crsd_writeb(byte_count,       iobase+SD_BYTE_CNT_L);     //0x24
    crsd_writeb(byte_count>>8,    iobase+SD_BYTE_CNT_H);     //0x28
    crsd_writeb(block_count,      iobase+SD_BLOCK_CNT_L);    //0x2C
    crsd_writeb(block_count>>8,   iobase+SD_BLOCK_CNT_H);    //0x30

    if(cmd_info->cmd->data->flags & MMC_DATA_READ){
        RTKSDPRINTF( "-----mmc data ddr read-----\n");
//        crsd_writel( 0x00, iobase+EMMC_CPU_ACC);
        crsd_writel( (u32)sa, iobase+CR_DMA_CTL1);
        crsd_writel( block_count, iobase+CR_DMA_CTL2);
		if(data_len==RESP_LEN64)
		{
			crsd_writel( RSP64_SEL|DDR_WR|DMA_XFER, iobase+CR_DMA_CTL3);
		} 
		else if(data_len==RESP_LEN17) 
		{
			crsd_writel( RSP17_SEL|DDR_WR|DMA_XFER, iobase+CR_DMA_CTL3);
		} 
		else 
		{
	        crsd_writel( DDR_WR|DMA_XFER, iobase+CR_DMA_CTL3);
		}
	}else if(cmd_info->cmd->data->flags & MMC_DATA_WRITE){
        RTKSDPRINTF( "-----mmc data write-----\n");
        RTKSDPRINTF( "DMA sa = 0x%x\nDMA len = 0x%x\nDMA set = 0x%x\n", (u32)sa, block_count, DMA_XFER);

//        crsd_writel( 0, iobase+EMMC_CPU_ACC);
        crsd_writel( (u32)sa, iobase+CR_DMA_CTL1);
        crsd_writel( block_count, iobase+CR_DMA_CTL2);

		if(data_len==RESP_LEN64)
		{
			crsd_writel( RSP64_SEL|DMA_XFER, iobase+CR_DMA_CTL3);
		} 
		else if(data_len==RESP_LEN17)
		{
			crsd_writel( RSP17_SEL|DMA_XFER, iobase+CR_DMA_CTL3);
		} 
		else 
		{
	        crsd_writel( DMA_XFER, iobase+CR_DMA_CTL3);
		}

    }
    else //sram read
    {
   	    BUG_ON(1);	//victor ????
        RTKSDPRINTF( "-----mmc data sram read (cpu mode)-----\n");

        cpu_mode = 1;
//        crsd_writel( 0, iobase+EMMC_CPU_ACC);
//        crsd_writel( CPU_MODE_EN, iobase+EMMC_CPU_ACC);
        crsd_writel( (u32)sa, iobase+CR_DMA_CTL1);
        crsd_writel( block_count, iobase+CR_DMA_CTL2);
        crsd_writel( DDR_WR|DMA_XFER, iobase+CR_DMA_CTL3);
    }

    sdport->cmd_opcode = cmd_idx;
    rtk_crsd_get_cmd_timeout(cmd_info);

#ifdef ENABLE_SD_INT_MODE
    err = rtk_crsd_int_wait_opt_end(DRIVER_NAME, sdport, cmdcode, cpu_mode);
#else
    err = rtk_crsd_wait_opt_end(DRIVER_NAME,sdport,cmdcode,cpu_mode);
#endif

    if(cmd_info->cmd->opcode == MMC_SEND_TUNING_BLOCK)
	{
		//reset dat64_sel and rsp17_sel, #CMD19 DMA won't be auto-cleared
		crsd_writel( 0x0, iobase+CR_DMA_CTL3);	
	}

    if(err == CR_TRANS_OK){
        if((cmdcode == SD_AUTOREAD1) || (cmdcode == SD_AUTOWRITE1))
        {
            RTKSDPRINTF( "AUTO READ/WRITE 1 skip response~\n");
        }
#ifdef CMD25_WO_STOP_COMMAND
        else if( cmdcode == SD_AUTOWRITE2 ) {
			// CMD + DATA
			RTKSDPRINTF( "AUTO WRITE 2 skip response~\n");
		}
		else if( cmdcode == SD_AUTOWRITE3 ) {
			// DATA only, clear rsp
			RTKSDPRINTF( "AUTO WRITE 3 skip response~\n");
		}
#endif
        else
        {
            rtk_crsd_read_rsp(sdport,rsp, rsp_len);
            RTKSDPRINTF( "---stream cmd done---\n");
        }
    }

    if(err){
        RTKSDPRINTF( "%s: %s fail\n",DRIVER_NAME,__func__);
    }
    return err;
}

static u32 rtk_crsd_chk_cmdcode(struct mmc_command* cmd){
    u32 cmdcode;
    RTKSDPRINTF( "\n");

    if(cmd->opcode <= 59){
    	if(cmd->opcode == 8 && MMCDetected == 1){
    		cmdcode = SD_NORMALREAD;
    	}else{
    		cmdcode = (u32)rtk_crsd_cmdcode[cmd->opcode][0];
    	}
        WARN_ON(cmd->data == NULL);
        if(cmd->data->flags & MMC_DATA_WRITE){
            if(cmd->opcode == 42)
                cmdcode = SD_NORMALWRITE;
            else if(cmd->opcode == 56)
                cmdcode = SD_AUTOWRITE2;
        }
    }else{
        cmdcode = SD_CMD_UNKNOW;
    }

    return cmdcode;

}
/*
static u32 rtk_crsd_chk_r1_type(struct mmc_command* cmd){
    RTKSDPRINTF( "\n");
    return 0;
}
*/
static u8 rtk_crsd_get_rsp_type(struct mmc_command* cmd){
    u32 rsp_type;
    RTKSDPRINTF( "\n");
    if ( mmc_resp_type(cmd)==MMC_RSP_R1 ){
        rsp_type = SD_R1;
    }else if ( mmc_resp_type(cmd)== MMC_RSP_R1B ){
        rsp_type = SD_R1b;
    }else if ( mmc_resp_type(cmd)== MMC_RSP_R2 ){
        rsp_type = SD_R2;
    }else if ( mmc_resp_type(cmd)== MMC_RSP_R3 ){
        rsp_type = SD_R3;
    }else if ( mmc_resp_type(cmd)== MMC_RSP_R6 ){
        rsp_type = SD_R6;
    }else if ( mmc_resp_type(cmd)== MMC_RSP_R7 ){
        rsp_type = SD_R7;
    }else{
        rsp_type = SD_R0;
    }
    return rsp_type;
}


static int rtk_crsd_Stream(struct crsd_cmd_pkt *cmd_info)
{
    u8 cmd_idx              = cmd_info->cmd->opcode;
    int err=0;
    u32 i;
    struct scatterlist *sg;
    u32 dir = 0;
    u32 dma_nents = 0;
    u32 dma_leng;
    u32 dma_addr;
    u32 old_arg;
//	u8 one_blk=0;
    u16 cmdcode = 0;
	u8 data_len = 0;
//  unsigned long flags;
    //enum dma_data_direction	direction;
    //dma_addr_t		dma_addr = 0;

    struct mmc_host *host = cmd_info->sdport->mmc;
    struct rtk_crsd_host *sdport = cmd_info->sdport;
    
#ifdef CMD25_WO_STOP_COMMAND
	rtksd_mmc_cmd = cmd_info->cmd;
	rtksd_sdport =	sdport;
#endif

    rtk_crsd_set_rspparam(cmd_info);   //for 119x

    if(cmd_info->data->flags & MMC_DATA_READ)
	{
        dir = DMA_FROM_DEVICE;
    }
	else
	{
        dir = DMA_TO_DEVICE;
    }

    cmd_info->data->bytes_xfered=0;
    dma_nents = dma_map_sg(mmc_dev(host), cmd_info->data->sg, cmd_info->data->sg_len, dir);
    sg = cmd_info->data->sg;

    RTKSDPRINTF( "sg_len:%u\n",cmd_info->data->sg_len);
    RTKSDPRINTF( "sg:0x%p; dma_nents:%u\n",sg,dma_nents);

    old_arg=cmd_info->cmd->arg;

    for(i=0; i<dma_nents; i++,sg++)
	{
        u32 blk_cnt;
		
		dma_leng = sg_dma_len(sg);
		dma_addr = sg_dma_address(sg);

        RTKSDPRINTF( "dma_addr:0x%x; dma_leng:0x%x\n",dma_addr,dma_leng);
        RTKSDPRINTF("host=%p\n",host);


		if((cmd_idx == SD_SWITCH)&&(cmd_info->cmd->flags | MMC_CMD_ADTC))
		{
		 	RTKSDPRINTF("is cmd 6 SD_SWITCH\n");
            cmd_info->byte_count = 0x40;     
            blk_cnt = dma_leng/0x40;
			data_len = RESP_LEN64;
		}
		else if((cmd_idx == SD_APP_SD_STATUS)&&((cmd_info->cmd->flags & (0x3<<5)) == MMC_CMD_ADTC))
		{
		 	RTKSDPRINTF("is acmd 13 SD_APP_SD_STATUS\n");
            cmd_info->byte_count = 0x40;     
            blk_cnt = dma_leng/0x40;
			data_len = RESP_LEN64;
		}
		else if((cmd_idx == MMC_SEND_TUNING_BLOCK)|(cmd_idx == SD_APP_SEND_SCR)|(cmd_idx == SD_APP_SEND_NUM_WR_BLKS))
		{
            cmd_info->byte_count = 0x40;     //rtk HW limite, one trigger 512 byte pass.
            blk_cnt = 1;
//	            blk_cnt = dma_leng/0x40;
			data_len = RESP_LEN64;
		}
		else if((cmd_idx == MMC_ALL_SEND_CID)|(cmd_idx == MMC_SEND_CSD)|(cmd_idx == MMC_SEND_CID))
		{
            cmd_info->byte_count = 0x11;
            blk_cnt = dma_leng/0x11;
			data_len = RESP_LEN17;
		}
		else
		{
            cmd_info->byte_count = BYTE_CNT;     //rtk HW limite, one trigger 512 byte pass.
            blk_cnt = dma_leng/BYTE_CNT;
			data_len = 0;
		}

        if(blk_cnt == 0 && dma_leng)
		{
            blk_cnt = 1;
        }

        cmd_info->block_count = blk_cnt;
        cmd_info->dma_buffer = (unsigned char *)dma_addr;

        //if(cmd_info->data->flags & MMC_DATA_READ){
		//	dma_cache_inv(KSEG0ADDR(dma_addr), dma_leng);
        //}
        //else
        //{
        //	rtk_crsd_wait_status(host->card,STATE_TRAN,0);
        //}
//printk(KERN_ERR ">>> %s [%d][cmd %d(0x%02x)][blks %d]\n",__FUNCTION__, i, cmd_idx, cmd_idx, cmd_info->block_count);
        cmdcode = sdport->ops->chk_cmdcode(cmd_info->cmd);

#ifdef CMD25_WO_STOP_COMMAND // new write method
//MMC_DEBUG_PRINT("            [%s][%d] CMD %d, Argu 0x%08x, Last Addr 0x%08x, card 0x%08x\n", __FUNCTION__,__LINE__, cmd_info->cmd->opcode, cmd_info->cmd->arg, sd_current_blk_address, host->card);
//MMC_DEBUG_PRINT("            [%s][%d] CMD %d, Argu 0x%08x, Last Addr 0x%08x\n", __FUNCTION__,__LINE__, cmd_info->cmd->opcode, cmd_info->cmd->arg, sd_current_blk_address);
		if( host->card && mmc_card_blockaddr(host->card) && (
			cmd_info->cmd->opcode == MMC_WRITE_BLOCK ||
			cmd_info->cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ||
			cmd_info->cmd->opcode == MMC_EXT_WRITE_MULTIPLE ) )
		{
			if( sd_in_receive_data_state ) {
				if( sd_current_blk_address == cmd_info->cmd->arg /*&& (sd_previous_blk_section == (cmd_info->cmd->arg>>19) )*/ ) {
MMC_DEBUG_PRINT("            [%s][%d] addr no gap\n", __FUNCTION__,__LINE__);
					cmdcode = SD_AUTOWRITE3; // DATA only
				}
				else {
MMC_DEBUG_PRINT("            [%s][%d] addr has gap !!!\n", __FUNCTION__,__LINE__);
					// send stop command first
					rtk_crsd_SendStopCMD(cmd_info->cmd, sdport);

					// stop trigger
					
					// not set SD in receive-data state
					sd_in_receive_data_state = 0;
					cmdcode = SD_AUTOWRITE2; // CMD + DATA
				}
			}
			else {
				cmdcode = SD_AUTOWRITE2; // CMD + DATA
			}

			if( cmd_info->cmd->opcode == MMC_WRITE_BLOCK ) {
				MMC_DEBUG_PRINT("            [%s][%d] change cmd24 --> cmd25\n", __FUNCTION__,__LINE__);
			}

			// alwasy use multi-write command
			if(cmd_info->cmd->opcode == MMC_EXT_WRITE_MULTIPLE){
				cmd_info->cmd->opcode = MMC_EXT_WRITE_MULTIPLE;
			}else{
				cmd_info->cmd->opcode = MMC_WRITE_MULTIPLE_BLOCK;
			}
		}
		else {
			if( cmd_info->cmd->opcode == MMC_SEND_STATUS ) { // CMD13
				// issue command	
			}
			else {
				if( sd_in_receive_data_state ) {
MMC_DEBUG_PRINT("            [%s][%d] sd in receive state\n", __FUNCTION__,__LINE__);
					// send stop command first
					rtk_crsd_SendStopCMD(cmd_info->cmd, sdport);

					// stop trigger
					
					// not set SD in receive-data state
					sd_in_receive_data_state = 0;
				}
				// issue command
			}
		}
#endif

//printk("[%d][cmd %d][argu 0x%08x][block_cnt 0x%04x]\n", i, cmd_info->cmd->opcode, cmd_info->cmd->arg, blk_cnt);	
if( cmd_info->cmd->opcode == MMC_WRITE_BLOCK ||
	cmd_info->cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ||
	cmd_info->cmd->opcode == MMC_READ_SINGLE_BLOCK ||
	cmd_info->cmd->opcode == MMC_READ_MULTIPLE_BLOCK )
{
MMC_DEBUG_PRINT("            [%s][%d] blksz = %d, blocks = %d, dma_leng = %d, blk_cnt = %d\n", __FUNCTION__,__LINE__, cmd_info->cmd->data->blksz, cmd_info->cmd->data->blocks, dma_leng, blk_cnt);
}
        err = rtk_crsd_Stream_Cmd(cmdcode,cmd_info,data_len);


        //if(cmd_info->data->flags & MMC_DATA_WRITE){
		//	dma_cache_wback(KSEG0ADDR(dma_addr), dma_leng);
        //}
        //dma_unmap_page(host->parent, dma_addr, PAGE_SIZE, dir);

        if(err == 0)
		{
#ifdef CMD25_WO_STOP_COMMAND // new write method
			if( host->card && mmc_card_blockaddr(host->card) && (
				cmd_info->cmd->opcode == MMC_WRITE_BLOCK ||
				cmd_info->cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ||
				cmd_info->cmd->opcode == MMC_EXT_WRITE_MULTIPLE ) )
			{
//				sd_previous_blk_section = cmd_info->cmd->arg >> 19; // div 0x80000
				sd_current_blk_address = cmd_info->cmd->arg + blk_cnt;
				sd_in_receive_data_state = 1;
				mod_timer(&rtksd_stop_cmd_timer, jiffies + 1*HZ );
			}
#endif
			cmd_info->cmd->arg += dma_leng;
            cmd_info->data->bytes_xfered += dma_leng;
        }

        if(err)
		{
            cmd_info->cmd->arg = old_arg;
            break;
        }

    }
    dma_unmap_sg(mmc_dev(host), cmd_info->data->sg, cmd_info->data->sg_len, dir);

    //reset cmd0
    g_cmd[0] = 0x00;

    return err;

}

static void rtk_crsd_req_end_tasklet(unsigned long param)
{
    struct rtk_crsd_host *sdport;
    struct mmc_request* mrq;
    unsigned long flags;
    RedSDPRINTF( "\n");
    sdport = (struct rtk_crsd_host *)param;
    spin_lock_irqsave(&sdport->lock,flags);

    mrq = sdport->mrq;
    sdport->mrq = NULL;

    spin_unlock_irqrestore(&sdport->lock, flags);
    mmc_request_done(sdport->mmc, mrq);
MMC_DEBUG_PRINT("          [%s][%d][mrq ptr 0x%08x]\n", __FUNCTION__,__LINE__, (unsigned int)mrq);
}

static void rtk_crsd_send_command(struct rtk_crsd_host *sdport, struct mmc_command *cmd)
{
    int rc = 0;
    struct crsd_cmd_pkt cmd_info;
//  unsigned long flags;
    RTKSDPRINTF( "\n");
    memset(&cmd_info, 0, sizeof(struct crsd_cmd_pkt));

    if ( !sdport || !cmd ){
        RTKSDPRINTF( "%s: sdport or cmd is null\n",DRIVER_NAME);
        return ;
    }

    if (cmd->opcode == MMC_SEND_OP_COND){
    	MMCDetected = 1;
    }

	/*work around of bug switch voltage fail : force clock disable after cmd11*/
    if(cmd->opcode == SD_SWITCH_VOLTAGE)
    {
		crsd_writeb(0x40, sdport->base_io+SD_BUS_STATUS);
		RTKSDPRINTF( "set SD_BUS_STATUS 0x%x= %x\n",sdport->base_io+SD_BUS_STATUS,crsd_readb(sdport->base_io+SD_BUS_STATUS));
 	}
if (cmd->data)
MMC_DEBUG_PRINT("          [%s][%d][mrq ptr 0x%08x][cmd %d][argu 0x%08x][%d:%d]\n", __FUNCTION__,__LINE__, (unsigned int)(sdport->mrq), cmd->opcode, cmd->arg, cmd->data->blksz, cmd->data->blocks);
else
MMC_DEBUG_PRINT("          [%s][%d][mrq ptr 0x%08x][cmd %d][argu 0x%08x]\n", __FUNCTION__,__LINE__, (unsigned int)(sdport->mrq), cmd->opcode, cmd->arg);	
    cmd_info.cmd    = cmd;
    cmd_info.sdport = sdport;
    cmd_info.rsp_para2 = rtk_crsd_get_rsp_type(cmd_info.cmd);
    cmd_info.rsp_len  = rtk_crsd_get_rsp_len(cmd_info.rsp_para2);

    if (cmd->data)
	{
        cmd_info.data = cmd->data;
        if(cmd->data->flags == MMC_DATA_READ)
		{
            /* do nothing */
        }
		else if(cmd->data->flags == MMC_DATA_WRITE)
        {
            if(sdport->wp ==1)
			{
                RTKSDPRINTF("%s: card is locked!",
                            DRIVER_NAME);
                rc = -1;
                cmd->retries = 0;
                goto err_out;
            }
        }
		else
		{
            RTKSDPRINTF( "error: cmd->data->flags=%d\n",
                    cmd->data->flags);
            cmd->error = -MMC_ERR_INVALID;
            cmd->retries = 0;
            goto err_out;
        }

	    if(cmd->opcode != SD_APP_SEND_SCR)
    	{
MMC_DEBUG_PRINT("          [%s][%d]\n", __FUNCTION__,__LINE__);
	        rc = rtk_crsd_Stream(&cmd_info);
#ifdef CMD25_USE_SD_AUTOWRITE2
			if( (cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK )&& !rc ) {
				rc = rtk_crsd_SendStopCMD_and_not_check_done(cmd, sdport);
			}
#endif
		}
		else
		{
			struct scatterlist sg;
			struct mmc_data data_SCR = {0};
			void *ssr;
			
			ssr = kmalloc(64, GFP_KERNEL);
			if (!ssr)
			{
				rc = CR_TRANSFER_FAIL;
				goto err_out;
			}
			sg_init_one(&sg, ssr, 64);

		    RTKSDPRINTF("data->blksz= %d, data->blocks= %d \n",cmd_info.data->blksz,cmd_info.data->blocks);

			data_SCR.blksz = cmd_info.data->blksz;
			data_SCR.blocks = cmd_info.data->blocks;
			data_SCR.sg = cmd_info.data->sg;
			data_SCR.sg_len = cmd_info.data->sg_len;

			cmd_info.data->blksz = 64;
			cmd_info.data->blocks = 1;
			cmd_info.data->sg = &sg;
			cmd_info.data->sg_len = 1;

MMC_DEBUG_PRINT("          [%s][%d]\n", __FUNCTION__,__LINE__);
		    rc = rtk_crsd_Stream(&cmd_info);

			if(!rc)
			{
				int i;
				sg_copy_from_buffer(data_SCR.sg, data_SCR.sg_len, ssr, data_SCR.blksz);
                cmd_info.data->bytes_xfered = data_SCR.blksz;
			    RTKSDPRINTF("SCR =\n");
				for(i=0;i<8;i++)
				    RTKSDPRINTF(" 0x%x= %x\n",((unsigned int)ssr+i),*((unsigned char*)((unsigned int)ssr+i)) );

			}
			kfree(ssr);
		}
    }
    else
	{
MMC_DEBUG_PRINT("          [%s][%d]\n", __FUNCTION__,__LINE__);
        rc = rtk_crsd_SendCMDGetRSP(&cmd_info);
    }

    RTKSDPRINTF("reg SD_CONFIGURE1 0x%x= %x\n",sdport->base_io+SD_CONFIGURE1,crsd_readb(sdport->base_io+SD_CONFIGURE1));
    RTKSDPRINTF("reg CR_SD_CKGEN_CTL 0x%x= %x\n",sdport->base_io+CR_SD_CKGEN_CTL,crsd_readl(sdport->base_io+CR_SD_CKGEN_CTL));
    RTKSDPRINTF("reg CR_PFUNC_CR 0x%x= %x\n",sdport->base_pll_io+CR_PFUNC_CR,crsd_readl(sdport->base_pll_io+CR_PFUNC_CR));
    if(cmd->opcode == MMC_SELECT_CARD)
    {
		sdport->rtflags |= RTKCR_FCARD_SELECTED;
		rtk_crsd_speed(sdport, CRSD_CLOCK_6200KHZ);
    }
	else if((cmd->opcode == SD_SEND_RELATIVE_ADDR)&&((cmd->flags & (0x3<<5)) == MMC_CMD_BCR))
	{
		crsd_rca = ((cmd->resp[0]) >> RCA_SHIFTER);
	}


//	else if((cmd->opcode == SD_SWITCH)&&((cmd->flags & (0x3<<5)) == MMC_CMD_ADTC))
//	{
//		rtk_crsd_tune_phase(&cmd_info);
//	}


	rtk_crsd_dump_reg(sdport);
/*
    if(cmd->opcode == MMC_SWITCH){
         if((cmd->arg & 0xffff00ff) == 0x03b30001) {
            if((cmd->arg & 0x0000ff00)==0){
                sdport->rtflags |= RTKCR_USER_PARTITION;
            }else{
                sdport->rtflags &= ~RTKCR_USER_PARTITION;
            }
         }
    }
*/
err_out:
    if (rc){
        if(rc == -RTK_RMOV)
            cmd->retries = 0;

        /* If SD card card no response do SD host reset, jamestai20151008 */
        if(cmd->opcode == 49 || cmd->opcode == 59 || cmd->opcode == 48|| cmd->opcode == 58){
            rtk_crsd_reset(sdport);
            cmd->error  = -110;
        }else{
            cmd->error = -MMC_ERR_FAILED;
        }
    }
    tasklet_schedule(&sdport->req_end_tasklet);
}

static void rtk_crsd_request(struct mmc_host *host, struct mmc_request *mrq)
{
    struct rtk_crsd_host *sdport;
    struct mmc_command *cmd;
    unsigned char card_cmd = 0;
    RedSDPRINTF( "\n");

    sdport = mmc_priv(host);
    BUG_ON(sdport->mrq != NULL);

    //spin_lock_irqsave(&sdport->lock,flags);
    down_write(&cr_rw_sem);
    cmd = mrq->cmd;
    sdport->mrq = mrq;


    /* Check SD card extension command support,jamestai20151008*/
    if (cmd->opcode == 58 || cmd->opcode == 59) {
    	card_cmd = host->card->raw_scr[0] & 0xF;
    	if((card_cmd & 0x8) != 0x8) {
    		cmd->error = -110;
    		goto done;
    	}
    }

    if (cmd->opcode == 48 || cmd->opcode == 49) {
    	card_cmd = host->card->raw_scr[0] & 0xF;
    	if((card_cmd & 0x8) != 0x8 && (card_cmd & 0x4) != 0x4) {
    		cmd->error = -110;
    		goto done;
    	}
    }

    if (!(sdport->rtflags & RTKCR_FCARD_DETECTED)){
        cmd->error = -MMC_ERR_RMOVE;
        cmd->retries = 0;
        goto done;
    }

    if ( sdport && cmd ){
        rtk_crsd_allocate_dma_buf(sdport, cmd);
#ifdef CMD25_USE_SD_AUTOWRITE2
		if( force_check_previous_xfer_done ) {
			extern int rtk_crsd_wait_opt_end2(char* drv_name, struct rtk_crsd_host *sdport);
			rtk_crsd_wait_opt_end2(DRIVER_NAME,sdport);
			force_check_previous_xfer_done = 0;
		}
#endif
//MMC_DEBUG_PRINT("        [%s][%d] >>>> card 0x%08x\n", __FUNCTION__,__LINE__, host->card);
        rtk_crsd_send_command(sdport, cmd);
//MMC_DEBUG_PRINT("        [%s][%d] <<<<\n", __FUNCTION__,__LINE__);
    }else{
done:
//MMC_DEBUG_PRINT("        [%s][%d] >>>> card 0x%08x\n", __FUNCTION__,__LINE__, host->card);
        tasklet_schedule(&sdport->req_end_tasklet);
//MMC_DEBUG_PRINT("        [%s][%d] <<<<\n", __FUNCTION__,__LINE__);
    }
	up_write(&cr_rw_sem);
	//spin_unlock_irqrestore(&sdport->lock, flags);
}


static int rtk_crsd_wait_voltage_stable_low(struct rtk_crsd_host *sdport)
{
	volatile u8 stat;
	u8 i=0;

	while(1)
	{
		stat = crsd_readb(sdport->base_io+SD_BUS_STATUS);
		BluSDPRINTF("-stat %x \n",stat);
		if ( (stat & (SD_DAT3_0_LEVEL | SD_CMD_LEVEL)) == 0x0 )
			break;
		msleep(3);
		if(i++>100)
			break;
	}
	BluSDPRINTF("-stat %x break\n",stat);
	return 0;
}

static int rtk_crsd_wait_voltage_stable_high(struct rtk_crsd_host *sdport)
{
	volatile u8 stat;
	u8 i=0;
	while(1)
	{
		stat = crsd_readb(sdport->base_io+SD_BUS_STATUS);
		BluSDPRINTF(".stat %x \n",stat);
		if ( (stat & (SD_DAT3_0_LEVEL | SD_CMD_LEVEL)) == (SD_DAT3_0_LEVEL | SD_CMD_LEVEL) )
			break;
		msleep(10);
		if(i++>100)
			break;
	}
	BluSDPRINTF(".stat %x break\n",stat);
	return 0;
}

static int rtk_crsd_switch_voltage(struct mmc_host *mmc, struct mmc_ios *ios)
{
    struct rtk_crsd_host *sdport;
	int err = 0;
    sdport = mmc_priv(mmc);

    BluSDPRINTF( "reg CR_SOFT_RESET2 = %x\n",crsd_readl(sdport->base_pll+CR_SOFT_RESET2));
    BluSDPRINTF( "reg CARD_EXIST = %x\n",crsd_readb(sdport->base_io+CARD_EXIST));
    BluSDPRINTF( "reg CARD_INT_PEND = %x\n",crsd_readb(sdport->base_io+CARD_INT_PEND));

//	spin_lock_irqsave(&sdport->lock,flags);	
	if(ios->signal_voltage != MMC_SIGNAL_VOLTAGE_330)
	{
		//check DAT[3:0] are at LOW level
		err = rtk_crsd_wait_voltage_stable_low(sdport);
		if (err < 0)
			goto out;
		
		//host stop clk
	    crsd_writeb(0x3b, sdport->base_io+CARD_CLOCK_EN_CTL );     	
		mdelay(10);				//delay 5 ms to fit SD spec

		/* Workaround : keep IO pad voltage at 3.3v for SD card compatibility, jamestai20141225 */
		crsd_writel(0x0, sdport->base_io+CR_SD_PAD_CTL);
		rtk_crsd_sync(sdport);
		crsd_writel( 0x00E0003,  sdport->base_pll_io+CR_PLL_SD1);	
		mdelay(10);				//delay 5 ms to fit SD spec

		//force output clk
	    crsd_writeb( 0x3f, sdport->base_io+CARD_CLOCK_EN_CTL );
		//force a short period 1.8v clk
		crsd_writeb(0x80, sdport->base_io+SD_BUS_STATUS);
		rtk_crsd_sync(sdport);

		//check DAT[3:0] are at HIGH level
		err = rtk_crsd_wait_voltage_stable_high(sdport);
		if (err < 0)
			goto out;

	}

	//stop 1.8v clk
	crsd_writeb(0x0, sdport->base_io+SD_BUS_STATUS);	

    BluSDPRINTF( "reg CR_SOFT_RESET2 = %x\n",crsd_readl(sdport->base_pll+CR_SOFT_RESET2));
    BluSDPRINTF( "reg CARD_EXIST = %x\n",crsd_readb(sdport->base_io+CARD_EXIST));
    BluSDPRINTF( "reg CARD_INT_PEND = %x\n",crsd_readb(sdport->base_io+CARD_INT_PEND));
    BluSDPRINTF( "reg CR_PLL_SD1 0x%x= %x\n",sdport->base_pll_io+CR_PLL_SD1,crsd_readl(sdport->base_pll_io+CR_PLL_SD1));
    BluSDPRINTF( "reg CR_PLL_SD3 0x%x= %x\n",sdport->base_pll_io+CR_PLL_SD3,crsd_readl(sdport->base_pll_io+CR_PLL_SD3));
    BluSDPRINTF( "reg CR_SD_CKGEN_CTL 0x%x= %x\n",sdport->base_io+CR_SD_CKGEN_CTL,crsd_readl(sdport->base_io+CR_SD_CKGEN_CTL));
    BluSDPRINTF( "reg SD_CONFIGURE1 0x%x= %x\n",sdport->base_io+SD_CONFIGURE1,crsd_readb(sdport->base_io+SD_CONFIGURE1));

out:
//	spin_unlock_irqrestore(&sdport->lock, flags);
	return err;

}

static void rtk_crsd_set_ios(struct mmc_host *host, struct mmc_ios *ios)
{
    struct rtk_crsd_host *sdport;
    sdport = mmc_priv(host);

    down_write(&cr_rw_sem);

    RTKSDPRINTF("\n");
    if (ios->bus_mode == MMC_BUSMODE_PUSHPULL){
        RTKSDPRINTF("ios busmode = pushpull\n");
        if (ios->bus_width == MMC_BUS_WIDTH_8){
            RTKSDPRINTF("set bus width 8\n");
            rtk_crsd_set_bits(sdport,BUS_WIDTH_8);
        }else if (ios->bus_width == MMC_BUS_WIDTH_4){
            rtk_crsd_set_bits(sdport,BUS_WIDTH_4);
            RTKSDPRINTF("set bus width 4\n");
        }else{
            rtk_crsd_set_bits(sdport,BUS_WIDTH_1);
            RTKSDPRINTF("set bus width 1\n");
        }

        if (ios->clock >= UHS_SDR104_MAX_DTR && ios->timing == MMC_TIMING_UHS_SDR104) {
        	rtk_crsd_set_access_mode(sdport, ACCESS_MODE_SD30);
        	rtk_crsd_speed(sdport, CRSD_CLOCK_208000KHZ);
        	RTKSDPRINTF("Ultra high speed CRSD_CLOCK_208000KHZ\n");
        } else if (ios->clock >= UHS_SDR50_MAX_DTR && ios->timing == MMC_TIMING_UHS_SDR50) {
        	rtk_crsd_set_access_mode(sdport, ACCESS_MODE_SD30);
        	rtk_crsd_speed(sdport, CRSD_CLOCK_100000KHZ);
        	RTKSDPRINTF("Ultra high speed CRSD_CLOCK_100000KHZ\n");
        } else if (ios->clock >= UHS_SDR25_MAX_DTR && ios->timing == MMC_TIMING_UHS_SDR25) {
        	rtk_crsd_speed(sdport, CRSD_CLOCK_50000KHZ);
        	RTKSDPRINTF("high speed CRSD_CLOCK_50000KHZ\n");
        } else if (ios->clock >= UHS_SDR12_MAX_DTR && ios->timing == MMC_TIMING_UHS_SDR12) {
        	rtk_crsd_speed(sdport, CRSD_CLOCK_25000KHZ);
        	RTKSDPRINTF("high speed CRSD_CLOCK_25000KHZ\n");
        } else if (ios->clock >= UHS_DDR50_MAX_DTR && ios->timing == MMC_TIMING_UHS_DDR50) {
        	rtk_crsd_set_access_mode(sdport, ACCESS_MODE_DDR);
        	rtk_crsd_speed(sdport, CRSD_CLOCK_50000KHZ);
        	RTKSDPRINTF("high speed CRSD_CLOCK_50000KHZ\n");
        } else if(ios->clock >= 20000000){
        	rtk_crsd_speed(sdport, CRSD_CLOCK_20000KHZ);
        	RTKSDPRINTF("mid speed RTKCR_FCARD_SELECTED = 0 CRSD_CLOCK_20000KHZ\n");
        } else if (ios->clock > 200000) {
        	if (sdport->rtflags & RTKCR_FCARD_SELECTED) {
        		rtk_crsd_speed(sdport, CRSD_CLOCK_6200KHZ);
        		RTKSDPRINTF("mid speed RTKCR_FCARD_SELECTED = 1 CRSD_CLOCK_6200KHZ\n");
        	} else {
        		rtk_crsd_speed(sdport, CRSD_CLOCK_400KHZ);
        		RTKSDPRINTF("mid speed RTKCR_FCARD_SELECTED = 0 CRSD_CLOCK_400KHZ\n");
        	}
        } else {
        	if (sdport->rtflags & RTKCR_FCARD_SELECTED) {
        		rtk_crsd_speed(sdport, CRSD_CLOCK_6200KHZ);
        		RTKSDPRINTF("mid speed RTKCR_FCARD_SELECTED = 1 CRSD_CLOCK_6200KHZ\n");
        	} else {
        		rtk_crsd_speed(sdport, CRSD_CLOCK_200KHZ);
        		RTKSDPRINTF("low speed CRSD_CLOCK_200KHZ\n");
        	}
        }

    }else{  //MMC_BUSMODE_OPENDRAIN
        RTKSDPRINTF("ios busmode != pushpull  low speed CRSD_CLOCK_400KHZ\n");
		rtk_crsd_speed(sdport, CRSD_CLOCK_400KHZ);
        rtk_crsd_set_bits(sdport,BUS_WIDTH_1);
    }


	if (ios->power_mode == MMC_POWER_UP) {
		sdport->ops->card_power(sdport, 1);	//power on
        RTKSDPRINTF("power on\n");
	} else if (ios->power_mode == MMC_POWER_OFF) {
		sdport->ops->card_power(sdport, 0);	//power off
        RTKSDPRINTF("power off\n");
	}

    up_write(&cr_rw_sem);
}

static inline const char *sdmmc_dev(struct rtk_crsd_host *sdport)
{
	return dev_name(&(sdport->mmc->class_dev));
}

static u8 rtk_crsd_search_final_phase(struct rtk_crsd_host *sdport, u32 phase_map)
{
	struct timing_phase_path path[MAX_PHASE + 1];
	struct timing_phase_path swap;
	int i, j, cont_path_cnt;
	int k = 0;
	int new_block, max_len, final_path_idx;
	u8 final_phase = 0xFF;

	/* Parse phase_map, take it as a bit-ring */
	cont_path_cnt = 0;
	new_block = 1;
	j = 0;
	for (i = 0; i < MAX_PHASE + 1; i++) {
		if (phase_map & (1 << i)) {
			if (new_block) {
				new_block = 0;
				j = cont_path_cnt++;
				path[j].start = i;
				path[j].end = i;
			} else {
				path[j].end = i;
			}
		} else {
			new_block = 1;
			if (cont_path_cnt) {
				/* Calculate path length and middle point */
				int idx = cont_path_cnt - 1;
				path[idx].len =
					path[idx].end - path[idx].start + 1;
				path[idx].mid =
					path[idx].start + path[idx].len / 2;
			}
		}
	}

	if (cont_path_cnt == 0) {
		RTKSDPRINTF_WARM( " %s No continuous phase path\n",sdmmc_dev(sdport));
		goto finish;
	} else {
		/* Calculate last continuous path length and middle point */
		int idx = cont_path_cnt - 1;
		path[idx].len = path[idx].end - path[idx].start + 1;
		path[idx].mid = path[idx].start + path[idx].len / 2;
	}

	/* Connect the first and last continuous paths if they are adjacent */
	if (!path[0].start && (path[cont_path_cnt - 1].end == MAX_PHASE)) {
		/* Using negative index */
		path[0].start = path[cont_path_cnt - 1].start - MAX_PHASE - 1;
		path[0].len += path[cont_path_cnt - 1].len;
		path[0].mid = path[0].start + path[0].len / 2;
		/* Convert negative middle point index to positive one */
		if (path[0].mid < 0)
			path[0].mid += MAX_PHASE + 1;
		cont_path_cnt--;
	}

	/* Sorting path array,jamestai20141223 */
	for(k = 0; k < cont_path_cnt; ++k){
		for (i = 0; i < cont_path_cnt - 1 - k; ++i) {
			if(path[i].len < path[i+1].len){
				swap.end = path[i+1].end;
				swap.len = path[i+1].len;
				swap.mid = path[i+1].mid;
				swap.start = path[i+1].start;

				path[i+1].end = path[i].end;
				path[i+1].len = path[i].len;
				path[i+1].mid = path[i].mid;
				path[i+1].start = path[i].start;

				path[i].end = swap.end;
				path[i].len = swap.len;
				path[i].mid = swap.mid;
				path[i].start = swap.start;
			}
		}
	}

	/* Choose the longest continuous phase path */
	max_len = 0;
	final_phase = 0;
	final_path_idx = 0;
	for (i = 0; i < cont_path_cnt; i++) {
		if (path[i].len > max_len) {
			max_len = path[i].len;
			if(max_len>6)	//for compatibility issue, continue len should bigger than 6
				final_phase = (u8)path[i].mid;
			else
				final_phase = 0xFF;				
			final_path_idx = i;
		}

		RTKSDPRINTF_WARM( " %s path[%d].start = %d\n",sdmmc_dev(sdport), 
				i, path[i].start);
		RTKSDPRINTF_WARM( " %s path[%d].end = %d\n",sdmmc_dev(sdport), 
				i, path[i].end);
		RTKSDPRINTF_WARM( " %s path[%d].len = %d\n",sdmmc_dev(sdport), 
				i, path[i].len);
		RTKSDPRINTF_WARM( " %s path[%d].mid = %d\n",sdmmc_dev(sdport), 
				i, path[i].mid);
	}

finish:
	RTKSDPRINTF_WARM( " %s Final chosen phase: %d\n", sdmmc_dev(sdport), final_phase);
	return final_phase;
}

static int rtk_crsd_change_tx_phase(struct rtk_crsd_host *sdport, u8 sample_point)
{
	volatile unsigned int temp_reg;
	temp_reg = 	crsd_readl(sdport->base_pll_io+CR_PLL_SD1);	 
	temp_reg = (temp_reg & ~0xf8)|(sample_point<<3);
	crsd_writel( temp_reg,  sdport->base_pll_io+CR_PLL_SD1);	
    rtk_crsd_sync(sdport);	
	udelay(100);
	return 0;
}

static int rtk_crsd_change_rx_phase(struct rtk_crsd_host *sdport, u8 sample_point)
{
	volatile unsigned int temp_reg;
	temp_reg = 	crsd_readl(sdport->base_pll_io+CR_PLL_SD1);	
	temp_reg = (temp_reg & ~0x1f00)|(sample_point<<8);
	crsd_writel( temp_reg,  sdport->base_pll_io+CR_PLL_SD1);	
    rtk_crsd_sync(sdport);
	udelay(100);
	return 0;
}

static int rtk_crsd_tuning_tx_cmd(struct rtk_crsd_host *sdport,
		 u8 sample_point)
{
   	struct mmc_command cmd;
    struct crsd_cmd_pkt cmd_info;

	rtk_crsd_change_tx_phase(sdport, sample_point);
	memset(&cmd, 0, sizeof(struct mmc_command));
    memset(&cmd_info, 0, sizeof(struct crsd_cmd_pkt));
    cmd.opcode         = MMC_SEND_STATUS;
    cmd.arg            = (crsd_rca<<RCA_SHIFTER);
    cmd_info.cmd       = &cmd;
    cmd_info.sdport    = sdport;
    cmd_info.rsp_para2  = 0x41;
    cmd_info.rsp_len   = rtk_crsd_get_rsp_len(0x41);

    rtk_crsd_SendCMDGetRSP_Cmd(&cmd_info);
	rtk_crsd_dump_reg(sdport);

//	if(sdport->sd_trans & 0x10)
	if(crsd_readb(sdport->base_io+SD_TRANSFER) & 0x10) 
		return -1;
	return 0;

}

static int rtk_crsd_tuning_rx_cmd(struct rtk_crsd_host *sdport,
		 u8 sample_point,struct scatterlist* p_sg)
{
    struct crsd_cmd_pkt cmd_info;
	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	
	rtk_crsd_change_rx_phase(sdport, sample_point);
	mrq.cmd = &cmd;
	mrq.data = &data;
	mrq.cmd->data = mrq.data;
	mrq.data->error = 0;
	mrq.data->mrq = &mrq;

	cmd.opcode = MMC_SEND_TUNING_BLOCK;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

	data.blksz = 64;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.sg = p_sg;
	data.sg_len = 1;
	
	mrq.cmd->error = 0;
	mrq.cmd->mrq = &mrq;
	mrq.cmd->data = mrq.data;
	mrq.data->error = 0;
	mrq.data->mrq = &mrq;

    memset(&cmd_info, 0, sizeof(struct crsd_cmd_pkt));
	cmd_info.cmd    = mrq.cmd;
    cmd_info.sdport = sdport;
    cmd_info.rsp_para2 = rtk_crsd_get_rsp_type(cmd_info.cmd);
    cmd_info.rsp_len  = rtk_crsd_get_rsp_len(cmd_info.rsp_para2);
    cmd_info.data = mrq.cmd->data;

	rtk_crsd_Stream(&cmd_info);
 	rtk_crsd_dump_reg(sdport);
	
//	if(sdport->sd_status1 & 1)
	if(crsd_readb(sdport->base_io+SD_STATUS1) & 1) 
		return -1;
	return 0;
}

static int rtk_crsd_tuning_tx_phase(struct rtk_crsd_host *sdport,
		u8 sample_point)
{
	int err;
	err = rtk_crsd_tuning_tx_cmd(sdport, (u8)sample_point);
	return err;
}

static int rtk_crsd_tuning_rx_phase(struct rtk_crsd_host *sdport,
		u8 sample_point, struct scatterlist* p_sg)
{
	int err;
	err = rtk_crsd_tuning_rx_cmd(sdport, (u8)sample_point, p_sg);
	return err;
}


static int rtk_crsd_tuning_tx(struct rtk_crsd_host *sdport)
{
	int sample_point;
	int err=0, i;
	u32 raw_phase_map[TUNING_CNT] = {0}, phase_map;
	u8 final_phase;

	for (sample_point = 0; sample_point <= MAX_PHASE; sample_point++) 
	{
		for (i = 0; i < TUNING_CNT; i++) 
		{
		    if (!(sdport->rtflags & RTKCR_FCARD_DETECTED))
		    {
				RTKSDPRINTF_WARM( " line %d, MMC_ERR_RMOVE \n",__LINE__); 
				err = -MMC_ERR_RMOVE;
				goto out ;
		    }
			
			err = rtk_crsd_tuning_tx_phase(sdport, (u8)sample_point);
			if (0 == err)
				raw_phase_map[i] |= (1 << sample_point);
		}
	}

	phase_map = 0xFFFFFFFF;
	for (i = 0; i < TUNING_CNT; i++) {
		RTKSDPRINTF_WARM( " %s TX raw_phase_map[%d] = 0x%08x\n",sdmmc_dev(sdport), 
				i, raw_phase_map[i]);
		phase_map &= raw_phase_map[i];
	}
	RTKSDPRINTF_WARM(" %s TX phase_map = 0x%08x\n",sdmmc_dev(sdport),  phase_map);

	if (phase_map) {
		final_phase = rtk_crsd_search_final_phase(sdport, phase_map);
		RTKSDPRINTF_WARM(" %s final phase = 0x%08x\n",sdmmc_dev(sdport),  final_phase);
		if (final_phase == 0xFF)
		{
			RTKSDPRINTF_WARM(" %s final phase = 0x%08x\n",sdmmc_dev(sdport),  final_phase);
			err = -EINVAL;
			goto out ;
		}
		rtk_crsd_change_tx_phase(sdport, final_phase);
		err = 0;
		goto out ;
	} else {
		RTKSDPRINTF_WARM(" %s  fail !phase_map\n",sdmmc_dev(sdport));
		err = -EINVAL;
		goto out ;
	}

out:
	return err;
}


static int rtk_crsd_tuning_rx(struct rtk_crsd_host *sdport)
{
	int sample_point;
	int err=0, i;
	u32 raw_phase_map[TUNING_CNT] = {0}, phase_map;
	u8 final_phase;
	u8 *ssr;
	struct scatterlist sg;
	ssr = kmalloc(512, GFP_KERNEL);
	if (!ssr)
		return -ENOMEM;

	sg_init_one(&sg, ssr, 512);
	for (sample_point = 0; sample_point <= MAX_PHASE; sample_point++) 
	{
		for (i = 0; i < TUNING_CNT; i++) 
		{
		    if (!(sdport->rtflags & RTKCR_FCARD_DETECTED))
		    {
				RTKSDPRINTF_WARM( " line %d, MMC_ERR_RMOVE \n",__LINE__); 
				err = -MMC_ERR_RMOVE;
				goto out ;
		    }
			
			err = rtk_crsd_tuning_rx_phase(sdport, (u8)sample_point,&sg);
			if (0 == err)
				raw_phase_map[i] |= (1 << sample_point);
		}
	}

	phase_map = 0xFFFFFFFF;
	for (i = 0; i < TUNING_CNT; i++) {
		RTKSDPRINTF_WARM(" %s RX raw_phase_map[%d] = 0x%08x\n",sdmmc_dev(sdport), 
				i, raw_phase_map[i]);
		phase_map &= raw_phase_map[i];
	}
	RTKSDPRINTF_WARM(" %s RX phase_map = 0x%08x\n",sdmmc_dev(sdport),  phase_map);

	if (phase_map) {
		final_phase = rtk_crsd_search_final_phase(sdport, phase_map);
		RTKSDPRINTF_WARM(" %s final phase = 0x%08x\n",sdmmc_dev(sdport),  final_phase);
		if (final_phase == 0xFF)
		{	
			RTKSDPRINTF_WARM(" %s final phase = 0x%08x\n",sdmmc_dev(sdport),  final_phase);
			err = -EINVAL;
			goto out ;
		}
		rtk_crsd_change_rx_phase(sdport, final_phase);
		err = 0;
		goto out ;
	} else {
		RTKSDPRINTF_WARM(" %s  fail !phase_map\n",sdmmc_dev(sdport));
		err = -EINVAL;
		goto out ;
	}

out:
	kfree(ssr);
	return err;

}


static int rtk_crsd_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	struct rtk_crsd_host *sdport = mmc_priv(mmc);
	int err = 0;
	unsigned int reg_tmp=0;
	unsigned int reg_tmp2=0;
	unsigned int reg_tuned3318=0;

	BluSDPRINTF("\n");

 	rtk_crsd_dump_reg(sdport);

 	// disable spectrum
	reg_tmp2 = crsd_readl(sdport->base_pll_io+CR_PLL_SD2);
	printk(KERN_INFO "    --- %s PLL2 0x%08x\n",__FUNCTION__, reg_tmp2);
	crsd_writel( (reg_tmp2&0xFFFF1FFF),  sdport->base_pll_io+CR_PLL_SD2);	//PLL_SD2 clear [15:13]

 	/*if tune tx phase fail, down 8MHz and retry*/
	do{
		err = rtk_crsd_tuning_tx(sdport);
		if (err == -MMC_ERR_RMOVE)
		{
			RTKSDPRINTF_WARM("rtk_crsd_tuning_tx MMC_ERR_RMOVE\n");	
			return err;
		}else if (err){

			reg_tmp = crsd_readl(sdport->base_pll_io+CR_PLL_SD3);
			reg_tuned3318 = (reg_tmp & 0x3FF0000) >> 16;

			if (reg_tuned3318 <= 100 ){
				RTKSDPRINTF_WARM("rtk_crsd_tuning_tx fail\n");	
				return err;
			}

			crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) | 0x00070000, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to 4MHz by Hsin-yin
			reg_tmp = ((reg_tmp & (~0x3FF0000)) | ((reg_tuned3318 - 8) << 16)); //down 8MHz
			crsd_writel( reg_tmp, sdport->base_pll_io+CR_PLL_SD3);			
			udelay(2);
			crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) & 0xFFF8FFFF, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to normal clock source by Hsin-yin
			crsd_writeb( crsd_readb(sdport->base_io+SD_CONFIGURE1) & 0xEF, sdport->base_io+SD_CONFIGURE1 ); //Reset FIFO pointer by Hsin-yin
		}
	}while(err);

	if(err)
	{
		RTKSDPRINTF_WARM("rtk_crsd_tuning_tx fail\n");	
		return err;
	}

	err = rtk_crsd_tuning_rx(sdport);

	// enable spectrum
	crsd_writel( reg_tmp2,  sdport->base_pll_io+CR_PLL_SD2);	//PLL_SD2

	if(err)
	{
		BluSDPRINTF("rtk_crsd_tuning_rx fail\n");	
		return err;
	}

	printk(KERN_ERR "    %s PLL3 SD %d (0x%02x)\n",__FUNCTION__, (crsd_readl(sdport->base_pll_io+CR_PLL_SD3)>>16), (crsd_readl(sdport->base_pll_io+CR_PLL_SD3)>>16));
	printk(KERN_ERR "    %s CLK_GEN div %d\n",__FUNCTION__, 1<<(crsd_readl(sdport->base_io+CR_SD_CKGEN_CTL)&0x03));

	return err;
}

static void rtk_crsd_plug_timer(unsigned long data)
{
    struct rtk_crsd_host *sdport;
    u32 reginfo;
	volatile unsigned int reg_tmp;
    volatile unsigned long timeend=0;

    sdport = (struct rtk_crsd_host *)data;
    reginfo = crsd_readb(sdport->base_io+CARD_EXIST);

    if((reginfo & SD_EXISTENCE) ^
       (sdport->int_status_old & SD_EXISTENCE))
    {
        u32 det_time = 0;
        RTKSDPRINTF_WARM( "Card status change, reginfo=0x%x, int_status=0x%x\n",
                reginfo,sdport->int_status_old);

        sdport->rtflags     &= ~RTKCR_FCARD_DETECTED;
        sdport->wp          = 0;
        if(reginfo & SD_EXISTENCE)
		{
            sdport->ins_event = EVENT_INSER;
            sdport->rtflags |= RTKCR_FCARD_DETECTED;
            det_time = 1;
		    RedSDPRINTF( "SD card is exist, regCARD_EXIST = %x\n",crsd_readb(sdport->base_io+CARD_EXIST));
			crsd_writel(0x0, sdport->base_io+CR_DMA_CTL3);	//stop dma control
			crsd_writeb( 0x0, sdport->base_io+CR_CARD_STOP );            // SD Card module transfer no stop
		    crsd_writeb( 0x2, sdport->base_io+CARD_SELECT );            	//Specify the current active card module for the coming data transfer, bit 2:0 = 010
		    crsd_writeb( 0x4, sdport->base_io+CR_CARD_OE );            	//arget module is SD/MMC card module, bit 2 =1
	//	    crsd_writeb( 0x4, sdport->base_io+CR_IP_CARD_INT_EN );     	//SD/MMC Interrupt Enable, bit 2 = 1
		    crsd_writeb( 0x4, sdport->base_io+CARD_CLOCK_EN_CTL );     	// SD Card Module Clock Enable, bit 2 = 1
		    crsd_writeb( 0xD0, sdport->base_io+SD_CONFIGURE1 );
			rtk_crsd_speed(sdport, CRSD_CLOCK_400KHZ);
		    crsd_writeb( 0x0, sdport->base_io+SD_STATUS2 );
        }
		else
		{
            sdport->ins_event = EVENT_REMOV;
            sdport->rtflags &= ~RTKCR_FCARD_DETECTED;
            det_time = 1;

			/*crsd reset*/
		    timeend = jiffies + msecs_to_jiffies(100);

		    /* Check EMMC resource exist ? jamestai20150721*/
		    if(crsd_readl(sdport->base_pll_io + 0x01fc) == 0){
		    	while(time_before(jiffies, timeend))
				{
					if((crsd_readb(sdport->base_sdio+SDIO_NORML_INT_STA)&0x2));
						break;
					RTKSDPRINTF_DBG(".");
				}
		    }else{
				while(time_before(jiffies, timeend))
				{
					if((!(crsd_readl(sdport->base_emmc+EMMC_DMA_CTL3)&0x1)) && (crsd_readb(sdport->base_sdio+SDIO_NORML_INT_STA)&0x2));
						break;
					RTKSDPRINTF_DBG(".");
				}
		    }

			reg_tmp = crsd_readl(sdport->base_pll+CR_SOFT_RESET2);
			crsd_writel((reg_tmp & (~0x400)) , sdport->base_pll+CR_SOFT_RESET2 );     	// SD Card module transfer stop and idle state
			udelay(10);
			crsd_writel((reg_tmp | 0x400) , sdport->base_pll+CR_SOFT_RESET2 );     	// SD Card module transfer stop and idle state
			udelay(10);

			crsd_writel(0x0, sdport->base_io+CR_DMA_CTL3);	//stop dma control
			crsd_writeb( 0xff, sdport->base_io+CR_CARD_STOP );     	// SD Card module transfer stop and idle state
			crsd_writel( 0x003E0003,  sdport->base_pll_io+CR_PLL_SD1);	//PLL_SD1
			udelay(100);
			crsd_writel( 0x078D1893,  sdport->base_pll_io+CR_PLL_SD2);	//Reduce the impact of spectrum by Hsin-yin, jamestai20150302
			udelay(100);
			crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) | 0x00070000, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to 4MHz by Hsin-yin
			udelay(100);
			crsd_writel( 0x00564388,  sdport->base_pll_io+CR_PLL_SD3);	//PLL_SD3
			udelay(100);
			crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) & 0xFFF8FFFF, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to normal clock source by Hsin-yin
			udelay(100);
			crsd_writeb( crsd_readb(sdport->base_io+SD_CONFIGURE1) & 0xEF, sdport->base_io+SD_CONFIGURE1 ); //Reset FIFO pointer by Hsin-yin
			crsd_writel( 0x00000007,  sdport->base_pll_io+CR_PLL_SD4);	//PLL_SD4
			udelay(100);
			crsd_writeb(0xD0, sdport->base_io+SD_CONFIGURE1);
			rtk_crsd_speed(sdport, CRSD_CLOCK_400KHZ);
			crsd_writeb(0x0, sdport->base_io+CARD_SD_CLK_PAD_DRIVE);
			crsd_writeb(0x0, sdport->base_io+CARD_SD_CMD_PAD_DRIVE);
			crsd_writeb(0x0, sdport->base_io+CARD_SD_DAT_PAD_DRIVE);
			crsd_writel(0x0, sdport->base_io+CR_SD_PAD_CTL);	//change to 3.3v

        }
		rtk_crsd_sync(sdport);
		sdport->rtflags &= ~RTKCR_FCARD_SELECTED;
		crsd_rca = 0;
        mmc_detect_change(sdport->mmc, msecs_to_jiffies(det_time));
	    RedSDPRINTF( "sdport->mmc = 0x%x\n",(unsigned int)sdport->mmc);		
    }

    sdport->int_status_old = reginfo;
    mod_timer(&sdport->plug_timer, jiffies + HZ/10 );
}

static void rtk_crsd_timeout_timer(unsigned long data){

    struct rtk_crsd_host *sdport;

  	unsigned long flags = 0;

    sdport = (struct rtk_crsd_host *)data;

    RTKSDPRINTF( "rtk_crsd_timeout_timer fired ...\n");
    RTKSDPRINTF( "%s - int_wait=%08x\n", __func__, (unsigned int) (sdport->int_waiting));

    spin_lock_irqsave(&sdport->lock, flags);
    //down_write(&cr_rw_sem);
    
    sdport->int_status = readl(sdport->base_io + 0x24);
    sdport->sd_trans = readb(sdport->base_io + 0x193);
    sdport->sd_status1 = readb(sdport->base_io + 0x183);
    sdport->sd_status2 = readb(sdport->base_io + 0x184);
    sdport->bus_status = readb(sdport->base_io + 0x185);

    rtk_crsd_op_complete(sdport);
    spin_unlock_irqrestore(&sdport->lock, flags);
    //up_write(&cr_rw_sem);
}

#ifdef EMMC_SHOUTDOWN_PROTECT
static void rtk_crsd_gpio_isr(VENUS_GPIO_ID gid,
                                  unsigned char assert,
                                  void *dev)
{
    struct rtk_crsd_host *sdport = dev;
    u32 reg_tmp;
    u32 bit_tmp;
    u32 reginfo;
    unsigned char assert_tmp;

    RTKSDPRINTF( "%s(%u) sdport=0x%p (0x%x) [assert:%x]\n",
            __func__,__LINE__,sdport,gid,assert);

    reginfo = GET_PIN_TYPE(sdport->gpio_isr_info);
    bit_tmp = GET_PIN_INDEX(sdport->gpio_isr_info);

    reg_tmp = ((bit_tmp & 0xffffffe0)>>5)<<2;
    bit_tmp = (bit_tmp & 0x1fUL);

    switch(reginfo){
        case PCB_PIN_TYPE_GPIO:
            reg_tmp += GP0DATI;
            RTKSDPRINTF( "PIN type is PCB_PIN_TYPE_GPIO\n");
            break;
        case PCB_PIN_TYPE_ISO_GPIO:
            reg_tmp += ISO_GPDATI;
            RTKSDPRINTF( "PIN type is PCB_PIN_TYPE_ISO_GPIO\n");
            break;
        default:
            RTKSDPRINTF( "PIN group not match\n");
            WARN_ON(1);
            return;
    }

    reginfo = crsd_readl(reg_tmp);
    assert_tmp = (unsigned char)((reginfo & (0x01UL << bit_tmp))>>bit_tmp);
    WARN_ON(assert_tmp != assert);

    if((reginfo & (0x01UL <<bit_tmp)) == 0)
    {
        RTKSDPRINTF( "%s(%u)Hold eMMC RSENT!!!\n",
            __func__,__LINE__);

        rtk_crsd_hold_card(sdport);
    }
}

#endif

static irqreturn_t rtk_crsd_irq(int irq, void *dev){

    struct rtk_crsd_host *sdport = dev;
    int irq_handled = 0;
    u32 int_status = 0;
    u32 sd_trans = 0;;
    u32  dma_trans = 0;
    u32 sd_status1 = 0;
    u32 sd_status2 = 0;
    u32 bus_status = 0;

    rtk_crsd_sync(sdport);

    if(sdport->cmd_opcode == 2 || sdport->cmd_opcode == 9 || sdport->cmd_opcode == 51 || sdport->cmd_opcode == 6){
    	udelay(100);
    }else if(sdport->cmd_opcode == 12){
    	rtk_crsd_sync(sdport);
    	writel(0x16, sdport->base_io + 0x24);
    	return IRQ_HANDLED;
    }else if(sdport->cmd_opcode == 13 ){
    	do{
    		int_status = readl(sdport->base_io + 0x24);
    	}while(!(int_status & ISRSTA_INT1));
    }else if(sdport->cmd_opcode == 17 || sdport->cmd_opcode == 18 || sdport->cmd_opcode == 25 || sdport->cmd_opcode == 24){
		do{
			int_status = readl(sdport->base_io + 0x24);
		}while(!(int_status & ISRSTA_INT4) || !(int_status & ISRSTA_INT1));
    }

	int_status = readl(sdport->base_io + 0x24);
    sd_trans = readb(sdport->base_io + 0x193);
    sd_status1 = readb(sdport->base_io + 0x183);
    sd_status2 = readb(sdport->base_io + 0x184);
    bus_status = readb(sdport->base_io + 0x185);
    dma_trans =  readl(sdport->base_io + CR_DMA_CTL3);

	if (int_status & (ISRSTA_INT1 | ISRSTA_INT2 | ISRSTA_INT4)){

		sdport->int_status = int_status;
		sdport->sd_trans = sd_trans;
		sdport->sd_status1 = sd_status1;
		sdport->sd_status2 = sd_status2;
		sdport->bus_status = bus_status;
		sdport->dma_trans = dma_trans;

		if (sdport->int_waiting) {
			del_timer(&sdport->timer);
			rtk_crsd_op_complete(sdport);
		}else{
			printk("No int_waiting!!!\n");
		}
		irq_handled = 1;
	}else{
		printk("INT no END_STATE!!!\n");
	}

	rtk_crsd_sync(sdport);

	//up_write(&cr_rw_sem);
	//spin_unlock(&sdport->lock);
	writel(0x16, sdport->base_io + 0x24);
	if (irq_handled)
		return IRQ_HANDLED; //liao
	else
		return IRQ_NONE;
}

static int rtk_crsd_get_ro(struct mmc_host *host)
{
	struct rtk_crsd_host *sdport;
	u32 iobase;
	u32 reginfo;


	sdport = mmc_priv(host);
	down_write(&cr_rw_sem);
	iobase = sdport->base_io;
	reginfo = crsd_readb(iobase+CARD_EXIST);
	up_write(&cr_rw_sem);
	if(reginfo & SD_WRITE_PROTECT)
	{
		RTKSDPRINTF( "SD card is write protect, regCARD_EXIST = %x\n",crsd_readb(iobase+CARD_EXIST));
		return 1;                                                          
	}
	RTKSDPRINTF( "SD card is not write protect, regCARD_EXIST = %x\n",crsd_readb(iobase+CARD_EXIST));
	return 0;
}

static int rtk_crsd_get_cd(struct mmc_host *host)
{
    struct rtk_crsd_host *sdport;
    u32 iobase;
    u32 reginfo;


    sdport = mmc_priv(host);
    down_write(&cr_rw_sem);
    iobase = sdport->base_io;
    reginfo = crsd_readb(iobase+CARD_EXIST);
    up_write(&cr_rw_sem);
    if(reginfo & SD_EXISTENCE)
	{
	    RedSDPRINTF( "SD card is exist, regCARD_EXIST = %x\n",crsd_readb(iobase+CARD_EXIST));
	    return 1;
    }
    RedSDPRINTF( "SD card is not exist, regCARD_EXIST = %x\n",crsd_readb(iobase+CARD_EXIST));
    return 0;
}

#ifdef CMD25_WO_STOP_COMMAND
static void rtk_crsd_cmd12_timer(unsigned long param)
{
	if( sd_in_receive_data_state && rtksd_mmc_cmd && rtksd_sdport ) {
		sd_in_receive_data_state = 0;
		rtk_crsd_SendStopCMD( rtksd_mmc_cmd, rtksd_sdport);
	}		
}
#endif

static struct rtk_crsd_host_ops crsd_ops = {
    .func_irq       = NULL,
    .re_init_proc   = NULL,
    .card_det       = NULL,
    .card_power     = rtk_crsd_card_power,
	//.chk_card_insert= rtk_crsd_chk_card_insert,
	//.set_crt_muxpad = rtk_crsd_set_crt_muxpad,
	.set_clk        = NULL,
	//we don't need to do the rst
    .reset_card     = NULL,
    .reset_host     = NULL,
	//.bus_speed_down = rtk_crsd_bus_speed_down,
    .get_cmdcode    = NULL, //rtk_get_emmc_cmdcode,
    .get_r1_type    = NULL, //rtk_emmc_get_r1_type,
    .chk_cmdcode    = rtk_crsd_chk_cmdcode,
	//.chk_r1_type    = rtk_crsd_chk_r1_type,
};

static int rtk_crsd_probe(struct platform_device *pdev)
{
    struct mmc_host *mmc = NULL;
    struct rtk_crsd_host *sdport = NULL;
    int ret, irq;
    int bErrorRetry_1=0;

#ifdef EMMC_SHOUTDOWN_PROTECT
    u64 rtk_tmp_gpio;
#endif

	rtk119x_crsd_node = pdev->dev.of_node;

	if (!rtk119x_crsd_node)
		panic("No irda of node found");

	/* Request IRQ */
	irq = irq_of_parse_and_map(rtk119x_crsd_node, 0);
	if (!irq) {
		printk(KERN_ERR "RTK CRSD: fail to parse of irq.\n");
		return -ENXIO;
	}

    RTKSDPRINTF( "%s : IRQ = %x\n",DRIVER_NAME, irq);

    if (irq < 0){
        return -ENXIO;
    }

    mmc = mmc_alloc_host(sizeof(struct rtk_crsd_host), &pdev->dev);

    if (!mmc) {
        ret = -ENOMEM;
        goto out;
    }
    mmc_sd_host_local = mmc;

    sdport = mmc_priv(mmc);
    memset(sdport, 0, sizeof(struct rtk_crsd_host));


///////mmap io
	sdport->base_pll = of_iomap(rtk119x_crsd_node, 0);
	if (!sdport->base_pll )
		panic("Can't map sdport->base_pll for %s", rtk119x_crsd_node->name);

	sdport->base_crsd = of_iomap(rtk119x_crsd_node, 1);
	if (!sdport->base_crsd)
		panic("Can't map sdport->base for %s", rtk119x_crsd_node->name);

	sdport->base_sysbrdg = of_iomap(rtk119x_crsd_node, 2);
	if (!sdport->base_sysbrdg)
		panic("Can't map sdport->base_sysbrdg for %s", rtk119x_crsd_node->name);

	sdport->base_emmc = of_iomap(rtk119x_crsd_node, 3);
	if (!sdport->base_emmc)
		panic("Can't map sdport->base_emmc for %s", rtk119x_crsd_node->name);

	sdport->base_sdio = of_iomap(rtk119x_crsd_node, 4);
	if (!sdport->base_sdio)
		panic("Can't map sdport->base_sdio for %s", rtk119x_crsd_node->name);


	RTKSDPRINTF(" sdport->base_pll = %x \n", (u32)sdport->base_pll);	 
	RTKSDPRINTF(" sdport->base_crsd = %x \n",(u32)sdport->base_crsd);	 
	RTKSDPRINTF(" sdport->base_sysbrdg = %x \n", (u32)sdport->base_sysbrdg);	 
	RTKSDPRINTF(" sdport->base_emmc = %x \n", (u32)sdport->base_emmc);	 
	RTKSDPRINTF(" sdport->base_sdio = %x \n", (u32)sdport->base_sdio);	 

	sdport->gpio_card_power = of_get_gpio_flags(rtk119x_crsd_node, 0, NULL);

	if (gpio_is_valid(sdport->gpio_card_power)) {
		ret = gpio_request(sdport->gpio_card_power, "crsd_power");
		if (ret < 0) {
			panic("can't request crsd_power gpio %d\n",sdport->gpio_card_power);
		}
	}else{
		panic("crsd_power gpio %d is not valid\n",sdport->gpio_card_power);
	}


    sdport->mmc = mmc;
    sdport->dev = &pdev->dev;
    sdport->base_io = (u32)(sdport->base_crsd);
    sdport->base_pll_io = (u32)(sdport->base_pll);
    sdport->base_sysbrdg_io = (u32)(sdport->base_sysbrdg);
    sdport->base_emmc_io = (u32)(sdport->base_emmc);
    sdport->base_sdio_io = (u32)(sdport->base_sdio);
    sdport->ops = &crsd_ops;

    /*SD PLL Initialization, jamestai20150721*/
    crsd_writel(0x00000004, sdport->base_pll_io+CR_PLL_SD4);
    crsd_writel(0x00000007, sdport->base_pll_io+CR_PLL_SD4);

    sema_init(&sdport->sem,1);
    sema_init(&sdport->sem_op_end,1);

    sdport->magic_num = (readl(sdport->base_sysbrdg_io + 0x204) >> 16);

    mmc->ocr_avail = MMC_VDD_30_31 | MMC_VDD_31_32 |
                     MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;

    mmc->caps = MMC_CAP_4_BIT_DATA
              | MMC_CAP_SD_HIGHSPEED
			  |	MMC_CAP_HW_RESET
              | MMC_CAP_UHS_SDR104
              | MMC_CAP_UHS_SDR12
              | MMC_CAP_UHS_SDR25
              | MMC_CAP_UHS_SDR50
              | MMC_CAP_UHS_DDR50
//              | MMC_CAP_1_8V_DDR;
              | MMC_CAP_MMC_HIGHSPEED;
//            | MMC_CAP_NONREMOVABLE;

    if(rtk_crsd_bus_wid == 4 || rtk_crsd_bus_wid == 5){
        mmc->caps &= ~MMC_CAP_8_BIT_DATA;
    }

    mmc->f_min = 10000000>>8;   /* RTK min bus clk is 10Mhz/256 */
    mmc->f_max = 208000000;      /* RTK max bus clk is 208Mhz */

    mmc->max_segs = 1;
    mmc->max_blk_size   = 512;

    if(rtk_crsd_chk_VerA())
        mmc->max_blk_count  = 0x100;
    else
        mmc->max_blk_count  = 0x400;

    mmc->max_seg_size   = mmc->max_blk_size * mmc->max_blk_count;
    mmc->max_req_size   = mmc->max_blk_size * mmc->max_blk_count;

    spin_lock_init(&sdport->lock);
    init_rwsem(&cr_rw_sem);
    tasklet_init(&sdport->req_end_tasklet, rtk_crsd_req_end_tasklet,
		        (unsigned long)sdport);

    RTKSDPRINTF( "\n");

    if ((!sdport->base_crsd)||(!sdport->base_pll)||(!sdport->base_sysbrdg)||(!sdport->base_emmc)||(!sdport->base_sdio))
	{
        RTKSDPRINTF( "---- Realtek EMMC Controller Driver probe fail - nomem ----\n\n");
        ret = -ENOMEM;
        goto out;
    }

#ifdef ENABLE_SD_INT_MODE
    ret = request_irq(irq, rtk_crsd_irq, IRQF_SHARED, DRIVER_NAME, sdport);   //rtk_crsd_interrupt
    if (ret){
        RTKSDPRINTF( "%s: cannot assign irq %d\n", DRIVER_NAME, irq);
        goto out;
    }else{
        sdport->irq = irq;
    }
#endif

  	setup_timer(&sdport->timer, rtk_crsd_timeout_timer, (unsigned long)sdport);
	setup_timer(&sdport->plug_timer, rtk_crsd_plug_timer, (unsigned long)sdport);
#ifdef CMD25_WO_STOP_COMMAND
	setup_timer(&rtksd_stop_cmd_timer, rtk_crsd_cmd12_timer, (unsigned long)NULL);
#endif

    if (sdport->ops->reset_card)
        sdport->ops->reset_card(sdport);

    crsd_writel( 0x003E0003,  sdport->base_pll_io+CR_PLL_SD1);	//PLL_SD1
    udelay(100);
    crsd_writel( 0x078D1893,  sdport->base_pll_io+CR_PLL_SD2);	//Reduce the impact of spectrum by Hsin-yin, jamestai20150302
    udelay(100);
    crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) | 0x00070000, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to 4MHz by Hsin-yin
    udelay(100);
    crsd_writel( 0x00564388,  sdport->base_pll_io+CR_PLL_SD3);	//PLL_SD3
	udelay(100);
	crsd_writel(crsd_readl( sdport->base_io + CR_SD_CKGEN_CTL) & 0xFFF8FFFF, sdport->base_io + CR_SD_CKGEN_CTL); //Switch SD source clock to normal clock source by Hsin-yin
	udelay(100);
	crsd_writeb( crsd_readb(sdport->base_io+SD_CONFIGURE1) & 0xEF, sdport->base_io+SD_CONFIGURE1 ); //Reset FIFO pointer by Hsin-yin
	crsd_writel( 0x00000007,  sdport->base_pll_io+CR_PLL_SD4);	//PLL_SD4
	udelay(100);
	crsd_writeb(0xD0, sdport->base_io+SD_CONFIGURE1);
	rtk_crsd_speed(sdport, CRSD_CLOCK_400KHZ);

	crsd_writeb(0x0, sdport->base_io+CARD_SD_CLK_PAD_DRIVE);
	crsd_writeb(0x0, sdport->base_io+CARD_SD_CMD_PAD_DRIVE);
	crsd_writeb(0x0, sdport->base_io+CARD_SD_DAT_PAD_DRIVE);
	crsd_writel(0x0, sdport->base_io+CR_SD_PAD_CTL);	//change to 3.3v


//	host->card_type_pre = CR_EM;  //default flow is sd
    mmc->card_type_pre = CR_SD;
    mmc->ops = &rtk_crsd_ops;
    sdport->rtflags     &= ~(RTKCR_FCARD_DETECTED|RTKCR_FCARD_SELECTED);
	sdport->int_status_old &=  ~SD_EXISTENCE;
	crsd_rca = 0;


#ifdef ENABLE_SD_INT_MODE
	writel(0x16 ,sdport->base_crsd + 0x24);
	writel(0x17 ,sdport->base_crsd + 0x28);
#endif

    platform_set_drvdata(pdev, mmc);

    crsd_writeb( 0x2, sdport->base_io+CARD_SELECT );            //for emmc, select SD ip
    if (bErrorRetry_1)
    {
        crsd_writeb( 0x8,  sdport->base_io+SD_SAMPLE_POINT_CTL );    //sample point = SDCLK / 4
        crsd_writeb( 0x10, sdport->base_io+SD_PUSH_POINT_CTL );     //output ahead SDCLK /4
    }
    else
    {
        crsd_writeb( 0x0, sdport->base_io+SD_SAMPLE_POINT_CTL );    //sample point = SDCLK / 4
        crsd_writeb( 0x0, sdport->base_io+SD_PUSH_POINT_CTL );     //output ahead SDCLK /4
    }
    g_crinit=0;

    rtk_crsd_sync(sdport);

    RTKSDPRINTF("%s: %s driver initialized\n",
               mmc_hostname(mmc), DRIVER_NAME);

    ret = mmc_add_host(mmc);
    if (ret)
        goto out;


	rtk_crsd_mdelay(500);
   	mod_timer(&sdport->plug_timer, jiffies + 3*HZ );
	rtk_crsd_mdelay(2000);


    return 0;

out:
    if (sdport) {
        if (sdport->irq)
            free_irq(sdport->irq, sdport);

        if (sdport->base_crsd)
            iounmap(sdport->base_crsd);

        if (sdport->base_pll)
            iounmap(sdport->base_pll);

	    if (sdport->base_sysbrdg)
            iounmap(sdport->base_sysbrdg);

	    if (sdport->base_emmc)
            iounmap(sdport->base_emmc);

	    if (sdport->base_sdio)
            iounmap(sdport->base_sdio);


    }
    if (mmc)
        mmc_free_host(mmc);
    return ret;
}

static int __exit rtk_crsd_remove(struct platform_device *pdev)
{
    struct mmc_host *mmc = platform_get_drvdata(pdev);
    RTKSDPRINTF( "\n");

    if (mmc) {
        struct rtk_crsd_host *sdport = mmc_priv(mmc);

        flush_scheduled_work();

        rtk_crsd_free_dma_buf(sdport);

        mmc_remove_host(mmc);
        if(!mmc){
            RTKSDPRINTF( "eMMC host have removed.\n");
            mmc_sd_host_local = NULL;
        }
        free_irq(sdport->irq, sdport);

      	del_timer_sync(&sdport->timer);
        del_timer_sync(&sdport->plug_timer);
#ifdef CMD25_WO_STOP_COMMAND
        del_timer_sync(&rtksd_stop_cmd_timer);
#endif
        if (sdport->base_crsd)
            iounmap(sdport->base_crsd);

        if (sdport->base_pll)
            iounmap(sdport->base_pll);

        mmc_free_host(mmc);
        gpio_free(sdport->gpio_card_power);
    }
    platform_set_drvdata(pdev, NULL);

    return 0;
}


/*****************************************************************************************/
/* driver / device attache area                                                                                                               */
/*****************************************************************************************/
static int rtk_crsd_pm_suspend(struct device *dev) 
{
	int ret = 0;
	struct mmc_host *mmc = dev_get_drvdata(dev);

	printk(KERN_INFO "rtk_crsd_pm_suspend\n");

	ret = mmc_suspend_host(mmc);

	return ret;
}

static int rtk_crsd_pm_resume(struct device *dev) 
{
	int ret = 0;
	struct mmc_host *mmc = dev_get_drvdata(dev);

	printk(KERN_INFO "rtk_crsd_pm_resume\n");

	ret = mmc_resume_host(mmc);

	return ret;
}

static const struct dev_pm_ops rtk_crsd_pm_ops = {
    .suspend    = rtk_crsd_pm_suspend,
    .resume     = rtk_crsd_pm_resume,
};

static const struct of_device_id crsd_rtk_dt_match[] = {
	{ .compatible = "Realtek,rtk119x-crsd" },
	{},
};

static struct platform_driver rtk_crsd_driver = {
    .probe      = rtk_crsd_probe,
    .remove     = __exit_p(rtk_crsd_remove),
    .driver     =
    {
            .name   = "rtksd",
            .owner  = THIS_MODULE,
			.of_match_table = crsd_rtk_dt_match,
			.pm     = &rtk_crsd_pm_ops,
    },
};

static void rtk_crsd_display_version (void)
{
    const __u8 *revision;
    const __u8 *date;
    const __u8 *time;
    char *running = (__u8 *)VERSION;
    RTKSDPRINTF( "\n");
    strsep(&running, " ");
    strsep(&running, " ");
    revision = strsep(&running, " ");
    date = strsep(&running, " ");
    time = strsep(&running, " ");
    printk(KERN_INFO "Rev:%s (%s %s)\n", revision, date, time);
    printk(KERN_INFO "%s: build at %s %s\n", DRIVER_NAME, __DATE__, __TIME__);

#ifdef CONFIG_MMC_BLOCK_BOUNCE
    RTKSDPRINTF( "%s: CONFIG_MMC_BLOCK_BOUNCE enable\n",DRIVER_NAME);
#else
    RTKSDPRINTF( "%s: CONFIG_MMC_BLOCK_BOUNCE disable\n",DRIVER_NAME);
#endif

#ifdef CONFIG_SMP
    RTKSDPRINTF( "%s: ##### CONFIG_SMP alert!! #####\n",DRIVER_NAME);
#else
    RTKSDPRINTF( "%s: ##### CONFIG_SMP disable!! #####\n",DRIVER_NAME);
#endif

}


static void rtk_crsd_dump_reg(struct rtk_crsd_host *sdport)
{
	unsigned int reg_tmp;

    RTKSDPRINTF_WARM( "CR_PLL_SD1 = 0x%x, CR_PLL_SD2 = 0x%x, CR_PLL_SD3 = 0x%x, CR_PLL_SD4 = 0x%x, CR_PFUNC_CR = 0x%x,\n", \
		crsd_readl( sdport->base_pll_io+CR_PLL_SD1),	\
		crsd_readl( sdport->base_pll_io+CR_PLL_SD2),	\
		crsd_readl( sdport->base_pll_io+CR_PLL_SD3),	\
		crsd_readl( sdport->base_pll_io+CR_PLL_SD4),	\
		crsd_readl( sdport->base_pll_io+CR_PFUNC_CR)	\
		);

	reg_tmp	= 	sdport->base_io;
	for (;reg_tmp < 0xFE0104b0;)
	{
	    REG_SDPRINTF("0x%x = 0x%x,  0x%x,  0x%x,  0x%x \n", \
			reg_tmp,	\
			crsd_readl( reg_tmp),	\
			crsd_readl( reg_tmp+4),	\
			crsd_readl( reg_tmp+8),	\
			crsd_readl( reg_tmp+12)	\
			);
		reg_tmp += 16;
	}

	reg_tmp	= 	sdport->base_io + 0x100;
	for (;reg_tmp < 0xFE0105bf;)
	{
	    REG_SDPRINTF("0x%x = 0x%x,  0x%x,  0x%x,  0x%x \n",\
			reg_tmp,\
			crsd_readb( reg_tmp),\
			crsd_readb( reg_tmp+1),\
			crsd_readb( reg_tmp+2),\
			crsd_readb( reg_tmp+3)\
			);
		reg_tmp += 4;
	}

}



static int rtk_crsd_set_bus_width(char * buf){
    /*
    get eMMC bus width setting by bootcode parameter, like below
    bootargs=console=ttyS0,115200 earlyRTKSDPRINTF emmc_bus=8
    the keyword is "emmc_bus"
    the getted parameter is hex.
    example:
        emmc_bus=8
    */
    RTKSDPRINTF( "\n");
    rtk_crsd_chk_param(&rtk_crsd_bus_wid,1,buf+1);
    RTKSDPRINTF( "%s: setting bus width is %u-bit\n",
                DRIVER_NAME,rtk_crsd_bus_wid);
    return 0;
}

static int __init rtk_crsd_init(void)
{
    int rc = 0;

    RTKSDPRINTF( "\n");

	mdelay(10);
   	rtk_crsd_display_version();

#ifdef CONFIG_ANDROID
   	RTKSDPRINTF( "%s: Android timming setting\n",DRIVER_NAME);
#endif

	rc = platform_driver_register(&rtk_crsd_driver);
    if (rc < 0){
        RTKSDPRINTF( "Realtek SD Controller Driver installation fails.\n\n");
        return -ENODEV;
    }else{
#ifdef ENABLE_SD_INT_MODE
    	RTKSDPRINTF( "Realtek SD Controller Driver is successfully installing.\n\n");
        RTKSDPRINTF( "Realtek SD Controller Driver is running interrupt mode.\n\n");
#endif
        RTKSDPRINTF( "Realtek SD Controller Driver is successfully installing.\n\n");
        return 0;
    }

    return rc;
}

static void __exit rtk_crsd_exit(void)
{
    RTKSDPRINTF( "\n");
    platform_driver_unregister(&rtk_crsd_driver);
}

// allow emmc driver initialization earlier
module_init(rtk_crsd_init);
//late_init_call(rtk_crsd_init);
//device_initcall_sync(rtk_crsd_init);
module_exit(rtk_crsd_exit);


/* maximum card clock frequency (default 50MHz) */
module_param(maxfreq, int, 0);

/* force PIO transfers all the time */
module_param(nodma, int, 0);

MODULE_AUTHOR("Elbereth");
MODULE_DESCRIPTION("Realtek Sd Host Controller driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:rtksd");

__setup("sd_bus",rtk_crsd_set_bus_width);



