/*=============================================================
  * Copyright (c)      Realtek Semiconductor Corporation, 2004
  *
  * All rights reserved.
  *
  *============================================================*/

/*======================= Description ============================
  *
  * file: 		hdmiRxDrv.c
  *
  * author: 	Justin Chung
  * date:
  * version: 	0.0
  *
  *============================================================*/

/*========================Header Files===========================*/
#include "phoenix_hdmiInternal.h"
#include "phoenix_hdmiPlatform.h"
#include "phoenix_hdmiHWReg.h"
#include "phoenix_MHL_CBUS.h"
#include "phoenix_hdmi_wrapper.h"
#include "phoenix_rbusDFEReg.h"
#include <linux/delay.h>
#include <linux/string.h>
#include <asm/io.h>

//#define DEBUG_PRINTK		HDMI_PRINTF
#define DEBUG_PRINTK(...)
#ifndef IRQ_HANDLED
#define IRQ_HANDLED  1
#endif
#define RGBlane '3'
#define  ManualZ0 0x17
//================for sirius k offset=========================================

#define ALL_ADP_OPEN   1
#define  HBR_enbuffer_off   1
#define  LBR_enbuffer_off    2
#define  LBR_enbuffer_on    3
#define  MHL_3G_enbuffer_on   4
#define  HBR_enbuffer_on     5
#define Auto_EQ 1
#define foreground_range 2

UINT8  HBR_enbuffer_off_R,HBR_enbuffer_off_G,HBR_enbuffer_off_B;
UINT8  LBR_enbuffer_off_R,LBR_enbuffer_off_G,LBR_enbuffer_off_B;
UINT8  LBR_enbuffer_on_R,LBR_enbuffer_on_G,LBR_enbuffer_on_B;
UINT8  MHL_3G_enbuffer_on_R,MHL_3G_enbuffer_on_G,MHL_3G_enbuffer_on_B;
UINT8  HBR_enbuffer_on_R,HBR_enbuffer_on_G,HBR_enbuffer_on_B;

UINT8  HBR_enbuffer_off_R_range,HBR_enbuffer_off_G_range,HBR_enbuffer_off_B_range;
UINT8  LBR_enbuffer_off_R_range,LBR_enbuffer_off_G_range,LBR_enbuffer_off_B_range;
UINT8  LBR_enbuffer_on_R_range,LBR_enbuffer_on_G_range,LBR_enbuffer_on_B_range;
UINT8  MHL_3G_enbuffer_on_R_range,MHL_3G_enbuffer_on_G_range,MHL_3G_enbuffer_on_B_range;
UINT8  HBR_enbuffer_on_R_range,HBR_enbuffer_on_G_range,HBR_enbuffer_on_B_range;
//============================================================================
extern void __iomem *hdmi_rx_base[HDMI_RX_REG_BLOCK_NUM];
// 0: mca ; 1: phy ; 2: cec ; 3:clock detect
// 4: MHL; 5: MHL_CBUS ; 6: ISO DDC; 7: MISC DDC


//extern char * phoenix_hdmi_sa0;
//extern char * phoenix_hdmi_sa1;

//int*    phoenix_pli_getIOAddress(int addr)
//{
//    //return (int *)(addr-0xb8000000+start_addr+0x20000000);
//    return (int *)(addr-0xb8000000+0x20000000+0x20000000);
//
//}

unsigned int hdmi_rx_reg_read32 (unsigned int addr_offset, HDMI_RX_REG_BASE_TYPE type) {
	//return (*(volatile unsigned int *)( phoenix_pli_getIOAddress(addr)));
	//return (hdmi_rx_reg_read32(addr));
	return (readl(hdmi_rx_base[type] + addr_offset));
}

void hdmi_rx_reg_write32 (unsigned int addr_offset, unsigned int value, HDMI_RX_REG_BASE_TYPE type) {
	//*(volatile unsigned int *)(phoenix_pli_getIOAddress(addr)) = (value);
	//hdmi_rx_reg_write32(addr, value);
	writel(value, hdmi_rx_base[type] + addr_offset);
	if(0)//(type == HDMI_RX_PHY)
	{
	    //pr_info("[%x=%x]\n",hdmi_rx_base[type] + addr_offset,value);
	    HDMI_DELAYMS(1);
	}
}

void hdmi_rx_reg_mask32(unsigned int addr_offset, unsigned int andMask, unsigned int orMask, HDMI_RX_REG_BASE_TYPE type) {
	hdmi_rx_reg_write32(addr_offset, ((hdmi_rx_reg_read32(addr_offset, type) & (andMask)) | (orMask)), type);
}

/*======================== Definition and Marco =============================*/


/*======================== Global Variable ================================*/
HDMIRX_PHY_IDLE_PATCH_T hdmi_phy_idle_patch;
//HDMI_AUDIO_PLL_BUFFER_T audio_pll_buffer;
HDMIRX_IOCTL_STRUCT_T hdmi_ioctl_struct;

HDMI_CONST HDMI_PHY_PARAM_T hdmi_phy_param[] =
{
//RD SD suggestion
			//	M_code	  N_bypass  *CK_LDOA 		   CK_CS		       *EQ_manual		   CDR_KP 		          CDR_KI				    *EQ_bias               PR
			//		N_code	  MD_adj		   *CK_LDOD 	         CK_RS			  *CDR_bias		         *CDR_KP2 		     CDR_KD   *EQ_gain 		     CK_Icp

	{3319, 1896,  18,	  2,	   0,	    0,	0x3,		0x3,		0x3,		0x4,		0x2,		0x1,	   (0xf<<2),		0x0,		0x0,		 0x0,      0x0,		0x1 ,	0x6,    0x0,  "200M~350M half rate"},  // 200~350 M
	{ 1896, 948, 	18,	  2,	   0,	    1,	0x3,		0x3,		0x3,		0x4,		0x4,		0x1,	   (0xf<<2),		0x4,		0x0,		 0x0,      0x1,		0x1 ,	0x2,    0x0,  "100M~200M full rate"},	// 100~200 M
	{ 948, 474 ,   18,	  0,	   0,	    2,	0x3,		0x3,		0x3,		0x4,		0x9,		0x1,	   (0xc<<2),		0x0,		0x0,		 0x0,      0x1,		0x1 ,	0x6 ,   0x1,  "50M~100M full rate+DS1"},	// 50~100 M
	{ 474, 237,    18,	  0,	   1,	    2,	0x3,		0x3,		0x3,		0x4,		0x4,		0x1,	   (0xc<<2),		0x0,		0x0,		 0x0,      0x1,		0x1 ,	0x6,    0x2,  "25M~50M full rate+DS2"},	//25~50M
	{  237, 118,    38,	  0,	   1,	    2,	0x3,		0x3,		0x3,		0x4,		0x4,		0x1,	   (0xc<<2),		0x0,		0x0,		 0x0,      0x1,		0x1 ,	0x6,    0x3,  "12.5M~25M full rate+DS3"}, // 12.5~25M
//	{ 38,	0,	1,	5,	 0x3,		0x3,		0x4,		0x2,		0x4,		0x1,		0x1,		0x0,		       0x0,		 0x0,      0x1,		0x1 ,	0x4, "<12.5M or >350M unknow"}, // <12.5M or >350M
};

char Hdmi_IsbReady(void) {
	 //if( (REG_CK_MD_get_ck_md_ok(hdmi_rx_reg_read32(REG_CK_MD, HDMI_RX_PHY)) == 0)|((hdmi_rx_reg_read32(HDMI_CLKDETSR, HDMI_RX_CLOCK_DETECT)&HDMI_CLKDETSR_clk_in_target_mask) == 0)) {
	 if( (REG_CK_RESULT_get_ck_md_ok(hdmi_rx_reg_read32(HDMIPHY_REG_CK_RESULT, HDMI_RX_PHY)) == 0)) {
	  //HDMI_PRINTF("Hdmi MAC  clock not in target ");
			return FALSE;
	 } else {
			return TRUE;
	 }
}

void Hdmi_TriggerbMeasurement(void) {
	hdmi_rx_reg_mask32(HDMIPHY_REG_CK_RESULT,~REG_CK_RESULT_ck_md_ok_mask, REG_CK_RESULT_ck_md_ok_mask, HDMI_RX_PHY);			//cloud modify for magellan2013 0108  Restart measure b
	hdmi_rx_reg_mask32(HDMI_CLKDETCR,~_BIT0, 0, HDMI_RX_CLOCK_DETECT);			//PMM clk detect function disable
	hdmi_rx_reg_mask32(HDMI_CLKDETSR,~_BIT0, _BIT0, HDMI_RX_CLOCK_DETECT);			//clr PMM clk detect flag
	hdmi_rx_reg_mask32(HDMI_CLKDETCR,~_BIT0, _BIT0, HDMI_RX_CLOCK_DETECT);			//PMM clk detect function enable
}


int Hdmi_get_b_value(void) {
	UINT8  bTimeout;
	int wValue;

	hdmi_rx_reg_mask32(HDMIPHY_REG_CK_RESULT,~REG_CK_RESULT_ck_md_ok_mask,REG_CK_RESULT_ck_md_ok_mask, HDMI_RX_PHY);
	for(bTimeout = 5; bTimeout > 0; bTimeout--){
		if(REG_CK_RESULT_get_ck_md_ok(hdmi_rx_reg_read32(HDMIPHY_REG_CK_RESULT,HDMI_RX_PHY)))
			break;
		//pr_info("[wait Hdmi_get_b_value]\n");
		//HDMI_DELAYMS(1);
		usleep_range(500, 1000);
	}
	//while((!REG_CK_MD_get_ck_md_ok(hdmi_rx_reg_read32(REG_CK_MD, HDMI_RX_PHY)))&&(bTimeout));

	//HDMI_PRINTF("Hdmi_get_b_value = %d \n",bTimeout);
	if(bTimeout !=0)
	{
		wValue = REG_CK_RESULT_get_ck_md_count(hdmi_rx_reg_read32(HDMIPHY_REG_CK_RESULT, HDMI_RX_PHY));
		//REG_CK_MD_get_ck_md_count(hdmi_rx_reg_read32(REG_CK_MD, HDMI_RX_PHY));
	}
	else
	{
		wValue = 0;
	}
	return wValue ;  //cloud modify for magellan2013 0108
}


#if HDMI_DEBUG_AUDIO_PLL_RING

AUDIO_DEBUG_INFO_T audio_pll_ring_buffer[AUDIO_PLL_RING_SIZE];

HDMI_AUDIO_PLL_BUFFER_T audio_pll_buffer;

#endif
UINT8 AVMute_Timer = 0;




int rtd_hdmiRx_cmd(UINT8 cmd,  void * arg)
{
	switch((HDMIRX_IOCTL_T) cmd) {

			case IOCTRL_HDMI_PHY_START:
			{
                pr_info("hdmiRxDrv.o:    IOCTRL_HDMI_PHY_START\n");
//				hdmi_rx_reg_write32(TC_TMR5_VR_VADDR, 1);   			    // clear interrupt pending
//				hdmi_rx_reg_write32(TC_TMR5_CR_VADDR, _BIT31 | _BIT29);
				hdmi_ioctl_struct.b = 0;
				hdmi_ioctl_struct.b_change = 0;
				hdmi_ioctl_struct.b_pre = 0;
				hdmi_ioctl_struct.b_count = 0;
				hdmi_ioctl_struct.LG_patch_timer = 0;
				hdmi_phy_idle_patch.idle_count = 0;
				hdmi_ioctl_struct.hotplug_delay_count = 0;
			//	hdmi_rx_reg_mask32(HDMIPHY_REG_CK_5_8_reg ,~_BIT23,_BIT23);				//Freq mode auto detect enable
				hdmi_rx_reg_mask32(REG_CK_MD,~REG_CK_MD_ck_md_auto_mask,REG_CK_MD_ck_md_auto_mask, HDMI_RX_PHY);				//Freq mode auto detect enable
				hdmi_rx_reg_mask32(HDMI_CLKDETCR,~_BIT0,_BIT0, HDMI_RX_CLOCK_DETECT);				//PMM clk detect enable
				Hdmi_TriggerbMeasurement();
//				hdmi_rx_reg_mask32(BUS_SIC_M_GIE1_VADDR, ~(0x01 << 21), 1 << 21);

			}
			break;
			case IOCTRL_HDMI_PHY_STOP:
                DEBUG_PRINTK("hdmiRxDrv.o:  IOCTRL_HDMI_PHY_STOP\n");
//				hdmi_rx_reg_mask32(BUS_SIC_M_GIE1_VADDR, ~(0x01 << 21), 0 << 21);
//				hdmi_rx_reg_write32(TC_TMR5_CR_VADDR, 0);
				hdmi_ioctl_struct.b = 0;
				hdmi_ioctl_struct.b_change = 0;
				hdmi_ioctl_struct.b_pre = 0;
				hdmi_ioctl_struct.LG_patch_timer = 0;
				hdmi_phy_idle_patch.idle_count = 0;
				hdmi_ioctl_struct.hotplug_delay_count = 0;
				//hdmi_rx_reg_mask32(HDMIPHY_REG_CK_5_8_reg ,~_BIT23,0);				//Freq mode auto detect disable
				hdmi_rx_reg_mask32(REG_CK_MD,~REG_CK_MD_ck_md_auto_mask,0, HDMI_RX_PHY);//Freq mode auto detect disable
				hdmi_rx_reg_mask32(HDMI_CLKDETCR,~_BIT0,0, HDMI_RX_CLOCK_DETECT);				//PMM clk detect disable

			break;
			case IOCTRL_HDMI_GET_STRUCT:
			{
//				local_irq_disable();
				memcpy(arg, &hdmi_ioctl_struct, sizeof(HDMIRX_IOCTL_STRUCT_T));
				hdmi_ioctl_struct.b_change = 0;
//				pr_info("[IOCTRL_GET_HDMI_STRUCT %d,%d]\n", hdmi_ioctl_struct.b, ((HDMIRX_IOCTL_STRUCT_T *)arg)->b);
//				local_irq_enable();
				return 0;
			}
			break;
			case IOCTRL_HDMI_SET_TIMER:
			{
//				local_irq_disable();
				memcpy(&(hdmi_ioctl_struct.timer), arg, sizeof(hdmi_ioctl_struct.timer));
//				local_irq_enable();
                DEBUG_PRINTK("hdmiRxDrv.o:  IOCTRL_HDMI_SET_TIMER %d\n", hdmi_ioctl_struct.timer);
				return 0;
			}
			break;
			case IOCTRL_HDMI_GET_TIMER:
			{
                DEBUG_PRINTK("hdmiRxDrv.o:  IOCTRL_HDMI_GET_TIMER %d\n", hdmi_ioctl_struct.timer);
//				local_irq_disable();
				memcpy(arg, &(hdmi_ioctl_struct.timer), sizeof(hdmi_ioctl_struct.timer));
//				local_irq_enable();
			return 0;
			}
			break;
			case IOCTRL_HDMI_SET_APSTATUS:
			{
                DEBUG_PRINTK("hdmiRxDrv.o:  IOCTRL_HDMI_SET_APSTATUS %d\n", arg);
//				local_irq_disable();
				hdmi_phy_idle_patch.apstatus  = (HDMIRX_APSTATUS_T) (UINT32) arg;
				if (hdmi_phy_idle_patch.apstatus != HDMIRX_DETECT_FAIL) {
					hdmi_phy_idle_patch.idle_count = 0;
				}
//				local_irq_enable();

			}
			break;
#if HDMI_DEBUG_AUDIO_PLL_RING
			case IOCTRL_HDMI_AUDIO_PLL_SAMPLE_START:
			{
                    DEBUG_PRINTK("hdmiRxDrv.o:  IOCTRL_HDMI_AUDIO_PLL_SAMPLE_START\n");
//					local_irq_disable();
					if (audio_pll_buffer.buffer == 0) {
						audio_pll_buffer.buffer = (AUDIO_DEBUG_INFO_T *)malloc( AUDIO_PLL_RING_SIZE * sizeof(AUDIO_DEBUG_INFO_T));
					}
					audio_pll_buffer.ring_end = 0;
					audio_pll_buffer.ring_start = 0;
					audio_pll_buffer.enable = 1;
//					local_irq_enable();
					return 0;
			}
			break;

			case IOCTRL_HDMI_AUDIO_PLL_SAMPLE_STOP:
			{
                    DEBUG_PRINTK("hdmiRxDrv.o:  IOCTRL_HDMI_AUDIO_PLL_SAMPLE_STOP\n");
//					local_irq_disable();
					if (audio_pll_buffer.buffer != 0) {
						free(audio_pll_buffer.buffer);
						audio_pll_buffer.buffer = NULL;
					}
					audio_pll_buffer.ring_end = 0;
					audio_pll_buffer.ring_start = 0;
					audio_pll_buffer.enable = 0;
//					local_irq_enable();
					return 0;
			}
			break;

			case IOCTRL_HDMI_AUDIO_PLL_SAMPLE_DUMP:
			{
                    DEBUG_PRINTK("hdmiRxDrv.o:  IOCTRL_HDMI_AUDIO_PLL_SAMPLE_DUMP\n");
//					local_irq_disable();
					if (audio_pll_buffer.enable) {
							while(audio_pll_buffer.ring_start != audio_pll_buffer.ring_end) {
								HDMI_PRINTF("%d %d %d %d %d %d %d\n", audio_pll_buffer.buffer[audio_pll_buffer.ring_start].sum_c, audio_pll_buffer.buffer[audio_pll_buffer.ring_start].cts,
									audio_pll_buffer.buffer[audio_pll_buffer.ring_start].n,
									audio_pll_buffer.buffer[audio_pll_buffer.ring_start].audio_disable, audio_pll_buffer.buffer[audio_pll_buffer.ring_start].pll_unstable,
									audio_pll_buffer.buffer[audio_pll_buffer.ring_start].overflow,audio_pll_buffer.buffer[audio_pll_buffer.ring_start].underflow  );
								audio_pll_buffer.ring_start ++;
								if (audio_pll_buffer.ring_start == AUDIO_PLL_RING_SIZE)
									audio_pll_buffer.ring_start = 0;
							}
					}

//					local_irq_enable();
					return 0;
			}
			break;
#endif
			default:
				break;
	}

	return 0;
}

