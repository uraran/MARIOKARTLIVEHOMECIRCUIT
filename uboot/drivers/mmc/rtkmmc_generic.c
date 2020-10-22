/*
 *  This is a driver for the eMMC controller found in Realtek RTD1195
 *  SoCs.
 *
 *  Copyright (C) 2013 Realtek Semiconductors, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "rtkmmc_generic.h"
#include "rtkmmc.h"

#define __RTKMMC_GENERIC_C__

int mmc_had_been_initialized;
int mmc_ready_to_use;
static struct mmc mmc_dev[2];

extern unsigned int rtkmmc_debug_msg;
extern unsigned int rtkmmc_off_error_msg_in_init_flow;
extern unsigned int sys_ext_csd_addr;

#ifdef CONFIG_GENERIC_MMC

static int mmc_send_cmd(
			struct mmc * mmc,
			struct mmc_cmd * cmd,
			struct mmc_data * data )
{    
	int ret_err=0;
	volatile int rsp_para1=0, rsp_para2=0, rsp_para3=0;
    	volatile struct rtk_cmd_info cmd_info;
	unsigned char cfg3=0,cfg1=0;

	MMCPRINTF("\n*** %s %s %d, cmdidx=0x%02x(%d), resp_type=0x%08x -------\n", __FILE__, __FUNCTION__, __LINE__, cmd->cmdidx, cmd->cmdidx, cmd->resp_type);
	if( data ) {
		MMCPRINTF("*** %s %s %d, flags=0x%08x(%s), blks=%d, blksize=%d\n", __FILE__, __FUNCTION__, __LINE__, data->flags, (data->flags&MMC_DATA_READ) ? "R" : "W", data->blocks, data->blocksize);
	}
	rsp_para1 = 0;
    	rsp_para2 = 0;
	rsp_para3 = 0;
	cfg3 = cr_readb(SD_CONFIGURE3)|RESP_TIMEOUT_EN;
	cfg1 = cr_readb(SD_CONFIGURE1)&(MASK_CLOCK_DIV|MASK_BUS_WIDTH|MASK_MODE_SELECT);
	MMCPRINTF("org cfg1=%02x, cfg3=%02x\n",cfg1,cfg3);
    //correct emmc setting for rtd1195
    switch(cmd->cmdidx)
    {
        case MMC_GO_IDLE_STATE:
            rsp_para1 = cfg1|SD1_R0;
            rsp_para2 = 0x50|SD_R0;
            rsp_para3 = 0;
            break;
        case MMC_SEND_OP_COND:
            rsp_para1 = cfg1|SD1_R0;
            rsp_para2 = SD_R3;
            rsp_para3 = 0;
	    if (GET_CHIP_REV() >= PHOENIX_REV_B)
            	cmd->cmdarg = (MMC_SECTOR_ADDR|EMMC_VDD_1_8);
	    else
            	cmd->cmdarg = (MMC_SECTOR_ADDR|EMMC_VDD_33_34|EMMC_VDD_32_33|EMMC_VDD_31_32|EMMC_VDD_30_31);
            break;
        case MMC_ALL_SEND_CID:
            rsp_para1 = cfg1|SD1_R0;
            rsp_para2 = SD_R2;
            rsp_para3 = SD2_R0|RESP_TIMEOUT_EN;
            break;
        case MMC_SET_RELATIVE_ADDR:
            rsp_para1 = cfg1|SD1_R0;
            rsp_para2 = SD_R1|CRC16_CAL_DIS;
            rsp_para3 = cfg3;
            cmd->cmdarg = 0x10000;
            break;
        case MMC_SEND_CSD:
        case MMC_SEND_CID:
            rsp_para1 = cfg1|SD1_R0;
            rsp_para2 = SD_R2;
            rsp_para3 = cfg3;
            cmd->cmdarg = 0x10000;
            break;
        case MMC_SEND_EXT_CSD:
            rsp_para1 = cfg1|SD1_R0;
            rsp_para2 = SD_R1;
            rsp_para3 = cfg3;
            break;
        case MMC_SELECT_CARD:
            rsp_para1 = cfg1|SD1_R0;
            rsp_para2 = SD_R1b|CRC16_CAL_DIS;
            rsp_para3 = -1;
            cmd->cmdarg = 0x10000;
            break;
        case MMC_SWITCH:
            rsp_para1 = cfg1;
            rsp_para2 = SD_R1;
            rsp_para3 = RESP_TIMEOUT_EN;
            break;
        case MMC_SEND_STATUS:
            rsp_para1 = -1;
            rsp_para2 = SD_R1;
            rsp_para3 = RESP_TIMEOUT_EN;
            cmd->cmdarg = 0x10000;
            break;
        case MMC_STOP_TRANSMISSION:
            rsp_para1 = -1;
            rsp_para2 = SD_R1|CRC16_CAL_DIS;
            rsp_para3 = -1;
            break;
	 case MMC_CMD_MICRON_60 :
            rsp_para1 = cfg1|SD1_R0;
            rsp_para2 = 0x1;
            rsp_para3 = 0x1;
            break;
	 case MMC_CMD_MICRON_61 :
            rsp_para1 = cfg1|SD1_R0;
            rsp_para2 = 0x1;
            rsp_para3 = 0x1;
            break;
	 case MMC_CMD_MICRON_62 :
            rsp_para1 = 0x90;
            rsp_para2 = 0x0;
            rsp_para3 = 0x1;
            break;
	 case MMC_CMD_MICRON_63 :
            rsp_para1 = 0x90;
            rsp_para2 = 0x0;
            rsp_para3 = 0x1;
	    break;
	 case MMC_SET_BLOCKLEN :
            rsp_para1 = -1;
            rsp_para2 = SD_R1;
            rsp_para3 = -1;
            break;
        default:
            rsp_para1 = -1;     //don't update
            rsp_para2 = cmd->resp_type;
            rsp_para3 = -1;     //don't update
            break;            
    }

	if( cmd->resp_type & MMC_RSP_PRESENT ) {
		if( cmd->resp_type & MMC_RSP_136 ) {
			rsp_para2 |= 0x02;  // RESP_TYPE_17B
		}
		else if (cmd->cmdidx <= MMC_CMD_MICRON_61) {
			rsp_para2 |= 0x01; // RESP_TYPE_6B
		}
		if( cmd->resp_type & MMC_RSP_CRC ) {

		}
		else {
			rsp_para2 |= ( /*(1<<7) |*/ (1<<2) ); // CRC7_CHK_DIS
		}
	}

    if (data)
    {
        switch(cmd->cmdidx)
        {
	    case MMC_CMD_MICRON_60 :
                MMCPRINTF("*** %s %s %d - blocks = %08x, blocksize = %08x***\n", __FILE__, __FUNCTION__, __LINE__, data->blocks, data->blocksize);
                ret_err = rtk_micron_eMMC_write(cmd->cmdarg/mmc->write_bl_len, data->blocks*EMMC_BLK_SIZE, (unsigned int *)data->dest);
		break;
	    case MMC_CMD_MICRON_61 :
                MMCPRINTF("*** %s %s %d - blocks = %08x, blocksize = %08x***\n", __FILE__, __FUNCTION__, __LINE__, data->blocks, data->blocksize);
                ret_err = rtk_micron_eMMC_read(cmd->cmdarg/EMMC_BLK_SIZE, data->blocks*EMMC_BLK_SIZE, (unsigned int *)data->dest);
		break;
            case MMC_READ_SINGLE_BLOCK :
            case MMC_READ_MULTIPLE_BLOCK :
                MMCPRINTF("*** %s %s %d - blocks = %08x, blocksize = %08x***\n", __FILE__, __FUNCTION__, __LINE__, data->blocks, data->blocksize);
                ret_err = rtk_eMMC_read(cmd->cmdarg/EMMC_BLK_SIZE, data->blocks*EMMC_BLK_SIZE, (unsigned int *)data->dest);
                break;
            case MMC_WRITE_BLOCK :
            case MMC_WRITE_MULTIPLE_BLOCK :
                MMCPRINTF("*** %s %s %d - blocks = %08x, blocksize = %08x***\n", __FILE__, __FUNCTION__, __LINE__, data->blocks, data->blocksize);
                ret_err = rtk_eMMC_write(cmd->cmdarg/mmc->write_bl_len, data->blocks*EMMC_BLK_SIZE, (unsigned int *)data->dest);
                break;
            default:
                MMCPRINTF("*** %s %s %d ***\n", __FILE__, __FUNCTION__, __LINE__);
		MMCPRINTF("cmdidx=%02x,arg=%02x,rsp1=%02x,rsp2=%02x,rsp3=%02x\n",cmd->cmdidx,cmd->cmdarg,rsp_para1, rsp_para2, rsp_para3);
                cmd_info.cmd = cmd;
                cmd->cmdarg = 0;
                cmd_info.rsp_para1    = rsp_para1;
                cmd_info.rsp_para2    = rsp_para2;
                cmd_info.rsp_para3    = rsp_para3;
                cmd_info.rsp_len     = mmc_get_rsp_len(rsp_para2);
                cmd_info.byte_count  = 0x200;
                cmd_info.block_count = 1;
                cmd_info.data_buffer = data->dest;
                //cmd_info.data_buffer = sys_ext_csd_addr;
                cmd_info.xfer_flag   = RTK_MMC_DATA_READ; //dma the result to ddr
                ret_err = mmccr_Stream_Cmd( SD_NORMALREAD, (struct rtk_cmd_info*) &cmd_info ,0);
		//memcpy(data->dest, sys_ext_csd_addr, 512);
                //flush_cache(data->dest, 512);
		//mmc_show_ext_csd(data->dest);
                break;
        }
    }
    else
    {
	MMCPRINTF("cmdidx=%02x,arg=%02x,rsp1=%02x,rsp2=%02x,rsp3=%02x\n",cmd->cmdidx,cmd->cmdarg,rsp_para1, rsp_para2, rsp_para3);
    	ret_err = mmccr_SendCmd( cmd->cmdidx, cmd->cmdarg, rsp_para1, rsp_para2, rsp_para3, cmd->response );
    }
	return ret_err;
}

