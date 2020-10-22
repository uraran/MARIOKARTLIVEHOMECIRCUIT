//#include <stdbool.h>
#include "phoenix_hdmiInternal.h"
#include "phoenix_hdmiHWReg.h"
#include "phoenix_hdmiFun.h"
#include "phoenix_MHL_CBUS.h"
#include <linux/kernel.h>
#include <linux/types.h>

//#include "Add_in_common/hdmiSetForScaler.h"

unsigned char drvif_HDMI_AFD(void) 
{
	return get_HDMI_AFD();
}

void drvif_HDMI_SetScreenMode(HDMI_OSD_MODE OSD_mode)
 {
 	pr_info("drvif_HDMI_SetScreenMode=%d\n",OSD_mode);
	SET_HDMI_OSD_MODE(OSD_mode);
 }

 HDMI_OSD_MODE drvif_HDMI_GetScreenMode(void)
 {
	return (HDMI_OSD_MODE) GET_HDMI_OSD_MODE();
 }



HDMI_BOOL drvif_Hdmi_HdcpEnabled(void) 
{
    UINT8 value = Hdmi_HdcpPortRead(0xc1);
    return ((value  &0x60) == 0x60);
}

/**
 * drvif_Hdmi_GetInterlace
 * To tell scaler hdmi signal is interlace or not
 *
 * @param 				{  }
 * @return 				{ TRUE means interlaced, FALSE means progressive }
 * @ingroup drv_hdmi
 */
HDMI_BOOL drvif_Hdmi_GetInterlace(void)
{
	return GET_HDMI_ISINTERLACE();
}

/**
 * drvif_IsHDMI
 * To tell  scaler is HDMI/DVI
 *
 * @param 				{  }
 * @return 				{ return MODE of detection, MODE_HDMI = 1, MODE_DVI = 0 }
 * @ingroup drv_hdmi
 */

HDMI_DVI_MODE_T drvif_IsHDMI(void)
{

	return (HDMI_DVI_MODE_T)GET_ISHDMI();

}

// { return MODE true=PCM  false=RAW }
bool hdmi_Audio_Is_LPCM(void)
{
	return HDMI_AUDIO_IS_LPCM();
}

HDMI_RGB_YUV_RANGE_MODE_T drvif_IsRGB_YUV_RANGE(void)
{
	return (HDMI_RGB_YUV_RANGE_MODE_T)GET_HDMI_RGB_Q_RANGE();
}

/**
 * InProcHDMI_Setup
 * setup function after detect mode, return FALSE will result in re-detect mode
 *
 * @param 				{  }
 * @return 				{ FALSE  }
 * @ingroup drv_adc
 */

HDMI_BOOL InProcHDMI_Setup(void)
{
#ifndef HDMI_NOT_SUPPORT_SCALER
	HDMI_LOG("%s \n", "InProcHDMI_Setup");
	//if (hdmi_in(HDMI_HDMI_SR_VADDR) & _BIT6) { // AVMute Set
	//	return FALSE;
	//}
	drvif_mode_enableonlinemeasure();

	//20110930 kist add
	if(GET_ISHDMI() == MODE_HDMI) {
		Hdmi_AudioModeDetect();
		drvif_Hdmi_Audio_Check();
		hdmiport_mask(HDMI_HDMI_WDCR0_VADDR, ~(_BIT7),(_BIT7));		// Enable AVMute  	WDG
		//Hdmi_AudioModeDetect();//for AUDIO_FSM_AUDIO_WAIT_PLL_READY if normally
		//Hdmi_AudioModeDetect();//for AUDIO_FSM_AUDIO_START_OUT if normally
	}
#else	
	HDMI_LOG("%s \n", "InProcHDMI_Setup, HDMI_NOT_SUPPORT_SCALER");
#endif	
	return TRUE;
}

/**
 * InProcHDMI_Init
 * Initial Function after InitSrc
 *
 * @param 				{  }
 * @return 				{  }
 * @ingroup drv_hdmi
 */