//***********************************************************************
//Function name :  HDMI_Set_Phy_Table()
//PARAM : NONE
//RETURN : Success 1  or Fail  0
//descript : This function is set for cbus detect and MHL flow ok
//***********************************************************************
void HDMI_Set_Phy_Table(int mode)
{
	// PHY parameter setting
	hdmi_rx_reg_mask32(HDMIPHY_REG_CK_1_4,~HDMIPHY_REG_CK_1_4_CK_DIVM_MASK,hdmi_phy_param[mode].M_code<<HDMIPHY_REG_CK_1_4_CK_DIVM_shift,HDMI_RX_PHY);		//M code set
	hdmi_rx_reg_mask32(HDMIPHY_REG_CK_1_4,~HDMIPHY_REG_CK_1_4_CK_DIVN_MASK,hdmi_phy_param[mode].N_code<<HDMIPHY_REG_CK_1_4_CK_DIVN_shift,HDMI_RX_PHY);		//N code set
	hdmi_rx_reg_mask32(HDMIPHY_REG_CK_1_4,~HDMIPHY_REG_CK_1_4_CK_DIVN_BYPASS,hdmi_phy_param[mode].N_bypass<<HDMIPHY_REG_CK_1_4_CK_DIVN_BYPASS_shift,HDMI_RX_PHY);		//N bypass

	//hdmi_rx_reg_mask32(HDMIPHY_REG_CK_5_8_reg,~ck_md_adj_mask,ck_md_adj(hdmi_phy_param[mode].MD_adj));		//CK Freq. mode set
	//for Sirius APHY bug
	hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~HDMIPHY_REG_R_5_8_reg_r_RATE_SEL_MASK,R_CDR_Rate((hdmi_phy_param[mode].MD_adj>3)?3:hdmi_phy_param[mode].MD_adj),HDMI_RX_PHY);		//R Freq. mode set
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~HDMIPHY_REG_G_5_8_reg_g_RATE_SEL_MASK,G_CDR_Rate((hdmi_phy_param[mode].MD_adj>3)?3:hdmi_phy_param[mode].MD_adj),HDMI_RX_PHY);		//G Freq. mode set
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~HDMIPHY_REG_B_5_8_reg_b_RATE_SEL_MASK,B_CDR_Rate((hdmi_phy_param[mode].MD_adj>3)?3:hdmi_phy_param[mode].MD_adj),HDMI_RX_PHY);		//B Freq. mode set
	//for Sirius APHY bug_end
	hdmi_rx_reg_mask32(HDMIPHY_REG_DIG_PHY_CTRL_NEW_1,~HDMIPHY_REG_DIG_PHY_CTRL_NEW_1_reg_cdr_rate_sel_mask,HDMIPHY_REG_DIG_PHY_CTRL_NEW_1_reg_cdr_rate_sel(hdmi_phy_param[mode].MD_adj),HDMI_RX_PHY); 	//DCDR mode set

	hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~R_divider_after_PR_mask,R_divider_after_PR(hdmi_phy_param[mode].PR),HDMI_RX_PHY);		//R PR set
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_9_12,~G_divider_after_PR_mask,G_divider_after_PR(hdmi_phy_param[mode].PR),HDMI_RX_PHY);		//G PR set
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_9_12,~B_divider_after_PR_mask,B_divider_after_PR(hdmi_phy_param[mode].PR),HDMI_RX_PHY);		//B PR set

	hdmi_rx_reg_mask32(HDMIPHY_REG_CK_1_4,~HDMIPHY_REG_CK_1_4_CK_SELCS_MASK,hdmi_phy_param[mode].CK_CS<<HDMIPHY_REG_CK_1_4_CK_SELCS_shift,HDMI_RX_PHY);		//CK CS set
	hdmi_rx_reg_mask32(HDMIPHY_REG_CK_5_8,~HDMIPHY_REG_CK_5_8_CK_RS,hdmi_phy_param[mode].CK_RS<<HDMIPHY_REG_CK_5_8_CK_RS_shift,HDMI_RX_PHY);					//CK RS set
	hdmi_rx_reg_mask32(HDMIPHY_REG_CK_5_8,~HDMIPHY_REG_CK_5_8_CK_ICP,hdmi_phy_param[mode].CK_Icp<<HDMIPHY_REG_CK_5_8_CK_ICP_shift,HDMI_RX_PHY);				//CK Icp set

	//hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12_reg,~HDMIPHY_REG_R_9_12_reg_R_CDR_SBIAS_MASK,hdmi_phy_param[mode].CDR_bias<<HDMIPHY_REG_R_9_12_reg_R_CDR_SBIAS_shift);		//R CDR bias SBIAS set
	//hdmi_rx_reg_mask32(HDMIPHY_REG_G_9_12_reg,~HDMIPHY_REG_G_9_12_reg_G_CDR_SBIAS_MASK,hdmi_phy_param[mode].CDR_bias<<HDMIPHY_REG_G_9_12_reg_G_CDR_SBIAS_shift);	//G CDR bias SBIAS set
	//hdmi_rx_reg_mask32(HDMIPHY_REG_B_9_12_reg,~HDMIPHY_REG_B_9_12_reg_B_CDR_SBIAS_MASK,hdmi_phy_param[mode].CDR_bias<<HDMIPHY_REG_B_9_12_reg_B_CDR_SBIAS_shift);		//B CDR bias SBIAS set

	hdmi_rx_reg_mask32(HDMIPHY_REG_DIG_PHY_CTRL_NEW_2,~HDMIPHY_REG_DIG_PHY_CTRL_NEW_2_reg_kp_mask,HDMIPHY_REG_DIG_PHY_CTRL_NEW_2_reg_kp(hdmi_phy_param[mode].CDR_KP),HDMI_RX_PHY);	//DCDR KP1
	hdmi_rx_reg_mask32(HDMIPHY_REG_DIG_PHY_CTRL_NEW_2,~HDMIPHY_REG_DIG_PHY_CTRL_NEW_2_reg_ki_mask,HDMIPHY_REG_DIG_PHY_CTRL_NEW_2_reg_ki(hdmi_phy_param[mode].CDR_KI),HDMI_RX_PHY); 	//DCDR KI
}


//***********************************************************************
//Function name :  void OPEN_RST_MAC(void)
//PARAM : tmds clock
//RETURN : none
//descript :release MAC reset
//***********************************************************************
void OPEN_RST_MAC(void)
{
	hdmi_rx_reg_mask32(HDMI_TMDS_Z0CC_VADDR,~TMDS_Z0CC_hde_mask,TMDS_Z0CC_hde(1),HDMI_RX_MAC);
	udelay(100);
	//HDMI_PRINTF("0xb800d01c=0x%x--MAC reset\n",rtd_inl(TMDS_PWDCTL_reg));
	hdmi_rx_reg_mask32(HDMI_TMDS_PWDCTL_VADDR,~(TMDS_PWDCTL_ebip_mask|TMDS_PWDCTL_egip_mask|TMDS_PWDCTL_erip_mask|TMDS_PWDCTL_ecc_mask),(TMDS_PWDCTL_ebip(1)|TMDS_PWDCTL_egip(1)|TMDS_PWDCTL_erip(1)|TMDS_PWDCTL_ecc(1)),HDMI_RX_MAC);                         //enable TMDS input
	//HDMI_PRINTF("0xb800d01c=0x%x--MAC set\n",rtd_inl(TMDS_PWDCTL_reg));
	hdmi_rx_reg_mask32(HDMI_TMDS_DPC_SET0_VADDR,~(TMDS_DPC_SET0_dpc_en_mask),TMDS_DPC_SET0_dpc_en_mask,HDMI_RX_MAC);
}

void EQ3_Enable_buffer_Setting(UINT8 enable)
{
   if (enable ==1)
   {
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~HDMIPHY_REG_R_5_8_enbuffer3_mask, HDMIPHY_REG_R_5_8_enbuffer3(1),HDMI_RX_PHY); // 20140304
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~HDMIPHY_REG_G_5_8_enbuffer3_mask, HDMIPHY_REG_G_5_8_enbuffer3(1),HDMI_RX_PHY); // 20140304
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~HDMIPHY_REG_B_5_8_enbuffer3_mask, HDMIPHY_REG_B_5_8_enbuffer3(1),HDMI_RX_PHY); // 20140304
   }
   else
   {
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~HDMIPHY_REG_R_5_8_enbuffer3_mask, HDMIPHY_REG_R_5_8_enbuffer3(0),HDMI_RX_PHY); // 20140304
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~HDMIPHY_REG_G_5_8_enbuffer3_mask, HDMIPHY_REG_G_5_8_enbuffer3(0),HDMI_RX_PHY); // 20140304
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~HDMIPHY_REG_B_5_8_enbuffer3_mask, HDMIPHY_REG_B_5_8_enbuffer3(0),HDMI_RX_PHY); // 20140304

   }
}

void EQ_Enable_buffer_bit22(UINT8 enable)
{
	if ( enable )
	{
		hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~HDMIPHY_REG_R_5_8_bit22_mask, HDMIPHY_REG_R_5_8_bit22(1),HDMI_RX_PHY);
		hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~HDMIPHY_REG_G_5_8_bit22_mask, HDMIPHY_REG_G_5_8_bit22(1),HDMI_RX_PHY);
		hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~HDMIPHY_REG_B_5_8_bit22_mask, HDMIPHY_REG_B_5_8_bit22(1),HDMI_RX_PHY);
	}
	else
	{
	    hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~HDMIPHY_REG_R_5_8_bit22_mask, HDMIPHY_REG_R_5_8_bit22(0),HDMI_RX_PHY);
		hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~HDMIPHY_REG_G_5_8_bit22_mask, HDMIPHY_REG_G_5_8_bit22(0),HDMI_RX_PHY);
		hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~HDMIPHY_REG_B_5_8_bit22_mask, HDMIPHY_REG_B_5_8_bit22(0),HDMI_RX_PHY);
	}
}

void FORGROUND_OFFSET_MODE(UINT8 mode)
{
    if (mode == HBR_enbuffer_off)
    {
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~(R_KOFF_FG_Manual_mode_mask),R_KOFF_FG_Manual(HBR_enbuffer_off_R),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~(G_KOFF_FG_Manual_mode_mask),G_KOFF_FG_Manual(HBR_enbuffer_off_G),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(B_KOFF_FG_Manual_mode_mask),B_KOFF_FG_Manual(HBR_enbuffer_off_B),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~HDMIPHY_REG_R_1_4_R_KOFF_RANG,R_KOFF_FG_Range(HBR_enbuffer_off_R_range),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~HDMIPHY_REG_G_1_4_G_KOFF_RANG,G_KOFF_FG_Range(HBR_enbuffer_off_G_range),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~HDMIPHY_REG_B_1_4_B_KOFF_RANG,B_KOFF_FG_Range(HBR_enbuffer_off_B_range),HDMI_RX_PHY);
    }
    else if (mode == LBR_enbuffer_off)
    {
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~(R_KOFF_FG_Manual_mode_mask),R_KOFF_FG_Manual(LBR_enbuffer_off_R),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~(G_KOFF_FG_Manual_mode_mask),G_KOFF_FG_Manual(LBR_enbuffer_off_G),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(B_KOFF_FG_Manual_mode_mask),B_KOFF_FG_Manual(LBR_enbuffer_off_B),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~HDMIPHY_REG_R_1_4_R_KOFF_RANG,R_KOFF_FG_Range(LBR_enbuffer_off_R_range),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~HDMIPHY_REG_G_1_4_G_KOFF_RANG,G_KOFF_FG_Range(LBR_enbuffer_off_G_range),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~HDMIPHY_REG_B_1_4_B_KOFF_RANG,B_KOFF_FG_Range(LBR_enbuffer_off_B_range),HDMI_RX_PHY);
    }
    else if (mode == LBR_enbuffer_on)
    {
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~(R_KOFF_FG_Manual_mode_mask),R_KOFF_FG_Manual(LBR_enbuffer_on_R),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~(G_KOFF_FG_Manual_mode_mask),G_KOFF_FG_Manual(LBR_enbuffer_on_G),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(B_KOFF_FG_Manual_mode_mask),B_KOFF_FG_Manual(LBR_enbuffer_on_B),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~HDMIPHY_REG_R_1_4_R_KOFF_RANG,R_KOFF_FG_Range(LBR_enbuffer_on_R_range),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~HDMIPHY_REG_G_1_4_G_KOFF_RANG,G_KOFF_FG_Range(LBR_enbuffer_on_G_range),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~HDMIPHY_REG_B_1_4_B_KOFF_RANG,B_KOFF_FG_Range(LBR_enbuffer_on_B_range),HDMI_RX_PHY);
    }
    else if (mode == MHL_3G_enbuffer_on)
    {
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(B_KOFF_FG_Manual_mode_mask),B_KOFF_FG_Manual(MHL_3G_enbuffer_on_B),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~HDMIPHY_REG_B_1_4_B_KOFF_RANG,B_KOFF_FG_Range(MHL_3G_enbuffer_on_B_range),HDMI_RX_PHY);
    }
    else if (mode == HBR_enbuffer_on)
    {
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~(R_KOFF_FG_Manual_mode_mask),R_KOFF_FG_Manual(HBR_enbuffer_on_R),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~(G_KOFF_FG_Manual_mode_mask),G_KOFF_FG_Manual(HBR_enbuffer_on_G),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(B_KOFF_FG_Manual_mode_mask),B_KOFF_FG_Manual(HBR_enbuffer_on_B),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~HDMIPHY_REG_R_1_4_R_KOFF_RANG,R_KOFF_FG_Range(HBR_enbuffer_on_R_range),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~HDMIPHY_REG_G_1_4_G_KOFF_RANG,G_KOFF_FG_Range(HBR_enbuffer_on_G_range),HDMI_RX_PHY);
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~HDMIPHY_REG_B_1_4_B_KOFF_RANG,B_KOFF_FG_Range(HBR_enbuffer_on_B_range),HDMI_RX_PHY);

    }
}

void EQ_HBR_Setting_EQ_bias_1X(void)
{

	hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~R_EQ_bias_current_mask,R_EQ_bias_current(6),HDMI_RX_PHY);	//R EQ bias 6 for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~G_EQ_bias_current_mask,G_EQ_bias_current(6),HDMI_RX_PHY);//G EQ bias 6 for over 1.1 GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~B_EQ_bias_current_mask,B_EQ_bias_current(6),HDMI_RX_PHY);//B EQ bias 6 for over 1.1 GHz

	hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~R_Rhase_Routator_Resistor,R_Rhase_Routator_Resistor,HDMI_RX_PHY);	//R PRR for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~G_Rhase_Routator_Resistor,G_Rhase_Routator_Resistor,HDMI_RX_PHY);	//G PRR for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~B_Rhase_Routator_Resistor,B_Rhase_Routator_Resistor,HDMI_RX_PHY);	//B PRR for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~R_EQ_HBR,R_EQ_HBR,HDMI_RX_PHY); //R High bit rate  for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~G_EQ_HBR,G_EQ_HBR,HDMI_RX_PHY); //G High bit rate  for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~B_EQ_HBR,B_EQ_HBR,HDMI_RX_PHY); //B High bit rate  for over 1.1GHz
}

void EQ_HBR_Setting_EQ_bias_1p33X(void)
{
	hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~R_EQ_bias_current_mask,R_EQ_bias_current(7),HDMI_RX_PHY);
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~G_EQ_bias_current_mask,G_EQ_bias_current(7),HDMI_RX_PHY);
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~B_EQ_bias_current_mask,B_EQ_bias_current(7),HDMI_RX_PHY);

	hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~R_Rhase_Routator_Resistor,R_Rhase_Routator_Resistor,HDMI_RX_PHY); //R PRR for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~G_Rhase_Routator_Resistor,G_Rhase_Routator_Resistor,HDMI_RX_PHY); //G PRR for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~B_Rhase_Routator_Resistor,B_Rhase_Routator_Resistor,HDMI_RX_PHY); //B PRR for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~R_EQ_HBR,R_EQ_HBR,HDMI_RX_PHY); //R High bit rate  for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~G_EQ_HBR,G_EQ_HBR,HDMI_RX_PHY); //G High bit rate  for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~B_EQ_HBR,B_EQ_HBR,HDMI_RX_PHY); //B High bit rate  for over 1.1GHz
}

void EQ_LBR_Setting(void)
{
    hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~R_EQ_bias_current_mask,R_EQ_bias_current(5),HDMI_RX_PHY); //R EQ bias 0 for below 1.1GHz
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~G_EQ_bias_current_mask,G_EQ_bias_current(5),HDMI_RX_PHY); //G EQ bias 0 for below 1.1 GHz
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~B_EQ_bias_current_mask,B_EQ_bias_current(5),HDMI_RX_PHY); //B EQ bias 0 for below 1.1 GHz


    hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~R_Rhase_Routator_Resistor,0,HDMI_RX_PHY); //R PRR for below 1.1GHz
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~G_Rhase_Routator_Resistor,0,HDMI_RX_PHY); //G PRR for below 1.1GHz
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~B_Rhase_Routator_Resistor,0,HDMI_RX_PHY); //B PRR for below 1.1GHz
    hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~R_EQ_HBR,0,HDMI_RX_PHY); //R low bit rate  for below 1.1GHz
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~G_EQ_HBR,0,HDMI_RX_PHY); //G low bit rate  for below 1.1GHz
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~B_EQ_HBR,0,HDMI_RX_PHY); //B low bit rate  for below 1.1GHz
}

//***********************************************************************
//Function name :  void EQ_bias_band_setting(UINT32 b)
//PARAM : tmds clock
//RETURN : none
//descript : EQ bias gain setting
//***********************************************************************

void EQ_bias_band_setting(UINT32 b)
{
	UINT16  bClock_Boundry_point ,bclock_75M_point;

  	bClock_Boundry_point = 2200;   //hdmi
	bclock_75M_point = 720;

	if (b>bClock_Boundry_point)	//>2.25G    HBR
	{
		// HDMI
		EQ3_Enable_buffer_Setting(0);
		FORGROUND_OFFSET_MODE(HBR_enbuffer_off);
		EQ_HBR_Setting_EQ_bias_1X();
		HDMI_PRINTF("[HDMI] EQ_HBR_Setting_EQ_bias_1p33X\n");
	}
	else   //LBR
	{
		if (b>bclock_75M_point)//>75M
		{
			EQ3_Enable_buffer_Setting(0);
			FORGROUND_OFFSET_MODE(LBR_enbuffer_off);
		}
		else
		{
			EQ3_Enable_buffer_Setting(1);
			FORGROUND_OFFSET_MODE(LBR_enbuffer_on);
		}
		EQ_LBR_Setting();
	}

	if ((b>246) && (b<266)) // fixed: Nike TX HDMI 27Mhz swing issue (low temperature); solution for Ver.B or later 
	{
		EQ_Enable_buffer_bit22(1);
	}
	else
	{
		EQ_Enable_buffer_bit22(0);
	}

}

