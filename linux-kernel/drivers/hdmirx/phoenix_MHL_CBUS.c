/*=============================================
  * Copyright (c)      Realtek Semiconductor Corporation, 2012
  * All rights reserved.
  * ============================================ */

/*================= File Description ================= */
/**
 * @file hdmi_cbus.c
 * To supply MHL 2.0 driver APIs.
 * @author cloud wu
 * @date 2012/11/16
 * @version 0.1
 * @ingroup drv_hdmi
 */

//#include "rtd_types.h"
//#include "MHL_cbus.h"
//#include "MHL_CDF.h"
#include "phoenix_hdmiInternal.h"
#include "phoenix_hdmi_wrapper.h"
#include "phoenix_MHL_CBUS.h"
#include <asm/io.h>
#if MHL_SUPPORT

MHL_PARAM_T MHL_CTRL;
// cloud add global variable for cbus use
CBUS_CHANNEL_T* cbus;
UINT8 _bMhl_clk_mode;
UINT8 _bMhl_phy_clk_mode;
UINT8 _bMhl_No_clock_cnt;
UINT8 _bMhl_Cbus_Wake_reset;

#define CBUS_REG_EDID_CTL_OFFSET          0x0c
#define CBUS_REG_DDC_SIR_OFFSET           0x20
#define CBUS_REG_DDC_SAP_OFFSET          0x24


/***************************************************************************************/
//Function :  char drvif_CBUSEDIDLoad(DDC_NUMBER_T ddc_index, unsigned char* EDID, int len)
//Description: LOAD EDID to on chip SRAM
//Parameter:  DDC 1 -  HDMI1     DDC2 - HDMI2       DDC3 - HDMI3
//return: no need return
/**************************************************************************************/
char drvif_CBUS_EDIDEnable(char enable) {

	int Edid_addr;
	Edid_addr = CBUS_DDC1_I2C_CR_REG ;

	if (enable) enable = 1;
	else enable = 0;

	//Enable/disable DDC func
	hdmi_rx_reg_mask32(Edid_addr+CBUS_REG_EDID_CTL_OFFSET, ~DDC1_EDID_CR_edid_en_mask, DDC1_EDID_CR_edid_en(enable), HDMI_RX_CBUS_DDC);

	return TRUE;

}

/***************************************************************************************/
//Function :  char drvif_CBUSEDIDLoad(DDC_NUMBER_T ddc_index, unsigned char* EDID, int len)
//Description: LOAD EDID to on chip SRAM
//Parameter:  DDC 1 -  HDMI1     DDC2 - HDMI2       DDC3 - HDMI3
//return: no need return
/**************************************************************************************/
char drvif_CBUS_EDIDLoad( unsigned char* EDID, int len) {
	int i;
	int Edid_addr;

	Edid_addr = CBUS_DDC1_I2C_CR_REG ;
	if (EDIDIsValid(EDID) == FALSE) return FALSE;

	//Disable DDC func, write EDID data to sram
	hdmi_rx_reg_mask32(Edid_addr + CBUS_REG_EDID_CTL_OFFSET, ~DDC1_EDID_CR_edid_en_mask, DDC1_EDID_CR_edid_en(0), HDMI_RX_CBUS_DDC);

	for(i=0; i< len; i++) //total 0 ~ 127
	{
			hdmi_rx_reg_write32(Edid_addr+CBUS_REG_DDC_SIR_OFFSET, i, HDMI_RX_CBUS_DDC);
			hdmi_rx_reg_write32(Edid_addr+CBUS_REG_DDC_SAP_OFFSET, EDID[i], HDMI_RX_CBUS_DDC);
	//frank@05272013 mark code to speed up
			//HDMI_PRINTF("drvif_CBUS_EDIDLoad index = %d , value = %d  \n", i,EDID[i]);
	}
	hdmi_rx_reg_write32(Edid_addr, 0x30, HDMI_RX_CBUS_DDC);


	//Enable DDC func
	hdmi_rx_reg_mask32(Edid_addr + CBUS_REG_EDID_CTL_OFFSET, ~DDC1_EDID_CR_edid_en_mask, DDC1_EDID_CR_edid_en(1), HDMI_RX_CBUS_DDC);

	return TRUE;

}

void drvif_CBUS_EDID_DDC12_AUTO_Enable(char enable) {

	int Edid_addr;
	Edid_addr = CBUS_DDC1_I2C_CR_REG ;

	if (enable) enable = 0;
	else enable = 1;
	//Enable DDC func
	hdmi_rx_reg_mask32(Edid_addr +CBUS_REG_EDID_CTL_OFFSET, ~(DDC1_EDID_CR_ddc1_force_mask | DDC1_EDID_CR_ddc2b_force_mask),
				(DDC1_EDID_CR_ddc1_force(0) | DDC1_EDID_CR_ddc2b_force(enable)), HDMI_RX_CBUS_DDC);

}

void drvif_CBUS_LoadEDID( unsigned char *EDID, int len) {


	//switch (cbus->edid_type)
	{
		//	case HDMI_EDID_ONCHIP:
				drvif_CBUS_EDIDLoad( EDID, len);
				drvif_CBUS_EDIDEnable(1);
				drvif_CBUS_EDID_DDC12_AUTO_Enable( 0);
		//	break;
		//	case HDMI_EDID_I2CMUX:
				/*
				HdmiMuxEDIDLoad(hdmi.channel[channel_index]->ddc_selected, EDID, len);
				HdmiMuxEDIDEnable(hdmi.channel[channel_index]->mux_ddc_port, 1);
				*/
		//	break;
		//	case HDMI_EDID_EEPROM:
		//	break;

	}


}

UINT8 bGet_MHL_CLK_TYPE(void)
{
    unsigned int reg_value = hdmi_rx_reg_read32(MSC_REG_31_reg, HDMI_RX_MHL_MSC_REG);
    
    return (reg_value & 0x07);
}

int phoenix_detect_MHL_in(void) // replace "DETECT_MHL2_IN"
{
    unsigned int gpio_reg_value;
                        
    gpio_reg_value = readl((unsigned int *)hdmi.gpio_direction_register);
    writel((gpio_reg_value | 0xffffff7f), (unsigned int *)hdmi.gpio_direction_register); //Set GPIO[7] input
    
    gpio_reg_value = readl((unsigned int *)hdmi.gpio_in_data_register);//GPIO input data reg.
    if(gpio_reg_value & 0x00000080)
        return 1; // MHL mode
    else
        return 0; // HDMI mode    
}


//--------------------------------------------------
// Description  : Power Process for CBUS Phy
// Input Value  : bEn --> _ON/_OFF
// Output Value : None
//--------------------------------------------------
void CBUSSwitch( UINT8 benable)
{
    //  HDMI_PRINTF("[CBUS_LOG]CBUSSwitch = %x \n",benable);
    if(benable == TRUE)    // MHL plug in 
    {
        // Discovery Function Power On and Enable System Clk
        
        // Enable Discovery IRQ
        //ScalerSetBit(P28_A0_CBUS0_CTRL_00, ~(_BIT6 | _BIT5 | _BIT0), (_BIT5 | _BIT0));
        hdmi_rx_reg_mask32(CBUS_STANDBY_00_reg, ~(CBUS_STANDBY_00_dis_irq_en_mask|CBUS_STANDBY_00_wake_irq_en_mask),0, HDMI_RX_MHL_CBUS_STANDBY);
        // MHL Attached
        //ScalerSetBit(P28_A8_CBUS0_CTRL_08, ~_BIT5, _BIT5);
        hdmi_rx_reg_mask32(CBUS_STANDBY_08_reg, ~(CBUS_STANDBY_08_cable_det_mask),(CBUS_STANDBY_08_cable_det_mask) , HDMI_RX_MHL_CBUS_STANDBY);
        phoenix_change_add_h(PHOENIX_RX_TYPE_SELECT_HDMI_IN);
        hdmi_rx_reg_mask32(HDMI_HDMI_VCR_VADDR, ~(HDMI_VCR_cbus_ddc_chsel_mask),(HDMI_VCR_cbus_ddc_chsel_mask) , HDMI_RX_MAC);//cbus ddc select  
        phoenix_change_add_h(PHOENIX_RX_TYPE_SELECT_MHL_IN);
        // SET GPIO AS CBUS 
        hdmi_rx_reg_mask32(CBUS_PHY_1_reg, ~(CBUS_PHY_1_sel_cbusb_gpio_mask),0 , HDMI_RX_MHL_CBUS_STANDBY); 
    }
    else                            // MHL plug out
    {
        // set GPIO be HPD
        hdmi_rx_reg_mask32(CBUS_PHY_1_reg, ~(CBUS_PHY_1_sel_cbusb_gpio_mask),(CBUS_PHY_1_sel_cbusb_gpio_mask), HDMI_RX_MHL_CBUS_STANDBY);
        
        // Discovery Function Power Down and System Clk Gated
        // Disable Discovery IRQ
        hdmi_rx_reg_mask32(CBUS_STANDBY_00_reg, ~(CBUS_STANDBY_00_dis_irq_en_mask|CBUS_STANDBY_00_wake_irq_en_mask),(0) , HDMI_RX_MHL_CBUS_STANDBY);
        
        // MHL Unattached
        hdmi_rx_reg_mask32(CBUS_STANDBY_08_reg, ~(CBUS_STANDBY_08_cable_det_mask),(0) , HDMI_RX_MHL_CBUS_STANDBY);
        phoenix_change_add_h(PHOENIX_RX_TYPE_SELECT_HDMI_IN);
        hdmi_rx_reg_mask32(HDMI_HDMI_VCR_VADDR, ~(HDMI_VCR_cbus_ddc_chsel_mask),(0) , HDMI_RX_MAC);  //cbus ddc select  
        phoenix_change_add_h(PHOENIX_RX_TYPE_SELECT_MHL_IN);
    
    }
    
}
//--------------------------------------------------
// Description  : Power Process for CBUS Phy
// Input Value  : bEn --> _ON/_OFF
// Output Value : None
//--------------------------------------------------
void MHLSwitch( UINT8 bchannel, UINT8 benable)
{

        if(benable == TRUE)    // MHL plug in   MHL mode
        {
             if(bGet_MHL_CLK_TYPE() == MHL_CLK_TYPE_packedpixel_mode)  _bMhl_clk_mode = 1;
        else                                                                                     _bMhl_clk_mode = 0;    
            
            //MHL for MHL Max Swing
          hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~HDMIPHY_REG_B_5_8_reg_b_Phase_Rotator_Cap,HDMIPHY_REG_B_5_8_reg_b_Phase_Rotator_Cap, HDMI_RX_PHY);    //Pierce20130219    
          //MHL
        hdmi_rx_reg_mask32(REG_MHL_CTRL,~(REG_MHL_CTRL_reg_z300_sel_2_mask|REG_MHL_CTRL_reg_z300_sel_1_mask|REG_MHL_CTRL_reg_z300_sel_0_mask|REG_MHL_CTRL_reg_z100k_enb_mask),0, HDMI_RX_PHY);    //MHL mode
        hdmi_rx_reg_mask32(REG_MHL_CTRL,~(REG_MHL_CTRL_reg_mhlpow_mask),REG_MHL_CTRL_reg_mhlpow(1), HDMI_RX_PHY); //MHL mode
        hdmi_rx_reg_mask32(REG_MHL_CTRL,~(REG_MHL_CTRL_reg_mhl_hdmisel_mask),0, HDMI_RX_PHY); //MHL mode
        if(_bMhl_clk_mode)  // packedpixel mode 
        {
            hdmi_rx_reg_mask32(REG_CK_MD,~REG_CK_MD_ck_mod_sel_mask,REG_CK_MD_ck_mod_sel(2), HDMI_RX_PHY);    
            hdmi_rx_reg_mask32(MHL_DEMUX_CTRL_reg, ~(MHL_DEMUX_CTRL_mhl_en_mask|MHL_DEMUX_CTRL_mhl_pp_en_mask),MHL_DEMUX_CTRL_mhl_en_mask|MHL_DEMUX_CTRL_mhl_pp_en_mask, HDMI_RX_MHL);
            phoenix_change_add_h(PHOENIX_RX_TYPE_SELECT_HDMI_IN);
            hdmi_rx_reg_mask32(MHL_HDMI_MAC_CTRL,~(MHL_HDMI_MAC_CTRL_ch_dec_pp_mode_en_mask|MHL_HDMI_MAC_CTRL_pp_mode_output_mask|MHL_HDMI_MAC_CTRL_packet_mhl_en_mask),(MHL_HDMI_MAC_CTRL_ch_dec_pp_mode_en_mask|MHL_HDMI_MAC_CTRL_pp_mode_output_mask|MHL_HDMI_MAC_CTRL_packet_mhl_en_mask), HDMI_RX_MAC);
            hdmi_rx_reg_mask32(MHL_HDMI_MAC_CTRL,~(MHL_HDMI_MAC_CTRL_xor_pixel_sel_mask),MHL_HDMI_MAC_CTRL_xor_pixel_sel(3), HDMI_RX_MAC);
            phoenix_change_add_h(PHOENIX_RX_TYPE_SELECT_MHL_IN);
        }
        else                         //24bit mode mode 
        {
            hdmi_rx_reg_mask32(REG_CK_MD,~REG_CK_MD_ck_mod_sel_mask,REG_CK_MD_ck_mod_sel(1), HDMI_RX_PHY);     //24bit normal mode
              //24 bit mode & MHL mode  MAC switch 
                hdmi_rx_reg_mask32(MHL_DEMUX_CTRL_reg, ~(MHL_DEMUX_CTRL_mhl_en_mask|MHL_DEMUX_CTRL_mhl_pp_en_mask),MHL_DEMUX_CTRL_mhl_en_mask, HDMI_RX_MHL);
                phoenix_change_add_h(PHOENIX_RX_TYPE_SELECT_HDMI_IN);
              hdmi_rx_reg_mask32(MHL_HDMI_MAC_CTRL,~(MHL_HDMI_MAC_CTRL_ch_dec_pp_mode_en_mask|MHL_HDMI_MAC_CTRL_pp_mode_output_mask|MHL_HDMI_MAC_CTRL_packet_mhl_en_mask),0, HDMI_RX_MAC);
              hdmi_rx_reg_mask32(MHL_HDMI_MAC_CTRL,~(MHL_HDMI_MAC_CTRL_xor_pixel_sel_mask),MHL_HDMI_MAC_CTRL_xor_pixel_sel(0), HDMI_RX_MAC);
              phoenix_change_add_h(PHOENIX_RX_TYPE_SELECT_MHL_IN);

        }
        hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~HDMIPHY_REG_PHY_CTL_CK_MHL_BiasMask,HDMIPHY_REG_PHY_CTL_CK_MHL_Bias(2), HDMI_RX_PHY);         //MHL current 1X
             //MHL clock select
          hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~HDMIPHY_REG_B_1_4_B_MHL_ClockSelMask,HDMIPHY_REG_B_1_4_B_MHL_ClockSel(bchannel), HDMI_RX_PHY);     //HW lock channel 2
        
           
        }
        else                            // MHL plug out  HDMI MODE 
        {
           //MHL for MHL Max Swing
          hdmi_rx_reg_mask32(HDMIPHY_REG_B_5_8,~HDMIPHY_REG_B_5_8_reg_b_Phase_Rotator_Cap,0, HDMI_RX_PHY);    //Pierce20130219    
          //MHL
        hdmi_rx_reg_mask32(REG_MHL_CTRL,~(REG_MHL_CTRL_reg_z300_sel_2_mask|REG_MHL_CTRL_reg_z300_sel_1_mask|REG_MHL_CTRL_reg_z300_sel_0_mask|REG_MHL_CTRL_reg_z100k_enb_mask),(REG_MHL_CTRL_reg_z300_sel_2_mask|REG_MHL_CTRL_reg_z300_sel_1_mask|REG_MHL_CTRL_reg_z300_sel_0_mask|REG_MHL_CTRL_reg_z100k_enb_mask), HDMI_RX_PHY); //HDMI mode
        hdmi_rx_reg_mask32(REG_MHL_CTRL,~(REG_MHL_CTRL_reg_mhlpow_mask),REG_MHL_CTRL_reg_mhlpow(0), HDMI_RX_PHY); //HDMI mode
        hdmi_rx_reg_mask32(REG_MHL_CTRL,~(REG_MHL_CTRL_reg_mhl_hdmisel_mask),REG_MHL_CTRL_reg_mhl_hdmisel_mask, HDMI_RX_PHY); //HDMI mode
        hdmi_rx_reg_mask32(REG_CK_MD,~REG_CK_MD_ck_mod_sel_mask,REG_CK_MD_ck_mod_sel(0), HDMI_RX_PHY);        //HDMI mode 
        hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~HDMIPHY_REG_PHY_CTL_CK_MHL_BiasMask,HDMIPHY_REG_PHY_CTL_CK_MHL_Bias(0), HDMI_RX_PHY);         //MHL current 1X
             //MHL clock select
          hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~HDMIPHY_REG_B_1_4_B_MHL_ClockSelMask,0, HDMI_RX_PHY);  //HW lock channel 2
          //24 bit mode & MHL mode  MAC switch 
          hdmi_rx_reg_mask32(MHL_DEMUX_CTRL_reg, ~(MHL_DEMUX_CTRL_mhl_en_mask|MHL_DEMUX_CTRL_mhl_pp_en_mask),0, HDMI_RX_MHL);  
             // disable 100k pull to vdd 
         //HW lock channel 2
        phoenix_change_add_h(PHOENIX_RX_TYPE_SELECT_HDMI_IN);         
         hdmi_rx_reg_mask32(MHL_HDMI_MAC_CTRL,~MHL_HDMI_MAC_CTRL_ch_dec_pp_mode_en_mask,0, HDMI_RX_MAC); // MHL pp mode clear
         phoenix_change_add_h(PHOENIX_RX_TYPE_SELECT_MHL_IN);
         hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(_BIT30|_BIT29|_BIT28),(_BIT30|_BIT29|_BIT28), HDMI_RX_PHY);    //disable  100k res 

        }
    
}

