//#include <stdio.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <asm/io.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#include "phoenix_hdmiInternal.h"
#include "phoenix_hdmiHWReg.h"
#include "phoenix_mipi_wrapper.h"
#include "phoenix_hdmi_wrapper.h"
#include "phoenix_MHL_CBUS.h"
//#include "Add_in_common/hdmiSetForScaler.h"

MIPI_TOP_INFO mipi_top;

HDMI_INFO_T hdmi;
MIPI_CSI_INFO_T mipi_csi;
extern unsigned int  hdmi_rx_output_format(unsigned int color);


void Hdmi_HdcpInit(void);

HDMI_CHANNEL_T default_channel[3];
#define Debug_FIFOclock 0
#define CLOCK_INVERT			1

/****************************
        EDID Info
*****************************/
int DDC_Max_Channel = HDMI_MAX_CHANNEL;

unsigned char hdmi_default_edid[3][256] = {
    {    0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x4A,0x8B,0x54,0x4C,0x01,0x00,0x00,0x00,
	0x0C,0x11,0x01,0x03,0x81,0x46,0x27,0x78,0x8A,0xA5,0x8E,0xA6,0x54,0x4A,0x9C,0x26,
	0x12,0x45,0x46,0xAD,0xCE,0x00,0x81,0x40,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x1D,0x00,0x72,0x51,0xD0,0x1E,0x20,0x6E,0x28,
	0x55,0x00,0xB9,0x88,0x21,0x00,0x00,0x1E,0x9A,0x29,0xA0,0xD0,0x51,0x84,0x22,0x30,
	0x50,0x98,0x36,0x00,0xB9,0x88,0x21,0x00,0x00,0x1C,0x00,0x00,0x00,0xFD,0x00,0x32,
	0x4B,0x18,0x3C,0x0B,0x00,0x0A,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xFC,
	0x00,0x33,0x32,0x56,0x33,0x48,0x2D,0x48,0x36,0x41,0x0A,0x20,0x20,0x20,0x01,0xE3,
	0x02,0x03,0x21,0x71,0x4E,0x06,0x07,0x02,0x03,0x15,0x96,0x11,0x12,0x13,0x04,0x14,
	0x05,0x1F,0x90,0x23,0x09,0x07,0x07,0x83,0x01,0x00,0x00,0x65,0x03,0x0C,0x00,0x10,
	0x00,0x8C,0x0A,0xD0,0x90,0x20,0x40,0x31,0x20,0x0C,0x40,0x55,0x00,0xB9,0x88,0x21,
	0x00,0x00,0x18,0x01,0x1D,0x80,0x18,0x71,0x1C,0x16,0x20,0x58,0x2C,0x25,0x00,0xB9,
	0x88,0x21,0x00,0x00,0x9E,0x01,0x1D,0x80,0xD0,0x72,0x1C,0x16,0x20,0x10,0x2C,0x25,
	0x80,0xB9,0x88,0x21,0x00,0x00,0x9E,0x01,0x1D,0x00,0xBC,0x52,0xD0,0x1E,0x20,0xB8,
	0x28,0x55,0x40,0xB9,0x88,0x21,0x00,0x00,0x1E,0x02,0x3A,0x80,0xD0,0x72,0x38,0x2D,
	0x40,0x10,0x2C,0x45,0x80,0xB9,0x88,0x21,0x00,0x00,0x1E,0x00,0x00,0x00,0x00,0xD0
    },
    {    0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x4A,0x8B,0x54,0x4C,0x01,0x00,0x00,0x00,
	0x0C,0x11,0x01,0x03,0x81,0x46,0x27,0x78,0x8A,0xA5,0x8E,0xA6,0x54,0x4A,0x9C,0x26,
	0x12,0x45,0x46,0xAD,0xCE,0x00,0x81,0x40,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x1D,0x00,0x72,0x51,0xD0,0x1E,0x20,0x6E,0x28,
	0x55,0x00,0xB9,0x88,0x21,0x00,0x00,0x1E,0x9A,0x29,0xA0,0xD0,0x51,0x84,0x22,0x30,
	0x50,0x98,0x36,0x00,0xB9,0x88,0x21,0x00,0x00,0x1C,0x00,0x00,0x00,0xFD,0x00,0x32,
	0x4B,0x18,0x3C,0x0B,0x00,0x0A,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xFC,
	0x00,0x33,0x32,0x56,0x33,0x48,0x2D,0x48,0x36,0x41,0x0A,0x20,0x20,0x20,0x01,0xE3,
	0x02,0x03,0x21,0x71,0x4E,0x06,0x07,0x02,0x03,0x15,0x96,0x11,0x12,0x13,0x04,0x14,
	0x05,0x1F,0x90,0x23,0x09,0x07,0x07,0x83,0x01,0x00,0x00,0x65,0x03,0x0C,0x00,0x10,
	0x00,0x8C,0x0A,0xD0,0x90,0x20,0x40,0x31,0x20,0x0C,0x40,0x55,0x00,0xB9,0x88,0x21,
	0x00,0x00,0x18,0x01,0x1D,0x80,0x18,0x71,0x1C,0x16,0x20,0x58,0x2C,0x25,0x00,0xB9,
	0x88,0x21,0x00,0x00,0x9E,0x01,0x1D,0x80,0xD0,0x72,0x1C,0x16,0x20,0x10,0x2C,0x25,
	0x80,0xB9,0x88,0x21,0x00,0x00,0x9E,0x01,0x1D,0x00,0xBC,0x52,0xD0,0x1E,0x20,0xB8,
	0x28,0x55,0x40,0xB9,0x88,0x21,0x00,0x00,0x1E,0x02,0x3A,0x80,0xD0,0x72,0x38,0x2D,
	0x40,0x10,0x2C,0x45,0x80,0xB9,0x88,0x21,0x00,0x00,0x1E,0x00,0x00,0x00,0x00,0xD0
    },
    {    0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x4A,0x8B,0x54,0x4C,0x01,0x00,0x00,0x00,
	0x0C,0x11,0x01,0x03,0x81,0x46,0x27,0x78,0x8A,0xA5,0x8E,0xA6,0x54,0x4A,0x9C,0x26,
	0x12,0x45,0x46,0xAD,0xCE,0x00,0x81,0x40,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x1D,0x00,0x72,0x51,0xD0,0x1E,0x20,0x6E,0x28,
	0x55,0x00,0xB9,0x88,0x21,0x00,0x00,0x1E,0x9A,0x29,0xA0,0xD0,0x51,0x84,0x22,0x30,
	0x50,0x98,0x36,0x00,0xB9,0x88,0x21,0x00,0x00,0x1C,0x00,0x00,0x00,0xFD,0x00,0x32,
	0x4B,0x18,0x3C,0x0B,0x00,0x0A,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xFC,
	0x00,0x33,0x32,0x56,0x33,0x48,0x2D,0x48,0x36,0x41,0x0A,0x20,0x20,0x20,0x01,0xE3,
	0x02,0x03,0x21,0x71,0x4E,0x06,0x07,0x02,0x03,0x15,0x96,0x11,0x12,0x13,0x04,0x14,
	0x05,0x1F,0x90,0x23,0x09,0x07,0x07,0x83,0x01,0x00,0x00,0x65,0x03,0x0C,0x00,0x10,
	0x00,0x8C,0x0A,0xD0,0x90,0x20,0x40,0x31,0x20,0x0C,0x40,0x55,0x00,0xB9,0x88,0x21,
	0x00,0x00,0x18,0x01,0x1D,0x80,0x18,0x71,0x1C,0x16,0x20,0x58,0x2C,0x25,0x00,0xB9,
	0x88,0x21,0x00,0x00,0x9E,0x01,0x1D,0x80,0xD0,0x72,0x1C,0x16,0x20,0x10,0x2C,0x25,
	0x80,0xB9,0x88,0x21,0x00,0x00,0x9E,0x01,0x1D,0x00,0xBC,0x52,0xD0,0x1E,0x20,0xB8,
	0x28,0x55,0x40,0xB9,0x88,0x21,0x00,0x00,0x1E,0x02,0x3A,0x80,0xD0,0x72,0x38,0x2D,
	0x40,0x10,0x2C,0x45,0x80,0xB9,0x88,0x21,0x00,0x00,0x1E,0x00,0x00,0x00,0x00,0xD0
    }
};

HDCP_KEY_T rx_hdcpkey;

HDMI_CONST HDMI_AUDIO_PLL_COEFF_T audio_pll_coeff[] = {
    { 32000, _AUDIO_256_TIMES, 0},
    { 44100, _AUDIO_256_TIMES, 0},
    { 48000, _AUDIO_256_TIMES, 0},
    { 88200, _AUDIO_256_TIMES, 1},
    { 96000, _AUDIO_256_TIMES, 1},
    { 176400, _AUDIO_128_TIMES, 2},
    { 192000, _AUDIO_128_TIMES, 3}
};


static unsigned int ACR_N=0;
unsigned char HDMI_Audio_Conut = 0;
HDMI_AUDIO_PLL_PARAM_T hdmi_audiopll_param[] = {

        { 2,   20,  2,  8,  1,  0x1D, 0xDC,  "32K"     },//0x1E, 0xB0
        { 2,   20,  2,  6,  1,  0xFE, 0x7A,  "44.1K"  },//0xFC, 0x68
        { 2,   22,  2,  6,  1,  0x07, 0x40,  "48K"     },
        { 2,   20,  1,  6,  1,  0xFE, 0x7A,  "88.2K"  },
        { 2,   22,  1,  6,  1,  0x07, 0x40,  "96K"     },
        { 2,   20,  1,  6,  0,  0xFE, 0x7A,  "176.4K" },
        { 2,   22,  1,  6,  0,  0x07, 0x40,  "192K"    }

};

HDMI_CONST VIDEO_DPLL_RATIO_T dpll_ratio[] = {

    {   15, 15,     1, 1    },  // 24 bits
    {   12, 15,     4, 5    },  // 30 bits
    {   10, 15,     2, 3    },  // 36 bits
    {   15, 30,     1, 2    },  // 48 bits
};

HDMI_CONST HDMI_COLOR_SPACE_T ColorMap[] = {
                                    COLOR_RGB,      // 0
                                    COLOR_YUV422,   // 01
                                    COLOR_YUV444,   // 10
                                    COLOR_YUV444,   // 11
};


HDMI_CONST unsigned short AUDIO_INFO_FREQ_TABLE[] = {
    0,      // refer to stream headers
    320,    // 44.1k
    441,     // Unkwon
    480, // 48k
    882,
    960,
    1764,
    1920
};

HDMI_CONST unsigned int AUDIO_CHANNEL_STATUS[] = {
    44100,
       0,                  // 000 indicate standard no support
    48000,
    32000,
    22000,
       0,
    24000,
       0,
    88200,
       0,
    96000,
       0,
    176000,
       0,
    192000,
       0,
};

#define AVI_Data_BYTE1     (4)
#define AVI_Data_BYTE3     (6)
#define AVI_Data_BYTE4	   (7)
#define AVI_Data_BYTE5     (8)
#define AVI_Y1Y0_mask  0x60
#define AVI_YQ1YQ0_mask  0xC0
#define AVI_Q_Range_mask  0x0c
#define RGB_Default                  0
#define RGB_Limite_Range         1
#define RGB_Full_Range             2
#define Not_Read                       3
#define YUV_Limite_Range         0
#define YUV_Full_Range             1

void HdmiRx_Save_DTS_EDID_Table_Init(HDMIRX_DTS_EDID_TBL_T * dts_edid, int max_ddc_channel)
{
   int i;

   for(i = 0; i < max_ddc_channel && i < HDMI_MAX_CHANNEL; i++)
   {
      default_channel[i].use_dts_edid = 1;
      memcpy( default_channel[i].EDID, dts_edid[i].edid, 256);
   }
   DDC_Max_Channel = max_ddc_channel;
}

void HdmiTable_Init(void)
{
    int i, osd_index;
    /*
        HDMI PHY Port /DDC/HotPlugPin Selection and Enable
    */
    pr_info("cloud UNIT TEST HDMI TABLE !!!!\n");
    for (osd_index=0; osd_index < 3; osd_index++)
    {
            default_channel[osd_index].HotPlugPin = 1;
            default_channel[osd_index].HotPlugPinHighValue = 1;
            default_channel[osd_index].DVIaudio_switch_time_10ms = 30;
//            default_channel[osd_index].VFreqUpperBound = 0;     // 0 means there is no Vfreq upper bound

            default_channel[osd_index].port_enable = 0;
            default_channel[osd_index].phy_port = osd_index;
            default_channel[osd_index].mux_ic = 2;
            default_channel[osd_index].mux_port = HDMI_MUX_PORT_NOTUSED;
            default_channel[osd_index].mux_ddc_port = 0;
            default_channel[osd_index].ddc_selected = osd_index;
            default_channel[osd_index].edid_type = 0;
            default_channel[osd_index].edid_initialized = 0;
            default_channel[osd_index].ddcci_enable = 0;
            default_channel[osd_index].HotPlugType = 0;
            default_channel[osd_index].DVIaudioPath = 0;
            default_channel[osd_index].HDMIaudioPath = 0;
    }
    for (i=0; i<3;i++)
    {
        if (default_channel[i].phy_port < HDMI_PHY_PORT_NOTUSED)
        {
            drvif_Hdmi_regTable((HDMI_TABLE_TYPE)(HDMI_CHANNEL0 + i), &default_channel[i] );

            if (default_channel[i].edid_type != HDMI_EDID_EEPROM)
            {
                if(default_channel[i].use_dts_edid == 1)
                {
                  default_channel[i].edid_initialized = 1;
                  //pr_info("[%x%x%x%x%x%x%x%x]\n",hdmi_default_edid[i][1],hdmi_default_edid[i][17],hdmi_default_edid[i][33],hdmi_default_edid[i][49],hdmi_default_edid[i][65],hdmi_default_edid[i][81],hdmi_default_edid[i][97],hdmi_default_edid[i][113]);
                  drvif_Hdmi_LoadEDID((DDC_NUMBER_T) i, default_channel[i].EDID, 256);
                }
                else
                {
                  memcpy( default_channel[i].EDID, hdmi_default_edid[i], 256);
                  default_channel[i].edid_initialized = 1;
                  //pr_info("[%x%x%x%x%x%x%x%x]\n",hdmi_default_edid[i][1],hdmi_default_edid[i][17],hdmi_default_edid[i][33],hdmi_default_edid[i][49],hdmi_default_edid[i][65],hdmi_default_edid[i][81],hdmi_default_edid[i][97],hdmi_default_edid[i][113]);
                  drvif_Hdmi_LoadEDID((DDC_NUMBER_T) i, default_channel[i].EDID, 256);
                }
            }
            else
            {
                default_channel[i].edid_initialized = 1;
            }
            //Scaler_RegTable(SCALER_REG_TABLE_HDMI_CHANNEL0_CONFIG + i, &default_channel[i] );
        }
    }
    pr_info("cloud UNIT TEST HDMI TABLE  finish !!!!\n");
#if 0//defined (ENABLE_MHL_FLAG)   // for cbus table
    drvif_Hdmi_regTable((HDMI_TABLE_TYPE)(CBUS_TABLE), hdmi_Cbus_edid );
    drvif_CBUS_LoadEDID(hdmi_Cbus_edid,256);
//  Scaler_RegTable(SCALER_REG_TABLE_HDMI_HDCP_KEY,(void *)&default_hdcpkey);
#endif
    //drvif_Hdmi_regTable(HDMI_HDCP_KEY_TABLE, (void *)&default_hdcpkey);
}

void HdmiRx_change_edid_physical_addr(unsigned char byteMSB, unsigned char byteLSB)
{
    unsigned char block_tag=0,block_size,offset,offset_max;
    unsigned int i,sum=0;

	offset = 0x84;//First data block
	offset_max = 0x80+default_channel[0].EDID[0x82];

	while(offset < offset_max)
	{
		block_tag = default_channel[0].EDID[offset]>>4;
		block_size = default_channel[0].EDID[offset] & 0xF;
		if(block_tag==6)
		{
			offset += 4;//Physical Address offset
			break;
		}
		else
			offset = offset + block_size +1;//Next block
	}

	if(block_tag != 6)//Make sure found VSDB
	{
		pr_err("[HDMI RX] Change EDID physical addr fail, couldn't find vendor specific data block\n");
		return;
	}

	//Repeater PA Increment
	if((byteMSB&0x0F) == 0x0)
		byteMSB = byteMSB | 0x1;
	else if((byteLSB&0xF0) == 0x0)
		byteLSB = byteLSB | 0x10;
	else if((byteLSB&0x0F) == 0)
		byteLSB = byteLSB | 0x1;
	else
	{
		byteMSB = 0xFF;
		byteLSB = 0xFF;
	}

	pr_info("[HDMI RX][%s] EDID[0x%02x]=0x%02x, EDID[0x%02x]=0x%02x\n",__FUNCTION__,offset,byteMSB,offset+1,byteLSB);
    default_channel[0].EDID[offset] = byteMSB;
    default_channel[0].EDID[offset+1] = byteLSB;

    //Calculate and set checksum
    for(i=0x80;i<0xFF;i++)
        sum += default_channel[0].EDID[i];
    default_channel[0].EDID[0xFF] = (0x100-(sum&0xFF))&0xFF;

    //Reload EDID table
    drvif_Hdmi_LoadEDID((DDC_NUMBER_T) 0, default_channel[0].EDID, 256);

    return;
}

void HdmiRx_enable_hdcp(unsigned char *bksv, unsigned char *device_key)
{
	if(hdmi.hdcpkey_enable)//Already save the key, return
		return;

	memcpy(&rx_hdcpkey.BKsv,bksv,5);
	memcpy(&rx_hdcpkey.Key,device_key,320);

	drvif_Hdmi_regTable(HDMI_HDCP_KEY_TABLE, (void *)&rx_hdcpkey);
	hdmi.hdcpkey_enable = true;
	if(mipi_top.hdmi_rx_init)
		Hdmi_HdcpInit();
	return;
}

/**********************************************************************************
/Function : unsigned char drvif_Hdmi_AVI_RGB_Range(void)
/parameter : none
/return : 0 :default 1 : Limited Range 2: Full Range
/ note : For Vip Get HDMI RGB Quantization Range
***********************************************************************************/
 unsigned char drvif_Hdmi_AVI_RGB_Range(void)
{
    unsigned char   Temp ,RGB_Temp,YUV_Temp;
    /*
        looking for ACR info using RSV2
    */

    HDMI_PRINTF("drvif_Hdmi_AVI_RGB_Range \n");
    hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, AVI_Data_BYTE1, HDMI_RX_MAC); //read AVI infor REG direct

    Temp =  (hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC) & AVI_Y1Y0_mask)>>5 ;

        if(Temp == 0)
       {
                hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, AVI_Data_BYTE3, HDMI_RX_MAC); //read AVI infor Data Byte 3
                        RGB_Temp =  (hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC) & AVI_Q_Range_mask)>>2 ;
                switch(RGB_Temp)
                {
                    case RGB_Default :
                        SET_HDMI_RGB_Q_RANGE(RGB_Default);
                    HDMI_PRINTF("drvif_Hdmi_AVI_RGB_Range test0  = %d\n", GET_HDMI_RGB_Q_RANGE());
                            break;
                    case RGB_Limite_Range:
                    SET_HDMI_RGB_Q_RANGE(RGB_Limite_Range);
                    HDMI_PRINTF("drvif_Hdmi_AVI_RGB_Range test1 = %d\n", GET_HDMI_RGB_Q_RANGE());
                        break;
                    case RGB_Full_Range:
                    SET_HDMI_RGB_Q_RANGE(RGB_Full_Range);
                    HDMI_PRINTF("drvif_Hdmi_AVI_RGB_Range test2  = %d\n", GET_HDMI_RGB_Q_RANGE());
                        break;
                }
                HDMI_PRINTF("drvif_Hdmi_AVI_RGB_Range = %d\n", RGB_Temp);
        }
        else if ((Temp == 0x01 )||(Temp == 0x02 ))
        {
                hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, AVI_Data_BYTE5, HDMI_RX_MAC); //read AVI infor Data Byte 3
                        YUV_Temp =  (hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC) & AVI_YQ1YQ0_mask)>>6 ;
                switch(YUV_Temp)
                {
                    case YUV_Limite_Range:
                    SET_HDMI_RGB_Q_RANGE(RGB_Limite_Range);
                    HDMI_PRINTF("drvif_Hdmi_AVI_YUV_Range test1 = %d\n", GET_HDMI_RGB_Q_RANGE());
                        break;
                    case YUV_Full_Range:
                    SET_HDMI_RGB_Q_RANGE(RGB_Full_Range);
                    HDMI_PRINTF("drvif_Hdmi_AVI_YUV_Range test2  = %d\n", GET_HDMI_RGB_Q_RANGE());
                        break;
                }
                HDMI_PRINTF("drvif_Hdmi_AVI_YUV_Range = %d\n", YUV_Temp);
        }

    return Temp;
}