UINT8 Hdmi_DFE_EQ_Set(UINT32 b)
{
    UINT8 dfe_mode;
	unsigned int reg_val;

//  UINT32  bClock_Boundry_2G,bClock_Boundry_1p3G;

    hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~HDMIPHY_REG_PHY_CTL_dfe_rstn_n_mask,HDMIPHY_REG_PHY_CTL_dfe_rstn_n_mask, HDMI_RX_PHY);	//DFE register release

	if (b>430 && b < 1060 )  // 45M~1.1G set mode 2
	{
		dfe_mode = 2;
	}
	else
	{
		dfe_mode = 3;
	}
		//HDMI
//		bClock_Boundry_2G = 1895;
//		bClock_Boundry_1p3G =  1280 ;

	//HDMI_PRINTF("DFE Mode = %d\n",dfe_mode);

//	if ( GET_IC_VERSION() != VERSION_A)
//	{
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4, ~R_EQ_back_koff_en, R_EQ_back_koff_en, HDMI_RX_PHY);		// EQ1_output short disable // see Garren 0411 mail
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4, ~G_EQ_back_koff_en, G_EQ_back_koff_en, HDMI_RX_PHY);		// EQ1_output short disable // see Garren 0411 mail
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4, ~B_EQ_back_koff_en, B_EQ_back_koff_en, HDMI_RX_PHY);		// EQ1_output short disable // see Garren 0411 mail
//	}

	hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~R_KOFF_BG_Range_mask,R_KOFF_BG_Range(3), HDMI_RX_PHY);	//R foreground K Off range
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~G_KOFF_BG_Range_mask,G_KOFF_BG_Range(3), HDMI_RX_PHY);	//G foreground K Off range
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~B_KOFF_BG_Range_mask,B_KOFF_BG_Range(3), HDMI_RX_PHY);	//B foreground K Off range

	reg_val = hdmi_rx_reg_read32(HDMIRX_WRAPPER_CONTROL_0_reg, HDMI_RX_HDMI_WRAPPER);
	reg_val |= (HDMIRX_WRAPPER_CONTROL_0_ADDR_H(0x7) | HDMIRX_WRAPPER_CONTROL_0_HDMIRX_EN(0x1));
	hdmi_rx_reg_write32(HDMIRX_WRAPPER_CONTROL_0_reg, reg_val, HDMI_RX_HDMI_WRAPPER);
#if 1
	//=========DEF initial===========
	//hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8_reg,~r_DFE_RVTH_mask,r_DFE_RVTH(3));				//VTH setting
	//hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8_reg,~g_DFE_RVTH_mask,g_DFE_RVTH(3));				//VTH setting
	//hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8_reg,~b_DFE_RVTH_mask,b_DFE_RVTH(3));				//VTH setting

	//hdmi_rx_reg_mask32(REG_DFE_INIT1_L0_reg,~REG_DFE_INIT1_L0_de_packet_en_lane0_mask,REG_DFE_INIT1_L0_de_packet_en_lane0(1));	//data packet area disable DFE calibration
	//hdmi_rx_reg_mask32(REG_DFE_INIT1_L1_reg,~REG_DFE_INIT1_L1_de_packet_en_lane2_mask,REG_DFE_INIT1_L1_de_packet_en_lane2(1));
	//hdmi_rx_reg_mask32(REG_DFE_INIT1_L2_reg,~REG_DFE_INIT1_L2_de_packet_en_lane2_mask,REG_DFE_INIT1_L2_de_packet_en_lane2(1));
/*
	hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8_reg,~HDMIPHY_REG_R_5_8_enbuffer3_mask, HDMIPHY_REG_R_5_8_enbuffer3(1)); // 20140304
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8_reg,~HDMIPHY_REG_G_5_8_enbuffer3_mask, HDMIPHY_REG_G_5_8_enbuffer3(1)); // 20140304
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8_reg,~HDMIPHY_REG_B_5_8_enbuffer3_mask, HDMIPHY_REG_B_5_8_enbuffer3(1)); // 20140304
	*/
	/*
	hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12_reg,~HDMIPHY_REG_R_9_12_tap0_bias_cur_mask, HDMIPHY_REG_R_9_12_tap0_bias_cur(2)); // 20140304
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_9_12_reg,~HDMIPHY_REG_G_9_12_tap0_bias_cur_mask, HDMIPHY_REG_G_9_12_tap0_bias_cur(2)); // 20140304
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_9_12_reg,~HDMIPHY_REG_B_9_12_tap0_bias_cur_mask, HDMIPHY_REG_B_9_12_tap0_bias_cur(2)); // 20140304
	*/
	hdmi_rx_reg_mask32(REG_DFE_MODE,~REG_DFE_MODE_adapt_mode_mask,REG_DFE_MODE_adapt_mode(dfe_mode),HDMI_RX_RBUS_DFE);	//adaptive mode sel
	hdmi_rx_reg_mask32(REG_DFE_MODE,~REG_DFE_MODE_gray_en_mask,REG_DFE_MODE_gray_en(0x1e),HDMI_RX_RBUS_DFE);	//gray code 0xA1--> 0x1e
	hdmi_rx_reg_mask32(REG_DFE_MODE,~REG_DFE_MODE_transition_only_mask,REG_DFE_MODE_transition_only(1),HDMI_RX_RBUS_DFE);	//transition mode enable
	hdmi_rx_reg_mask32(REG_DFE_MODE,~REG_DFE_MODE_le_auto_reload_mask,0,HDMI_RX_RBUS_DFE);	//auto reload enable
	//
	//cloud test for DFE 20140227
	//hdmi_rx_reg_mask32(REG_DFE_MODE_reg,~REG_DFE_MODE_cs_mode_mask,REG_DFE_MODE_cs_mode(1));	//Current step mode select
	hdmi_rx_reg_mask32(REG_DFE_MODE,~REG_DFE_MODE_cs_mode_mask,REG_DFE_MODE_cs_mode(0),HDMI_RX_RBUS_DFE);
	//cloud test for dfe  20140227
	//hdmi_rx_reg_mask32(REG_DFE_INIT1_L0_reg,~REG_DFE_INIT1_L0_dfe_set_cs_manual_lane0_mask,REG_DFE_INIT1_L0_dfe_set_cs_manual_lane0(0));	//Current step mode select	//Auto
	//hdmi_rx_reg_mask32(REG_DFE_INIT1_L1_reg,~REG_DFE_INIT1_L1_dfe_set_cs_manual_lane1_mask,REG_DFE_INIT1_L1_dfe_set_cs_manual_lane1(0));	//Current step mode select
	//hdmi_rx_reg_mask32(REG_DFE_INIT1_L2_reg,~REG_DFE_INIT1_L2_dfe_set_cs_manual_lane2_mask,REG_DFE_INIT1_L2_dfe_set_cs_manual_lane2(0));	//Current step mode select

	hdmi_rx_reg_mask32(REG_DFE_INIT1_L0,~REG_DFE_INIT1_L0_dfe_set_cs_manual_lane0_mask,REG_DFE_INIT1_L0_dfe_set_cs_manual_lane0(1),HDMI_RX_RBUS_DFE);
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L1,~REG_DFE_INIT1_L1_dfe_set_cs_manual_lane1_mask,REG_DFE_INIT1_L1_dfe_set_cs_manual_lane1(1),HDMI_RX_RBUS_DFE);	//Current step mode select
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L2,~REG_DFE_INIT1_L2_dfe_set_cs_manual_lane2_mask,REG_DFE_INIT1_L2_dfe_set_cs_manual_lane2(1),HDMI_RX_RBUS_DFE);

	hdmi_rx_reg_mask32(REG_DFE_INIT1_L0,~REG_DFE_INIT1_L0_dfe_en_buffer_manual_lane0_mask,REG_DFE_INIT1_L0_dfe_en_buffer_manual_lane0(1),HDMI_RX_RBUS_DFE);
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L1,~REG_DFE_INIT1_L1_dfe_en_buffer_manual_lane1_mask,REG_DFE_INIT1_L1_dfe_en_buffer_manual_lane1(1),HDMI_RX_RBUS_DFE);	//buffer mode select
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L2,~REG_DFE_INIT1_L2_dfe_en_buffer_manual_lane2_mask,REG_DFE_INIT1_L2_dfe_en_buffer_manual_lane2(1),HDMI_RX_RBUS_DFE);

	hdmi_rx_reg_mask32(REG_DFE_INIT1_L0,~REG_DFE_INIT1_L0_dfe_set_cs_lane0_mask,REG_DFE_INIT1_L0_dfe_set_cs_lane0(1),HDMI_RX_RBUS_DFE);	//Current step set
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L1,~REG_DFE_INIT1_L1_dfe_set_cs_lane1_mask,REG_DFE_INIT1_L1_dfe_set_cs_lane1(1),HDMI_RX_RBUS_DFE);	//Current step set
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L2,~REG_DFE_INIT1_L2_dfe_set_cs_lane2_mask,REG_DFE_INIT1_L2_dfe_set_cs_lane2(1),HDMI_RX_RBUS_DFE);	//Current step set

	//hdmi_rx_reg_mask32(REG_DFE_INIT1_L0_reg,~REG_DFE_INIT1_L0_dfe_en_buffer_manual_lane0_mask,REG_DFE_INIT1_L0_dfe_en_buffer_manual_lane0(1));	//enable buffer		//Auto
	//hdmi_rx_reg_mask32(REG_DFE_INIT1_L1_reg,~REG_DFE_INIT1_L1_dfe_en_buffer_manual_lane1_mask,REG_DFE_INIT1_L1_dfe_en_buffer_manual_lane1(1));	//enable buffer
	//hdmi_rx_reg_mask32(REG_DFE_INIT1_L2_reg,~REG_DFE_INIT1_L2_dfe_en_buffer_manual_lane2_mask,REG_DFE_INIT1_L2_dfe_en_buffer_manual_lane2(1));	//enable buffer

	//hdmi_rx_reg_mask32(REG_DFE_INIT1_L0_reg,~REG_DFE_INIT1_L0_dfe_en_buffer_lane0_mask,REG_DFE_INIT1_L0_dfe_en_buffer_lane0(0));	//enable buffer step
	//hdmi_rx_reg_mask32(REG_DFE_INIT1_L1_reg,~REG_DFE_INIT1_L1_dfe_en_buffer_lane1_mask,REG_DFE_INIT1_L1_dfe_en_buffer_lane1(0));	//enable buffer step
	//hdmi_rx_reg_mask32(REG_DFE_INIT1_L2_reg,~REG_DFE_INIT1_L2_dfe_en_buffer_lane2_mask,REG_DFE_INIT1_L2_dfe_en_buffer_lane2(0));	//enable buffer step

	//limit
	hdmi_rx_reg_mask32(REG_DFE_LIMIT0,~REG_DFE_LIMIT0_tap0_max_mask,REG_DFE_LIMIT0_tap0_max(0x12),HDMI_RX_RBUS_DFE);	//TAP0_MAX  0xf 0x12
	hdmi_rx_reg_mask32(REG_DFE_LIMIT0,~REG_DFE_LIMIT0_tap1_min_mask,REG_DFE_LIMIT0_tap1_min(0x2e),HDMI_RX_RBUS_DFE);	//  cloud modify 20140325 TAP1_MIN 0x37
	hdmi_rx_reg_mask32(REG_DFE_LIMIT0,~REG_DFE_LIMIT0_tap2_max_mask,REG_DFE_LIMIT0_tap2_max(0xA),HDMI_RX_RBUS_DFE);	//TAP2_MAX
	hdmi_rx_reg_mask32(REG_DFE_LIMIT0,~REG_DFE_LIMIT0_tap2_min_mask,REG_DFE_LIMIT0_tap2_min(0x19),HDMI_RX_RBUS_DFE);	//TAP2_MIN
	hdmi_rx_reg_mask32(REG_DFE_LIMIT0,~REG_DFE_LIMIT0_tap3_max_mask,REG_DFE_LIMIT0_tap3_max(6),HDMI_RX_RBUS_DFE);	//TAP3_MAX
	hdmi_rx_reg_mask32(REG_DFE_LIMIT0,~REG_DFE_LIMIT0_tap3_min_mask,REG_DFE_LIMIT0_tap3_min(0x19),HDMI_RX_RBUS_DFE);	//TAP3_MIN
	hdmi_rx_reg_mask32(REG_DFE_LIMIT1,~REG_DFE_LIMIT1_tap4_max_mask,REG_DFE_LIMIT1_tap4_max(6),HDMI_RX_RBUS_DFE);	//TAP4_MAX
	hdmi_rx_reg_mask32(REG_DFE_LIMIT1,~REG_DFE_LIMIT1_tap4_min_mask,REG_DFE_LIMIT1_tap4_min(0x19),HDMI_RX_RBUS_DFE);	//TAP4_MIN

	//threshold
	hdmi_rx_reg_mask32(REG_DFE_LIMIT1,~REG_DFE_LIMIT1_tap0_threshold_mask,REG_DFE_LIMIT1_tap0_threshold(0x0),HDMI_RX_RBUS_DFE);	//TAP0 threshold 0xc
	hdmi_rx_reg_mask32(REG_DFE_LIMIT1,~REG_DFE_LIMIT1_vth_threshold_mask, REG_DFE_LIMIT1_vth_threshold(0x0),HDMI_RX_RBUS_DFE); // 20140304  0x05

	//divisor
	hdmi_rx_reg_mask32(REG_DFE_LIMIT2,~REG_DFE_LIMIT2_servo_divisor_mask,REG_DFE_LIMIT2_servo_divisor(0x28),HDMI_RX_RBUS_DFE);	//servo divisor
	hdmi_rx_reg_mask32(REG_DFE_LIMIT2,~REG_DFE_LIMIT2_tap_divisor_mask,REG_DFE_LIMIT2_tap_divisor(0xa),HDMI_RX_RBUS_DFE);	//tap divisor
	hdmi_rx_reg_mask32(REG_DFE_LIMIT2,~REG_DFE_LIMIT2_vth_divisor_mask,REG_DFE_LIMIT2_vth_divisor(0xf),HDMI_RX_RBUS_DFE);	//Vth divisor
	//delay
	hdmi_rx_reg_mask32(REG_DFE_MODE,~REG_DFE_MODE_tap1_delay_mask,REG_DFE_MODE_tap1_delay(7),HDMI_RX_RBUS_DFE);	//tap1 delay
	hdmi_rx_reg_mask32(REG_DFE_MODE,~REG_DFE_MODE_tap24_delay_mask,REG_DFE_MODE_tap24_delay(7),HDMI_RX_RBUS_DFE);	//tap24 delay
	hdmi_rx_reg_mask32(REG_DFE_MODE,~REG_DFE_MODE_servo_delay_mask,1,HDMI_RX_RBUS_DFE);	//servo delay  cloud modify 2014 0326 set 1

	//lane timer enable
	hdmi_rx_reg_mask32(REG_DFE_EN_L0,~REG_DFE_EN_L0_timer_ctrl_en_lane0_mask,REG_DFE_EN_L0_timer_ctrl_en_lane0(1),HDMI_RX_RBUS_DFE);	//lane0 timer enable
	hdmi_rx_reg_mask32(REG_DFE_EN_L1,~REG_DFE_EN_L1_timer_ctrl_en_lane1_mask,REG_DFE_EN_L1_timer_ctrl_en_lane1(1),HDMI_RX_RBUS_DFE);	//lane1 timer enable
	hdmi_rx_reg_mask32(REG_DFE_EN_L2,~REG_DFE_EN_L2_timer_ctrl_en_lane2_mask,REG_DFE_EN_L2_timer_ctrl_en_lane2(1),HDMI_RX_RBUS_DFE);	//lane2 timer enable
	//run-length detect threshold
	hdmi_rx_reg_mask32(REG_DFE_EN_L0,~REG_DFE_EN_L0_rl_threshold_lane0_mask,REG_DFE_EN_L0_rl_threshold_lane0(1),HDMI_RX_RBUS_DFE);	//lane0 run-length detect threshold
	hdmi_rx_reg_mask32(REG_DFE_EN_L1,~REG_DFE_EN_L1_rl_threshold_lane1_mask,REG_DFE_EN_L1_rl_threshold_lane1(1),HDMI_RX_RBUS_DFE);	//lane1 run-length detect threshold
	hdmi_rx_reg_mask32(REG_DFE_EN_L2,~REG_DFE_EN_L2_rl_threshold_lane2_mask,REG_DFE_EN_L2_rl_threshold_lane2(1),HDMI_RX_RBUS_DFE);	//lane2 run-length detect threshold
	//run-length detect enable
	hdmi_rx_reg_mask32(REG_DFE_EN_L0,~REG_DFE_EN_L0_rl_det_mode_lane0_mask,REG_DFE_EN_L0_rl_det_mode_lane0(1),HDMI_RX_RBUS_DFE);	//lane0 run-length mode
	hdmi_rx_reg_mask32(REG_DFE_EN_L1,~REG_DFE_EN_L1_rl_det_mode_lane1_mask,REG_DFE_EN_L1_rl_det_mode_lane1(1),HDMI_RX_RBUS_DFE);	//lane1 run-length mode
	hdmi_rx_reg_mask32(REG_DFE_EN_L2,~REG_DFE_EN_L2_rl_det_mode_lane2_mask,REG_DFE_EN_L2_rl_det_mode_lane2(1),HDMI_RX_RBUS_DFE);	//lane2 run-length mode
      //20140319 test
	hdmi_rx_reg_mask32(REG_DFE_MODE,~(REG_DFE_MODE_servo_notrans_mask|REG_DFE_MODE_tap0_notrans_mask),REG_DFE_MODE_servo_notrans_mask|REG_DFE_MODE_tap0_notrans_mask,HDMI_RX_RBUS_DFE);	//notrans

      //
	hdmi_rx_reg_mask32(REG_DFE_EN_L0,~REG_DFE_EN_L0_rl_det_en_lane0_mask,0,HDMI_RX_RBUS_DFE);	//lane0 run-length detect disable
	hdmi_rx_reg_mask32(REG_DFE_EN_L1,~REG_DFE_EN_L1_rl_det_en_lane1_mask,0,HDMI_RX_RBUS_DFE);	//lane1 run-length detect disable
	hdmi_rx_reg_mask32(REG_DFE_EN_L2,~REG_DFE_EN_L2_rl_det_en_lane2_mask,0,HDMI_RX_RBUS_DFE);	//lane2 run-length detect disable

	//Gain
	hdmi_rx_reg_mask32(REG_DFE_GAIN,~REG_DFE_GAIN_tap0_gain_mask,REG_DFE_GAIN_tap0_gain(2),HDMI_RX_RBUS_DFE);	//tap0 gain
	hdmi_rx_reg_mask32(REG_DFE_GAIN,~REG_DFE_GAIN_tap1_gain_mask,REG_DFE_GAIN_tap1_gain(2),HDMI_RX_RBUS_DFE);	//tap1 gain
	hdmi_rx_reg_mask32(REG_DFE_GAIN,~REG_DFE_GAIN_tap2_gain_mask,REG_DFE_GAIN_tap2_gain(1),HDMI_RX_RBUS_DFE);	//tap2 gain
	hdmi_rx_reg_mask32(REG_DFE_GAIN,~REG_DFE_GAIN_tap3_gain_mask,REG_DFE_GAIN_tap3_gain(1),HDMI_RX_RBUS_DFE);	//tap3 gain
	hdmi_rx_reg_mask32(REG_DFE_GAIN,~REG_DFE_GAIN_tap4_gain_mask,REG_DFE_GAIN_tap4_gain(1),HDMI_RX_RBUS_DFE);	//tap4 gain
	hdmi_rx_reg_mask32(REG_DFE_GAIN,~REG_DFE_GAIN_servo_gain_mask,REG_DFE_GAIN_servo_gain(1),HDMI_RX_RBUS_DFE);	//servo gain

	//LE
    //20140306  test
    /*
	hdmi_rx_reg_mask32(REG_DFE_EN_L0_reg,~REG_DFE_EN_L0_le_min_lane0_mask,REG_DFE_EN_L0_le_min_lane0(0xc));	//lane0 LE coefficient min		//0x18
	hdmi_rx_reg_mask32(REG_DFE_EN_L1_reg,~REG_DFE_EN_L1_le_min_lane1_mask,REG_DFE_EN_L1_le_min_lane1(0xc));	//lane1 LE coefficient min		//0x18
	hdmi_rx_reg_mask32(REG_DFE_EN_L2_reg,~REG_DFE_EN_L2_le_min_lane2_mask,REG_DFE_EN_L2_le_min_lane2(0xc));	//lane2 LE coefficient min		//0x18
    */

    hdmi_rx_reg_mask32(REG_DFE_EN_L0,~REG_DFE_EN_L0_le_min_lane0_mask,REG_DFE_EN_L0_le_min_lane0(0xf),HDMI_RX_RBUS_DFE);	//lane0 LE coefficient min		//0x18
    hdmi_rx_reg_mask32(REG_DFE_EN_L1,~REG_DFE_EN_L1_le_min_lane1_mask,REG_DFE_EN_L1_le_min_lane1(0xf),HDMI_RX_RBUS_DFE);	//lane1 LE coefficient min		//0x18
    hdmi_rx_reg_mask32(REG_DFE_EN_L2,~REG_DFE_EN_L2_le_min_lane2_mask,REG_DFE_EN_L2_le_min_lane2(0xf),HDMI_RX_RBUS_DFE);	//lane2 LE coefficient min		//0x18