static void mmc_set_ios(struct mmc * mmc, unsigned int ios_caps)
{
	MMCPRINTF("*** %s %s %d, bw=%d, clk=%d\n", __FILE__, __FUNCTION__, __LINE__, mmc->bus_width, mmc->clock);

	// mmc->ldo
	if(ios_caps & MMC_IOS_LDO)
		mmccr_set_ldo(mmc->ldo, 0);
	
	// mmc->clk mode
	if(ios_caps & MMC_IOS_CLK)
		{
		if( mmc->mode_sel & MMC_MODE_HS200 ) {
			mmccr_set_mode_selection(MODE_SD30);
		}
		else if( mmc->mode_sel & MMC_MODE_HSDDR_52MHz ) {
			mmccr_set_mode_selection(MODE_DDR);
		}
		else
			mmccr_set_mode_selection(MODE_SD20);
		}

	// mmc->bus_width
	if(ios_caps & MMC_IOS_BUSWIDTH)
		{
		if( mmc->bus_width == 1 ) {
			mmccr_set_bits_width(BUS_WIDTH_1);
		}
		else if( mmc->bus_width == 4 ) {
			mmccr_set_bits_width(BUS_WIDTH_4);
			mmccr_set_div(EMMC_NORMAL_CLOCK_DIV, 0); 
		}
		else if( mmc->bus_width == 8 ) {
			mmccr_set_bits_width(BUS_WIDTH_8);
			mmccr_set_div(EMMC_NORMAL_CLOCK_DIV, 0); 
			//LY : for current plan, we only run at highest speed without wrapper divider
			//mmccr_set_speed(0); //high speed
		}
		else {
			UPRINTF("*** %s %s %d, ERR bw=%d, clk=%d\n", __FILE__, __FUNCTION__, __LINE__, mmc->bus_width, mmc->clock);
		}
	}

	// divider
	if(ios_caps & MMC_IOS_INIT_DIV)
		mmccr_set_div(EMMC_INIT_CLOCK_DIV, 0); 
	else if(ios_caps & MMC_IOS_NONE_DIV)
		mmccr_set_div(EMMC_NORMAL_CLOCK_DIV, 0); 

	if (ios_caps & MMC_IOS_GET_PAD_DRV)    //just get pad values
		mmccr_set_pad_driving(MMC_IOS_GET_PAD_DRV, 0xff,0xff,0xff);
	else if (ios_caps & MMC_IOS_SET_PAD_DRV)    
		mmccr_set_pad_driving(MMC_IOS_SET_PAD_DRV, 0xff,0xff,0xff);
	else if (ios_caps & MMC_IOS_RESTORE_PAD_DRV)    
		mmccr_set_pad_driving(MMC_IOS_RESTORE_PAD_DRV, 0xff,0xff,0xff);
}

