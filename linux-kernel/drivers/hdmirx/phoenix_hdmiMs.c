//#include <stdbool.h>

#include "phoenix_hdmiInternal.h"
#include "phoenix_hdmiHWReg.h"
#include "phoenix_hdmiPlatform.h"
#include "phoenix_hdmi_wrapper.h"
#include "phoenix_mipi_wrapper.h"
#include "phoenix_hdmiInternal.h"
#include "sd_test.h"

#include <linux/kernel.h>
#include <linux/kthread.h>
#include<linux/string.h>
//#include "Add_in_common/hdmiSetForScaler.h"

#define ANALOG_MODE_MEASUREMENT      0
#define DIGITAL_MODE_MEASUREMENT     1


extern int hdmi_stream_on;

unsigned int hdmi_rx_output_format(unsigned int color)
{
    switch(color){
        case COLOR_RGB:
            return 0;
        case COLOR_YUV422:
            if((Rreg32(0x1801A200) & 0xFFFF) == 0x6329)
                return 0;   //1195: set format to RGB for YUV422 input
            else
                return 2;
        case COLOR_YUV444:
            return 1;
        default:
            return 3;
    }
}

unsigned int get_output_pitch_factor(MIPI_OUT_COLOR_SPACE_T output_color)
{
    switch(output_color){
    case OUT_RAW8:
    case OUT_RAW10:
        return 2;
    case OUT_YUV422:
    case OUT_YUV420:
    case OUT_JPEG8:
        return 1;
    default: //all other RGB case
        return 4;
    }
}

HDMI_VIC_TABLE_T hdmi_vic_table[] = {
/* {width, height, fps, interlace} */
	{ 0, 0, 60, 0 }, // No such vic, set fps=60 for standard timings and DVI mode
    { 640, 480, 60, 0 }, // vic: 1
    { 720, 480, 60, 0 }, // vic: 2 , 480p
    { 720, 480, 60, 0 }, // vic: 3
    {1280, 720, 60, 0 }, // vic: 4 , 720p60
    {1920,1080, 60, 1 }, // vic: 5, 1080i60
    { 720, 480, 60, 1 }, // vic: 6, NTSC
    { 720, 480, 60, 1 }, // vic: 7, NTSC
    { 720, 240, 60, 0 }, // vic: 8
    { 720, 240, 60, 0 }, // vic: 9
    {2880, 480, 60, 1 }, // vic: 10
    {2880, 480, 60, 1 }, // vic: 11
    {2880, 240, 60, 0 }, // vic: 12
    {2880, 240, 60, 0 }, // vic: 13
    {1440, 480, 60, 0 }, // vic: 14
    {1440, 480, 60, 0 }, // vic: 15
    {1920,1080, 60, 0 }, // vic: 16, 1080p60
    { 720, 576, 50, 0 }, // vic: 17, 576p
    { 720, 576, 50, 0 }, // vic: 18
    {1280, 720, 50, 0 }, // vic: 19, 720p50
    {1920,1080, 50, 1 }, // vic: 20, 1080i50
    { 720, 576, 50, 1 }, // vic: 21, PAL
    { 720, 576, 50, 1 }, // vic: 22
    { 720, 288, 50, 0 }, // vic: 23
    { 720, 288, 50, 0 }, // vic: 24
    {2880, 576, 50, 1 }, // vic: 25
    {2880, 576, 50, 1 }, // vic: 26
    {2880, 288, 50, 0 }, // vic: 27
    {2880, 288, 50, 0 }, // vic: 28
    {1440, 576, 50, 0 }, // vic: 29
    {1440, 576, 50, 0 }, // vic: 30
    {1920,1080, 50, 0 }, // vic: 31, 1080p50
    {1920,1080, 24, 0 }, // vic: 32
    {1920,1080, 25, 0 }, // vic: 33
    {1920,1080, 30, 0 }, // vic: 34
    {2880, 480, 60, 0 }, // vic: 35
    {2880, 480, 60, 0 }, // vic: 36
    {2880, 576, 50, 0 }, // vic: 37
    {2880, 576, 50, 0 }, // vic: 38
    {1920,1080, 50, 1 }, // vic: 39
    {1920,1080,100, 1 }, // vic: 40
    {1280, 720,100, 0 }, // vic: 41
    { 720, 576,100, 0 }, // vic: 42
    { 720, 576,100, 0 }, // vic: 43
    { 720, 576,100, 1 }, // vic: 44
    { 720, 576,100, 1 }, // vic: 45
    {1920,1080,120, 1 }, // vic: 46
    {1280, 720,120, 0 }, // vic: 47
    { 720, 480,120, 0 }, // vic: 48
    { 720, 480,120, 0 }, // vic: 49
    { 720, 480,120, 1 }, // vic: 50
    { 720, 480,120, 1 }, // vic: 51
    { 720, 576,200, 0 }, // vic: 52
    { 720, 576,200, 0 }, // vic: 53
    { 720, 576,200, 1 }, // vic: 54
    { 720, 576,200, 1 }, // vic: 55
    { 720, 480,240, 0 }, // vic: 56
    { 720, 480,240, 0 }, // vic: 57
    { 720, 480,240, 1 }, // vic: 58
    { 720, 480,240, 1 }, // vic: 59
};

