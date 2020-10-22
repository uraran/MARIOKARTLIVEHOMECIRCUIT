/*
 * Realtek MMC/SD/SDIO driver
 *
 * Authors:
 * Copyright (C) 2008-2013 Realtek Ltd.
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
#include <linux/semaphore.h>           //liao
#include <linux/mmc/card.h>             //liao
#include <linux/mmc/mmc.h>              //liao
#include <linux/mmc/sd.h>               //liao
#include <linux/workqueue.h>            //liao
#include <linux/completion.h>           //liao
#include <rbus/reg_mmc.h>               //liao
#include <rbus/reg_iso.h>               //liao
#include "rtksd.h"                    //liao
#include "rtk_crsd_ops.h"                    //liao
#include "../mmc_debug.h"               //liao

//static unsigned char g_cmd[6]={0};

#ifdef GPIO_DEBUG
void trans_db_gpio(void)
{
    //set debug GPIO
    u32 reginfo;
    RTKSDPRINTF( "\n");
	
    //1. 0xB800_0804[31:28] = F    --> pin share as gpio
    //2. 0x1801_BC00[4] = 1  --> output mode
    //3. 0x1801_BC18[4]   is output data
    crsd_writel(crsd_readl(GP0DIR_reg)|0x10,GP0DIR_reg);

    reginfo = crsd_readl(GP0DATO_reg);
    if(reginfo & 0x10){
        mmcrtk("GP HI\n");
        crsd_writel(reginfo & ~0x10,GP0DATO_reg);
    }else{
        mmcrtk("GP LO\n");
        crsd_writel(reginfo | 0x10,GP0DATO_reg);
    }

}

void trans_rst_gpio(void)
{
    //set rst GPIO
    u32 reginfo;
    RTKSDPRINTF( "\n");
	
    crsd_writel(crsd_readl(GP0DIR_reg) |  0x00100000, GP0DIR_reg);
    reginfo = crsd_readl(GP0DATO_reg);

    if(reginfo & 0x00100000){
        crsd_writel(reginfo & ~0x00100000, GP0DATO_reg);
    }else{
        crsd_writel(reginfo |  0x00100000, GP0DATO_reg);
    }

}
#else
#define trans_db_gpio()
#define trans_rst_gpio()
#endif

void rtk_crsd_sync(struct rtk_crsd_host *sdport){
    crsd_writel(0x0, sdport->base_sysbrdg_io+CR_SYNC);
}

EXPORT_SYMBOL_GPL(rtk_crsd_sync);

DECLARE_COMPLETION(rtk_crsd_wait);
void rtk_crsd_int_waitfor(struct rtk_crsd_host *sdport, u8 cmdcode, unsigned long msec,unsigned long dma_msec){

	rtk_crsd_sync(sdport);

    if (sdport->int_waiting){

        rtk_crsd_sync(sdport);

        crsd_writeb((u8) (cmdcode|START_EN), sdport->base_io+SD_TRANSFER ); //cmd fire
        rtk_crsd_sync(sdport);

        wait_for_completion(sdport->int_waiting);
        //rtk_crsd_sync(sdport);    //jim add code
    }

}
EXPORT_SYMBOL_GPL(rtk_crsd_int_waitfor);

void rtk_crsd_int_enable_and_waitfor(struct rtk_crsd_host *sdport, u8 cmdcode, unsigned long msec, unsigned long dma_msec){

    unsigned long timeend=0;

    sdport->int_status  = 0;
    sdport->sd_trans    = -1;
    sdport->sd_status1   = 0;
    sdport->sd_status2   = 0;
    sdport->bus_status   = 0;
    sdport->dma_trans   = 0;

    sdport->int_waiting = &rtk_crsd_wait;

    /* timeout timer fire */
    if (&sdport->timer){
        timeend = msecs_to_jiffies(msec)+sdport->tmout;
        mod_timer(&sdport->timer, (jiffies + timeend) );
    }

    rtk_crsd_int_waitfor(sdport, cmdcode, msec, dma_msec); //wait for
    smp_wmb();

}
EXPORT_SYMBOL_GPL(rtk_crsd_int_enable_and_waitfor);

