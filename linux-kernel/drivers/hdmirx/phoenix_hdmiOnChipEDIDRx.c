
#include "phoenix_hdmiInternal.h"
#include "phoenix_hdmiHWReg.h"

#define REG_EDID_CTL_OFFSET          0x0c
#define REG_DDC_SIR_OFFSET           0x20
#define REG_DDC_SAP_OFFSET          0x24

/***************************************************************************************/
//Function :  char drvif_EDIDLoad(DDC_NUMBER_T ddc_index, unsigned char* EDID, int len)
//Description: LOAD EDID to on chip SRAM
//Parameter:  DDC 1 -  HDMI1     DDC2 - HDMI2       DDC3 - HDMI3
//return: no need return
/**************************************************************************************/
char drvif_EDIDLoad(DDC_NUMBER_T ddc_index, unsigned char* EDID, int len) {
	int i;
	int Edid_addr;

	if(ddc_index!=0)//1195 have only one HDMI In
		return FALSE;

	Edid_addr =  DDC1_I2C_CR_REG;
	
//	if (ddc_index >= DDC_NOTUSED) return FALSE;

//	if (EDIDIsValid(EDID) == FALSE) return FALSE;

	/* Wait until RX DDC working */
	hdmi_rx_reg_write32(REG_EDID_CTL_OFFSET, 0x01, HDMI_RX_DDC);
	i=0;
	while((hdmi_rx_reg_read32(REG_EDID_CTL_OFFSET,HDMI_RX_DDC)!=0x1) && i<20)
	{
		//pr_info("[HDMI RX] Wait RX DDC %d\n",i);
		hdmi_rx_reg_write32(REG_EDID_CTL_OFFSET, 0x01, HDMI_RX_DDC);
		usleep_range(15000, 20000);// 15ms~20ms
		i++;
	}
	if(i>=20)
		pr_err("[HDMI RX] %s RX DDC no response\n",__FUNCTION__);

	//Disable DDC func, write EDID data to sram
	hdmi_rx_reg_mask32(Edid_addr + REG_EDID_CTL_OFFSET, ~DDC1_EDID_CR_edid_en_mask, DDC1_EDID_CR_edid_en(0), HDMI_RX_DDC);

	for(i=0; i< len; i++)
	{
			hdmi_rx_reg_write32(Edid_addr+REG_DDC_SIR_OFFSET, i, HDMI_RX_DDC);
			hdmi_rx_reg_write32(Edid_addr+REG_DDC_SAP_OFFSET, EDID[i], HDMI_RX_DDC);
	}
	/*Set to 3 clock debounce*/
	//hdmi_rx_reg_write32(Edid_addr, 0x30, HDMI_RX_MISC_DDC);
	/*Set to No Debounce*/
	hdmi_rx_reg_write32(Edid_addr, 0x00, HDMI_RX_DDC);


	//Enable DDC func
	hdmi_rx_reg_mask32(Edid_addr + REG_EDID_CTL_OFFSET, ~DDC1_EDID_CR_edid_en_mask, DDC1_EDID_CR_edid_en(1), HDMI_RX_DDC);

	return TRUE;

}

char drvif_EDIDEnable(DDC_NUMBER_T ddc_index, char enable) {

	int Edid_addr;

	if(ddc_index!=0)//1195 have only one HDMI In
		return FALSE;

	Edid_addr =  DDC1_I2C_CR_REG;
	//Enable/disable DDC func
	hdmi_rx_reg_mask32(Edid_addr+REG_EDID_CTL_OFFSET, ~DDC1_EDID_CR_edid_en_mask, DDC1_EDID_CR_edid_en(enable), HDMI_RX_DDC);

	return TRUE;

}

void drvif_EDID_DDC12_AUTO_Enable(DDC_NUMBER_T ddc_index,char enable) {

	int Edid_addr;

	if(ddc_index!=0)//1195 have only one HDMI In
		return;

	Edid_addr =  DDC1_I2C_CR_REG;
	hdmi_rx_reg_mask32(Edid_addr +REG_EDID_CTL_OFFSET, ~(DDC1_EDID_CR_ddc1_force_mask | DDC1_EDID_CR_ddc2b_force_mask),
				(DDC1_EDID_CR_ddc1_force(0) | DDC1_EDID_CR_ddc2b_force((enable)?0:1)), HDMI_RX_DDC);

}