/**********************************************************************************
/Function : unsigned char drvif_Hdmi_AVI_VIC(void)
/parameter : none
/return : VIC value
/ note : AVI(Auxiliary Video Information), VIC(Video Identification Code, CEA-861)
***********************************************************************************/
unsigned char drvif_Hdmi_AVI_VIC(void)
{
	unsigned char reg_val;
	hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, AVI_Data_BYTE4, HDMI_RX_MAC);

	reg_val = (hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC)) & 0x7F;

	if(reg_val <= 59)
		return reg_val;
	else
		return 0;
}

unsigned char Hdmi_WaitAudioSample(void) {


    char   timeout_cnt = 6;
    /*
        looking for ACR info using RSV2
    */

    hdmi_rx_reg_mask32(HDMI_HDMI_PTRSV1_VADDR, ~0x00FF00, 0x02<< 8 , HDMI_RX_MAC);        // Wait for ACR : Packet Type = 0x01
    hdmi_rx_reg_write32(HDMI_HDMI_GPVS_VADDR, _BIT6, HDMI_RX_MAC);  // Clear RSV2 indicator

    for (;timeout_cnt>0;timeout_cnt--)  { // Wait 30ms max. to wait ACR
         HDMI_DELAYMS(10);
        if (hdmi_rx_reg_read32(HDMI_HDMI_GPVS_VADDR, HDMI_RX_MAC) & _BIT6)  {
            hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 110, HDMI_RX_MAC);

            if (hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC) & 0x10) {
                SET_HDMI_AUDIO_LAYOUT(1);
            } else {
                SET_HDMI_AUDIO_LAYOUT(0);
            }
            //HDMI_PRINTF("Layout = %d\n", GET_HDMI_AUDIO_LAYOUT());
#if HDMI_SUPPORT_HBR
        timeout_cnt = 6;
//      hdmi_rx_reg_write32(HDMI_HBR_PACKET_VADDR,0x06);
        for (;timeout_cnt>0;timeout_cnt--)  { // Wait 30ms max. to wait HBR
             HDMI_DELAYMS(5);
            if  (hdmi_rx_reg_read32(HDMI_HBR_PACKET_VADDR) & 0x01) {
//              hdmi_rx_reg_write32(HDMI_HBR_PACKET_VADDR,0x04);
                SET_HDMI_HBR_MODE(0);
                HDMI_PRINTF("Both Packet 2 and HBR exist\n");
            }
        }
#else
        hdmi_rx_reg_write32(HDMI_HIGH_BIT_RATE_AUDIO_PACKET_VADDR, 0x04, HDMI_RX_MAC);
        //HDMI_PRINTF("Audio sample counter = %d\n",timeout_cnt);
#endif
        return TRUE;
        }

    }
#if HDMI_SUPPORT_HBR
        timeout_cnt = 6;
//      hdmi_rx_reg_write32(HDMI_HBR_PACKET_VADDR,0x06);
        for (;timeout_cnt>0;timeout_cnt--)  { // Wait 30ms max. to wait HBR
             HDMI_DELAYMS(5);
            if  (hdmi_rx_reg_read32(HDMI_HBR_PACKET_VADDR) & 0x01) {
//              hdmi_rx_reg_write32(HDMI_HBR_PACKET_VADDR,0x06);
                SET_HDMI_HBR_MODE(1);
                //hdmi_rx_reg_write32(0x61, 0xd8); //for arrange HBR channel
                   HDMI_PRINTF("Only HBR exist\n");
                return TRUE;
            }
        }
#endif
   //    HDMI_PRINTF("Audio Sample miss\n");
    return FALSE;
}

void inline Hdmi_VideoOutputEnable(void)
{
    hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR,~_BIT3,_BIT3, HDMI_RX_MAC);
}

HDMI_BOOL Hdmi_Measure(void)
{
    // phoenix use auto polarity detect, so remove related function

    if (Hdmi_MeasureTiming(&hdmi.tx_timing, hdmi.b) == FALSE) {
                pr_info("Hdmi_MeasureTiming Error\n");
                return PHOENIX_MODE_NOSUPPORT;
    }

    return PHOENIX_MODE_SUCCESS;
}

unsigned char inline Hdmi_PacketSRAMRead(unsigned char addr)
{
    hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, addr, HDMI_RX_MAC);
    return hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC) ;
}

unsigned char Hdmi_WaitACR(void)
{

    char   timeout_cnt = 6;

    /*
        looking for ACR info using RSV1
    */

    hdmi_rx_reg_mask32(HDMI_HDMI_PTRSV1_VADDR, ~0xFF0000, 0x01<<16, HDMI_RX_MAC);     // Wait for ACR : Packet Type = 0x01
    hdmi_rx_reg_mask32(HDMI_HDMI_GPVS_VADDR, ~(_BIT7), _BIT7, HDMI_RX_MAC);  // Clear RSV3 indicator

    for (;timeout_cnt>0;timeout_cnt--)  { // Wait 30ms max. to wait ACR
        if (hdmi_rx_reg_read32(HDMI_HDMI_GPVS_VADDR, HDMI_RX_MAC) & _BIT7)
                if(Hdmi_PacketSRAMRead(140) == 0)  return TRUE;  // if ACR in BCH correct
         HDMI_DELAYMS(5);
    }
    HDMI_PRINTF("N&CTS counter= %d \n",timeout_cnt);
    // timeout
    return FALSE;
}

HDMI_COLOR_SPACE_T Hdmi_GetColorSpace(void)
{
    HDMIRX_IOCTL_STRUCT_T isr_info;
    HdmiGetStruct(&isr_info);
#if HDMI_DEBUG_COLOR_SPACE
//  HDMI_PRINTF("%s %x\n", __FUNCTION__, hdmi_rx_reg_read32(HDMI_HDMI_ACRCR_RO_VADDR));
#endif
#if SIMPLAY_TEST
    if (isr_info.avi_info_in == 0) {
        return ColorMap[0];
    }
#endif
    return ColorMap[HDMI_VCR_get_csc_r(hdmi_rx_reg_read32(HDMI_HDMI_VCR_VADDR, HDMI_RX_MAC))];
}


void Hdmi_Power(char enable) {
	HDMI_PRINTF("[HDMI RX][%s] %s\n",__FUNCTION__, enable?"ON":"OFF");

    if (enable)
    {
		hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~R_DFEPOW,R_DFEPOW,HDMI_RX_PHY);		//R DAPOW on
		hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~G_DFEPOW,G_DFEPOW,HDMI_RX_PHY);		//G DAPOW on
		hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~B_DFEPOW,B_DFEPOW,HDMI_RX_PHY);		//B DAPOW on

        hdmi_rx_reg_mask32(HDMIPHY_REG_CK_1_4, ~(HDMIPHY_REG_CK_1_4_CK_PLLPOW|HDMIPHY_REG_CK_1_4_CK_AFE_FORW_POW1),(HDMIPHY_REG_CK_1_4_CK_PLLPOW|HDMIPHY_REG_CK_1_4_CK_AFE_FORW_POW1), HDMI_RX_PHY);  //CK PLL&AFE

        hdmi_rx_reg_mask32(HDMI_TMDS_OUTCTL_VADDR,~(TMDS_OUTCTL_tbcoe_mask | TMDS_OUTCTL_tgcoe_mask | TMDS_OUTCTL_trcoe_mask | TMDS_OUTCTL_ocke_mask)
                    ,(TMDS_OUTCTL_tbcoe(1) | TMDS_OUTCTL_tgcoe(1) | TMDS_OUTCTL_trcoe(1) | TMDS_OUTCTL_ocke(1)), HDMI_RX_MAC);       //enable PLL TMDS, RGB clock output
             //cloud add for power request
        hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL, ~(HDMIPHY_REG_PHY_CTL_reg_ckcmp_mask),(HDMIPHY_REG_PHY_CTL_reg_ckcmp_mask),HDMI_RX_PHY); //CK PLL&AFE
        hdmi_rx_reg_mask32(REG_MHL_CTRL,~(REG_MHL_CTRL_reg_z300pow_mask|REG_MHL_CTRL_reg_z300_sel_0_mask|REG_MHL_CTRL_reg_z300_sel_1_mask|REG_MHL_CTRL_reg_z300_sel_2_mask),
                                            (REG_MHL_CTRL_reg_z300pow(0x07)|REG_MHL_CTRL_reg_z300_sel_2(1)|REG_MHL_CTRL_reg_z300_sel_1(1)|REG_MHL_CTRL_reg_z300_sel_0(1)), HDMI_RX_PHY);

        hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR,~(HDMI_AVMCR_aoc_mask|HDMI_AVMCR_ve_mask),(HDMI_AVMCR_aoc_mask|HDMI_AVMCR_ve_mask), HDMI_RX_MAC);  //audio & vedio output disable
        // cloud modify magellan skyworth bug 20130710
//        hdmi_rx_reg_mask32(HDMI_HDCP_CR_VADDR,~HDCP_CR_hdcp_en_mask,HDCP_CR_hdcp_en_mask, HDMI_RX_MAC);
        hdmi_related_wrapper_init();
    }
    else
    {
        hdmi_rx_reg_mask32(HDMI_HDMI_VPLLCR1_VADDR, ~HDMI_VPLLCR1_dpll_pwdn_mask,HDMI_VPLLCR1_dpll_pwdn_mask, HDMI_RX_MAC);// Disable video PLL   _BIT0
        hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_VADDR, ~HDMI_APLLCR1_dpll_pwdn_mask,HDMI_APLLCR1_dpll_pwdn_mask, HDMI_RX_MAC);// Disable audio PLL	  _BIT0
        hdmi_rx_reg_mask32( HDMIPHY_REG_CK_9_12, ~MHL_Z100K, MHL_Z100K, HDMI_RX_PHY); // turn off cbus 100k

        hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR, ~(HDMI_AVMCR_aoc_mask|HDMI_AVMCR_ve_mask), 0, HDMI_RX_MAC);  //audio & vedio output disable

        hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4, ~HDMIPHY_REG_R_1_4_R_EQPOW1, 0, HDMI_RX_PHY);	//R foreground K Off Set power off
        hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4, ~HDMIPHY_REG_G_1_4_G_EQPOW1, 0, HDMI_RX_PHY);	//G foreground K Off Set power off
        hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4, ~HDMIPHY_REG_B_1_4_B_EQPOW1, 0, HDMI_RX_PHY);	//B foreground K Off Set power off

        hdmi_rx_reg_mask32(REG_MHL_CTRL,~(REG_MHL_CTRL_reg_z300pow_mask|REG_MHL_CTRL_reg_z300_sel_0_mask|REG_MHL_CTRL_reg_z300_sel_1_mask|REG_MHL_CTRL_reg_z300_sel_2_mask),
            (REG_MHL_CTRL_reg_z300pow(0)|REG_MHL_CTRL_reg_z300_sel_2(1)|REG_MHL_CTRL_reg_z300_sel_1(1)|REG_MHL_CTRL_reg_z300_sel_0(1)), HDMI_RX_PHY);

        hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL, ~(HDMIPHY_REG_PHY_CTL_reg_ckcmp_mask), 0, HDMI_RX_PHY);	//CK PLL&AFE
        hdmi_rx_reg_mask32(HDMI_TMDS_OUTCTL_VADDR,~(TMDS_OUTCTL_tbcoe_mask | TMDS_OUTCTL_tgcoe_mask | TMDS_OUTCTL_trcoe_mask | TMDS_OUTCTL_ocke_mask)
					,(TMDS_OUTCTL_tbcoe(0) | TMDS_OUTCTL_tgcoe(0) | TMDS_OUTCTL_trcoe(0) | TMDS_OUTCTL_ocke(0)), HDMI_RX_MAC);		//disable PLL TMDS, RGB clock output

        hdmi_rx_reg_mask32(HDMIPHY_REG_CK_1_4, ~(HDMIPHY_REG_CK_1_4_CK_PLLPOW|HDMIPHY_REG_CK_1_4_CK_AFE_FORW_POW1), 0, HDMI_RX_PHY);	//CK PLL&AFE
        hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL_2, //Z0 resister
            ~(HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_ck_z0pow_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_b_z0pow_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_g_z0pow_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_r_z0pow_mask),
            (HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_ck_z0pow(0)|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_b_z0pow(0)|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_g_z0pow(0)|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_r_z0pow(0)), HDMI_RX_PHY);

    }
}

void inline Hdmi_HdcpPortWrite(unsigned char addr ,unsigned char value)
{
    hdmi_rx_reg_write32(HDMI_HDCP_AP_VADDR, addr, HDMI_RX_MAC);
    hdmi_rx_reg_write32(HDMI_HDCP_DP_VADDR, value, HDMI_RX_MAC);
}


void inline Hdmi_HdcpPortCWrite(unsigned char addr ,HDMI_CONST unsigned char* value ,unsigned char num )
{
    hdmi_rx_reg_mask32(HDMI_HDCP_PCR_VADDR, ~_BIT0, 0, HDMI_RX_MAC);
    hdmi_rx_reg_write32(HDMI_HDCP_AP_VADDR, addr, HDMI_RX_MAC);

    while(num--)
        hdmi_rx_reg_write32(HDMI_HDCP_DP_VADDR, *(value++), HDMI_RX_MAC);

    hdmi_rx_reg_mask32(HDMI_HDCP_PCR_VADDR, ~_BIT0, _BIT0, HDMI_RX_MAC);
}


/**
 * Hdmi_AudioOutputDisable
 * Disable HDMI Audio Ouput
 *
 * @param
 * @return
 * @ingroup drv_hdmi
 */
void Hdmi_AudioOutputDisable(void)
{
    //USER: jacklong DATE: 20100722   K-8256A  changing  hdmi timing ,tv will reboot
    if (HDMI_IS_AUDIO_SPDIF_PATH())
        hdmi_rx_reg_mask32(HDMI_HDMI_AOCR_VADDR,(~0x0ff),0x00, HDMI_RX_MAC); //Disable SPDIF/I2S Output

    hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR,~(_BIT6 | _BIT5), _BIT6, HDMI_RX_MAC);
    hdmi_rx_reg_write32(HDMI_HDMI_CMCR_VADDR,0x50, HDMI_RX_MAC);   //K code =2

    //pr_info("Audio Disable\n");
    //HDMI_DELAYMS(100);
    //HdmiAudioPLLSampleDump();
}

int Hdmi_AudioFreqCorrect(unsigned int freq, unsigned long b, HDMI_AUDIO_TRACK_MODE *track_mode)
{

    /*
        TO-DO : use ABS function
    */
    unsigned int b_ratio=1000;
    *track_mode = HDMI_AUDIO_N_CTS_TREND_BOUND;

    freq *= 10;
    if((freq >= (31700*b_ratio/100)) && (freq <= (32300*b_ratio/100)))
            freq = 32000;
    else if((freq >= (43500*b_ratio/100)) && (freq <= (44600*b_ratio/100)))
            freq = 44100;
    else if((freq >= 47500*b_ratio/100) && (freq <= (48500*b_ratio/100)))
            freq = 48000;
    else if((freq >= (87700*b_ratio/100)) && (freq <= (88700*b_ratio/100)))
            freq = 88200;
    else if((freq >= (95500*b_ratio/100)) && (freq <= (96500*b_ratio/100)))
            freq = 96000;
    else if((freq >= (175400*b_ratio/100)) && (freq <= (177400*b_ratio/100)))
            freq = 176400;
    else if((freq >= (191000*b_ratio/100)) && (freq <= (193000*b_ratio/100)))
            freq = 192000;
    else
         freq = 0;


    return freq;
}

void Hdmi_GetAudioFreq(HDMI_AUDIO_FREQ_T *freq, HDMI_AUDIO_TRACK_MODE *track_mode){

    unsigned long cts, n , b;
    unsigned char count=0;
    HDMIRX_IOCTL_STRUCT_T isr_info;

    HDMI_LOG("%s %d\n", "Hdmi_GetAudioFreq", __LINE__);

    freq->ACR_freq = 0;
    freq->AudioInfo_freq = 0;
    freq->SPDIF_freq = 0;

    /*
        Set trigger to get CTS&N and LPCM Channel Status Info
    */

    //Start Pop up N_CTS value
    hdmi_rx_reg_mask32(HDMI_HDMI_ACRCR_VADDR, ~(_BIT1|_BIT0),_BIT1|_BIT0, HDMI_RX_MAC);

    // Restart measure b
    hdmi_rx_reg_mask32(HDMI_HDMI_NTX1024TR0_VADDR,~ _BIT3,_BIT3, HDMI_RX_MAC);


    // Clear Info Frame update indicator
    hdmi_rx_reg_write32(HDMI_HDMI_ASR0_VADDR, 0x07, HDMI_RX_MAC);


    if (Hdmi_WaitACR() == FALSE) {
        HDMI_PRINTF( "No CTS & N Packet\n");
    };

    //HDMI_DELAYMS(25 * 6);
    for (count = 0; count < 150; count++) {
    //while(1) {

        HDMI_LOG("HDMI_HDMI_ACRCR_VADDR=%x\n",hdmi_rx_reg_read32(HDMI_HDMI_ACRCR_VADDR, HDMI_RX_MAC));

        HDMI_LOG("HDMI_HDMI_NTX1024TR0_VADDR=%x\n",hdmi_rx_reg_read32(HDMI_HDMI_NTX1024TR0_VADDR, HDMI_RX_MAC));

//      if (((hdmi_rx_reg_read32(HDMI_HDMI_ACRCR_VADDR) & (_BIT1|_BIT0)) == 0) && ((hdmi_rx_reg_read32(HDMI_HDMI_ASR0_VADDR) & 0x03) == 0x03))    //adams modify form 0x3 ot 0x1, 20120612,for STB LT5K no sound
        if (((hdmi_rx_reg_read32(HDMI_HDMI_ACRCR_VADDR, HDMI_RX_MAC) & (_BIT1|_BIT0)) == 0) && ((hdmi_rx_reg_read32(HDMI_HDMI_ASR0_VADDR, HDMI_RX_MAC) & 0x01) == 0x01))
            break;

        HDMI_DELAYMS(1);
    }
        if (count >=150)
        {
            HDMI_PRINTF("POP UP TIME OUT %x  %x \n",hdmi_rx_reg_read32(HDMI_HDMI_ACRCR_VADDR, HDMI_RX_MAC),hdmi_rx_reg_read32(HDMI_HDMI_ASR0_VADDR, HDMI_RX_MAC));
        }
        HDMI_PRINTF("POP UP TIME %d \n",count);

    // Delay 25ms
    //HDMI_DELAYMS(25);

//  HDMI_LOG("HDMI_HDMI_ACRCR_VADDR=%x\n",hdmi_rx_reg_read32(HDMI_HDMI_ACRCR_VADDR));

    //if (((hdmi_rx_reg_read32(HDMI_HDMI_ACRCR_VADDR) & (_BIT1|_BIT0)) != (_BIT1 | _BIT0)) && ((hdmi_rx_reg_read32(HDMI_HDMI_ASR0_VADDR) & 0x03) != 0x03)) return FALSE;

    // goto METHOD_AUDIO_INFO;
    //if ((hdmi_rx_reg_read32(HDMI_HDMI_ACRCR_VADDR) & (_BIT1|_BIT0)) != (_BIT1 | _BIT0)) goto METHOD_AUDIO_INFO;

    /*
            Get Audio Frequency from CTS&N
    */
    cts= HDMI_ACRSR0_get_cts(hdmi_rx_reg_read32(HDMI_HDMI_ACRSR0_VADDR, HDMI_RX_MAC));
    n =  HDMI_ACRSR1_get_n(hdmi_rx_reg_read32(HDMI_HDMI_ACRSR1_VADDR, HDMI_RX_MAC));

    ACR_N = n;
    HdmiGetStruct(&isr_info);
    b = isr_info.b;

    HDMI_PRINTF("cts=%ld\nn=%ld\nb=%ld\n",cts, n, b);

    if(cts==0 || n==0 || b == 0)  goto METHOD_AUDIO_INFO;

    // 128fs = 1024/b * fx * N / CTS  =>  fs = (1024 * fx *N)/(128 * b * CTS) = (8 * fx *N)/(b*CTS)
    // calculate freq in 0.1kHz unit
    // freq = (unsigned long)8 * 2 * 10000 * HDMI_RTD_XTAL/ cts * n / ((unsigned long)b * 1000);
#if HDMI_OLD_CLK_DETECT
    freq->ACR_freq = (8 * 2 * 27000 *10/b)*n/cts;
    freq->ACR_freq  = (freq->ACR_freq >> 1) + (freq->ACR_freq & 0x01);  //四捨五入//
    freq->ACR_freq *= 100;
#else
    freq->ACR_freq = ((((270000 * b)/256)/128) * n) / (cts);
    freq->ACR_freq *= 100;
#endif
    freq->ACR_freq = Hdmi_AudioFreqCorrect(freq->ACR_freq, b, track_mode);

    /*
        Get Audio Frequency from Audio Info Frame
    */
    METHOD_AUDIO_INFO:

//  if ((hdmi_rx_reg_read32(HDMI_HDMI_ASR0_VADDR) & 0x03) == 0x03)  {      //adams modify form 0x3 ot 0x1, 20120612,for STB LT5K no sound
    if ((hdmi_rx_reg_read32(HDMI_HDMI_ASR0_VADDR, HDMI_RX_MAC) & 0x01) == 0x01)  {

        freq->SPDIF_freq = AUDIO_CHANNEL_STATUS[hdmi_rx_reg_read32(HDMI_HDMI_ASR1_VADDR, HDMI_RX_MAC)&0xf];

    }

    HDMI_PRINTF("\n *************** SPDIF freq=%ld\n", freq->SPDIF_freq);

}