int rtk_crsd_int_wait_opt_end(char* drv_name, struct rtk_crsd_host *sdport,u8 cmdcode,u8 cpu_mode){

    volatile int err=CR_TRANS_OK;
    volatile unsigned long timeend = 0;
    volatile unsigned int sd_trans = 0;
    
    unsigned long timeout = 0;
    unsigned long old_jiffles = jiffies;

    err = CR_TRANSFER_TO;

    rtk_crsd_sync(sdport);

    rtk_crsd_int_enable_and_waitfor(sdport, cmdcode, 500, 0);

    sd_trans = readb(sdport->base_io + SD_TRANSFER);

    rtk_crsd_sync(sdport);

	if ((sd_trans & (END_STATE | IDLE_STATE)) == (END_STATE | IDLE_STATE)) {
		err = CR_TRANS_OK;
	}
        //add this section to fix the kingston long response
        else {
            timeout=600;
            while(time_before(jiffies, old_jiffles + timeout)) {
                rtk_crsd_sync(sdport);
                sd_trans = readb(sdport->base_io + SD_TRANSFER);
                if ((sd_trans & (END_STATE | IDLE_STATE)) == (END_STATE | IDLE_STATE)) {
                    err = CR_TRANS_OK;
                    break;
                }

		else if ((sd_trans & (ERR_STATUS)) == (ERR_STATUS)) {

			err = CR_TRANSFER_FAIL;
			printk( KERN_ERR "%s - trans error 5 :\ncfg1 : 0x%02x, cfg2 : 0x%02x, cfg3 : 0x%02x, trans : 0x%08x, st1 : 0x%08x, st2 : 0x%08x, bus : 0x%08x\n",
				drv_name,
				crsd_readb(sdport->base_io + SD_CONFIGURE1),
				crsd_readb(sdport->base_io + SD_CONFIGURE2),
				crsd_readb(sdport->base_io + SD_CONFIGURE3),
				crsd_readb(sdport->base_io + SD_TRANSFER),
				crsd_readb(sdport->base_io + SD_STATUS1),
				crsd_readb(sdport->base_io + SD_STATUS2),
				crsd_readb(sdport->base_io + SD_BUS_STATUS));
                         break;
		}
                else if(!(sdport->rtflags & RTKCR_FCARD_DETECTED)){
                    err = CR_TRANSFER_FAIL;
                    break;
                }
            }
	}

	if (err == CR_TRANSFER_TO){

		err = CR_TRANSFER_TO;

		if ((sd_trans & (END_STATE | IDLE_STATE)) == (END_STATE | IDLE_STATE)) {
			err = CR_TRANS_OK;
		}else{
			printk( KERN_ERR "\n%s - trans error 3 :\ntrans : 0x%08x, st1 : 0x%08x, st2 : 0x%08x, bus : 0x%08x\n",
					drv_name,
					crsd_readb(sdport->base_io + SD_TRANSFER),
					crsd_readb(sdport->base_io + SD_STATUS1),
					crsd_readb(sdport->base_io + SD_STATUS2),
					crsd_readb(sdport->base_io + SD_BUS_STATUS));
			printk(KERN_ERR "CR_TRANSFER_TO : cmd->opcode = %d\n", sdport->mrq->cmd->opcode);

		}

		return err;
	}


    RTKSDPRINTF("%s : register settings(base=0x%08x,0x%08x)\n", drv_name, sdport->base_io, sdport->base_io+SD_CMD0);
    RTKSDPRINTF(" cmd0:0x%02x cmd1:0x%02x cmd2:0x%02x cmd3:0x%02x cmd4:0x%02x cmd5:0x%02x\n",crsd_readb(sdport->base_io+SD_CMD0),crsd_readb(sdport->base_io+SD_CMD1),crsd_readb(sdport->base_io+SD_CMD2),crsd_readb(sdport->base_io+SD_CMD3),crsd_readb(sdport->base_io+SD_CMD4),crsd_readb(sdport->base_io+SD_CMD5));
    RTKSDPRINTF(" trans:0x%02x status1:0x%02x status2:0x%02x bus_status:0x%02x\n",crsd_readb(sdport->base_io+SD_TRANSFER),crsd_readb(sdport->base_io+SD_STATUS1),crsd_readb(sdport->base_io+SD_STATUS2),crsd_readb(sdport->base_io+SD_BUS_STATUS));
    RTKSDPRINTF(" configure1:0x%02x configure2:0x%02x configure3:0x%02x\n",crsd_readb(sdport->base_io+SD_CONFIGURE1),crsd_readb(sdport->base_io+SD_CONFIGURE2),crsd_readb(sdport->base_io+SD_CONFIGURE3));
    RTKSDPRINTF(" byteH:0x%02x byteL:0x%02x blkH:0x%02x blkL:0x%02x\n",crsd_readb(sdport->base_io+SD_BYTE_CNT_H),crsd_readb(sdport->base_io+SD_BYTE_CNT_L),crsd_readb(sdport->base_io+SD_BLOCK_CNT_H),crsd_readb(sdport->base_io+SD_BLOCK_CNT_L));
    RTKSDPRINTF(" dma_ctl1:0x%08x dma_ctl2:0x%08x dma_ctl3:0x%08x\n",crsd_readl(sdport->base_io+CR_DMA_CTL1),crsd_readl(sdport->base_io+CR_DMA_CTL2),crsd_readl(sdport->base_io+CR_DMA_CTL3));

    return err;
}
EXPORT_SYMBOL_GPL(rtk_crsd_int_wait_opt_end);