/***************************************************************************************/
//Function :  MHL_LinkerSetting(UINT8 bkeycode)
//Description: MHL_MSG_UCPE  
//                  UTF-8 Character Protocol (UCP)
//return: SEND MSG RCP key To follower
/**************************************************************************************/
void CBUS_LinkerSetting(int fSystemClk)
{
    int  One_clk_period ,fTemp1,fTemp2 ;
    UINT8 bTemp1,bTemp2;

       // Upper Bound of SYNC Pulse Low Time Period  & Lower Bound 
        //ScalerSetByte(P28_AF_CBUS0_CTRL_0F, 0xEA);
        //upper bound of sync pulse 
        One_clk_period = 10000/(fSystemClk/2) ; //unit  n -sec
        fTemp1 = 16*T_BIT_CBUS_MAX/10/One_clk_period;
     bTemp1 = (UINT8)fTemp1 ;  //0xc2 if clock is 202Mhz
//        fTemp2 = 1.4*T_BIT_CBUS_MIN/One_clk_period;
        fTemp2 = 14*T_BIT_CBUS_MIN*9/100/One_clk_period;
     bTemp2 = (UINT8)fTemp2 ;  //0x71 if clock is 202Mhz
     HDMI_PRINTF("[CBUS_LOG]CBUS_LinkerSetting Sync pulse low Time Upper bound  = %x (0xc2)Lower bound = %x (0x71) \n",bTemp1 ,bTemp2);
        hdmi_rx_reg_mask32(CBUS_LINK_00_reg, ~(CBUS_LINK_00_sync0_hb_8_0_mask|CBUS_LINK_00_sync0_lb_8_0_mask), (CBUS_LINK_00_sync0_hb_8_0(bTemp1)|CBUS_LINK_00_sync0_lb_8_0(bTemp2)),HDMI_RX_MHL_CBUS_LINK);

        // Upper Bound of SYNC Pulse High Time Period & Lower Bound of SYNC Pulse High Time Period
        //ScalerSetByte(P28_B1_CBUS0_CTRL_11, 0x58);
        fTemp1 = 6*T_BIT_CBUS_MAX/10/One_clk_period;
     bTemp1 = (UINT8)fTemp1 ;  //0x48 if clock is 202Mhz
        fTemp2 = 4*T_BIT_CBUS_MIN/10/One_clk_period;
     bTemp2 = (UINT8)fTemp2 ;  //0x21 if clock is 202Mhz
      HDMI_PRINTF("[CBUS_LOG]CBUS_LinkerSetting Sync pulse High Time Upper bound  = %x (0x48) Lower bound = %x (0x21) \n",bTemp1 ,bTemp2);
        hdmi_rx_reg_mask32(CBUS_LINK_01_reg, ~(CBUS_LINK_01_sync1_hb_7_0_mask|CBUS_LINK_01_sync1_lb_7_0_mask), (CBUS_LINK_01_sync1_hb_7_0(bTemp1)|CBUS_LINK_01_sync1_lb_7_0(bTemp2)), HDMI_RX_MHL_CBUS_LINK);
        
        // Absolute Threshold Time
        //ScalerSetByte(P28_B4_CBUS0_CTRL_14, 0x5D);// not set bit 8 enable 
         fTemp1 = 76*T_BIT_CBUS_AVE/100/One_clk_period; ;
       bTemp1 = (UINT8)fTemp1 ;  //0x48 if clock is 202Mhz
      HDMI_PRINTF("[CBUS_LOG] Absolute Threshold Time = %x (0x4d) \n",bTemp1 );
         hdmi_rx_reg_mask32(CBUS_LINK_03_reg, ~(CBUS_LINK_03_abs_threshold_mask), (CBUS_LINK_03_abs_threshold(bTemp1)), HDMI_RX_MHL_CBUS_LINK);

        // Parity Bit Time
        //ScalerSetByte(P28_B5_CBUS0_CTRL_15, 0x79);
         fTemp1 = T_BIT_CBUS_AVE/One_clk_period; ;
       bTemp1 = (UINT8)fTemp1 ;  //0x65 if clock is 202Mhz    
      HDMI_PRINTF("[CBUS_LOG] Parity Bit Time = %x (0x65) \n",bTemp1 );
        hdmi_rx_reg_mask32(CBUS_LINK_04_reg, ~(CBUS_LINK_04_parity_time_mask), (CBUS_LINK_04_parity_time(bTemp1)), HDMI_RX_MHL_CBUS_LINK);

        // Parity Error Limit
        //ScalerSetBit(P28_B6_CBUS0_CTRL_16, ~(_BIT7 | _BIT4 | _BIT3 | _BIT2 | _BIT1 | _BIT0), 0x0F);
        
     hdmi_rx_reg_mask32(CBUS_LINK_05_reg, ~(CBUS_LINK_05_parity_limit_mask|CBUS_LINK_05_parity_fail_mask), (CBUS_LINK_05_parity_limit(0x0f)|CBUS_LINK_05_parity_fail_mask), HDMI_RX_MHL_CBUS_LINK); 

        // Ack Bit Initial Falling Edge
        //ScalerSetBit(P28_B7_CBUS0_CTRL_17, ~(_BIT6 | _BIT5 | _BIT4 | _BIT3 | _BIT2 | _BIT1 | _BIT0), 0x0F);
        fTemp1 = 128/One_clk_period; ;
      bTemp1 = (UINT8)fTemp1 ;  // if clock is 202Mhz       ????????????
      HDMI_PRINTF("[CBUS_LOG] Ack Bit Initial Falling Edge = %x (0x0d) \n",bTemp1 );
        hdmi_rx_reg_mask32(CBUS_LINK_06_reg, ~(CBUS_LINK_06_ack_fall_mask), (CBUS_LINK_06_ack_fall(bTemp1)), HDMI_RX_MHL_CBUS_LINK);    

        // Ack Bit Drive Low Time
        //ScalerSetBit(P28_B8_CBUS0_CTRL_18, ~(_BIT6 | _BIT5 | _BIT4 | _BIT3 | _BIT2 | _BIT1 | _BIT0), 0x3C);
        // 400nsec ~600nsec 
        fTemp1 = T_CBUS_ACK_0/One_clk_period; 
      bTemp1 = (UINT8)fTemp1 ;  //0x32  if clock is 202Mhz       
      HDMI_PRINTF("[CBUS_LOG] Ack Bit Drive Low Time = %x (0x32) \n",bTemp1 );
        hdmi_rx_reg_mask32(CBUS_LINK_07_reg, ~(CBUS_LINK_07_ack_0_mask), (CBUS_LINK_07_ack_0(bTemp1)), HDMI_RX_MHL_CBUS_LINK);  

        // Requester Bit Time
        //ScalerSetByte(P28_BA_CBUS0_CTRL_1A, 0x80);
         fTemp1 = T_TX_bit_time/One_clk_period; 
      bTemp1 = (UINT8)fTemp1 ;  //0x32  if clock is 202Mhz       
      HDMI_PRINTF("[CBUS_LOG] Ack Bit Drive Low Time = %x (0x65) \n",bTemp1 );
        hdmi_rx_reg_mask32(CBUS_LINK_08_reg, ~(CBUS_LINK_08_tx_bit_time_mask), (CBUS_LINK_08_tx_bit_time(bTemp1)), HDMI_RX_MHL_CBUS_LINK);  

        // Check Received Ack Bit's Falling Edge
        //ScalerSetBit(P28_BC_CBUS0_CTRL_1C, ~(_BIT6 | _BIT5 | _BIT4 | _BIT3 | _BIT2 | _BIT1 | _BIT0), 0x7F);
        // fTemp1 = T_TX_bit_time*0.7/One_clk_period; 
        //fTemp1 = T_TX_bit_time/One_clk_period; 
      //bTemp1 = (UINT8)fTemp1 ;  //0x32  if clock is 202Mhz
      //cloud change for cts test
      bTemp1 = 0x55;
      HDMI_PRINTF("[CBUS_LOG] Check Received Ack Bit's Falling Edge = %x (0x46) \n",bTemp1 );
        hdmi_rx_reg_mask32(CBUS_LINK_0A_reg, ~(CBUS_LINK_0A_tx_ack_fal_mask), (CBUS_LINK_0A_tx_ack_fal(bTemp1)), HDMI_RX_MHL_CBUS_LINK);    

        // Check Received Ack Bit's Driving Low Period Upper Bound
        //ScalerSetBit(P28_BD_CBUS0_CTRL_1D, ~(_BIT6 | _BIT5 | _BIT4 | _BIT3 | _BIT2 | _BIT1 | _BIT0), 0x4F);
        // fTemp1 = T_CBUS_ACK_0_max/One_clk_period; 
      //bTemp1 = (UINT8)fTemp1 ;  //0x32  if clock is 202Mhz     
      bTemp1 = 0x45;
      HDMI_PRINTF("[CBUS_LOG] Check Received Ack Bit's Driving Low Period Upper Bound = %x (0x3E) \n",bTemp1 );
        hdmi_rx_reg_mask32(CBUS_LINK_0B_reg, ~(CBUS_LINK_0B_tx_ack_low_hb_mask), (CBUS_LINK_0B_tx_ack_low_hb(bTemp1)), HDMI_RX_MHL_CBUS_LINK);

        // Check Received Ack Bit's Driving Low Period Lower Bound
        //ScalerSetBit(P28_BE_CBUS0_CTRL_1E, ~(_BIT6 | _BIT5 | _BIT4 | _BIT3 | _BIT2 | _BIT1 | _BIT0), 0x25);
        // fTemp1 = T_CBUS_ACK_0_min/One_clk_period; 
      //bTemp1 = (UINT8)fTemp1 ;  //0x32  if clock is 202Mhz   
      //cloud change for cts test
      bTemp1 = 0x20;
      HDMI_PRINTF("[CBUS_LOG] Check Received Ack Bit's Driving Low Period lower Bound = %x (0x27) \n",bTemp1 );
        hdmi_rx_reg_mask32(CBUS_LINK_0C_reg, ~(CBUS_LINK_0C_tx_ack_low_lb_mask), (CBUS_LINK_0C_tx_ack_low_lb(bTemp1)), HDMI_RX_MHL_CBUS_LINK);

        // Actively Driving High Time for CBUS
        //ScalerSetBit(P28_C0_CBUS0_CTRL_20, ~(_BIT5 | _BIT4 | _BIT3 | _BIT2 | _BIT1 | _BIT0), 0x19);
         fTemp1 = T_drive_H_AVE/One_clk_period; 
      bTemp1 = (UINT8)fTemp1 ;  //0x32  if clock is 202Mhz       
      HDMI_PRINTF("[CBUS_LOG] Actively Driving High Time for CBUS = %x (0x19) \n",bTemp1 );
        hdmi_rx_reg_mask32(CBUS_LINK_0E_reg, ~(CBUS_LINK_0E_drv_hi_cbus_mask), (CBUS_LINK_0E_drv_hi_cbus(bTemp1)), HDMI_RX_MHL_CBUS_LINK);

        // CBUS Requester Transmit Opportunity after Arbitration
        //ScalerSetByte(P28_C2_CBUS0_CTRL_22, 0x79);
//         fTemp1 = T_TX_bit_time/One_clk_period; 
         fTemp1 = T_TX_bit_time/One_clk_period *8/10; 
      bTemp1 = (UINT8)fTemp1 ;  //0x32  if clock is 202Mhz       
      HDMI_PRINTF("[CBUS_LOG] CBUS Requester Transmit Opportunity after Arbitration = %x (0x65) \n",bTemp1 );
         hdmi_rx_reg_mask32(CBUS_LINK_10_reg, ~(CBUS_LINK_10_req_opp_flt_mask), (CBUS_LINK_10_req_opp_flt(0x65)), HDMI_RX_MHL_CBUS_LINK);

        // CBUS Requester Continue After Ack
        //ScalerSetByte(P28_C3_CBUS0_CTRL_23, 0x3C);
          fTemp1 = T_TX_bit_time*5/10/(One_clk_period); 
        bTemp1 = (UINT8)fTemp1 ;  //0x32  if clock is 202Mhz       
        HDMI_PRINTF("[CBUS_LOG] CBUS Requester Continue After Ack = %x (0x32) \n",bTemp1 );
           hdmi_rx_reg_mask32(CBUS_LINK_11_reg, ~(CBUS_LINK_11_req_cont_mask), (CBUS_LINK_11_req_cont(bTemp1)), HDMI_RX_MHL_CBUS_LINK);

        // Link Layer Timeout
        //ScalerSetBit(P28_C5_CBUS0_CTRL_25, ~(_BIT5 | _BIT4 | _BIT3 | _BIT2 | _BIT1), (_BIT4 | _BIT3 | _BIT2));
          fTemp1 = ((T_link_timeout/One_clk_period)-15)/16; 
        bTemp1 = (UINT8)fTemp1 ;  //0x32  if clock is 202Mhz       
        HDMI_PRINTF("[CBUS_LOG] CBUS Requester Continue After Ack = %x (0x0c) \n",bTemp1 );
           hdmi_rx_reg_mask32(CBUS_LINK_13_reg, ~(CBUS_LINK_13_link_time_mask), (CBUS_LINK_13_link_time(bTemp1)), HDMI_RX_MHL_CBUS_LINK);
        /*
        // Set IIC SCL Clock Frequency
        ScalerSetByte(P28_F9_CBUS0_CTRL_59, 0x12);
    */
        
        
}
//--------------------------------------------------
// Description  : Clock Select For MHL
// Input Value  : ucClockSel --> System refernce clock select.
// Output Value : None
//--------------------------------------------------