HDMI_BOOL Hdmi_AudioModeDetect(void)
{

    unsigned char result = FALSE;
    unsigned int d_code;
//  unsigned char value1, value2;
    int i;
    HDMI_AUDIO_FREQ_T t, t2;
    HDMI_AUDIO_TRACK_MODE track_mode;

    if (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & _BIT6){
               HDMI_PRINTF("Audio Detect AV Mute\n");
            Hdmi_AudioOutputDisable();
            SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            return FALSE;
    }

    switch(GET_HDMI_AUDIO_FSM()) {
        case AUDIO_FSM_AUDIO_START:
        {
            //HDMI_PRINTF("AUDIO_FSM_AUDIO_START\n");

            SET_HDMI_AUDIO_TYPE(HDMI_AUDIO_PCM);
// FIXME
//          hdmi_rx_reg_write32(HDMI_HDMI_AFSM_MOD,0x00);
//          hdmi_rx_reg_write32(HDMI_HDMI_AUDIO_LD_P_TIME_M,0x00);
//          hdmi_rx_reg_write32(HDMI_HDMI_AUDIO_LD_P_TIME_M1,0x00);
//          hdmi_rx_reg_write32(0x0C,0x06);
//          hdmi_rx_reg_write32(0x07,0x0b);
//          hdmi_rx_reg_write32(HDMI_HDMI_AUDIO_FREQDET,0x80);
//          hdmi_rx_reg_write32(0xb800d3dc,0x04);  //disable HBR packet
            SET_HDMI_HBR_MODE(0);
            //clear Overflow,Underflow,phase_non_lock, auto load double buffter
            hdmi_rx_reg_write32(HDMI_HDMI_DBCR_VADDR , 0x00, HDMI_RX_MAC);
            /*Disable audio watch dog*/
            hdmi_rx_reg_mask32(HDMI_HDMI_WDCR0_VADDR ,(~_BIT15), 0x00, HDMI_RX_MAC);  //disable  tmds clock audio watch dog
            hdmi_rx_reg_mask32(HDMI_HDMI_WDCR0_VADDR , ~(_BIT1 |_BIT2|_BIT3 | _BIT4 ),0x00, HDMI_RX_MAC);
            Hdmi_AudioOutputDisable();

            /*Disable FIFO trend tracking*/
            hdmi_rx_reg_write32(HDMI_HDMI_PSCR_VADDR,0xE2, HDMI_RX_MAC);
            /*Update Double Buffer*/
            hdmi_rx_reg_write32(HDMI_HDMI_CMCR_VADDR,0x50, HDMI_RX_MAC);   //K code =2
            if(Hdmi_WaitAudioSample() == FALSE)
                break;
            if (HDMI_AUDIO_IS_LPCM() || HDMI_AUDIO_SUPPORT_NON_PCM()) {

            } else {
//              break;
            }
#if HDMI_DEBUG_AUDIO_PLL_RING
            HDMI_DELAYMS(1000);
            HdmiAudioPLLSampleDump();
#endif
            SET_HDMI_AUDIO_FSM(AUDIO_FSM_FREQ_DETECT);
        }
//      break;
        case AUDIO_FSM_FREQ_DETECT:
        {
            HDMI_PRINTF("AUDIO_FSM_FREQ_DETECT\n");
            if (HDMI_AUDIO_IS_LPCM() == 0) {
                HDMI_PRINTF("HDMI NON-PCM Audio\n");
                SET_HDMI_AUDIO_TYPE(HDMI_AUDIO_NPCM);
            }
            if (HDMI_AUDIO_IS_LPCM() || HDMI_AUDIO_SUPPORT_NON_PCM()) {
                    Hdmi_GetAudioFreq(&t, &track_mode);
                    /*detect HDMI audio freq twice for stable freq
                    USER:alanli DATE:2010/04/06*/
                    Hdmi_GetAudioFreq(&t2, &track_mode);
                    //HDMI_PRINTF("Hdmi_GetAudioFreq t=%d\n",t.ACR_freq);
                    //HDMI_PRINTF("Hdmi_GetAudioFreq t2=%d\n",t2.ACR_freq);
                    if ((t.ACR_freq != 0 )&& (t.ACR_freq==t2.ACR_freq)) {
                        if (HDMI_Audio_Conut == 0){
                            HDMI_Audio_Conut = 1;
                            if (Hdmi_AudioPLLSetting(t.ACR_freq, track_mode) == TRUE) {
                                SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_WAIT_PLL_READY);
                                hdmi.tx_timing.spdif_freq = t.SPDIF_freq;
                                break;
                                //HDMI_DELAYMS(100);
                            }
                        }
                        else{//HDMI_Audio_Conut  = 1, force to trend_boundary tracking
                            HDMI_Audio_Conut = 0;
                            if (Hdmi_AudioPLLSetting(t.ACR_freq, HDMI_AUDIO_TREND_BOUND) == TRUE) {
                                SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_WAIT_PLL_READY);
                                hdmi.tx_timing.spdif_freq = t.SPDIF_freq;
                                break;
                                //HDMI_DELAYMS(100);
                            }
                        }
                    } else {
                        if ((t.ACR_freq == 0)||(t2.ACR_freq == 0)){//cts = 0,use t.SPDIF_freq and force to trend_boundary tracking
                            if (Hdmi_AudioPLLSetting(t.SPDIF_freq, HDMI_AUDIO_TREND_BOUND) == TRUE) {
                                if (t.SPDIF_freq == t2.SPDIF_freq){
                                    SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_WAIT_PLL_READY);
                                    hdmi.tx_timing.spdif_freq = t.SPDIF_freq;
                                    break;
                                    //HDMI_DELAYMS(100);
                                }
                            }
                        }
                    }
            }
            //SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            break;
        }
        case AUDIO_FSM_AUDIO_WAIT_PLL_READY:
            HDMI_PRINTF("AUDIO_FSM_AUDIO_WAIT_PLL_READY\n");

            for (i = 0; i < 5; i++) {
                hdmi_rx_reg_write32(HDMI_HDMI_NCPER_VADDR,0xff, HDMI_RX_MAC);
                HDMI_DELAYMS(20);
                if ((hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & (_BIT3|_BIT2|_BIT1))==0)
                    break;
                hdmi_rx_reg_mask32(HDMI_HDMI_SR_VADDR,~(_BIT3|_BIT2|_BIT1),(_BIT3|_BIT2|_BIT1), HDMI_RX_MAC);
            }

            HDMI_PRINTF( "FIFO timeout count2= %d\n",i);
            if (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & (_BIT1|_BIT2|_BIT3)){
                HDMI_PRINTF("Audio PLL not ready = %x\n",hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC));
                SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
                return FALSE;
            }
            SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START_OUT);

        //break;
        // Play audio here
        case AUDIO_FSM_AUDIO_START_OUT:
        {
            HDMI_PRINTF("AUDIO_FSM_AUDIO_START_OUT\n");
            hdmi_rx_reg_mask32(HDMI_HDMI_DBCR_VADDR, 0xF0, 0x0F, HDMI_RX_MAC);
// FIXME:
            d_code = hdmi_rx_reg_read32(HDMI_HDMI_APLLCR3_VADDR, HDMI_RX_MAC);

            for (i=0; i<5; i++) {
                if (d_code == hdmi_rx_reg_read32(HDMI_HDMI_APLLCR3_VADDR, HDMI_RX_MAC)) break;
            }

            hdmi_rx_reg_write32(HDMI_HDMI_DCAPR0_VADDR, d_code, HDMI_RX_MAC);   //pre-set D code


            hdmi_rx_reg_write32(HDMI_HDMI_PSCR_VADDR,0xE2, HDMI_RX_MAC);       //pre-disable N/CTS tracking & FIFO depth
#if HDMI_DEBUG_AUDIO_PLL_RING
            HDMI_DELAYMS(1000);
            HdmiAudioPLLSampleDump();
#endif
            //Enable audio Overflow & Underflow watch dog but not Audio type wdg
            hdmi_rx_reg_mask32(HDMI_HDMI_WDCR0_VADDR ,(~(_BIT1|_BIT2|_BIT3 | _BIT4)), _BIT1|_BIT2|_BIT3 | _BIT4, HDMI_RX_MAC);
            hdmi_rx_reg_mask32(HDMI_HDMI_WDCR0_VADDR , (~_BIT15),_BIT15, HDMI_RX_MAC);//Enable audio tmds clock  watch dog
            hdmi_rx_reg_mask32(HDMI_HDMI_AOCR_VADDR,(unsigned char)(~0x0ff),0xFF, HDMI_RX_MAC);  //Enable SPDIF/I2S Output
            SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_CHECK);
            result = TRUE;
        }
        break;

        case AUDIO_FSM_AUDIO_CHECK:
        {
            HDMI_PRINTF("AUDIO_FSM_AUDIO_CHECK\n");
            HDMI_Audio_Conut = 0;
            #if 0
            HDMI_PRINTF("FIFO@DE2 falling = %x\n", hdmi_rx_reg_read32(HDMI_HDMI_FDDF_VADDR));
            #endif
            // if FIFO overflow then restart Audio process
#if 1
            if (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & (_BIT3)) {
                HDMI_PRINTF( "Audio Output Disable cause by pll unlock :%x\n",hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC));
                Hdmi_AudioOutputDisable();
                SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            }
            if(hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & (_BIT1|_BIT2)) {
                HDMI_PRINTF( "Audio Output Disable cause by over_underflow :%x\n",hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC));
                Hdmi_AudioOutputDisable();
                SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            }

            if ((hdmi_rx_reg_read32(HDMI_HDMI_AVMCR_VADDR, HDMI_RX_MAC) & (_BIT5)) == 0) {
                HDMI_PRINTF( "Audio Output Disable cause by AVMCR output disable:%x\n",hdmi_rx_reg_read32(HDMI_HDMI_AVMCR_VADDR, HDMI_RX_MAC));
                Hdmi_AudioOutputDisable();
                SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            }

#endif
            if ((HDMI_AUDIO_IS_LPCM() == 0) && (HDMI_AUDIO_SUPPORT_NON_PCM()==0)) {     // if TX change audio mode to non-LPCM
                Hdmi_AudioOutputDisable();
                HDMI_PRINTF( "Audio Output Disable cause non-Linear PCM\n");
                SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            }
            if ( ((HDMI_AUDIO_IS_LPCM()) && GET_HDMI_AUDIO_TYPE()) || ((HDMI_AUDIO_IS_LPCM() == 0) && (GET_HDMI_AUDIO_TYPE()==0))) {
                Hdmi_AudioOutputDisable();
                HDMI_PRINTF( "Audio Type change \n");
                SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            }
#if 0
            if ((hdmi_rx_reg_read32(HDMI_HDMI_AFCR_VADDR, HDMI_RX_MAC) & 0x700)!=0x0600) {
                HDMI_PRINTF("Audio FIFO sfail %x\n", hdmi_rx_reg_read32(HDMI_HDMI_AFCR_VADDR));
                Hdmi_AudioOutputDisable();
            //  HDMI_DELAYMS(10000);
                SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            }
#endif

        }
        break;

        default:
        {
                SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
        }
        break;

    }

    return result;
}


void drvif_Hdmi_LoadEDID(unsigned char channel_index, unsigned char *EDID, int len)
{
    char edid_type;

    //if (channel_index >= HDMI_MAX_CHANNEL)  {
    //    HDMI_PRINTF("Wrong Channel Number: %d\n", channel_index);
    //    return;
    //}
    if(channel_index >= DDC_Max_Channel)
    {
      pr_warn("[HDMI Rx][%s:%d] Return because of Channel: %d\n", __FUNCTION__, __LINE__, channel_index);
      return;
    }

    edid_type = 0;//(char)hdmi.channel[channel_index]->edid_type;
    switch (edid_type) {
            case HDMI_EDID_ONCHIP:
                #if 0
                drvif_EDIDLoad(hdmi.channel[channel_index]->ddc_selected, EDID, len);
                drvif_EDIDEnable(hdmi.channel[channel_index]->ddc_selected, 1);
                drvif_EDID_DDC12_AUTO_Enable(hdmi.channel[channel_index]->ddc_selected, 0);
                #else
                drvif_EDIDLoad((DDC_NUMBER_T)channel_index, EDID, len);
                drvif_EDIDEnable((DDC_NUMBER_T)channel_index, 1);
                drvif_EDID_DDC12_AUTO_Enable((DDC_NUMBER_T)channel_index, 0);
                #endif
            break;
            case HDMI_EDID_I2CMUX:
                HdmiMuxEDIDLoad(hdmi.channel[channel_index]->ddc_selected, EDID, len);
                HdmiMuxEDIDEnable(hdmi.channel[channel_index]->mux_ddc_port, 1);
            break;
            case HDMI_EDID_EEPROM:
            break;

    }


}

unsigned char Hdmi_AudioPLLSetting(int freq, HDMI_AUDIO_TRACK_MODE track_mode) {

    UINT8 coeff = 0, rate = 0;
    //UINT8 i;
    unsigned char i;
    int timeout = 10;
    UINT32 tmp1;
    UINT32 I_Code=0,S=0;
    //UINT32 b;//cts,n,b;
    //HDMIRX_IOCTL_STRUCT_T isr_info;
    HDMI_LOG( "HDMI_HDMI_AVMCR_VADDR = %x\n", hdmi_rx_reg_read32(HDMI_HDMI_AVMCR_VADDR));


    for (i=0; i < 7; i++) {
        if (audio_pll_coeff[i].freq == freq)  {
            coeff = audio_pll_coeff[i].coeff;
            rate = audio_pll_coeff[i].rate;
            goto PLL_SETTING;
        }
    }
    HDMI_PRINTF( "Unsupport audio freq = %d\n", freq);
    return FALSE;

    PLL_SETTING:
/*                      //adams mask 20120612,for STB LT5K no sound
    if (ACR_N == 0) {
        HDMI_PRINTF( "ACR_N = 0\n");
        return FALSE;
    }
*/
    /*(2)audio output enable = auto mode*/
    hdmi_rx_reg_mask32(HDMI_HDMI_AFCR_VADDR,~_BIT6,_BIT6, HDMI_RX_MAC);
    /*(3)Disable trend and boundary tracking*/
    hdmi_rx_reg_mask32(HDMI_HDMI_WDCR0_VADDR,~HDMI_WDCR0_bt_track_en_mask,0x0, HDMI_RX_MAC);//<2>Disable trend and boundary tracking
    hdmi_rx_reg_mask32(HDMI_HDMI_PSCR_VADDR, ~(_BIT3 |_BIT2),0x00, HDMI_RX_MAC);//<1>Disable trend and boundary tracking
    hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4 , _BIT4, HDMI_RX_MAC);//Update Double Buffer
    /*(4)FSM back to Pre-mode*/
    hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR, ~_BIT5,0x00, HDMI_RX_MAC);
    /*(5)Disable N/CTS tracking*/
    hdmi_rx_reg_mask32(AUDIO_CTS_UP_BOUND,~_BIT20,0x0, HDMI_RX_MAC);      //CTS has glitch not to tracking disable
    hdmi_rx_reg_mask32(AUDIO_N_UP_BOUND,~_BIT20,0x0, HDMI_RX_MAC);            //N has glitch not to tracking disable
    hdmi_rx_reg_mask32(HDMI_HDMI_PSCR_VADDR, ~_BIT4,0x00, HDMI_RX_MAC);
    hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4 , _BIT4, HDMI_RX_MAC);//Update Double Buffer
    /*(6)Disable SDM*/
    hdmi_rx_reg_mask32(HDMI_HDMI_AAPNR_VADDR, ~_BIT1,0x0, HDMI_RX_MAC);
    /*(7)Disable PLL*/
    //hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_VADDR,  ~( _BIT2 | _BIT0| _BIT13| _BIT12| _BIT11), (_BIT2 | _BIT0)); // Disable PLL
    // cloud modify magellan remove bit 16 ~bit10  , need check
    hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_VADDR,  ~( HDMI_APLLCR1_dpll_freeze_mask | HDMI_APLLCR1_dpll_pwdn_mask), (HDMI_APLLCR1_dpll_freeze_mask | HDMI_APLLCR1_dpll_pwdn_mask), HDMI_RX_MAC); // Disable PLL
    /*(8)resetS &S1 code to avoid dead lock*/
    hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,  ~( _BIT6|_BIT5), 0x0, HDMI_RX_MAC); // PLL output clk sel from crystal
    hdmi_rx_reg_write32(HDMI_HDMI_SCAPR_VADDR,0x00, HDMI_RX_MAC);  //S1 & S2 code clear to 0 , to avoid dead lock
    hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4 , _BIT4, HDMI_RX_MAC);//Update Double Buffer
    HDMI_DELAYMS(1);
    hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,  ~( _BIT6|_BIT5), (_BIT6), HDMI_RX_MAC); // PLL output clk sel from VCO
    hdmi_rx_reg_write32(HDMI_HDMI_CMCR_VADDR,  0x50, HDMI_RX_MAC); //Enable Double Buffer
    /*(9)D code*/
    hdmi_rx_reg_write32(HDMI_HDMI_DCAPR0_VADDR, (hdmi_audiopll_param[i].D_HighByte << 8) | hdmi_audiopll_param[i].D_LowByte, HDMI_RX_MAC);
    hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~(HDMI_CMCR_dbdcb_mask),HDMI_CMCR_dbdcb(1), HDMI_RX_MAC);                //Enable Double Buffer for K/M/S/D/O

    /*(10)Initial PLL*/
    hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR0_VADDR, ~(HDMI_APLLCR0_dpll_m_mask |HDMI_APLLCR0_dpll_o_mask |HDMI_APLLCR0_dpll_n_mask),
                                    HDMI_APLLCR0_dpll_m(hdmi_audiopll_param[i].M - 2) | HDMI_APLLCR0_dpll_o(hdmi_audiopll_param[i].O), HDMI_RX_MAC);
    hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~(HDMI_CMCR_dbdcb_mask),HDMI_CMCR_dbdcb(1), HDMI_RX_MAC);                //Enable Double Buffer for K/M/S/D/O

    if (hdmi_audiopll_param[i].N < 2) {
        hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR0_VADDR, ~(HDMI_APLLCR0_dpll_bpn_mask | HDMI_APLLCR0_dpll_n_mask),   HDMI_APLLCR0_dpll_bpn(1) |  HDMI_APLLCR0_dpll_n(0), HDMI_RX_MAC);        //set audio PLL N code
    } else {
        hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR0_VADDR, ~(HDMI_APLLCR0_dpll_bpn_mask | HDMI_APLLCR0_dpll_n_mask),
                                    HDMI_APLLCR0_dpll_bpn(0) |  HDMI_APLLCR0_dpll_n(hdmi_audiopll_param[i].N-2), HDMI_RX_MAC);       //set audio PLL N code
    }
    hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~(HDMI_CMCR_dbdcb_mask),HDMI_CMCR_dbdcb(1), HDMI_RX_MAC);                //Enable Double Buffer for K/M/S/D/O
    hdmi_rx_reg_write32(HDMI_HDMI_SCAPR_VADDR,  (hdmi_audiopll_param[i].S1) ? ((hdmi_audiopll_param[i].S / 2) | 0x80) : (hdmi_audiopll_param[i].S / 2), HDMI_RX_MAC);
    hdmi_rx_reg_write32(HDMI_PRESET_S_CODE1_VADDR, 0xf800, HDMI_RX_MAC);       //S1 code
    hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR0_VADDR,~(_BIT2|_BIT1|_BIT0), 0x3, HDMI_RX_MAC);      //set Icp
    hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4,_BIT4, HDMI_RX_MAC);              //Enable Double Buffer for K/M/S/D/O
    hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR0_VADDR,~(_BIT5|_BIT4|_BIT3),(_BIT4| _BIT3) , HDMI_RX_MAC);   //set RS=13k
    hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4,_BIT4, HDMI_RX_MAC);              //Enable Double Buffer for K/M/S/D/O

    //hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_VADDR,~(_BIT18|_BIT17),(_BIT18|_BIT17) );            //set CS=42pf
    hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_VADDR,~(HDMI_APLLCR1_dpll_CS_MASK),(HDMI_APLLCR1_dpll_CS_66P) , HDMI_RX_MAC);           //set CS=42pf
    //hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_ADDR,~_BIT1,_BIT1);           // Enable divider K and enable VCOSTART
    hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4,_BIT4, HDMI_RX_MAC);              //Enable Double Buffer for K/M/S/D/O
    //hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_VADDR,~(_BIT3 | _BIT1 |_BIT0), (_BIT3 | _BIT1));//Enable PLL
    hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_VADDR,~(HDMI_APLLCR1_dpll_stop_mask | HDMI_APLLCR1_dpll_vcorstb_mask |HDMI_APLLCR1_dpll_pwdn_mask), (HDMI_APLLCR1_dpll_stop_mask | HDMI_APLLCR1_dpll_vcorstb_mask), HDMI_RX_MAC);//Enable PLL
    //PLL Calibration
    //hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_VADDR,  ~(_BIT15|_BIT16), _BIT16); // Set VCO default    //cloud mark for magellan 2013 0110
       //HDMI_DELAYMS(1);
      // hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_VADDR, ~_BIT11, _BIT11);         // Reg DPLL_CMPEN       //cloud mark for magellan 2013 0110
      // HDMI_DELAYMS(1);
      // hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_VADDR, ~_BIT12, _BIT12);         // Reg DPLL_CALLCH      //cloud mark for magellan 2013 0110
      // HDMI_DELAYMS(1);
      // hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_VADDR, ~_BIT13, _BIT13);         // Reg DPLL_CALSW      //cloud mark for magellan 2013 0110
    HDMI_PRINTF(" m = %x\n o = %x\n s = %x\n ", hdmi_audiopll_param[i].M, hdmi_audiopll_param[i].O, hdmi_audiopll_param[i].S);
       //Wait PLL Stable
       HDMI_DELAYMS(1);
    //PLL un-freeze
    //hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_VADDR,~_BIT2, 0x0); //DPLL normal->clk really output to fifo for read
    hdmi_rx_reg_mask32(HDMI_HDMI_APLLCR1_VADDR,~HDMI_APLLCR1_dpll_freeze_mask,0x0, HDMI_RX_MAC);//
    /*(11)Enable SDM*/
    //hdmi_rx_reg_mask32(HDMI_HDMI_AAPNR_VADDR, ~_BIT1,_BIT1);
    hdmi_rx_reg_mask32(HDMI_HDMI_AAPNR_VADDR, ~HDMI_AAPNR_esdm_mask,HDMI_AAPNR_esdm_mask, HDMI_RX_MAC);
    HDMI_DELAYMS(1);

    if(track_mode == HDMI_AUDIO_N_CTS_TREND_BOUND)
    {
        HDMI_PRINTF("\n *****N/CTS Trend& Boundary Tracking*****\n");
    /*(12)Enable N/CTS tracking*/
        /*Modify N/CTS tracking parameter  USER:kistlin DATE:2011/08/04*/
        //for phase error count source Fpec = Fdds = Fvco/4
        //PEpec x Tpec = delta(Tvco)xNxSxPLLO = Tvco(1/8)(D[15:0]/2^16)xNxSxPLLO
        //D[15:0] = PEpec x Tpec /[Tvco(1/8)(1//2^16)xNxSxPLLO]
        //and D[15:0] = PEpec x (1/8)Icode
        //Icode calculate I code =2^24/(N*S*PLLO)

        //for phase error count source = fvco/4,fdds
        //Icode calculate I code =2*2048*2^10/(N*S*PLLO)-->x2 才夠力 20110701 kist
            if (hdmi_audiopll_param[i].S1)
                S = hdmi_audiopll_param[i].S*2;
            else
                S = hdmi_audiopll_param[i].S;
            HDMI_PRINTF("S = %d , ACR_N=%d ,    hdmi_audiopll_param[i].O = %d\n",S, ACR_N, hdmi_audiopll_param[i].O );
            if (ACR_N)
                {
                I_Code =16*1024*1024/(ACR_N*S*(hdmi_audiopll_param[i].O<<1));
                I_Code = I_Code;
                }
            else
                I_Code = 0x02;

        //calculate 4*N*(1/128fa) or 4*CTS*Tv, 4x for delay (HDMI_DELAYMS(1) 約為300us)
        tmp1 = 4*ACR_N*1000/(128*freq);
        if (tmp1 < 5)
            tmp1 = tmp1;//CTS*Tv < tmp1/4 > 2* CTS*Tv,  CTS*Tv(0.67ms~3.3ms)
        else if (tmp1 < 9)
            tmp1 = tmp1+2;
        else
            tmp1 = tmp1+3;

        //HDMI_PRINTF( "I Code = %d\n",I_Code);
        //HDMI_PRINTF( "tmp1 = %d\n",tmp1);

        hdmi_rx_reg_write32(HDMI_HDMI_ICPSNCR0_VADDR,I_Code, HDMI_RX_MAC);     //Set I code of Ncts[15:8]
        hdmi_rx_reg_write32(HDMI_HDMI_PCPSNCR0_VADDR,0x0000, HDMI_RX_MAC); //Set P code of Ncts [15:8]
    //  hdmi_rx_reg_mask32(HDMI_NPECR,~_BIT30,_BIT30);   //N_CTS tracking re-enable toggle function enable
          hdmi_rx_reg_mask32(HDMI_NPECR,~HDMI_NPECR_ncts_re_enable_off_en_mask,HDMI_NPECR_ncts_re_enable_off_en_mask, HDMI_RX_MAC);
        hdmi_rx_reg_write32(HDMI_HDMI_PSCR_VADDR,0x92, HDMI_RX_MAC);   //Enable N_CTS tracking & set FIFO depth
        hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4 , _BIT4, HDMI_RX_MAC);    //Update Double Buffer