int rtk_crsd_wait_opt_end(char* drv_name, struct rtk_crsd_host *sdport,u8 cmdcode,u8 cpu_mode){

    volatile int err=CR_TRANS_OK;
    volatile unsigned long timeend = 0;
    rtk_crsd_sync(sdport);

    RTKSDPRINTF("%s : register settings(base=0x%08x,0x%08x)\n", drv_name, sdport->base_io, sdport->base_io+SD_CMD0);
    RTKSDPRINTF(" cmd0:0x%02x cmd1:0x%02x cmd2:0x%02x cmd3:0x%02x cmd4:0x%02x cmd5:0x%02x\n",crsd_readb(sdport->base_io+SD_CMD0),crsd_readb(sdport->base_io+SD_CMD1),crsd_readb(sdport->base_io+SD_CMD2),crsd_readb(sdport->base_io+SD_CMD3),crsd_readb(sdport->base_io+SD_CMD4),crsd_readb(sdport->base_io+SD_CMD5));
    RTKSDPRINTF(" trans:0x%02x status1:0x%02x status2:0x%02x bus_status:0x%02x\n",crsd_readb(sdport->base_io+SD_TRANSFER),crsd_readb(sdport->base_io+SD_STATUS1),crsd_readb(sdport->base_io+SD_STATUS2),crsd_readb(sdport->base_io+SD_BUS_STATUS));
    RTKSDPRINTF(" configure1:0x%02x configure2:0x%02x configure3:0x%02x\n",crsd_readb(sdport->base_io+SD_CONFIGURE1),crsd_readb(sdport->base_io+SD_CONFIGURE2),crsd_readb(sdport->base_io+SD_CONFIGURE3));
    RTKSDPRINTF(" byteH:0x%02x byteL:0x%02x blkH:0x%02x blkL:0x%02x\n",crsd_readb(sdport->base_io+SD_BYTE_CNT_H),crsd_readb(sdport->base_io+SD_BYTE_CNT_L),crsd_readb(sdport->base_io+SD_BLOCK_CNT_H),crsd_readb(sdport->base_io+SD_BLOCK_CNT_L));
    RTKSDPRINTF(" dma_ctl1:0x%08x dma_ctl2:0x%08x dma_ctl3:0x%08x\n",crsd_readl(sdport->base_io+CR_DMA_CTL1),crsd_readl(sdport->base_io+CR_DMA_CTL2),crsd_readl(sdport->base_io+CR_DMA_CTL3));
    
    crsd_writeb((u8) (cmdcode|START_EN), sdport->base_io+SD_TRANSFER );
    rtk_crsd_sync(sdport);
    //spin_unlock_irqrestore(&sdport->lock, flags);
    rtk_crsd_int_enable(sdport,100);
    rtk_crsd_sync(sdport);
    rtk_crsd_waitfor(sdport);
    rtk_crsd_sync(sdport);

    if (sdport->sd_trans & ERR_STATUS) //transfer error
    {
    	printk(KERN_ERR "%s trans error 1  trans:0x%08x, st1:0x%08x, st2:0x%08x, bus:0x%08x\n",drv_name,crsd_readb(sdport->base_io+SD_TRANSFER),crsd_readb(sdport->base_io+SD_STATUS1),crsd_readb(sdport->base_io+SD_STATUS2),crsd_readb(sdport->base_io+SD_BUS_STATUS));
		//return CR_TRANSFER_FAIL;
	}

    err = CR_TRANSFER_TO;
    timeend = jiffies + msecs_to_jiffies(1000);

    while(time_before(jiffies, timeend)){

    	if ((crsd_readb(sdport->base_io+SD_STATUS2) & 0x01) == 1){
    		err = CR_TRANSFER_FAIL;
    		break;
    	}
    	if (!(sdport->rtflags & RTKCR_FCARD_DETECTED)){
    		err = CR_TRANSFER_FAIL;
    		break;
    	}
	    if ((cpu_mode == 1)&&(((crsd_readb(sdport->base_io+SD_BLOCK_CNT_H)<<8)|crsd_readb(sdport->base_io+SD_BLOCK_CNT_L))>1))
        {
    	    err = CR_TRANS_OK;
    		break;
    	}
    	if ((crsd_readb(sdport->base_io+SD_TRANSFER) & (END_STATE|IDLE_STATE))==(END_STATE|IDLE_STATE))
        {
    	    err = CR_TRANS_OK;
    		break;
    	}
    	if ((crsd_readb(sdport->base_io+SD_TRANSFER) & (ERR_STATUS))==(ERR_STATUS))
        {
    	    err = CR_TRANSFER_FAIL;
    		break;
    	}
		RTKSDPRINTF_DBG("*");
    }
	
    if (jiffies >=  timeend){

		if ((sdport->sd_trans & (END_STATE | IDLE_STATE)) == (END_STATE | IDLE_STATE)) {
			//printk(KERN_ERR "CR_TRANS_OK\n");
			err = CR_TRANS_OK;
		}else{
			 printk(KERN_ERR "\n%s - trans error 3 :\n trans : 0x%08x, st1 : 0x%08x, st2 : 0x%08x, bus : 0x%08x\n",
							   drv_name,
							   crsd_readb(sdport->base_io+SD_TRANSFER),
							   crsd_readb(sdport->base_io+SD_STATUS1),
							   crsd_readb(sdport->base_io+SD_STATUS2),
							   crsd_readb(sdport->base_io+SD_BUS_STATUS));
		}

        return err;
    }

    return err;
}
EXPORT_SYMBOL_GPL(rtk_crsd_wait_opt_end);