void CDF_Cability_Setting(void)
{


        // Modify Device Capabilities according to MHL Version
        /*
    #if(_MHL_VERSION == _MHL_VERSION_1_2)
        // MHL Version
        //ScalerSetDataPortByte(P28_AC_CBUS0_CTRL_0C, _MSC_MHL_VERSION_01, 0x12);
        hdmi_rx_reg_mask32(MSC_REG_01_reg, ~MSC_REG_01_mhl_ver_mask, 0x12);
        // RAP/RCP/UCP Support      
       // ScalerSetDataPortBit(P29_AC_CBUS1_CTRL_0C, _MSC_FEATURE_FLAG_0A, ~(_BIT4 | _BIT3 | _BIT2 | _BIT1 | _BIT0), (_BIT2 | _BIT1 | _MHL_RCP_SUPPORT));
        hdmi_rx_reg_mask32(MSC_REG_0A_reg, ~(MSC_REG_0A_rcp_supp_mask|MSC_REG_0A_rap_supp_mask|MSC_REG_0A_sp_supp_mask|MSC_REG_0A_ucp_send_supp_mask|MSC_REG_0A_ucp_recv_supp_mask), (MSC_REG_0A_rcp_supp_mask|MSC_REG_0A_rap_supp_mask|MSC_REG_0A_sp_supp_mask));
    #elif(_MHL_VERSION == _MHL_VERSION_2_0)
        // RAP/RCP/UCP Support
        //ScalerSetDataPortBit(P28_AC_CBUS0_CTRL_0C, _MSC_FEATURE_FLAG_0A, ~(_BIT4 | _BIT3 | _BIT2 | _BIT1 | _BIT0), ((BYTE)(_MHL_UCP_SUPPORT << 4) | (_MHL_UCP_SUPPORT << 3) | (_BIT2 | _BIT1 | _MHL_RCP_SUPPORT)));
         hdmi_rx_reg_mask32(MSC_REG_0A_reg, ~(MSC_REG_0A_rcp_supp_mask|MSC_REG_0A_rap_supp_mask|MSC_REG_0A_sp_supp_mask|MSC_REG_0A_ucp_send_supp_mask|MSC_REG_0A_ucp_recv_supp_mask), (MSC_REG_0A_rcp_supp_mask|MSC_REG_0A_rap_supp_mask|MSC_REG_0A_sp_supp_mask));

        // Display Support
            hdmi_rx_reg_mask32(MSC_REG_08_reg, ~MSC_REG_08_ld_display_mask, MSC_REG_08_ld_display_mask);
    #endif
    */
       hdmi_rx_reg_mask32(MSC_REG_01_reg, ~MSC_REG_01_mhl_ver_mask, (CDF_CR_MHL_VER_MAJOR<<4)|CDF_CR_MHL_VER_MINOR, HDMI_RX_MHL_MSC_REG);  //MHL 2.0
       hdmi_rx_reg_mask32(MSC_REG_02_reg, ~(MSC_REG_02_plim_mask|MSC_REG_02_pow_mask|MSC_REG_02_dev_type_mask), MSC_REG_02_plim(CDF_CR_POW_PLIM)|MSC_REG_02_pow(CDF_CR_POW)|MSC_REG_02_dev_type(CDF_CR_DEV_TYPE), HDMI_RX_MHL_MSC_REG);  //MHL 2.0
        //adopter_H
      hdmi_rx_reg_write32(MSC_REG_03_reg,CDF_CR_ADOPTER_ID_H, HDMI_RX_MHL_MSC_REG);
       // adopter_L
        hdmi_rx_reg_write32(MSC_REG_04_reg,CDF_CR_ADOPTER_ID_L, HDMI_RX_MHL_MSC_REG);
       
       
       hdmi_rx_reg_mask32(MSC_REG_05_reg, ~(MSC_REG_05_vid_link_md_mask|MSC_REG_05_supp_vga_mask|MSC_REG_05_supp_islands_mask|MSC_REG_05_supp_ppixel_mask|MSC_REG_05_supp_yuv422_mask|MSC_REG_05_supp_yuv444_mask|MSC_REG_05_supp_rgb444_mask),(MSC_REG_05_supp_vga(CDF_CR_SUPP_VGA))|MSC_REG_05_supp_islands(CDF_CR_SUPP_ISLANDS)|MSC_REG_05_supp_ppixel(CDF_CR_SUPP_PPIXEL)|MSC_REG_05_supp_yuv422(CDF_CR_SUPP_YCBCR422)|MSC_REG_05_supp_yuv444(CDF_CR_SUPP_YCBCR444)|MSC_REG_05_supp_rgb444(CDF_CR_SUPP_YCBCR444), HDMI_RX_MHL_MSC_REG);// not support PP mode 

     hdmi_rx_reg_mask32(MSC_REG_06_reg, ~(MSC_REG_06_aud_8ch_mask|MSC_REG_06_aud_2ch_mask),MSC_REG_06_aud_8ch(CDF_CR_AUD_8CH)|MSC_REG_06_aud_2ch(CDF_CR_AUD_2CH), HDMI_RX_MHL_MSC_REG);// not support PP mode 

     hdmi_rx_reg_mask32(MSC_REG_07_reg, ~(MSC_REG_07_video_type_mask|MSC_REG_07_vt_game_mask|MSC_REG_07_vt_cinema_mask|MSC_REG_07_vt_photo_mask|MSC_REG_07_vt_graphics_mask),MSC_REG_07_vt_game(CDF_CR_VT_GAME)|MSC_REG_07_vt_cinema(CDF_CR_VT_CINEMA)|MSC_REG_07_vt_photo(CDF_CR_VT_PHOTO)|MSC_REG_07_vt_graphics(CDF_CR_VT_GRAPHICS), HDMI_RX_MHL_MSC_REG);

     hdmi_rx_reg_mask32(MSC_REG_08_reg, ~(MSC_REG_08_ld_gui_mask|MSC_REG_08_ld_speaker_mask|MSC_REG_08_ld_record_mask|MSC_REG_08_ld_tuner_mask|MSC_REG_08_ld_media_mask|MSC_REG_08_ld_audio_mask|MSC_REG_08_ld_video_mask|MSC_REG_08_ld_display_mask),MSC_REG_08_ld_gui(CDF_CR_LD_GUI)|MSC_REG_08_ld_speaker(CDF_CR_LD_SPEAKER)|MSC_REG_08_ld_record(CDF_CR_LD_RECORD)|MSC_REG_08_ld_tuner(CDF_CR_LD_TUNER)|MSC_REG_08_ld_media(CDF_CR_LD_MEDIA)|MSC_REG_08_ld_audio(CDF_CR_LD_AUDIO)|MSC_REG_08_ld_video(CDF_CR_LD_VIDEO)|MSC_REG_08_ld_display(CDF_CR_LD_DISPLAY), HDMI_RX_MHL_MSC_REG);//

    hdmi_rx_reg_mask32(MSC_REG_09_reg, ~MSC_REG_09_bandwid_mask,MSC_REG_09_bandwid(CDF_CR_BANDWIDTH), HDMI_RX_MHL_MSC_REG);// not support PP mode  
       // RCP  RAP support   UCP RECV support 
       hdmi_rx_reg_mask32(MSC_REG_0A_reg, ~(MSC_REG_0A_ucp_recv_supp_mask|MSC_REG_0A_ucp_send_supp_mask|MSC_REG_0A_sp_supp_mask|MSC_REG_0A_rap_supp_mask|MSC_REG_0A_rcp_supp_mask),MSC_REG_0A_ucp_recv_supp(CDF_CR_UCP_RECV_SUPPORT)|MSC_REG_0A_ucp_send_supp(CDF_CR_UCP_SEND_SUPPORT)|MSC_REG_0A_sp_supp(CDF_CR_SP_SUPPORT)|MSC_REG_0A_rap_supp(CDF_CR_RAP_SUPPORT)|MSC_REG_0A_rcp_supp(CDF_CR_RCP_SUPPORT), HDMI_RX_MHL_MSC_REG);// not support RAP   RCP  UCP send

    hdmi_rx_reg_mask32(MSC_REG_0B_reg, ~MSC_REG_0B_device_id_h_mask,CDF_CR_DEVICE_ID_H, HDMI_RX_MHL_MSC_REG);// not support PP mode

    hdmi_rx_reg_mask32(MSC_REG_0C_reg, ~MSC_REG_0C_device_id_l_mask,CDF_CR_DEVICE_ID_L, HDMI_RX_MHL_MSC_REG);// not support PP mode  
    
    hdmi_rx_reg_mask32(MSC_REG_0D_reg, ~MSC_REG_0D_scr_size_mask,MSC_REG_0D_scr_size(CDF_CR_SCRATCHPAD_SIZE), HDMI_RX_MHL_MSC_REG);// not support PP mode

    hdmi_rx_reg_mask32(MSC_REG_0E_reg, ~(MSC_REG_0E_stat_size_mask|MSC_REG_0E_int_size_mask),MSC_REG_0E_stat_size(CDF_CR_INT_SIZE)|MSC_REG_0E_int_size(CDF_CR_STAT_SIZE), HDMI_RX_MHL_MSC_REG);// not support PP mode  


}
//--------------------------------------------------
// Description  : Clock Select For MHL
// Input Value  : ucClockSel --> System refernce clock select.
// Output Value : None
//--------------------------------------------------

void CBUSLINKClk(UINT8  bmode)
{
    if(bmode == _M2PLL_CLK)     //standyby mode 
    {
        // MHL System Clk select to M2PLL
        //ScalerSetBit(P28_A0_CBUS0_CTRL_00, ~(_BIT4 | _BIT3 | _BIT2), 0x00);
      

        // Sys Clk Divider
        //ScalerSetBit(P28_A1_CBUS0_CTRL_01, ~(_BIT7 | _BIT6 | _BIT5), _BIT6); //there is no function bit in this reg

        // Set Debounce For Core Function Set to 2 cycles
       // ScalerSetBit(P28_A2_CBUS0_CTRL_02, ~(_BIT7 | _BIT6 | _BIT5), _BIT6);


     CBUS_LinkerSetting(bmode);
        
    }
    else if(bmode == _ACTIVE_CLK)   // ACTIVE mode 
    {
        // MHL System Clk select to IOSC
       // ScalerSetBit(P28_A0_CBUS0_CTRL_00, ~(_BIT4 | _BIT3 | _BIT2), _BIT4);
     

        // Sys Clk Divider
       // ScalerSetBit(P28_A1_CBUS0_CTRL_01, ~(_BIT7 | _BIT6 | _BIT5), 0x00);
       

        // Disable Debounce For Core Function
       // ScalerSetBit(P28_A2_CBUS0_CTRL_02, ~(_BIT7 | _BIT6 | _BIT5), 0x00);

     CBUS_LinkerSetting(2025);

       
    }
}
/***************************************************************************************/
//Function :  MHLTmdsInitial(void)
//Description: MHL Tmds Setting 
// 
//return: SEND MSG RCP key To follower
/**************************************************************************************/
void MHLTmdsInitial(void)
{

        // Enable MHL Mac and Revert CDR Data Polarity
       // ScalerSetBit(P26_A0_MHL_CTRL_00, ~(_BIT7 | _BIT5 | _BIT1 | _BIT0), (_BIT7 | _BIT5 | _BIT0));
       //pn no swap
       hdmi_rx_reg_mask32(MHL_DAL_STATUS_reg, ~(MHL_DAL_STATUS_pn_swap_mask),0, HDMI_RX_MHL);
    
//MHL
    hdmi_rx_reg_mask32(MHL_DAL_STATUS_reg,~MHL_DAL_STATUS_mhl_ver_1_2_mask,MHL_DAL_STATUS_mhl_ver_1_2(1), HDMI_RX_MHL);
//  hdmi_rx_reg_mask32(MHL_DEMUX_CTRL_reg,~(MHL_DEMUX_CTRL_mhl_pp_en_mask|MHL_DEMUX_CTRL_mhl_en_mask),MHL_DEMUX_CTRL_mhl_en(1));
        // Guard Band reference enable for data remapping  // not found this reg
        //ScalerSetBit(P26_B2_MHL_CTRL_12, ~_BIT7, _BIT7);

        // Enable MHL OP for Data Removing Common Mode  // need check this reg REG_MHLPOW
       // hdmi_rx_reg_mask32(REG_MHL_CTRL_reg, ~REG_MHL_CTRL_reg_mhlpow_mask, REG_MHL_CTRL_reg_mhlpow_mask);
       CDF_Cability_Setting();

    }