#if 0
        if (tmp1 > 9)
            {
            HDMI_DELAYMS(tmp1-9);// HDMI_DELAYMS 要小於10 才行,所以做兩次,因為兩個的單位不大一樣
            HDMI_DELAYMS(9);
            }
        else
            HDMI_DELAYMS(tmp1);
#endif
        hdmi_rx_reg_write32(HDMI_RHDMI_PETR_VADDR,0x1e, HDMI_RX_MAC);//phase error threshold
             for (timeout = 0; timeout < 25; timeout++) {
            hdmi_rx_reg_mask32(HDMI_HDMI_SR_VADDR,~_BIT3,_BIT3, HDMI_RX_MAC);
            hdmi_rx_reg_write32(HDMI_HDMI_NCPER_VADDR,0xff, HDMI_RX_MAC);
            HDMI_DELAYMS(20);
            if ((hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & (_BIT3)) == 0)
                break;
        }

        if (timeout == 25)
            HDMI_PRINTF( "PLL 1st check not lock = %x\n",hdmi_rx_reg_read32(HDMI_HDMI_NCPER_VADDR, HDMI_RX_MAC));
        else
            HDMI_PRINTF( "PLL 1st check lock count = %d\n",timeout);


        //hdmi_rx_reg_mask32(HDMI_HDMI_PSCR_VADDR,~_BIT4,0);    //disable N_CTS tracking
        hdmi_rx_reg_mask32(HDMI_HDMI_PSCR_VADDR,~HDMI_PSCR_etcn_mask,0, HDMI_RX_MAC);
        //hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4 , _BIT4);  //Update Double Buffer
        hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~HDMI_CMCR_dbdcb_mask , HDMI_CMCR_dbdcb_mask, HDMI_RX_MAC);  //Update Double Buffer
        //hdmi_rx_reg_mask32(HDMI_NPECR,~_BIT30,0);  //N_CTS tracking re-enable toggle function disable
          hdmi_rx_reg_mask32(HDMI_NPECR,~HDMI_NPECR_ncts_re_enable_off_en_mask,0, HDMI_RX_MAC);
        //hdmi_rx_reg_write32(HDMI_HDMI_ICPSNCR0_VADDR,0x0002);     //Set I code of Ncts[15:8]
        hdmi_rx_reg_write32(HDMI_HDMI_ICPSNCR0_VADDR,(HDMI_ICPSNCR0_icl(0x02)|HDMI_ICPSNCR0_ich(0)), HDMI_RX_MAC);     //Set I code of Ncts[15:8]
        //hdmi_rx_reg_write32(HDMI_HDMI_PCPSNCR0_VADDR,0x2000);   //Set P code of Ncts [15:8]
        hdmi_rx_reg_write32(HDMI_HDMI_PCPSNCR0_VADDR,(HDMI_PCPSNCR0_pcl(0x0)|HDMI_PCPSNCR0_pch(0x20)), HDMI_RX_MAC);   //Set P code of Ncts [15:8]
          //hdmi_rx_reg_mask32(HDMI_HDMI_PSCR_VADDR,~_BIT4,_BIT4);  //enable N_CTS tracking
        hdmi_rx_reg_mask32(HDMI_HDMI_PSCR_VADDR,~HDMI_PSCR_etcn_mask,HDMI_PSCR_etcn_mask, HDMI_RX_MAC);
        //hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4 , _BIT4);  //Update Double Buffer
        hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~HDMI_CMCR_dbdcb_mask,HDMI_CMCR_dbdcb_mask, HDMI_RX_MAC);
        //N&CTS boundary set
        //hdmi_rx_reg_mask32(AUDIO_CTS_UP_BOUND,~0xfffff,0x6ddd0);       //CTS up boundary set 450000
        hdmi_rx_reg_mask32(AUDIO_CTS_UP_BOUND,~(AUDIO_CTS_UP_BOUND_cts_up_bound_mask),AUDIO_CTS_UP_BOUND_cts_up_bound(450000), HDMI_RX_MAC);      //CTS up boundary set 450000
        hdmi_rx_reg_mask32(AUDIO_CTS_LOW_BOUND,~0xfffff,0x4e20, HDMI_RX_MAC);     //CTS low boundary set 20000
        hdmi_rx_reg_mask32(AUDIO_N_UP_BOUND,~0xfffff,0x13880, HDMI_RX_MAC);       //N up boundary set 80000
        hdmi_rx_reg_mask32(AUDIO_N_LOW_BOUND,~0xfffff,0x7d0, HDMI_RX_MAC);        //N low boundary set 2000
        hdmi_rx_reg_mask32(AUDIO_CTS_UP_BOUND,~_BIT20,_BIT20, HDMI_RX_MAC);       //CTS has glitch not to tracking enable
        hdmi_rx_reg_mask32(AUDIO_N_UP_BOUND,~_BIT20,_BIT20, HDMI_RX_MAC);         //N has glitch not to tracking enable

        /*(13)Wait PLL lock by N&CTS tracking*/
        hdmi_rx_reg_write32(HDMI_RHDMI_PETR_VADDR,0x1e, HDMI_RX_MAC);//phase error threshold
             for (timeout = 0; timeout < 25; timeout++) {
            hdmi_rx_reg_mask32(HDMI_HDMI_SR_VADDR,~_BIT3,_BIT3, HDMI_RX_MAC);
            hdmi_rx_reg_write32(HDMI_HDMI_NCPER_VADDR,0xff, HDMI_RX_MAC);
            HDMI_DELAYMS(20);
            if ((hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & (_BIT3)) == 0)
                break;
        }

        if (timeout == 25)
            HDMI_PRINTF( "PLL not lock = %x\n",hdmi_rx_reg_read32(HDMI_HDMI_NCPER_VADDR, HDMI_RX_MAC));
        else
            HDMI_PRINTF( "PLL lock count = %d\n",timeout);

        /*(14)FSM Initial*/
        //hdmi_rx_reg_write32(HDMI_HDMI_FBR_VADDR,0x77);//Target FIFO depth = 14 ,Boundary address distance = 7
        hdmi_rx_reg_write32(HDMI_HDMI_FBR_VADDR,0x74, HDMI_RX_MAC);
        //hdmi_rx_reg_write32(HDMI_HDMI_FTR_VADDR,0x03);//target times for summation of one trend to decide the trend
        hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR,~_BIT6,_BIT6, HDMI_RX_MAC);//FSM entry Pre-mode (AOC=1)
        hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR,~_BIT5,_BIT5, HDMI_RX_MAC);//FSM entry next step (AOMC=1)
        HDMI_DELAYMS(1);//wait fifo to target fifo level

        /*(15)Enable trend and boundary tracking*/
        hdmi_rx_reg_write32(HDMI_HDMI_ICTPSR0_VADDR,0x0005, HDMI_RX_MAC);          //Set I code  of trend [15:8]
        hdmi_rx_reg_write32(HDMI_HDMI_PCTPSR0_VADDR,0x02FF, HDMI_RX_MAC);  //Set P code of trend [15:8]
        hdmi_rx_reg_write32(HDMI_HDMI_ICBPSR0_VADDR,0x0005, HDMI_RX_MAC);      //Set I code of bnd [15:8]
        hdmi_rx_reg_write32(HDMI_HDMI_PCBPSR0_VADDR,0x02FF, HDMI_RX_MAC);  //Set P code of bnd [15:8]
        hdmi_rx_reg_write32(HDMI_HDMI_STBPR_VADDR,0x01, HDMI_RX_MAC);      //Set Boundary Tracking Update Response Time
        hdmi_rx_reg_mask32(HDMI_HDMI_PSCR_VADDR, ~(_BIT3|_BIT2), (_BIT3|_BIT2), HDMI_RX_MAC);//<1>Enable trend and boundary tracking
        hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4 , _BIT4, HDMI_RX_MAC);//Update Double Buffer
        HDMI_DELAYMS(20);
        hdmi_rx_reg_mask32(HDMI_HDMI_WDCR0_VADDR,~HDMI_WDCR0_bt_track_en_mask,HDMI_WDCR0_bt_track_en_mask, HDMI_RX_MAC);//<2>Enable trend and boundary tracking
    }
    else if(track_mode == HDMI_AUDIO_TREND_BOUND)
    {
        HDMI_PRINTF("\n ***** TREND_BOUND Tracking*****\n");
        /*(14)FSM Initial*/
        //hdmi_rx_reg_write32(HDMI_HDMI_FBR_VADDR,0x77);//Target FIFO depth = 14 ,Boundary address distance = 7
        hdmi_rx_reg_write32(HDMI_HDMI_FBR_VADDR,0x74, HDMI_RX_MAC);
        //hdmi_rx_reg_write32(HDMI_HDMI_FTR_VADDR,0x03);//target times for summation of one trend to decide the trend
        hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR,~_BIT6,_BIT6, HDMI_RX_MAC);//FSM entry Pre-mode (AOC=1)
        hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR,~_BIT5,_BIT5, HDMI_RX_MAC);//FSM entry next step (AOMC=1)
        HDMI_DELAYMS(1);//wait fifo to target fifo level

        /*(15)Enable trend and boundary tracking*/
        hdmi_rx_reg_write32(HDMI_HDMI_ICTPSR0_VADDR,0x0005, HDMI_RX_MAC);          //Set I code  of trend [15:8]
        hdmi_rx_reg_write32(HDMI_HDMI_PCTPSR0_VADDR,0x02FF, HDMI_RX_MAC);  //Set P code of trend [15:8]
        hdmi_rx_reg_write32(HDMI_HDMI_ICBPSR0_VADDR,0x0001, HDMI_RX_MAC);      //Set I code of bnd [15:8]
        hdmi_rx_reg_write32(HDMI_HDMI_PCBPSR0_VADDR,0x02FF, HDMI_RX_MAC);  //Set P code of bnd [15:8]
        hdmi_rx_reg_write32(HDMI_HDMI_STBPR_VADDR,0x01, HDMI_RX_MAC);      //Set Boundary Tracking Update Response Time
        hdmi_rx_reg_write32(HDMI_HDMI_PSCR_VADDR,  (HDMI_PSCR_fdint(4) | HDMI_PSCR_etcn(0) | HDMI_PSCR_etfd(1) | HDMI_PSCR_etfbc(1) |HDMI_PSCR_pecs(2)), HDMI_RX_MAC);// FIFO depth tracking
        hdmi_rx_reg_mask32(HDMI_HDMI_PSCR_VADDR, ~(_BIT3|_BIT2), (_BIT3|_BIT2), HDMI_RX_MAC);//<1>Enable trend and boundary tracking
        hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4 , _BIT4, HDMI_RX_MAC);//Update Double Buffer
        HDMI_DELAYMS(20);
        hdmi_rx_reg_mask32(HDMI_HDMI_WDCR0_VADDR,~HDMI_WDCR0_bt_track_en_mask,HDMI_WDCR0_bt_track_en_mask, HDMI_RX_MAC);//<2>Enable trend and boundary tracking

    }else{//  H/W N/CTS Tracking
        HDMI_PRINTF("\n ***** N/CTS Tracking*****\n");
        /*(12)Enable N/CTS tracking*/
        if (hdmi_audiopll_param[i].S1)
                S = hdmi_audiopll_param[i].S*2;
            else
                S = hdmi_audiopll_param[i].S;
            HDMI_PRINTF("S = %d , ACR_N=%d ,    hdmi_audiopll_param[i].O = %d\n",S, ACR_N, hdmi_audiopll_param[i].O );
            if (ACR_N)
                {
                I_Code =16*1024*1024/(ACR_N*S*(hdmi_audiopll_param[i].O<<1));
                I_Code = I_Code;
                }
            else
                I_Code = 0x02;

        //calculate 4*N*(1/128fa) or 4*CTS*Tv, 4x for delay (HDMI_DELAYMS(1) 約為300us)
        tmp1 = 4*ACR_N*1000/(128*freq);
        if (tmp1 < 5)
            tmp1 = tmp1;//CTS*Tv < tmp1/4 > 2* CTS*Tv,  CTS*Tv(0.67ms~3.3ms)
        else if (tmp1 < 9)
            tmp1 = tmp1+2;
        else
            tmp1 = tmp1+3;

        //HDMI_PRINTF( "I Code = %d\n",I_Code);
        //HDMI_PRINTF( "tmp1 = %d\n",tmp1);

        hdmi_rx_reg_write32(HDMI_HDMI_ICPSNCR0_VADDR,I_Code, HDMI_RX_MAC);     //Set I code of Ncts[15:8]
        hdmi_rx_reg_write32(HDMI_HDMI_PCPSNCR0_VADDR,0x0000, HDMI_RX_MAC); //Set P code of Ncts [15:8]
        //hdmi_rx_reg_mask32(HDMI_NPECR,~_BIT30,_BIT30); //N_CTS tracking re-enable toggle function enable
        hdmi_rx_reg_mask32(HDMI_NPECR,~HDMI_NPECR_ncts_re_enable_off_en_mask,HDMI_NPECR_ncts_re_enable_off_en_mask, HDMI_RX_MAC);
        hdmi_rx_reg_write32(HDMI_HDMI_PSCR_VADDR,0x92, HDMI_RX_MAC);   //Enable N_CTS tracking & set FIFO depth
        hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4 , _BIT4, HDMI_RX_MAC);    //Update Double Buffer
#if 0
        if (tmp1 > 9)
            {
            HDMI_DELAYMS(tmp1-9);// HDMI_DELAYMS 要小於10 才行,所以做兩次,因為兩個的單位不大一樣
            HDMI_DELAYMS(9);
            }
        else
            HDMI_DELAYMS(tmp1);