/**
 * Hdmi_Get3DInfo
 *    Get 3D information from 0x81 Vendor Specific Packet
 *
 * @param               {  }
 * @return              { TRUE if ACR is recieved or return FALSE }
 * @ingroup drv_hdmi
 */

HDMI_3D_T Hdmi_Get3DInfo(HDMI_MS_MODE_T mode)
{
    switch (mode) 
    {
        case HDMI_MS_MODE_ONESHOT:
        {
            hdmi_rx_reg_mask32(HDMI_HDMI_PTRSV1_VADDR, 0xff00ffff, 0x00810000, HDMI_RX_MAC);//Packet Type = 0x81(HDMI Vendor Specific InfoFrame)
            hdmi_rx_reg_mask32(HDMI_HDMI_GPVS_VADDR, ~(HDMI_GPVS_pis_7_mask), (HDMI_GPVS_pis_7(1)), HDMI_RX_MAC);  // Clear RSV3 indicator
            HDMI_DELAYMS(50);
            if (hdmi_rx_reg_read32(HDMI_HDMI_GPVS_VADDR, HDMI_RX_MAC) & HDMI_GPVS_pis_7_mask) 
            {
                if (HDMI_VIDEO_FORMAT_get_hvf(hdmi_rx_reg_read32(HDMI_HDMI_VIDEO_FORMAT_VADDR, HDMI_RX_MAC))  != 0x02)
                { // if 3D format indication present is absent, return 2D anyway
                        return HDMI3D_2D_ONLY;
                } 
                else {
                    return (HDMI_3D_T) HDMI_3D_FORMAT_get_3d_structure(hdmi_rx_reg_read32(HDMI_HDMI_3D_FORMAT_VADDR, HDMI_RX_MAC));
                }
            }
            else
            {
                    HDMI_PRINTF(" !!!!!!! Don't GET  Hdmi_Get3DInfo \n");
            }
            return HDMI3D_2D_ONLY;
        }
        case HDMI_MS_MODE_ONESHOT_INIT:
        {
            hdmi_rx_reg_mask32(HDMI_HDMI_PTRSV1_VADDR, 0xff00ffff, 0x00810000, HDMI_RX_MAC);//Packet Type = 0x81(HDMI Vendor Specific InfoFrame)
            hdmi_rx_reg_mask32(HDMI_HDMI_GPVS_VADDR, ~(HDMI_GPVS_pis_7_mask), (HDMI_GPVS_pis_7(1)), HDMI_RX_MAC);  // Clear RSV3 indicator
            break;
        }
        default:
            return HDMI3D_UNKOWN;
        break;
    }
    return HDMI3D_UNKOWN;
}

bool Hdmi_3DInfo_IsChange(void)
{
     HDMI_3D_T check_3dFomat = Hdmi_Get3DInfo(HDMI_MS_MODE_ONESHOT);
     if(drvif_HDMI_GetReal3DFomat() != check_3dFomat)
     {
          hdmi.tx_timing.hdmi_3dformat = check_3dFomat;
          return true;     
     }
     return false;
}




unsigned char Hdmi_Get3DExtInfo(void) {

        return HDMI_3D_FORMAT_get_3d_ext_data(hdmi_rx_reg_read32(HDMI_HDMI_3D_FORMAT_VADDR, HDMI_RX_MAC));


}