//--------------------------------------------------
// Description  : TMDS Set PHY Function (EXINT0 Only)
// Input Value  : Measured Clk Count for PHY Setting
// Output Value : None
//--------------------------------------------------
void MHLTmdsSetPhy(void)
{
     UINT8 bTemp;

        // Check if Packed Pixel Mode
        bTemp = bGet_MHL_CLK_TYPE();
        //ScalerSetByte_EXINT0(P28_AC_CBUS0_CTRL_0C, _MSC_LINK_MODE);
        if(bTemp == LINK_MODE_CLK_PP_MODE)
        {
            //Set CDR counter
            /*
            ScalerSetBit_EXINT0(PB_C8_FLD_CONT_0, ~(_BIT7 | _BIT6), _BIT7);
            ScalerSetByte_EXINT0(PB_C9_FLD_CONT_1, 0x3F);
            ScalerSetByte_EXINT0(PB_CA_FLD_CONT_2, 0x4F);
            */
        }
        else if(bTemp == LINK_MODE_CLK_24BIT_MODE)// Otherwise --> 24 Bit Mode
        {
            //Set CDR counter
            /*
            ScalerSetBit_EXINT0(PB_C8_FLD_CONT_0, ~(_BIT7 | _BIT6), _BIT7);
            ScalerSetByte_EXINT0(PB_C9_FLD_CONT_1, 0x3F);
            ScalerSetByte_EXINT0(PB_CA_FLD_CONT_2, 0x6A);
            */
        }
    
    
}
/***************************************************************************************/
//Function :  MHL_SBUS_TIMER_ISR(UINT8 bkeycode)
//Description: MHL cbus 10msec modify flag & event
// 
//return: SEND MSG RCP key To follower
/**************************************************************************************/
void CBUS_TIMER_Handle_ISR(void)
{
   if(GET_MHL_TIMER1_SEC() !=0 )    // set CBUS can transmit command
   {
         SET_MHL_TIMER1_SEC((GET_MHL_TIMER1_SEC()-1));      //count down
   }
   else
   {
        SET_MHL_READY_TO_TRANSMIT(1);  
      //  HDMI_PRINTF("[CBUS_LOG]Ready to send command \n");
     
   }

   if(GET_MHL_TIMER2_SEC() !=0 )
   {
       SET_MHL_TIMER2_SEC((GET_MHL_TIMER2_SEC()-1));      //count down 
    //   HDMI_PRINTF("[CBUS_LOG]SET_MHL_RECONNECT_OK Count down   = %d \n",GET_MHL_TIMER2_SEC());
   }
   else
   {
         SET_MHL_RECONNECT_OK(1); // set FSM can re connect 
        // HDMI_PRINTF("[CBUS_LOG]SET_MHL_RECONNECT_OK  = %d \n",GET_MHL_RECONNECT_OK());
   }

    if(GET_MHL_TIMER3_SEC() !=0 )
   {
         SET_MHL_TIMER3_SEC((GET_MHL_TIMER3_SEC()-1));      //count down
   }
   else
   {
        SET_MHL_READY_TO_RESEND( 1);
     //   HDMI_PRINTF("[CBUS_LOG]SET_MHL_READY_TO_RESEND\n");
     
   }
   
}
/***************************************************************************************/
//Function :   void CBUS_InitialPHYSettings(void)
//Description:  Calibration for 1K/100K and LDO Level Adjust
//                 
//return: S
/**************************************************************************************/
void CBUS_InitialPHYSettings(void)
{
    // Disable CBUS LDO and Input Comparator
    hdmi_rx_reg_mask32(CBUS_PHY_1_reg, ~(CBUS_PHY_1_en_ldo_cbus_mask|CBUS_PHY_1_en_cmp_cbus_mask), (0), HDMI_RX_MHL_CBUS_STANDBY); 
    
    // Disable Output Driver
    hdmi_rx_reg_mask32(CBUS_PHY_1_reg, ~(CBUS_PHY_1_en_driver_cbus_mask), (0), HDMI_RX_MHL_CBUS_STANDBY);
    
    // Auto Calibration for 1K and Manual Calibration for 100K For monitor
    // TV set 1 k 100k auto , see if it is ok or not ????
    //set resistance manual mode
    hdmi_rx_reg_mask32(CBUS_PHY_1_reg, ~(CBUS_PHY_1_auto_k_100k_mask|CBUS_PHY_1_auto_k_1k_mask), (0), HDMI_RX_MHL_CBUS_STANDBY);
    //hdmi_rx_reg_mask32(CBUS_PHY_1_reg, ~(CBUS_PHY_1_auto_k_100k_mask|CBUS_PHY_1_auto_k_1k_mask), (CBUS_PHY_1_auto_k_100k_mask|CBUS_PHY_1_auto_k_1k_mask));

    // Adjust CBUS Input Comparator VIH = 0.95V and VIL = 0.65V
    hdmi_rx_reg_mask32(CBUS_PHY_1_reg, ~(CBUS_PHY_1_sel_cbus0_input_high_mask|CBUS_PHY_1_sel_cbus0_input_low_mask), (CBUS_PHY_1_sel_cbus0_input_high(1)|CBUS_PHY_1_sel_cbus0_input_low(1)), HDMI_RX_MHL_CBUS_STANDBY);
  // Set CBUS Max Driving Strength    43n   cloud modify 2013 change 43=>32
    hdmi_rx_reg_mask32(CBUS_PHY_1_reg, ~(CBUS_PHY_1_sel_cbus_0_driving_mask), (CBUS_PHY_1_sel_cbus_0_driving(6)), HDMI_RX_MHL_CBUS_STANDBY);
  // for cts test 2na to rxsence
  hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(_BIT30|_BIT29|_BIT28),(_BIT30|_BIT29|_BIT28), HDMI_RX_PHY);   //disable    100k res 

   

}

/***************************************************************************************/
//Function :   void MHLInitialMACSettings(void)
//Description:  Cbus setting
//                 
//return: S
/**************************************************************************************/
void MHLInitialMACSettings(void)
{


  
    // Discovery Function Power Down and System Clk Gated - there is no reg in magellan 
   
    // Write clear wake pulse & discovery pulse 
     hdmi_rx_reg_mask32(CBUS_STANDBY_00_reg, ~(CBUS_STANDBY_00_wake_pulse_det_mask|CBUS_STANDBY_00_dis_pulse_det_mask),(CBUS_STANDBY_00_wake_pulse_det_mask|CBUS_STANDBY_00_dis_pulse_det_mask), HDMI_RX_MHL_CBUS_STANDBY);
    
    // Set MHL 1MHz Clk Divider  27M clock 

     hdmi_rx_reg_mask32(CBUS_STANDBY_01_reg, ~(CBUS_STANDBY_01_clk_1m_div_mask), CBUS_STANDBY_01_clk_1m_div(13), HDMI_RX_MHL_CBUS_STANDBY);
    // Set MHL 1KHz Clk Divider 
    //cloud fix for HTC issue
     hdmi_rx_reg_mask32(CBUS_STANDBY_01_reg, ~(CBUS_STANDBY_01_clk_1k_div_mask), CBUS_STANDBY_01_clk_1k_div(648), HDMI_RX_MHL_CBUS_STANDBY);  //519* 1.25
    //initial wake up pulse width
    //2013 0621 for HTC wake up timing  16.25msec ~33.75msec
    // HTC ONE and butterfly
     hdmi_rx_reg_mask32(CBUS_STANDBY_04_reg,~(CBUS_STANDBY_04_wake_offset_mask|CBUS_STANDBY_04_cbus_disconn_mask|CBUS_STANDBY_04_wake_cnt_mask),CBUS_STANDBY_04_wake_offset(7), HDMI_RX_MHL_CBUS_STANDBY);//20msec +-7msec
    //Set Cbus discuss  Low Time to 150us
     hdmi_rx_reg_mask32(CBUS_STANDBY_02_reg, ~(CBUS_STANDBY_02_disconn_mask), CBUS_STANDBY_02_disconn(1), HDMI_RX_MHL_CBUS_STANDBY); //cloud test for 4.3.20
      
    // Set Discovery Upper/Lower Bound  60 micro second ~ 140 micro sec
//     hdmi_rx_reg_mask32(CBUS_STANDBY_05_reg, ~(CBUS_STANDBY_05_dis_upper_mask|CBUS_STANDBY_05_dis_lower_mask), (CBUS_STANDBY_05_dis_upper(5)|CBUS_STANDBY_05_dis_lower(5)));


    // Set Wake Up Pulse Number to 0 , this is for cover range
   //  hdmi_rx_reg_mask32(CBUS_STANDBY_06_reg, ~(CBUS_STANDBY_06_wake_num_mask|CBUS_STANDBY_06_dis_num_mask), (CBUS_STANDBY_06_wake_num(0)|CBUS_STANDBY_06_dis_num(5)));
     //cloud  test

}
#define  CBUS_discovery_bug  0
//--------------------------------------------------
// Description  : detect discovery pulse & wake up pulse by interrupt
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void MHLCBUS_ISR(void)
{

 // check disconnected state 
  //  if((hdmi_rx_reg_read32(CBUS_STANDBY_04_reg)&(CBUS_STANDBY_00_dis_irq_en_mask|CBUS_STANDBY_04_cbus_disconn_mask)) == (CBUS_STANDBY_00_dis_irq_en_mask | CBUS_STANDBY_04_cbus_disconn_mask))   // discovery occurs
#if CBUS_discovery_bug
    UINT32  dwtemp[16];
  if( _bMhl_Cbus_Wake_reset == 1)
  {

        
    //  hdmiport_mask (CBUS_CLK_CTL_reg,~(CBUS_CLK_CTL_cbus_core_pwdn_mask),CBUS_CLK_CTL_cbus_core_pwdn_mask);
       // hdmiport_mask (CBUS_CLK_CTL_reg,~(CBUS_CLK_CTL_cbus_core_pwdn_mask),0);
        dwtemp[0] =  hdmi_rx_reg_read32(CBUS_STANDBY_00_reg);
      dwtemp[1] =  hdmi_rx_reg_read32(CBUS_STANDBY_01_reg);
      dwtemp[2] =  hdmi_rx_reg_read32(CBUS_STANDBY_02_reg);
      dwtemp[3] =  hdmi_rx_reg_read32(CBUS_STANDBY_04_reg);
      dwtemp[4] =  hdmi_rx_reg_read32(CBUS_STANDBY_05_reg);
      dwtemp[5] =  hdmi_rx_reg_read32(CBUS_PHY_1_reg);
        hdmiport_mask ( 0xb8060034,~_BIT4,0);
      hdmiport_mask ( 0xb8060044,~_BIT4,0);
      hdmiport_mask ( 0xb8060034,~_BIT4,_BIT4);
      hdmiport_mask ( 0xb8060044,~_BIT4,_BIT4);
      /*
      hdmi_rx_reg_mask32(CBUS_PHY_1_reg, ~(CBUS_PHY_1_sel_cbusb_gpio_mask),0 ); 
      hdmi_rx_reg_mask32(CBUS_STANDBY_08_reg,~(CBUS_STANDBY_08_cable_det_mask),CBUS_STANDBY_08_cable_det_mask);
      */
      hdmiport_out(CBUS_PHY_1_reg, dwtemp[5]);
      hdmiport_out(CBUS_STANDBY_05_reg,dwtemp[4]);
      CBUSSwitch(1);
      Cbus_Power(1);
      hdmi_rx_reg_mask32(CBUS_STANDBY_08_reg,~(CBUS_STANDBY_08_goto_sink1_pos_mask),CBUS_STANDBY_08_goto_sink1_pos_mask);
      hdmiport_out(CBUS_STANDBY_00_reg, dwtemp[0]);
      hdmiport_out(CBUS_STANDBY_01_reg,dwtemp[1]);
      hdmiport_out(CBUS_STANDBY_02_reg, dwtemp[2]);
      hdmiport_out(CBUS_STANDBY_04_reg,dwtemp[3]);
      HDMI_PRINTF("[CBUS_LOG]Reset Cbus power !!! 444444\n"); 
      _bMhl_Cbus_Wake_reset = 0 ;
      
  }
#endif 
   if((hdmi_rx_reg_read32(CBUS_STANDBY_04_reg, HDMI_RX_MHL_CBUS_STANDBY)&(CBUS_STANDBY_04_cbus_disconn_mask)) == ( CBUS_STANDBY_04_cbus_disconn_mask))   // discovery occurs
    {
       _bMhl_Cbus_Wake_reset = 0;
      // Port 2 Z0 Disable
       hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_2_mask),0, HDMI_RX_PHY); //disable 70 ohm  z _ tmds 
       hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(_BIT30|_BIT29|_BIT28),(_BIT30|_BIT29|_BIT28), HDMI_RX_PHY);  //disable   100k res 
        // Clear Stuck Low Flags and IRQ
         hdmi_rx_reg_mask32(CBUS_STANDBY_04_reg,~(CBUS_STANDBY_04_disconn_irq_en_mask|CBUS_STANDBY_04_cbus_disconn_mask),(CBUS_STANDBY_04_cbus_disconn_mask), HDMI_RX_MHL_CBUS_STANDBY);
        // MCU_IRQ_STATUS_FF00 &= ~_BIT5;

      
        // Enable Discovery Debounce
         hdmi_rx_reg_mask32(CBUS_STANDBY_02_reg,~(CBUS_STANDBY_02_dis_deb_lv_mask),CBUS_STANDBY_02_dis_deb_lv(4), HDMI_RX_MHL_CBUS_STANDBY);
        // Reset CBUS Core Function
        /*
        ScalerSetBit_EXINT0(P28_A0_CBUS0_CTRL_00, ~(_BIT7), _BIT7);
        ScalerSetBit_EXINT0(P28_A0_CBUS0_CTRL_00, ~(_BIT7), 0x00);
        */
        //hdmi_rx_reg_mask32(CBUS_STANDBY_08_reg, ~(CBUS_STANDBY_08_cable_det_mask),(0) );  //can't use this command to reset state , it make problem
        
        //Force state to initial 

     hdmi_rx_reg_mask32(CBUS_STANDBY_08_reg, ~(CBUS_STANDBY_08_goto_sink1_mask),(CBUS_STANDBY_08_goto_sink1_mask) , HDMI_RX_MHL_CBUS_STANDBY);
        SET_MHL_PROCESS_STATE( _MHL_STATE_INITIAL);
      SET_SETTINGMHL(0);
      // cloud fix code
      SET_MHL_TIMER1_SEC(0);
      //cloud test
       hdmi_rx_reg_mask32(CBUS_STANDBY_00_reg,~(CBUS_STANDBY_00_dis_pulse_det_mask|CBUS_STANDBY_00_wake_pulse_det_mask),(CBUS_STANDBY_00_dis_pulse_det_mask|CBUS_STANDBY_00_wake_pulse_det_mask), HDMI_RX_MHL_CBUS_STANDBY);
      HDMI_PRINTF("[CBUS_LOG]MHLCBUS_ISR !! DETECT disconnected occurs!! CLR timer \n");    
    }
    
    // Check Discovery Pulse
    // discovery occurs
     if((hdmi_rx_reg_read32(CBUS_STANDBY_00_reg, HDMI_RX_MHL_CBUS_STANDBY)&(CBUS_STANDBY_00_wake_pulse_det_mask)) == (  CBUS_STANDBY_00_wake_pulse_det_mask) ) // wake up  occurs
    {
        hdmi_rx_reg_mask32(CBUS_STANDBY_00_reg,~(CBUS_STANDBY_00_dis_pulse_det_mask),(CBUS_STANDBY_00_dis_pulse_det_mask), HDMI_RX_MHL_CBUS_STANDBY);

              _bMhl_Cbus_Wake_reset = 1;

        
          
        
      HDMI_PRINTF("[CBUS_LOG]MHLCBUS_ISR !! Wake up pulse  occurs!! %x  \n",hdmi_rx_reg_read32(CBUS_STANDBY_00_reg, HDMI_RX_MHL_CBUS_STANDBY));      
    }
    //
   // if((hdmi_rx_reg_read32(CBUS_STANDBY_00_reg)&(CBUS_STANDBY_00_dis_pulse_det_mask|CBUS_STANDBY_00_dis_irq_en_mask)) == (CBUS_STANDBY_00_dis_pulse_det_mask | CBUS_STANDBY_00_dis_irq_en_mask))   // discovery occurs
    if((hdmi_rx_reg_read32(CBUS_STANDBY_00_reg, HDMI_RX_MHL_CBUS_STANDBY)&(CBUS_STANDBY_00_dis_pulse_det_mask)) == (CBUS_STANDBY_00_dis_pulse_det_mask ))   // discovery occurs
    {
      // Port 2 Z0 Enable
        if((GET_HDMI_CHANNEL() ==0))
        {
            hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_0_mask),(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_0_mask), HDMI_RX_PHY); 
        }
      else if((GET_HDMI_CHANNEL() ==1))
      {
        hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_1_mask),(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_1_mask), HDMI_RX_PHY); 
      }
      else if((GET_HDMI_CHANNEL() ==2))
      {
        hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_2_mask),(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_2_mask), HDMI_RX_PHY); 
      }
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(_BIT30|_BIT29|_BIT28),(0), HDMI_RX_PHY);    //enable   100k res 
       //HDMI_PRINTF("[CBUS_LOG]MHLCBUS_ISR !! DETECT Discovery occurs!! \n");  
        // Clear Wake Up and Discovery Flags
        hdmi_rx_reg_mask32(CBUS_STANDBY_00_reg,~(CBUS_STANDBY_00_dis_pulse_det_mask|CBUS_STANDBY_00_wake_pulse_det_mask),(CBUS_STANDBY_00_dis_pulse_det_mask|CBUS_STANDBY_00_wake_pulse_det_mask), HDMI_RX_MHL_CBUS_STANDBY);
       // MCU_IRQ_STATUS_FF00 &= ~_BIT5;  //clear CPU setting?

        // Disable Discovery Debounce
        hdmi_rx_reg_mask32(CBUS_STANDBY_02_reg,~(CBUS_STANDBY_02_dis_deb_lv_mask),0, HDMI_RX_MHL_CBUS_STANDBY);
        // Enable Stuck Low IRQ and Clear Flag
        hdmi_rx_reg_mask32(CBUS_STANDBY_04_reg,~(CBUS_STANDBY_04_disconn_irq_en_mask|CBUS_STANDBY_04_cbus_disconn_mask),(CBUS_STANDBY_04_disconn_irq_en_mask|CBUS_STANDBY_04_cbus_disconn_mask), HDMI_RX_MHL_CBUS_STANDBY);
      

      SET_MHL_PROCESS_STATE(_MHL_STATE_DISCOVERY_DONE);
        SET_MHL_READY_TO_TRANSMIT(1);
       
      HDMI_PRINTF("[CBUS_LOG]MHLCBUS_ISR !! DETECT Discovery occurs!! \n");     
    }



   