#endif
        hdmi_rx_reg_write32(HDMI_RHDMI_PETR_VADDR,0x1e, HDMI_RX_MAC);//phase error threshold
             for (timeout = 0; timeout < 25; timeout++) {
            hdmi_rx_reg_mask32(HDMI_HDMI_SR_VADDR,~_BIT3,_BIT3, HDMI_RX_MAC);
            hdmi_rx_reg_write32(HDMI_HDMI_NCPER_VADDR,0xff, HDMI_RX_MAC);
            HDMI_DELAYMS(20);
            if ((hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & (_BIT3)) == 0)
                break;
        }

        if (timeout == 25)
            HDMI_PRINTF( "PLL 1st check not lock = %x\n",hdmi_rx_reg_read32(HDMI_HDMI_NCPER_VADDR, HDMI_RX_MAC));
        else
            HDMI_PRINTF( "PLL 1st check lock count = %d\n",timeout);


        hdmi_rx_reg_mask32(HDMI_HDMI_PSCR_VADDR,~_BIT4,0, HDMI_RX_MAC);  //disable N_CTS tracking
        hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4 , _BIT4, HDMI_RX_MAC);    //Update Double Buffer
        //hdmi_rx_reg_mask32(HDMI_NPECR,~_BIT30,0);  //N_CTS tracking re-enable toggle function disable
        hdmi_rx_reg_mask32(HDMI_NPECR,~HDMI_NPECR_ncts_re_enable_off_en_mask,0, HDMI_RX_MAC);//N_CTS tracking re-enable toggle function disable
        hdmi_rx_reg_write32(HDMI_HDMI_ICPSNCR0_VADDR,0x0002, HDMI_RX_MAC);     //Set I code of Ncts[15:8]
        hdmi_rx_reg_write32(HDMI_HDMI_PCPSNCR0_VADDR,0x2000, HDMI_RX_MAC); //Set P code of Ncts [15:8]
        hdmi_rx_reg_mask32(HDMI_HDMI_PSCR_VADDR,~_BIT4,_BIT4, HDMI_RX_MAC);  //enable N_CTS tracking
        hdmi_rx_reg_mask32(HDMI_HDMI_CMCR_VADDR,~_BIT4 , _BIT4, HDMI_RX_MAC);    //Update Double Buffer

        //N&CTS boundary set
        hdmi_rx_reg_mask32(AUDIO_CTS_UP_BOUND,~0xfffff,0x6ddd0, HDMI_RX_MAC);     //CTS up boundary set 450000
        hdmi_rx_reg_mask32(AUDIO_CTS_LOW_BOUND,~0xfffff,0x4e20, HDMI_RX_MAC);     //CTS low boundary set 20000
        hdmi_rx_reg_mask32(AUDIO_N_UP_BOUND,~0xfffff,0x13880, HDMI_RX_MAC);       //N up boundary set 80000
        hdmi_rx_reg_mask32(AUDIO_N_LOW_BOUND,~0xfffff,0x7d0, HDMI_RX_MAC);        //N low boundary set 2000
        hdmi_rx_reg_mask32(AUDIO_CTS_UP_BOUND,~_BIT20,_BIT20, HDMI_RX_MAC);       //CTS has glitch not to tracking enable
        hdmi_rx_reg_mask32(AUDIO_N_UP_BOUND,~_BIT20,_BIT20, HDMI_RX_MAC);         //N has glitch not to tracking enable

        /*(13)Wait PLL lock by N&CTS tracking*/
        hdmi_rx_reg_write32(HDMI_RHDMI_PETR_VADDR,0x1e, HDMI_RX_MAC);//phase error threshold
             for (timeout = 0; timeout < 25; timeout++) {
            hdmi_rx_reg_mask32(HDMI_HDMI_SR_VADDR,~_BIT3,_BIT3, HDMI_RX_MAC);
            hdmi_rx_reg_write32(HDMI_HDMI_NCPER_VADDR,0xff, HDMI_RX_MAC);
            HDMI_DELAYMS(20);
            if ((hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & (_BIT3)) == 0)
                break;
        }

        if (timeout == 25)
            HDMI_PRINTF( "PLL not lock = %x\n",hdmi_rx_reg_read32(HDMI_HDMI_NCPER_VADDR, HDMI_RX_MAC));
        else
            HDMI_PRINTF( "PLL lock count = %d\n",timeout);

        /*(14)FSM Initial*/
        hdmi_rx_reg_write32(HDMI_HDMI_FBR_VADDR,0x77, HDMI_RX_MAC);//Target FIFO depth = 14 ,Boundary address distance = 7
        hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR,~_BIT6,_BIT6, HDMI_RX_MAC);//FSM entry Pre-mode (AOC=1)
        hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR,~_BIT5,_BIT5, HDMI_RX_MAC);//FSM entry next step (AOMC=1)
        HDMI_DELAYMS(1);//wait fifo to target fifo level
    }
#if 0
            hdmi_rx_reg_write32(HDMI_HDMI_PTRSV1_VADDR, 0x01, HDMI_RX_MAC);     // Wait for ACR : Packet Type = 0x01
            for (i=0; i<100; i++) {
            hdmi_rx_reg_write32(HDMI_HDMI_NCPER_VADDR,0xff, HDMI_RX_MAC);
            if ((hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & (_BIT2|_BIT1))!=0)
                hdmi_rx_reg_mask32(HDMI_HDMI_SR_VADDR,~(_BIT2|_BIT1),(_BIT2|_BIT1), HDMI_RX_MAC);
            HdmiGetStruct(&isr_info);
            b = isr_info.b;
            HDMI_DELAYMS(20);

            HDMI_PRINTF( "Sum C  = %x  %x \n",hdmi_rx_reg_read32(HDMI_HDMI_DPCR4_VADDR, HDMI_RX_MAC),hdmi_rx_reg_read32(HDMI_HDMI_DPCR5_VADDR, HDMI_RX_MAC));
            //if ((hdmi_rx_reg_read32(HDMI_HDMI_DPCR4_VADDR)&0xff)!=0x7)
            {
                //cts = Hdmi_PacketSRAMRead(113)<<16;
                //cts = cts|Hdmi_PacketSRAMRead(114)<<8;
                //cts = cts|Hdmi_PacketSRAMRead(115);
                //n = Hdmi_PacketSRAMRead(116)<<16;
                //n = n|Hdmi_PacketSRAMRead(117)<<8;
                //n = n|Hdmi_PacketSRAMRead(118);
                //HDMI_PRINTF( "CTS = %d  N = %d  \n",cts,n);
                HDMI_PRINTF( "FIFO depth  =%x  %x\n",hdmi_rx_reg_read32(HDMI_HDMI_FDDR_VADDR, HDMI_RX_MAC),hdmi_rx_reg_read32(HDMI_HDMI_FDDF_VADDR, HDMI_RX_MAC));
                HDMI_PRINTF( "b=%d Phase err = %x\n",b,hdmi_rx_reg_read32(HDMI_HDMI_NCPER_VADDR, HDMI_RX_MAC));
                HDMI_PRINTF("FIFO Check  = %x\n", hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC));
            }
            }

#endif
    /*(16)Wait FIFO stable*/
    for (timeout = 0; timeout < 5; timeout++) {
        hdmi_rx_reg_write32(HDMI_HDMI_NCPER_VADDR,0xff, HDMI_RX_MAC);
        HDMI_DELAYMS(20);
        if ((hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & (_BIT3|_BIT2|_BIT1))==0)
            break;
        hdmi_rx_reg_mask32(HDMI_HDMI_SR_VADDR,~(_BIT3|_BIT2|_BIT1),(_BIT3|_BIT2|_BIT1), HDMI_RX_MAC);
    }

    if (timeout == 5)
        HDMI_PRINTF( "FIFO Unstable  = %x \n",hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC));
    else
        HDMI_PRINTF( "FIFO timeout count = %d\n",timeout);

    HDMI_PRINTF( "HDMI_HDMI_AVMCR_VADDR = %x\n", hdmi_rx_reg_read32(HDMI_HDMI_AVMCR_VADDR, HDMI_RX_MAC));

    return TRUE;

}


void drvif_Hdmi_Release(void) {

    HDMI_PRINTF("%s \n", "drvif_Hdmi_Release");
    HDMI_PRINTF("Release Channel %d\n", GET_HDMI_CHANNEL());
    SET_IS_FIRST_DETECT_MODE(1);
    HdmiSetAPStatus(HDMIRX_RELEASED);
#if HDMI_DEBUG_AUDIO_PLL_RING
    HdmiAudioPLLSampleStop();
#endif
    Hdmi_AudioOutputDisable();
    Hdmi_VideoOutputDisable();

    Hdmi_Power(0);

    if (GET_HDMI_CHANNEL() >= HDMI_MAX_CHANNEL || GET_HDMI_CHANNEL() < 0) return;

    if (hdmi.channel[GET_HDMI_CHANNEL()]->phy_port == HDMI_PHY_PORT_NOTUSED) {
        HDMI_PRINTF("This channel %d is Disable\n", GET_HDMI_CHANNEL());
        return;
    }

    if (GET_HDMI_FASTER_BOOT_EN() && GET_HDMI_BOOT_FIRST_RUN()) return;

    if (GET_HDMI_FASTER_SWITCH_EN())
        return;  // never turn off the hdmi Phy

//    HdmiISREnable(0);
    if (GET_HDMI_CHANNEL() < HDMI_MAX_CHANNEL && GET_HDMI_CHANNEL()>= 0) {
            if (hdmi.channel[GET_HDMI_CHANNEL()]) {
    #if(defined USE_CEC) //// cloud Add CEC function request
        //support CEC not set hot plug low
    #else   //#if(defined USE_CEC)
        #ifdef HDMI_SWITCH_ITE6633
            HdmiMux_IT6633_HPD_Set(0);
        #else
                drvif_Hdmi_HPD(GET_HDMI_CHANNEL(), 0);
        #endif
    #endif  //#if(defined USE_CEC)
                //drvif_Hdmi_EnableEDID(GET_HDMI_CHANNEL(), 0);
                //drvif_Hdmi_PhyPortEnable(hdmi.channel[GET_HDMI_CHANNEL()]->phy_port, 0);

            }
    }

}

void drvif_Hdmi_HPD(unsigned char channel_index, char high) {

    switch ((unsigned int) hdmi.channel[channel_index]->edid_type) {
        case HDMI_HPD_ONCHIP:
                if ((channel_index < HDMI_MAX_CHANNEL) && hdmi.channel[channel_index]->HotPlugPin >= 0) {
                    if (high)
                    {
						switch(channel_index)
						{
							case 0://Channel 0 hotplug control
								gpio_direction_output(hdmi.gpio_hpd_ctrl,0);
								break;
							default:
								break;
						}
                    }
                    else
                    {
						switch(channel_index)
						{
							case 0://Channel 0 hotplug control
								gpio_direction_output(hdmi.gpio_hpd_ctrl,1);
								break;
							default:
								break;
						}
                    }
                }
        break;
        case HDMI_HPD_I2CMUX:
                if ((channel_index < HDMI_MAX_CHANNEL) && hdmi.channel[channel_index]->HotPlugPin >= 0) {
                    if (high) {
                        HdmiMuxHPD( hdmi.channel[channel_index]->HotPlugPin, hdmi.channel[channel_index]->HotPlugPinHighValue != 0);
                    } else {
                        HdmiMuxHPD( hdmi.channel[channel_index]->HotPlugPin, hdmi.channel[channel_index]->HotPlugPinHighValue == 0);
                    }
                }
        break;
        default:
        break;
    }
}

HDMI_DVI_MODE_T IsHDMI(void)
{

    if(hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & _BIT0) {
//      HDMI_PRINTF("<<<<<<<<<< HDMI mode \n");
        return MODE_HDMI; // HDMI mode
    }  else {
//      HDMI_PRINTF("<<<<<<<<<< DVI mode \n");
        return MODE_DVI; // DVI mode
    }
}


HDMI_BOOL Hdmi_VideoPLLSetting(int b, int cd, int Enable2X) {

//  Fin =   Fxtal * 1024 / b
//  Target vco = ( Fin * m / n )     , TagretVco_HB = 400 ,  TagretVco_LB=200
//  Fin * M / N / 2^o / 2 * s = Fout = Fin * [24/30, 24/36, 24/48] ,  [10bits, 12bits,16bits]
//  200 <  ( Fin * m / n )  < 400  -->  200 <   Fin * 2^o * s * [ 8/5 , 4/3, 1 ]   < 400
//  Scode_LB =  200 * 15 * b  / ( Fxtal *1024 * [24,20,15] )
//  Scode_HB =  400 * 15 * b  / ( Fxtal *1024 * [24,20,15] )
//  Smean = (Scode_LB +Scode_HB ) /2
//  M/N = [8/5 , 4/3 , 1 ] * S

    unsigned int large_ratio, Smean,Stest, m, n, o, fraction1, fraction2, pixel_clockx1024, fvco;
	int is_progressive = Enable2X ? 0 : 1;

    if (b <= 0){
        return FALSE;
    }

    if (cd >= 4) cd = 0;

    //hdmi_rx_reg_mask32(HDMI_HDMI_VPLLCR1_VADDR, (~(_BIT0|_BIT1 |_BIT2|_BIT3|_BIT11|_BIT12|_BIT13)),_BIT0|_BIT2);// Disable PLL
    //cloud modify for magellan remove bit 16~bit 10
       hdmi_rx_reg_mask32(HDMI_HDMI_VPLLCR1_VADDR, (~(HDMI_VPLLCR1_dpll_pwdn_mask|HDMI_VPLLCR1_dpll_vcorstb_mask |HDMI_VPLLCR1_dpll_freeze_mask|HDMI_VPLLCR1_dpll_stop_mask)),HDMI_VPLLCR1_dpll_pwdn_mask|HDMI_VPLLCR1_dpll_freeze_mask, HDMI_RX_MAC);// Disable PLL
    #define FCVO_MIN    250
    #define FCVO_MAX    500
#if HDMI_OLD_CLK_DETECT
    pixel_clockx1024 = ((27 * 1024 * 1024 * dpll_ratio[cd].SM ) + (hdmi.b * ((unsigned int) dpll_ratio[cd].SN)/2)) / hdmi.b/ dpll_ratio[cd].SN;
#else
    pixel_clockx1024 =(hdmi.b * 27 * dpll_ratio[cd].SM * 1024) / (dpll_ratio[cd].SN * 256);
#endif
    HDMI_PRINTF("pixel_clock = %d\n", pixel_clockx1024);

    if (pixel_clockx1024 < (160 * 1024) && Enable2X == 0) {
        Enable2X = 1;       // if pixel_clock is under 160MHz then enable 2X clock maybe for DI
    }

    if (Enable2X&&(is_progressive == 0)) {
        large_ratio = 2;
        hdmi_rx_reg_mask32(HDMI_TMDS_CPS_VADDR, ~TMDS_CPS_pll_div2_en_mask, TMDS_CPS_pll_div2_en(0), HDMI_RX_MAC);
    }   else  {
        large_ratio = 1;
        hdmi_rx_reg_mask32(HDMI_TMDS_CPS_VADDR, ~TMDS_CPS_pll_div2_en_mask, TMDS_CPS_pll_div2_en(0), HDMI_RX_MAC);

    }

    if  ((cd || Enable2X)&&(is_progressive == 0)) {
         hdmi_rx_reg_mask32(HDMI_TMDS_DPC_SET0_VADDR, ~TMDS_DPC_SET0_dpc_bypass_dis_mask, TMDS_DPC_SET0_dpc_bypass_dis(0), HDMI_RX_MAC);
     } else {
         hdmi_rx_reg_mask32(HDMI_TMDS_DPC_SET0_VADDR, ~TMDS_DPC_SET0_dpc_bypass_dis_mask, TMDS_DPC_SET0_dpc_bypass_dis(0), HDMI_RX_MAC);
     }

    if (large_ratio != 1)
    {
#if HDMI_OLD_CLK_DETECT
        pixel_clockx1024 = ((27 * 1024 * 1024 * dpll_ratio[cd].SM * large_ratio) + (hdmi.b * ((unsigned int) dpll_ratio[cd].SN)/2)) / hdmi.b/ dpll_ratio[cd].SN;
#else
        pixel_clockx1024 =(hdmi.b * 27 * dpll_ratio[cd].SM * 1024 * 2) / (dpll_ratio[cd].SN * 256);
#endif
        HDMI_PRINTF("large_ratio=%d, 2X pixel clock PLL = %d\n", large_ratio, pixel_clockx1024);
    }


    if (pixel_clockx1024 == 0) {
        HDMI_PRINTF("pixel_clockx1024 is zero\n");
        return FALSE;
    }

    o = 1;
    Smean = 0;
    Stest = 1;
    while(pixel_clockx1024 <= (200*1024)){
        if (Smean == 0) Stest = 1;
        else Stest = Smean * 2;
        // 2---> 2^o
        if (((pixel_clockx1024*2)*Stest)>= (FCVO_MIN*1024))
            break;
        Smean ++;
        //pr_info("[In while loop 2]\n");
    };

    if (pixel_clockx1024 > (200*1024)){// >200MHz
        o = 0;
        Smean = 0;
    }

    HDMI_PRINTF( "Smean =  %d\n", Smean);

    //if (Smean == 0) Stest = 1;
    //else Stest = Smean * 2;


    n = 0;
    do {
            n += dpll_ratio[cd].RatioN;
            m =((dpll_ratio[cd].RatioM * Stest * n * large_ratio)<<o) / dpll_ratio[cd].RatioN;
            HDMI_PRINTF( "%d %d\n", m, n);
    } while(n < 2);

#if HDMI_OLD_CLK_DETECT
    fvco = (27 * m * 1024) / hdmi.b / n;
#else
    fvco = (hdmi.b * 27 * m) / (256 * n);
#endif

//  Icp x (N/M) x (27M/Fin) = 0.95uA
//  Icp x (N/M) x (27M/(27Mx1024/b) = 0.95
//  Icp x (N/M) x (b/1024) = 0.95
//  Icp x (N/M) x (b/1024) x 100 = 95
//  Icp = (Mx1024x95)/(Nxbx100)
#if HDMI_OLD_CLK_DETECT
    fraction1 = ((unsigned long)m *1024 *95 *4 /(n *b *100)) ;    // 2bit fractional part
#else
    fraction1 = ((unsigned long)m *95 *4 *b /(n *256 *100)) ;    // 2bit fractional part
#endif
    fraction2 = 0x00;
    HDMI_PRINTF("***************fraction1=%d\n",fraction1);
    //if(((fraction1&0x01) == 0x00)||(fraction1 > 80))
        //fraction2 |= 0x40;

    if (fraction1 >=10)
       fraction1 -= 10;

        if(fraction1 >= 40)        // 2bit fractional part
        {
       fraction1 -= 40;
       fraction2 |= 0x04;
        }

        if(fraction1 >= 20)        // 2bit fractional part
        {
       fraction1 -= 20;
       fraction2 |= 0x02;
        }

        if(fraction1 >= 10)        // 2bit fractional part
        {
       fraction1 -= 10;
       fraction2 |= 0x01;
        }
    HDMI_PRINTF("***************fraction2=%d\n",fraction2);
    //fraction2 |= 0x18;

    HDMI_PRINTF("***************cd=%d\n",cd);
    HDMI_PRINTF("***************m=%d\n",m);
    HDMI_PRINTF("***************n=%d\n",n);
    HDMI_PRINTF("***************o=%d\n",o);
    HDMI_PRINTF("***************s=%d\n",Smean);
    //HDMI_PRINTF("***************fraction1=%d\n",fraction1);
    //HDMI_PRINTF("***************fraction2=%d\n",fraction2);
    HDMI_PRINTF("***************pixel_clockx1024=%d\n",pixel_clockx1024);
    HDMI_PRINTF("***************fvco=%d MHz\n",fvco);
    HDMI_PRINTF("***************larget=%d\n",large_ratio);


    hdmi_rx_reg_write32(HDMI_HDMI_VPLLCR0_VADDR, HDMI_VPLLCR0_dpll_m(m-2) |HDMI_VPLLCR0_dpll_o(o) |HDMI_VPLLCR0_dpll_n(n-2) |HDMI_VPLLCR0_dpll_rs(3) |HDMI_VPLLCR0_dpll_ip(fraction2), HDMI_RX_MAC);// | fraction2);   //set video PLL parameter
    hdmi_rx_reg_write32(HDMI_MN_SCLKG_DIVS_VADDR,Smean, HDMI_RX_MAC);      //set video PLL output divider
    //hdmi_rx_reg_mask32(HDMI_HDMI_VPLLCR1_VADDR, (~(_BIT0 | _BIT1 | _BIT2)),_BIT1|_BIT3);  // Enable PLL
    hdmi_rx_reg_mask32(HDMI_MN_SCLKG_CTRL_VADDR,~_BIT4,_BIT4, HDMI_RX_MAC);      //video PLL double buffer load
    //hdmi_rx_reg_mask32(HDMI_HDMI_VPLLCR1_VADDR, (~(_BIT0|_BIT1 |_BIT2|_BIT3)),_BIT1|_BIT3);// Enable PLL
    //hdmi_rx_reg_mask32(HDMI_HDMI_VPLLCR1_VADDR, ~((HDMI_VPLLCR1_dpll_pwdn_mask|HDMI_VPLLCR1_dpll_vcorstb_mask |HDMI_VPLLCR1_dpll_freeze_mask|HDMI_VPLLCR1_dpll_stop_mask)),(HDMI_VPLLCR1_dpll_vcorstb_mask|HDMI_VPLLCR1_dpll_stop_mask), HDMI_RX_MAC);

	hdmi_rx_reg_mask32(HDMI_HDMI_VPLLCR1_VADDR, ~(HDMI_VPLLCR1_dpll_pwdn_mask | HDMI_VPLLCR1_dpll_stop_mask), HDMI_VPLLCR1_dpll_stop_mask, HDMI_RX_MAC); 
	hdmi_rx_reg_mask32(HDMI_HDMI_VPLLCR1_VADDR, ~(HDMI_VPLLCR1_dpll_vcorstb_mask), (HDMI_VPLLCR1_dpll_vcorstb_mask), HDMI_RX_MAC);
	HDMI_DELAYMS(10);
	hdmi_rx_reg_mask32(HDMI_HDMI_VPLLCR1_VADDR, ~(HDMI_VPLLCR1_dpll_freeze_mask), 0, HDMI_RX_MAC);


    return TRUE;
}

