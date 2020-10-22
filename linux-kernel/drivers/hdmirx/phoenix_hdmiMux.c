#include "phoenix_hdmiFun.h"
#include "phoenix_hdmiInternal.h"
#include<linux/string.h>

static HDMI_MUX_FUNC_T mux_func;

char EDIDIsValid(unsigned char* EDID) {

	if((0x0 != EDID[0])||(0xff != EDID[1])||(0xff != EDID[6])||(0x0 != EDID[7]))
	{
		return FALSE;
	}

	return TRUE;

}

void HdmiMuxSel(HDMI_MUX_IC_T mux_ic) {

	memset(&mux_func, 0, sizeof(mux_func));
	switch (mux_ic) {
		case HDMI_MUX_NOUSED:
			pr_info("HDMI MUX not used\n");
		break;
#if 0 // not support mux
#ifdef	HDMI_SWITCH_SI9199		
		case HDMI_SWITCH_SiI9199:
			SiI9199_Init();
			mux_func.EDIDEnable = SiI9199_HdmiMuxEDIDEnable;
			mux_func.EDIDLoad = SiI9199_HdmiMuxEDIDLoad;
			mux_func.HPD = SiI9199_HPD;
			mux_func.PhyPortEnable = SiI9199_HdmiMux_PhyPortEnable;
			break;
#else
#ifndef	BOARD_ID_RTD2648_TV005_ATV_V1		
		case HDMI_MUX_PS331:
			mux_func.EDIDEnable = PS331_HdmiMuxEDIDEnable;
			mux_func.EDIDLoad = PS331_HdmiMuxEDIDLoad;
			mux_func.HPD = PS331_HPD;
			mux_func.PhyPortEnable = PS331_HdmiMux_PhyPortEnable;
		break;
#endif		
#endif
#endif		
		default: 
			pr_info("unkwon HDMI MUX\n");
		break;

	}

}


char HdmiMuxHPD(int index, char high) {

	if (index >= 3) return FALSE;
	if (mux_func.HPD) {
		mux_func.HPD(index, high);	
	}
	return TRUE;
}

char HdmiMuxEDIDLoad(int ddc_index, unsigned char* EDID, int len) {
	if (ddc_index >= DDC_NOTUSED) return FALSE;
	
	if (EDIDIsValid(EDID) == FALSE) return FALSE;
	if (mux_func.EDIDLoad) {
		mux_func.EDIDLoad(ddc_index, EDID, len);	
	}
	return TRUE;

}

void HdmiMux_PhyPortEnable(HDMI_MUX_PORT_T port, char enable) {
	if (mux_func.PhyPortEnable) {
		mux_func.PhyPortEnable(port, enable);
	}

}

char HdmiMuxEDIDEnable(int ddc_index, char enable) {

	if (ddc_index >= DDC_NOTUSED) return FALSE;
	if (mux_func.EDIDEnable) {
		mux_func.EDIDEnable(ddc_index, enable);	
	}
	return TRUE;

}