/*
    if(hdmi_rx_reg_read32(CBUS_DDC_02_reg)&CBUS_DDC_02_ddc_cmd_event_mask)
    {
            hdmi_rx_reg_mask32(CBUS_DDC_02_reg,~(CBUS_DDC_02_ddc_cmd_event_mask),(CBUS_DDC_02_ddc_cmd_event_mask));
        HDMI_PRINTF("[CBUS_LOG]Recieived DDC CMD  \n");     
    }

    if(hdmi_rx_reg_read32(CBUS_DDC_02_reg)&CBUS_DDC_02_ddc_data_event_mask)
    {
            hdmi_rx_reg_mask32(CBUS_DDC_02_reg,~(CBUS_DDC_02_ddc_data_event_mask),(CBUS_DDC_02_ddc_data_event_mask));
        HDMI_PRINTF("[CBUS_LOG]Recieived DDC DATA  \n");    
    }
*/
    if(hdmi_rx_reg_read32(MHL_MD_CTRL_reg, HDMI_RX_MHL)&MHL_MD_CTRL_mhl_mode_mask)
    {
        HDMI_PRINTF("[CBUS_LOG]Packed pixel mode  \n");     
    }
}

//--------------------------------------------------
// Description  : Send MSC HPD Operation
// Input Value  : ucInputPort --> D0 or D1
//                enumMSCType --> MSC Command
//                enumMSCOffset --> MSC Device Register
//                enumMSCValue --> MSC Device Register Field
// Output Value : Success or Different Fail Situations
//--------------------------------------------------
BYTE MHLMscFIFOSendCommand( UINT8 enumMSCCommand, UINT8 enumMSCOffset, UINT8 enumMSCValue, BYTE ucDataLength, BYTE *pucData)
{
    UINT8 ucTimeOut = 100;
    UINT8 ucSendType = 0;
    UINT8 ucWaitType = 0;
    UINT32 ucI = 0;
  //  HDMI_PRINTF("[CBUS_LOG] cloud MHLMscFIFOSendCommand = %x ,  offset = %x ,  enumMSCValue = %x \n" ,enumMSCCommand,enumMSCOffset,enumMSCValue); 
    switch(enumMSCCommand)
    {
        case MHL_MSC_CMD_CODE_SET_INT:   //MHL_MSC_CMD_CODE_WRT_STAT 
        case MHL_MSC_CMD_CODE_MSC_MSG:
            
            ucSendType = MHL_TX_CASE_COM_OFF_DAT;
            ucWaitType = MHL_TX_WAIT_CASE_CMD_ONLY;
            break;

        case MHL_MSC_CMD_CODE_SET_HPD:

            
            ucSendType = MHL_TX_CASE_COMMAND;
            ucWaitType = MHL_TX_WAIT_CASE_CMD_ONLY;
            break;

            
      case MHL_MSC_CMD_CODE_GET_STATE:
      case MHL_MSC_CMD_CODE_GET_VENDOR_ID:
      case MHL_MSC_CMD_CODE_GET_MSC_ERR:
        
         ucSendType = MHL_TX_CASE_COMMAND;
            ucWaitType = MHL_TX_WAIT_CASE_DATA_ONLY;
            break;

        case MHL_MSC_CMD_CODE_ABORT:
            
            ucSendType = MHL_TX_CASE_COMMAND;
            ucWaitType = MHL_TX_WAIT_CASE_CMD_NO_WAIT;
            break;

        case MHL_MSC_CMD_CODE_WRT_BURST:

            ucSendType = MHL_TX_CASE_COM_OFF_DAT_COM;
            ucWaitType = MHL_TX_WAIT_CASE_CMD_ONLY;
            break;

        default:

            break;
    }
    

        // Check If Source Has Sent An ABORT Packet
      if(CBUS_MSC_06_get_abort_req_irq(hdmi_rx_reg_read32(CBUS_MSC_06_reg, HDMI_RX_MHL_CBUS_MSC)) == TRUE)       
        {
           //clear Abort flag 
           //hdmi_rx_reg_mask32(CBUS_MSC_06_reg,~(CBUS_MSC_06_abort_req_irq_mask),CBUS_MSC_06_abort_req_irq_mask);
           hdmi_rx_reg_write32(CBUS_MSC_06_reg,CBUS_MSC_06_abort_req_irq_mask, HDMI_RX_MHL_CBUS_MSC);
        /*  
           SET_MHL_READY_TO_TRANSMIT(0);  // can't transmit

           // delay 2.5 sec then set ready to transmit
           SET_MHL_TIMER_SEC(2.5);
         HDMI_PRINTF("[CBUS_LOG]MHLMscFIFOSendCommand  GET ABORT \n");   
         */
         
            return _MHL_ABORT_FAIL;
        }
     //  HDMI_PRINTF("[CBUS_LOG] cloud MHLMscFIFOSendCommand REQUEST !!! \n" ); 
        // Clear FIFO and Send Command Only
      FW0Requester_set_tx_case(ucSendType);
        hdmi_rx_reg_mask32(FW0_REQ_00_reg,~(FW0_REQ_00_fw0_fifo_clr_mask),FW0_REQ_00_fw0_fifo_clr_mask, HDMI_RX_MHL_CBUS_MSC_FW);
        // Set MSC Command
     FW0Requester_set_cmd1(enumMSCCommand);
        // Set MSC Offset
        FW0Requester_set_offset(enumMSCOffset);
        if(enumMSCCommand == MHL_MSC_CMD_CODE_WRT_BURST)
        {
            HDMI_PRINTF("[CBUS_LOG]MHLMscFIFOSendCommand -> MHL_MSC_CMD_CODE_WRT_BURST \n");   
            // Set MSC Last Command
            FW0Requester_set_cmd2(MHL_MSC_CMD_CODE_EOF);
            // Set MSC Data
            for(ucI = 0; ucI < ucDataLength; ucI++)
            {
                FW0Requester_set_data(pucData[ucI]);
            }
        }
        else
        {
            // Set MSC Value
          FW0Requester_set_data(enumMSCValue);
        }
        
        // MSC Packet and Wait Type
        FW0Requester_set_head(MHL_HEADER_MSC_CMD);
      FW0Requester_set_wait_case(ucWaitType);
        // Send Request
      FW0Requester_set_req_en(1);
        
        // Check if Process is finished
        while(--ucTimeOut != 0)
        {
          
           //  HDMI_PRINTF("[CBUS_LOG]HW REQ finish  = %d  , Error flag = %d  \n",FW0Requester_get_finish_flag() ,FW0Requester_get_error_flag());
            if(CBUS_MSC_06_get_abort_req_irq(hdmi_rx_reg_read32(CBUS_MSC_06_reg, HDMI_RX_MHL_CBUS_MSC)) == TRUE)  //abort occur      
            {
                //clear Abort flag 
                //hdmi_rx_reg_mask32(CBUS_MSC_06_reg,~(CBUS_MSC_06_abort_req_irq_mask),CBUS_MSC_06_abort_req_irq_mask);
            hdmi_rx_reg_write32(CBUS_MSC_06_reg,CBUS_MSC_06_abort_req_irq_mask, HDMI_RX_MHL_CBUS_MSC);
                  HDMI_PRINTF("[CBUS_LOG]SEND CMD return Abort \n");
                    return _MHL_ABORT_FAIL;
            }
            else if((FW0Requester_get_finish_flag() == TRUE) && (FW0Requester_get_error_flag() == 0x00))
            {   
                // Transmit success
                // Clear Flag
                
           FW0Requester_set_finish_flag(1);

                if(ucWaitType == MHL_TX_WAIT_CASE_CMD_NO_WAIT)
                {
                 //   HDMI_PRINTF("[CBUS_LOG]Transmit finish !!_MHL_SUCCESS!!!!!!\n");
                    return _MHL_SUCCESS;
                }
                else
                {
                    // Check ACK Packet
                   // HDMI_PRINTF("[CBUS_LOG]Transmit finish !!!!!!\n");
             if(FW0Requester_get_cmd()== MHL_MSC_CMD_CODE_ACK)
                    {
                    //    HDMI_PRINTF("[CBUS_LOG]SEND CMD return _MHL_SUCCESS \n");
                        return _MHL_SUCCESS;
                    }
                    else
                    {
                        HDMI_PRINTF("[CBUS_LOG]SEND CMD return _MHL_FAIL \n");
                        return _MHL_FAIL;
                    }
                }
            }
            else if((FW0Requester_get_error_flag() == TRUE)  && (FW0Requester_get_error_code() == _MHL_ERR_CODE_PRO_ER))
            {
                // Clear Flags
                FW0Requester_set_finish_flag(1);
           FW0Requester_set_error_flag(1);
                FW0Requester_CLR_error_code() ;
                HDMI_PRINTF("[CBUS_LOG]SEND CMD return _MHL_ABORT_REPLY \n");
                return _MHL_ABORT_REPLY;
            }
          HDMI_DELAYMS(10);
          //  ScalerTimerDelayXms(1);
        }

       // Clear Flags
      FW0Requester_set_finish_flag(1);
      FW0Requester_set_error_flag(1);
      FW0Requester_CLR_error_code() ;
    HDMI_PRINTF("[CBUS_LOG]not finish ucTimeOut = %d !! _MHL_FAIL !!!!!! \n",ucTimeOut);  
    return _MHL_FAIL;
}