void Hdmi_DumpState(void) {

    char *colormetry_name[]  = {
            "HDMI_COLORIMETRY_NOSPECIFIED",
            "HDMI_COLORIMETRY_601",
            "HDMI_COLORIMETRY_709",
            "HDMI_COLORIMETRY_XYYCC601",
            "HDMI_COLORIMETRY_XYYCC709",
            "HDMI_COLORIMETRY_SYCC601",
            "HDMI_COLORIMETRY_ADOBE_YCC601",
            "HDMI_COLORIMETRY_ADOBE_RGB",

    };

    char *depth_name[] = {
        "HDMI_COLOR_DEPTH_8B",
        "HDMI_COLOR_DEPTH_10B",
        "HDMI_COLOR_DEPTH_12B",
        "HDMI_COLOR_DEPTH_16B",

    };

    char *colorspace_name[] = {
        "COLOR_RGB",
        "COLOR_YUV422",
        "COLOR_YUV444",
        "COLOR_YUV411",
        "COLOR_UNKNOW"
    };

    char *hdmi_3d_name[] = {
        "HDMI3D_FRAME_PACKING",
        "HDMI3D_FIELD_ALTERNATIVE",
        "HDMI3D_LINE_ALTERNATIVE",
        "HDMI3D_SIDE_BY_SIDE_FULL",
        "HDMI3D_L_DEPTH",
        "HDMI3D_L_DEPTH_GPX",
        "HDMI3D_TOP_AND_BOTTOM",
        "HDMI3D_RSV0",
        "HDMI3D_SIDE_BY_SIDE_HALF",
        "HDMI3D_RSV1",
        "HDMI3D_2D_ONLY",
    };

    pr_err("bHDMIColorSpace = %s\n",( hdmi.tx_timing.color < (sizeof(colorspace_name)/4) ) ? colorspace_name[hdmi.tx_timing.color] : "UNDEFINED");
    pr_err("IsInterlaced = %d\n", GET_HDMI_ISINTERLACE());
    pr_err("bIsHDMIDVI = %d\n", GET_ISHDMI());
    pr_err("VedioFSMState = %d\n", GET_HDMI_VIDEO_FSM());
    pr_err("AudioFSMState = %d\n", GET_HDMI_AUDIO_FSM());
    pr_err("ColorDepth = %s\n", (hdmi.tx_timing.depth <(sizeof(depth_name)/4))  ? depth_name[hdmi.tx_timing.depth] : "UNDEFINED");
    pr_err("ColorMetry = %s\n", (hdmi.tx_timing.colorimetry < (sizeof(colormetry_name)/4))  ? colormetry_name[hdmi.tx_timing.colorimetry]: "UNDEFINED");
    pr_err("3D Format  = %s\n", (hdmi.tx_timing.hdmi_3dformat < (sizeof(hdmi_3d_name)/4)) ? hdmi_3d_name[hdmi.tx_timing.hdmi_3dformat] : "UNDEFINED");
//  pr_err("Is422 = %d\n", GET_SCALER_IS422());
}

void restartHdmiRxWrapperDetection(void)
{
	stop_mipi_process();
	set_hdmirx_wrapper_interrupt_en(0,0,0);
    SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);
    SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
    set_hdmirx_wrapper_control_0(-1, 0,-1,-1,-1,-1);
    set_hdmirx_wrapper_hor_threshold(HDMI_MIN_SUPPORT_H_PIXEL);

	memset(&hdmi_ioctl_struct, 0, sizeof(hdmi_ioctl_struct));
}

HDMI_BOOL drvif_Hdmi_DetectMode(void)
{
    unsigned char result = PHOENIX_MODE_DETECT;
    HDMIRX_IOCTL_STRUCT_T isr_info;
	unsigned int retry_times = 0;

retry:
    if (Hdmi_CheckConditionChange() != HDMI_ERR_NO)
        SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);

    //pr_info("[In drvif_Hdmi_DetectMode case :%d]\n",GET_HDMI_VIDEO_FSM());
    switch(GET_HDMI_VIDEO_FSM()) {

        case MAIN_FSM_HDMI_SETUP_VEDIO_PLL:
                    HDMI_PRINTF("MAIN_FSM_HDMI_SETUP_VEDIO_PLL\n");
                    SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
                    HDMI_Audio_Conut = 0;

                    hdmi_rx_reg_mask32(HDMI_HDMI_WDCR0_VADDR, ~0xFFDF9E, 0x00, HDMI_RX_MAC); // Clear Audio Watch Dog and Set X: 15
                    hdmi_rx_reg_mask32(HDMI_HDMI_VCR_VADDR, (~_BIT7) , 0, HDMI_RX_MAC);  // not inverse EVEN/ODD
                    // enable auto detection of colorspace and pixel repeat
                    hdmi_rx_reg_mask32(HDMI_HDMI_VCR_VADDR, ~(HDMI_VCR_csam_mask | HDMI_VCR_prdsam_mask), HDMI_VCR_csam(1) | HDMI_VCR_prdsam(1), HDMI_RX_MAC);
                    // get TMDS b value for ISR/high priority task
                    HdmiGetStruct(&isr_info);
                    hdmi.b = isr_info.b;

					if (hdmi.b <116)  // 20140819 jerrychen: ignore noise clock signal
					{
						return PHOENIX_MODE_NOSIGNAL;
					}

                    //if (hdmi.b  < 80 || hdmi.b > 2100) return PHOENIX_MODE_NOSIGNAL;
                    // measure interlace/progressive
                    hdmi.tx_timing.progressive = (Hdmi_GetInterlace(HDMI_MS_MODE_ONESHOT) == 0);
                    SET_HDMI_ISINTERLACE((hdmi.tx_timing.progressive == 0));
                    // measure color depth
                    SET_HDMI_CD(hdmi_rx_reg_read32(HDMI_TMDS_DPC0_VADDR, HDMI_RX_MAC) & (0x0f));
                    // tx_timing
                    hdmi.tx_timing.depth = (HDMI_COLOR_DEPTH_T) drvif_Hdmi_GetColorDepth();

                    // setup Video PLL
                    if ((hdmi.tx_timing.progressive) == 0) {
                            pr_info("Interlace.\n");
                            #if HDMI_AUTO_DEONLY
                            // for Interlace timing, disable DE_ONLY mode is better idea: for IVSTA is the same for odd/even field
                            SET_HDMI_DEONLY_MODE(0);
                            hdmi_rx_reg_mask32(HDMI_TMDS_PWDCTL_VADDR,~(TMDS_PWDCTL_deo_mask), TMDS_PWDCTL_deo(0), HDMI_RX_MAC);
                            #endif
                            if (!Hdmi_VideoPLLSetting(hdmi.b, (int)hdmi.tx_timing.depth, 1))
                                return PHOENIX_MODE_NOSIGNAL;
    //                      hdmi_rx_reg_mask32(HDMI_TMDS_DPC_SET0_VADDR, ~TMDS_DPC_SET0_dpc_bypass_dis_mask, TMDS_DPC_SET0_dpc_bypass_dis(1));
                    } else {
                            pr_info("Progressive.\n");
                            #if HDMI_AUTO_DEONLY
                            // for progressive timing, enable DE_ONLY mode is better idea: for TX may not generate HVSync
                            SET_HDMI_DEONLY_MODE(1);                             //kist
                            hdmi_rx_reg_mask32(HDMI_TMDS_PWDCTL_VADDR,~(TMDS_PWDCTL_deo_mask), TMDS_PWDCTL_deo(1));
                            #endif
                            if (!Hdmi_VideoPLLSetting(hdmi.b, (int)hdmi.tx_timing.depth, 0))
                                return PHOENIX_MODE_NOSIGNAL;
                            //hdmi_rx_reg_mask32(HDMI_TMDS_DPC_SET0_VADDR, ~TMDS_DPC_SET0_dpc_bypass_dis_mask, TMDS_DPC_SET0_dpc_bypass_dis(0));
                    }
                    //FIXME: Hdmi_VideoPLLSetting() causes signal unstable, wait long enough to let HDMI Rx wrapper detection flow complete
                    usleep_range(150000, 200000);//150ms~200ms

                    SET_ISHDMI(IsHDMI());
                    if (GET_ISHDMI() == MODE_DVI) {
                        //Disable Auto color space detect,Auto pixel reapeat down sample
                        HDMI_PRINTF("DVI mode setting\n");
                        hdmi_rx_reg_mask32(HDMI_HDMI_VCR_VADDR, ~(HDMI_VCR_csam_mask | HDMI_VCR_prdsam_mask | HDMI_VCR_dsc_mask),
                            HDMI_VCR_csam(0) | HDMI_VCR_prdsam(0) | HDMI_VCR_dsc(0), HDMI_RX_MAC);
                        HDMI_DELAYMS(20);//Set down sampling  =1 can't set if no this delay
                        // Set down sampling = 1
                        hdmi_rx_reg_mask32(HDMI_HDMI_VCR_VADDR, ~(HDMI_VCR_csam_mask | HDMI_VCR_prdsam_mask | HDMI_VCR_dsc_mask |HDMI_VCR_csc_mask),
                            HDMI_VCR_csam(0) | HDMI_VCR_prdsam(0) | HDMI_VCR_dsc(0) | HDMI_VCR_csc(0), HDMI_RX_MAC);
                        // this code is strange , why set twice in this way
                                      //cloud add for MHL Setting
                        //
                        HDMI_DELAYMS(20);
                    }
                    Hdmi_Get3DInfo(HDMI_MS_MODE_ONESHOT_INIT);
                    //Hdmi_GetInterlace(HDMI_MS_MODE_CONTINUOUS_INIT);
                    SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_MEASURE);
        case MAIN_FSM_HDMI_MEASURE:
                HDMI_PRINTF("MAIN_FSM_HDMI_MEASURE\n");

                Hdmi_VideoOutputEnable();
            //   if (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR) & _BIT6) {
                 if (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC)&HDMI_SR_avmute_mask){
                    HDMI_PRINTF("AV Mute\n");
                    HdmiSetAPStatus(HDMIRX_DETECT_AVMUTE);
                 } else {
                    HdmiSetAPStatus(HDMIRX_DETECT_FAIL);
                 }
                if (GET_HDMI_ISINTERLACE() != Hdmi_GetInterlace(HDMI_MS_MODE_ONESHOT)) {
                    pr_info("interlace change in measure mode\n");
                    SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);  // reset Check mode state to initial
                    return PHOENIX_MODE_DETECT;
                }

                if( (result = Hdmi_Measure()) != PHOENIX_MODE_SUCCESS) {
                    SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);  // reset Check mode state to initial
                    if (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & _BIT6) return PHOENIX_MODE_DETECT;
                    return result;
                }
                if (GET_HDMI_ISINTERLACE() != Hdmi_GetInterlace(HDMI_MS_MODE_ONESHOT)) {
                    pr_info("interlace change in measure mode\n");
                    SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);  // reset Check mode state to initial
                    return PHOENIX_MODE_DETECT;
                }

                HdmiSetAPStatus(HDMIRX_DETECT_SUCCESS);
                if(GET_ISHDMI() == MODE_HDMI) {
                    SET_HDMI_COLOR_SPACE(Hdmi_GetColorSpace());
                }else {
                    // Determine Color Space
                    SET_HDMI_COLOR_SPACE(COLOR_RGB);
                }
                hdmi.tx_timing.hdmi_3dformat = Hdmi_Get3DInfo(HDMI_MS_MODE_ONESHOT);
                SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_DISPLAY_ON);

				if(retry_times<3)//Prevent stuck in this function
				{
					retry_times++;
					goto retry;
				}

        break;

        case MAIN_FSM_HDMI_DISPLAY_ON:
//              hdmi_rx_reg_mask32(0xb80004e4,~(_BIT2),(0)); //disable distur
                HDMI_PRINTF("MAIN_FSM_HDMI_DISPLAY_ON\n");
                //if ((hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & _BIT6) != 0) {
                if ((hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & HDMI_SR_avmute_mask) != 0) {
                    HDMI_PRINTF("#################### AV mute ####################\n");
                    SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);  // reset Check mode state to initial
                    return PHOENIX_MODE_DETECT;
                }
                // Determine DVI/HDMI mode
                if(GET_ISHDMI() == MODE_HDMI) {
                    SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
                    Hdmi_AudioModeDetect();//for AUDIO_FSM_FREQ_DETECT if normally
                    //Hdmi_AudioModeDetect();//for AUDIO_FSM_FREQ_DETECT if normally
                    // Determine Color Space
                    SET_HDMI_COLOR_SPACE(Hdmi_GetColorSpace());
                }else {
                    // Determine Color Space
                    SET_HDMI_COLOR_SPACE(COLOR_RGB);
                }
                drvif_Hdmi_AVI_RGB_Range();//cloud test
                if (GET_HDMI_ISINTERLACE() != Hdmi_GetInterlace(HDMI_MS_MODE_ONESHOT)) {
                    pr_info("interlace change in measure mode\n");
                    SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);  // reset Check mode state to initial
                    return PHOENIX_MODE_DETECT;
                }
                // To gather detail information from TX
                hdmi.tx_timing.colorimetry = Hdmi_GetColorimetry();
                hdmi.tx_timing.color = GET_HDMI_COLOR_SPACE();
                hdmi.tx_timing.progressive = GET_HDMI_ISINTERLACE() == 0;
                hdmi.tx_timing.depth =(HDMI_COLOR_DEPTH_T) drvif_Hdmi_GetColorDepth();

                Hdmi_DumpState();
                //if (Hdmi_CheckConditionChange() != HDMI_ERR_NO) return _MODE_DETECT;   // this is already mark , check if it need or not 2013 01 14
                //if (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & _BIT6) { // if AVMute Set --> wait    AVMute Off
                if (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & HDMI_SR_avmute_mask) {
                    HDMI_PRINTF("#################### AV mute ####################\n");
                    SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);  // reset Check mode state to initial
                    return PHOENIX_MODE_DETECT;
                }
                hdmi_rx_reg_write32(HDMI_HDMI_SR_VADDR, hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC), HDMI_RX_MAC);  // this line is strange ,cloud need check
                #if HDMI_FIX_GREEN_LINE
                hdmi_rx_reg_mask32(HDMI_TMDS_OUT_CTRL_VADDR, ~(TMDS_OUT_CTRL_tmds_bypass_mask) ,TMDS_OUT_CTRL_tmds_bypass(1), HDMI_RX_MAC);
                hdmi_rx_reg_write32(HDMI_TMDS_ROUT_VADDR, 0, HDMI_RX_MAC);
                hdmi_rx_reg_write32(HDMI_TMDS_GOUT_VADDR, 0, HDMI_RX_MAC);
                hdmi_rx_reg_write32(HDMI_TMDS_BOUT_VADDR, 0, HDMI_RX_MAC);

                if (GET_HDMI_COLOR_SPACE() != COLOR_RGB) {
                     hdmi_rx_reg_mask32(HDMI_TMDS_OUT_CTRL_VADDR, ~(TMDS_OUT_CTRL_tmds_bypass_mask) ,TMDS_OUT_CTRL_tmds_bypass(0), HDMI_RX_MAC);
                     hdmi_rx_reg_write32(HDMI_TMDS_ROUT_VADDR, 0x8000, HDMI_RX_MAC);
                     hdmi_rx_reg_write32(HDMI_TMDS_GOUT_VADDR, 0x1000, HDMI_RX_MAC);
                     hdmi_rx_reg_write32(HDMI_TMDS_BOUT_VADDR, 0x8000, HDMI_RX_MAC);
                }
                #endif

                SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);  // reset Check mode state to initial
                //SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_WAIT_READY);  // reset Check mode state to initial
                HDMI_DELAYMS(30);
                //HDMI_DELAYMS(100000);
                SET_IS_FIRST_CHECK_MODE(1);
                SET_DVI_AUDIO_PATH_SETUP(0);
                drvif_Hdmi_AVI_RGB_Range();//cloud test
                set_hdmirx_wrapper_control_0(-1,-1,-1,-1, hdmi_rx_output_format(hdmi.tx_timing.color),-1);
                hdmi_ioctl_struct.detect_done = 1;
				set_hdmirx_wrapper_interrupt_en(0,0,1);
                return PHOENIX_MODE_SUCCESS;

        break;
        case MAIN_FSM_HDMI_MEASURE_ACTIVE_SPACE:
        break;
        case MAIN_FSM_HDMI_WAIT_READY:
        {
            HDMI_PRINTF("MAIN_FSM_HDMI_WAIT_READY\n");
            if (hdmi_onms_measure(NULL) != HDMI_ERR_NO)
            {
                    SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);  // reset Check mode state to initial
                    return PHOENIX_MODE_DETECT;
            }
            if (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & _BIT6) return PHOENIX_MODE_DETECT;
            SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);  // reset Check mode state to initial
            return PHOENIX_MODE_SUCCESS;
        }
        break;

        default:
            SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);  // reset Check mode state to initial
        break;


    }
    return result;
}

int drvif_Hdmi_CheckMode(void)
{
    HDMIRX_IOCTL_STRUCT_T isr_info;
    HDMI_LOG("%s \n", "drvif_Hdmi_CheckMode");
    HdmiSetAPStatus(HDMIRX_CHECK_MODE);
    /*
        Enable Vedio WDG as Check Mode
    */
    HDMI_LOG( "PLL lock  %x\n", hdmi_rx_reg_read32(HDMI_HDMI_AVMCR_VADDR));
    HDMI_LOG( "HDMI_HDMI_SR_VADDR = %x\n", hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC));
    HDMI_LOG( "HDMI_HDMI_AFCR_VADDR = %x\n", hdmi_rx_reg_read32(HDMI_HDMI_AFCR_VADDR));

    //  Hdmi_DumpState();
    if(GET_ISHDMI() == MODE_HDMI) {           //HDMI mode
        //Hdmi_DumpState();
        //hdmi_rx_reg_mask32(HDMI_HDMI_WDCR0_VADDR, ~(_BIT7),(_BIT7));       // Enable AVMute    WDG

        // Check Vedio Format change
        if (Hdmi_GetColorSpace() != GET_HDMI_COLOR_SPACE()) {
            HDMI_PRINTF("Color Space Change\n");
            SET_HDMI_COLOR_SPACE(Hdmi_GetColorSpace());
            HDMI_LOG( " Format = %d\n", GET_HDMI_COLOR_SPACE());
//          HDMISetScalerInfo();
//          HdmiSetScalerColor();
            return 2;
        };

        Hdmi_AudioModeDetect();

        if (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & _BIT6) {
            Hdmi_VideoOutputDisable();
            Hdmi_AudioOutputDisable();
            HDMI_PRINTF("AVMute ON----> Force re-detect signal\n");
            SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            SET_IS_FIRST_DETECT_MODE(1);
            return 1;
        }
    }

    if (GET_HDMI_ISINTERLACE() != Hdmi_GetInterlace(HDMI_MS_MODE_ONESHOT)) {   // if interlace mode change
            Hdmi_VideoOutputDisable();
            Hdmi_AudioOutputDisable();
            HDMI_PRINTF("IsInterlaced change\n");
            SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            SET_IS_FIRST_DETECT_MODE(1);
            return 2;
    }

    if (GET_ISHDMI()!= IsHDMI()) {
            Hdmi_VideoOutputDisable();
            Hdmi_AudioOutputDisable();
            HDMI_PRINTF("bIsHDMIDVI change %d %d\n", GET_ISHDMI(), IsHDMI());
            SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            SET_IS_FIRST_DETECT_MODE(1);
            return 2;
    }

    if ((unsigned int)GET_HDMI_CD()!= (hdmi_rx_reg_read32(HDMI_TMDS_DPC0_VADDR, HDMI_RX_MAC) & 0xf)) {
            Hdmi_VideoOutputDisable();
            Hdmi_AudioOutputDisable();
            HDMI_PRINTF("bIsColorDepth change\n");
            SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            SET_IS_FIRST_DETECT_MODE(1);
            return 2;
    }

    HdmiGetStruct(&isr_info);
    if (ABS(hdmi.b, isr_info.b) > 4) {
            Hdmi_VideoOutputDisable();
            Hdmi_AudioOutputDisable();
            HDMI_PRINTF("isr_info.b change\n");
            SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            SET_IS_FIRST_DETECT_MODE(1);
            return 2;
    };

    if (isr_info.b_change) {
            Hdmi_VideoOutputDisable();
            Hdmi_AudioOutputDisable();
            HDMI_PRINTF("b change = %d \n",isr_info.b);
            SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
            SET_IS_FIRST_DETECT_MODE(1);
            return 2;

    }

    if (hdmi_onms_measure(NULL) != HDMI_ERR_NO)
    {
                HDMI_PRINTF("hdmi online measure error\n");
                Hdmi_VideoOutputDisable();
                Hdmi_AudioOutputDisable();
                SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
                SET_IS_FIRST_DETECT_MODE(1);
                return 2;
    }

   return 0;
}

