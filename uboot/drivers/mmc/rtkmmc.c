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

#include "rtkmmc.h"
#include <platform_lib/board/gpio.h>
#include <asm/arch/rbus/crt_reg.h>
#define __RTKMMC_C__

/* mmc spec definition */
const unsigned int tran_exp[] = {
    10000, 100000, 1000000, 10000000,
    0,     0,      0,       0
};

const unsigned char tran_mant[] = {
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

const unsigned int tacc_exp[] = {
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
};

const unsigned int tacc_mant[] = {
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

const char *const state_tlb[9] = {
    "STATE_IDLE",
    "STATE_READY",
    "STATE_IDENT",
    "STATE_STBY",
    "STATE_TRAN",
    "STATE_DATA",
    "STATE_RCV",
    "STATE_PRG",
    "STATE_DIS"
};
const char *const bit_tlb[4] = {
    "1bit",
    "4bits",
    "8bits",
    "unknow"
};

const char *const clk_tlb[8] = {
    "30MHz",
    "40MHz",
    "49MHz",
    "49MHz",
    "15MHz",
    "20MHz",
    "24MHz",
    "24MHz"
};

/************************************************************************
 *  global variable
 ************************************************************************/
unsigned cr_int_status_reg;
unsigned int sys_ext_csd_addr;
unsigned int rtkmmc_debug_msg;
unsigned int rtkmmc_off_error_msg_in_init_flow;
static e_device_type emmc_card;
static unsigned int sys_rsp17_addr;
static unsigned char* ptr_ext_csd;
static volatile struct backupRegs gRegTbl;
static volatile UINT8 g_cmd[6]={0};
static int bErrorRetry_1=0, bErrorRetry_2=0,bErrorRetry=0;
extern int mmc_ready_to_use;
int swap_endian(UINT32 input);
int gCurrentBootDeviceType=0;
volatile unsigned int gCurrentBootMode=MODE_SD20;

/**************************************************************************************
 * phoenix mmc driver
 **************************************************************************************/
/*******************************************************
 *
 *******************************************************/
int mmc_data_sync( struct mmc * mmc )
{
    MMCPRINTF("*** %s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
    
    mmc->rca = emmc_card.rca;
    mmc->high_capacity = emmc_card.sector_addressing ? 1 : 0;
	return 0;
}

/*******************************************************
 *
 *******************************************************/
int mmccr_send_status(UINT8* state, uint show_msg, int bIgnore)
{
    UINT32 rom_resp[4];
    int rom_err = 0;

    MMCPRINTF("*** %s %s %d - rca : %08x, rca addr : %08x\n", __FILE__, __FUNCTION__, __LINE__, emmc_card.rca, &emmc_card.rca);
   
    sync(); 
    rom_err = mmccr_SendCmd(MMC_SEND_STATUS,
                                   emmc_card.rca,
                                   -1,
                                   SD_R1|CRC16_CAL_DIS,
                                   -1,
                                   rom_resp,bIgnore);
    if(rom_err){
	if(show_msg)
        	printf("MMC_SEND_STATUS fail\n");
    }else{
        UINT8 cur_state = R1_CURRENT_STATE(rom_resp[0]);
        *state = cur_state;

	if(show_msg)
	{
            printf("cur_state=");
            printf(state_tlb[cur_state]);
            printf("\n");
	}
    }

    return rom_err;
}

/*******************************************************
 *
 *******************************************************/
int mmccr_wait_status(UINT8 state, int bIgnore)
{
    UINT32 rom_resp[4];
    UINT32 timeend;
    int rom_err = 0;

    timeend = 5;

    while(timeend--)
     {
        sync();
        rom_err = mmccr_SendCmd(MMC_SEND_STATUS,
                                       emmc_card.rca,
                                       -1,
                                       SD_R1|CRC16_CAL_DIS,
                                       -1,
                                       rom_resp,bIgnore);

        if(rom_err){
            printf("wait ");
            printf(state_tlb[state]);
            printf(" fail\n");
            break;
        }
        else{
            UINT8 cur_state = R1_CURRENT_STATE(rom_resp[0]);
	    #ifdef MMC_DEBUG
            printf("cur_state=");
            printf(state_tlb[cur_state]);
            printf("\n");
	    #endif
            rom_err = -10;
            if(cur_state == state){
                if(rom_resp[0] & R1_READY_FOR_DATA){
                    rom_err = 0;
                    break;
                }
            }
        }
        mdelay(1);
    }

    return rom_err;
}

/*******************************************************
 *
 *******************************************************/
void mmccr_set_div( unsigned int set_div, unsigned int show_msg )
{
    unsigned int tmp_div;
    
    MMCPRINTF("mmccr_set_div; switch to 0x%08x\n", set_div);
    
    tmp_div = cr_readb(SD_CONFIGURE1) & ~MASK_CLOCK_DIV;
    cr_writeb(tmp_div|set_div,SD_CONFIGURE1);
    sync();

    if( show_msg )
    	MMCPRINTF("0x18012180=0x%08x\n", cr_readb(SD_CONFIGURE1));
}

void mmccr_set_pad_driving(unsigned int mode, unsigned char CLOCK_DRIVING, unsigned char CMD_DRIVING, unsigned char DATA_DRIVING)
{
    static volatile unsigned char card_pad_val=0,cmd_pad_val=0,data_pad_val=0;

    switch(mode)
    {
	case MMC_IOS_GET_PAD_DRV:
		card_pad_val = cr_readb(CARD_PAD_DRV);
		cmd_pad_val = cr_readb(CMD_PAD_DRV);
		data_pad_val = cr_readb(DATA_PAD_DRV);
    		MMCPRINTF("backup card=0x%02x, cmd=0x%02x, data=0x%02x\n", card_pad_val, cmd_pad_val,data_pad_val);
    		MMCPRINTF("read : card=0x%02x, cmd=0x%02x, data=0x%02x\n", cr_readb(CARD_PAD_DRV), cr_readb(CMD_PAD_DRV),cr_readb(DATA_PAD_DRV));
		break;
	case MMC_IOS_SET_PAD_DRV:
    		cr_writeb(CLOCK_DRIVING, CARD_PAD_DRV); //clock pad driving
    		cr_writeb(CMD_DRIVING, CMD_PAD_DRV);   //cmd pad driving
    		cr_writeb(DATA_DRIVING, DATA_PAD_DRV);  //data pads driving
    		sync();
    		sync();
    		sync();
    		MMCPRINTF("set card=0x%02x, cmd=0x%02x, data=0x%02x\n", cr_readb(CARD_PAD_DRV), cr_readb(CMD_PAD_DRV),cr_readb(DATA_PAD_DRV));
		break;
	case MMC_IOS_RESTORE_PAD_DRV:
    		cr_writeb(card_pad_val, CARD_PAD_DRV); //clock pad driving
    		cr_writeb(cmd_pad_val, CMD_PAD_DRV);   //cmd pad driving
    		cr_writeb(data_pad_val, DATA_PAD_DRV);  //data pads driving
    		MMCPRINTF("val card=0x%02x, cmd=0x%02x, data=0x%02x\n", card_pad_val, cmd_pad_val,data_pad_val);
    		MMCPRINTF("restore card=0x%02x, cmd=0x%02x, data=0x%02x\n", cr_readb(CARD_PAD_DRV), cr_readb(CMD_PAD_DRV),cr_readb(DATA_PAD_DRV));
		break;
    }
    sync();
    mdelay(10);
}

/*******************************************************
 *
 *******************************************************/
void mmccr_set_ldo( unsigned int set_ldo, unsigned int show_msg )
{
    MMCPRINTF("mmccr_set_ldo; switch to 0x%08x\n", set_ldo);
    
    REG32(PLL_EMMC3) = (REG32(PLL_EMMC3) & 0xffff)|(set_ldo<<16);
    sync();

    if( show_msg )
    	MMCPRINTF("0x180001f8=0x%08x\n", REG32(PLL_EMMC3));
}

/*******************************************************
 *
 *******************************************************/
void mmccr_set_bits_width( unsigned int set_bit )
{
    unsigned int tmp_bits;

    MMCPRINTF("mmccr_set_bits_width; switch to 0x%08x\n",set_bit);
    tmp_bits = cr_readb(SD_CONFIGURE1) & ~MASK_BUS_WIDTH;
    cr_writeb((UINT8)(tmp_bits|set_bit),SD_CONFIGURE1);
    sync();
}

/*******************************************************
 *
 *******************************************************/
void mmccr_set_mode_selection( unsigned int set_bit )
{
    unsigned int tmp_bits;

    MMCPRINTF("%s - start\n", __func__);
    MMCPRINTF("mmccr_set_mode_selection; switch to 0x%08x\n",set_bit);
    tmp_bits = cr_readb(SD_CONFIGURE1) & ~MASK_MODE_SELECT;
    cr_writeb((UINT8)(tmp_bits|set_bit),SD_CONFIGURE1);
    sync();
}
int mmccr_get_mode_selection()
{
    int tmp_bits;

    MMCPRINTF("%s - start\n", __func__);
    tmp_bits = cr_readb(SD_CONFIGURE1) & MASK_MODE_SELECT;
    MMCPRINTF("mmccr_set_mode_selection; switch to 0x%08x\n",tmp_bits);
    sync();
    return tmp_bits;
}

/*******************************************************
 *
 *******************************************************/
void mmccr_set_speed( unsigned int para )
{
    MMCPRINTF("%s : set card/cmd/data pad driving\n",__func__);

#if 0
    cr_writeb(0x66,CARD_PAD_DRV);
    cr_writeb(0x64,CMD_PAD_DRV);
    cr_writeb(0x66,DATA_PAD_DRV);
#endif
 
    switch(para)
    {
        case 0: //DDR50
            cr_writel(0x2100,CR_CKGEN_CTL);
            break;
        case 1:
            cr_writel(0x2101,CR_CKGEN_CTL);
            break;
        case 2:
        default :
            cr_writel(0x2102,CR_CKGEN_CTL);
            break;          
    }
    sync();
    MMCPRINTF("read card pad reg : card=0x%02x, cmd=0x%02x, data=0x%02x\n", cr_readb(CARD_PAD_DRV), cr_readb(CMD_PAD_DRV),cr_readb(DATA_PAD_DRV));
}

/*******************************************************
 *
 *******************************************************/
void mmccr_read_rsp( void * rsp, int reg_count )
{
    UINT8 tmpcmd[5]={0};
    UINT32 *ptr = rsp;
    uchar *pRSP, *pRSP1;
    int i;
    
    if ( reg_count == 6 ){
        sync();
        
        tmpcmd[0] = cr_readb(SD_CMD0);
        tmpcmd[1] = cr_readb(SD_CMD1);
        tmpcmd[2] = cr_readb(SD_CMD2);
        tmpcmd[3] = cr_readb(SD_CMD3);
        tmpcmd[4] = cr_readb(SD_CMD4);
        tmpcmd[5] = cr_readb(SD_CMD5);
        //device is big-endian
        REG32(ptr) = (UINT32)((tmpcmd[0]<<24) |
                 (tmpcmd[1]<<16) |
                 (tmpcmd[2]<<8) |
                  tmpcmd[3]) ;
        REG32(ptr+1) = (UINT32)((tmpcmd[5]<<24) |
                                (tmpcmd[4]<<16));
        
        #ifdef THIS_IS_FLASH_WRITE_U_ENV
        prints("rsp len 6 : ");
        prints("cmd0: ");
        print_val(tmpcmd[0],2);
        prints(" cmd1: ");
        print_val(tmpcmd[1],2);
        prints(" cmd2: ");
        print_val(tmpcmd[2],2);
        prints(" cmd3: ");
        print_val(tmpcmd[3],2);
        prints(" cmd4: ");
        print_val(tmpcmd[4],2);
        prints(" cmd5: ");
        print_val(tmpcmd[5],2);
        prints(" ptr0: ");
        print_hex(REG32(ptr));
        prints(" ptr1: ");
        print_hex(REG32(ptr+1));
        prints("\n");
        #else
        MMCPRINTF("rsp 6 : 0 = 0x%08x , 1 = 0x%08x\n", *ptr, *(ptr+1));
        #endif
        
    }
    else if( reg_count == 17 ){
        REG32(ptr+0) = REG32(sys_rsp17_addr+0x00);
        REG32(ptr+1) = REG32(sys_rsp17_addr+0x04);
        REG32(ptr+2) = REG32(sys_rsp17_addr+0x08);
        REG32(ptr+3) = REG32(sys_rsp17_addr+0x0c);

        pRSP = sys_rsp17_addr;
        pRSP1 = ptr;
        for(i=0;i<15;i++)
        {
            *(uchar*)(pRSP1+i) = *(uchar*)(pRSP+i+1);
        }

        flush_cache(pRSP1, 16);
        REG32(ptr+0) = swap_endian(REG32(ptr+0));
        REG32(ptr+1) = swap_endian(REG32(ptr+1));
        REG32(ptr+2) = swap_endian(REG32(ptr+2));
        REG32(ptr+3) = swap_endian(REG32(ptr+3));
        MMCPRINTF("csd1 :%08x %08x %08x %08x\n", REG32(ptr+0),REG32(ptr+1),REG32(ptr+2),REG32(ptr+3));
    
        #ifdef THIS_IS_FLASH_WRITE_U_ENV
        prints("rsp len 17B :[0] 0x");
        print_hex(REG32(ptr+0));
        prints(" [1] 0x");
        print_hex(REG32(ptr+1));
        prints(" [2] 0x");
        print_hex(REG32(ptr+2));
        prints(" [3] 0x");
        print_hex(REG32(ptr+3));
        prints("\n");
        #else
        MMCPRINTF("rsp 17 : [0]0x%08x , [1]0x%08x\n",*(ptr+0), *(ptr+1));
        MMCPRINTF("rsp 17 : [2]0x%08x , [3]0x%08x\n",*(ptr+2), *(ptr+3));
        #endif
    }
    
}

/*******************************************************
 *
 *******************************************************/
UINT32 mmccr_read_sram_dma_data(UINT32 sram_buf, UINT32 block_count)
{
				UINT32 i=0,blk_no=0, fifo_tmp0=0, fifo_tmp1=0;
                INT32 time_cnt=0;
                UINT32 cpu_acc_reg=0, dma_reg=0;
                
                if(sram_buf){
                    #ifdef THIS_IS_FLASH_WRITE_U_ENV
                    prints("sram_buf addr: 0x");
                    print_hex(sram_buf);
                    prints("\n");
                    prints("Read data from SRAM FIFO : blk_cnt=0x");
                    print_hex(block_count);
                    prints("\n");
                    #endif
                    while (block_count--)
                    {
                        #ifdef THIS_IS_FLASH_WRITE_U_ENV
                        prints("\n[blk no. ");
                        print_val(block_count+1, 2);
                        prints(" \n");
                        #endif
                        //1st half blk
                        for( i=0; i<512/4; i+=2 ){
                            fifo_tmp0 = cr_readl(CR_SRAM_BASE_0+i*4);
                            fifo_tmp1 = cr_readl(CR_SRAM_BASE_0+(i+1)*4);
                            REG32(sram_buf+(blk_no*0x200)+(i*4))= swap_endian(fifo_tmp1);
                            REG32(sram_buf+(blk_no*0x200)+(i+1)*4) = swap_endian(fifo_tmp0);
                            #ifdef THIS_IS_FLASH_WRITE_U_ENV
                            prints(" 0x");
                            print_hex(REG32(sram_buf+(blk_no*0x200)+(i*4)));
                            prints(" 0x");
                            print_hex(REG32(sram_buf+(blk_no*0x200)+(i+1)*4));
                            if ((i%10 == 0)&&(i!=0))
                                prints("\n");
                            #endif
                        }
                        if (block_count==0)
                        {
                            time_cnt=10*500; //1sec to timeout
                            for(i=time_cnt;i<=0;i--)
                            {
                                if ((REG8(SD_TRANSFER) & END_STATE) == END_STATE)
                                    break;
                                udelay(100);
                            }
                            if (i <= 0)
                                return ERR_EMMC_SRAM_DMA_TIME; 
                        }
                        if (block_count>=0)
                        {
                            //get next block
                            cpu_acc_reg = REG32(CR_CPU_ACC);
                            REG32(CR_CPU_ACC) = cpu_acc_reg|0x3;
                            time_cnt=10*1000; //1sec to timeout
                            for(i=time_cnt;i<=0;i--)
                            {
                                cpu_acc_reg = REG32(CR_CPU_ACC);
                                if ((cpu_acc_reg & BUF_FULL) == BUF_FULL)
                                    break;
                                udelay(100);
                            }
                            if (i <= 0)
                                return ERR_EMMC_SRAM_DMA_TIME; 
                            blk_no++;
                        }
                    }
                }
    //polling the buf_full to 0
    #ifdef THIS_IS_FLASH_WRITE_U_ENV
    prints("polling buf_full to 0\n");
    #endif
    cpu_acc_reg = REG32(CR_CPU_ACC);
    time_cnt=10*500; //1sec to timeout
    for(i=time_cnt;i<=0;i--)
    {
        cpu_acc_reg = REG32(CR_CPU_ACC);
        if ((cpu_acc_reg & BUF_FULL) == 0x00)
            break;
        udelay(100);
    }
    if (i <= 0)
        return ERR_EMMC_SRAM_DMA_TIME; 

    //polling dma to 0
    #ifdef THIS_IS_FLASH_WRITE_U_ENV
    prints("polling dma ctl3 to 0\n");
    #endif
    time_cnt=10*500; //1sec to timeout
    for(i=time_cnt;i<=0;i--)
    {
        dma_reg = REG32(CR_DMA_CTL3);
        if ((dma_reg  & DMA_XFER) == 0x00)
            break;
        udelay(100);
    }
    if (i <= 0)
        return ERR_EMMC_SRAM_DMA_TIME; 
    
    return 0;
}

/*******************************************************
 *
 *******************************************************/
int mmc_state_is_transfer_and_ready( e_device_type * card ,int bIgnore)
{

    unsigned int rsp_buffer[4];
    unsigned int  curr_state;
    int ret_err = 0;
    unsigned int retry_cnt;

	MMCPRINTF("mmc_state_is_transfer_and_ready()\n");

	retry_cnt = 0;
	while( retry_cnt++ < 2 ) {
		ret_err = mmccr_SendCmd(MMC_SEND_STATUS, card->rca, -1, SD_R1|CRC16_CAL_DIS, 0, rsp_buffer, bIgnore);
	    if( !ret_err ) {
	    	if( retry_cnt == 2 ) {
		        curr_state = R1_CURRENT_STATE(rsp_buffer[0]);
		        MMCPRINTF("curr_state=%d\n", curr_state);
		        if( curr_state == STATE_TRAN /*4*/ ) {
		        	if(rsp_buffer[0] & R1_READY_FOR_DATA){
		                return 1;
		            }
		        }
		        else {
		        	return 0;
		        }
		    }
	    }
    	else {
    		return -1;
    	}
	}
    return -1;
}

/*******************************************************
 *
 *******************************************************/
unsigned int mmc_get_rsp_len( unsigned char rsp_para )
{
    switch (rsp_para & 0x3) {
	    case 0:
	        return 0; // hw bug ??
	    case 1:
	        return 6;
	    case 2:
	        return 17;
	    default:
	        return 0;
    }
}

/*******************************************************
 *
 *******************************************************/
unsigned int mmc_get_rsp_type( struct mmc_cmd * cmd )
{
    unsigned int rsp_type = 0;

    /* the marked case are used. */
    switch( cmd->cmdidx)
    {
        case 3:
        case 7:
        case 13:
        case 16:
        case 23:
        case 35:
        case 36:
        case 55:
            rsp_type |= CRC16_CAL_DIS;
        case 8:
        case 11:
        case 14:
        case 19:
        case 17:
        case 18:
        case 20:
        case 24:
        case 25:
        case 26:
        case 27:
        case 30:
        case 42:
        case 56:
	case 60:
	case 61:
            rsp_type |= SD_R1;
            break;
        case 6:
        case 12:
        case 28:
        case 29:
        case 38:
            rsp_type = SD_R1b|CRC16_CAL_DIS;
            break;
        case 2:
        case 9:
        case 10:
            rsp_type = SD_R2;
            break;
        case 1:
            rsp_type = SD_R3;
            break;
        default:
            rsp_type = SD_R0;
            break;
    }
    return rsp_type;
}

/*******************************************************
 *
 *******************************************************/
int mmccr_TriggerXferCmd( unsigned char xferCmdCode, unsigned int cpu_mode , unsigned int bignore)
{
    volatile UINT8 sd_transfer_reg;
    volatile int loops=0, loops1=0;
    volatile int err=0, timeout_cnt=0;
    volatile int dma_val=0;

    sync(); 
    cr_writeb((UINT8) (xferCmdCode|START_EN), SD_TRANSFER );
    sync(); 
    sync(); 

    if ((cr_readb(SD_TRANSFER) & ERR_STATUS) != 0x0) //transfer error
    {
        if (bignore&&(g_cmd[0] == 0x59)&&((cr_readb(SD_STATUS1)==CRC7_STATUS)||(cr_readb(SD_STATUS1)==(CRC7_STATUS|0x8))))
        {
                printf("\nignore cmd25 crc7 error \n");
                return CR_TRANSFER_IGN;
        }
        else
        {
            #ifdef FOR_ICE_LOAD
            prints("\ncard trans err1 : 0x");
            print_hex(cr_readb(SD_TRANSFER));
            prints("st1 : 0x");
            print_hex(cr_readb(SD_STATUS1));
            prints("st2 : 0x");
            print_hex(cr_readb(SD_STATUS2));
            prints("bus st : 0x");
            print_hex(cr_readb(SD_BUS_STATUS));
            
            prints("\n");
            #endif
            return CR_TRANSFER_FAIL;
        }
    }

    //check1
    if (g_cmd[0] == 0x41)
    {
     loops = 10;     
     loops1 = 300; 
     while(loops ){
        while(loops1--)
        {
    	    sync(); 
            if ((cr_readb(SD_TRANSFER) & ERR_STATUS) != 0x0) //transfer error
            {
                #ifdef FOR_ICE_LOAD  
                prints("\ncard trans err2 : 0x");
                print_hex(cr_readb(SD_TRANSFER));
                prints("\n");
                #endif
                
                return CR_TRANSFER_FAIL;
            }
    	    sync(); 
            if ((cr_readb(SD_TRANSFER) & (END_STATE|IDLE_STATE))==(END_STATE|IDLE_STATE))
            {
                #ifdef FOR_ICE_LOAD  
                prints("\ncard transferred \n");
                #endif
                
                err = 0;
                break;
            }
            mdelay(1);
        }
    	sync(); 
        
        //card busy ??
        if ((cr_readb(SD_CMD1)&0x80)!=0x80)
        {
            //resend cmd again
            cr_writeb(g_cmd[0], SD_CMD0);
            cr_writeb(g_cmd[1],  SD_CMD1);
            cr_writeb(g_cmd[2],  SD_CMD2);
            cr_writeb(g_cmd[3],   SD_CMD3);
            cr_writeb(g_cmd[4],      SD_CMD4);
            cr_writeb(g_cmd[5],      SD_CMD5);
            sd_transfer_reg = (xferCmdCode|START_EN);
            cr_writeb(sd_transfer_reg , SD_TRANSFER );   
     	    loops1 = 300; 

            #ifdef FOR_ICE_LOAD  
            prints("\ncard busy : retry cmd = 0x");
            print_hex(g_cmd[0]);
            prints("\n");
            #endif
        }
        else
            break;
        
        mdelay(5);
        loops--;
     }
    }
    else
    {
        //check1
        loops = 6000;
        err = CR_TRANSFER_TO;
        while(loops ){
		sync();
		sd_transfer_reg = cr_readb(SD_TRANSFER);
		sync();
        	if ((sd_transfer_reg & (END_STATE))==(END_STATE))
            {
        		break;
        	}
            udelay(500);
            loops--;
        }
	sync();
	sd_transfer_reg = cr_readb(SD_TRANSFER);
	sync();
       	if ((sd_transfer_reg & (ERR_STATUS))==(ERR_STATUS))
        {
            printf("\cmd=0x%02x, STATUS1=0x%02x, bIgnore=0x%02x\n", g_cmd[0], cr_readb(SD_STATUS1), bignore);
            if (bignore&&(g_cmd[0] == 0x59)&&((cr_readb(SD_STATUS1)==CRC7_STATUS)||(cr_readb(SD_STATUS1)==(CRC7_STATUS|0x8))))
            if (bignore && (g_cmd[0] == 0x59) && ((cr_readb(SD_STATUS1)==CRC7_STATUS)||(cr_readb(SD_STATUS1)==(CRC7_STATUS|0x8))))
            {
                    printf("\ignore cmd25 crc7 error\n");
                    return CR_TRANSFER_IGN;
            }
            else
            {
		#ifndef MMC_DEBUG
		if (gCurrentBootMode == 0)
		#endif
                	printf("\n0 sd transfer error (cmd/2193/status1/status2/bus_status/cfg1/cfg2/cfg3) : \n\t0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%02x 0x%02x 0x%02x\n",g_cmd[0], cr_readb(SD_TRANSFER), cr_readb(SD_STATUS1), cr_readb(SD_STATUS2), cr_readb(SD_BUS_STATUS),cr_readb(SD_CONFIGURE1),cr_readb(SD_CONFIGURE2),cr_readb(SD_CONFIGURE3));
        	    err = CR_TRANSFER_TO;
        		return err;
            }
       	}
	sync();
	sd_transfer_reg = cr_readb(SD_TRANSFER);
	sync();
       	if ((sd_transfer_reg & (IDLE_STATE))==(IDLE_STATE))
        {
        	    err = 0;
        }
        if ((err == CR_TRANSFER_TO)||(loops <= 0))
	{
  	  #ifndef MMC_DEBUG
	  if (gCurrentBootMode == 0)
	  #endif
            printf("\n1 sd transfer timeout error (cmd/2193/status1/status2/bus_status/cfg1/cfg2/cfg3) : \n\t0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%02x 0x%02x 0x%02x\n",g_cmd[0], cr_readb(SD_TRANSFER), cr_readb(SD_STATUS1), cr_readb(SD_STATUS2), cr_readb(SD_BUS_STATUS),cr_readb(SD_CONFIGURE1),cr_readb(SD_CONFIGURE2),cr_readb(SD_CONFIGURE3));
		return CR_TRANSFER_TO;
        }
    
            timeout_cnt = 0;
            while (timeout_cnt++ < 6000)
            {
		dma_val = REG32(CR_DMA_CTL3);
		sync();
            	if ((dma_val & DMA_XFER)!=DMA_XFER)
                {
                    return 0;
                }
		sync();
                udelay(500);
            }
	    if (timeout_cnt >= 6000)
	    {
		REG32(CR_DMA_CTL3) = 0;
		return CR_DMA_FAIL;
	    }

	    if (g_cmd[0] == 21)
	    {
		REG32(CR_DMA_CTL3) = (DAT64_SEL|DDR_WR);
		sync();
        	
		loops = 6000;
        	err = CR_TRANSFER_TO;
        	while(loops ){
		 sync();
		 sd_transfer_reg = cr_readb(SD_TRANSFER);
		 sync();
        	 if ((sd_transfer_reg & (END_STATE|IDLE_STATE))==(END_STATE|IDLE_STATE))
            	 {
        	    	err = 0;
        		break;
        	 }
            	 udelay(500);
            	 loops--;
        	}
		if (loops < 0)
	    	{
			return err;
	    	}
		sync();
		err = CR_DMA_FAIL;
            	timeout_cnt = 0;
            	while (timeout_cnt++ < 6000)
            	{
			dma_val = REG32(CR_DMA_CTL3);
			sync();
            		if ((dma_val & DMA_XFER)!=DMA_XFER)
                	{
                    		return 0;
                	}
			sync();
                	udelay(500);
            	}
	    	if (timeout_cnt >= 6000)
	    	{
			REG32(CR_DMA_CTL3) = 0;
			return CR_DMA_FAIL;
	    	}
	    }
            return 0;        
    }
   
    cr_int_status_reg = 0;
    if (err == CR_TRANSFER_TO)
	printf("[LY] cmd to : 0x%08x\n", g_cmd[0]); 
    return err;
}


/*******************************************************
 *
 *******************************************************/
UINT32 IsAddressSRAM(UINT32 addr)
{
    if ((addr<0x10000000) || (addr > 0x10003FFF))
        //ddr address
        return 0;
    else
        //secureRam addr
        return 1;
}

/*******************************************************
 *
 *******************************************************/
int mmccr_SendCmd( unsigned int cmd_idx,
                   unsigned int cmd_arg,
                   int rsp_para1,
                   int rsp_para2,
                   int rsp_para3,
                   void * rsp_buffer , int bIgnore)
{
    unsigned int rsp_len;
    volatile int ret_err;
    UINT32 sa;
    UINT32 byte_count = 0x200, block_count = 1;
    UINT32 cpu_mode=0;
    UINT32 retry_count=0, fake_cnt=0;
    e_device_type * card = &emmc_card;

    if (cmd_idx == 0x9)
    {
         sys_rsp17_addr += 0x200;       
    }
    sa = sys_rsp17_addr / 8;
    cpu_mode = IsAddressSRAM(sys_rsp17_addr);
    retry_count=0;
RET_CMD:
	rsp_len = mmc_get_rsp_len(rsp_para2);

    if( rtkmmc_debug_msg ) {
        #ifdef THIS_IS_FLASH_WRITE_U_ENV
        prints("cmd_idx=0x");
        print_hex(cmd_idx);
        prints(" cmd_arg=0x");
        print_hex(cmd_arg);
        prints(" rsp_para1=0x");
        print_hex(rsp_para1);
        prints(" rsp_para2=0x");
        print_hex(rsp_para2);
        prints(" rsp_para3=0x");
        print_hex(rsp_para3);
        prints(" rsp_len=0x");
        print_val(rsp_len, 2);
        prints("\n");
        #else
		MMCPRINTF("cmd_idx=0x%08x cmd_arg=0x%08x rsp_para1=0x%02x rsp_para2=0x%02x \n\trsp_para3=0x%02x rsp_len=0x%02x cpu_mode=0x%02x\n", cmd_idx, cmd_arg, rsp_para1,rsp_para2,rsp_para3,rsp_len,cpu_mode);
        #endif
	}

    if (rsp_para1 != -1)
        cr_writeb(rsp_para1,     SD_CONFIGURE1);
    cr_writeb(rsp_para2,     SD_CONFIGURE2);
    if (rsp_para3 != -1)
        cr_writeb((UINT8)rsp_para3,     SD_CONFIGURE3);

    g_cmd[0] = (0x40|cmd_idx);
    g_cmd[1] = (cmd_arg>>24)&0xff;
    g_cmd[2] = (cmd_arg>>16)&0xff;
    g_cmd[3] = (cmd_arg>>8)&0xff;
    g_cmd[4] = cmd_arg&0xff;
    g_cmd[5] = 0x00;

    #ifdef THIS_IS_FLASH_WRITE_U_ENV
    prints("cmd0:");
    print_val(g_cmd[0],2);
    prints(" cmd1:");
    print_val(g_cmd[1],2);
    prints(" cmd2:");
    print_val(g_cmd[2],2);
    prints(" cmd3:");
    print_val(g_cmd[3],2);
    prints(" cmd4:");
    print_val(g_cmd[4],2);
    prints(" cmd5:");
    print_val(g_cmd[5],2);
    prints("\n");
    #else
    MMCPRINTF("cmd0:0x%02x cmd1:0x%02x cmd2:0x%02x cmd3:0x%02x cmd4:0x%02x cmd5:0x%02x\n",g_cmd[0],g_cmd[1],g_cmd[2],g_cmd[3],g_cmd[4],g_cmd[5]);
    MMCPRINTF("cfg1:0x%02x, cfg2:0x%02x, cfg3:0x%02x\n", cr_readb(SD_CONFIGURE1),cr_readb(SD_CONFIGURE2),cr_readb(SD_CONFIGURE3));
    #endif

    sync();
    flush_cache((unsigned long)g_cmd, 6);
    //mdelay(10);
    
    cr_writeb(g_cmd[0], SD_CMD0);
    cr_writeb(g_cmd[1],  SD_CMD1);
    cr_writeb(g_cmd[2],  SD_CMD2);
    cr_writeb(g_cmd[3],   SD_CMD3);
    cr_writeb(g_cmd[4],      SD_CMD4);
    cr_writeb(g_cmd[5],      SD_CMD5);

    cr_writel( 0, CR_DMA_CTL1);
    cr_writel( 0, CR_DMA_CTL2);
    cr_writel( 0, CR_DMA_CTL3);  
    sync();

    if (RESP_TYPE_17B & rsp_para2)
    {        
    #ifdef THIS_IS_FLASH_WRITE_U_ENV
        prints("-----rsp 17B-----");
        prints(" DMA_sa=0x");
        print_hex(sa);
        prints(" DMA_len=0x");
        print_val(1, 2);
        prints(" DMA_setting=0x");
        print_hex(RSP17_SEL|DDR_WR|DMA_XFER);
    #else
        MMCPRINTF("-----rsp 17B-----\n DMA_sa=0x%08x DMA_len=0x%08x DMA_setting=0x%08x\n", sa,1,RSP17_SEL|DDR_WR|DMA_XFER);
    #endif
        cr_writeb(byte_count,       SD_BYTE_CNT_L);     //0x24
        cr_writeb(byte_count>>8,    SD_BYTE_CNT_H);     //0x28
        cr_writeb(block_count,      SD_BLOCK_CNT_L);    //0x2C
        cr_writeb(block_count>>8,   SD_BLOCK_CNT_H);    //0x30
	sync();

        if (cpu_mode)
            cr_writel( CPU_MODE_EN, CR_CPU_ACC); //enable cpu mode
        else
            cr_writel( 0, CR_CPU_ACC);
        cr_writel(sa, CR_DMA_CTL1);   //espeical for R2
        cr_writel(1, CR_DMA_CTL2);   //espeical for R2
	sync();
        cr_writel((RSP17_SEL|DDR_WR|DMA_XFER)&0x3f, CR_DMA_CTL3);   //espeical for R2
	sync();
    }        
    else if (RESP_TYPE_6B & rsp_para2)
    {   
        #ifdef THIS_IS_FLASH_WRITE_U_ENV
        prints("-----rsp 6B-----\n");
        #else
        MMCPRINTF("-----rsp 6B-----\n");
        #endif
        cr_writel( 0x00, CR_CPU_ACC);
    }

    MMCPRINTF("xfer_flag : %x, cfg1:0x%02x, cfg2:0x%02x, cfg3:0x%02x\n", SD_SENDCMDGETRSP|START_EN,cr_readb(SD_CONFIGURE1),cr_readb(SD_CONFIGURE2),cr_readb(SD_CONFIGURE3));

    flush_cache((unsigned long)rsp_buffer, 16);
    sync();
    ret_err = mmccr_TriggerXferCmd( SD_SENDCMDGETRSP, cpu_mode , bIgnore);
    if (ret_err == CR_TRANSFER_IGN)
    {
		error_handling(cmd_idx, bIgnore);
        ret_err = 0;
    }
#if 0
    if (cmd_idx == 0x1)
    {
	printf("[LY] fake start : %d \n",fake_cnt);
	if(fake_cnt++ < 4) ret_err=1;
    }
#endif

    if( !ret_err ){
        mmccr_read_rsp(rsp_buffer, rsp_len);
    }
    else
    {
        MMCPRINTF("case I : transfer cmd fail - 0x%08x\n", ret_err);
#if 0
	if (cmd_idx < MMC_CMD_SET_DSR)
	{
		printf("[LY] send cmd fail: cmd=%d\n",cmd_idx);
		sample_ctl_switch(cmd_idx);
		return ret_err;
	}
	else
	{
#endif
		#ifdef MMC_DEBUG
		printf("[LY]snd gCurrentBootMode =%d\n",gCurrentBootMode);
		#endif
		if (bIgnore)
			return ret_err;
		if (gCurrentBootMode >= MODE_DDR)
			return ret_err;
		if (cmd_idx == MMC_CMD_SEND_STATUS) //prevent dead lock looping
			return ret_err;
		if(retry_count++ < MAX_CMD_RETRY_COUNT)
		{
		   printf("Retry Cmd Cnt:%d\n",retry_count);
		   ret_err = error_handling(cmd_idx,bIgnore);
		   goto RET_CMD;
		}
		else
		   printf("Retry Cmd Cnt fail\n");

#if 0
    	}
#endif
    }
    return ret_err;
}

/*******************************************************
 *
 *******************************************************/
int error_handling(unsigned int cmd_idx, unsigned int bIgnore)
{
	unsigned char sts1_val=0;
	int err=0;
	struct mmc * mmc;
	e_device_type * card = &emmc_card;
	extern unsigned char g_ext_csd[];

		card_stop();
		sync();
		polling_to_tran_state(cmd_idx,1);
		sync();
		sts1_val = cr_readb(SD_STATUS1);
		sync();
		MMCPRINTF("[LY] status1 val=%02x\n", sts1_val);
		if ((sts1_val&WRT_ERR_BIT)||(sts1_val&CRC16_STATUS)||(sts1_val&CRC7_STATUS))
		{
			mmc_send_stop(card,1);
		}

		if (bIgnore)
			return 0;
		//till here, we are good to go
		if(gCurrentBootMode >= MODE_DDR)
		{
			printf("change mode from %d to %d ---> \n", gCurrentBootMode,MODE_SD20);
			mmc = find_mmc_device(0);
			flush_cache((unsigned long)g_ext_csd, CSD_ARRAY_SIZE);
			#ifdef MMC_DEBUG
			mmc_show_ext_csd(g_ext_csd);
			#endif
			if (gCurrentBootMode == MODE_SD30)
			{
				//ddr50
                		err = mmc_select_ddr50(mmc,g_ext_csd);
				mmc->boot_caps &= ~MMC_MODE_HSDDR_52MHz;
			}
			else 
			{	//sdr50
                		err = mmc_select_sdr50(mmc,g_ext_csd);
				mmc->boot_caps &= ~MMC_MODE_HS200;
			}
			if (err)
				printf(" change mode result ==> fail\n");
			else
			{
				printf(" change mode result ==> successful\n");
			}
		}
		else
		{
		   MMCPRINTF("%s : gCurrentBootMode = %d\n", __func__,gCurrentBootMode);
		   #if 0
		   switch(gCurrentBootMode)
		   {
			case MODE_SD20:
				err = sample_ctl_switch(cmd_idx);
				break;	
			case MODE_DDR:
				err = mmc_Tuning_DDR50();
				break;	
			case MODE_SD30:
				err = mmc_Tuning_HS200();
				break;	
		   }
		   #else
		   err = sample_ctl_switch(cmd_idx);
		   #endif
		}
		if (err)
			printf("[LY] error handling fail\n");
	return err;
}

/*******************************************************
 *
 *******************************************************/
int mmc_show_ocr( void * ocr )
{
    struct rtk_mmc_ocr_reg *ocr_ptr;
    struct rtk_mmc_ocr *ptr = (struct rtk_mmc_ocr *) ocr;
    ocr_ptr = (struct rtk_mmc_ocr_reg *)&ptr->reg;

#ifdef THIS_IS_FLASH_WRITE_U_ENV
    prints("emmc: OCR\n");
    prints(" - start bit : ");
    print_val(ptr->start, 1);
    prints("\n");
    prints(" - transmission bit : ");
    print_val(ptr->transmission, 1);
    prints("\n");
    prints(" - check bits : ");
    print_val(ptr->check1, 2);
    prints("\n");
    prints(" - OCR register : \n");
    prints("   - 1.7v to 1.95v : ");
    print_val(ocr_ptr->lowV, 1);
    prints("\n   - 2.0v to 2.6v : ");
    print_val(ocr_ptr->val1, 2);
    prints("\n   - 2.7v to 3.6v : ");
    print_val(ocr_ptr->val2, 2);
    prints("\n   - access mode : ");
    print_val(ocr_ptr->access_mode, 1);
    prints("\n   - power up status : ");
    print_val(ocr_ptr->power_up, 1);
    prints("\n");
    prints(" - check bits : ");
    print_val(ptr->check2, 2);
    prints("\n");
    prints(" - end bit : ");
    print_val(ptr->end, 1);
    prints("\n");
#else
    MMCPRINTF("emmc: OCR\n");
    MMCPRINTF("- start bit : 0x%x\n", ptr->start);
    MMCPRINTF(" - transmission bit : 0x%x\n", ptr->transmission);
    MMCPRINTF(" - check bit : 0x%x\n", ptr->check1);
    MMCPRINTF(" - OCR register : \n");
    MMCPRINTF("   - 1.7v to 1.95v : 0x%x\n", ocr_ptr->lowV);
    MMCPRINTF("   - 2.0v to 2.6v : 0x%x\n", ocr_ptr->val1);
    MMCPRINTF("   - 2.7v to 3.6v : 0x%x\n", ocr_ptr->val2);
    MMCPRINTF("   - access mode : 0x%x\n", ocr_ptr->access_mode);
    MMCPRINTF("   - power up status : 0x%x\n", ocr_ptr->power_up);
    MMCPRINTF(" - check bits : 0x%x\n", ptr->check2);
    MMCPRINTF(" - end bits : 0x%x\n", ptr->end);
#endif
}

/*******************************************************
 *
 *******************************************************/
int mmc_send_stop( e_device_type * card , int bIgnore)
{
    unsigned int rsp_buffer[4];

    MMCPRINTF("mmc_send_stop\n");

    return mmccr_SendCmd(MMC_STOP_TRANSMISSION, card->rca, -1, SD_R1|CRC16_CAL_DIS, -1, rsp_buffer, bIgnore);
}

/*******************************************************
 *
 *******************************************************/
int mmccr_Stream_Cmd( unsigned int xferCmdCode, struct rtk_cmd_info * cmd_info, unsigned int bIgnore )
{
    unsigned int cmd_idx      = cmd_info->cmd->cmdidx;
    unsigned int cmd_arg      = cmd_info->cmd->cmdarg;
    int rsp_para1     = cmd_info->rsp_para1;
    int rsp_para2     = cmd_info->rsp_para2;
    int rsp_para3     = cmd_info->rsp_para3;
    unsigned int rsp_len       = cmd_info->rsp_len;
    unsigned int * rsp        = cmd_info->cmd->response;
    unsigned int byte_count   = cmd_info->byte_count;
    unsigned int block_count  = cmd_info->block_count;
    unsigned int phy_buf_addr = (unsigned int) cmd_info->data_buffer;
    unsigned int timeout;
    e_device_type * card = &emmc_card;
    int ret_err=1;
    UINT32 cpu_mode=0;
    unsigned int retry_count=0;

    retry_count=0;

	if( rtkmmc_debug_msg ) {
	    RDPRINTF("cmd_idx=0x%08x arg=0x%08x rsp_para1=0x%02x rsp_para2=0x%02x \nrsp_para3=0x%02x"
	            "     byte_count=0x%04x; block_count=0x%04x; phy addr=0x%08x\n",
	            cmd_idx, cmd_arg, rsp_para1, rsp_para2, rsp_para3, byte_count, block_count, phy_buf_addr);

		switch( xferCmdCode ) {
			case SD_AUTOREAD1:		RDPRINTF("     XferCmdCode(08h)=0x%02x, SD_AUTOREAD1\n", xferCmdCode);	break;
			case SD_AUTOWRITE1:		RDPRINTF("     XferCmdCode(08h)=0x%02x, SD_AUTOWRITE1\n", xferCmdCode);	break;
			case SD_AUTOREAD2:		RDPRINTF("     XferCmdCode(08h)=0x%02x, SD_AUTOREAD2\n", xferCmdCode);	break;
			case SD_AUTOWRITE2:		RDPRINTF("     XferCmdCode(08h)=0x%02x, SD_AUTOWRITE2\n", xferCmdCode);	break;
			case SD_NORMALREAD:		RDPRINTF("     XferCmdCode(08h)=0x%02x, SD_NORMALREAD\n", xferCmdCode);	break;
			case SD_NORMALWRITE:		RDPRINTF("     XferCmdCode(08h)=0x%02x, SD_NORMALWRITE\n", xferCmdCode);	break;
			case SD_TUNING:			RDPRINTF("     XferCmdCode(08h)=0x%02x, SD_TUNING\n", xferCmdCode);	break;
			default:			RDPRINTF("     XferCmdCode(08h)=0x%02x, unknown\n", xferCmdCode);
		}
	}
STR_CMD_RET:
        #ifdef THIS_IS_FLASH_WRITE_U_ENV
        prints("cmd_idx=0x");
        print_hex(cmd_idx);
        prints(" cmd_arg=0x");
        print_hex(cmd_arg);
        prints(" rsp_para1=0x");
        print_hex(rsp_para1);
        prints(" rsp_para2=0x");
        print_hex(rsp_para2);
        prints(" rsp_para3=0x");
        print_hex(rsp_para3);
        prints("\n");
        prints(" byte_count=0x");
        print_hex(byte_count);
        prints(" block_count=0x");
        print_hex(block_count);
        prints("\n");
        #else
        RDPRINTF("cmd_idx=0x%08x arg=0x%08x rsp_para1=0x%02x rsp_para2=0x%02x \nrsp_para3=0x%02x"
	    "     byte_count=0x%04x; block_count=0x%04x; phy addr=0x%08x\n",
	    cmd_idx, cmd_arg, rsp_para1, rsp_para2, rsp_para3, byte_count, block_count, phy_buf_addr);
        #endif

    g_cmd[0] = (0x40|cmd_idx);
    g_cmd[1] = (cmd_arg>>24)&0xff;
    g_cmd[2] = (cmd_arg>>16)&0xff;
    g_cmd[3] = (cmd_arg>>8)&0xff;
    g_cmd[4] = cmd_arg&0xff;
    g_cmd[5] = 0x00;

    #ifdef THIS_IS_FLASH_WRITE_U_ENV
    prints("cmd0:");
    print_val(g_cmd[0],2);
    prints(" cmd1:");
    print_val(g_cmd[1],2);
    prints(" cmd2:");
    print_val(g_cmd[2],2);
    prints(" cmd3:");
    print_val(g_cmd[3],2);
    prints(" cmd4:");
    print_val(g_cmd[4],2);
    prints(" cmd5:");
    print_val(g_cmd[5],2);
    prints(" conf1:");
    print_val(rsp_para1&0xff,2);
    prints(" conf2:");
    print_val(rsp_para2&0xff,2);
    prints(" conf3:");
    print_val(rsp_para3&0xff,2);
    prints("\n");
    #else
    RDPRINTF("cmd0:0x%02x cmd1:0x%02x cmd2:0x%02x cmd3:0x%02x cmd4:0x%02x cmd5:0x%02x\n",g_cmd[0], g_cmd[1], g_cmd[2], g_cmd[3], g_cmd[4], g_cmd[5]);
    #endif

    cr_writeb(g_cmd[0], SD_CMD0);
    cr_writeb(g_cmd[1],  SD_CMD1);
    cr_writeb(g_cmd[2],  SD_CMD2);
    cr_writeb(g_cmd[3],   SD_CMD3);
    cr_writeb(g_cmd[4],      SD_CMD4);
    cr_writeb(g_cmd[5],      SD_CMD5);

    if (rsp_para1 != -1)
        cr_writeb(rsp_para1,       SD_CONFIGURE1);     //0x0C
    cr_writeb(rsp_para2,       SD_CONFIGURE2);     //0x0C
    if (rsp_para3 != -1)
        cr_writeb(rsp_para3,       SD_CONFIGURE3);     //0x0C
    cr_writeb((byte_count)&0xff,     SD_BYTE_CNT_L);     //0x24
    cr_writeb((byte_count>>8)&0xff,  SD_BYTE_CNT_H);     //0x28
    cr_writeb((block_count)&0xff,    SD_BLOCK_CNT_L);    //0x2C
    cr_writeb((block_count>>8)&0xff, SD_BLOCK_CNT_H);    //0x30
    sync();

    cr_writel( 0, CR_DMA_CTL1);
    cr_writel( 0, CR_DMA_CTL2);
    cr_writel( 0, CR_DMA_CTL3);  
    sync();

    #ifdef THIS_IS_FLASH_WRITE_U_ENV
    prints("xfer_flag : ");
    print_hex(xferCmdCode|START_EN);
    prints("\n");
    #else
    RDPRINTF("xfer_flag : %x, cfg1:0x%02x, cfg2:0x%02x, cfg3:0x%02x\n", xferCmdCode|START_EN,cr_readb(SD_CONFIGURE1),cr_readb(SD_CONFIGURE2),cr_readb(SD_CONFIGURE3));
    #endif

   	if(( cmd_info->xfer_flag == RTK_MMC_DATA_WRITE) || ( cmd_info->xfer_flag == MMC_CMD_MICRON_60 )){
        #ifdef THIS_IS_FLASH_WRITE_U_ENV
        prints("-----mmc data write-----\n");
        prints("DMA sa = 0x");
        print_hex(phy_buf_addr/8);
        prints("\nDMA len = 0x");
        print_hex(block_count);
        prints("\nDMA set = 0x");
        print_hex(DMA_XFER);
        prints("\n");
        #else
        RDPRINTF("-----mmc data write-----\n");
        RDPRINTF("DMA sa = 0x%x\nDMA len = 0x%x\nDMA set = 0x%x\n", phy_buf_addr/8, block_count, DMA_XFER);
        #endif

        cr_writel( 0, CR_CPU_ACC);
        cr_writel( phy_buf_addr/8, CR_DMA_CTL1);
        cr_writel( block_count, CR_DMA_CTL2);
	sync();
        cr_writel( DMA_XFER, CR_DMA_CTL3);  
	sync();
    }
    else if (cmd_info->xfer_flag == RTK_MMC_TUNING)
    {
            RDPRINTF("-----mmc data ddr tuning-----\n");
            cr_writel( 0x00, CR_CPU_ACC);
            cr_writel( phy_buf_addr/8, CR_DMA_CTL1);
            RDPRINTF("DMA sa = 0x%x\nDMA len = 0x%x\nDMA set = 0x%x\n", phy_buf_addr/8, block_count, DAT64_SEL|DDR_WR|DMA_XFER);
            cr_writel( block_count, CR_DMA_CTL2);
	    sync();
            cr_writel( DAT64_SEL|DDR_WR|DMA_XFER, CR_DMA_CTL3);
	    sync();
    }
    else{
        if( cmd_info->xfer_flag == RTK_MMC_SRAM_READ){
            #ifdef THIS_IS_FLASH_WRITE_U_ENV
            prints("-----mmc data sram read (cpu mode)-----\n");
            prints("CR_CPU_ACC(0x18012080) = 0x");
            print_hex(CPU_MODE_EN);
            prints("\n");
            #else
            RDPRINTF("-----mmc data sram read (cpu mode)-----\n");
            RDPRINTF("CR_CPU_ACC(0x18012080) = 0x%x\n", CPU_MODE_EN);
            #endif

            if (cmd_idx == MMC_READ_MULTIPLE_BLOCK)
                cpu_mode = 1;
            cr_writel( 0, CR_CPU_ACC);
            cr_writel( CPU_MODE_EN, CR_CPU_ACC);
            cr_writel( phy_buf_addr/8, CR_DMA_CTL1);
        }
        else
        {
            #ifdef THIS_IS_FLASH_WRITE_U_ENV
            prints("-----mmc data ddr read-----\n");
            #else
            RDPRINTF("-----mmc data ddr read-----\n");
            #endif
            cr_writel( 0x00, CR_CPU_ACC);
            cr_writel( phy_buf_addr/8, CR_DMA_CTL1);
        }
        #ifdef THIS_IS_FLASH_WRITE_U_ENV
        prints("DMA sa = 0x");
        print_hex(phy_buf_addr/8);
        prints("\nDMA len = 0x");
        print_hex(block_count);
        prints("\nDMA set = 0x");
        print_hex(DDR_WR|DMA_XFER);
        prints("\n");
        #else
        RDPRINTF("DMA sa = 0x%x\nDMA len = 0x%x\nDMA set = 0x%x\n", phy_buf_addr/8, block_count, DDR_WR|DMA_XFER);
        #endif
        cr_writel( block_count, CR_DMA_CTL2);
	sync();
        cr_writel( DDR_WR|DMA_XFER, CR_DMA_CTL3);
	sync();
    }

    sync();
    ret_err = mmccr_TriggerXferCmd( xferCmdCode, cpu_mode , bIgnore);
    if (ret_err == CR_TRANSFER_IGN)
    {
        error_handling(cmd_idx, bIgnore);
        ret_err = 0;
    }
#ifdef FORCE_CHG_SDR50_MODE
    //printf("hacker cmd_arg=%08x\n", cmd_arg);
    if (cmd_arg == 0x9999)
    {
	ret_err = -1;
    }
#endif

    if( !ret_err ){
        if( cr_int_status_reg & RTKCR_INT_DECODE_ERROR )
        {
            RDPRINTF("cr_int_status_reg=0x%x\n", cr_int_status_reg);
            ret_err = -1;
        }
        else{
            if( cmd_info->xfer_flag == RTK_MMC_SRAM_READ ) {
                if ((ret_err = mmccr_read_sram_dma_data(phy_buf_addr, block_count)) == ERR_EMMC_SRAM_DMA_TIME)
                    return ret_err;
                udelay(1000);
            }
            else
                mmccr_read_rsp(rsp, rsp_len);
		}
        RDPRINTF("---stream cmd done---\n");
    }
    else 
    {
        #ifdef THIS_IS_FLASH_WRITE_U_ENV
        prints("ret_err=");
        print_hex(ret_err);
        prints("\n");
        #else
    	RDPRINTF("strm cmd=%d,ret_err=%d\n",cmd_idx ,ret_err);
        #endif
		if (bIgnore)
			return ret_err;
	if (retry_count++ < MAX_CMD_RETRY_COUNT)
	{
		printf("Retry Cnt=%d\n",retry_count);
		ret_err = error_handling(cmd_idx, bIgnore);
		#ifdef FORCE_CHG_SDR50_MODE
		if ((cmd_arg == 0x9999)&&(!ret_err))
			return 0;
		#endif
		goto STR_CMD_RET;
	}
	else
		printf("Retry Cnt fail\n");
    }
    RDPRINTF("1 strm cmd=%d,ret_err=%d\n",cmd_idx ,ret_err);
    return ret_err;
}

/*******************************************************
 *
 *******************************************************/
int mmc_send_cmd8(unsigned int bIgnore)
{
	int ret_err=0;
	volatile int err=1,retry_cnt=5;
	int rsp_para1, rsp_para2, rsp_para3;
    	volatile struct rtk_cmd_info cmd_info;
	unsigned char cfg3=0,cfg1=0;
	struct mmc_cmd *cmd;
	struct mmc_cmd cmd_val;
	int sts1_val=0;
	e_device_type * card = &emmc_card;

	ALLOC_CACHE_ALIGN_BUFFER(char, crd_ext_csd,  CSD_ARRAY_SIZE);

	MMCPRINTF("\n*** %s %s %d, cmdidx=0x%02x(%d), resp_type=0x%08x -------\n", __FILE__, __FUNCTION__, __LINE__, cmd->cmdidx, cmd->cmdidx, cmd->resp_type);

	rsp_para1 = 0;
	rsp_para2 = 0;
	rsp_para3 = 0;
	cfg3 = cr_readb(SD_CONFIGURE3)|RESP_TIMEOUT_EN;
	cfg1 = cr_readb(SD_CONFIGURE1)&(MASK_CLOCK_DIV|MASK_BUS_WIDTH|MASK_MODE_SELECT);
	rsp_para1 = cfg1|SD1_R0;
	rsp_para2 = SD_R1;
	rsp_para3 = cfg3;

	MMCPRINTF("*** %s %s %d ***\n", __FILE__, __FUNCTION__, __LINE__);
	cmd_info.cmd= &cmd_val;
	cmd_info.cmd->cmdarg=0;
	cmd_info.cmd->cmdidx = MMC_SEND_EXT_CSD;
	cmd_info.rsp_para1	  = rsp_para1;
	cmd_info.rsp_para2	  = rsp_para2;
	cmd_info.rsp_para3	  = rsp_para3;
	cmd_info.rsp_len	 = mmc_get_rsp_len(rsp_para2);
	cmd_info.byte_count  = 0x200;
	cmd_info.block_count = 1;
	cmd_info.data_buffer = crd_ext_csd;
	cmd_info.xfer_flag	 = RTK_MMC_DATA_READ; //dma the result to ddr
	sync();
	ret_err = mmccr_Stream_Cmd( SD_NORMALREAD, &cmd_info, 1);
	if (ret_err)
	{
		if (bIgnore)
			return ret_err;
		card_stop();
		sync();
		polling_to_tran_state(MMC_CMD_SEND_EXT_CSD,1);
		sync();
		sts1_val = cr_readb(SD_STATUS1);
		sync();
		MMCPRINTF("[LY] status1 val=%02x\n", sts1_val);
		if ((sts1_val&CRC16_STATUS)||(sts1_val&CRC7_STATUS))
		{
			mmc_send_stop(card,bIgnore);
		}
        }

	return ret_err;
}
int mmc_send_cmd21()
{
	int ret_err=0;
    	struct rtk_cmd_info cmd_info;
	struct mmc_cmd cmd_val;
	int sts1_val=0,sts2_val=0;
	e_device_type * card = &emmc_card;

	ALLOC_CACHE_ALIGN_BUFFER(char, crd_tmp_buffer,  0x400);

	memcpy(crd_tmp_buffer, (volatile unsigned int*)0x20000, 0x400);

	//MMCPRINTF("\n*** %s %s %d, cmdidx=0x%02x(%d), resp_type=0x%08x -------\n", __FILE__, __FUNCTION__, __LINE__, cmd->cmdidx, cmd->cmdidx, cmd->resp_type);
	MMCPRINTF("*** %s %s %d ***\n", __FILE__, __FUNCTION__, __LINE__);
	cmd_info.cmd= &cmd_val;
	cmd_info.cmd->cmdarg = 0x00;
	cmd_info.cmd->cmdidx = MMC_TUNING;
	cmd_info.rsp_para1	  = -1;
	cmd_info.rsp_para2	  = SD_R1;
	cmd_info.rsp_para3	  = -1;
	cmd_info.rsp_len	 = mmc_get_rsp_len(SD_R1);
	cmd_info.byte_count  = 0x80;
	cmd_info.block_count = 1;
	cmd_info.data_buffer = crd_tmp_buffer;
	cmd_info.xfer_flag	 = RTK_MMC_TUNING; 
	ret_err = mmccr_Stream_Cmd(SD_TUNING, &cmd_info, 1);
	if (ret_err)
	{
		if ((cr_readb(SD_TRANSFER) & (ERR_STATUS))==(ERR_STATUS))
		{
			sts1_val = cr_readb(SD_STATUS1);
			sts2_val = cr_readb(SD_STATUS2);
			sync();
				
		 	if (sts1_val&CRC7_STATUS)
		 	{
				ret_err = 98;
		 	}
		 	else if (sts1_val&CRC16_STATUS)
		 	{
				ret_err = 98;
		 	}
		 	else if (sts1_val&WRT_ERR_BIT)
		 	{
		 	}
		 	else if ((sts2_val&RSP_TIMEOUT)==RSP_TIMEOUT)
		 	{	 
				ret_err = 98;
		 	}

			card_stop();
			sync();
			polling_to_tran_state(MMC_CMD_TUNING,1);
			sync();
			sts1_val = cr_readb(SD_STATUS1);
			sync();
			MMCPRINTF("[LY] status1 val=%02x\n", sts1_val);
			if ((sts1_val&CRC16_STATUS)||(sts1_val&CRC7_STATUS))
			{
				mmc_send_stop(card,1);
			}
		}
		else
			printf("%s : cmd fail\n",__func__);
	}
	return ret_err;
}

int polling_to_tran_state(int cmd_idx, int bIgnore)
{
	e_device_type * card = &emmc_card;
	int err=1, retry_cnt=5;
	int ret_state=0;

                err=1;
                retry_cnt=5;
                while(retry_cnt-- && err)
                {
                        #ifdef MMC_DEBUG
                        err = mmccr_send_status(&ret_state,1,bIgnore);
			#else
                        err = mmccr_send_status(&ret_state,0,bIgnore);
			#endif
                }
		sync();
                if (ret_state == STATE_DATA)
                {
                        #ifdef MMC_DEBUG
                        printf("before send stop\n");
                        #endif
			if ((cmd_idx != MMC_READ_SINGLE_BLOCK)&&(cmd_idx != MMC_CMD_WRITE_SINGLE_BLOCK))
                        	mmc_send_stop(card,1);
                        err = mmccr_wait_status(STATE_TRAN,bIgnore);
                        #ifdef MMC_DEBUG
                        printf("before send stop: err=%d\n",err);
                        #endif
                }
                else if (ret_state == STATE_RCV)
                {
                        #ifdef MMC_DEBUG
                        printf("before send stop\n");
                        #endif
			if ((cmd_idx != MMC_READ_SINGLE_BLOCK)&&(cmd_idx != MMC_CMD_WRITE_SINGLE_BLOCK))
                        	mmc_send_stop(card,1);
                        err = mmccr_wait_status(STATE_TRAN,bIgnore);
                        #ifdef MMC_DEBUG
                        printf("before send stop: err=%d\n",err);
                        #endif
                }
	return err;
}

void card_stop(){
	MMCPRINTF("[LY] card_stop \n");
	mmc_CRT_reset();
	sync();
}
int mmc_send_cmd18()
{
	int ret_err=0;
    	struct rtk_cmd_info cmd_info;
	struct mmc_cmd cmd_val;
	e_device_type * card = &emmc_card;
	int sts1_val=0;

	ALLOC_CACHE_ALIGN_BUFFER(char, crd_tmp_buffer,  0x400);

	//memcpy(crd_tmp_buffer, (volatile unsigned int*)0x20000, 0x400);
	//flush_cache(crd_tmp_buffer, 0x400);

	//MMCPRINTF("\n*** %s %s %d, cmdidx=0x%02x(%d), resp_type=0x%08x -------\n", __FILE__, __FUNCTION__, __LINE__, cmd->cmdidx, cmd->cmdidx, cmd->resp_type);
	MMCPRINTF("*** %s %s %d ***\n", __FILE__, __FUNCTION__, __LINE__);
	cmd_info.cmd= &cmd_val;
	cmd_info.cmd->cmdarg = 0x100;
	cmd_info.cmd->cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	cmd_info.rsp_para1	  = -1;
	cmd_info.rsp_para2	  = SD_R1;
	cmd_info.rsp_para3	  = -1;
	cmd_info.rsp_len	 = mmc_get_rsp_len(SD_R1);
	cmd_info.byte_count  = 0x200;
	cmd_info.block_count = 2;
	cmd_info.data_buffer = crd_tmp_buffer;
	cmd_info.xfer_flag	 = RTK_MMC_DATA_READ; //dma the result to ddr
	ret_err = mmccr_Stream_Cmd(SD_AUTOREAD1, &cmd_info, 1);
	if (ret_err)
	{
		card_stop();
		sync();
		polling_to_tran_state(MMC_CMD_READ_MULTIPLE_BLOCK,1);
		sync();
		sts1_val = cr_readb(SD_STATUS1);
		sync();
		MMCPRINTF("[LY] status1 val=%02x\n", sts1_val);
		if ((sts1_val&WRT_ERR_BIT)||(sts1_val&CRC16_STATUS)||(sts1_val&CRC7_STATUS)||(sts1_val&WRT_CRC_TO_ERR))
		{
			mmc_send_stop(card,1);
		}
	}
	return ret_err;
}
int mmc_send_cmd13()
{
	int ret_err=0;
	int sts_val=0;
	e_device_type * card = &emmc_card;

	#ifdef MMC_DEBUG
	ret_err = mmccr_send_status(&sts_val, 1, 0);
	#else
	ret_err = mmccr_send_status(&sts_val, 0, 0);
	#endif
	if (ret_err)
	{
		card_stop();
		sync();
		polling_to_tran_state(MMC_CMD_SEND_STATUS,1);
		sync();
		sts_val = cr_readb(SD_STATUS1);
		sync();
		MMCPRINTF("[LY] status1 val=%02x\n", sts_val);
		if ((sts_val&WRT_ERR_BIT)||(sts_val&CRC16_STATUS)||(sts_val&CRC7_STATUS))
		{
			mmc_send_stop(card,1);
		}
	}
	return ret_err;
}
int mmc_send_cmd25()
{
	int ret_err=0;
    	struct rtk_cmd_info cmd_info;
	struct mmc_cmd cmd_val;
	e_device_type * card = &emmc_card;
	int sts1_val=0;

	ALLOC_CACHE_ALIGN_BUFFER(char, crd_tmp_buffer,  0x400);

	memcpy(crd_tmp_buffer, (volatile unsigned int*)0x20000, 0x400);

	//MMCPRINTF("\n*** %s %s %d, cmdidx=0x%02x(%d), resp_type=0x%08x -------\n", __FILE__, __FUNCTION__, __LINE__, cmd->cmdidx, cmd->cmdidx, cmd->resp_type);
	MMCPRINTF("*** %s %s %d ***\n", __FILE__, __FUNCTION__, __LINE__);
	cmd_info.cmd= &cmd_val;
	cmd_info.cmd->cmdarg = 0xfe;
	cmd_info.cmd->cmdidx = MMC_CMD_WRITE_MULTIPLE_BLOCK;
	cmd_info.rsp_para1	  = -1;
	cmd_info.rsp_para2	  = SD_R1;
	cmd_info.rsp_para3	  = -1;
	cmd_info.rsp_len	 = mmc_get_rsp_len(SD_R1);
	cmd_info.byte_count  = 0x200;
	cmd_info.block_count = 2;
	cmd_info.data_buffer = crd_tmp_buffer;
	cmd_info.xfer_flag	 = RTK_MMC_DATA_WRITE; //dma the result to ddr
	ret_err = mmccr_Stream_Cmd(SD_AUTOWRITE1, &cmd_info, 1);
	if (ret_err)
	{
		card_stop();
		sync();
		polling_to_tran_state(MMC_CMD_WRITE_MULTIPLE_BLOCK,1);
		sync();
		sts1_val = cr_readb(SD_STATUS1);
		sync();
		MMCPRINTF("[LY] status1 val=%02x\n", sts1_val);
		if ((sts1_val&WRT_ERR_BIT)||(sts1_val&CRC16_STATUS)||(sts1_val&CRC7_STATUS))
		{
			mmc_send_stop(card,1);
		}
	}
	return ret_err;
}
int mmccr_Stream( unsigned int address,
                  unsigned int blk_cnt,
                  unsigned int xfer_flag,
                  unsigned int * data_buffer )
{
    struct rtk_cmd_info cmd_info;
    struct mmc_cmd  cmd;
    unsigned int xferCmdCode;
    unsigned int counter;
    int rw_cmd_err;
    int card_state;
    int err = -1;
    int send_stop_cmd = 0;
    unsigned int rsp_type;
    cmd_info.cmd = &cmd;
    cmd.cmdarg      = address;

    switch(xfer_flag)
    {
	case RTK_MMC_DATA_WRITE:
        	xferCmdCode = SD_AUTOWRITE2; // cmd + data
        	cmd.cmdidx = MMC_WRITE_BLOCK;
		break;
	case RTK_MMC_DATA_READ:
        	xferCmdCode = SD_AUTOREAD2; // cmd + data
        	cmd.cmdidx = MMC_READ_SINGLE_BLOCK;
		break;
	case MMC_CMD_MICRON_60:
        	xferCmdCode = SD_AUTOWRITE2; // cmd + data
        	cmd.cmdidx = MMC_CMD_MICRON_60;
		break;
	case MMC_CMD_MICRON_61:
        	xferCmdCode = SD_NORMALREAD; // cmd + data
        	cmd.cmdidx = MMC_CMD_MICRON_61;
		break;
    }

    /* multi sector accress opcode */
    if( blk_cnt > 1 ){
        xferCmdCode-=1;
	if ((xfer_flag == RTK_MMC_DATA_WRITE) || (xfer_flag == RTK_MMC_DATA_READ))
        	cmd.cmdidx++; // 25 or 18 ( multi_block_xxxx )
    }

    rsp_type = mmc_get_rsp_type(&cmd);

    cmd_info.rsp_para2    = rsp_type;
    switch(cmd.cmdidx)
    {
        case MMC_CMD_READ_SINGLE_BLOCK:
        case MMC_CMD_READ_MULTIPLE_BLOCK:
    	    cmd_info.rsp_para1    = -1;
            cmd_info.rsp_para3    = 0x01;
            break;
	case MMC_CMD_WRITE_SINGLE_BLOCK:
        case MMC_CMD_WRITE_MULTIPLE_BLOCK:
    	    cmd_info.rsp_para1    = -1;
            cmd_info.rsp_para3    = 0x01;
            break;
	case MMC_CMD_MICRON_60:
	case MMC_CMD_MICRON_61:
    	    cmd_info.rsp_para1    = -1;
            cmd_info.rsp_para3    = 0x1;
	    break;
        default:
            cmd_info.rsp_para3    = -1;
            break;
    }

    cmd_info.rsp_len     = mmc_get_rsp_len(rsp_type);
    cmd_info.byte_count  = 0x200;
    cmd_info.block_count = blk_cnt;
    cmd_info.data_buffer = data_buffer;
    cmd_info.xfer_flag   = xfer_flag;

    err = mmccr_Stream_Cmd( xferCmdCode, &cmd_info, 0);

    MMCPRINTF("[LY] exit mmccr_Stream_Cmd, ret=%d\n", err);

    //reset cmd0
    g_cmd[0] = 0x00;

    return err;
}

/*******************************************************
 *
 *******************************************************/
int mmc_show_cid( e_device_type *card )
{
	int i;
	unsigned int sn;
	unsigned char cid[16];
	for( i = 0; i < 4; i++ ) {
		cid[(i<<2)]   = ( card->raw_cid[i]>>24 ) & 0xFF;
		cid[(i<<2)+1] = ( card->raw_cid[i]>>16 ) & 0xFF;
		cid[(i<<2)+2] = ( card->raw_cid[i]>>8  ) & 0xFF;
		cid[(i<<2)+3] = ( card->raw_cid[i]     ) & 0xFF;
	}
#ifdef EMMC_SHOW_CID
    #ifdef THIS_IS_FLASH_WRITE_U_ENV
    prints("emmc:CID");
    #else
	MMCPRINTF("emmc:CID");
    #endif
	for( i = 0; i < 16; i++ ) {
        
        #ifdef THIS_IS_FLASH_WRITE_U_ENV
        print_hex(cid[i]);
        #else
		MMCPRINTF(" %02x", cid[i]);
        #endif
	}
    #ifdef THIS_IS_FLASH_WRITE_U_ENV
    prints("\n");
    #else
	MMCPRINTF("\n");
    #endif
#endif
    #ifdef THIS_IS_FLASH_WRITE_U_ENV
    prints("CID     0x");
    print_hex(cid[0]);
    prints("\n");
    #else
	MMCPRINTF("CID    0x%02x\n", cid[0]);
    #endif
	switch( (cid[1] & 0x3) ) {
		case 0:
            #ifdef THIS_IS_FLASH_WRITE_U_ENV
            prints("CBX    Card\n");
            #else
			MMCPRINTF("CBX    Card\n");
            #endif
            break;
		case 1:
            #ifdef THIS_IS_FLASH_WRITE_U_ENV
            prints("CBX    BGA\n");	
            #else
			MMCPRINTF("CBX    BGA\n");		
            #endif
            break;
		case 2:
            #ifdef THIS_IS_FLASH_WRITE_U_ENV
            prints("CBX    POP\n");
            #else
			MMCPRINTF("CBX    POP\n");		
            #endif
            break;
		case 3:
            #ifdef THIS_IS_FLASH_WRITE_U_ENV
            prints("CBX    Unknown\n");
            #else
			MMCPRINTF("CBX    Unknown\n");	
            #endif
            break;
	}
    #ifdef THIS_IS_FLASH_WRITE_U_ENV
    prints("OID    0x");
    print_val(cid[2],2);
    prints("\n");
    prints("PNM    0x");
    print_val(cid[3],2);
    print_val(cid[4],2);
    print_val(cid[5],2);
    print_val(cid[6],2);
    print_val(cid[7],2);
    print_val(cid[8],2);
    prints("\n");
    prints("PRV    0x");
    print_val((cid[9]>>4)&0xf,2);
    print_val(cid[9]&0xf,2);
    prints("\n");
    #else
	MMCPRINTF("OID    0x%02x\n", cid[2]);
	MMCPRINTF("PNM    %c%c%c%c%c%c\n", cid[3], cid[4], cid[5], cid[6], cid[7], cid[8]);
	MMCPRINTF("PRV    %d.%d\n", (cid[9]>>4)&0xf, cid[9]&0xf);
    #endif
	sn = cid[13];
	sn = (sn<<8) | cid[12];
	sn = (sn<<8) | cid[11];
	sn = (sn<<8) | cid[10];
    #ifdef THIS_IS_FLASH_WRITE_U_ENV
    prints("PSN    ");
    print_hex(sn);
    prints("\n");
    #else
	MMCPRINTF("PSN    %u(0x%08x)\n", sn, sn);
    #endif
	if( cid[9] ) {
        #ifdef THIS_IS_FLASH_WRITE_U_ENV
		prints("MDT    ");
        print_hex((cid[9]>>4)&0xf);
        print_hex((cid[14]&0xf)+1997);
        prints("\n");
        #else
		MMCPRINTF("MDT    %02d/%d)", (cid[9]>>4)&0xf, (cid[14]&0xf)+1997);
        #endif
	}
	else {
        #ifdef THIS_IS_FLASH_WRITE_U_ENV
        prints("MDT    --/----\n");
        #else
		MMCPRINTF("MDT    --/----\n", (cid[9]>>4)&0xf, (cid[14]&0xf)+1997);
        #endif
	}
}


/*******************************************************
 *
 *******************************************************/
int mmc_decode_csd( struct mmc* mmc )
{
    struct rtk_mmc_csd vcsd;
    struct rtk_mmc_csd *csd = (struct rtk_mmc_csd*)&vcsd;
    unsigned int e, m;
    unsigned int * resp = mmc->csd;
    int err = 0;

    printf("mmc_decode_csd\n");
    memset(&vcsd, 0x00, sizeof(struct rtk_mmc_csd));

  /*
     * We only understand CSD structure v1.1 and v1.2.
     * v1.2 has extra information in bits 15, 11 and 10.
     * We also support eMMC v4.4 & v4.41.
     */
    csd->csd_ver2 = 0xff;
    csd->csd_ver = UNSTUFF_BITS(resp, 126, 2);

    // 0, CSD Ver. 1.0
    // 1, CSD Ver. 1.1
    // 2, CSD Ver. 1.2, define in spec. 4.1-4.3
    // 3, CSD Ver define in EXT_CSD[194]
    //    EXT_CSD[194] 0, CSD Ver. 1.0
    //                 1, CSD Ver. 1.1
    //                 2, CSD Ver. 1.2, define in spec. 4.1-4.51
    //                 others, RSV

    csd->mmca_vsn       = UNSTUFF_BITS(resp, 122, 4);
    m = UNSTUFF_BITS(resp, 115, 4);
    e = UNSTUFF_BITS(resp, 112, 3);
    csd->tacc_ns        = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
    csd->tacc_clks      = UNSTUFF_BITS(resp, 104, 8) * 100;

    m = UNSTUFF_BITS(resp, 99, 4);
    e = UNSTUFF_BITS(resp, 96, 3);
    csd->max_dtr        = tran_exp[e] * tran_mant[m];
    csd->cmdclass       = UNSTUFF_BITS(resp, 84, 12);

    m = UNSTUFF_BITS(resp, 62, 12);
    e = UNSTUFF_BITS(resp, 47, 3);
    csd->capacity       = (1 + m) << (e + 2);
    csd->read_blkbits   = UNSTUFF_BITS(resp, 80, 4);
    csd->read_partial   = UNSTUFF_BITS(resp, 79, 1);
    csd->write_misalign = UNSTUFF_BITS(resp, 78, 1);
    csd->read_misalign  = UNSTUFF_BITS(resp, 77, 1);
    csd->r2w_factor     = UNSTUFF_BITS(resp, 26, 3);
    csd->write_blkbits  = UNSTUFF_BITS(resp, 22, 4);
    csd->write_partial  = UNSTUFF_BITS(resp, 21, 1);

    printf("CSD_STRUCTURE :%02x\n", csd->csd_ver);
    printf("SPEC_VERS   :%02x\n", csd->mmca_vsn);
    printf("TRAN_SPEED  :%02x\n", UNSTUFF_BITS(resp, 96, 8));
    printf("C_SIZE      :%08x\n",m);
    printf("C_SIZE_MULT :%08x\n",e);
    printf("Block Num   :%08x\n",csd->capacity);
    printf("Block Len   :%08x\n" ,1<<csd->read_blkbits);
    printf("Total Bytes :%08x\n",csd->capacity*(1<<csd->read_blkbits));
err_out:
    return err;
}

/*******************************************************
 *
 *******************************************************/
int mmc_decode_cid( struct mmc * rcard )
{
    e_device_type card;
    unsigned int * resp = rcard->cid;
    unsigned int * resp1 = rcard->csd;
    uint vsn = UNSTUFF_BITS(resp1, 122, 4);

    printf("mmc_decode_cid\n");
    /*
     * The selection of the format here is based upon published
     * specs from sandisk and from what people have reported.
     */
    switch (vsn) {
    case 0: /* MMC v1.0 - v1.2 */
    case 1: /* MMC v1.4 */
        card.cid.manfid        = UNSTUFF_BITS(resp, 104, 24);
        card.cid.prod_name[0]  = UNSTUFF_BITS(resp, 96, 8);
        card.cid.prod_name[1]  = UNSTUFF_BITS(resp, 88, 8);
        card.cid.prod_name[2]  = UNSTUFF_BITS(resp, 80, 8);
        card.cid.prod_name[3]  = UNSTUFF_BITS(resp, 72, 8);
        card.cid.prod_name[4]  = UNSTUFF_BITS(resp, 64, 8);
        card.cid.prod_name[5]  = UNSTUFF_BITS(resp, 56, 8);
        card.cid.prod_name[6]  = UNSTUFF_BITS(resp, 48, 8);
        card.cid.hwrev         = UNSTUFF_BITS(resp, 44, 4);
        card.cid.fwrev         = UNSTUFF_BITS(resp, 40, 4);
        card.cid.serial        = UNSTUFF_BITS(resp, 16, 24);
        card.cid.month         = UNSTUFF_BITS(resp, 12, 4);
        card.cid.year          = UNSTUFF_BITS(resp, 8,  4) + 1997;
        printf("cid v_1.0-1.4, manfid=%02x\n", card.cid.manfid);
        break;

    case 2: /* MMC v2.0 - v2.2 */
    case 3: /* MMC v3.1 - v3.3 */
    case 4: /* MMC v4 */
        card.cid.manfid        = UNSTUFF_BITS(resp, 120, 8);
        card.cid.oemid         = UNSTUFF_BITS(resp, 104, 16);
        card.cid.prod_name[0]  = UNSTUFF_BITS(resp, 96, 8);
        card.cid.prod_name[1]  = UNSTUFF_BITS(resp, 88, 8);
        card.cid.prod_name[2]  = UNSTUFF_BITS(resp, 80, 8);
        card.cid.prod_name[3]  = UNSTUFF_BITS(resp, 72, 8);
        card.cid.prod_name[4]  = UNSTUFF_BITS(resp, 64, 8);
        card.cid.prod_name[5]  = UNSTUFF_BITS(resp, 56, 8);
        card.cid.serial        = UNSTUFF_BITS(resp, 16, 32);
        card.cid.month         = UNSTUFF_BITS(resp, 12, 4);
        card.cid.year          = UNSTUFF_BITS(resp, 8, 4) + 1997;
        printf("cid v_2.0-4, manfid=%02x, cbx=%02x\n", card.cid.manfid, UNSTUFF_BITS(resp, 112, 2));
        break;

    default:
        printf("card has unknown MMCA version %d\n",
                card.csd.mmca_vsn);
        return -1;
    }

    return 0;
}

/*******************************************************
 *
 *******************************************************/
int mmc_show_csd( struct mmc* card )
{
	int i;
	unsigned char csd[16];
	for( i = 0; i < 4; i++ ) {
		csd[(i<<2)]   = ( card->csd[i]>>24 ) & 0xFF;
		csd[(i<<2)+1] = ( card->csd[i]>>16 ) & 0xFF;
		csd[(i<<2)+2] = ( card->csd[i]>>8  ) & 0xFF;
		csd[(i<<2)+3] = ( card->csd[i]     ) & 0xFF;
	}
#ifdef MMC_DEBUG
	printf("emmc:CSD(hex)");
	for( i = 0; i < 16; i++ ) {
		printf(" %02x", csd[i]);
	}
	printf("\n");
#endif
	mmc_decode_csd(card);
}

/*******************************************************
 *
 *******************************************************/
int mmc_show_ext_csd( unsigned char * pext_csd )
{
	int i,j,k;
	unsigned int sec_cnt;
	unsigned int boot_size_mult;

	printf("emmc:EXT CSD\n");
	k = 0;
	for( i = 0; i < 32; i++ ) {
		printf("    : %03x", i<<4);
		for( j = 0; j < 16; j++ ) {
			printf(" %02x ", pext_csd[k++]);
		}
		printf("\n");
	}
	printf("    :k=%02x\n",k);

	boot_size_mult = pext_csd[226];
    
	printf("emmc:BOOT PART %04x ",boot_size_mult<<7);
    printf(" Kbytes(%04x)\n", boot_size_mult);
    
	sec_cnt = pext_csd[215];
	sec_cnt = (sec_cnt<<8) | pext_csd[214];
	sec_cnt = (sec_cnt<<8) | pext_csd[213];
	sec_cnt = (sec_cnt<<8) | pext_csd[212];

	printf("emmc:SEC_COUNT %04x\n", sec_cnt);
	printf(" emmc:reserve227 %02x\n", pext_csd[227]);
	printf(" emmc:reserve240 %02x\n", pext_csd[240]);
	printf(" emmc:reserve254 %02x\n", pext_csd[254]);
	printf(" emmc:reserve256 %02x\n", pext_csd[256]);
	printf(" emmc:reserve493 %02x\n", pext_csd[493]);
	printf(" emmc:reserve505 %02x\n", pext_csd[505]);
}


/*******************************************************
 *
 *******************************************************/
int mmc_read_ext_csd( e_device_type * card )
{
    struct rtk_cmd_info cmd_info;
    struct mmc_cmd cmd;
    unsigned int xferCmdCode;
    unsigned int rsp_type;
    int ret_err;

    #ifdef THIS_IS_FLASH_WRITE_U_ENV
    prints("mmc_read_ext_csd\n");
    #else
    MMCPRINTF("mmc_read_ext_csd\n");
    #endif

    cmd_info.cmd= &cmd;
    cmd.cmdarg     = 0;
    cmd.cmdidx = MMC_SEND_EXT_CSD;

    rsp_type = mmc_get_rsp_type(&cmd);

    MMCPRINTF("ext_csd ptr 0x%p\n",sys_ext_csd_addr);

    cmd_info.rsp_para1    = 0x10;
    cmd_info.rsp_para2    = rsp_type;
    cmd_info.rsp_para3    = 0x3;
    cmd_info.rsp_len     = mmc_get_rsp_len(rsp_type);
    cmd_info.byte_count  = 0x200;
    cmd_info.block_count = 1;
    cmd_info.data_buffer = sys_ext_csd_addr;
    cmd_info.xfer_flag   = RTK_MMC_DATA_READ;
    xferCmdCode          = SD_NORMALREAD;

    ret_err = mmccr_Stream_Cmd( xferCmdCode, &cmd_info, 0);

    if(ret_err){
        #ifdef THIS_IS_FLASH_WRITE_U_ENV
        prints("unable to read EXT_CSD(ret_err=");
        print_hex(ret_err);
        prints("\n");
        #else
        MMCPRINTF("unable to read EXT_CSD(ret_err=%d)", ret_err);
        #endif
    }
    else{
    	mmc_show_ext_csd(sys_ext_csd_addr);

		card->ext_csd.boot_blk_size = (*(ptr_ext_csd+226)<<8);
        card->csd.csd_ver2 = *(ptr_ext_csd+194);
        card->ext_csd.rev = *(ptr_ext_csd+192);
        card->ext_csd.part_cfg = *(ptr_ext_csd+179);
        card->ext_csd.boot_cfg = *(ptr_ext_csd+178);
        card->ext_csd.boot_wp_sts = *(ptr_ext_csd+174);
        card->ext_csd.boot_wp = *(ptr_ext_csd+173);
		card->curr_part_indx = card->ext_csd.part_cfg & 0x07;

        #ifdef THIS_IS_FLASH_WRITE_U_ENV
		prints("emmc:BOOT PART MULTI = 0x");
        print_hex(*(ptr_ext_csd+226));
        prints(", BP_BLKS=0x");
        print_hex(*(ptr_ext_csd+226)<<8);
        prints("\n");
		prints("emmc:BOOT PART CFG = 0x");
        print_hex(card->ext_csd.part_cfg);
        prints("(0x");
        print_hex(card->curr_part_indx);
        prints(")\n");
        #else
		MMCPRINTF("emmc:BOOT PART MULTI = 0x%02x, BP_BLKS=0x%08x\n", *(ptr_ext_csd+226), *(ptr_ext_csd+226)<<8);
		MMCPRINTF("emmc:BOOT PART CFG = 0x%02x(%d)\n", card->ext_csd.part_cfg, card->curr_part_indx);
        #endif

		if (card->ext_csd.rev > 6) {
            #ifdef THIS_IS_FLASH_WRITE_U_ENV
            prints("unrecognized EXT_CSD structure version ");
            print_hex(card->ext_csd.rev);
            prints(", please update fw\n", card->ext_csd.rev);
            #else
            MMCPRINTF("unrecognized EXT_CSD structure version %d, please update fw\n", card->ext_csd.rev);
            #endif
        }

        if (card->ext_csd.rev >= 2) {
            card->ext_csd.sectors = *((unsigned int *)(ptr_ext_csd+EXT_CSD_SEC_CNT));
        }

        switch (*(ptr_ext_csd+EXT_CSD_CARD_TYPE) & (EXT_CSD_CARD_TYPE_26|EXT_CSD_CARD_TYPE_52)) {
	        case EXT_CSD_CARD_TYPE_52 | EXT_CSD_CARD_TYPE_26:
	            card->ext_csd.hs_max_dtr = 52000000;
	            break;
	        case EXT_CSD_CARD_TYPE_26:
	            card->ext_csd.hs_max_dtr = 26000000;
	            break;
	        default:
	            /* MMC v4 spec says this cannot happen */
                #ifdef THIS_IS_FLASH_WRITE_U_ENV
                prints("card is mmc v4 but does not support any high-speed modes.\n");
                #else
	            MMCPRINTF("card is mmc v4 but doesn't " "support any high-speed modes.\n");
                #endif
        }

        if (card->ext_csd.rev >= 3) {
            unsigned int sa_shift = *(ptr_ext_csd+EXT_CSD_S_A_TIMEOUT);
            /* Sleep / awake timeout in 100ns units */
            if (sa_shift > 0 && sa_shift <= 0x17){
                card->ext_csd.sa_timeout = 1 << *(ptr_ext_csd+EXT_CSD_S_A_TIMEOUT);
            }
        }
    }

    return ret_err;
}


/*******************************************************
 *
 *******************************************************/
int mmc_send_csd( e_device_type * card )
{
    MMCPRINTF("mmc_send_csd\n");

    /* Read CSD ==> CMD9 */
    return mmccr_SendCmd(MMC_SEND_CSD, emmc_card.rca, 0, SD_R2, SD2_R0, emmc_card.raw_csd,0);
}

/*******************************************************
 *
 *******************************************************/
int mmccr_hw_reset_signal( void )
{
	unsigned int tmp_mux;
	unsigned int tmp_dir;
	unsigned int tmp_dat;

	#define PIN_MUX_REG		(0xb8000814UL)
	#define PIN_DIR_REG		(0xb801bc00UL)
	#define PIN_DAT_REG		(0xb801bc18UL)

	tmp_mux = cr_readl( PIN_MUX_REG );
	tmp_dir = cr_readl( PIN_DIR_REG );
	tmp_dat = cr_readl( PIN_DAT_REG );

	cr_writel( tmp_dat|(1<<20), PIN_DAT_REG );
	cr_writel( tmp_dir|(1<<20), PIN_DIR_REG );
	cr_writel( tmp_dat|(1<<20), PIN_DAT_REG ); // high

	cr_writel( tmp_mux|(0xF<<28), PIN_MUX_REG );

	cr_writel( tmp_dat&~(1<<20), PIN_DAT_REG );  // low
	udelay(10);
	cr_writel( tmp_dat|(1<<20), PIN_DAT_REG ); // high

	cr_writel( tmp_mux, PIN_MUX_REG ); // restore original status

	return 0;
}

void set_emmc_pin_mux(void)
{
        //1195
        //set default i/f to cr
        unsigned int reg_val=0;
        reg_val = REG32(SYS_muxpad0);
    reg_val &= ~0xFFFF0FFF;
        reg_val |= 0xaaaa0aa8;
        REG32(SYS_muxpad0) = reg_val;
}

/*******************************************************
 *
 *******************************************************/
int mmccr_init_setup( void )
{   
    	set_emmc_pin_mux();
	#ifdef MMC_DEBUG 
	printf("[LY] gCurrentBootMode =%d\n",gCurrentBootMode);
	#endif
	gCurrentBootMode = MODE_SD20;
	rtkmmc_debug_msg = 1;
	rtkmmc_off_error_msg_in_init_flow = 1;
    	sys_rsp17_addr = S_RSP17_ADDR;   //set rsp buffer addr
    	sys_ext_csd_addr = S_EXT_CSD_ADDR;
    	ptr_ext_csd = (UINT8*)sys_ext_csd_addr;
	memset((struct backupRegs*)&gRegTbl, 0x00, sizeof(struct backupRegs));
	emmc_card.rca = 1<<16;
    	emmc_card.sector_addressing = 0;

    if (GET_CHIP_REV() >= PHOENIX_REV_B)
    {
	printf("rtk_emmc : Detect chip rev. >= B\n");
    	REG32(PLL_EMMC1) = 0xe0003;     //LDO1.8V
    	REG32(CR_PAD_CTL) = 0;              //PAD to 1.8V
    }

    #ifdef CONFIG_BOARD_FPGA_RTD1195_EMMC
    	cr_writel( 0x200000, DUMMY_SYS );
    #endif
    	cr_writeb( 0x2, CARD_SELECT );            //select SD

	udelay(100000);
	mmccr_set_pad_driving(MMC_IOS_SET_PAD_DRV, 0x66,0x64,0x66);
        cr_writeb(0, SD_SAMPLE_POINT_CTL);      //Sample point =output at falling edge
        cr_writeb(0, SD_PUSH_POINT_CTL);        //Push point =output at falling edge
    	mmccr_set_div(EMMC_INIT_CLOCK_DIV, 0);
	sync();
	return 0;
}

/*******************************************************
 *
 *******************************************************/
 //emmc init flow that for fast init purpose. (for romcode)
int mmc_initial( int reset_only )
{
    unsigned int tmp;
    unsigned int rsp_buffer[4];
    int ret_err=0,i;
    unsigned int counter=0;
    unsigned int arg=0;
    unsigned int cmd_retry_cnt=0;

    if (reset_only)
    {
        mmccr_init_setup();
	#ifdef MMC_DEBUG
    	printf("\nemmc : SYS_PLL_EMMC2 = 0x%08x",cr_readl(PLL_EMMC2));
    	printf("\nemmc : SYS_PLL_EMMC4 = 0x%08x",cr_readl(PLL_EMMC4));
    	printf("\nemmc : EMMC_CKGEN_CTL= 0x%08x",cr_readl(CR_CKGEN_CTL));
    	printf("\nemmc : SYS_NF_CKSEL = 0x%08x",cr_readl(NF_CKSEL));
    	printf("\nemmc : emmc pin mux = 0x%08x",cr_readl(0x1801a900));
	#endif
        return 0;
    }
}
void mmc_set_pad_driving(unsigned int mode, unsigned char CLOCK_DRIVING, unsigned char CMD_DRIVING, unsigned char DATA_DRIVING)
{
    static unsigned char card_pad_val=0,cmd_pad_val=0,data_pad_val=0;

    switch(mode)
    {
        case MMC_IOS_GET_PAD_DRV:
                card_pad_val = cr_readb(CARD_PAD_DRV);
                cmd_pad_val = cr_readb(CMD_PAD_DRV);
                data_pad_val = cr_readb(DATA_PAD_DRV);
                MMCPRINTF("backup card=0x%02x, cmd=0x%02x, data=0x%02x\n", card_pad_val, cmd_pad_val,data_pad_val);
                MMCPRINTF("read : card=0x%02x, cmd=0x%02x, data=0x%02x\n", cr_readb(CARD_PAD_DRV), cr_readb(CMD_PAD_DRV),cr_readb(DATA_PAD_DRV));
                break;
        case MMC_IOS_SET_PAD_DRV:
                cr_writeb(CLOCK_DRIVING, CARD_PAD_DRV); //clock pad driving
                cr_writeb(CMD_DRIVING, CMD_PAD_DRV);   //cmd pad driving
                cr_writeb(DATA_DRIVING, DATA_PAD_DRV);  //data pads driving
                sync();
                sync();
                sync();
                MMCPRINTF("set card=0x%02x, cmd=0x%02x, data=0x%02x\n", cr_readb(CARD_PAD_DRV), cr_readb(CMD_PAD_DRV),cr_readb(DATA_PAD_DRV));
                break;
        case MMC_IOS_RESTORE_PAD_DRV:
                cr_writeb(card_pad_val, CARD_PAD_DRV); //clock pad driving
                cr_writeb(cmd_pad_val, CMD_PAD_DRV);   //cmd pad driving
                cr_writeb(data_pad_val, DATA_PAD_DRV);  //data pads driving
                sync();
                sync();
                MMCPRINTF("val card=0x%02x, cmd=0x%02x, data=0x%02x\n", card_pad_val, cmd_pad_val,data_pad_val);
                MMCPRINTF("restore card=0x%02x, cmd=0x%02x, data=0x%02x\n", cr_readb(CARD_PAD_DRV), cr_readb(CMD_PAD_DRV),cr_readb(DATA_PAD_DRV));
                break;
    }
    sync();
    sync();
}

static void mmc_dump_registers()
{
    printf("%s : \n", __func__);
    printf("card_select=0x%02x \n", gRegTbl.card_select);
    printf("sample_point_ctl=0x%02x \n", gRegTbl.sample_point_ctl);
    printf("push_point_ctl=0x%02x \n", gRegTbl.push_point_ctl);
    printf("sys_pll_emmc3=0x%08x \n", gRegTbl.sys_pll_emmc3);
    printf("pll_emmc1=0x%08x \n", gRegTbl.pll_emmc1);
    printf("sd_configure1=0x%02x \n", gRegTbl.sd_configure1);
    printf("sd_configure2=0x%02x \n", gRegTbl.sd_configure2);
    printf("sd_configure3=0x%02x \n", gRegTbl.sd_configure3);
    printf("EMMC_CARD_PAD_DRV=0x%02x \n", cr_readb(CARD_PAD_DRV));
    printf("EMMC_CMD_PAD_DRV=0x%02x \n", cr_readb(CMD_PAD_DRV));
    printf("EMMC_DATA_PAD_DRV=0x%02x \n", cr_readb(DATA_PAD_DRV));
    sync();
}

static void mmc_restore_registers()
{
    //printf("%s : \n", __func__);
    mmc_set_pad_driving(MMC_IOS_RESTORE_PAD_DRV, 0x66,0x64,0x66);
    cr_writeb(gRegTbl.card_select, CARD_SELECT);
    cr_writeb(gRegTbl.sample_point_ctl, SD_SAMPLE_POINT_CTL);
    cr_writeb(gRegTbl.push_point_ctl, SD_PUSH_POINT_CTL);
    cr_writel(gRegTbl.sys_pll_emmc3, PLL_EMMC3);
    cr_writel(gRegTbl.pll_emmc1, PLL_EMMC1);
    cr_writeb(gRegTbl.sd_configure1, SD_CONFIGURE1);
    cr_writeb(gRegTbl.sd_configure2, SD_CONFIGURE2);
    cr_writeb(gRegTbl.sd_configure3, SD_CONFIGURE3);
    cr_writel(gRegTbl.emmc_pad_ctl, CR_PAD_CTL);
    cr_writel(gRegTbl.emmc_ckgen_ctl, CR_CKGEN_CTL);
    sync();
    //mmc_dump_registers();
}
static void mmc_backup_registers()
{
    //printf("%s : \n", __func__);
    mmc_set_pad_driving(MMC_IOS_GET_PAD_DRV, 0x66,0x64,0x66);
    gRegTbl.card_select      = cr_readb(CARD_SELECT);
    gRegTbl.sample_point_ctl = cr_readb(SD_SAMPLE_POINT_CTL);
    gRegTbl.push_point_ctl   = cr_readb(SD_PUSH_POINT_CTL);
    gRegTbl.sys_pll_emmc3    = cr_readl(PLL_EMMC3);
    gRegTbl.pll_emmc1        = cr_readl(PLL_EMMC1);
    //printf("%s : pll_emmc1=0x%08x, reg_pll_emmc1=0x%08x\n", __func__,gRegTbl.pll_emmc1,cr_readl(PLL_EMMC1));

    gRegTbl.sd_configure1    = cr_readb(SD_CONFIGURE1);
    gRegTbl.sd_configure2    = cr_readb(SD_CONFIGURE2);
    gRegTbl.sd_configure3    = cr_readb(SD_CONFIGURE3);
    gRegTbl.emmc_pad_ctl     = cr_readl(CR_PAD_CTL);
    gRegTbl.emmc_ckgen_ctl   = cr_readl(CR_CKGEN_CTL);
    sync();
    //mmc_dump_registers();
}
void mmc_CRT_reset()
{
	sync();
        mmc_backup_registers();
	sync();
        cr_writel(cr_readl(SOFT_RESET2)&((u32)~(1<<11)), SOFT_RESET2);          //reset emmc
        cr_writel(cr_readl(CLOCK_ENABLE1)&((u32)~(1<<24)), CLOCK_ENABLE1);      //disable emmc clk
	sync();
        cr_writel(cr_readl(CLOCK_ENABLE1)|(1<<24), CLOCK_ENABLE1);      //disable emmc clk
        cr_writel(cr_readl(SOFT_RESET2)|(1<<11), SOFT_RESET2);          //reset emmc
        sync();
        mmc_set_pad_driving(MMC_IOS_SET_PAD_DRV, 0xff,0xff,0xff);
        sync();
        mmc_restore_registers();
        sync();
}

void mmc_Phase_Adjust(UINT32 VP0, UINT32 VP1)
{
//phase selection
	if((VP0==0xff) & (VP1==0xff)){
		#ifdef MMC_DEBUG		
		MMCPRINTF("Phase VP0 and VP1 no change \n");
		#endif	
		}
	else if((VP0!=0xff) & (VP1==0xff)){
		#ifdef MMC_DEBUG		
		MMCPRINTF("Phase VP0=%x, VP1 no change \n", VP0);
		#endif	
		REG32(PLL_EMMC1) &= 0xfffffffd;	//reset pll	
		REG32(PLL_EMMC1) = ((REG32(PLL_EMMC1)&0xffffff07)|(VP0<<3));	//vp0 phase:0x0~0x1f
		REG32(PLL_EMMC1) |= 0x2;		//release reset pll
		}
	else if((VP0==0xff) & (VP1!=0xff)){
		#ifdef MMC_DEBUG		
		MMCPRINTF("Phase VP0 no change, VP1=%x \n", VP1);
		#endif	
		REG32(PLL_EMMC1) &= 0xfffffffd;		//reset pll	
		REG32(PLL_EMMC1) = ((REG32(PLL_EMMC1)&0xffffe0ff)|(VP1<<8));	//vp0 phase:0x0~0x1f
		REG32(PLL_EMMC1) |= 0x2;			//release reset pll
		}
	else{
		#ifdef MMC_DEBUG		
		MMCPRINTF("Phase VP0=%x, VP1=%x \n", VP0, VP1);
		#endif	
		REG32(PLL_EMMC1) &= 0xfffffffd;		//reset pll	
		REG32(PLL_EMMC1) = ((REG32(PLL_EMMC1)&0xffffff07)|(VP0<<3));	//vp0 phase:0x0~0x1f
		REG32(PLL_EMMC1) = ((REG32(PLL_EMMC1)&0xffffe0ff)|(VP1<<8));	//vp0 phase:0x0~0x1f
		REG32(PLL_EMMC1) |= 0x2;			//release reset pll
		}
	udelay(300);
	sync();
	sync();
}

int SEARCH_BEST(UINT32 window)
{
	int i, j, k, max;
	int window_temp[32];
	int window_start[32];
	int window_end[32];	
	int window_max=0;	
	int window_best=0;	
	int parse_end=1;
	for( i=0; i<0x20; i++ ){
		window_temp[i]=0;	
		window_start[i]=0;	
		window_end[i]=-1;	
		}
	j=1;
	i=0;
	k=0;
	max=0;
	while((i<0x1f) && (k<0x1f)){	
		parse_end=0;
		for( i=window_end[j-1]+1; i<0x20; i++ ){
			if (((window>>i)&1)==1 ){
				window_start[j]=i;
//				parse_end=1;
//				printf("window_start=0x%x \n", window_start[j]);
				break;
				}
			}	
//		printf("i=0x%x \n", i);
		if( i==0x20){			
			break;
			}	
		for( k=window_start[j]+1; k<0x20; k++ ){
			if(((window>>k)&1)==0){
				window_end[j]=k-1;
				parse_end=1;
	//			printf("test \n");
				break;			
				}
			}
		if(parse_end==0){
			window_end[j]=0x1f;
			}				
//			printf("window_end=0x%x \n", window_end[j]);
			j++;
		}	
	for(i=1; i<j; i++){
		window_temp[i]= window_end[i]-window_start[i]+1;
//		printf("window temp=0x%x \n", window_temp[i]);			
		}	
	if(((window&1)==1)&&(((window>>0x1f)&1)==1))
	{
		window_temp[1]=window_temp[1]+window_temp[j-1];
		window_start[1]=window_start[j-1];
//		printf("merge \n");			
		}
	for(i=1; i<j; i++){
		if(window_temp[i]>window_max){
			window_max=window_temp[i];
			max=i;
			}
		}
	if((((window&1)==1)&&(((window>>0x1f)&1)==1))&&(max==1)){
		window_best=((window_start[max]+window_end[max]+0x20)/2)&0x1f;
		
		}
	else {
		window_best=((window_start[max]+	window_end[max])/2)&0x1f;	
		}	
	MMCPRINTF("window start=0x%x \n", window_start[max]);	
	MMCPRINTF("window end=0x%x \n", window_end[max]);	
	MMCPRINTF("window best=0x%x \n", window_best);	
	return window_best;
}	
int mmc_Tuning_HS200(){
	volatile UINT32 TX_window=0xffffffff;
	int TX_fail_start=-1;
	int TX_fail_end=0x20;
	int TX_pass_start=-1;
	int TX_pass_end=0x20;
	volatile int TX_best=0xff;	
	volatile UINT32 RX_window=0xffffffff;
	int RX_fail_start=-1;
	int RX_fail_end=0x20;
	int RX_pass_start=-1;
	int RX_pass_end=0x20;
	volatile int RX_best=0xff;	
	volatile int i=0;

	gCurrentBootMode= MODE_SD30;
	#ifdef MMC_DEBUG		
	printf("[LY]hs200 gCurrentBootMode =%d\n",gCurrentBootMode);
	#endif

	mmccr_set_ldo(0xaf, 0);
        mmccr_set_speed(0);        //no wrapper divider
	mmccr_set_pad_driving(MMC_IOS_SET_PAD_DRV, 0xff,0xff,0xff);
	mmc_Phase_Adjust(0, 0);	//VP0, VP1 phas	
	mdelay(5);
	sync();
	#ifdef MMC_DEBUG
	MMCPRINTF("==============Start HS200 TX Tuning ==================\n");
	#endif
	for(i=0x0; i<0x20; i++){
		mmc_Phase_Adjust(i, 0xff);
		#ifdef MMC_DEBUG		
		printf("phase =0x%x \n", i);
		#endif
		//if(mmc_send_cmd21() == 99)
		if(mmc_send_cmd25() != 0)
		{
			TX_window= TX_window&(~(1<<i));
		}
		//else if (mmc_send_cmd13()!=0)
		//	TX_window= TX_window&(~(1<<i));
	}
	TX_best = SEARCH_BEST(TX_window);
	#ifdef MMC_DEBUG
	MMCPRINTF("TX_WINDOW= 0x%x \n", TX_window);
	MMCPRINTF("TX phase fail from 0x%x to=0x%x \n", TX_fail_start, TX_fail_end);
	MMCPRINTF("TX phase pass from 0x%x to=0x%x \n", TX_pass_start, TX_pass_end);
	MMCPRINTF("TX_best= 0x%x \n", TX_best);
	#endif
	printf("TX_WINDOW=0x%x, TX_best=0x%x \n",TX_window, TX_best);
	if (TX_window == 0x0)
	{
		printf("[LY] HS200 TX tuning fail \n");
		return -1;
	}
	mmc_Phase_Adjust(TX_best, 0xff);
	#ifdef MMC_DEBUG
	MMCPRINTF("++++++++++++++++++ Start HS200 RX Tuning ++++++++++++++++++\n");
	#endif
	i=0;
	for(i=0x0; i<0x20; i++){
		mmc_Phase_Adjust(0xff, i);
		#ifdef MMC_DEBUG		
		MMCPRINTF("phase =0x%x \n", i);
		#endif
		//if(mmc_send_cmd21()==98)
		if(mmc_send_cmd18()!=0)
		{
			RX_window= RX_window&(~(1<<i));
			}
	}
	RX_best = SEARCH_BEST(RX_window);
	#ifdef MMC_DEBUG
	MMCPRINTF("RX_WINDOW= 0x%x \n", RX_window);
	MMCPRINTF("RX phase fail from 0x%x to=0x%x \n", RX_fail_start, RX_fail_end);
	MMCPRINTF("RX phase pass from 0x%x to=0x%x \n", RX_pass_start, RX_pass_end);
	MMCPRINTF("RX_best= 0x%x \n", RX_best);
	#endif
	printf("RX_WINDOW=0x%x, RX_best=0x%x \n",RX_window, RX_best);
	if (RX_window == 0x0)
	{
		printf("[LY] HS200 RX tuning fail \n");
		return -2;
	}
	mmc_Phase_Adjust( 0xff, RX_best);
	mmc_CRT_reset();

	return 0;
}
	
int mmc_Tuning_DDR50(){	
	volatile UINT32 TX_window=0xffffffff;
	int TX_fail_start=-1;
	int TX_fail_end=0x20;
	int TX_pass_start=-1;
	int TX_pass_end=0x20;
	volatile int TX_best=0xff;	
	volatile UINT32 RX_window=0xffffffff;
	int RX_fail_start=-1;
	int RX_fail_end=0x20;
	int RX_pass_start=-1;
	int RX_pass_end=0x20;
	volatile int RX_best=0xff;	
	volatile int i=0;
	volatile int ret_state=0;
	volatile int err = 0;
		
	//set current boot mode
	gCurrentBootMode = MODE_DDR;	
	#ifdef MMC_DEBUG
	printf("[LY]ddr50 gCurrentBootMode =%d\n",gCurrentBootMode);
	#endif

	mmccr_set_ldo(0x57,0); //50Mhz
	sync();
	mmccr_set_speed(0);    //no wrapper divider
	sync();
	mmccr_set_pad_driving(MMC_IOS_SET_PAD_DRV, 0x66,0x64,0x66);
	cr_writeb(0xa8, SD_SAMPLE_POINT_CTL);	//Using phase-shift clock for DDR mode command/data sample point selection
	cr_writeb(0x90, SD_PUSH_POINT_CTL);	//Using phase-shift clock for DDR mode command/data push point selection
	mdelay(5);
	sync();
	#ifdef MMC_DEBUG
	MMCPRINTF("==============Start DDR50 TX Tuning ==================\n");
	#endif
	for(i=0x0; i<0x20; i++){
		mmc_Phase_Adjust(i, 0xff);
		#ifdef MMC_DEBUG
		MMCPRINTF("phase (%d) - VP=0x%08x\n", i,REG32(PLL_EMMC1));
		#endif
		if (mmc_send_cmd25()!=0)
		{
				TX_window= TX_window&(~(1<<i));
		}
	}
	TX_best = SEARCH_BEST(TX_window);
	#ifdef MMC_DEBUG
	MMCPRINTF("TX_WINDOW= 0x%x \n", TX_window);
	MMCPRINTF("TX phase fail from 0x%x to=0x%x \n", TX_fail_start, TX_fail_end);
	MMCPRINTF("TX phase pass from 0x%x to=0x%x \n", TX_pass_start, TX_pass_end);
	MMCPRINTF("TX_best= 0x%x \n", TX_best);
	#endif
	printf("TX_WINDOW=0x%x, TX_best=0x%x \n",TX_window,TX_best);
	if (TX_window == 0x0)
	{
		printf("[LY] DDR50 TX tuning fail \n");
		return -1;
	}
	mmc_Phase_Adjust(TX_best, 0xff);

	sync();
	#ifdef MMC_DEBUG
	MMCPRINTF("++++++++++++++Start DDR50 RX Tuning ++++++++++++++++++\n");
	#endif
	i=0;
	for(i=0x0; i<0x20; i++){
		mmc_Phase_Adjust(0xff, i);
		#ifdef MMC_DEBUG		
		MMCPRINTF("phase =0x%x \n", i);
		#endif
		//if(mmc_send_cmd8(0)!=0)
		if(mmc_send_cmd18()!=0)
		{
			RX_window= RX_window&(~(1<<i));
		}
	}
	RX_best = SEARCH_BEST(RX_window);
	#ifdef MMC_DEBUG
	MMCPRINTF("RX_WINDOW= 0x%x \n", RX_window);
	MMCPRINTF("RX phase fail from 0x%x to=0x%x \n", RX_fail_start, RX_fail_end);
	MMCPRINTF("RX phase pass from 0x%x to=0x%x \n", RX_pass_start, RX_pass_end);
	MMCPRINTF("RX_best= 0x%x \n", RX_best);
	#endif
	printf("RX_WINDOW=0x%x, RX_best=0x%x \n",RX_window,RX_best);
	sync();
	if (RX_window == 0x0)
	{
		printf("[LY] DDR50 RX tuning fail \n");
		return -2;
	}
	mmc_Phase_Adjust( 0xff, RX_best);
	return 0;
}

int mmc_Select_SDR50_Push_Sample(){			
	volatile int err=0;
	volatile int ret_state=0;

	gCurrentBootMode = MODE_SD20;
	#ifdef MMC_DEBUG
	printf("[LY]sdr gCurrentBootMode =%d\n",gCurrentBootMode);
	#endif

	mmccr_set_ldo(0x57,0); //50Mhz
	mmccr_set_speed(0);    //no wrapper divider
      	mmccr_set_div(EMMC_NORMAL_CLOCK_DIV, 0);
	//mmccr_set_pad_driving(MMC_IOS_SET_PAD_DRV, 0,0,0);
	cr_writeb(0, SD_SAMPLE_POINT_CTL);	//Sample point =output at falling edge
	cr_writeb(0, SD_PUSH_POINT_CTL);	//Push point =output at falling edge
	mdelay(5);
	sync();	
	#ifdef MMC_DEBUG
	MMCPRINTF("==============Select SDR50 Push Point ==================\n");
	#endif
	if ((err = mmc_send_cmd25())==0)
	{
		MMCPRINTF("Output at FALLING edge of SDCLK \n");
	}
	else
	{
		cr_writeb(0x10, SD_PUSH_POINT_CTL);	//Push point =output at falling edge
		sync();
		if ((err = mmc_send_cmd25())==0)
		{
			MMCPRINTF("Output is ahead by 1/4 SDCLK period \n");		
		}
		else 
		{
			printf("sdr tuning : No good push point \n");
			return -1;			
		}
	}
	#ifdef MMC_DEBUG
	MMCPRINTF("++++++++++++++Select SDR50 Sample Point ++++++++++++++++++\n");
	#endif
	if ((err = mmc_send_cmd18())==0)
	{
			MMCPRINTF("Sample at RISING edge of SDCLK \n");		
	}
	else {
		cr_writeb(8, SD_SAMPLE_POINT_CTL);	//sample point is delayed by 1/4 SDCLK period
		sync();
		if ((err = mmc_send_cmd18())==0)
		{
			MMCPRINTF("sample point is delayed by 1/4 SDCLK period \n");
		}
		else {
			printf("sdr tuning : No good Sample point \n");
			return -2;					
		}
	}
	sync();	
	printf("SDR select (sample/push) : 0x%02x/0x%02x\n", cr_readb(SD_SAMPLE_POINT_CTL), cr_readb(SD_PUSH_POINT_CTL));

	return 0;
}

int sample_ctl_switch(int cmd_idx)
{
	int err=0;
	UINT8 ret_state=0;
	e_device_type * card = &emmc_card;
	int retry_cnt=5;
	int mode_val=0;
	int sts1_val=0;

	mode_val = mmccr_get_mode_selection();
	sync();
	printf("cur point: s:0x%02x, p:0x%02x, mode=0x%02x\n",cr_readb(SD_SAMPLE_POINT_CTL),cr_readb(SD_PUSH_POINT_CTL),mode_val);
		switch(bErrorRetry)
		{
			case 0:
                    		cr_writeb( 0x0, SD_SAMPLE_POINT_CTL );    //sample point = SDCLK / 4
                    		cr_writeb( 0x10, SD_PUSH_POINT_CTL );     //output ahead SDCLK /4 
                    		printf("mode switch 0x0/0x10\n");
				bErrorRetry++;
				break;
			case 1:
                    		cr_writeb( 0x8, SD_SAMPLE_POINT_CTL );    //sample point = SDCLK / 4
                    		cr_writeb( 0x10, SD_PUSH_POINT_CTL );     //output ahead SDCLK /4 
                    		printf("mode switch 0x8/0x10\n");
				bErrorRetry++;
				break;
			case 2:
                    		cr_writeb( 0x8, SD_SAMPLE_POINT_CTL );    //sample point = SDCLK / 4
                    		cr_writeb( 0x0, SD_PUSH_POINT_CTL );     //output ahead SDCLK /4 
                    		printf("mode switch 0x8/0x0\n");
				bErrorRetry++;
				break;
			case 3:
                    		cr_writeb( 0x0, SD_SAMPLE_POINT_CTL );    //sample point = SDCLK / 4
                    		cr_writeb( 0x0, SD_PUSH_POINT_CTL );     //output ahead SDCLK /4 
                    		printf("mode switch 0x0/0x0\n");
				bErrorRetry=0;
				break;
		}
                mdelay(5);
		sync();
                REG8(CR_CARD_STOP) = 0x14;
                mdelay(5);
		sync();

	if((cmd_idx != MMC_CMD_SET_BLOCKLEN)&&(cmd_idx > MMC_CMD_SET_RELATIVE_ADDR))
	{
		card_stop();
		sync();
		polling_to_tran_state(cmd_idx,1);
		sync();
		sts1_val = cr_readb(SD_STATUS1);
		sync();
		MMCPRINTF("[LY] status1 val=%02x\n", sts1_val);
		if ((sts1_val&WRT_ERR_BIT)||(sts1_val&CRC16_STATUS)||(sts1_val&CRC7_STATUS))
		{
			mmc_send_stop(card,1);
		}
	}

	return err;
}

/*******************************************************
 *
 *******************************************************/
int rtk_micron_eMMC_write( unsigned int blk_addr, unsigned int data_size, unsigned int * buffer )
{
    int ret_err = 0;
    unsigned int total_blk_cont;
    unsigned int curr_xfer_blk_cont;
    unsigned int address;
    unsigned int curr_blk_addr;
    unsigned int target_part_idx;
    e_device_type * card = &emmc_card;
    unsigned int counter;
    unsigned int rsp_buffer[4];
    UINT8 ret_state=0;
    UINT32 tar_dest=0, bRetry=0;
    UINT32 retry_cnt=0, retry_cnt1=0, retry_cnt2=0, retry_cnt3=0;
    int err = 0, err1=0;

    RDPRINTF("\nemmc:%s blk_addr=0x%08x, data_size=0x%08x, buffer=0x%08x, addressing=%d\n", __FUNCTION__, blk_addr, data_size, buffer, card->sector_addressing);

    total_blk_cont = data_size>>9;
    if(data_size & 0x1ff) {
        total_blk_cont+=1;
    }

	curr_blk_addr = blk_addr;
#ifndef FORCE_SECTOR_MODE
	if( card->sector_addressing ) {
		address = curr_blk_addr;
	}
	else {
		address = curr_blk_addr << 9;
	}
#else
    address = curr_blk_addr;
#endif

#ifdef EMMC_SPLIT_BLK
	while( total_blk_cont ) {
		if( total_blk_cont > EMMC_MAX_XFER_BLKCNT ) {
			curr_xfer_blk_cont = EMMC_MAX_XFER_BLKCNT;
		}
		else {
			curr_xfer_blk_cont = total_blk_cont;
		}

		flush_cache((unsigned long)buffer, curr_xfer_blk_cont << 9);
		ret_err = mmccr_Stream( address, curr_xfer_blk_cont, MMC_CMD_MICRON_60, buffer );

		if( ret_err ) {
			return 0;
		}
		total_blk_cont -= curr_xfer_blk_cont;
		buffer += (curr_xfer_blk_cont<<(9-2));
		address += curr_xfer_blk_cont;
	}

	total_blk_cont = data_size>>9;
    if( data_size & 0x1ff ) {
        total_blk_cont+=1;
    }

    return total_blk_cont;
#else
RETRY_RD_CMD:
    flush_cache((unsigned long)buffer, data_size);

    ret_err = mmccr_Stream( address, total_blk_cont, MMC_CMD_MICRON_60, buffer );  //tbd : not verify


    if (ret_err)
    {
        if (retry_cnt2++ < MAX_CMD_RETRY_COUNT)
	{
            sample_ctl_switch(-1);
            goto RETRY_RD_CMD;
	}
        return ret_err;
    }

    /* To wait status change complete */
    bRetry=0;
    retry_cnt=0;
    retry_cnt1=0;
    while(1)
    {
        err1 = mmccr_send_status(&ret_state,1,0);
        //1. if cmd sent error, try again
        if (err1)
        {
            #ifdef THIS_IS_FLASH_WRITE_U_ENV
	        prints("retry - case 1\n");
            #else
            MMCPRINTF("retry - case 1\n");
            #endif
            sample_ctl_switch(-1);
            if (retry_cnt++ > MAX_CMD_RETRY_COUNT*2)
                return ERR_EMMC_SEND_STATUS_RETRY_FAIL;
            mdelay(1);
            continue;
        }
        //2. get state
        if (ret_state != STATE_TRAN)
        {
            #ifdef THIS_IS_FLASH_WRITE_U_ENV
	        prints("retry - case 2\n");
            #else
            MMCPRINTF("retry - case 2\n");
            #endif
            bRetry = 1;
            if (retry_cnt1++ > MAX_CMD_RETRY_COUNT*2)
                return ERR_EMMC_SEND_RW_CMD_RETRY_FAIL;
            mmc_send_stop(&emmc_card,0);
            mdelay(1000);
        }
        else
        {
            //out peaceful
            if (bRetry == 0)
            {
                #ifdef THIS_IS_FLASH_WRITE_U_ENV
	            prints("retry - case 3\n");
                #else
                MMCPRINTF("retry - case 3\n");
                #endif
                return !ret_err ?  total_blk_cont : 0;
            }
            else
            {
                #ifdef THIS_IS_FLASH_WRITE_U_ENV
	            prints("retry - case 4\n");
                #else
                MMCPRINTF("retry - case 4\n");
                #endif
                retry_cnt2 = 0;
                if (retry_cnt3++ > MAX_CMD_RETRY_COUNT*2)
                    return ERR_EMMC_SEND_RW_CMD_RETRY_FAIL;
                goto RETRY_RD_CMD;
            }
        }
    }

#endif

    return !ret_err ?  total_blk_cont : 0;
}

/*******************************************************
 *
 *******************************************************/
int rtk_micron_eMMC_read( unsigned int blk_addr, unsigned int data_size, unsigned int * buffer )
{
    int ret_err = 0;
    unsigned int total_blk_cont;
    unsigned int curr_xfer_blk_cont;
    unsigned int address;
    unsigned int curr_blk_addr;
    unsigned int target_part_idx;
    e_device_type * card = &emmc_card;
    unsigned int counter;
    unsigned int rsp_buffer[4];
    UINT8 ret_state=0;
    UINT32 tar_dest=0, bRetry=0;
    UINT32 retry_cnt=0, retry_cnt1=0, retry_cnt2=0, retry_cnt3=0;
    int err = 0, err1=0;

    RDPRINTF("\nemmc:%s blk_addr=0x%08x, data_size=0x%08x, buffer=0x%08x, addressing=%d\n", __FUNCTION__, blk_addr, data_size, buffer, card->sector_addressing);

    total_blk_cont = data_size>>9;
    if( data_size & 0x1ff ) {
        total_blk_cont+=1;
    }

#ifndef FORCE_SECTOR_MODE
	if( card->sector_addressing ) {
		address = blk_addr;
	}
	else {
		address = blk_addr << 9;
	}
#else
    address = blk_addr;
#endif
#ifdef EMMC_SPLIT_BLK
	while( total_blk_cont ) {
		if( total_blk_cont > EMMC_MAX_XFER_BLKCNT ) {
			curr_xfer_blk_cont = EMMC_MAX_XFER_BLKCNT;
		}
		else {
			curr_xfer_blk_cont = total_blk_cont;
		}

		flush_cache((unsigned long)buffer, curr_xfer_blk_cont << 9);
        	ret_err = mmccr_Stream( address, curr_xfer_blk_cont, MMC_CMD_MICRON_61, buffer );
        
		if( ret_err ) {
			return 0;
		}
		total_blk_cont -= curr_xfer_blk_cont;
		buffer += (curr_xfer_blk_cont<<(9-2));
		address += curr_xfer_blk_cont;

		//EXECUTE_CUSTOMIZE_FUNC(0); // insert execute customer callback at here

	}

	total_blk_cont = data_size>>9;
    if( data_size & 0x1ff ) {
        total_blk_cont+=1;
    }
	return total_blk_cont;
#else
RETRY_RD_CMD:
    flush_cache((unsigned long)buffer, data_size);

    ret_err = mmccr_Stream( address, total_blk_cont, MMC_CMD_MICRON_61, buffer );


    if (ret_err)
    {
        if (retry_cnt2++ < MAX_CMD_RETRY_COUNT*2)
	{
            sample_ctl_switch(-1);
            goto RETRY_RD_CMD;
	}
        return ret_err;
    }
    
    /* To wait status change complete */
    bRetry=0;
    retry_cnt=0;
    retry_cnt1=0;
    while(1)
    {
        err1 = mmccr_send_status(&ret_state,1,0);
        //1. if cmd sent error, try again
        if (err1)
        {
            sample_ctl_switch(-1);
            if (retry_cnt++ > MAX_CMD_RETRY_COUNT*2)
                return ERR_EMMC_SEND_STATUS_RETRY_FAIL;
            mdelay(1);
            continue;
        }
        //2. get state
        if (ret_state != STATE_TRAN) 
        {
            bRetry = 1;
            if (retry_cnt1++ > MAX_CMD_RETRY_COUNT*2)
                return ERR_EMMC_SEND_RW_CMD_RETRY_FAIL;
            mmc_send_stop(&emmc_card,0);
            mdelay(1000);
        }
        else
        {
            //out peaceful
            if (bRetry == 0)
                return !ret_err ?  total_blk_cont : 0;
            else
            {
                retry_cnt2 = 0;
                if (retry_cnt3++ > MAX_CMD_RETRY_COUNT*2)
                    return ERR_EMMC_SEND_RW_CMD_RETRY_FAIL;
                goto RETRY_RD_CMD;
            }
        }
    }
#endif

    return !ret_err ?  total_blk_cont : 0;
}

/*******************************************************
 *
 *******************************************************/
int rtk_eMMC_write( unsigned int blk_addr, unsigned int data_size, unsigned int * buffer )
{
    int ret_err = 0;
    unsigned int total_blk_cont;
    unsigned int curr_xfer_blk_cont;
    unsigned int address;
    unsigned int curr_blk_addr;
    unsigned int target_part_idx;
    e_device_type * card = &emmc_card;
    unsigned int counter;
    unsigned int rsp_buffer[4];
    UINT8 ret_state=0;
    UINT32 tar_dest=0, bRetry=0;
    UINT32 retry_cnt=0, retry_cnt1=0, retry_cnt2=0, retry_cnt3=0;
    int err = 0, err1=0;

    RDPRINTF("\nemmc:%s blk_addr=0x%08x, data_size=0x%08x, buffer=0x%08x, addressing=%d\n", __FUNCTION__, blk_addr, data_size, buffer, card->sector_addressing);

    if( !mmc_ready_to_use ) {
         MMCPRINTF("emmc: not ready to use\n");
    }

    total_blk_cont = data_size>>9;
    if(data_size & 0x1ff) {
        total_blk_cont+=1;
    }

	curr_blk_addr = blk_addr;
#ifndef FORCE_SECTOR_MODE
	if( card->sector_addressing ) {
		address = curr_blk_addr;
	}
	else {
		address = curr_blk_addr << 9;
	}
#else
    address = curr_blk_addr;
#endif

#ifdef EMMC_SPLIT_BLK
	while( total_blk_cont ) {
		if( total_blk_cont > EMMC_MAX_XFER_BLKCNT ) {
			curr_xfer_blk_cont = EMMC_MAX_XFER_BLKCNT;
		}
		else {
			curr_xfer_blk_cont = total_blk_cont;
		}

		flush_cache((unsigned long)buffer, curr_xfer_blk_cont << 9);
		ret_err = mmccr_Stream( address, curr_xfer_blk_cont, RTK_MMC_DATA_WRITE, buffer );

		if( ret_err ) {
                    	printf("%s - fail ....\n",__func__);
			return 0;
		}
		total_blk_cont -= curr_xfer_blk_cont;
		buffer += (curr_xfer_blk_cont<<(9-2));
		address += curr_xfer_blk_cont;
	}

	total_blk_cont = data_size>>9;
    if( data_size & 0x1ff ) {
        total_blk_cont+=1;
    }

    return total_blk_cont;
#else
RETRY_RD_CMD:
    flush_cache((unsigned long)buffer, data_size);

    ret_err = mmccr_Stream( address, total_blk_cont, RTK_MMC_DATA_WRITE, buffer );  //tbd : not verify


    if (ret_err)
    {
        if (retry_cnt2++ < MAX_CMD_RETRY_COUNT*2)
	{
            sample_ctl_switch(-1);
            goto RETRY_RD_CMD;
	}
        return ret_err;
    }

    /* To wait status change complete */
    bRetry=0;
    retry_cnt=0;
    retry_cnt1=0;
    while(1)
    {
        err1 = mmccr_send_status(&ret_state,1,0);
        //1. if cmd sent error, try again
        if (err1)
        {
            #ifdef THIS_IS_FLASH_WRITE_U_ENV
	        prints("retry - case 1\n");
            #else
            MMCPRINTF("retry - case 1\n");
            #endif
            sample_ctl_switch(-1);
            if (retry_cnt++ > MAX_CMD_RETRY_COUNT*2)
                return ERR_EMMC_SEND_STATUS_RETRY_FAIL;
            mdelay(1);
            continue;
        }
        //2. get state
        if (ret_state != STATE_TRAN)
        {
            #ifdef THIS_IS_FLASH_WRITE_U_ENV
	        prints("retry - case 2\n");
            #else
            MMCPRINTF("retry - case 2\n");
            #endif
            bRetry = 1;
            if (retry_cnt1++ > MAX_CMD_RETRY_COUNT*2)
                return ERR_EMMC_SEND_RW_CMD_RETRY_FAIL;
            mmc_send_stop(&emmc_card,0);
            mdelay(1000);
        }
        else
        {
            //out peaceful
            if (bRetry == 0)
            {
                #ifdef THIS_IS_FLASH_WRITE_U_ENV
	            prints("retry - case 3\n");
                #else
                MMCPRINTF("retry - case 3\n");
                #endif
                return !ret_err ?  total_blk_cont : 0;
            }
            else
            {
                #ifdef THIS_IS_FLASH_WRITE_U_ENV
	            prints("retry - case 4\n");
                #else
                MMCPRINTF("retry - case 4\n");
                #endif
                retry_cnt2 = 0;
                if (retry_cnt3++ > MAX_CMD_RETRY_COUNT*2)
                    return ERR_EMMC_SEND_RW_CMD_RETRY_FAIL;
                goto RETRY_RD_CMD;
            }
        }
    }

#endif

    return !ret_err ?  total_blk_cont : 0;
}

/*******************************************************
 *
 *******************************************************/
int rtk_eMMC_read( unsigned int blk_addr, unsigned int data_size, unsigned int * buffer )
{
    int ret_err = 0;
    unsigned int total_blk_cont;
    unsigned int curr_xfer_blk_cont;
    unsigned int address;
    unsigned int curr_blk_addr;
    unsigned int target_part_idx;
    e_device_type * card = &emmc_card;
    unsigned int counter;
    unsigned int rsp_buffer[4];
    UINT8 ret_state=0;
    UINT32 tar_dest=0, bRetry=0;
    UINT32 retry_cnt=0, retry_cnt1=0, retry_cnt2=0, retry_cnt3=0;
    int err = 0, err1=0;

    RDPRINTF("\nemmc:%s blk_addr=0x%08x, data_size=0x%08x, buffer=0x%08x, addressing=%d\n", __FUNCTION__, blk_addr, data_size, buffer, card->sector_addressing);

    if( !mmc_ready_to_use ) {
         RDPRINTF("emmc: not ready to use\n");
    }

    total_blk_cont = data_size>>9;
    if( data_size & 0x1ff ) {
        total_blk_cont+=1;
    }

#ifndef FORCE_SECTOR_MODE
	if( card->sector_addressing ) {
		address = blk_addr;
	}
	else {
		address = blk_addr << 9;
	}
#else
    address = blk_addr;
#endif
#ifdef EMMC_SPLIT_BLK
	while( total_blk_cont ) {
		if( total_blk_cont > EMMC_MAX_XFER_BLKCNT ) {
			curr_xfer_blk_cont = EMMC_MAX_XFER_BLKCNT;
		}
		else {
			curr_xfer_blk_cont = total_blk_cont;
		}

		flush_cache((unsigned long)buffer, curr_xfer_blk_cont << 9);
        if (IS_SRAM_ADDR(buffer))
        	ret_err = mmccr_Stream( address, curr_xfer_blk_cont, RTK_MMC_SRAM_READ, buffer ); 
        else
        	ret_err = mmccr_Stream( address, curr_xfer_blk_cont, RTK_MMC_DATA_READ, buffer );
        
		if( ret_err ) {
                    	printf("%s - fail ....\n",__func__);
			return 0;
		}
		total_blk_cont -= curr_xfer_blk_cont;
		buffer += (curr_xfer_blk_cont<<(9-2));
		address += curr_xfer_blk_cont;

		//EXECUTE_CUSTOMIZE_FUNC(0); // insert execute customer callback at here

	}

	total_blk_cont = data_size>>9;
    if( data_size & 0x1ff ) {
        total_blk_cont+=1;
    }
	return total_blk_cont;
#else
RETRY_RD_CMD:
    flush_cache((unsigned long)buffer, data_size);

    if (IS_SRAM_ADDR(buffer))
    	ret_err = mmccr_Stream( address, total_blk_cont, RTK_MMC_SRAM_READ, buffer ); 
    else
    	ret_err = mmccr_Stream( address, total_blk_cont, RTK_MMC_DATA_READ, buffer );


    if (ret_err)
    {
        if (retry_cnt2++ < MAX_CMD_RETRY_COUNT*2)
	{
            sample_ctl_switch(-1);
            goto RETRY_RD_CMD;
	}
        return ret_err;
    }
    
    /* To wait status change complete */
    bRetry=0;
    retry_cnt=0;
    retry_cnt1=0;
    while(1)
    {
        err1 = mmccr_send_status(&ret_state,1,0);
        //1. if cmd sent error, try again
        if (err1)
        {
            sample_ctl_switch(-1);
            if (retry_cnt++ > MAX_CMD_RETRY_COUNT*2)
                return ERR_EMMC_SEND_STATUS_RETRY_FAIL;
            mdelay(1);
            continue;
        }
        //2. get state
        if (ret_state != STATE_TRAN) 
        {
            bRetry = 1;
            if (retry_cnt1++ > MAX_CMD_RETRY_COUNT*2)
                return ERR_EMMC_SEND_RW_CMD_RETRY_FAIL;
            mmc_send_stop(&emmc_card,0);
            mdelay(1000);
        }
        else
        {
            //out peaceful
            if (bRetry == 0)
                return !ret_err ?  total_blk_cont : 0;
            else
            {
                retry_cnt2 = 0;
                if (retry_cnt3++ > MAX_CMD_RETRY_COUNT*2)
                    return ERR_EMMC_SEND_RW_CMD_RETRY_FAIL;
                goto RETRY_RD_CMD;
            }
        }
    }
#endif

    return !ret_err ?  total_blk_cont : 0;
}

/*******************************************************
 *
 *******************************************************/
int do_rtkmmc (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = CMD_RET_USAGE;
	unsigned int blk_addr, byte_size;
	void * mem_addr;

	if( argc == 5 ) {
		mem_addr   = (void *)simple_strtoul(argv[2], NULL, 16);
		blk_addr   = simple_strtoul(argv[3], NULL, 16);
		byte_size  = simple_strtoul(argv[4], NULL, 16);
		if( strncmp( argv[1], "read", 4 ) == 0 ) {
			ret = rtk_eMMC_read( blk_addr, byte_size, mem_addr);
			return ((ret==0) ? 1 : 0);
		}
		else if( strncmp( argv[1], "write", 5 ) == 0 ) {
			ret = rtk_eMMC_write( blk_addr, byte_size, mem_addr);
			return ((ret==0) ? 1 : 0);
		}
	}

	return  ret;	
}

/*******************************************************
 *
 *******************************************************/
U_BOOT_CMD(
	rtkmmc, 5, 0, do_rtkmmc,
	"RTK MMC direct read function",
	"          - rtkmmc read/write\n"
	"rtkmmc read addr blk_addr byte_size\n"
	"rtkmmc write addr blk_addr byte_size\n"
	""
);