void InProcHDMI_Init(void)
{
	HDMI_LOG("%s\n", "InProcHDMI_Init");
}


/**
 * drvif_Hdmi_GetColorDepth
 * to get the hdmi color depth
 *
 * @param 	{  }
 * @return 				{ 0: 8Bits , 1:10bits 2:12bits 3:16bits }
 * @ingroup drv_hdmi
 */

unsigned char drvif_Hdmi_GetColorDepth(void) {

	if (GET_HDMI_CD() >= 4) {
		return (GET_HDMI_CD()-4);
	} else {
		return 0;
	}
}


/**
 * drvif_HDMI_Acp_Type
 * To tell  scaler the acp type is.
 *
 * @param 				{  }
 * @return 				{ return 0 ~ 3 }
 * @ingroup drv_hdmi
 */

unsigned char drvif_HDMI_Acp_Type(void)
{
	return 0;
}

HDMI_BOOL drvif_IsAVMute(void) {
	return ((hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & _BIT6)	!= 0);
}

/*

	function for EDID/Hotplug/HDCP Key Registration

*/
HDMI_BOOL drvif_Hdmi_regTable(HDMI_TABLE_TYPE index, void *ptr)
{

	if ( !ptr )
		return FALSE;

//	HDMI_PRINTF("drvif_Hdmi_regTable %d %x\n", index, (unsigned int) ptr);
	switch ( index )
	{
		case HDMI_HDCP_KEY_TABLE:
#if (defined ENABLE_HDCPKEY_ENDIAN_SWAP)
			hdmi.hdcpkey = (HDCP_KEY_T1 *) ptr;
#else
			hdmi.hdcpkey = (HDCP_KEY_T *) ptr;
#endif
			break;
		case HDMI_CHANNEL0:
			hdmi.channel[0] = (HDMI_CHANNEL_T *) ptr;
			if (hdmi.channel[0]->mux_port == HDMI_MUX_PORT_NOTUSED) {
				hdmi.channel[0]->mux_ic = HDMI_MUX_NOUSED;
			}
			break;
			
		default:

			return FALSE;
	}
	return TRUE;
}


HDMI_3D_T drvif_HDMI_GetReal3DFomat(void) {
	return hdmi.tx_timing.hdmi_3dformat;
}

char drvif_Hdmi_IsAudioLock(void) {

//	return (GET_HDMI_AUDIO_FSM() == AUDIO_FSM_AUDIO_CHECK);
	return (GET_HDMI_AUDIO_FSM() != AUDIO_FSM_AUDIO_START);

}

HDMI_COLORIMETRY_T drvif_Hdmi_GetColorimetry(void) {

	return Hdmi_GetColorimetry();
//	return HDMI_COLORIMETRY_601;

}
//USER:LewisLee DATE:2012/08/08
//for Adams request, fix some player unstable bfore display
unsigned char drvif_HDMI_CheckConditionChange(void)
{
	if(HDMI_ERR_NO == Hdmi_CheckConditionChange() && false == Hdmi_3DInfo_IsChange())
		return  false;
	
	return true;
}

#if 0//def CONFIG_SOURCE_AUTO_SWITCH
unsigned char drvif_HDMI_PluginDetection1(void)
{
	if (IO_Direct_Get("PIN_HDMI1_DETECT")==1)
		{
		pr_info("#############HDMI1 detected!!!!!!!!!!!!\n");
		return _TRUE;
		}
	else
		{
		pr_info("#############HDMI1 not detected!!!!!!!!!!!!\n");
		return _FALSE;
		}
}
#if 0
unsigned char drvif_HDMI_PluginDetection2(void)
	{
		if (IO_Direct_Get("PIN_HDMI2_DETECT")==1)
			{
			pr_info("#############HDMI2 detected!!!!!!!!!!!!\n");
			return _TRUE;
			}
		else
			{
			pr_info("#############HDMI2 not detected!!!!!!!!!!!!\n");
			return _FALSE;
			}
	}
#endif
#endif