void drvif_Hdmi_EnableEDID(unsigned char channel_index, char enable) {
    char edid_type;

    if (channel_index >= HDMI_MAX_CHANNEL)  {
        HDMI_PRINTF("Wrong Channel Number: %d\n", channel_index);
        return;
    }
    HDMI_PRINTF("%s : %d %d\n", __FUNCTION__, channel_index, hdmi.channel[channel_index]->ddc_selected);
    edid_type = (char)hdmi.channel[channel_index]->edid_type;
    switch (edid_type) {
            case HDMI_EDID_ONCHIP:
                drvif_EDIDEnable(hdmi.channel[channel_index]->ddc_selected, enable);
                drvif_EDID_DDC12_AUTO_Enable(hdmi.channel[channel_index]->ddc_selected, 0);
            break;
            case HDMI_EDID_I2CMUX:
                HdmiMuxEDIDEnable(hdmi.channel[channel_index]->mux_ddc_port, enable);
            break;
            case HDMI_EDID_EEPROM:
            break;
    }


}
extern int Hdmi_get_b_value(void);
extern char Hdmi_IsbReady(void);
HDMI_ERR_T Hdmi_CheckConditionChange(void) {

    HDMIRX_IOCTL_STRUCT_T isr_info;
    UINT32 b;
    if (Hdmi_IsbReady() == TRUE)
    {
        b =  Hdmi_get_b_value();
    }
    HdmiGetStruct(&isr_info);
    if (isr_info.b < 10) {
        //HDMI_PRINTF("b < 10 change = %d  \n" ,isr_info.b);
        return HDMI_EER_GENERIC;
    }
    if (HDMI_ABS(hdmi.b, isr_info.b) > 4) {
        HDMI_PRINTF(" abs b > 4  change \n");
        return HDMI_EER_GENERIC;
    };

    if (isr_info.b_change) {
        HDMI_PRINTF("b change = %d  \n",isr_info.b);
        return HDMI_EER_GENERIC;
    }
    if ((unsigned int)GET_HDMI_CD() != (hdmi_rx_reg_read32(HDMI_TMDS_DPC0_VADDR, HDMI_RX_MAC) & 0xf)) {
        HDMI_PRINTF("color depth\n");
        return HDMI_EER_GENERIC;
    }

	if (GET_HDMI_ISINTERLACE()!= Hdmi_GetInterlace(HDMI_MS_MODE_ONESHOT)) {   // if interlace mode change
		HDMI_PRINTF("interlace change\n");
		hdmi_interlace_polarity_check = 1;
		return HDMI_EER_GENERIC;
	}

	if (GET_ISHDMI() != IsHDMI()) {
		HDMI_PRINTF("HDMI/DVI change\n");
		return HDMI_EER_GENERIC;
	}

    return HDMI_ERR_NO;
}

void Hdmi_HdcpInit(void)
{
    int i;

	if(!hdmi.hdcpkey_enable)
		return;

#if (defined ENABLE_HDCPKEY_ENDIAN_SWAP)
    unsigned char TmpData[4],k,j;

    if(drvif_Hdmi_GetHDCPKeySwap()==true){
        //pr_info("Hdmi_HdcpInit do Swap\n");
    //1st 4 bytes endian swap
        for(k=0;k<4;k++)
            TmpData[k] = hdmi.hdcpkey->BKsv[k];
        for(k=0;k<4;k++)
            hdmi.hdcpkey->BKsv[k] = TmpData[3-k];
    //2nd 4 bytes endian swap
        TmpData[0] = hdmi.hdcpkey->BKsv[4];
        TmpData[1] = hdmi.hdcpkey->Key[0];
        TmpData[2] = hdmi.hdcpkey->Key[1];
        TmpData[3] = hdmi.hdcpkey->Key[2];
        hdmi.hdcpkey->BKsv[4] = TmpData[3];
        hdmi.hdcpkey->Key[0] = TmpData[2];
        hdmi.hdcpkey->Key[1] = TmpData[1];
        hdmi.hdcpkey->Key[2] = TmpData[0];
    //3rd above 4 bytes endian swap
        for(j=0;j<80;j++) {
            for(k=0;k<4;k++)
                TmpData[k] = hdmi.hdcpkey->Key[3+k+j*4];
            for(k=0;k<4;k++)
                hdmi.hdcpkey->Key[3+k+j*4] = TmpData[3-k];
        }
    }
#endif

    // Disable HDCP and clear HDCP address
    hdmi_rx_reg_write32(HDMI_HDCP_CR_VADDR,0x06, HDMI_RX_MAC);
    hdmi_rx_reg_mask32(HDMI_HDCP_PCR_VADDR ,~(HDCP_PCR_km_clk_sel_mask|HDCP_PCR_enc_tog_mask),0, HDMI_RX_MAC);

    // write BKsv into DDC channel
#ifdef CONFIG_HDCP_KEY_PRINT
    HDMI_PRINTF("\n");
    for(i=0;i<5;i++) {
        HDMI_PRINTF("%x ", hdmi.hdcpkey->BKsv[i]);
    }
    HDMI_PRINTF("\n");
#endif //#ifdef CONFIG_HDCP_KEY_PRINT

    Hdmi_HdcpPortCWrite(0x00 , hdmi.hdcpkey->BKsv, 5 ); //set KSV 40 bits
    #if defined (ENABLE_HDMI_1_1_SUPPORT)
    Hdmi_HdcpPortWrite(0x40,HDCP_11_HDMI);              //set OESS for DVI
    #else
    Hdmi_HdcpPortWrite(0x40,HDCP_10_DVI);               //set OESS for DVI
    #endif

    /*Write device private key*/
    for(i=0;i<320;i++) {
#ifdef CONFIG_HDCP_KEY_PRINT
        if (i% 8 == 0) HDMI_PRINTF("\n");
        HDMI_PRINTF("%x ", hdmi.hdcpkey->Key[i]);
#endif //#ifdef CONFIG_HDCP_KEY_PRINT

        hdmi_rx_reg_write32(HDMI_HDCP_DKAP_VADDR,hdmi.hdcpkey->Key[i], HDMI_RX_MAC);
    }

    hdmi_rx_reg_write32(HDMI_HDCP_CR_VADDR,0x00, HDMI_RX_MAC);

    // enable HDCP function for all
    hdmi_rx_reg_mask32(HDMI_HDCP_CR_VADDR,~(_BIT7|_BIT0),(_BIT7|_BIT0), HDMI_RX_MAC);//for New_AC_Mode Enable,fix simplay bug

    Hdmi_HdcpPortWrite(0xc4,0x00); // keep old setting
}

void hdmi_phy_port_select(int port, char enable) {
        hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0_CTRL_2,  //Z0 resister
            ~(HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_ck_z0pow_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_b_z0pow_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_g_z0pow_mask|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_r_z0pow_mask),
            (HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_ck_z0pow(7)|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_b_z0pow(7)|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_g_z0pow(7)|HDMIPHY_REG_BANDGAP_Z0_CTRL_2_reg_r_z0pow(7)), HDMI_RX_PHY);
    pr_info("[port select %d,%d]\n",port,enable);
    switch (port)
    {
        case 0:
            hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~(HDMIPHY_REG_R_1_4_R_SCHR0|HDMIPHY_REG_R_1_4_R_SCHR1|HDMIPHY_REG_R_1_4_R_SCHR2),HDMIPHY_REG_R_1_4_R_SCHR0, HDMI_RX_PHY);                    //R input port sel
            hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~(HDMIPHY_REG_G_1_4_G_SCHR0|HDMIPHY_REG_G_1_4_G_SCHR1|HDMIPHY_REG_G_1_4_G_SCHR2),HDMIPHY_REG_G_1_4_G_SCHR0, HDMI_RX_PHY);                    //G input port sel
            hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(HDMIPHY_REG_B_1_4_B_SCHR0|HDMIPHY_REG_B_1_4_B_SCHR1|HDMIPHY_REG_B_1_4_B_SCHR2),HDMIPHY_REG_B_1_4_B_SCHR0, HDMI_RX_PHY);                    //B input port sel
            hdmi_rx_reg_mask32(HDMIPHY_REG_CK_1_4,~(HDMIPHY_REG_CK_1_4_CK_SCHR0|HDMIPHY_REG_CK_1_4_CK_SCHR1|HDMIPHY_REG_CK_1_4_CK_SCHR2),HDMIPHY_REG_CK_1_4_CK_SCHR0, HDMI_RX_PHY);               //CLK input port sel
            hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~(HDMIPHY_REG_PHY_CTL_CK_P2_ON|HDMIPHY_REG_PHY_CTL_CK_P1_ON|HDMIPHY_REG_PHY_CTL_CK_P0_ON),HDMIPHY_REG_PHY_CTL_CK_P0_ON, HDMI_RX_PHY);              //PMM CLK input detect sel
        break;
        case 1:
            hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~(HDMIPHY_REG_R_1_4_R_SCHR0|HDMIPHY_REG_R_1_4_R_SCHR1|HDMIPHY_REG_R_1_4_R_SCHR2),HDMIPHY_REG_R_1_4_R_SCHR1, HDMI_RX_PHY);                    //R input port sel
            hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~(HDMIPHY_REG_G_1_4_G_SCHR0|HDMIPHY_REG_G_1_4_G_SCHR1|HDMIPHY_REG_G_1_4_G_SCHR2),HDMIPHY_REG_G_1_4_G_SCHR1, HDMI_RX_PHY);                    //G input port sel
            hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(HDMIPHY_REG_B_1_4_B_SCHR0|HDMIPHY_REG_B_1_4_B_SCHR1|HDMIPHY_REG_B_1_4_B_SCHR2),HDMIPHY_REG_B_1_4_B_SCHR1, HDMI_RX_PHY);                    //B input port sel
            hdmi_rx_reg_mask32(HDMIPHY_REG_CK_1_4,~(HDMIPHY_REG_CK_1_4_CK_SCHR0|HDMIPHY_REG_CK_1_4_CK_SCHR1|HDMIPHY_REG_CK_1_4_CK_SCHR2),HDMIPHY_REG_CK_1_4_CK_SCHR1, HDMI_RX_PHY);               //CLK input port sel
            hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~(HDMIPHY_REG_PHY_CTL_CK_P2_ON|HDMIPHY_REG_PHY_CTL_CK_P1_ON|HDMIPHY_REG_PHY_CTL_CK_P0_ON),HDMIPHY_REG_PHY_CTL_CK_P1_ON, HDMI_RX_PHY);              //PMM CLK input detect sel
        break;
        case 2:
            hdmi_rx_reg_mask32(HDMIPHY_REG_R_1_4,~(HDMIPHY_REG_R_1_4_R_SCHR0|HDMIPHY_REG_R_1_4_R_SCHR1|HDMIPHY_REG_R_1_4_R_SCHR2),HDMIPHY_REG_R_1_4_R_SCHR2, HDMI_RX_PHY);                    //R input port sel
            hdmi_rx_reg_mask32(HDMIPHY_REG_G_1_4,~(HDMIPHY_REG_G_1_4_G_SCHR0|HDMIPHY_REG_G_1_4_G_SCHR1|HDMIPHY_REG_G_1_4_G_SCHR2),HDMIPHY_REG_G_1_4_G_SCHR2, HDMI_RX_PHY);                    //G input port sel
            hdmi_rx_reg_mask32(HDMIPHY_REG_B_1_4,~(HDMIPHY_REG_B_1_4_B_SCHR0|HDMIPHY_REG_B_1_4_B_SCHR1|HDMIPHY_REG_B_1_4_B_SCHR2),HDMIPHY_REG_B_1_4_B_SCHR2, HDMI_RX_PHY);                    //B input port sel
            hdmi_rx_reg_mask32(HDMIPHY_REG_CK_1_4,~(HDMIPHY_REG_CK_1_4_CK_SCHR0|HDMIPHY_REG_CK_1_4_CK_SCHR1|HDMIPHY_REG_CK_1_4_CK_SCHR2),HDMIPHY_REG_CK_1_4_CK_SCHR2, HDMI_RX_PHY);               //CLK input port sel
            hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~(HDMIPHY_REG_PHY_CTL_CK_P2_ON|HDMIPHY_REG_PHY_CTL_CK_P1_ON|HDMIPHY_REG_PHY_CTL_CK_P0_ON),HDMIPHY_REG_PHY_CTL_CK_P2_ON, HDMI_RX_PHY);              //PMM CLK input detect sel
        break;
        default:
            HDMI_PRINTF("unknow port\n");
        break;
    }

    // for DFE PHY
    hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~(HDMIPHY_REG_PHY_CTL_ck_vcorstb_mask|HDMIPHY_REG_PHY_CTL_ck_pllrstb_mask|HDMIPHY_REG_PHY_CTL_ck_md_rstb_mask),0, HDMI_RX_PHY);	//PLL clk reset
    HDMI_DELAYMS(1);
    hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~(HDMIPHY_REG_PHY_CTL_ck_vcorstb_mask|HDMIPHY_REG_PHY_CTL_ck_pllrstb_mask|HDMIPHY_REG_PHY_CTL_ck_md_rstb_mask),
                                                                                              (HDMIPHY_REG_PHY_CTL_ck_vcorstb_mask|HDMIPHY_REG_PHY_CTL_ck_pllrstb_mask|HDMIPHY_REG_PHY_CTL_ck_md_rstb_mask), HDMI_RX_PHY);	//PLL clk reset
}


void drvif_Hdmi_PhyPortEnable(unsigned char channel, char enable) {

    if (channel >= HDMI_MAX_CHANNEL)  {
        HDMI_PRINTF("Wrong Channel Number: %d\n", channel);
        return;
    }
    hdmi_phy_port_select(hdmi.channel[channel]->phy_port, enable);

    HDMI_PRINTF("%s : %d %d\n", __FUNCTION__, channel, hdmi.channel[channel]->ddc_selected);
    if (hdmi.channel[channel]->mux_port  != HDMI_MUX_PORT_NOTUSED)  {
            HdmiMux_PhyPortEnable(hdmi.channel[channel]->mux_port , enable);
    }


}

HDMI_BOOL Hdmi_MacInit(void)
{
    hdmi_rx_reg_write32(HDMI_HDMI_SCR_VADDR , 0x122, HDMI_RX_MAC);//DVI/HDMI condition(A,B) select
    hdmi_rx_reg_write32(HDMI_HDMI_AFCR_VADDR , 0x06, HDMI_RX_MAC);//Enable Audio FIFO
    hdmi_rx_reg_write32(HDMI_HDMI_AVMCR_VADDR , 0x48, HDMI_RX_MAC);//enable video & audio output
    hdmi_rx_reg_write32(HDMI_HDMI_WDCR0_VADDR , 0x00, HDMI_RX_MAC);  //disable watch dog
    hdmi_rx_reg_write32(HDMI_HDMI_BCHCR_VADDR , 0x19, HDMI_RX_MAC); // Enable BCH Function
    hdmi_rx_reg_write32(HDMI_HDMI_PVGCR0_VADDR , 0x09, HDMI_RX_MAC);    //For HDMI Packet
#if 1 //fix panasonic issue((dvd mode + hdcp) fail)
    //hdmi_rx_reg_mask32(HDMI_HDCP_PCR_VADDR, ~HDCP_PCR_km_clk_sel_mask,0);
    hdmi_rx_reg_mask32(HDMI_HDCP_PCR_VADDR, ~(HDCP_PCR_km_clk_sel_mask|HDCP_PCR_hdcp_vs_sel_mask),HDCP_PCR_hdcp_vs_sel_mask, HDMI_RX_MAC);// vsync virtual
#endif
    hdmi_rx_reg_mask32(HDMI_HDMI_VCR_VADDR, ~(HDMI_VCR_csam_mask | HDMI_VCR_prdsam_mask), HDMI_VCR_csam(1) | HDMI_VCR_prdsam(1), HDMI_RX_MAC);
//  hdmi_rx_reg_mask32(HDMI_HDMI_AOCR_VADDR,(unsigned char)~(0xff),0x00);//Disable SPDIF/I2S Output
//  hdmi_rx_reg_mask32(HDMI_HDMI_AOCR_VADDR,~_BIT10,_BIT10);//Hold avmute value outside v-sync region
    hdmi_rx_reg_write32(HDMI_HDMI_MAGCR0_VADDR, 0xE000/*0x14*/, HDMI_RX_MAC);
       hdmi_rx_reg_write32(HDMI_HDMI_PTRSV1_VADDR, 0x82, HDMI_RX_MAC);
    // clear HDMI interrupt control register
    hdmi_rx_reg_write32(HDMI_HDMI_INTCR_VADDR, 0, HDMI_RX_MAC);
    hdmi_rx_reg_mask32(HDMI_HDMI_VCR_VADDR, ~(_BIT7) , 0, HDMI_RX_MAC);  // not inverse EVEN/ODD

    hdmi_rx_reg_mask32(HDMI_HDMI_PAMICR_VADDR ,~_BIT0, 0x00, HDMI_RX_MAC);//Disable packet variation Watch Dog
    //hdmi_rx_reg_mask32(HDMI_HDMI_AOCR_VADDR,0x00, 0x00);//Disable SPDIF/I2S Output
    hdmi_rx_reg_mask32(HDMI_HDMI_AOCR_VADDR,(~0x0ff),0xFF, HDMI_RX_MAC); //Enable SPDIF/I2S Output

#if 0
    //Magellan_clock set
       hdmi_rx_reg_write32(HDMI_GDI_TMDSCLK_SETTING_00,0x10010, HDMI_RX_CLOCK_DETECT); //Set TC
       hdmi_rx_reg_write32(HDMI_GDI_TMDSCLK_SETTING_01,0x7800047, HDMI_RX_CLOCK_DETECT); //clk detect disable
    //
#endif

    hdmi_rx_reg_mask32 (HDMI_TMDS_ERRC_VADDR,~TMDS_ERRC_nl_mask,TMDS_ERRC_nl(4), HDMI_RX_MAC); // cycle debounce
    //cloud modify Mac2 bug 20130716
    if (Debug_FIFOclock==2)
    	hdmi_rx_reg_mask32(HDMI_TMDS_OUTCTL_VADDR ,~TMDS_OUTCTL_clk25xinv_r_mask,TMDS_OUTCTL_clk25xinv_r(CLOCK_INVERT), HDMI_RX_MAC);	//R channel input clock positive
    else if (Debug_FIFOclock==1)
    	hdmi_rx_reg_mask32(HDMI_TMDS_OUTCTL_VADDR ,~TMDS_OUTCTL_clk25xinv_g_mask,TMDS_OUTCTL_clk25xinv_g(CLOCK_INVERT), HDMI_RX_MAC);	//G channel input clock positive
    else
    	hdmi_rx_reg_mask32(HDMI_TMDS_OUTCTL_VADDR ,~TMDS_OUTCTL_clk25xinv_b_mask,TMDS_OUTCTL_clk25xinv_b(CLOCK_INVERT), HDMI_RX_MAC);	//B channel input clock positive

    //BCH function enable
    hdmi_rx_reg_mask32(HDMI_HDMI_BCHCR_VADDR,~(HDMI_BCHCR_enrwe_mask|HDMI_BCHCR_bche_mask),(HDMI_BCHCR_enrwe(1)|HDMI_BCHCR_bche(1)), HDMI_RX_MAC);   // BCH function enable

    return TRUE;
}

HDMI_BOOL  Hdmi_TmdsInit(void)
{
	//initial HDMI
	hdmi_rx_reg_mask32(MHL_DEMUX_CTRL_reg,~(MHL_DEMUX_CTRL_mhl_pp_en_mask|MHL_DEMUX_CTRL_mhl_en_mask),0, HDMI_RX_MHL);
	hdmi_rx_reg_mask32(MHL_HDMI_MAC_CTRL,~(MHL_HDMI_MAC_CTRL_pp_mode_output_mask|MHL_HDMI_MAC_CTRL_packet_mhl_en_mask|MHL_HDMI_MAC_CTRL_xor_pixel_sel_mask|MHL_HDMI_MAC_CTRL_ch_dec_pp_mode_en_mask),0, HDMI_RX_MAC);

	//for BCH & debounce
	hdmi_rx_reg_mask32(HDMI_TMDS_ERRC_VADDR,~TMDS_ERRC_nl_mask,TMDS_ERRC_nl(4), HDMI_RX_MAC);		// 1+8 cycle debounce + de masking transition of vs/hs + masking first 8-line de
	hdmi_rx_reg_mask32(HDMI_HDMI_BCHCR_VADDR,~(HDMI_BCHCR_enrwe_mask|HDMI_BCHCR_bche_mask),(HDMI_BCHCR_enrwe(1)|HDMI_BCHCR_bche(1)), HDMI_RX_MAC);	// BCH function enable

	//hdmi_rx_reg_mask32(TMDS_PWDCTL_reg,~(TMDS_PWDCTL_ebip_mask|TMDS_PWDCTL_egip_mask|TMDS_PWDCTL_erip_mask|TMDS_PWDCTL_ecc_mask),(TMDS_PWDCTL_ebip(1)|TMDS_PWDCTL_egip(1)|TMDS_PWDCTL_erip(1)|TMDS_PWDCTL_ecc(1)));				//enable TMDS input
	hdmi_rx_reg_mask32(HDMI_TMDS_PWDCTL_VADDR,~(TMDS_PWDCTL_ebip_mask|TMDS_PWDCTL_egip_mask|TMDS_PWDCTL_erip_mask|TMDS_PWDCTL_ecc_mask),0, HDMI_RX_MAC);				//enable TMDS input
	//HDMI_PRINTF("0xb800d01c=0x%x--MAC reset\n",rtd_inl(TMDS_PWDCTL_reg));

	//hdmi_rx_reg_mask32(TMDS_Z0CC_reg,~TMDS_Z0CC_hde_mask,TMDS_Z0CC_hde(1));					//HDMI&DVI function enable
	hdmi_rx_reg_mask32(HDMI_TMDS_Z0CC_VADDR,~TMDS_Z0CC_hde_mask,0, HDMI_RX_MAC);					//HDMI&DVI function disable
	//HDMI_PRINTF("0xb800d020=0x%x--MAC reset\n",rtd_inl(TMDS_Z0CC_reg));
#ifdef Debug_LDO
	//Debug low yield
	hdmi_rx_reg_mask32(HDMIPHY_REG_PHY_CTL,~HDMIPHY_REG_PHY_CTL_ldo_ref_mask,HDMIPHY_REG_PHY_CTL_ldo_ref(7), HDMI_RX_PHY);	//LDO 1.2V
	//HDMI_PRINTF("0xb800db44=0x%x--LDO level\n",rtd_inl(HDMIPHY_REG_PHY_CTL_reg));
#endif

	hdmi_rx_reg_mask32(HDMI_HDMI_VCR_VADDR,~HDMI_VCR_iclk_sel_mask,HDMI_VCR_iclk_sel(0), HDMI_RX_MAC);
	hdmi_rx_reg_mask32(HDMI_TMDS_CPS_VADDR,~TMDS_CPS_clkv_meas_sel_mask,TMDS_CPS_clkv_meas_sel(3), HDMI_RX_MAC);		//measure input clock source sel
	hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR,~HDMI_AVMCR_ve_mask,HDMI_AVMCR_ve(1), HDMI_RX_MAC);					//need to enable video before enable TMDS clock
	hdmi_rx_reg_mask32(HDMI_TMDS_OUTCTL_VADDR,~(TMDS_OUTCTL_ocke_mask|TMDS_OUTCTL_tbcoe_mask|TMDS_OUTCTL_tgcoe_mask|TMDS_OUTCTL_trcoe_mask),(TMDS_OUTCTL_ocke(1)|TMDS_OUTCTL_tbcoe(1)|TMDS_OUTCTL_tgcoe(1)|TMDS_OUTCTL_trcoe(1)), HDMI_RX_MAC);		//enable TMDS output
	hdmi_rx_reg_mask32(HDMI_TMDS_DPC_SET0_VADDR,~TMDS_DPC_SET0_dpc_bypass_dis_mask,TMDS_DPC_SET0_dpc_en(1), HDMI_RX_MAC);					// video function block enable

	return TRUE;
}