//--------------------------------------------------
// Description  : Send MSC HPD Operation
// Input Value  : ucInputPort --> D0 or D1
//                enumMSCType --> MSC Command
//                enumMSCOffset --> MSC Device Register
//                enumMSCValue --> MSC Device Register Field
// Output Value : Success or Fail
//--------------------------------------------------
UINT8 MHLMscSendCommand( UINT8 enumMSCCommand, UINT8 enumMSCOffset, UINT8 enumMSCValue)
{
    UINT8 ucResultPacket = 0;
    
    if(GET_MHL_READY_TO_TRANSMIT() == 1)
    {
        ucResultPacket = MHLMscFIFOSendCommand( enumMSCCommand, enumMSCOffset, enumMSCValue, 0, _NULL_POINTER);
       //  HDMI_PRINTF("[CBUS_LOG]SEND Command  =  %x  , Result = %x  \n" , enumMSCCommand ,ucResultPacket );  
        switch(ucResultPacket)
        {
            case _MHL_SUCCESS: // Source reply ACK Packet

                SET_MHL_READY_TO_TRANSMIT(1);

                return _MHL_OK;
                
                break;
                
            case  _MHL_FAIL:
            case _MHL_ABORT_FAIL: // Source reply ABORT Packet

                SET_MHL_READY_TO_TRANSMIT(0);  // can't transmit

           // delay 2.5 sec then set ready to transmit
                SET_MHL_TIMER1_SEC(250);  // 10 msec * 250 = 2.5sec
                HDMI_PRINTF("[CBUS_LOG] SEND command fail will wait 2.5 sec can send command  ");
                return _MHL_NG;

                break;

            case _MHL_ABORT_REPLY: // Source Reply Data Packet Instead of Control Packet
                
                MHLMscFIFOSendCommand( MHL_MSC_CMD_CODE_ABORT, MHL_MSC_CMD_CODE_NULL, MHL_MSC_CMD_CODE_NULL, 0, _NULL_POINTER);

                return _MHL_NG;
                
                break;

            default: // Source Reply No Packet(Timeout) or NACK

                return _MHL_NG;
                
                break;
        }
    }
    else
    {                
        return _MHL_NG;
    }
}

//--------------------------------------------------
// Description  : Send Write Burst Operation
// Input Value  : ucInputPort --> D0 or D1
//                ucDataLength --> Data Length
//                pucData --> Data
//                ucMode --> Write Burst Mode
// Output Value : Success or Different Fail Situations
//--------------------------------------------------

UINT8 MHLMscSendWriteBurst( BYTE ucOffset, BYTE ucDataLength, BYTE *pucData, EnumMHLWriteBurstMode enumMode)
{
    BYTE ucResultPacket = 0 , bTimecnt =100;

    if(enumMode == _MHL_WRITE_BURST_WITH_REQ)
    {
 
            // Clear Grant To Write Flag
            hdmi_rx_reg_mask32(MSC_REG_20_reg,~MSC_REG_20_grt_wrt_mask,MSC_REG_20_grt_wrt_mask, HDMI_RX_MHL_MSC_REG);
            // Send Request to Write
            MHLMscSendCommand( MHL_MSC_CMD_CODE_SET_INT, _MHL_REG_RCHANGE_INT, REQ_WRT);

    //  this area is different with monitor
            // Disable Access Port Auto Increase
        //    ScalerSetBit(P28_AB_CBUS0_CTRL_0B, ~_BIT0, 0x00);
            // Polling Grant to Write
        //    ScalerSetByte(P28_AC_CBUS0_CTRL_0C, _MSC_RCHANGE_INT);
        //    ScalerTimerPollingFlagProc(100, P28_AD_CBUS0_CTRL_0D, _MSC_GRT_WRT, _TRUE);
            // Enable Access Port Auto Increase
        //    ScalerSetBit(P28_AB_CBUS0_CTRL_0B, ~_BIT0, _BIT0);        
     
        do
            {
                 HDMI_DELAYMS(10);
                 bTimecnt --;
             if(MSC_REG_20_get_grt_wrt(hdmi_rx_reg_read32(MSC_REG_20_reg, HDMI_RX_MHL_MSC_REG)))   break ;
        }
            while ((!MSC_REG_20_get_grt_wrt(hdmi_rx_reg_read32(MSC_REG_20_reg, HDMI_RX_MHL_MSC_REG)))&&(bTimecnt) );

          if(bTimecnt ==0)    HDMI_PRINTF("[CBUS_LOG]  MHLMscSendWriteBurst  wait Timeout!!! \n" );
           
    }

    if(GET_MHL_READY_TO_TRANSMIT() == TRUE)
    {
        ucResultPacket = MHLMscFIFOSendCommand(MHL_MSC_CMD_CODE_WRT_BURST, ucOffset, 0x00, ucDataLength, pucData);

        switch(ucResultPacket)
        {
            case _MHL_SUCCESS: // Source reply ACK Packet

                // Send Device Scratchpad Change
                MHLMscSendCommand( MHL_MSC_CMD_CODE_SET_INT, _MHL_REG_RCHANGE_INT, DSCR_CHG);
                
                return _MHL_OK;
                
                break;

            case _MHL_ABORT_FAIL: // Source reply ABORT Packet

                SET_MHL_READY_TO_TRANSMIT(0);

                SET_MHL_TIMER1_SEC(250); // 250 * 10 msec  =2.5 sec
                

                
                return _MHL_NG;

                break;

            case _MHL_ABORT_REPLY: // Source Reply Data Packet Instead of Control Packet

                MHLMscFIFOSendCommand( MHL_MSC_CMD_CODE_ABORT, MHL_MSC_CMD_CODE_NULL, MHL_MSC_CMD_CODE_NULL, 0, _NULL_POINTER);

                return _MHL_NG;
                
                break;

            default: // Source Reply No Packet(Timeout) or NACK

                return _MHL_NG;
                
                break;
        }
    }
    else
    {                
        return _MHL_NG;
    }
}


/***************************************************************************************/
//Function :  MHLMscCheckDeviceINT(void)
//Description: MHL cbus RX check TX request INT
// 
//return: SEND MSG RCP key To follower
/**************************************************************************************/

void  MHLMscCheckDeviceINT(void)
{
    
    if(CBUS_MSC_06_get_wr_stat_irq(hdmi_rx_reg_read32(CBUS_MSC_06_reg, HDMI_RX_MHL_CBUS_MSC)) == TRUE)   
    {
         HDMI_PRINTF("[CBUS_LOG]MHLMscCheckDeviceINT   GET WRT STAT IRQ !!! \n" );  
        // Check Request To Write
      if(MSC_REG_20_get_req_wrt(hdmi_rx_reg_read32(MSC_REG_20_reg, HDMI_RX_MHL_MSC_REG)) == TRUE)
        {
             HDMI_PRINTF("[CBUS_LOG]READ TX REQUEST WRT  !!! \n" );  
            if(MHLMscSendCommand( MHL_MSC_CMD_CODE_SET_INT, _MHL_REG_RCHANGE_INT, GRT_WRT) == _MHL_OK)
            {
                //write clr flag 
                hdmi_rx_reg_mask32(MSC_REG_20_reg,~MSC_REG_20_req_wrt_mask,MSC_REG_20_req_wrt_mask, HDMI_RX_MHL_MSC_REG);
            
            }
    
        }

        // Check 3D Request and Reply No Support For All Timings
      if(MSC_REG_20_get_cbus_3d_req(hdmi_rx_reg_read32(MSC_REG_20_reg, HDMI_RX_MHL_MSC_REG)) == TRUE)
        {
            HDMI_PRINTF("[CBUS_LOG]MHLMscCheckDeviceINT   3D request !!! \n" ); 
            // 3D_VIC Header
            MHL_CTRL.bBuf[0] = _MHL_3D_VIC_HB;   //ADOPTER ID HIGH
            MHL_CTRL.bBuf[1] = _MHL_3D_VIC_LB;   //ADOPTER ID LOW

            // Total Entries -> 0
            MHL_CTRL.bBuf[3] = 0x00;

            // Checksum -> pData[0] ^ pData[1] ^ pData[3]
            MHL_CTRL.bBuf[2] = 0x10;

            // 3D_DTD Header
            MHL_CTRL.bBuf[4] = _MHL_3D_DTD_HB;
            MHL_CTRL.bBuf[5] = _MHL_3D_DTD_LB;

            // Total Entries -> 0
            MHL_CTRL.bBuf[7] = 0x00;

            // Checksum -> pData[0] ^ pData[1] ^ pData[3]
            MHL_CTRL.bBuf[6] = 0x11;

          /*
            if((MHLMscSendWriteBurst( _MHL_REG_SCRATCH_START, 4, MHL_CTRL.bBuf, _MHL_WRITE_BURST_WITH_REQ) == _MHL_OK) &&
               (MHLMscSendWriteBurst(_MHL_REG_SCRATCH_START, 4, &MHL_CTRL.bBuf[4], _MHL_WRITE_BURST_WITH_REQ)) == _MHL_OK)
               */

            if(MHLMscSendWriteBurst( _MHL_REG_SCRATCH_START, 4, MHL_CTRL.bBuf, _MHL_WRITE_BURST_WITH_REQ) == _MHL_OK)   
            {
                //ScalerSetDataPortByte(P28_AC_CBUS0_CTRL_0C, _MSC_RCHANGE_INT, _BIT4);
              hdmi_rx_reg_mask32(MSC_REG_20_reg,~MSC_REG_20_cbus_3d_req_shift,MSC_REG_20_cbus_3d_req_shift, HDMI_RX_MHL_MSC_REG);
              HDMI_PRINTF("[CBUS_LOG] MHLMscCheckDeviceINT CMD SEND VIV ok!!!!!! \n" );     
            }
        else
        {
              HDMI_PRINTF("[CBUS_LOG] MHLMscCheckDeviceINT CMD SEND VIC fail!!!!!! \n" ); 
        }
             HDMI_DELAYMS(20);
        if ((MHLMscSendWriteBurst(_MHL_REG_SCRATCH_START, 4, &MHL_CTRL.bBuf[4], _MHL_WRITE_BURST_WITH_REQ)) == _MHL_OK)
        {
              hdmi_rx_reg_mask32(MSC_REG_20_reg,~MSC_REG_20_cbus_3d_req_shift,MSC_REG_20_cbus_3d_req_shift, HDMI_RX_MHL_MSC_REG);
              HDMI_PRINTF("[CBUS_LOG] MHLMscCheckDeviceINT CMD SEND DTD ok!!!!!! \n" ); 
        }
        else
        {
               HDMI_PRINTF("[CBUS_LOG] MHLMscCheckDeviceINT CMD SEND DTD fail!!!!!! \n" );
        }
        }

        // Clear Flag WRT_STAT
        //hdmi_rx_reg_mask32(CBUS_MSC_06_reg,~CBUS_MSC_06_wr_stat_irq_mask,CBUS_MSC_06_wr_stat_irq_mask);
      hdmi_rx_reg_write32(CBUS_MSC_06_reg,CBUS_MSC_06_wr_stat_irq_mask, HDMI_RX_MHL_CBUS_MSC);
    }

}

//--------------------------------------------------
// Description  : RAP Process
// Input Value  : None
// Output Value : True or False (False if Content Off  need return serch mode)
//--------------------------------------------------
void  MHLMscRAPHandler(void)
{
   UINT8 bRap_code;

    // Check RAP Command ,check rap_irq flag

    if(CBUS_MSC_06_get_rap_irq(hdmi_rx_reg_read32(CBUS_MSC_06_reg, HDMI_RX_MHL_CBUS_MSC)) == TRUE)
    {
        HDMI_PRINTF("[CBUS_LOG]  GET RAP IRQ!!!!! \n");
        bRap_code = CBUS_MSC_12_rap_rcv(hdmi_rx_reg_read32(CBUS_MSC_12_reg, HDMI_RX_MHL_CBUS_MSC)) ;
        if(((bRap_code) == _MSC_RAP_POLL) || ((bRap_code) == _MSC_RAP_CONTENT_ON) ||((bRap_code) == _MSC_RAP_CONTENT_OFF))
        {
            HDMI_PRINTF("[CBUS_LOG]  RAPK and No Error!!!!! \n");
            // Reply RAPK and No Error
            MHLMscSendCommand( MHL_MSC_CMD_CODE_MSC_MSG, MHL_MSG_RAPK, _MSC_NO_ERROR);
        }
        else
        {
            HDMI_PRINTF("[CBUS_LOG]  RAPK Error!!!!! \n");
            // Reply RAPK and Ineffective Code
            MHLMscSendCommand( MHL_MSC_CMD_CODE_MSC_MSG, MHL_MSG_RAPK, _MSC_INEFFECTIVE_CODE);
        }

        // Clear  RAP Flag
       // hdmi_rx_reg_mask32(CBUS_MSC_06_reg,~CBUS_MSC_06_rap_irq_mask,CBUS_MSC_06_rap_irq_mask);
       hdmi_rx_reg_write32(CBUS_MSC_06_reg,CBUS_MSC_06_rap_irq_mask, HDMI_RX_MHL_CBUS_MSC);
        if(CBUS_MSC_12_rap_rcv(hdmi_rx_reg_read32(CBUS_MSC_12_reg, HDMI_RX_MHL_CBUS_MSC)) == _MSC_RAP_CONTENT_OFF)
        {
            
           if(MHLMscSendCommand( MHL_MSC_CMD_CODE_WRT_STAT, _MHL_REG_LINK_MODE, 0x00) == _MHL_OK)
            {
                HDMI_PRINTF("[CBUS_LOG]  _MSC_RAP_CONTENT_OFF!!!!! CMD ok  \n");
            }
         else
         {
             HDMI_PRINTF("[CBUS_LOG]  _MSC_RAP_CONTENT_OFF!!!!! CMD fail  \n");
         }
          // FALSE make system reserch
        }
      else if(CBUS_MSC_12_rap_rcv(hdmi_rx_reg_read32(CBUS_MSC_12_reg, HDMI_RX_MHL_CBUS_MSC)) == _MSC_RAP_CONTENT_ON)
      {
        if(MHLMscSendCommand( MHL_MSC_CMD_CODE_WRT_STAT, _MHL_REG_LINK_MODE, LINK_MODE_PATH_EN) == _MHL_OK)
            {
                HDMI_PRINTF("[CBUS_LOG]  _MSC_RAP_CONTENT_ON!!!!! CMD ok  \n");
            }
         else
         {
             HDMI_PRINTF("[CBUS_LOG]  _MSC_RAP_CONTENT_ON!!!!! CMD fail  \n");
         }
      
      }

 
    }
    

}