DECLARE_COMPLETION(rtk_wait);
struct completion* rtk_crsd_int_enable(struct rtk_crsd_host *sdport, unsigned long msec)
{
    unsigned long timeend=0;
	
    sdport->int_status = 0;
    sdport->sd_trans = -1;
    sdport->sd_status1 = 0;
    sdport->sd_status2 = 0;
    sdport->bus_status = 0;

   // return NULL;
    //#ifndef ENABLE_SD_INT_MODE
    sdport->int_waiting = &rtk_crsd_wait;
    //smp_wmb();
    return NULL;
    //#endif
    if (sdport->int_waiting)
        return NULL;
    
//  DECLARE_COMPLETION(rtk_wait);
    sdport->int_status  = 0;
    sdport->sd_trans    = 0;
    sdport->sd_status1   = 0;
    sdport->sd_status2   = 0;
    sdport->bus_status   = 0;
    sdport->int_waiting = &rtk_wait;

    RTKSDPRINTF( "rtk wait complete addr = %08x\n", (unsigned int) sdport->int_waiting);

    /* timeout timer fire */
    if (&sdport->timer){
    	if(sdport->tmout){
            timeend = sdport->tmout;
            sdport->tmout = 0;
        }else
            timeend = msecs_to_jiffies(msec);

        RTKSDPRINTF( "TO = 0x%08lx\n", timeend);
        mod_timer(&sdport->timer, (jiffies + timeend) );
    }