unsigned char get_HDMI_AFD(void) {
#ifdef CONFIG_ENABLE_HDMI_AUTO_CHANGE_DISPLAY_RATIO
    UINT8 ucResult,ucAVI;
    hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 0x04, HDMI_RX_MAC);
     ucAVI = hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC);
    if ((ucAVI&0x10)== 0) // Active format no data
        return 0;
    hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 0x05, HDMI_RX_MAC);
    ucAVI = hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC);
    ucResult = (ucAVI >>4)&0x03; // Picture aspect ratio
    //HDMI_PRINTF("AFD = %x\n", ucResult);
    return ucResult;
#else //#ifdef CONFIG_ENABLE_HDMI_AUTO_CHANGE_DISPLAY_RATIO
#ifdef BUILD_TV005_1_ISDB
    hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 0x0, HDMI_RX_MAC);
    if (hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC) != 0) return 0;
    hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 0x5, HDMI_RX_MAC);
    //HDMI_PRINTF("AFD = %d\n", hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR));
    hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 0x5, HDMI_RX_MAC);
    return (hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC));
#else
      UINT8 ucResult1,ucResult2,ucAVI;
    hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 0x04, HDMI_RX_MAC);
     ucAVI = hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC);
    if ((ucAVI&0x10)== 0) // Active format no data
        return 0;
    hdmi_rx_reg_write32(HDMI_HDMI_PSAP_VADDR, 0x05, HDMI_RX_MAC);
    ucAVI = hdmi_rx_reg_read32(HDMI_HDMI_PSDP_VADDR, HDMI_RX_MAC);
    HDMI_PRINTF("HDMI AVI info AFD = %x\n", ucAVI);
    ucResult1 = (ucAVI >>4)&0x03; // Picture aspect ratio
    ucResult2 = ucAVI &0x0F;
    //cloud mark for complier code sync magellan
    /*
    if(ucResult1 == 0x00)
    {
          HDMI_PRINTF("SLR_AFD_RATIO_NO_DATA\n");
        return  SLR_AFD_RATIO_NO_DATA;
    }
    else if(ucResult1 == 0x01)    //  4D3
    {
         if((ucResult2 == 0x08)||(ucResult2 == 0x09))
         {
          HDMI_PRINTF("SLR_AFD_RATIO_4_3\n");
              return  SLR_AFD_RATIO_4_3;
         }
         else if((ucResult2 == 0x0A))  //  4D3  & 16D9
         {
           HDMI_PRINTF("SLR_AFD_RATIO_4_3_AFAR_16_9\n");
              return  SLR_AFD_RATIO_4_3_AFAR_16_9;
         }
         else if ((ucResult2 == 0x0B))
         {
         HDMI_PRINTF("SLR_AFD_RATIO_4_3_AFAR_14_9\n");
              return  SLR_AFD_RATIO_4_3_AFAR_14_9;
         }
    }
    else if(ucResult1 == 0x02)    // 16D9
    {
         if((ucResult2 == 0x08)||(ucResult2 == 0x0A))
         {
         HDMI_PRINTF("SLR_AFD_RATIO_16_9\n");
              return  SLR_AFD_RATIO_16_9;
         }
         else if((ucResult2 == 0x09))  //  16D9  & 16D9
         {
         HDMI_PRINTF("SLR_AFD_RATIO_16_9AFAR_4_3\n");
              return  SLR_AFD_RATIO_16_9AFAR_4_3;
         }
         else if ((ucResult2 == 0x0B))
         {
         HDMI_PRINTF("SLR_AFD_RATIO_16_9AFAR_14_9\n");
              return  SLR_AFD_RATIO_16_9AFAR_14_9;
         }
    }

    HDMI_PRINTF("SLR_AFD_RATIO_MAX\n");
    return SLR_AFD_RATIO_MAX;
    */
    return 1 ;
#endif
#endif //#ifdef CONFIG_ENABLE_HDMI_AUTO_CHANGE_DISPLAY_RATIO
}


#ifdef CRC_CHECK
int HDMI_FormatDetect(void)
{
	if (hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC)&HDMI_SR_mode_mask)
		return TRUE;
	else
	{
	    pr_info("[DVI  mode]\n");
		return FALSE;
	}
}

int Hdmi_CRC_en(void)
{
	UINT8 tmo=50;
	// read CRC value first
	hdmi_rx_reg_mask32(HDMI_TMDS_CRCC_VADDR,~TMDS_CRCC_crcc_mask,0, HDMI_RX_MAC);	//CRC check disable
	hdmi_rx_reg_mask32(HDMI_TMDS_CRCC_VADDR,~TMDS_CRCC_crcc_mask,TMDS_CRCC_crcc(1), HDMI_RX_MAC);	//CRC check enable

	while(!(hdmi_rx_reg_read32(HDMI_TMDS_CRCC_VADDR, HDMI_RX_MAC)&TMDS_CRCC_crc_done_mask))//wait CRC done bit
	{
		HDMI_DELAYMS(10);
		if (!tmo--)
		{
			//HDMI_PRINTF("[HDMI] WARNING: FAIL: Wait CRC Timeout !!!\n");
			return FALSE;
		}
	}
	return TRUE;
}

// type=0: HDMI; type=1: MHL
int QC_CRCCheck(UINT8 type)
{
#define CRC_COUNT				(10)	// In general test every 100ms, crc check loop count.  (ex. 10*60*60*n mins)
	int is_interlace;
	UINT32 i, CRC_READ[3][2];
//	UINT8  FalseOne=0, interlace=0,timeout = 0;
	UINT32 err_cnt_crc = 0, err_cnt_rgbhv = 0;
	int CRC_result = 0xcc;

#if 1//RGBlane == '3'
	if (!(TMDS_CTRL_get_bcd(hdmi_rx_reg_read32(HDMI_TMDS_CTRL_VADDR, HDMI_RX_MAC))&&TMDS_CTRL_get_gcd(hdmi_rx_reg_read32(HDMI_TMDS_CTRL_VADDR, HDMI_RX_MAC))&&TMDS_CTRL_get_rcd(hdmi_rx_reg_read32(HDMI_TMDS_CTRL_VADDR, HDMI_RX_MAC))))
#else
	if (!TMDS_CTRL_get_bcd(hdmi_rx_reg_read32(HDMI_TMDS_CTRL_VADDR, HDMI_RX_MAC)))
#endif
	{
		pr_info("[HDMI] No Input Signal\n");
		CRC_result = FALSE;
		return CRC_result;
	}
	HDMI_DELAYMS((type==1)?3500:500); //HDMI=500; MHL=3500

	if ( !Hdmi_CRC_en() )
	{
		pr_info("[HDMI] WARNING: FAIL: Wait CRC Timeout !!!\n");
		CRC_result = FALSE;
		return CRC_result;
	}
	CRC_READ[0][0]=hdmi_rx_reg_read32(HDMI_TMDS_CRCO0_VADDR, HDMI_RX_MAC);
	CRC_READ[0][1]=hdmi_rx_reg_read32(HDMI_TMDS_CRCO1_VADDR, HDMI_RX_MAC);
	CRC_READ[1][0] = CRC_READ[0][0];
	CRC_READ[1][1] = CRC_READ[0][1];
	pr_info("[HDMI] CRC_READ[0]=0x%08x |0x%08x\n", CRC_READ[0][0], CRC_READ[0][1]);

	if (CRC_READ[0][0]==0x0)
	{
		CRC_result = FALSE;
		return CRC_result;
	}
	if ( (is_interlace=(hdmi_rx_reg_read32(HDMI_HDMI_VCR_VADDR, HDMI_RX_MAC) & HDMI_VCR_eot_mask)) ) // if interlace, get 2 crc values
	{
		for ( i=0; i<100; i++ )
		{
			CRC_READ[2][0]=hdmi_rx_reg_read32(HDMI_TMDS_CRCO0_VADDR, HDMI_RX_MAC);
			CRC_READ[2][1]=hdmi_rx_reg_read32(HDMI_TMDS_CRCO1_VADDR, HDMI_RX_MAC);
			if ( (CRC_READ[0][0] != CRC_READ[2][0])  || (CRC_READ[0][1] !=CRC_READ[2][1])  )
			{
				CRC_READ[1][0] = CRC_READ[2][0];
				CRC_READ[1][1] = CRC_READ[2][1];
				pr_info("[HDMI] CRC_READ[1]=0x%08x|0x%08x\n", CRC_READ[1][0], CRC_READ[1][1]);
				break;
			}
			HDMI_DELAYMS(10);
		}
	}

	hdmi_rx_reg_mask32(HDMI_TMDS_CRCC_VADDR,~TMDS_CRCC_crc_nonstable_mask,TMDS_CRCC_crc_nonstable(1), HDMI_RX_MAC);	//CRC unstable clear
	HDMI_DELAYMS(10);
	i = 0;
	do {
		if (CRC_COUNT>20)// && GETCHAR_ESC)
			break;

		if ( is_interlace )
		{
			CRC_READ[2][0]=hdmi_rx_reg_read32(HDMI_TMDS_CRCO0_VADDR, HDMI_RX_MAC);
			CRC_READ[2][1]=hdmi_rx_reg_read32(HDMI_TMDS_CRCO1_VADDR, HDMI_RX_MAC);
			if (( (CRC_READ[0][0] != CRC_READ[2][0])  || (CRC_READ[0][1] !=CRC_READ[2][1])  ) &&
				( (CRC_READ[1][0] != CRC_READ[2][0])  || (CRC_READ[1][1] !=CRC_READ[2][1])  ) )
			{
				err_cnt_crc++;
			}
		}
		else if (hdmi_rx_reg_read32(HDMI_TMDS_CRCC_VADDR, HDMI_RX_MAC)&TMDS_CRCC_crc_nonstable_mask)
		{
			hdmi_rx_reg_mask32(HDMI_TMDS_CRCC_VADDR,~TMDS_CRCC_crc_nonstable_mask,TMDS_CRCC_crc_nonstable(1), HDMI_RX_MAC);	//CRC unstable clear
			err_cnt_crc++;
		}
#if 1//RGBlane == '3'
		else if ( hdmi_rx_reg_read32(HDMI_TMDS_CTRL_VADDR, HDMI_RX_MAC) != 0xf8 )	//no r,g,b,h,v?
		{
			err_cnt_rgbhv++;
		}
#endif

		i++;
		//HDMI_PRINTF("[HDMI] Pass: %d; crc failed=%d, no data=%d\r", (i-err_cnt_crc-err_cnt_rgbhv), err_cnt_crc, err_cnt_rgbhv);
		HDMI_DELAYMS(100);
	}while( i<CRC_COUNT );
	pr_info("\n");

	if ( err_cnt_crc|err_cnt_rgbhv)
	{
		CRC_result = FALSE;
		return CRC_result;
	}
	else
	{
		return CRC_result;
	}

}
#endif

void drvif_Hdmi_InitSrc(unsigned char channel)
{
    //pr_info("%s:%d\n", __func__, __LINE__);
    HDMI_PRINTF("%s %d\n", "drvif_Hdmi_InitSrc", channel);

    if (channel >= HDMI_MAX_CHANNEL)  {
        HDMI_PRINTF("Wrong Channel Number: %d\n", channel);
        return;
    }

    //pr_info("[1.~~~check,%x,%x,%x]\n",channel,(hdmi.channel[channel])->ddc_selected,hdmi.channel[channel]->phy_port);
    if(hdmi.channel[channel] == 0)
        return;

    if(hdmi.channel[channel]->phy_port == HDMI_PHY_PORT_NOTUSED )
    {
        HDMI_PRINTF("This channel %d is Disable\n", channel);
        return;
    }


    if (GET_HDMI_FASTER_SWITCH_EN() || (GET_HDMI_FASTER_BOOT_EN() && GET_HDMI_BOOT_FIRST_RUN())) {
    if (channel == GET_HDMI_CHANNEL()) {
        HDMI_PRINTF("drvif_Hdmi_InitSrc bypass\n");
        goto initsrc_bypass;
        }
    }

    HdmiSetPreviousSource(channel);

    hdmi_phy_port_select(0, 1);
    //pr_info("[1.~~~check,%x,%x]\n",channel,(hdmi.channel[channel])->ddc_selected);
    hdmi_rx_reg_mask32(HDMI_HDMI_VCR_VADDR, ~HDMI_VCR_hdcp_ddc_chsel_mask, HDMI_VCR_hdcp_ddc_chsel(channel), HDMI_RX_MAC);

    Hdmi_Power(1);

    Hdmi_TmdsInit();
    Hdmi_MacInit();

	if (channel < HDMI_MAX_CHANNEL) {
        HdmiMuxSel(hdmi.channel[channel]->mux_ic);
        drvif_Hdmi_PhyPortEnable(hdmi.channel[channel]->phy_port, 0);
        drvif_Hdmi_HPD(channel , 0);
        drvif_Hdmi_EnableEDID(channel, 0);
        HDMI_DELAYMS(HDMI_HPD_INSRC_TIME);  //HDMI_HPD_INSRC_TIME
        drvif_Hdmi_EnableEDID(channel , 1);
        drvif_Hdmi_HPD(channel , 1);
        drvif_Hdmi_PhyPortEnable(hdmi.channel[channel]->phy_port, 1);
    } else {
        HDMI_PRINTF("HDMI assigned channel excess maximun Channel\n");
    }

#if HDMI_DEBUG_AUDIO_PLL_RING
    HdmiAudioPLLSampleStart();
#endif

initsrc_bypass:
#if 1
    // setup Audio PLL
    Hdmi_AudioOutputDisable();
//  Hdmi_AudioPLLSetting(48000, 0);
    // reset hdmi struct
    SET_HDMI_BOOT_FIRST_RUN(0);
    SET_IS_FIRST_DETECT_MODE(1);
    SET_HDMI_COLOR_SPACE(COLOR_RGB);
    SET_HDMI_ISINTERLACE(0);
    SET_ISHDMI(MODE_DVI);
    SET_HDMI_CD(0);
    SET_HDMI_CHANNEL(channel);
    // reset FSM
    SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);
    SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);
#endif
    hdmi.b = 0;
    hdmi.tx_timing.hdmi_3dformat = HDMI3D_2D_ONLY;
//    Hdmi_GetVSyncCountInit();
//    HdmiISREnable(1);  // enable 10ms timer ISR for PHY adjustment

}

void drvif_Hdmi_Init_CRT_INIT(void)
{
    // reset register state to init
    hdmi_rx_reg_mask32(HDMIPHY_REG_BANDGAP_Z0,~HDMIPHY_REG_BANDGAP_Z0_bgpow_mask,HDMIPHY_REG_BANDGAP_Z0_bgpow(1), HDMI_RX_PHY);		//BG power on (Bit25)
    // effect koffset result
    hdmi_rx_reg_write32(HDMIPHY_REG_R_1_4, 0, HDMI_RX_PHY);
    hdmi_rx_reg_write32(HDMIPHY_REG_G_1_4, 0, HDMI_RX_PHY);
    hdmi_rx_reg_write32(HDMIPHY_REG_B_1_4, 0, HDMI_RX_PHY);
    hdmi_rx_reg_write32(HDMIPHY_REG_R_5_8, R_EQ_bias_current(4), HDMI_RX_PHY);			//R EQ bias 6 for over 1.1GHz
    hdmi_rx_reg_write32(HDMIPHY_REG_G_5_8, G_EQ_bias_current(4), HDMI_RX_PHY);			//G EQ bias 6 for over 1.1 GHz
    hdmi_rx_reg_write32(HDMIPHY_REG_B_5_8, B_EQ_bias_current(4), HDMI_RX_PHY);			//B EQ bias 6 for over 1.1 GHz
}

/**
 * drvif_Hdmi_Init
 * initial function for HDMI
 *
 * @param               {  }
 * @return              {  }
 * @ingroup drv_hdmi
 */
void drvif_Hdmi_Init(void)
{
    int j;

    HDMI_PRINTF("%s \n", "drvif_Hdmi_Init");

    hdmi_rx_reg_write32(HDMI_HDCP_FLAG2_VADDR, 0, HDMI_RX_MAC);

    hdmi.fast_boot_source = -1;

	for (j=0; j<HDMI_MAX_CHANNEL; j++)
	{
		hdmi.channel[j]->ddc_selected= j;
		hdmi.channel[j]->phy_port = (HDMI_PHY_PORT_T) j;
	}

    drvif_Hdmi_Init_CRT_INIT();
    // fill out EDID table
    for (j=0; j<HDMI_MAX_CHANNEL; j++) {
        if (hdmi.channel[j] == 0) continue;
        if (hdmi.channel[j]->phy_port == HDMI_PHY_PORT_NOTUSED ) continue;

        if (hdmi.channel[j]->edid_initialized == 0 && hdmi.channel[j]->edid_type != HDMI_EDID_EEPROM) {
            drvif_Hdmi_LoadEDID(j, hdmi.channel[j]->EDID, 256);
        }
    }
//    HdmiISRInit();
    SET_HDMI_CHANNEL(HDMI_MAX_CHANNEL);
    SET_HDMI_FASTER_BOOT_EN(HDMI_FASTER_BOOT);
    SET_HDMI_FASTER_SWITCH_EN(HDMI_FASTER_SWITCH);
    SET_HDMI_BOOT_FIRST_RUN(1);

    if (GET_HDMI_FASTER_BOOT_EN()) {
        HdmiGetFastBootStatus();
        if (hdmi.fast_boot_source >= 0 && hdmi.fast_boot_source < HDMI_MAX_CHANNEL) {
            HDMI_PRINTF("bypass hdmi_rx_reg_read32it\n");
            SET_HDMI_CHANNEL(hdmi.fast_boot_source);
            // all hot plug high
            for (j=0; j<HDMI_MAX_CHANNEL; j++) {
                if (j == hdmi.fast_boot_source) continue;
                if (hdmi.channel[j] == 0) continue;
                if (hdmi.channel[j]->phy_port == HDMI_PHY_PORT_NOTUSED ) continue;
                if (hdmi.channel[j]->HotPlugPin < 0) continue;
#if HDMI_HOTPLUG_DEBUG
                drvif_Hdmi_HPD(j, 1);
                drvif_Hdmi_EnableEDID(j, 1);
                drvif_Hdmi_PhyPortEnable(hdmi.channel[j]->phy_port, 0);
#else
                drvif_Hdmi_HPD(j, 0);
                drvif_Hdmi_PhyPortEnable(hdmi.channel[j]->phy_port, 0);
#endif
            }
            return;
        }
    }

    SET_HDMI_FASTER_BOOT_EN(0); // there is no Fast boot info at filesystem

    Hdmi_HdcpInit();
    //cloud modify MD_SEL change function in addr :DA00 [15]  CKSEL
    //hdmi_rx_reg_mask32(HDMIPHY_REG_CK_5_8,~(_BIT19),(_BIT19)); //(MD_SEL) Mode detect source clk select from PMM
    //EQ_KOFF_Cali();
    Hdmi_PhyInit();

}