//--------------------------------------------------
// Description  : Get RCP Key Code
// Input Value  : None
// Output Value : _D0_INPUT_PORT/_D1_INPUT_PORT/_MSC_NONE
//--------------------------------------------------
UINT8 MHLMscRCPGetCommand(BYTE *pucKeyCode)
{

    if(CBUS_MSC_06_get_rcp_irq(hdmi_rx_reg_read32(CBUS_MSC_06_reg, HDMI_RX_MHL_CBUS_MSC)) == TRUE)
    {
        // Clear Flag
      //hdmi_rx_reg_mask32(CBUS_MSC_06_reg,~CBUS_MSC_06_rcp_irq_mask,CBUS_MSC_06_rcp_irq_mask);
        hdmi_rx_reg_write32(CBUS_MSC_06_reg,CBUS_MSC_06_rcp_irq_mask, HDMI_RX_MHL_CBUS_MSC);
        pucKeyCode[0] =  CBUS_MSC_0F_get_rcp_rcv(hdmi_rx_reg_read32(CBUS_MSC_0F_reg, HDMI_RX_MHL_CBUS_MSC));

        return TRUE;
    }
    return FALSE;
}

//--------------------------------------------------
// Description  : Get UCP Key Code
// Input Value  : None
// Output Value : _D0_INPUT_PORT/_D1_INPUT_PORT/_MSC_NONE
//--------------------------------------------------
UINT8 MHLMscUCPGetCommand(BYTE *pucKeyCode)
{
    
    // Check UCP Command
    if((CBUS_MSC_06_get_ucp_irq(hdmi_rx_reg_read32(CBUS_MSC_06_reg, HDMI_RX_MHL_CBUS_MSC))== TRUE))
    {
        // Clear Flag
      //hdmi_rx_reg_mask32(CBUS_MSC_06_reg,~CBUS_MSC_06_ucp_irq_mask,CBUS_MSC_06_ucp_irq_mask);
        hdmi_rx_reg_write32(CBUS_MSC_06_reg,CBUS_MSC_06_ucp_irq_mask, HDMI_RX_MHL_CBUS_MSC);
        pucKeyCode[0] = CBUS_MSC_0C_get_msg_cmd(hdmi_rx_reg_read32(CBUS_MSC_20_reg, HDMI_RX_MHL_CBUS_MSC));
       // HDMI_PRINTF("[CBUS_LOG]  MHL Msc UCPGetCommand = %x  \n",pucKeyCode[0] );

        return TRUE;
    }
    return FALSE;
}
//--------------------------------------------------
// Description  : MHLMSCRE_SEND_DCAD_RDY
// Input Value  : none
// Output Value : none
//              :none
//--------------------------------------------------
void MHLMSCRE_SEND_DCAD_RDY(void)
{

        if((GET_MHL_READY_TO_TRANSMIT() == TRUE) && (GET_MHL_READY_TO_RESEND() == TRUE))
            {
                MHLMscSendCommand( MHL_MSC_CMD_CODE_WRT_STAT, _MHL_REG_CONNECTED_RDY, CONNECT_DCAP_RDY);

                SET_MHL_READY_TO_RESEND( 0);
                SET_MHL_TIMER3_SEC(100);  //10 msec * 100 = 1 sec
                    
           HDMI_PRINTF("[CBUS_LOG]  MHL MHLMSCRE_SEND_DCAD_RDY \n");
           
            }
}
//--------------------------------------------------
// Description  : UserMHLMscCHECK_MODE
// Input Value  : none
// Output Value : none
//              :none
//--------------------------------------------------

void UserMHLMscCHECK_MODE(void)
{
    UINT8 ucActivePort = 0;


    //re send ready command for test
    //MHLMSCRE_SEND_DCAD_RDY();
    // Check Source If SET_INT
    //cloud test 0403
    MHLMscCheckDeviceINT();

    // RAP Handler
    MHLMscRAPHandler(); 

        /*
        if(SysPowerGetPowerStatus() == _POWER_STATUS_NORMAL)
        {
            SysModeSetResetTarget(_MODE_ACTION_RESET_TO_SEARCH);
        }
        */



    // RCP Handler
    ucActivePort = MHLMscRCPGetCommand(MHL_CTRL.bBuf);

    if(ucActivePort == TRUE)
    {
        // Check RCP Key Code
        switch(MHL_CTRL.bBuf[0])
        {
            case _MHL_RCP_VOLUME_UP:
              /*
                if(GET_OSD_VOLUME() < 100)
                {
                    SET_OSD_VOLUME(GET_OSD_VOLUME() + 1);
                }
                
                UserAdjustAudioVolume(GET_OSD_VOLUME());
                */
                break;

            case _MHL_RCP_VOLUME_DOWN:
                /*
                if(GET_OSD_VOLUME() > 0)
                {
                    SET_OSD_VOLUME(GET_OSD_VOLUME() - 1);
                }
                
                UserAdjustAudioVolume(GET_OSD_VOLUME());
                */
                break;

            case _MHL_RCP_MUTE:
            case _MHL_RCP_MUTE_FUNCTION:
                /*
                SET_OSD_VOLUME_MUTE(!GET_OSD_VOLUME_MUTE());
                UserAdjustAudioMuteSwitch();
                */
                break;

            case _MHL_RCP_RESTORE_VOLUME_FUNCTION:
               /*
                SET_OSD_VOLUME(50);
                UserAdjustAudioVolume(GET_OSD_VOLUME());
                */
                break;

            default:

                MHLMscSendCommand( MHL_MSC_CMD_CODE_MSC_MSG, MHL_MSG_RCPE, MHL_CTRL.bBuf[0]);
                break;
        }

        MHLMscSendCommand( MHL_MSC_CMD_CODE_MSC_MSG, MHL_MSG_RCPK, MHL_CTRL.bBuf[0]);
    }

    // UCP Handler
    ucActivePort = MHLMscUCPGetCommand(MHL_CTRL.bBuf);

    if(ucActivePort == TRUE)
    {
        MHL_CTRL.bBuf[1] = 0;
        //DebugMessageOsd(pData, pData[0]);
      HDMI_PRINTF("[CBUS_LOG]  UCP Handler  Get UCP data = %x  \n" , MHL_CTRL.bBuf[0]);     
        MHLMscSendCommand( MHL_MSC_CMD_CODE_MSC_MSG, MHL_MSG_UCPK, MHL_CTRL.bBuf[0]);
    }

}

//***********************************************************************
//Function name :  drvif_MHL_DetectMode()
//PARAM : NONE
//RETURN : Success 1  or Fail  0
//descript : This function is set for cbus detect and MHL flow ok
//***********************************************************************
HDMI_BOOL drvif_MHL_DetectMode(void)
{
    //if(appcb_get_hdmi_HML_detect())  // it need define in pcb file , every project may be different
    if((GET_HDMI_CHANNEL() ==MHL_CHANNEL))
    {
        //  HDMI_PRINTF("GET_HDMI_CHANNEL() = %d!!!!\n",GET_HDMI_CHANNEL());
        if(phoenix_detect_MHL_in() )
        {
            //  HDMI_PRINTF("GET_HDMI_CHANNEL() = %d!!!!\n",GET_HDMI_CHANNEL());
            if(GET_SETTINGMHL() ==0)
            {
            // For CTS Test
              //        Cbus_Power(1);
                if(GET_HDMI_CHANNEL() ==0 )
                {
                    hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_0_mask),0, HDMI_RX_PHY);// Z tmds 70 ohm close
                }
                else if (GET_HDMI_CHANNEL() ==1 )
                {
                    hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_1_mask),0, HDMI_RX_PHY);// Z tmds 70 ohm close
                }
                else if (GET_HDMI_CHANNEL() ==2 )
                {
                    hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_2_mask),0, HDMI_RX_PHY);// Z tmds 70 ohm close
                }
                //
                hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(_BIT30|_BIT29|_BIT28),(_BIT30|_BIT29|_BIT28), HDMI_RX_PHY); //disable 100k
                       SET_SETTINGMHL(1);
                set_gpio6_output_low();//HDMI2_MHL2_SEL_MHL_OUT;
                CBUSSwitch(1);
                Cbus_Power(1);
    
            }else
                {
                    MHLMscRAPHandler();
                }
            
              //cloud test
        //  UserMHLMscCHECK_MODE();
            if (! MHLNormalPreDetect())
            {
            //       HDMI_PRINTF("MHL CABLE _MODE_NOSIGNAL !!!!\n");
                return FALSE;    // _MODE_NOSIGNAL;
            }
            else
            {
                UserMHLMscCHECK_MODE();
            //disable audio 196M
                //hdmi_rx_reg_mask32(0xb80004e4,~(_BIT2),(_BIT2)); //disable distur
                  //MAGELLAN_AUDIO_PATCH();
                SET_ISMHL(TRUE);
    
                //MHL_SCRIP576P();
                //  HDMI_PRINTF("MHL Cbus OK!!! Video Start  !!!!\n");
                //while(1);
                MHLSwitch(MHL_CHANNEL,1);
                return TRUE;    // MHL  Cbus OK
            }
    
    
        }
    
        else
        {
            // For CTS Test
            if(GET_HDMI_CHANNEL() ==0 )
            {
                hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_0_mask),HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_0_mask, HDMI_RX_PHY);// Z tmds 70 ohm close
            }
            else if (GET_HDMI_CHANNEL() ==1 )
            {
                hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_1_mask),HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_1_mask, HDMI_RX_PHY);// Z tmds 70 ohm close
            }
            else if (GET_HDMI_CHANNEL() ==2 )
            {
                hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_2_mask),HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_2_mask, HDMI_RX_PHY);// Z tmds 70 ohm close
            }
            SET_ISMHL(0);                          //
                 SET_SETTINGMHL(0);
            MHLSwitch(MHL_CHANNEL,0);
            CBUSSwitch(0);
            hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(_BIT30|_BIT29|_BIT28),(_BIT30|_BIT29|_BIT28), HDMI_RX_PHY); //disable 100k
                 SET_MHL_PROCESS_STATE( _MHL_STATE_INITIAL);
            //HDMI2_MHL2_SEL_HDMI_OUT
            set_gpio6_output_low();
            set_gpio6_output_high();
            return TRUE;
        }
    
        
        //if(!DETECT_HDMI2_IN &&(!phoenix_detect_MHL_in()))
        if(!phoenix_detect_MHL_in())
        {
            //HDMI2_MHL2_SEL_HDMI_OUT
            set_gpio6_output_low();
            set_gpio6_output_high();
            return TRUE;
            //HDMI_PRINTF("DETECT_HDMI2_IN !!!!\n");
        }
    }
    else  //other channel must be HDMI
        {
              if(((GET_HDMI_CHANNEL() !=MHL_CHANNEL)))
              {
                    SET_SETTINGMHL(0);
                MHLSwitch(MHL_CHANNEL,0);
                CBUSSwitch(0);
                hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(_BIT30|_BIT29|_BIT28),(_BIT30|_BIT29|_BIT28), HDMI_RX_PHY); //disable 100k
    
            }
             if(!phoenix_detect_MHL_in() )
             {
                //HDMI2_MHL2_SEL_HDMI_OUT
                set_gpio6_output_low();
                set_gpio6_output_high();
             }
            return TRUE;
        }
    
}

//--------------------------------------------------
// Description  :Cbus power on or poer off setting 
// Input Value  : none
// Output Value : no
//             
//--------------------------------------------------
void Cbus_Power(char enable) 
{

       if(enable)
        {
      // Enable CBUS Phy, LDO and Input Comparator  Enable Output Driver
          hdmi_rx_reg_mask32(CBUS_PHY_1_reg, ~(CBUS_PHY_1_en_cbus_mask|CBUS_PHY_1_en_cmp_cbus_mask|CBUS_PHY_1_en_driver_cbus_mask|CBUS_PHY_1_en_ldo_cbus_mask|CBUS_PHY_1_psm_cbus_mask|CBUS_PHY_1_sel_cbusb_gpio_mask),(CBUS_PHY_1_en_cbus_mask|CBUS_PHY_1_en_cmp_cbus_mask|CBUS_PHY_1_en_driver_cbus_mask|CBUS_PHY_1_en_ldo_cbus_mask) , HDMI_RX_MHL_CBUS_STANDBY);
          HDMI_PRINTF("[CBUS_LOG] Cbus_Power ON  !!!\n"); 
        }
       else
       {
             hdmi_rx_reg_mask32(CBUS_PHY_1_reg, ~(CBUS_PHY_1_en_cbus_mask|CBUS_PHY_1_en_cmp_cbus_mask|CBUS_PHY_1_en_driver_cbus_mask|CBUS_PHY_1_en_ldo_cbus_mask),0 , HDMI_RX_MHL_CBUS_STANDBY);
             //FIX HUAWEI NOT SET CBUS pulse issue
             //set input 
             hdmi_rx_reg_mask32(CBUS_PHY_0_reg,~(CBUS_PHY_0_cbus_e_mask),(0), HDMI_RX_MHL_CBUS_STANDBY);  //set input 
        hdmi_rx_reg_mask32(CBUS_PHY_1_reg,~(CBUS_PHY_1_sel_cbusb_gpio_mask),(CBUS_PHY_1_sel_cbusb_gpio_mask), HDMI_RX_MHL_CBUS_STANDBY); //disable cbus ,SET GPIO function
         // Disable CBUS Phy, LDO, Input Comparator & Disable Output Driver
           
            hdmi_rx_reg_mask32(CBUS_STANDBY_08_reg, ~(CBUS_STANDBY_08_goto_sink1_mask),(CBUS_STANDBY_08_goto_sink1_mask) , HDMI_RX_MHL_CBUS_STANDBY);
            SET_MHL_PROCESS_STATE( _MHL_STATE_INITIAL);
         SET_SETTINGMHL(0);
          HDMI_PRINTF("[CBUS_LOG] Cbus_Power OFF  !!!\n");   
       }
}
//--------------------------------------------------
// Description  : int MHL_get_b_value(void) 
// Input Value  : 
// Output Value : True : TMDS Signal Detected
//              : False : No Signal
//--------------------------------------------------