    smp_wmb();
    return (struct completion *)&rtk_wait;
}
EXPORT_SYMBOL_GPL(rtk_crsd_int_enable);

void rtk_crsd_waitfor(struct rtk_crsd_host *sdport){

    unsigned long timeend = 0;
    u32 sd_trans = 0;
    u32 sd_status1 = 0;
    u32 sd_status2 = 0;
    u32 bus_status=0;
    unsigned long flags = 0;
	
//  spin_lock_irqsave(&sdport->lock,flags);
    timeend = jiffies + msecs_to_jiffies(300);
    RTKSDPRINTF("START from polling\n");

    while(time_before(jiffies, timeend)){
	    if (!(sdport->rtflags & RTKCR_FCARD_DETECTED))
    		break;
	
		//udelay(800);
        if ((crsd_readb(sdport->base_io+SD_TRANSFER) & (END_STATE|IDLE_STATE))==(END_STATE|IDLE_STATE))
            break;
    }

    rtk_crsd_get_sd_trans(sdport->base_io,&sd_trans);
    rtk_crsd_get_sd_sta(sdport->base_io,&sd_status1,&sd_status2,&bus_status);

	spin_lock_irqsave(&sdport->lock,flags);
    sdport->sd_trans    = sd_trans;
    sdport->sd_status1   = sd_status1;
    sdport->sd_status2   = sd_status2;
    sdport->bus_status   = bus_status;
  	spin_unlock_irqrestore(&sdport->lock,flags);

    RTKSDPRINTF("int sts : 0x%08x sd_trans : 0x%08x, sd_st1 : 0x%08x\n", sdport->int_status, sdport->sd_trans, sdport->sd_status1);
    RTKSDPRINTF("int st2 : 0x%08x bus_sts : 0x%08x\n", sdport->sd_status2, sdport->bus_status);
//  spin_unlock_irqrestore(&sdport->lock,flags);
    return;
}
EXPORT_SYMBOL_GPL(rtk_crsd_waitfor);

void rtk_crsd_op_complete(struct rtk_crsd_host *sdport)
{
    RTKSDPRINTF( "\n");
    if (sdport->int_waiting) {
        struct completion *waiting = sdport->int_waiting;
		RTKSDPRINTF( "rtk wait complete addr = %08x\n", (unsigned int) sdport->int_waiting);
		complete(waiting);
    }
}
EXPORT_SYMBOL_GPL(rtk_crsd_op_complete);