#if 0
	if (b<450)
	{
		if (dfe_mode == 2)
		{
			hdmi_rx_reg_mask32(REG_DFE_EN_L0_reg,~REG_DFE_EN_L0_le_init_lane0_mask,REG_DFE_EN_L0_le_init_lane0(0x3)),	//lane0  LE coefficient initial
			hdmi_rx_reg_mask32(REG_DFE_EN_L1_reg,~REG_DFE_EN_L1_le_init_lane1_mask,REG_DFE_EN_L1_le_init_lane1(0x3)),	//lane1  LE coefficient initial
			hdmi_rx_reg_mask32(REG_DFE_EN_L2_reg,~REG_DFE_EN_L2_le_init_lane2_mask,REG_DFE_EN_L2_le_init_lane2(0x3));	//lane2  LE coefficient initial
		}
	}

#endif


	//servo loop&Tap initial value
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L0,~REG_DFE_INIT0_L0_servo_init_lane0_mask,REG_DFE_INIT0_L0_servo_init_lane0(0xf),HDMI_RX_RBUS_DFE);	//lane0 servo initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L1,~REG_DFE_INIT0_L1_servo_init_lane1_mask,REG_DFE_INIT0_L1_servo_init_lane1(0xf),HDMI_RX_RBUS_DFE);	//lane1 servo initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L2,~REG_DFE_INIT0_L2_servo_init_lane2_mask,REG_DFE_INIT0_L2_servo_init_lane2(0xf),HDMI_RX_RBUS_DFE);	//lane2 servo initial

	hdmi_rx_reg_mask32(REG_DFE_INIT0_L0,~REG_DFE_INIT0_L0_tap0_init_lane0_mask,REG_DFE_INIT0_L0_tap0_init_lane0(0xf),HDMI_RX_RBUS_DFE);	//lane0 tap0 initial	//0xc
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L1,~REG_DFE_INIT0_L1_tap0_init_lane1_mask,REG_DFE_INIT0_L1_tap0_init_lane1(0xf),HDMI_RX_RBUS_DFE);	//lane1 tap0 initial	//0xc
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L2,~REG_DFE_INIT0_L2_tap0_init_lane2_mask,REG_DFE_INIT0_L2_tap0_init_lane2(0xf),HDMI_RX_RBUS_DFE);	//lane2 tap0 initial	//0xc

	if (dfe_mode == 2)
	{
	// mode 2   tap1 = tap1   LE = LE set in tap1 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L0,~REG_DFE_INIT0_L0_tap1_init_lane0_mask,REG_DFE_INIT0_L0_tap1_init_lane0(0x0),HDMI_RX_RBUS_DFE);	//lane0 tap1 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L1,~REG_DFE_INIT0_L1_tap1_init_lane1_mask,REG_DFE_INIT0_L1_tap1_init_lane1(0x0),HDMI_RX_RBUS_DFE);	//lane1 tap1 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L2,~REG_DFE_INIT0_L2_tap1_init_lane2_mask,REG_DFE_INIT0_L2_tap1_init_lane2(0x0),HDMI_RX_RBUS_DFE);	//lane2 tap1 initial

	hdmi_rx_reg_mask32(REG_DFE_EN_L0,~REG_DFE_EN_L0_le_init_lane0_mask,REG_DFE_EN_L0_le_init_lane0(0x7),HDMI_RX_RBUS_DFE);	//lane0  LE coefficient initial
	hdmi_rx_reg_mask32(REG_DFE_EN_L1,~REG_DFE_EN_L1_le_init_lane1_mask,REG_DFE_EN_L1_le_init_lane1(0x7),HDMI_RX_RBUS_DFE);	//lane1  LE coefficient initial
	hdmi_rx_reg_mask32(REG_DFE_EN_L2,~REG_DFE_EN_L2_le_init_lane2_mask,REG_DFE_EN_L2_le_init_lane2(0x7),HDMI_RX_RBUS_DFE);	//lane2  LE coefficient initial
	}
	else    // mode 3
	{
	// mode 3   tap1 = tap1 +LE set in tap1 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L0,~REG_DFE_INIT0_L0_tap1_init_lane0_mask,REG_DFE_INIT0_L0_tap1_init_lane0(0xc),HDMI_RX_RBUS_DFE);	//lane0 tap1 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L1,~REG_DFE_INIT0_L1_tap1_init_lane1_mask,REG_DFE_INIT0_L1_tap1_init_lane1(0xc),HDMI_RX_RBUS_DFE);	//lane1 tap1 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L2,~REG_DFE_INIT0_L2_tap1_init_lane2_mask,REG_DFE_INIT0_L2_tap1_init_lane2(0xc),HDMI_RX_RBUS_DFE);	//lane2 tap1 initial
	}
#if 0
	else
	{

	if (b<450)
	{
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L0_reg,~REG_DFE_INIT0_L0_tap1_init_lane0_mask,REG_DFE_INIT0_L0_tap1_init_lane0(0x1)),	//lane0 tap1 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L1_reg,~REG_DFE_INIT0_L1_tap1_init_lane1_mask,REG_DFE_INIT0_L1_tap1_init_lane1(0x1)),	//lane1 tap1 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L2_reg,~REG_DFE_INIT0_L2_tap1_init_lane2_mask,REG_DFE_INIT0_L2_tap1_init_lane2(0x1));	//lane2 tap1 initial
	}
	else

	{
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L0_reg,~REG_DFE_INIT0_L0_tap1_init_lane0_mask,REG_DFE_INIT0_L0_tap1_init_lane0(0xc)),	//lane0 tap1 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L1_reg,~REG_DFE_INIT0_L1_tap1_init_lane1_mask,REG_DFE_INIT0_L1_tap1_init_lane1(0xc)),	//lane1 tap1 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L2_reg,~REG_DFE_INIT0_L2_tap1_init_lane2_mask,REG_DFE_INIT0_L2_tap1_init_lane2(0xc));	//lane2 tap1 initial
	}
	}
#endif
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L0,~REG_DFE_INIT0_L0_tap2_init_lane0_mask,0,HDMI_RX_RBUS_DFE);		//lane0 tap2 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L1,~REG_DFE_INIT0_L1_tap2_init_lane1_mask,0,HDMI_RX_RBUS_DFE);		//lane1 tap2 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L2,~REG_DFE_INIT0_L2_tap2_init_lane2_mask,0,HDMI_RX_RBUS_DFE);		//lane2 tap2 initial

	hdmi_rx_reg_mask32(REG_DFE_INIT0_L0,~REG_DFE_INIT0_L0_tap3_init_lane0_mask,0,HDMI_RX_RBUS_DFE);		//lane0 tap3 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L1,~REG_DFE_INIT0_L1_tap3_init_lane1_mask,0,HDMI_RX_RBUS_DFE);		//lane1 tap3 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L2,~REG_DFE_INIT0_L2_tap3_init_lane2_mask,0,HDMI_RX_RBUS_DFE);		//lane2 tap3 initial

	hdmi_rx_reg_mask32(REG_DFE_INIT0_L0,~REG_DFE_INIT0_L0_vth_init_lane0_mask,REG_DFE_INIT0_L0_vth_init_lane0(5),HDMI_RX_RBUS_DFE);		//lane0 Vth initial	//8
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L1,~REG_DFE_INIT0_L1_vth_init_lane1_mask,REG_DFE_INIT0_L1_vth_init_lane1(5),HDMI_RX_RBUS_DFE);		//lane1 Vth initial	//8
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L2,~REG_DFE_INIT0_L2_vth_init_lane2_mask,REG_DFE_INIT0_L2_vth_init_lane2(5),HDMI_RX_RBUS_DFE);		//lane2 Vth initial	//8
	//Load initial value
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L0,~REG_DFE_INIT1_L0_load_in_init_lane0_mask,0,HDMI_RX_RBUS_DFE);		//lane0  initial load
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L1,~REG_DFE_INIT1_L1_load_in_init_lane1_mask,0,HDMI_RX_RBUS_DFE);		//lane1  initial load
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L2,~REG_DFE_INIT1_L2_load_in_init_lane2_mask,0,HDMI_RX_RBUS_DFE);		//lane2  initial load
	HDMI_DELAYMS(1);
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L0,~REG_DFE_INIT1_L0_load_in_init_lane0_mask,REG_DFE_INIT1_L0_load_in_init_lane0(0xff),HDMI_RX_RBUS_DFE);		//lane0  initial load
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L1,~REG_DFE_INIT1_L1_load_in_init_lane1_mask,REG_DFE_INIT1_L1_load_in_init_lane1(0xff),HDMI_RX_RBUS_DFE);		//lane1  initial load
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L2,~REG_DFE_INIT1_L2_load_in_init_lane2_mask,REG_DFE_INIT1_L2_load_in_init_lane2(0xff),HDMI_RX_RBUS_DFE);		//lane2  initial load
	HDMI_DELAYMS(1);
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L0,~REG_DFE_INIT1_L0_load_in_init_lane0_mask,0,HDMI_RX_RBUS_DFE);		//lane0  initial load
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L1,~REG_DFE_INIT1_L1_load_in_init_lane1_mask,0,HDMI_RX_RBUS_DFE);		//lane1  initial load
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L2,~REG_DFE_INIT1_L2_load_in_init_lane2_mask,0,HDMI_RX_RBUS_DFE);		//lane2  initial load

	//LE Gain set
	//hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4_reg,~R_BIAS_ENHANCE_mask,0);		//R bias enhance
	//hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4_reg,~G_BIAS_ENHANCE_mask,0);		//G bias enhance
	//hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4_reg,~B_BIAS_ENHANCE_mask,0);		//B bias enhance


    hdmi_rx_reg_mask32(REG_DFE_LIMIT1,~REG_DFE_LIMIT1_vth_min_mask,REG_DFE_LIMIT1_vth_min(0),HDMI_RX_RBUS_DFE);	//Vth min 2014 0326 set 0 judge Vth


	//hdmi_rx_reg_mask32(REG_DFE_LIMIT1_reg,~REG_DFE_LIMIT1_vth_min_mask,REG_DFE_LIMIT1_vth_min((b>bClock_Boundry_2G)?1:1));	//Vth min 6 for over 2GHz
	hdmi_rx_reg_mask32(REG_DFE_LIMIT1,~REG_DFE_LIMIT1_vth_dis_buf_range_mask,0,HDMI_RX_RBUS_DFE);	//Vth_dis_buf_range 0
#endif
	//PHY Tap en
//    if (b>bClock_Boundry_1p3G)	//>1.35G
	{
		hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~R_DFE_TAPEN4321,R_DFE_TAPEN4321,HDMI_RX_PHY);		//R tap1~4 enable
		hdmi_rx_reg_mask32(HDMIPHY_REG_G_9_12,~G_DFE_TAPEN4321,G_DFE_TAPEN4321,HDMI_RX_PHY);		//G tap1~4 enable
		hdmi_rx_reg_mask32(HDMIPHY_REG_B_9_12,~B_DFE_TAPEN4321,B_DFE_TAPEN4321,HDMI_RX_PHY);		//B tap1~4 enable
	}
	/*
	else		//<1.35G
	{
		hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12_reg,~R_DFE_TAPEN4321,R_DFE_TAPEN21);	//R tap1~2 enable
		hdmi_rx_reg_mask32(HDMIPHY_REG_G_9_12_reg,~G_DFE_TAPEN4321,G_DFE_TAPEN21);	//G tap1~2 enable
		hdmi_rx_reg_mask32(HDMIPHY_REG_B_9_12_reg,~B_DFE_TAPEN4321,B_DFE_TAPEN21);	//B tap1~2 enable
	}
    */
	//DFE reset (write initial value) 20140109
#if 1
	hdmi_rx_reg_mask32(REG_DFE_MODE,~REG_DFE_MODE_dfe_adapt_rstb_mask,0,HDMI_RX_RBUS_DFE);		//lane0~2 reset adaptive
	HDMI_DELAYMS(1);
	hdmi_rx_reg_mask32(REG_DFE_MODE,~REG_DFE_MODE_dfe_adapt_rstb_mask,REG_DFE_MODE_dfe_adapt_rstb(0xf),HDMI_RX_RBUS_DFE);		//lane0~2 reset adaptive
#endif
	//PHY adaptive en
	hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~R_DFE_ADAPT_EN,R_DFE_ADAPT_EN,HDMI_RX_PHY);	//R adaptive enable
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_9_12,~G_DFE_ADAPT_EN,G_DFE_ADAPT_EN,HDMI_RX_PHY);	//G adaptive enable
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_9_12,~B_DFE_ADAPT_EN,B_DFE_ADAPT_EN,HDMI_RX_PHY);	//B adaptive enable

	return dfe_mode;

}

void DFE_Manual_Set(void)
{
	hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~HDMIPHY_REG_PHY_CTL_dfe_rstn_n_mask,HDMIPHY_REG_PHY_CTL_dfe_rstn_n_mask,HDMI_RX_PHY);	//DFE register release
	hdmi_rx_reg_mask32(REG_DFE_MODE,~REG_DFE_MODE_adapt_mode_mask,REG_DFE_MODE_adapt_mode(3),HDMI_RX_RBUS_DFE);	//adaptive mode sel
	hdmi_rx_reg_mask32(REG_DFE_EN_L0,~REG_DFE_EN_L0_le_min_lane0_mask,REG_DFE_EN_L0_le_min_lane0(0xf),HDMI_RX_RBUS_DFE);	//lane0 LE coefficient min		//0x18
	hdmi_rx_reg_mask32(REG_DFE_EN_L1,~REG_DFE_EN_L1_le_min_lane1_mask,REG_DFE_EN_L1_le_min_lane1(0xf),HDMI_RX_RBUS_DFE);	//lane1 LE coefficient min		//0x18
	hdmi_rx_reg_mask32(REG_DFE_EN_L2,~REG_DFE_EN_L2_le_min_lane2_mask,REG_DFE_EN_L2_le_min_lane2(0xf),HDMI_RX_RBUS_DFE);	//lane2 LE coefficient min		//0x18
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L0,~REG_DFE_INIT0_L0_tap1_init_lane0_mask,REG_DFE_INIT0_L0_tap1_init_lane0(8),HDMI_RX_RBUS_DFE);	//lane0 tap1 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L1,~REG_DFE_INIT0_L1_tap1_init_lane1_mask,REG_DFE_INIT0_L1_tap1_init_lane1(8),HDMI_RX_RBUS_DFE);	//lane1 tap1 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT0_L2,~REG_DFE_INIT0_L2_tap1_init_lane2_mask,REG_DFE_INIT0_L2_tap1_init_lane2(8),HDMI_RX_RBUS_DFE);	//lane2 tap1 initial
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L0,~REG_DFE_INIT1_L0_load_in_init_lane0_mask,REG_DFE_INIT1_L0_load_in_init_lane0(2),HDMI_RX_RBUS_DFE);		//lane0  initial load
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L1,~REG_DFE_INIT1_L1_load_in_init_lane1_mask,REG_DFE_INIT1_L1_load_in_init_lane1(2),HDMI_RX_RBUS_DFE);		//lane1  initial load
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L2,~REG_DFE_INIT1_L2_load_in_init_lane2_mask,REG_DFE_INIT1_L2_load_in_init_lane2(2),HDMI_RX_RBUS_DFE);		//lane2  initial load
	HDMI_DELAYMS(1);	//10000
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L0,~REG_DFE_INIT1_L0_load_in_init_lane0_mask,0,HDMI_RX_RBUS_DFE);		//lane0  initial load
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L1,~REG_DFE_INIT1_L1_load_in_init_lane1_mask,0,HDMI_RX_RBUS_DFE);		//lane1  initial load
	hdmi_rx_reg_mask32(REG_DFE_INIT1_L2,~REG_DFE_INIT1_L2_load_in_init_lane2_mask,0,HDMI_RX_RBUS_DFE);		//lane2  initial load
	hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~R_DFE_TAPEN4321,R_DFE_TAPEN1,HDMI_RX_PHY);	//R tap1 enable
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_9_12,~G_DFE_TAPEN4321,G_DFE_TAPEN1,HDMI_RX_PHY);	//G tap1 enable
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_9_12,~B_DFE_TAPEN4321,B_DFE_TAPEN1,HDMI_RX_PHY);	//B tap1 enable
}