/**********************************************************************************
/Function : unsigned char drvif_Hdmi_AVI_RGB_Range(void)
/parameter : none
/return : 0 :default 1 : Limited Range 2: Full Range
/ note : For Vip Get HDMI RGB Quantization Range
***********************************************************************************/
/*//////////////////////////////////////////////////////////////////////////////////////////
// Function : unsigned char drvif_mode_onlinemeasure(unsigned char mode)
//
//////////////////////////////////////////////////////////////////////////////////////////*/
unsigned int rx_pitch_measurement(unsigned int output_h, MIPI_OUT_COLOR_SPACE_T output_color)
{
    unsigned int pitch;
    pr_info("rx_pitch_measurement,output_color=%x\n",output_color);
    pitch = output_h * get_output_pitch_factor(output_color);
    // pitch has to be n*16 byte
    if(output_color < OUT_ARGB)
        pitch = roundup16(pitch);
    return pitch;
}

void hdmi_rx_size_measurement(void)
{
    HDMI_TIMING_T *timing = &hdmi.tx_timing;
    timing->h_act_len = hdmirx_wrapper_get_active_pixel();
    timing->v_act_len = hdmirx_wrapper_get_active_line();
 
    pr_info("[H=%x,V=%x,O_H=%x,O_V=%x,O_C=%d]\n",timing->h_act_len,timing->v_act_len,timing->output_h, timing->output_v, timing->output_color);
}

int hdmi_onms_measure(HDMI_TIMING_T *timing) 
{
	if (timing == NULL) timing = &hdmi.tx_timing;

	timing->h_act_len = hdmirx_wrapper_get_active_pixel();
	timing->v_act_len = hdmirx_wrapper_get_active_line();

	if((timing->h_act_len<640)||(timing->v_act_len<240))
		return HDMI_ERR_HDMIWRAPPER_NOT_READY;
	else
		return HDMI_ERR_NO;
}

char Hdmi_GetPixelDownSample(void) {

    if (HDMI_VCR_get_prdsam(hdmi_rx_reg_read32(HDMI_HDMI_VCR_VADDR, HDMI_RX_MAC))) {
        return HDMI_VCR_get_dsc_r(hdmi_rx_reg_read32(HDMI_HDMI_VCR_VADDR, HDMI_RX_MAC));
    } else {
        return HDMI_VCR_get_dsc(hdmi_rx_reg_read32(HDMI_HDMI_VCR_VADDR, HDMI_RX_MAC));
    }


}

int Hdmi_GetColorDepth(void) {
    return TMDS_DPC0_dpc_cd(hdmi_rx_reg_read32(HDMI_TMDS_DPC0_VADDR, HDMI_RX_MAC));
}

/**
 * Hdmi_GetInterlace
 * check if input signal is interlace or not , for
 *
 * @param               {  }
 * @return              { TRUE means interlaced, FALSE means progressive }
 * @ingroup drv_hdmi
 */

HDMI_BOOL Hdmi_GetInterlace(HDMI_MS_MODE_T mode)
{

    //mark to solve segmentation fail static int progressive =  GET_HDMI_ISINTERLACE() == 0;
    static int progressive = 0;
    progressive = !GET_HDMI_ISINTERLACE();

    switch (mode) {
        case HDMI_MS_MODE_ONESHOT:
            hdmi_rx_reg_write32(HDMI_HDMI_VCR_VADDR, hdmi_rx_reg_read32(HDMI_HDMI_VCR_VADDR, HDMI_RX_MAC) | _BIT6, HDMI_RX_MAC); //clear status
            Hdmi_WaitVsync(15);
            Hdmi_WaitVsync(15);
            if(hdmi_rx_reg_read32(HDMI_HDMI_VCR_VADDR, HDMI_RX_MAC) & _BIT6) {// interlace mode
                //HDMI_PRINTF(">>>> HDMI interlaced >>>>\n");
                progressive = 0;
                return 1;
            } else {
                //HDMI_PRINTF(">>>> HDMI Progressive >>>>\n");
                progressive = 1;
                return 0;
            }
        break;
        default:
        break;
    }
    if(progressive) {// interlace mode
        return 0;
    } else {
        return 1;
    }

}