static int mmc_init_setup(struct mmc * mmc)
{
	MMCPRINTF("*** %s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	mmccr_init_setup();
	return 0;
}

int board_mmc_init(bd_t * bis)
{
	int ret_err;
	struct mmc * pmmc;

    MMCPRINTF("*** %s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
    
	mmc_had_been_initialized = 0;
	mmc_ready_to_use = 0;

#if 0 // directly check eMMC device by customized init flow ( only for eMMC )
	rtkmmc_debug_msg = 1; // 1: enable debug message
	rtkmmc_off_error_msg_in_init_flow = 0; // 1: not show error message in initial stage
	ret_err = rtk_eMMC_init();
#else
	rtkmmc_debug_msg = 1; // 1: enable debug message
	rtkmmc_off_error_msg_in_init_flow = 1; // 1: not show error message in initial stage
	mmc_initial(1);
	ret_err = 0;
#endif
	if( !ret_err ) {
		pmmc = &mmc_dev[0];
		sprintf(pmmc->name, "RTD1195 eMMC");
		pmmc->send_cmd = mmc_send_cmd;
		pmmc->set_ios = mmc_set_ios;
		pmmc->init = mmc_init_setup;
		pmmc->getcd = NULL;
		pmmc->priv = NULL;
		pmmc->voltages = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;
#ifdef CONFIG_MMC_MODE_4BIT
		pmmc->host_caps = (MMC_MODE_4BIT | MMC_MODE_HS_52MHz | MMC_MODE_HS | MMC_MODE_HC);
#else
		pmmc->host_caps = (MMC_MODE_4BIT | MMC_MODE_8BIT | MMC_MODE_HS200 | MMC_MODE_HSDDR_52MHz | MMC_MODE_HS_52MHz | MMC_MODE_HS | MMC_MODE_HC);
#endif

		pmmc->f_min = 400000;
		pmmc->f_max = 50000000;
		pmmc->b_max = 256; // max transfer block size
		mmc_register(pmmc);
	}

	return ret_err;
}

int bringup_mmc_driver( void )
{
	int ret_err;
	struct mmc * mmc;
	int total_device_num = -1;
	int curr_device;

    MMCPRINTF("*** %s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
    
	total_device_num = get_mmc_num();
	if ( total_device_num < 0 ) {
		printf("no registed mmc device\n");
		return curr_device;
	}
	
	if( mmc_had_been_initialized ) {
		return 0; // alwasy use slot 0
	}

	curr_device = 0; // alwasy use slot 0
	rtkmmc_debug_msg = 1; // 1: enable debug message
	rtkmmc_off_error_msg_in_init_flow = 1; // 1: not show error message in initial stage
	mmc = find_mmc_device(curr_device);
	if( mmc ) {
                EXECUTE_CUSTOMIZE_FUNC(0);
		mmc_init(mmc);
                EXECUTE_CUSTOMIZE_FUNC(0);
		printf("Device: %s\n", mmc->name);
		printf("Manufacturer ID: %x\n", mmc->cid[0] >> 24);
		printf("OEM: %x\n", (mmc->cid[0] >> 8) & 0xffff);
		printf("Name: %c%c%c%c%c \n", (mmc->cid[0] & 0xff),
		                              (mmc->cid[1] >> 24) & 0xff,
		                              (mmc->cid[1] >> 16) & 0xff,
				                      (mmc->cid[1] >> 8) & 0xff,
				                      (mmc->cid[1] >> 0) & 0xff);
		printf("Tran Speed: %llx\n", (u64)mmc->tran_speed);
		printf("Rd Block Len: %d\n", mmc->read_bl_len);
		printf("%s version %d.%d\n", IS_SD(mmc) ? "SD" : "MMC", (mmc->version >> 4) & 0xf, mmc->version & 0xf);
		printf("High Capacity: %s\n", mmc->high_capacity ? "Yes" : "No");
		puts("Capacity: ");
		print_size((u64)mmc->capacity, "\n");
		printf("Bus Width: %d-bit\n", mmc->bus_width);
		printf("Speed: ");
		if (mmc->boot_caps & MMC_MODE_HS200)
			printf("HS200\n");
		else if (mmc->boot_caps & MMC_MODE_HSDDR_52MHz)
			printf("DDR50\n");
		else
			printf("SDR50\n");
		
		ret_err = curr_device;
		
		mmc_had_been_initialized = 1;
		if( mmc->capacity ) {
			mmc_ready_to_use = 1;
		}
		else {
			mmc_ready_to_use = 0;	
		}
	}
	else {
		printf("no mmc device at slot %x\n", curr_device);
		ret_err = -999;
	}
	rtkmmc_debug_msg = 0; // 0: off cmd message
	rtkmmc_off_error_msg_in_init_flow = 0; // 0: show error message after init done
	return ret_err;
}

#endif // end of CONFIG_GENERIC_MMC