int MHL_get_b_value(void) {
UINT8  bTimeout = 5;
int wValue;
    hdmi_rx_reg_mask32(REG_CK_MD,~REG_CK_MD_ck_md_ok_mask,REG_CK_MD_ck_md_ok_mask, HDMI_RX_PHY);
    do
    {
        HDMI_DELAYMS(1);
        bTimeout --;
    }while((!REG_CK_MD_get_ck_md_ok(hdmi_rx_reg_read32(REG_CK_MD, HDMI_RX_PHY)))&&(bTimeout));

    //HDMI_PRINTF("Hdmi_get_b_value = %d \n",bTimeout); 
    if(bTimeout !=0)
    {
        wValue = REG_CK_MD_get_ck_md_count(hdmi_rx_reg_read32(REG_CK_MD, HDMI_RX_PHY));
    }
    else
    {
        wValue = 0;
    }
    return wValue ;  //cloud modify for magellan2013 0108

}


//--------------------------------------------------
//
//
//
//--------------------------------------------------
HDMI_BOOL Cbus_SEND_RCP_SUB_Command(unsigned char bKeyCode)
{
      UINT8 bCode,bTemp;
    UINT16 bTimeout = 0;
    HDMI_BOOL bStatus =_MHL_SUCCESS ;

     bCode = bKeyCode &0x7f;
     //     if ((bCode == 0x00)||(bCode == 0x01)||(bCode == 0x02)||(bCode == 0x03)||(bCode == 0x04)||(bCode == 0x0d)||(bCode == 0x2B))
    //  {
    if(MHLMscSendCommand( MHL_MSC_CMD_CODE_MSC_MSG, MHL_MSG_RCP, bKeyCode) == _MHL_OK)
    {
         HDMI_PRINTF("[CBUS_LOG]  RCP CMD %x  \n",bKeyCode );  

            while (1)
            {
            if(CBUS_MSC_06_get_rcpk_irq(hdmi_rx_reg_read32(CBUS_MSC_06_reg, HDMI_RX_MHL_CBUS_MSC)) == TRUE)
                {

                // Clear Flag
                //hdmi_rx_reg_mask32(CBUS_MSC_06_reg,~CBUS_MSC_06_rcpk_irq_mask,CBUS_MSC_06_rcpk_irq_mask);
                    hdmi_rx_reg_write32(CBUS_MSC_06_reg,CBUS_MSC_06_rcpk_irq_mask, HDMI_RX_MHL_CBUS_MSC);
                    bCode=  CBUS_MSC_0F_get_rcp_rcv(hdmi_rx_reg_read32(CBUS_MSC_10_reg, HDMI_RX_MHL_CBUS_MSC));
                HDMI_PRINTF("[CBUS_LOG]  RCPK  Key code %x  , time = %d !!!!!  \n",bCode ,bTimeout );  
                break;
            }
            else
            {    
                  HDMI_DELAYMS(10);
                  bTimeout++;
                  if(bTimeout >=10)
                    {
                          
                    HDMI_PRINTF("[CBUS_LOG]  RCPK wait   Key code TIME OUT !!! !!!!!  \n" ); 
                    bStatus = _MHL_FAIL;
                    return bStatus;
                    }
                    
            }
            }   
            
            HDMI_DELAYMS(50);
          bTemp = bKeyCode|0x80;
          if(MHLMscSendCommand( MHL_MSC_CMD_CODE_MSC_MSG, MHL_MSG_RCP, bTemp) == _MHL_OK)
          {
            while (1)
                {
                if(CBUS_MSC_06_get_rcpk_irq(hdmi_rx_reg_read32(CBUS_MSC_06_reg, HDMI_RX_MHL_CBUS_MSC)) == TRUE)
                    {

                    // Clear Flag
                    //hdmi_rx_reg_mask32(CBUS_MSC_06_reg,~CBUS_MSC_06_rcpk_irq_mask,CBUS_MSC_06_rcpk_irq_mask);
                        hdmi_rx_reg_write32(CBUS_MSC_06_reg,CBUS_MSC_06_rcpk_irq_mask, HDMI_RX_MHL_CBUS_MSC);
                        bCode=  CBUS_MSC_0F_get_rcp_rcv(hdmi_rx_reg_read32(CBUS_MSC_10_reg, HDMI_RX_MHL_CBUS_MSC));
                    HDMI_PRINTF("[CBUS_LOG]  RCPK  Key code %x  , time = %d !!!!!  \n",bCode ,bTimeout );  
                    break;
                }
                else
                {
                      HDMI_DELAYMS(10);
                        bTimeout++;
                        if(bTimeout >=10)
                        {
                          
                        HDMI_PRINTF("[CBUS_LOG]  RCPK wait   Key code TIME OUT !!! !!!!!  \n" ); 
                        bStatus = _MHL_FAIL;
                        return bStatus;
                        }
                        
                }
                }   
           return bStatus;
          }
          else
          {
                 HDMI_PRINTF("[CBUS_LOG]  RCP CMD send fail!!!!!  \n" );  
                bStatus = _MHL_FAIL;
                return bStatus;
          }
            
    }
    else
    {
         HDMI_PRINTF("[CBUS_LOG]  RCP CMD send fail!!!!!  \n" );  
         bStatus = _MHL_FAIL;
         return bStatus;
    }
  // }
  // else
  // {
  //    HDMI_PRINTF("[CBUS_LOG]  RCP CMD send OUT of RANGE  !!!!!  \n" ); 
  // }
}

//--------------------------------------------------
// Description  : Signal PreDetection for TMDS(Power Normal)
// Input Value  : Input Port(D0 or D1, HDMI or DVI)
// Output Value : True : TMDS Signal Detected
//              : False : No Signal
//--------------------------------------------------
UINT8 MHLNormalPreDetect(void)
{

    //BYTE ucResultPacket = 0;
    //BYTE ucTemp = 0;
    int wClock_value;

    if(CBUS_MSC_12_get_rap_rcv(hdmi_rx_reg_read32(CBUS_MSC_12_reg, HDMI_RX_MHL_CBUS_MSC)) == _MSC_RAP_CONTENT_OFF)
    {
            HDMI_PRINTF("[CBUS_LOG]  MHLNormalPreDetect  _MSC_RAP_CONTENT_OFF \n" );  
            return FALSE;
    }
    
    //ScalerTimerDelayXms(6);
    /*    //set phy or not 
    if(GET_TMDS_PHY_SET() == _TRUE)
    {
        DebugMessageScaler("7. wuzanne test : Normal TMDS Clock Stable", 0);
        
        return _SUCCESS;
    }
    */

    switch(GET_MHL_PROCESS_STATE())
    {
        case _MHL_STATE_INITIAL:  //wait interrupt to change state to  _MHL_STATE_DISCOVERY_DONE
                //ScalerTimerActiveTimerEvent(SEC(5), _SCALER_TIMER_EVENT_MHL_D1_RECONNECT_1K);
            //     HDMI_PRINTF("[CBUS_LOG]  _MHL_STATE_INITIAL  \n" );  
       
           if(GET_MHL_RECONNECT_OK())
           {
                //  if(((ScalerGetByte(P28_A7_CBUS0_CTRL_07) & 0x38) >> 3) == 0x02)     //????
                   
                         HDMI_PRINTF("[CBUS_LOG]  Cable in wait signal _MHL_STATE_INITIAL  \n" );  
                        SET_MHL_RECONNECT_OK(0);
                SET_MHL_TIMER2_SEC(500); // 10 msec * 500
                //HDMI_PRINTF("[CBUS_LOG] timer2 = %x  \n" , GET_MHL_TIMER2_SEC()  );  
                // MHL initial Setting 
                          CBUSSwitch(TRUE);
                
                //cloud mark for HUAWEI issue  , this action need check 2013 0607
                /*
                        if(CBUS_STANDBY_07_get_sink_fsm_st(hdmi_rx_reg_read32(CBUS_STANDBY_07_reg)) == 0x02)
                    { 
                    hdmi_rx_reg_mask32(CBUS_STANDBY_08_reg,~CBUS_STANDBY_08_cable_det_mask,0);       
                            //ScalerTimerDelayXms(50);  for samsung monitor
                            HDMI_DELAYMS(20);
                    HDMI_PRINTF("[CBUS_LOG]  FW plug in out cable  \n" );  
                                hdmi_rx_reg_mask32(CBUS_STANDBY_08_reg,~CBUS_STANDBY_08_cable_det_mask,CBUS_STANDBY_08_cable_det_mask);
                                        
                        }
*/
           }
           /*
           if(GET_MHL_TIMER2_SEC() == 0)
           {
                   SET_MHL_TIMER2_SEC(5);

           }
           */
            return FALSE;
            
            break;
            
        case _MHL_STATE_DISCOVERY_DONE:

            SET_MHL_TIMER2_SEC(0);// disable re connect timer
        //    HDMI_PRINTF("[CBUS_LOG] _MHL_STATE_DISCOVERY_DONE  \n" );  
        //ScalerTimerDelayXms(150);
        HDMI_DELAYMS(300);   // FOR cts 4.3.23.2
             hdmi_ioctl_struct.b_mhl_debounce = 0;  //for Set MHL PHY clear
            if(MHLMscSendCommand( MHL_MSC_CMD_CODE_SET_HPD, MHL_MSC_CMD_CODE_NULL, MHL_MSC_CMD_CODE_NULL) == _MHL_OK)
            {
                //DebugMessageDigital("MHL HPD Done", 0x01);
                HDMI_PRINTF("[CBUS_LOG] _MHL_SET_HPD!!!!!  \n" );  
        //   HDMI_PRINTF("[CBUS_LOG]  UCP Handler  Get UCP data = %d  \n");
                SET_MHL_PROCESS_STATE( _MHL_STATE_HPD_DONE);

            }
            else
            {
                HDMI_PRINTF("[CBUS_LOG]  _MHL_SET_HPD FAIL!!!!!  \n" ); 
           SET_MHL_PROCESS_STATE( _MHL_STATE_DISCOVERY_DONE);
                return FALSE;
            }

            break;

        case _MHL_STATE_HPD_DONE:

            HDMI_PRINTF("[CBUS_LOG] _MHL_STATE_HPD_DONE  \n" );  
            if(MHLMscSendCommand( MHL_MSC_CMD_CODE_WRT_STAT, _MHL_REG_LINK_MODE, LINK_MODE_PATH_EN) == _MHL_OK)
            {
                   HDMI_PRINTF("[CBUS_LOG]   _MHL_STATE_PATH_EN_DONE \n");
            // set z_Tmds link
            // hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL_reg,~(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_2_mask),(HDMIPHY_REG_BANDGAP_Z0_CTRL_z0pow_2_mask)); 
                   //  hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4_reg,~(_BIT30|_BIT29|_BIT28),(_BIT30|_BIT29|_BIT28));  //disable   100k res 
                    SET_MHL_PROCESS_STATE(_MHL_STATE_PATH_EN_DONE);

            }
            else
            {
                return FALSE;
            }

            break;

        case _MHL_STATE_PATH_EN_DONE:
           #if 1    
            //CBUS_CTS_CMD_TEST();           
          #endif   
            if(MHLMscSendCommand(MHL_MSC_CMD_CODE_WRT_STAT, _MHL_REG_CONNECTED_RDY, CONNECT_DCAP_RDY) == _MHL_OK)
            {
                hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(_BIT30|_BIT29|_BIT28),(_BIT30|_BIT29|_BIT28), HDMI_RX_PHY);     //enable   100k res 
           HDMI_PRINTF("[CBUS_LOG]  MHL Dev Cap RDY\n");

                SET_MHL_PROCESS_STATE(_MHL_STATE_DEV_CAP_RDY);
                SET_MHL_READY_TO_RESEND( 1);


            }
            else
            {
                return FALSE;
            }

            break;
         // this case can put in check mode 
         
        case _MHL_STATE_DEV_CAP_RDY:
 
            if((GET_MHL_READY_TO_TRANSMIT() == TRUE) && (GET_MHL_READY_TO_RESEND() == TRUE))
            {
                MHLMscSendCommand( MHL_MSC_CMD_CODE_WRT_STAT, _MHL_REG_CONNECTED_RDY, CONNECT_DCAP_RDY);

                SET_MHL_READY_TO_RESEND( 0);
                SET_MHL_TIMER3_SEC(100);
                wClock_value =   MHL_get_b_value() ;
           HDMI_PRINTF("[CBUS_LOG]  MHL CBUS Done MHL clock =%d  \n", wClock_value);
           if (wClock_value == 0)   _bMhl_No_clock_cnt ++;

           if (_bMhl_No_clock_cnt  > 5)
           {
            SET_MHL_PROCESS_STATE(_MHL_STATE_INITIAL);
            _bMhl_No_clock_cnt =0 ; 
            CBUSSwitch(0);  
            HDMI_PRINTF("[CBUS_LOG]  MHL CBUS RESET !!!!!!!!!!!!!!!!!! \n");
           }
           return TRUE;
            }

             return FALSE;

            break;
         
        default:

            break;
    }

    return FALSE;
}

#endif  //end of MHL_SUPPORT