#if 1
HDMI_BOOL Hdmi_MeasureTiming(HDMI_TIMING_T *timing, int b) 
{
    //Measure Setup

    #if HDMI_CRC_DUMP
    hdmi_rx_reg_write32(HDMI_TMDS_CTRL_VADDR, 0xf9);
    #endif
    SET_HDMI_COLOR_SPACE(Hdmi_GetColorSpace());
    
    timing->colorimetry = Hdmi_GetColorimetry();
    timing->color = GET_HDMI_COLOR_SPACE();
    timing->progressive = GET_HDMI_ISINTERLACE() == 0;
    timing->depth =(HDMI_COLOR_DEPTH_T) drvif_Hdmi_GetColorDepth();
    if (hdmi_onms_measure(timing) != HDMI_ERR_NO)
    {
        pr_info("[HDMI RX] %s return false\n",__FUNCTION__);
        return FALSE;
    }

    //Measure Result  Read
    if((timing->h_act_len == 0) || (timing->v_act_len == 0) )//1221 kist

    {

        pr_err("Wrong Timing %xx%x\n",timing->h_act_len , timing->v_act_len );

        //HDMI_PRINTF("\n( IHTotal == %x) || (IVTotal== %x)\n", timing->h_total , timing->v_total);

        #if HDMI_CRC_DUMP

        HdmiDumpCRC();

        #endif
        pr_info("[return 2,%x,%x]\n",timing->h_act_len,timing->v_act_len);
        return FALSE;

    }

    timing->progressive = GET_HDMI_ISINTERLACE() == 0;
    // Horizontal active width must be even
    if (timing->h_act_len & 0x01) timing->h_act_len -= 1;


    pr_err("IHWidth:%d\n", timing->h_act_len);

    pr_err("IVHeight:%d\n", timing->v_act_len);

    #if HDMI_CRC_DUMP

    HdmiDumpCRC();

    #endif

    return TRUE;
}


#if HDMI_CRC_DUMP
void HdmiDumpCRC(void) {




}
#endif



void HdmiMACCRC(HDMI_MS_MODE_T action) {


    switch(action) {
            case HDMI_MS_MODE_CONTINUOUS_INIT:
                    hdmi_rx_reg_mask32(HDMI_TMDS_CRCC_VADDR, ~(TMDS_CRCC_crcc_mask | TMDS_CRCC_crc_nonstable_mask),
                                    (TMDS_CRCC_crcc_mask | TMDS_CRCC_crc_nonstable_mask), HDMI_RX_MAC);
            break;

            case HDMI_MS_MODE_CONTINUOUS_CHECK:

                if ((hdmi_rx_reg_read32(HDMI_TMDS_CRCC_VADDR, HDMI_RX_MAC) & TMDS_CRCC_crc_done_mask) != TMDS_CRCC_crc_done_mask) {
                    HDMI_PRINTF("MAC CRC not done yet\n");
                    return;
                }
//              if (hdmi_rx_reg_read32(HDMI_TMDS_CRCC_VADDR) & TMDS_CRCC_crc_nonstable_mask) {
                if (1) {
                    HDMI_PRINTF("MAC CRC = %x %x nonstatble = %d\n", hdmi_rx_reg_read32(HDMI_TMDS_CRCO0_VADDR, HDMI_RX_MAC), hdmi_rx_reg_read32(HDMI_TMDS_CRCO1_VADDR, HDMI_RX_MAC), hdmi_rx_reg_read32(HDMI_TMDS_CRCC_VADDR, HDMI_RX_MAC) & TMDS_CRCC_crc_nonstable_mask);
                    hdmi_rx_reg_mask32(HDMI_TMDS_CRCC_VADDR, ~TMDS_CRCC_crc_nonstable_mask, TMDS_CRCC_crc_nonstable_mask, HDMI_RX_MAC);
                }

            break;
            default:
                break;
    }
}



#endif

void Hdmi_WaitVsync(int num)
{
    int i;

	hdmi_rx_reg_mask32(HDMI_TMDS_CTRL_VADDR, ~TMDS_CTRL_yo_mask, TMDS_CTRL_yo(1), HDMI_RX_MAC);
	for (i=0; i<num; i++)
	{
		if (TMDS_CTRL_get_yo(hdmi_rx_reg_read32(HDMI_TMDS_CTRL_VADDR, HDMI_RX_MAC)))
		{
			//Wait vsync done
			return;
		}
		usleep_range(10000, 15000);//10~15ms
	}
	return;
}

HDMI_COLORIMETRY_T Hdmi_GetColorimetry(void) {

    unsigned int C, EC;
    hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 0x0, HDMI_RX_MAC);
    if (hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC) != 0) return HDMI_COLORIMETRY_NOSPECIFIED;
    hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 0x5, HDMI_RX_MAC);
    C =  ((hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC) >> 6) & 0x03);
    hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 0x6, HDMI_RX_MAC);
    EC =((hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC) >> 4) & 0x07);

    if (C != 0x03) {
        return (HDMI_COLORIMETRY_T) C;
    } else {
        return (HDMI_COLORIMETRY_T) (HDMI_COLORIMETRY_XYYCC601 + EC);
    }

}