void Dump_DFE_Para(UINT8 lane)
{
	//HDMI_I2C_MASK(DFE_REG_DFE_READBACK_VADDR,~(_BIT31),_BIT31);	//enable record function
	//ScalerTimer_DelayXms(50);
	hdmi_rx_reg_mask32(REG_DFE_READBACK,~limit_set_lane_mask,lane<<limit_set_lane_shift,HDMI_RX_RBUS_DFE);
	hdmi_rx_reg_mask32(REG_DFE_READBACK,~limit_set_mask,0,HDMI_RX_RBUS_DFE);
	//HDMI_DELAYMS(1);	//1000
	HDMI_PRINTF("[HDMI] ********************Lane%d******************\n",lane);
	HDMI_PRINTF("[HDMI] Lane%d Tap0 max = 0x%02x  ",lane,get_Tap0_max(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

	hdmi_rx_reg_mask32(REG_DFE_READBACK,~limit_set_mask,limit_sel(1),HDMI_RX_RBUS_DFE);
	//HDMI_DELAYMS(1);	//1000
	HDMI_PRINTF(" min = 0x%02x\n",get_Tap0_min(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

	hdmi_rx_reg_mask32(REG_DFE_READBACK,~limit_set_mask,limit_sel(2),HDMI_RX_RBUS_DFE);
	//udelay(10);	//1000
	HDMI_PRINTF("[HDMI] Lane%d Tap1 max = 0x%02x  ",lane,get_Tap1_max(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

	hdmi_rx_reg_mask32(REG_DFE_READBACK,~limit_set_mask,limit_sel(3),HDMI_RX_RBUS_DFE);
	//udelay(10);	//1000
	HDMI_PRINTF(" min = 0x%02x\n",get_Tap1_min(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

	hdmi_rx_reg_mask32(REG_DFE_READBACK,~limit_set_mask,limit_sel(4),HDMI_RX_RBUS_DFE);
	//udelay(10);	//1000
	HDMI_PRINTF("[HDMI] Lane%d LE   max = 0x%02x  ",lane,get_LEQ_max(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

	hdmi_rx_reg_mask32(REG_DFE_READBACK,~limit_set_mask,limit_sel(5),HDMI_RX_RBUS_DFE);
	//udelay(10);	//1000
	HDMI_PRINTF(" min = 0x%02x\n",get_LEQ_min(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

	//read DFE result
	hdmi_rx_reg_mask32(REG_DFE_READBACK,~coef_set_lane_mask,lane<<coef_set_lane_shift,HDMI_RX_RBUS_DFE);
	hdmi_rx_reg_mask32(REG_DFE_READBACK,~coef_set_mask,0,HDMI_RX_RBUS_DFE);
	//udelay(10);	//5000
	HDMI_PRINTF("[HDMI] Lane%d Vth COEF = 0x%x\n",lane,get_VTH_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));
	HDMI_PRINTF("[HDMI] Lane%d current Step = 0x%x\n",lane,get_CurrentStep_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));
	HDMI_PRINTF("[HDMI] Lane%d Buffer mode = 0x%x\n",lane,get_BufferMode_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

	hdmi_rx_reg_mask32(REG_DFE_READBACK,~coef_set_mask,coef_sel(1),HDMI_RX_RBUS_DFE);
	//udelay(10);	//5000
	HDMI_PRINTF("[HDMI] Lane%d COEF Tap0=0x%02x  ",lane,get_TAP0_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

	hdmi_rx_reg_mask32(REG_DFE_READBACK,~coef_set_mask,coef_sel(2),HDMI_RX_RBUS_DFE);
	//udelay(10);	//5000
	HDMI_PRINTF("Tap1=0x%02x  ", get_TAP1_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

	hdmi_rx_reg_mask32(REG_DFE_READBACK,~coef_set_mask,coef_sel(3),HDMI_RX_RBUS_DFE);
	//udelay(10);	//5000
	HDMI_PRINTF("Tap2=%+d  ", (get_TAP2_coef_sign(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)))? -get_TAP2_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)): get_TAP2_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

	hdmi_rx_reg_mask32(REG_DFE_READBACK,~coef_set_mask,coef_sel(4),HDMI_RX_RBUS_DFE);
	//udelay(10);	//5000
	HDMI_PRINTF("Tap3=%+d  ", (get_TAP3_coef_sign(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)))? -get_TAP3_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)): get_TAP3_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

	hdmi_rx_reg_mask32(REG_DFE_READBACK,~coef_set_mask,coef_sel(5),HDMI_RX_RBUS_DFE);
	//udelay(10);	//5000
	HDMI_PRINTF("Tap4=%+d\n", (get_TAP4_coef_sign(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)))? -get_TAP4_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)): get_TAP4_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

	hdmi_rx_reg_mask32(REG_DFE_READBACK,~coef_set_mask,coef_sel(6),HDMI_RX_RBUS_DFE);
	//udelay(10);	//5000
	HDMI_PRINTF("[HDMI] Lane%d Servo COEF = 0x%x\n",lane,get_SERVO_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

	hdmi_rx_reg_mask32(REG_DFE_READBACK,~coef_set_mask,coef_sel(7),HDMI_RX_RBUS_DFE);
	//udelay(10);	//5000
	HDMI_PRINTF("[HDMI] Lane%d LE COEF = 0x%x\n\n",lane,get_LE1_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE)));

}

void SetupTMDSPhy(UINT32 b, char force) {

    int mode;
    UINT8 dfe_mode;
    UINT8  servo_lan[3];

    if ((HDMI_ABS(hdmi_ioctl_struct.b, b) < 4) && (force == 0)) return;
    pr_info("[SetupTMDSPhy b=%d]\n",b);

    hdmi_ioctl_struct.b = b;
    hdmi_ioctl_struct.b_change = 1;
    hdmi_phy_idle_patch.idle_count = 0;
    EQ_bias_band_setting(b);

	hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4, ~R_EQ_back_koff_en, R_EQ_back_koff_en, HDMI_RX_PHY);		// see Garren 0411 mail
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4, ~G_EQ_back_koff_en, G_EQ_back_koff_en, HDMI_RX_PHY);		// see Garren 0411 mail
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4, ~B_EQ_back_koff_en, B_EQ_back_koff_en, HDMI_RX_PHY);		// see Garren 0411 mail

    hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8, ~(R_S_BIAS_DFE|R_RL_SELB), R_RL_SELB, HDMI_RX_PHY);		// see Garren 0411 mail
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8, ~(G_S_BIAS_DFE|G_RL_SELB), G_RL_SELB, HDMI_RX_PHY);		// see Garren 0411 mail
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8, ~(B_S_BIAS_DFE|B_RL_SELB), B_RL_SELB, HDMI_RX_PHY);		// see Garren 0411 mail

#if  1
	hdmi_rx_reg_mask32(HDMI_TMDS_DPC_SET0_VADDR,~(TMDS_DPC_SET0_dpc_en_mask),0, HDMI_RX_MAC);   
    hdmi_rx_reg_mask32(HDMI_TMDS_PWDCTL_VADDR,~(TMDS_PWDCTL_ebip_mask|TMDS_PWDCTL_egip_mask|TMDS_PWDCTL_erip_mask|TMDS_PWDCTL_ecc_mask),0, HDMI_RX_MAC);                         //disable TMDS input
    hdmi_rx_reg_mask32(HDMI_TMDS_Z0CC_VADDR,~TMDS_Z0CC_hde_mask,0, HDMI_RX_MAC);                            //HDMI&DVI function disable
#endif
    //analog cdr reset
    hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~(HDMIPHY_REG_PHY_CTL_b_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_b_dig_rstb_mask|HDMIPHY_REG_PHY_CTL_g_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_g_dig_rstb_mask|HDMIPHY_REG_PHY_CTL_r_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_r_dig_rstb_mask),0, HDMI_RX_PHY);	//RGB CRD release
    if ( Auto_EQ )
    {
    	if (b>430 ) //  TMDS 45M > set auto EQ
    	{
    		HDMI_PRINTF("[HDMI] Auto EQ Enable\n");
    		dfe_mode = Hdmi_DFE_EQ_Set(b);
    	}
    	else
    	{
    		dfe_mode = 3;
    		DFE_Manual_Set();
    		HDMI_PRINTF("[HDMI] Manual mode \n");
    	}
    }
    else
    {
    	dfe_mode = 3;
    	DFE_Manual_Set();
    }

    for ( mode=0; mode<5; mode++)
    {
        if (hdmi_phy_param[mode].b_upper > b && hdmi_phy_param[mode].b_lower <= b)
        {
            hdmi_rx_reg_mask32(HDMIPHY_REG_CK_5_8,~CCO_Band_mask,(mode)?0: CCO_Band(1),HDMI_RX_PHY);	//CCO band 2~3.4GHz

            HDMI_Set_Phy_Table(mode);
            hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~(HDMIPHY_REG_PHY_CTL_ck_vcorstb_mask|HDMIPHY_REG_PHY_CTL_ck_pllrstb_mask),0,HDMI_RX_PHY);	//PLL clk reset
            HDMI_DELAYMS(1);
            //hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL_reg,~(HDMIPHY_REG_PHY_CTL_b_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_b_dig_rstb_mask|HDMIPHY_REG_PHY_CTL_g_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_g_dig_rstb_mask|HDMIPHY_REG_PHY_CTL_r_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_r_dig_rstb_mask),0);	//DPHY CDR reset
            hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~(HDMIPHY_REG_PHY_CTL_ck_vcorstb_mask|HDMIPHY_REG_PHY_CTL_ck_pllrstb_mask),(HDMIPHY_REG_PHY_CTL_ck_vcorstb_mask|HDMIPHY_REG_PHY_CTL_ck_pllrstb_mask),HDMI_RX_PHY);	//PLL clk reset
            //udelay(100);	//1000

            if ( Auto_EQ )
            {
                if (b>430 )
                {
                	//HDMI_PRINTF("[MHL] open switch Hdmi_DFE_EQ_Set bit 14 %x  \n",hdmi_rx_reg_read32(0xb8007d28));
                	//  if (b>=1280)	//>1.35GHz
                	if (dfe_mode == 2 )  // don't open LE
                	{
						hdmi_rx_reg_mask32(REG_DFE_EN_L0,~REG_DFE_EN_L0_dfe_adapt_en_lane0_mask,REG_DFE_EN_L0_dfe_adapt_en_lane0(0x7f),HDMI_RX_RBUS_DFE);	//lane0 adaptive enable
						hdmi_rx_reg_mask32(REG_DFE_EN_L1,~REG_DFE_EN_L1_dfe_adapt_en_lane1_mask,REG_DFE_EN_L1_dfe_adapt_en_lane1(0x7f),HDMI_RX_RBUS_DFE);	//lane1 adaptive enable
						hdmi_rx_reg_mask32(REG_DFE_EN_L2,~REG_DFE_EN_L2_dfe_adapt_en_lane2_mask,REG_DFE_EN_L2_dfe_adapt_en_lane2(0x7f),HDMI_RX_RBUS_DFE);	//lane2 adaptive enable
                	}
                	else if (dfe_mode == 3 )
                	{
                    #if ALL_ADP_OPEN  // all open
                		hdmi_rx_reg_mask32(REG_DFE_EN_L0,~REG_DFE_EN_L0_dfe_adapt_en_lane0_mask,REG_DFE_EN_L0_dfe_adapt_en_lane0(0xff),HDMI_RX_RBUS_DFE);	//lane0 adaptive enable
                		hdmi_rx_reg_mask32(REG_DFE_EN_L1,~REG_DFE_EN_L1_dfe_adapt_en_lane1_mask,REG_DFE_EN_L1_dfe_adapt_en_lane1(0xff),HDMI_RX_RBUS_DFE);	//lane1 adaptive enable
                		hdmi_rx_reg_mask32(REG_DFE_EN_L2,~REG_DFE_EN_L2_dfe_adapt_en_lane2_mask,REG_DFE_EN_L2_dfe_adapt_en_lane2(0xff),HDMI_RX_RBUS_DFE);	//lane2 adaptive enable
                    #else  // closed 234
                		hdmi_rx_reg_mask32(REG_DFE_EN_L0,~REG_DFE_EN_L0_dfe_adapt_en_lane0_mask,REG_dfe_adapt_en_lane0_TAP0|REG_dfe_adapt_en_lane0_TAP1|REG_dfe_adapt_en_lane0_SERVO|REG_dfe_adapt_en_lane0_Vth|REG_dfe_adapt_en_lane0_LE,HDMI_RX_RBUS_DFE);	//lane0 adaptive enable
                		hdmi_rx_reg_mask32(REG_DFE_EN_L1,~REG_DFE_EN_L1_dfe_adapt_en_lane1_mask,REG_dfe_adapt_en_lane1_TAP0|REG_dfe_adapt_en_lane1_TAP1|REG_dfe_adapt_en_lane1_SERVO|REG_dfe_adapt_en_lane1_Vth|REG_dfe_adapt_en_lane1_LE,HDMI_RX_RBUS_DFE);	//lane1 adaptive enable
                		hdmi_rx_reg_mask32(REG_DFE_EN_L2,~REG_DFE_EN_L2_dfe_adapt_en_lane2_mask,REG_dfe_adapt_en_lane2_TAP0|REG_dfe_adapt_en_lane2_TAP1|REG_dfe_adapt_en_lane2_SERVO|REG_dfe_adapt_en_lane2_Vth|REG_dfe_adapt_en_lane2_LE,HDMI_RX_RBUS_DFE);	//lane2 adaptive enable
                    #endif
                	}
                	//cloud modify 20140319
                	/*
                	hdmi_rx_reg_mask32(REG_DFE_EN_L0_reg,~REG_DFE_EN_L0_reduce_adapt_gain_lane0_mask,REG_DFE_EN_L0_reduce_adapt_gain_lane0(2));	//lane0 adaptive gain reduce
                	hdmi_rx_reg_mask32(REG_DFE_EN_L1_reg,~REG_DFE_EN_L1_reduce_adapt_gain_lane1_mask,REG_DFE_EN_L1_reduce_adapt_gain_lane1(2));	//lane1 adaptive gain reduce
                	hdmi_rx_reg_mask32(REG_DFE_EN_L2_reg,~REG_DFE_EN_L2_reduce_adapt_gain_lane2_mask,REG_DFE_EN_L2_reduce_adapt_gain_lane2(2));	//lane2 adaptive gain reduce
                	*/
                }
            }

            hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~(HDMIPHY_REG_PHY_CTL_b_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_b_dig_rstb_mask|HDMIPHY_REG_PHY_CTL_g_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_g_dig_rstb_mask|HDMIPHY_REG_PHY_CTL_r_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_r_dig_rstb_mask),(HDMIPHY_REG_PHY_CTL_b_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_b_dig_rstb_mask|HDMIPHY_REG_PHY_CTL_g_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_g_dig_rstb_mask|HDMIPHY_REG_PHY_CTL_r_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_r_dig_rstb_mask),HDMI_RX_PHY);	//RGB CRD release
            if ( Auto_EQ )
            {
                int lane;
                if (b>430 )
                {
                    HDMI_DELAYMS(1);
                    hdmi_rx_reg_mask32(REG_DFE_READBACK,~REG_DFE_READBACK_record_limit_en_mask,REG_DFE_READBACK_record_limit_en(1),HDMI_RX_RBUS_DFE);		//limit value record
                    HDMI_DELAYMS(1);
                    hdmi_rx_reg_mask32(REG_DFE_READBACK,~REG_DFE_READBACK_record_limit_en_mask,0,HDMI_RX_RBUS_DFE);		//limit value record

                    hdmi_rx_reg_mask32(REG_DFE_EN_L0,~REG_DFE_EN_L0_dfe_adapt_en_lane0_mask,0,HDMI_RX_RBUS_DFE);	//lane0 adaptive disable
                    hdmi_rx_reg_mask32(REG_DFE_EN_L1,~REG_DFE_EN_L1_dfe_adapt_en_lane1_mask,0,HDMI_RX_RBUS_DFE);	//lane1 adaptive disable
                    hdmi_rx_reg_mask32(REG_DFE_EN_L2,~REG_DFE_EN_L2_dfe_adapt_en_lane2_mask,0,HDMI_RX_RBUS_DFE);	//lane2 adaptive disable

                    //read result max and min value
                    for (lane=0; lane<3; lane++)
                    {
                    	Dump_DFE_Para(lane);
                    	hdmi_rx_reg_mask32(REG_DFE_READBACK,~coef_set_mask,coef_sel(6),HDMI_RX_RBUS_DFE);
                    	servo_lan[lane] = get_SERVO_coef(hdmi_rx_reg_read32(REG_DFE_READBACK,HDMI_RX_RBUS_DFE));
                    }
                }
            }

			OPEN_RST_MAC();
			hdmi_ioctl_struct.DEF_ready = 1;
			HDMI_PRINTF("SET phy finish\n");
            return;
        }

    }
}

//extern int HDMI_FormatDetect(void);
int HdmiMeasureVedioClock(void) {

	UINT32 b;
#if HDMI_DEBUG_AUDIO_PLL_RING
	UINT32 value;
#endif

	DEBUG_PRINTK("HDMI_HDMI_NTX1024TR0_VADDR = %x\n", hdmiport_in(HDMI_HDMI_NTX1024TR0_VADDR));
#if HDMI_DEBUG_AUDIO_PLL_RING
	 if (audio_pll_buffer.enable) {
				audio_pll_buffer.buffer[audio_pll_buffer.ring_end].audio_disable = (hdmiport_in(HDMI_HDMI_AVMCR_VADDR) & _BIT5) != 0;
				audio_pll_buffer.buffer[audio_pll_buffer.ring_end].overflow = (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR) & _BIT1 ) != 0;
				audio_pll_buffer.buffer[audio_pll_buffer.ring_end].underflow = (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR) & _BIT2 ) != 0;
				audio_pll_buffer.buffer[audio_pll_buffer.ring_end].pll_unstable = (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR) & _BIT3) != 0;
				audio_pll_buffer.buffer[audio_pll_buffer.ring_end].cts = HDMI_ACRSR0_get_cts(hdmi_rx_reg_read32(HDMI_ACRSR0_reg));
				audio_pll_buffer.buffer[audio_pll_buffer.ring_end].n = HDMI_ACRSR1_get_n(hdmi_rx_reg_read32(HDMI_ACRSR1_reg));
				value =  HDMI_APLLCR3_get_sum_c_samp(hdmi_rx_reg_read32(HDMI_APLLCR3_reg));
				if (value & 0x8000) {
					value = ~value;
					audio_pll_buffer.buffer[audio_pll_buffer.ring_end].sum_c = value + 1;
					audio_pll_buffer.buffer[audio_pll_buffer.ring_end].sum_c  = -audio_pll_buffer.buffer[audio_pll_buffer.ring_end].sum_c;

				} else {
					audio_pll_buffer.buffer[audio_pll_buffer.ring_end].sum_c  = value;
				}
				audio_pll_buffer.ring_end ++;
				if (audio_pll_buffer.ring_end == AUDIO_PLL_RING_SIZE)
					audio_pll_buffer.ring_end = 0;
				if (audio_pll_buffer.ring_start == audio_pll_buffer.ring_end) {
					audio_pll_buffer.ring_start ++;
					if (audio_pll_buffer.ring_start == AUDIO_PLL_RING_SIZE)
						audio_pll_buffer.ring_start = 0;
				}
	 }