char *rtk_crsd_parse_token(const char *parsed_string, const char *token)
{
    const char *ptr = parsed_string;
    const char *start, *end, *value_start, *value_end;
    char *ret_str;
    RTKSDPRINTF( "\n");
	
    while(1) {
        value_start = value_end = 0;
        for(;*ptr == ' ' || *ptr == '\t'; ptr++);
        if(*ptr == '\0')        break;
        start = ptr;
        for(;*ptr != ' ' && *ptr != '\t' && *ptr != '=' && *ptr != '\0'; ptr++) ;
        end = ptr;
        if(*ptr == '=') {
            ptr++;
            if(*ptr == '"') {
                ptr++;
                value_start = ptr;
                for(; *ptr != '"' && *ptr != '\0'; ptr++);
                if(*ptr != '"' || (*(ptr+1) != '\0' && *(ptr+1) != ' ' && *(ptr+1) != '\t')) {
                    RTKSDPRINTF( "system_parameters error! Check your parameters     .");
                    break;
                }
            } else {
                value_start = ptr;
                for(;*ptr != ' ' && *ptr != '\t' && *ptr != '\0' && *ptr != '"'; ptr++) ;
                if(*ptr == '"') {
                    RTKSDPRINTF( "system_parameters error! Check your parameters.");
                    break;
                }
            }
            value_end = ptr;
        }

        if(!strncmp(token, start, end-start)) {
            if(value_start) {
                ret_str = kmalloc(value_end-value_start+1, GFP_KERNEL);
                // KWarning: checked ok by alexkh@realtek.com
                if(ret_str){
                    strncpy(ret_str, value_start, value_end-value_start);
                    ret_str[value_end-value_start] = '\0';
                }
                return ret_str;
            } else {
                ret_str = kmalloc(1, GFP_KERNEL);
                // KWarning: checked ok by alexkh@realtek.com
                if(ret_str)
                    strcpy(ret_str, "");
                return ret_str;
            }
        }
//		RTKSDPRINTF_INFO("=");
    }

    return (char*)NULL;
}
EXPORT_SYMBOL_GPL(rtk_crsd_parse_token);

void rtk_crsd_chk_param(u32 *pparam, u32 len, u8 *ptr)
{
    u32 value,i;
    RTKSDPRINTF( "\n");
    
    *pparam = 0;
    for(i=0;i<len;i++){
        value = ptr[i] - '0';
        // KWarning: checked ok by alexkh@realtek.com
        if((value >= 0) && (value <=9))
        {
            *pparam+=value<<(4*(len-1-i));
            continue;
        }

        value = ptr[i] - 'a';
        // KWarning: checked ok by alexkh@realtek.com
        if((value >= 0) && (value <=5))
        {
            value+=10;
            *pparam+=value<<(4*(len-1-i));
            continue;
        }

        value = ptr[i] - 'A';
        // KWarning: checked ok by alexkh@realtek.com
        if((value >= 0) && (value <=5))
        {
            value+=10;
            *pparam+=value<<(4*(len-1-i));
            continue;
        }
    }
}
EXPORT_SYMBOL_GPL(rtk_crsd_chk_param);

u32 crsd_verA_magic_num = 0;
int rtk_crsd_chk_VerA(void)
{
    RTKSDPRINTF( "\n");
    //TODO : chk
#if 0 
    if(!crsd_verA_magic_num)
        crsd_verA_magic_num = crsd_readl(0xb8060000);

    if(crsd_verA_magic_num == 0x62270000){
        return 1;
    }else{
        return 0;
    }
#else
    return crsd_verA_magic_num; //0: polling, 1: interrupt
#endif
}
EXPORT_SYMBOL_GPL(rtk_crsd_chk_VerA);

void rtk_crsd_show_config123(struct rtk_crsd_host *sdport){
    u32 reginfo = 0;
//  u32 clksel_sht;
    u32 iobase = sdport->base_io;
    RTKSDPRINTF( "\n");
    if(iobase == EM_BASE_ADDR){
        reginfo = crsd_readl(iobase+EMMC_CKGEN_CTL);
        //clksel_sht = EMMC_CLKSEL_SHT;
    }else if(iobase == CR_BASE_ADDR){
        reginfo = crsd_readl(iobase+CR_SD_CKGEN_CTL);
        //clksel_sht = SD_CLKSEL_SHT;
    }

    RTKSDPRINTF( "CFG1=0x%x CFG2=0x%x CFG3=0x%x bus clock CKGEN=%08x\n",
        crsd_readl(iobase+SD_CONFIGURE1),
        crsd_readl(iobase+SD_CONFIGURE2),
        crsd_readl(iobase+SD_CONFIGURE3),
        reginfo );
}
EXPORT_SYMBOL_GPL(rtk_crsd_show_config123);

/* end of file */