#endif
#if 1
	 if (hdmi_rx_reg_read32(HDMI_HDMI_GPVS_VADDR, HDMI_RX_MAC) & _BIT5) {
#if HDMI_DEBUG_COLOR_SPACE
	 	hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 78, HDMI_RX_MAC);
 		HDMI_PRINTF( "AVI info in %x ", hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC));
	 	hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 80, HDMI_RX_MAC);
 		HDMI_PRINTF("%x ", hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC));
	 	hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 82);
 		HDMI_PRINTF("%x %d\n", hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR), hdmi_ioctl_struct.avi_info_miss_cnt);
#endif
	      	hdmi_ioctl_struct.avi_info_in = 1;
	       hdmi_rx_reg_write32(HDMI_HDMI_GPVS_VADDR, _BIT5, HDMI_RX_MAC);
	       hdmi_ioctl_struct.avi_info_miss_cnt = 0;
	 } else {
	      hdmi_ioctl_struct.avi_info_miss_cnt ++;
	      if (hdmi_ioctl_struct.avi_info_miss_cnt > 200) {
//	  		    HDMI_PRINTF("AVI info miss %x \n", hdmiport_in(HDMI_HDMI_ACRCR_RO_VADDR));
		  	    hdmi_ioctl_struct.avi_info_in = 0;
			    hdmi_ioctl_struct.avi_info_miss_cnt = 0;
	      }
	 }
#endif
	 if (Hdmi_IsbReady() == TRUE)
	 {
		b =  Hdmi_get_b_value();

		hdmi_ioctl_struct.b_debouce_count = 0;
		//pr_info("[b=%d,pre_b=%d,b_count=%d]\n",b, hdmi_ioctl_struct.b_pre,hdmi_ioctl_struct.b_count);
		if ((HDMI_ABS(b, hdmi_ioctl_struct.b_pre) < 5)&&(b != 0 )) {
			if (hdmi_ioctl_struct.b_count >= HDMI_B_TEST_COUNT_MAX) {
				//pr_info("[b=%d]\n",b);
				if(!(hdmi_rx_reg_read32(HDMI_HDMI_AFCR_VADDR, HDMI_RX_MAC)&(1<<29)))
				{
					//pr_info("[SET TMDS PHY~~~~]\n");
					SetupTMDSPhy(b, 0);
					//pr_info("[TMDSPHY 3]\n");
				}
//						SetupTMDSPhy(b, 0);
				hdmi_ioctl_struct.b_count = 0;
			} else {
				hdmi_ioctl_struct.b_count++;

			}
		} else {
			hdmi_ioctl_struct.b_pre = b;
			hdmi_ioctl_struct.b_count = 0;
		}
//		pr_info("[2.b=%d]\n",hdmi_ioctl_struct.b);
		Hdmi_TriggerbMeasurement();
//		HDMI_FormatDetect();
//		pr_info("[Trigger end]\n");
	}else {
		DEBUG_PRINTK("measure vedio time out \n");
		Hdmi_TriggerbMeasurement();
		hdmi_ioctl_struct.b_debouce_count ++;
		if (hdmi_ioctl_struct.b_debouce_count < 2) {
			DEBUG_PRINTK("debounce vedio time out\n");
			return 0;
		}
		else {
			hdmi_ioctl_struct.b_count = 0;
			hdmi_ioctl_struct.b_debouce_count = 0;
			hdmi_ioctl_struct.b  = 0;
			hdmi_ioctl_struct.b_pre = 0;
			//pr_info("[3.b=%d]\n",hdmi_ioctl_struct.b);
			return 0;
		}
	}

	return -1;
}


void HdmiPhyElectricalIdlePatch(void) {

		if (hdmi_ioctl_struct.b != 0 && HDMI_ABS(hdmi_ioctl_struct.b, hdmi_ioctl_struct.b_pre) < 3) {
			if ((hdmi_phy_idle_patch.apstatus == HDMIRX_DETECT_FAIL)||(hdmi_phy_idle_patch.apstatus == HDMIRX_DETECT_AVMUTE)) {
				hdmi_phy_idle_patch.idle_count ++;
				if (hdmi_phy_idle_patch.idle_count >= HDMI_PHY_IDLE_PATCH_COUNT) {
					HDMI_PRINTF("************* phy idle patch **************\n");
					#if MHL_SUPPORT
                             	if(GET_ISMHL())
                             	{
                             		SetupMHLTMDSPhy(hdmi_ioctl_struct.b, 1);
                             	}
				      else
					#endif
					SetupTMDSPhy(hdmi_ioctl_struct.b, 1);
					pr_info("[TMDSPHY 2]\n");
				}
			}
		}
}

/**
 * Hdmi_HdcpPortRead
 * HDCP DDC port setting read function
 * where HDCP port ddc address for HOST side is 0x74/0x75
 *
 * @param <addr>			{ hdcp register address }
 * @return 				{ read data }
 * @ingroup drv_hdmi
 */
UINT8 Hdmi_HdcpPortRead(UINT8 addr)
{
	hdmi_rx_reg_write32(HDMI_HDCP_AP_VADDR, addr, HDMI_RX_MAC);
	return hdmi_rx_reg_read32(HDMI_HDCP_DP_VADDR, HDMI_RX_MAC);
}
/*======================= API Function ========================*/
/**
 * IsHDMI
 * check is HDMI/DVI
 *
 * @param 				{  }
 * @return 				{ return MODE of detection, MODE_HDMI = 1, MODE_DVI = 0 }
 * @ingroup drv_hdmi
 */
HDMI_DVI_MODE_T IsHDMI_isr(void)
{

	if(hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & _BIT0) {
//		HDMI_PRINTF("<<<<<<<<<< HDMI mode \n");
		return MODE_HDMI; // HDMI mode
	}  else {
//		HDMI_PRINTF("<<<<<<<<<< DVI mode \n");
		return MODE_DVI; // DVI mode
	}
}

/*solved K-6258 repeater&LG bug USER:kistlin DATE:2010/12/14*/
void HdmiLGPatchFSM(void) {
	static UINT8 hdcp_flag = 0;
	static UINT32 i=0;
	UINT8 result = Hdmi_HdcpPortRead(0xC1) & ( _BIT5|_BIT6) ? 1:0;
	UINT8 Aksv_flag = 0, Ri_flag = 0;
	UINT8 result2 = (hdmi_rx_reg_read32(HDMI_HDCP_FLAG1_VADDR, HDMI_RX_MAC)&HDCP_FLAG1_wr_aksv_flag_mask)?1:0;//Aksv flag

	if (result2 == 1){//Tx write Aksv

		if (IsHDMI_isr()){//HDMI mode
			HDMI_PRINTF("\n i=%d  HDMI AKSV Flag = %x, %x ,%x ,%x, %x \n",i ,  Hdmi_HdcpPortRead(0x10), Hdmi_HdcpPortRead(0x11), Hdmi_HdcpPortRead(0x12), Hdmi_HdcpPortRead(0x13), Hdmi_HdcpPortRead(0x14));
			hdmi_rx_reg_write32(HDMI_HDCP_FLAG1_VADDR, HDCP_FLAG1_wr_aksv_flag_mask, HDMI_RX_MAC);//clear Aksv flag
#if K6258_Repeater
                   //SET_ISHDMI(IsHDMI());    // tingguang_he debug on
			hdmi_rx_reg_mask32(HDMI_HDMI_SCR_VADDR , ~(_BIT3|_BIT2),_BIT3|_BIT2, HDMI_RX_MAC);//set force hdmi mode
#endif
			Aksv_flag = 1;
		}
	}

#if LG_DF9921N_PWR_ON
	UINT8 result3 = (hdmi_rx_reg_read32(HDMI_HDCP_FLAG1_VADDR, HDMI_RX_MAC)&HDCP_FLAG1_rd_ri_flag_mask)?1:0;//Read Ri flag

       if (result3 == 1){//Tx Read Ri
              hdmi_rx_reg_write32(HDMI_TMDS_ACC1_VADDR,HDCP_FLAG1_rd_ri_flag_mask);//clear Read Ri flag
              HDMI_PRINTF("\n  i=%d Read Ri \n",i);

	       if (AVMute_Timer == 3)
		   Ri_flag = 1;
       }
#endif

	if (Aksv_flag == 1){
		Aksv_flag = 0;//clear aksv flag
		i=0;
		hdmi_ioctl_struct.LG_patch_timer = 350;
		AVMute_Timer=1;
	}
	else if ((result ^ hdcp_flag)&&(result == 0)) {
		HDMI_PRINTF("\nhdcp_flag change but result  = 0 !!!!\n");
		hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR, ~(HDMI_AVMCR_avmute_flag_mask | HDMI_AVMCR_avmute_win_en_mask),0, HDMI_RX_MAC);//set auto mode	/*solved K-6258 repeater bug USER:kistlin DATE:2010/09/14*/
		AVMute_Timer = 0;
	}
	hdcp_flag= result;

	switch(AVMute_Timer) {
		case 0:
			hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR, ~(HDMI_AVMCR_avmute_flag_mask), 0x0, HDMI_RX_MAC);
			break;
		case 1:
			i++;
			/*solved K-6258 repeater bug
			USER:kistlin DATE:2010/09/14*/
#if K6258_Repeater
			if (hdmi_ioctl_struct.LG_patch_timer  == 200){
				HDMI_PRINTF("\n  Clear Force HDMI mode \n");
				hdmi_rx_reg_mask32(HDMI_HDMI_SCR_VADDR , ~(_BIT3|_BIT2),0, HDMI_RX_MAC);//set auto mode
			}
#endif
			if (hdmi_ioctl_struct.LG_patch_timer  == 0){
				HDMI_PRINTF("\n i= %d ,AVMute Clear Start .......\n",i);
#if LG_DF9921N/*USER:kislin DATE:2011/11/16*/
				hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR, ~HDMI_AVMCR_avmute_flag_mask, HDMI_AVMCR_avmute_flag(1), HDMI_RX_MAC);
#endif
				AVMute_Timer = 2;
				hdmi_ioctl_struct.LG_patch_timer = 200;
			}
			break;
		case 2:
			i++;
			if (hdmi_ioctl_struct.LG_patch_timer == 0){
				hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR, ~HDMI_AVMCR_avmute_flag_mask, HDMI_AVMCR_avmute_flag(0), HDMI_RX_MAC);
				AVMute_Timer = 3;
				HDMI_PRINTF("\nAVMute Clear End !!!!!\n");
			}
			break;
		case 3:
			i++;
			if (Ri_flag == 1){
				HDMI_PRINTF("\n i= %d ,AVMute Clear Start 2 .......\n",i);
				hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR, ~HDMI_AVMCR_avmute_flag_mask, HDMI_AVMCR_avmute_flag(1), HDMI_RX_MAC);
				AVMute_Timer = 4;
				hdmi_ioctl_struct.LG_patch_timer = 500;
			}
			break;
		case 4:
			i++;
			if (hdmi_ioctl_struct.LG_patch_timer == 0){
				hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR, ~HDMI_AVMCR_avmute_flag_mask, HDMI_AVMCR_avmute_flag(0), HDMI_RX_MAC);
				AVMute_Timer = 0;
				HDMI_PRINTF("\nAVMute Clear End  2 !!!!\n");
			}
			break;
		default:
			HDMI_PRINTF("\n i= %d ,AVMute Clear ERROR.......\n",i);
			hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR, ~HDMI_AVMCR_avmute_flag_mask, HDMI_AVMCR_avmute_flag(0), HDMI_RX_MAC);
			break;
	}
}
#define CONTINUOUS_MODE   0
#define ONE_SHOT_MODE  1
#define VIDEO_GB_Channel_B_Value  0x2CC
#define GB_FAIL 0x01
#define GB_SUCCESS  0x00
/*========================================================================================
Function: Check_GB_Stable()
Author: Cloud Wu
Para:   return Success or Fail
Note: None;
=========================================================================================*/
int rtd_hdmiCheck_GB_Stable(UINT8 bMode ,UINT16 *wValue1)
{
      UINT8 result =0;//FailCnt =0,SuccessCnt =0;
      UINT16 wValue2;
	UINT32 bTemp;
     // hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~(_BIT14|_BIT13|_BIT12|_BIT11) ,0<<11);  //bad EQ Gain
      HDMI_PRINTF("\n rtd_hdmiCheck_GB_Stable....... Start!!\n");
	  //Disable watch dog & interrupt
      hdmi_rx_reg_mask32(HDMI_LEADING_GB_CMP_CTRL, ~(HDMI_LEADING_GB_CMP_CTRL_cmp_err_wd_en_mask|HDMI_LEADING_GB_CMP_CTRL_cmp_err_int_en_mask),
	 HDMI_LEADING_GB_CMP_CTRL_cmp_err_wd_en(0)|HDMI_LEADING_GB_CMP_CTRL_cmp_err_int_en(0) , HDMI_RX_MAC);
	  //Set Video preamble
      hdmi_rx_reg_mask32(HDMI_LEADING_GB_CMP_CTRL, ~(HDMI_LEADING_GB_CMP_CTRL_preamble_mask),HDMI_LEADING_GB_CMP_CTRL_preamble(1), HDMI_RX_MAC);
	  //Set HDMI Mode & sel channel B
      hdmi_rx_reg_mask32(HDMI_LEADING_GB_CMP_CTRL, ~(HDMI_LEADING_GB_CMP_CTRL_dvi_mode_sel_mask|HDMI_LEADING_GB_CMP_CTRL_channel_sel_mask),
      HDMI_LEADING_GB_CMP_CTRL_dvi_mode_sel(0)|HDMI_LEADING_GB_CMP_CTRL_channel_sel(0), HDMI_RX_MAC);
	  //
	hdmi_rx_reg_mask32(HDMI_LEADING_GB_CMP_CTRL, ~(HDMI_LEADING_GB_CMP_CTRL_cmp_value_mask),HDMI_LEADING_GB_CMP_CTRL_cmp_value(VIDEO_GB_Channel_B_Value), HDMI_RX_MAC);
	switch(bMode)
	{
	     case  CONTINUOUS_MODE:


               break;

	     case  ONE_SHOT_MODE:
	//	for(i=0;i<3;i++)
		{
	      		hdmi_rx_reg_mask32(HDMI_LEADING_GB_CMP_TIMES,~HDMI_LEADING_GB_CMP_TIMES_cmp_times_mask,HDMI_LEADING_GB_CMP_TIMES_cmp_times(0x1000) , HDMI_RX_MAC);// cloud note this reg value is important
            		hdmi_rx_reg_mask32(HDMI_LEADING_GB_CMP_CTRL, ~(HDMI_LEADING_GB_CMP_CTRL_lgb_cal_conti_mask|HDMI_LEADING_GB_CMP_CTRL_lgb_cal_en_mask),
				(HDMI_LEADING_GB_CMP_CTRL_lgb_cal_conti(0)|HDMI_LEADING_GB_CMP_CTRL_lgb_cal_en(1)), HDMI_RX_MAC);
			hdmi_rx_reg_mask32(HDMI_LEADING_GB_CMP_CTRL, ~(HDMI_LEADING_GB_CMP_CTRL_lgb_cal_en_mask),
				(HDMI_LEADING_GB_CMP_CTRL_lgb_cal_en(1)), HDMI_RX_MAC);// must dummy write bit 0 to reset HW


			do{
				bTemp =  hdmi_rx_reg_read32(HDMI_LEADING_GB_CMP_CTRL, HDMI_RX_MAC) ;
				//pr_info("[In while loop 1]\n");
			}  while( bTemp & HDMI_LEADING_GB_CMP_CTRL_lgb_cal_en_mask);
		//HDMI_PRINTF(" btemp = 0x%x ",bTemp);

	      if (( bTemp & HDMI_LEADING_GB_CMP_CTRL_cmp_err_flag_mask)&& (hdmi_rx_reg_read32(HDMI_LEADING_GB_CMP_CNT, HDMI_RX_MAC)&HDMI_LEADING_GB_CMP_CNT_preamble_cmp_cnt_mask))
	      	{
	      	     result=GB_FAIL;
	      	/*
	      	    FailCnt++;
		    if(FailCnt >1)  break;
		    */
	      	}
		 else
		 {
		     result=GB_SUCCESS;
		 /*
		    SuccessCnt++;
		    if(SuccessCnt >1)  break;
		    */
		 }
		}
		/*
		if(FailCnt >SuccessCnt)   result=FAILED;
		else                             result=SUCCESS;
		*/
		*wValue1 = (hdmi_rx_reg_read32(HDMI_LEADING_GB_CMP_TIMES, HDMI_RX_MAC)>>16);
		wValue2 =  (hdmi_rx_reg_read32(HDMI_LEADING_GB_CMP_TIMES, HDMI_RX_MAC)>>16);
	//	wValue2 = (hdmi_rx_reg_read32(HDMI_LEADING_GBCMP_CNT_reg)>>16);
	//	HDMI_PRINTF(" GB Check finsih !! Compare Error Cnt = %d ,Total Compare Cnt = %d  , RESULT = %d \n" ,wValue1 ,wValue2 ,result);
	      HDMI_PRINTF(" GB Check finsih !! ,result = %d ,Error Cnt = %x , wValue2 = %d\n",result,*wValue1 ,wValue2);
		 break;
	}
	return result;
}

/**********************************************************************************
Function: rtd_hdmiPhy_ISR()
Author: Cloud Wu
Para:   void
Note: Timer interrupt  10 msec
***********************************************************************************/
void rtd_hdmiPhy_ISR(void) {
//		UINT8 bAksvTemp ,bBksvTemp ;
//		UINT8 hdmi_ap_addr = hdmi_rx_reg_read32(HDMI_HDMI_AP_VADDR);
		UINT8 hdmi_psap_addr = hdmi_rx_reg_read32(HDMI_HDMI_PSAP_VADDR, HDMI_RX_MAC);
//		DEBUG_PRINTK("%s\n" , __FUNCTION__);

//		if ((hdmi_rx_reg_read32(TC_TMR5_VR_VADDR) & 1) == 0) return 0;

		if (hdmi_ioctl_struct.LG_patch_timer != 0) hdmi_ioctl_struct.LG_patch_timer--;

		if (hdmi_ioctl_struct.timer != 0) hdmi_ioctl_struct.timer--;

	#if  MHL_SUPPORT
	if((GET_HDMI_CHANNEL() ==MHL_CHANNEL))
	{
           CBUS_TIMER_Handle_ISR();
	     MHLCBUS_ISR();
	}
	#endif
	#if HDMI_FASTER_BOOT
		if (hdmi_ioctl_struct.hotplug_timer >= 0) {
			if (hdmi_ioctl_struct.hotplug_timer == 0) {
				HDMI_HPD_LEVEL(fast_boot_channel.HotPlugPin, fast_boot_channel.HotPlugOn);
			}
			hdmi_ioctl_struct.hotplug_timer --;
		}
	#endif
		HdmiMeasureVedioClock();

		#if HDMI_PHY_IDLE_PATCH_ENABLE
		HdmiPhyElectricalIdlePatch();
		#endif


//#if LG_DF9921N /*move in HdmiLGPatchFSM USER:kislin DATE:2011/11/16*/
		HdmiLGPatchFSM();
//#endif

		hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, hdmi_psap_addr, HDMI_RX_MAC);
//		hdmi_rx_reg_write32(HDMI_HDMI_AP_VADDR, hdmi_ap_addr);
//		hdmi_rx_reg_write32(TC_TMR5_VR_VADDR, 1);   			    // clear interrupt pending
//		hdmi_rx_reg_write32(TC_TMR5_CR_VADDR, 0);
//		hdmi_rx_reg_write32(TC_TMR5_CR_VADDR, _BIT31 | _BIT29);
		return ;//IRQ_HANDLED;

}

void DFEinitial(void)
{
    hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~HDMIPHY_REG_PHY_CTL_dfe_rstn_n_mask,0,HDMI_RX_PHY); //DFE reset
    hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~R_DFE_ADAPT_EN,0,HDMI_RX_PHY);   //R adaptive disable
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_9_12,~G_DFE_ADAPT_EN,0,HDMI_RX_PHY);   //G adaptive disable
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_9_12,~B_DFE_ADAPT_EN,0,HDMI_RX_PHY);   //B adaptive disable
    hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~(HDMIPHY_REG_PHY_CTL_b_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_b_dig_rstb_mask|HDMIPHY_REG_PHY_CTL_g_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_g_dig_rstb_mask|HDMIPHY_REG_PHY_CTL_r_dig_cdr_rstb_mask|HDMIPHY_REG_PHY_CTL_r_dig_rstb_mask),0,HDMI_RX_PHY);   //RGB CDR reset
}

void Z0_Calibration (void)
{
#ifndef ManualZ0
    UINT8 TimeOut = 10;
#endif
    //HDMI setting
    hdmi_rx_reg_mask32(REG_MHL_CTRL,~REG_MHL_CTRL_reg_mhlpow_mask,REG_MHL_CTRL_reg_mhlpow(0),HDMI_RX_PHY);
    hdmi_rx_reg_mask32(REG_MHL_CTRL,~REG_MHL_CTRL_reg_mhl_hdmisel_mask,REG_MHL_CTRL_reg_mhl_hdmisel(1),HDMI_RX_PHY);
    hdmi_rx_reg_mask32(HDMIPHY_REG_CK_9_12,~MHL_Z100K,MHL_Z100K,HDMI_RX_PHY);       //Z100K disable
    hdmi_rx_reg_mask32(REG_MHL_CTRL,~(REG_MHL_CTRL_reg_z300pow_mask|REG_MHL_CTRL_reg_z300_sel_2_mask|REG_MHL_CTRL_reg_z300_sel_1_mask|REG_MHL_CTRL_reg_z300_sel_0_mask),(REG_MHL_CTRL_reg_z300pow(7)|REG_MHL_CTRL_reg_z300_sel_2(1)|REG_MHL_CTRL_reg_z300_sel_1(1)|REG_MHL_CTRL_reg_z300_sel_0(1)),HDMI_RX_PHY);    //Z300 enable & sel Vterm
    hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL_2,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_ck_z0pow_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_b_z0pow_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_g_z0pow_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_r_z0pow_mask),(HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_ck_z0pow(7)|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_b_z0pow(7)|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_g_z0pow(7)|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_r_z0pow(7)),HDMI_RX_PHY);       //Z60 register on

#ifndef ManualZ0
    //Z0 calibration
    hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0,~HDMIPHY_REG_BANDGAP_Z0_z0tune_mask,HDMIPHY_REG_BANDGAP_Z0_z0tune(1),HDMI_RX_PHY);        //Z0 calibration sel auto
    hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0,~HDMIPHY_REG_BANDGAP_Z0_z0pow_mask,HDMIPHY_REG_BANDGAP_Z0_z0pow(1),HDMI_RX_PHY);          //Z0 calibration power on
    do
    {
        HDMI_DELAYMS(1);
        if(hdmi_rx_reg_read32(HDMIPHY_REG_BANDGAP_Z0,HDMI_RX_PHY) & HDMIPHY_REG_BANDGAP_Z0_z0_ok_mask)      //wait Z0 calibrate finish
        {
            HDMI_PRINTF("Z0 calibrate result = 0x%x\n",HDMIPHY_REG_BANDGAP_Z0_get_z0_res(hdmi_rx_reg_read32(HDMIPHY_REG_BANDGAP_Z0_reg)));
            break;
        }
    } while (--TimeOut);
    if (!TimeOut) {
        HDMI_PRINTF("Auto Z0 Calibration Time Out\n");
    }
#else
    //Z0 manual
    hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0,~HDMIPHY_REG_BANDGAP_Z0_z0tune_mask,0,HDMI_RX_PHY);
    hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_adjr_0_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_adjr_1_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_adjr_2_mask),(HDMIPHY_REG_BANDGAP_Z0_CTRL_adjr_0(ManualZ0)|HDMIPHY_REG_BANDGAP_Z0_CTRL_adjr_1(ManualZ0)|HDMIPHY_REG_BANDGAP_Z0_CTRL_adjr_2(ManualZ0)),HDMI_RX_PHY);
#endif
}

void EQ_HBR_Setting(void)
{
	hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~R_EQ_bias_current_mask,R_EQ_bias_current(6),HDMI_RX_PHY);         //R EQ bias 6 for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~G_EQ_bias_current_mask,G_EQ_bias_current(6),HDMI_RX_PHY);         //G EQ bias 6 for over 1.1 GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~B_EQ_bias_current_mask,B_EQ_bias_current(6),HDMI_RX_PHY);         //B EQ bias 6 for over 1.1 GHz

	hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~R_Rhase_Routator_Resistor,R_Rhase_Routator_Resistor,HDMI_RX_PHY);     //R PRR for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~G_Rhase_Routator_Resistor,G_Rhase_Routator_Resistor,HDMI_RX_PHY);     //G PRR for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~B_Rhase_Routator_Resistor,B_Rhase_Routator_Resistor,HDMI_RX_PHY);     //B PRR for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~R_EQ_HBR,R_EQ_HBR,HDMI_RX_PHY);   //R High bit rate  for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~G_EQ_HBR,G_EQ_HBR,HDMI_RX_PHY);   //G High bit rate  for over 1.1GHz
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~B_EQ_HBR,B_EQ_HBR,HDMI_RX_PHY);   //B High bit rate  for over 1.1GHz
}

void EQ_KOFF_Cali(UINT8 Range, UINT8 mode )
{
    UINT8 Koffset_result_flag=0, TimeOut, i ,OFFSET_Result;
    UINT32 reg7be0, reg7b50;

    hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~(R_InOff|R_Xtal_for_Koffset_enable),(R_InOff|R_Xtal_for_Koffset_enable),HDMI_RX_PHY);        //R input short GND
    //Load initial tap0 value
#if 1     //20140307

    // hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~(R_InOff|R_Xtal_for_Koffset_enable),(R_InOff|R_Xtal_for_Koffset_enable));       //R input short GND
    //Load initial tap0 value

    if (mode ==MHL_3G_enbuffer_on)
    {
        hdmi_rx_reg_mask32(REG_DFE_INIT0_L0,~REG_DFE_INIT0_L0_tap0_init_lane0_mask,REG_DFE_INIT0_L0_tap0_init_lane0(0x12),HDMI_RX_RBUS_DFE);  //lane0 tap0 initial    //0xc
        hdmi_rx_reg_mask32(REG_DFE_INIT0_L1,~REG_DFE_INIT0_L1_tap0_init_lane1_mask,REG_DFE_INIT0_L1_tap0_init_lane1(0x12),HDMI_RX_RBUS_DFE);  //lane1 tap0 initial    //0xc
        hdmi_rx_reg_mask32(REG_DFE_INIT0_L2,~REG_DFE_INIT0_L2_tap0_init_lane2_mask,REG_DFE_INIT0_L2_tap0_init_lane2(0x12),HDMI_RX_RBUS_DFE);  //lane2 tap0 initial    //0xc
    }
    else
    {
        hdmi_rx_reg_mask32(REG_DFE_INIT0_L0,~REG_DFE_INIT0_L0_tap0_init_lane0_mask,REG_DFE_INIT0_L0_tap0_init_lane0(0xc),HDMI_RX_RBUS_DFE);   //lane0 tap0 initial    //0xc
        hdmi_rx_reg_mask32(REG_DFE_INIT0_L1,~REG_DFE_INIT0_L1_tap0_init_lane1_mask,REG_DFE_INIT0_L1_tap0_init_lane1(0xc),HDMI_RX_RBUS_DFE);   //lane1 tap0 initial    //0xc
        hdmi_rx_reg_mask32(REG_DFE_INIT0_L2,~REG_DFE_INIT0_L2_tap0_init_lane2_mask,REG_DFE_INIT0_L2_tap0_init_lane2(0xc),HDMI_RX_RBUS_DFE);   //lane2 tap0 initial    //0xc

    }

    hdmi_rx_reg_mask32(REG_DFE_INIT1_L0,~(1<<24),0,HDMI_RX_RBUS_DFE);        //lane0  initial load
    hdmi_rx_reg_mask32(REG_DFE_INIT1_L1,~(1<<24),0,HDMI_RX_RBUS_DFE);        //lane1  initial load
    hdmi_rx_reg_mask32(REG_DFE_INIT1_L2,~(1<<24),0,HDMI_RX_RBUS_DFE);        //lane2  initial load
    HDMI_DELAYMS(1);
    hdmi_rx_reg_mask32(REG_DFE_INIT1_L0,~(1<<24),(1<<24),HDMI_RX_RBUS_DFE);       //lane0  initial load
    hdmi_rx_reg_mask32(REG_DFE_INIT1_L1,~(1<<24),(1<<24),HDMI_RX_RBUS_DFE);       //lane1  initial load
    hdmi_rx_reg_mask32(REG_DFE_INIT1_L2,~(1<<24),(1<<24),HDMI_RX_RBUS_DFE);       //lane2  initial load
    HDMI_DELAYMS(1);
    hdmi_rx_reg_mask32(REG_DFE_INIT1_L0,~(1<<24),0,HDMI_RX_RBUS_DFE);        //lane0  initial load
    hdmi_rx_reg_mask32(REG_DFE_INIT1_L1,~(1<<24),0,HDMI_RX_RBUS_DFE);        //lane1  initial load
    hdmi_rx_reg_mask32(REG_DFE_INIT1_L2,~(1<<24),0,HDMI_RX_RBUS_DFE);        //lane2  initial load



     HDMI_PRINTF("set tap0 initial \n");
#endif
/*  // close En_able buffer
    hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8,~HDMIPHY_REG_R_5_8_enbuffer3_mask, HDMIPHY_REG_R_5_8_enbuffer3(1)); // 20140304
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8,~HDMIPHY_REG_G_5_8_enbuffer3_mask, HDMIPHY_REG_G_5_8_enbuffer3(1)); // 20140304
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~HDMIPHY_REG_B_5_8_enbuffer3_mask, HDMIPHY_REG_B_5_8_enbuffer3(1)); // 20140304
    */
 //   hdmi_rx_reg_mask32(HDMIPHY_REG_DIG_PHY_CTRL_NEW_1,~HDMIPHY_REG_DIG_PHY_CTRL_NEW_1_reg_ldo_sel_mask,HDMIPHY_REG_DIG_PHY_CTRL_NEW_1_reg_ldo_sel(3));//LDO 1.1

    hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~HDMIPHY_REG_R_9_12_tap0_bias_cur_mask, HDMIPHY_REG_R_9_12_tap0_bias_cur(2),HDMI_RX_PHY); // 20140304
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_9_12,~HDMIPHY_REG_G_9_12_tap0_bias_cur_mask, HDMIPHY_REG_G_9_12_tap0_bias_cur(2),HDMI_RX_PHY); // 20140304
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_9_12,~HDMIPHY_REG_B_9_12_tap0_bias_cur_mask, HDMIPHY_REG_B_9_12_tap0_bias_cur(2),HDMI_RX_PHY); // 20140304

    hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~HDMIPHY_REG_R_9_12_reg_R_CDR_SBIAS_MASK,(1<<19),HDMI_RX_PHY); // 20140304
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_9_12,~HDMIPHY_REG_R_9_12_reg_R_CDR_SBIAS_MASK, (1<<19),HDMI_RX_PHY); // 20140304
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_9_12,~HDMIPHY_REG_R_9_12_reg_R_CDR_SBIAS_MASK, (1<<19),HDMI_RX_PHY); // 20140304


    Koffset_result_flag=0;

    reg7be0 = hdmi_rx_reg_read32(REG_MHL_CTRL,HDMI_RX_PHY);
    reg7b50 = hdmi_rx_reg_read32(HDMIPHY_REG_BANDGAP_Z0_CTRL_2,HDMI_RX_PHY);
    hdmi_rx_reg_mask32(REG_MHL_CTRL,~(REG_MHL_CTRL_reg_z300pow_mask|REG_MHL_CTRL_reg_z300_sel_2_mask|REG_MHL_CTRL_reg_z300_sel_1_mask|REG_MHL_CTRL_reg_z300_sel_0_mask), 0,HDMI_RX_PHY); //Z300 disable
    hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL_2,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_ck_z0pow_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_b_z0pow_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_g_z0pow_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_r_z0pow_mask), 0,HDMI_RX_PHY);        //Z60 register off

    hdmi_rx_reg_mask32(HDMIPHY_REG_R_5_8, ~R_EQ_HBR, 0,HDMI_RX_PHY);
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_5_8, ~G_EQ_HBR, 0,HDMI_RX_PHY);
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8, ~B_EQ_HBR, 0,HDMI_RX_PHY);
    hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4, ~R_KOFF_BG_Range_mask, 0,HDMI_RX_PHY); //R background K Off range
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4, ~G_KOFF_BG_Range_mask, 0,HDMI_RX_PHY); //G background K Off range
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4, ~B_KOFF_BG_Range_mask, 0,HDMI_RX_PHY); //B background K Off range
//Koffset calibration
    //CK channel CKPOW1 on
    //Reg_Z300-->1, Reg_HML_HDMI-->1, (Z0_POW-->0?)
    hdmi_rx_reg_mask32(REG_MHL_CTRL,~REG_MHL_CTRL_reg_mhl_hdmisel_mask,REG_MHL_CTRL_reg_mhl_hdmisel(1),HDMI_RX_PHY); //HDMI sel
    hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~(HDMIPHY_REG_R_1_4_R_SCHR0|HDMIPHY_REG_R_1_4_R_SCHR1|HDMIPHY_REG_R_1_4_R_SCHR2),0,HDMI_RX_PHY);                  //R input port sel
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~(HDMIPHY_REG_G_1_4_G_SCHR0|HDMIPHY_REG_G_1_4_G_SCHR1|HDMIPHY_REG_G_1_4_G_SCHR2),0,HDMI_RX_PHY);                  //G input port sel
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(HDMIPHY_REG_B_1_4_B_SCHR0|HDMIPHY_REG_B_1_4_B_SCHR1|HDMIPHY_REG_B_1_4_B_SCHR2),0,HDMI_RX_PHY);                  //B input port sel

    //R channel foreground K off set
    //hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~(R_InOff|R_Xtal_for_Koffset_enable),(R_InOff|R_Xtal_for_Koffset_enable));       //R input short GND
    hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~(HDMIPHY_REG_R_1_4_R_KOFF_STUNE|R_KOFF_Fore_mode|HDMIPHY_REG_R_1_4_R_KOFF_EN),(HDMIPHY_REG_R_1_4_R_KOFF_STUNE|R_KOFF_Fore_mode|HDMIPHY_REG_R_1_4_R_KOFF_EN),HDMI_RX_PHY);  //R foreground K Off Set Auto & enable
    //hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~HDMIPHY_REG_R_9_12_reg_R_CDR_SBIAS_MASK,(_BIT19));      //R SBIAS set

    for (i=Range;;i++)
    {
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~HDMIPHY_REG_R_1_4_R_EQPOW1,0,HDMI_RX_PHY);  //R foreground K Off Set power down
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~HDMIPHY_REG_R_1_4_R_KOFF_RANG,R_KOFF_FG_Range(i),HDMI_RX_PHY);  //R foreground K Off range
        hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~HDMIPHY_REG_R_1_4_R_EQPOW1,HDMIPHY_REG_R_1_4_R_EQPOW1,HDMI_RX_PHY);  //R foreground K Off Set power on
        TimeOut = 20;
        do
        {
            usleep_range(10000, 11000);
            if(hdmi_rx_reg_read32(HDMIPHY_REG_R_RESULT,HDMI_RX_PHY) & HDMIPHY_REG_R_RESULT_r_koffok_mask)     //wait Koffset calibrate finish
                break;
        } while (--TimeOut);
        if (!TimeOut)
            HDMI_PRINTF("R channel foreground K Off Set Time Out\n");
        if(!(hdmi_rx_reg_read32(HDMIPHY_REG_R_RESULT,HDMI_RX_PHY) & HDMIPHY_REG_R_RESULT_r_koff_bound_mask))       //check  K Off Set boundary
        {
              if (mode == HBR_enbuffer_off)
                {
                    HBR_enbuffer_off_R = HDMIPHY_REG_RGB_KOFF_SEL_get_r_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
                    HBR_enbuffer_off_R_range = i ;
                }
            else if (mode == LBR_enbuffer_off)
            {
                LBR_enbuffer_off_R = HDMIPHY_REG_RGB_KOFF_SEL_get_r_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
                LBR_enbuffer_off_R_range =i;
            }
            else if (mode == LBR_enbuffer_on)
            {
                LBR_enbuffer_on_R = HDMIPHY_REG_RGB_KOFF_SEL_get_r_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
                LBR_enbuffer_on_R_range = i;
            }
            else if (mode == MHL_3G_enbuffer_on)
            {
                MHL_3G_enbuffer_on_R = HDMIPHY_REG_RGB_KOFF_SEL_get_r_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
                MHL_3G_enbuffer_on_R_range = i;
            }
            else if (mode == HBR_enbuffer_on)
            {
            	HBR_enbuffer_on_R = HDMIPHY_REG_RGB_KOFF_SEL_get_r_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
            	HBR_enbuffer_on_R_range = i;
            }
            OFFSET_Result = HDMIPHY_REG_RGB_KOFF_SEL_get_r_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
            HDMI_PRINTF("R Koffset Range = %d, R Koffset calibrate result = 0x%x\n",i,OFFSET_Result);
            //hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~(R_KOFF_FG_Manual_mode_mask),R_KOFF_FG_Manual(OFFSET_Result));
            break;
        }
        else if (i>2)
        {
            Koffset_result_flag++;
            HDMI_PRINTF("R Koffset Range = %d, R Koffset calibrate result = 0x%x\n!!!!!ESD R Koffset result fail!!!!!\n",i,HDMIPHY_REG_RGB_KOFF_SEL_get_r_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY)));
            break;
        }
    }

    //G channel foreground K off set
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_9_12,~(HDMIPHY_REG_G_9_12_reg_G_INOFF|G_Xtal_for_Koffset_enable),(HDMIPHY_REG_G_9_12_reg_G_INOFF|G_Xtal_for_Koffset_enable),HDMI_RX_PHY);     //G input short GND
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~(HDMIPHY_REG_G_1_4_G_KOFF_AUTO|G_KOFF_Fore_mode|HDMIPHY_REG_G_1_4_G_KOFF_EN),(HDMIPHY_REG_G_1_4_G_KOFF_AUTO|G_KOFF_Fore_mode|HDMIPHY_REG_G_1_4_G_KOFF_EN),HDMI_RX_PHY);  //G foreground K Off Set Auto & enable
    //hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~HDMIPHY_REG_R_9_12_reg_R_CDR_SBIAS_MASK,(_BIT19));      //R SBIAS set

    for (i=Range;;i++)
    {
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~HDMIPHY_REG_G_1_4_G_EQPOW1,0,HDMI_RX_PHY);  //G foreground K Off Set power down
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~HDMIPHY_REG_G_1_4_G_KOFF_RANG,G_KOFF_FG_Range(i),HDMI_RX_PHY);  //G foreground K Off range
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~HDMIPHY_REG_G_1_4_G_EQPOW1,HDMIPHY_REG_G_1_4_G_EQPOW1,HDMI_RX_PHY);  //G foreground K Off Set power on
        TimeOut = 20;
        do
        {
            usleep_range(10000, 11000);
            if(hdmi_rx_reg_read32(HDMIPHY_REG_G_RESULT,HDMI_RX_PHY) & HDMIPHY_REG_G_RESULT_g_koffok_mask)      //wait Koffset calibrate finish
                break;
        } while (--TimeOut);
        if (!TimeOut)
            HDMI_PRINTF("G channel foreground K Off Set Time Out\n");
        if(!(hdmi_rx_reg_read32(HDMIPHY_REG_G_RESULT,HDMI_RX_PHY) & HDMIPHY_REG_G_RESULT_g_koff_bound_mask))       //check  K Off Set boundary
        {

              if (mode == HBR_enbuffer_off)
                {
                    HBR_enbuffer_off_G = HDMIPHY_REG_RGB_KOFF_SEL_get_g_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
                HBR_enbuffer_off_G_range = i;
                }
            else if (mode == LBR_enbuffer_off)
            {
                LBR_enbuffer_off_G = HDMIPHY_REG_RGB_KOFF_SEL_get_g_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
                LBR_enbuffer_off_G_range = i;
            }
            else if (mode == LBR_enbuffer_on)
            {
                LBR_enbuffer_on_G = HDMIPHY_REG_RGB_KOFF_SEL_get_g_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
                LBR_enbuffer_on_G_range = i;
            }
            else if (mode == MHL_3G_enbuffer_on)
            {
                MHL_3G_enbuffer_on_G = HDMIPHY_REG_RGB_KOFF_SEL_get_g_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
                MHL_3G_enbuffer_on_G_range = i;
            }
            else if (mode == HBR_enbuffer_on)
            {
            	HBR_enbuffer_on_G = HDMIPHY_REG_RGB_KOFF_SEL_get_g_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
            	HBR_enbuffer_on_G_range = i;
            }
            OFFSET_Result = HDMIPHY_REG_RGB_KOFF_SEL_get_g_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
            HDMI_PRINTF("G Koffset Range = %d, G Koffset calibrate result = 0x%x\n",i,OFFSET_Result);
//          hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~(G_KOFF_FG_Manual_mode_mask),G_KOFF_FG_Manual(OFFSET_Result));
            break;
        }
        else if (i>2)
        {
            Koffset_result_flag++;
            HDMI_PRINTF("G Koffset Range = %d, G Koffset calibrate result = 0x%x\n!!!!!ESD G Koffset result fail\n",i,HDMIPHY_REG_RGB_KOFF_SEL_get_g_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY)));
            break;
        }
    }

    //B channel foreground K off set
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_9_12,~(HDMIPHY_REG_B_9_12_reg_B_INOFF|B_Xtal_for_Koffset_enable),(HDMIPHY_REG_B_9_12_reg_B_INOFF|B_Xtal_for_Koffset_enable),HDMI_RX_PHY);     //B input short GND
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(HDMIPHY_REG_B_1_4_B_KOFF_AUTO|B_KOFF_Fore_mode|HDMIPHY_REG_B_1_4_B_KOFF_EN),(HDMIPHY_REG_B_1_4_B_KOFF_AUTO|B_KOFF_Fore_mode|HDMIPHY_REG_B_1_4_B_KOFF_EN),HDMI_RX_PHY);  //B foreground K Off Set Auto & enable
    //hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~HDMIPHY_REG_R_9_12_reg_R_CDR_SBIAS_MASK,(_BIT19));      //R SBIAS set

    for (i=Range;;i++)
    {
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~HDMIPHY_REG_B_1_4_B_EQPOW1,0,HDMI_RX_PHY);  //B foreground K Off Set power down
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~HDMIPHY_REG_B_1_4_B_KOFF_RANG,B_KOFF_FG_Range(i),HDMI_RX_PHY);  //B foreground K Off range
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~HDMIPHY_REG_B_1_4_B_EQPOW1,HDMIPHY_REG_B_1_4_B_EQPOW1,HDMI_RX_PHY);  //B foreground K Off Set power on
        TimeOut = 20;
        do
        {
            usleep_range(10000, 11000);
            if(hdmi_rx_reg_read32(HDMIPHY_REG_B_RESULT,HDMI_RX_PHY) & HDMIPHY_REG_B_RESULT_b_koffok_mask)      //wait Koffset calibrate finish
                break;
        } while (--TimeOut);
        if (!TimeOut)
            HDMI_PRINTF("B channel foreground K Off Set Time Out\n");
        if(!(hdmi_rx_reg_read32(HDMIPHY_REG_B_RESULT,HDMI_RX_PHY) & HDMIPHY_REG_B_RESULT_b_koff_bound_mask))       //check  K Off Set boundary
        {
              if (mode == HBR_enbuffer_off)
                {
                    HBR_enbuffer_off_B = HDMIPHY_REG_RGB_KOFF_SEL_get_b_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
                HBR_enbuffer_off_B_range = i;
                }
            else if (mode == LBR_enbuffer_off)
            {
                LBR_enbuffer_off_B = HDMIPHY_REG_RGB_KOFF_SEL_get_b_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
                LBR_enbuffer_off_B_range =i;
            }
            else if (mode == LBR_enbuffer_on)
            {
                LBR_enbuffer_on_B = HDMIPHY_REG_RGB_KOFF_SEL_get_b_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
                LBR_enbuffer_on_B_range = i;
            }
            else if (mode == MHL_3G_enbuffer_on)
            {
                MHL_3G_enbuffer_on_B = HDMIPHY_REG_RGB_KOFF_SEL_get_b_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
                MHL_3G_enbuffer_on_B_range = i;
            }
            else if (mode == HBR_enbuffer_on)
            {
            	HBR_enbuffer_on_B = HDMIPHY_REG_RGB_KOFF_SEL_get_b_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
            	HBR_enbuffer_on_B_range = i;
            }
            OFFSET_Result = HDMIPHY_REG_RGB_KOFF_SEL_get_b_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY));
            HDMI_PRINTF("B Koffset Range = %d, B Koffset calibrate result = 0x%x\n",i,OFFSET_Result);
//          hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(B_KOFF_FG_Manual_mode_mask),B_KOFF_FG_Manual(OFFSET_Result));
            break;
        }
        else if (i>2)
        {
            Koffset_result_flag++;
            HDMI_PRINTF("B Koffset Range = %d, B Koffset calibrate result = 0x%x\n!!!!!ESD B Koffset result fail!!!!!\n",i,HDMIPHY_REG_RGB_KOFF_SEL_get_b_koff_sel(hdmi_rx_reg_read32(HDMIPHY_REG_RGB_KOFF_SEL,HDMI_RX_PHY)));
            break;
        }
    }

    hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~R_InOff,0,HDMI_RX_PHY);       //R inoff disable
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_9_12,~HDMIPHY_REG_G_9_12_reg_G_INOFF,0,HDMI_RX_PHY);       //G inoff disable
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_9_12,~HDMIPHY_REG_B_9_12_reg_B_INOFF,0,HDMI_RX_PHY);       //B inoff disable

    hdmi_rx_reg_write32(REG_MHL_CTRL, reg7be0,HDMI_RX_PHY);
    hdmi_rx_reg_write32(HDMIPHY_REG_BANDGAP_Z0_CTRL_2, reg7b50,HDMI_RX_PHY);
    // set manual
    //7b10 20 30 note bit31 =0
    hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~(HDMIPHY_REG_R_1_4_R_KOFF_STUNE),0,HDMI_RX_PHY);
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~(HDMIPHY_REG_G_1_4_G_KOFF_AUTO),0,HDMI_RX_PHY);
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(HDMIPHY_REG_B_1_4_B_KOFF_AUTO),0,HDMI_RX_PHY);
    hdmi_rx_reg_mask32(HDMIPHY_REG_R_9_12,~(R_Xtal_for_Koffset_enable),0,HDMI_RX_PHY);
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_9_12,~(G_Xtal_for_Koffset_enable),0,HDMI_RX_PHY);
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_9_12,~(B_Xtal_for_Koffset_enable),0,HDMI_RX_PHY);

}

void K_Different_Offset_Condition(void)
{
   UINT8 mode;
    hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4, ~R_EQ_OUTPUT_SHORT_DISABLE, 0,HDMI_RX_PHY);        // EQ1_output short disable // see Garren 0411 mail
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4, ~G_EQ_OUTPUT_SHORT_DISABLE, 0,HDMI_RX_PHY);        // EQ1_output short disable // see Garren 0411 mail
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4, ~B_EQ_OUTPUT_SHORT_DISABLE, 0,HDMI_RX_PHY);        // EQ1_output short disable // see Garren 0411 mail

	hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4, ~R_EQ_back_koff_en, 0,HDMI_RX_PHY);		// see Garren 0411 mail
	hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4, ~G_EQ_back_koff_en, 0,HDMI_RX_PHY);		// see Garren 0411 mail
	hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4, ~B_EQ_back_koff_en, 0,HDMI_RX_PHY);		// see Garren 0411 mail

    for(mode =1 ;mode < HBR_enbuffer_on+1;mode++)
    {
         switch(mode)
         {
            case HBR_enbuffer_off:
                EQ_HBR_Setting_EQ_bias_1X();
                EQ3_Enable_buffer_Setting(0);
                EQ_KOFF_Cali(foreground_range,HBR_enbuffer_off);
                HDMI_PRINTF("mode = %d R  result = 0x%x, G  result = 0x%x, B  result = 0x%x\n",mode,HBR_enbuffer_off_R,HBR_enbuffer_off_G,HBR_enbuffer_off_B);
                break;
             case LBR_enbuffer_off:
                EQ_LBR_Setting();
                EQ3_Enable_buffer_Setting(0);
                EQ_KOFF_Cali(foreground_range,LBR_enbuffer_off);
                HDMI_PRINTF("mode = %d R  result = 0x%x, G  result = 0x%x, B  result = 0x%x\n",mode,LBR_enbuffer_off_R,LBR_enbuffer_off_G,LBR_enbuffer_off_B);
                break;
             case LBR_enbuffer_on:
                EQ_LBR_Setting();
                EQ3_Enable_buffer_Setting(1);
                EQ_KOFF_Cali(foreground_range,LBR_enbuffer_on);
                HDMI_PRINTF("mode = %d R  result = 0x%x, G  result = 0x%x, B  result = 0x%x\n",mode,LBR_enbuffer_on_R,LBR_enbuffer_on_G,LBR_enbuffer_on_B);
                break;
             case MHL_3G_enbuffer_on:
                EQ_HBR_Setting_EQ_bias_1p33X();
                EQ3_Enable_buffer_Setting(1);
                EQ_KOFF_Cali(foreground_range,MHL_3G_enbuffer_on);
                HDMI_PRINTF("mode = %d R  result = 0x%x, G  result = 0x%x, B  result = 0x%x\n",mode,MHL_3G_enbuffer_on_R,MHL_3G_enbuffer_on_G,MHL_3G_enbuffer_on_B);
                HDMI_PRINTF(" R  result range = 0x%x, G  result range = 0x%x, B  result range  = 0x%x\n",MHL_3G_enbuffer_on_R_range,MHL_3G_enbuffer_on_G_range,MHL_3G_enbuffer_on_B_range);
                break;
            case HBR_enbuffer_on:
                EQ_HBR_Setting_EQ_bias_1X();
                EQ3_Enable_buffer_Setting(1);
                EQ_KOFF_Cali(foreground_range,HBR_enbuffer_on);
                HDMI_PRINTF("[HDMI] mode=%d, KOffset Result(R,G,B)=(0x%02x, 0x%02x, 0x%02x), ",mode,HBR_enbuffer_on_R,HBR_enbuffer_on_G,HBR_enbuffer_on_B);
                HDMI_PRINTF("Range(R,G,B)=(%d,%d,%d)\n", HBR_enbuffer_on_R_range, HBR_enbuffer_on_G_range, HBR_enbuffer_on_B_range);
                break;
         }

//         HDMI_PRINTF("+++++++++++++++++++++++++++++++++++++\n");
    }
    hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4, ~R_EQ_OUTPUT_SHORT_DISABLE, R_EQ_OUTPUT_SHORT_DISABLE,HDMI_RX_PHY);		// EQ1_output short disable // see Garren 0411 mail
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4, ~G_EQ_OUTPUT_SHORT_DISABLE, G_EQ_OUTPUT_SHORT_DISABLE,HDMI_RX_PHY);		// EQ1_output short disable // see Garren 0411 mail
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4, ~B_EQ_OUTPUT_SHORT_DISABLE, B_EQ_OUTPUT_SHORT_DISABLE,HDMI_RX_PHY);		// EQ1_output short disable // see Garren 0411 mail
}

void Hdmi_PhyInit(void)
{
    hdmi_rx_reg_mask32(HDMIPHY_DPLL_CTRL,~(HDMIPHY_DPLL_CTRL_reg_dpll_pow_mask|HDMIPHY_DPLL_CTRL_pow_ldo_mask),(HDMIPHY_DPLL_CTRL_reg_dpll_pow(1)|HDMIPHY_DPLL_CTRL_pow_ldo(1)),HDMI_RX_PHY);   //DPLL 1.8V enable, ALDO 1.1V enable
    HDMI_DELAYMS(1);
    DFEinitial();
#ifdef High_Speed_Test_Dye
    hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0,~HDMIPHY_REG_BANDGAP_Z0_hspow_mask,HDMIPHY_REG_BANDGAP_Z0_hspow(1),HDMI_RX_PHY);
    HDMI_PRINTF("HDMI High Speed Test\n");
#endif
    //Power initial
    hdmi_rx_reg_mask32(HDMIPHY_REG_CK_1_4,~(HDMIPHY_REG_CK_1_4_CK_PLLPOW|HDMIPHY_REG_CK_1_4_CK_AFE_FORW_POW1),(HDMIPHY_REG_CK_1_4_CK_PLLPOW|HDMIPHY_REG_CK_1_4_CK_AFE_FORW_POW1),HDMI_RX_PHY);  //PLL Power On & HDMI PLL LDO 1.8V power on, Clock AFE power on
    hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~R_DFEPOW,R_DFEPOW,HDMI_RX_PHY);       //R DAPOW on
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~G_DFEPOW,G_DFEPOW,HDMI_RX_PHY);       //G DAPOW on
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~B_DFEPOW,B_DFEPOW,HDMI_RX_PHY);       //B DAPOW on
    hdmi_rx_reg_mask32(HDMIPHY_REG_CK_1_4,~(0x1<<14),(0),HDMI_RX_PHY);
    Z0_Calibration();
    #if 0  // 20140220 For AIC flow
    MHL_DFE_ForeGround_Set();
    #endif
//    EQ_KOFF_Cali(0);

    K_Different_Offset_Condition();
    hdmi_rx_reg_mask32(REG_CK_MD,~REG_CK_MD_ck_md_auto_mask,REG_CK_MD_ck_md_auto(1),HDMI_RX_PHY);
    hdmi_rx_reg_mask32(HDMIPHY_REG_CK_1_4,~CKAFE_hysterisis, Clock_Hysteresis,HDMI_RX_PHY);
    //set pll ldo    1.92V
    hdmi_rx_reg_mask32(HDMIPHY_REG_CK_5_8,~CK_PLL_LDO_SET_mask, CK_PLL_LDO_SET(3),HDMI_RX_PHY);

    hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~R_DFEPOW,0,HDMI_RX_PHY);       //R DAPOW off
    hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~G_DFEPOW,0,HDMI_RX_PHY);       //G DAPOW off
    hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~B_DFEPOW,0,HDMI_RX_PHY);       //B DAPOW off
}




