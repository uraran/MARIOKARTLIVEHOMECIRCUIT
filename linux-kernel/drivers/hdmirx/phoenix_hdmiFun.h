#ifndef PHOENIX_HDMI_FUN_H
#define PHOENIX_HDMI_FUN_H

/********************************************************************************************
						HDMI Driver Mode Config : important !!!!!!
********************************************************************************************/
#define HDMI_FIX_GREEN_LINE 		   	1  	   	// mask green line into black when measure result is error

#define HDMI_AUTO_DEONLY		   		0		// auto DE_ONLY mode decision flow
#define HDMI_DE_ONLY  		   0		// manual control of DE_ONLY mode , avaliable when HDMI_AUTO_DEONLY = 0
#define HDMI_CRC_DUMP			   0		// Enable HDMI CRC value dump
#define HDMI_RUNTIME_AUDIO_PLL_CAL         0 		// Enable Runtime Audio PLL parameter caculation
#define HDMI_BULD_IN_TABLE 		   0		// 1: channel table define by code 0: load by drvif_Hdmi_regTable
#if defined(ENABLE_HDMIRX_NONPCM)
#define HDMI_SUPPORT_NONPCM		   1		// 1: SUPPROT_NONPCM
#else
#define HDMI_SUPPORT_NONPCM		   0		// 1: SUPPROT_NONPCM
#endif
#define HDMI_MAX_CHANNEL		   			1
#define HDMI_DEBUG_AUDIO_PLL_RING	   	0
#define AUDIO_PLL_RING_SIZE		   		300
#define HDMI_B_TEST_COUNT_MAX 		   	4
#define HDMI_RTD_XTAL  (27000UL)
#define HDMI_PHY_IDLE_PATCH_ENABLE		1
#define HDMI_PHY_IDLE_PATCH_COUNT   		5			// 1 sec
#define HDMI_HOTPLUG_DEBUG				0
#define LG_DF9921N						(0)//20091127 kist
#define	LG_DF9921N_PWR_ON				0
#define	K6258_Repeater				0
#define SIMPLAY_TEST					0//20091222 kist
#define HDMI_DEBUG_COLOR_SPACE		0
#define HDMI_SUPPORT_HBR				0
#define HDMI_NCTS_BND_TRD_TRACKING	1
#define HDMI_MEMSURE_RETRY			1
#define HDMI_FASTER_BOOT 				0
#define HDMI_FASTER_SWITCH			0
#define HDMI_SEARCH_MODE_START_INDEX		_MODE_480I-_MODE_480I
#define HDMI_SEARCH_MODE_END_INDEX			_MODE_1080P30-_MODE_480I
#define HDMI_PHY_AT_USERLAND			1
#define HDMI_OLD_CLK_DETECT			0

#define MHL_SUPPORT    0

#ifdef BUILD_TV005_1_ATV_PATCH_LIB_FROM_MP_BRANCH_SVN_434497_437131
#ifdef	HDMI_SWITCH_SI9199
#define HDMI_AUTO_EQ 				0
#else	//HDMI_SWITCH_SI9199
#define HDMI_AUTO_EQ 				1
#endif	//HDMI_SWITCH_SI9199
#endif

#ifdef HDMI_SWITCH_ITE6633
#ifdef TV032_1
#define HDMI_AUTO_EQ 				0
#else
#define HDMI_AUTO_EQ 				1
#endif
#else
#define HDMI_AUTO_EQ 				1
#endif
#define HDCP_CTS_SUPPORT			1
#define HDMI_GB_DETECT	                   0   // cloud add function

/****************************************************************************/

typedef enum {

	HDMI3D_FRAME_PACKING = 0,
	HDMI3D_FIELD_ALTERNATIVE = 1,
	HDMI3D_LINE_ALTERNATIVE = 2,
	HDMI3D_SIDE_BY_SIDE_FULL = 3,
	HDMI3D_L_DEPTH = 4,
	HDMI3D_L_DEPTH_GPX = 5,
	HDMI3D_TOP_AND_BOTTOM =6,
	HDMI3D_FRAMESEQUENCE=7,
	HDMI3D_SIDE_BY_SIDE_HALF = 8,
	// --- new 3D format ---
	HDMI3D_VERTICAL_STRIPE,
	HDMI3D_CHECKER_BOARD,
	HDMI3D_REALID,
	HDMI3D_SENSIO,
	// -------------------
	HDMI3D_RSV1,
	HDMI3D_2D_ONLY,
	HDMI3D_UNKOWN = 0xFFFFFFFF
} HDMI_3D_T;

typedef enum _HDMI_TABLE_TYPE{
	HDMI_CHANNEL0 = 0,
	HDMI_CHANNEL1,
	HDMI_CHANNEL2,	
	HDMI_HDCP_KEY_TABLE,
	CBUS_TABLE,
}HDMI_TABLE_TYPE;

enum PHOENIX_MODE_RESULT
{
	PHOENIX_MODE_NOSIGNAL	= 0xFF,
	PHOENIX_MODE_NOSUPPORT	= 0xFE,
	PHOENIX_MODE_NOCABLE		= 0xFD,
	PHOENIX_MODE_SUCCESS		= 0xFC,
	PHOENIX_MODE_DETECT		= 0xFB,
	PHOENIX_MDOE_UNDKOWN	= 0xFA,
};


typedef void (*HDMI_MUX_ADVANCED_FUNCTION)(unsigned char index, unsigned int Value_1, unsigned int Value_2, unsigned int Value_3);

typedef  unsigned char HDMI_BOOL;

typedef enum {
	MODE_DVI	= 0x0,
	MODE_HDMI  ,
	MODE_UNKNOW
} HDMI_DVI_MODE_T;

typedef enum {
	MODE_RAG_DEFAULT	= 0x0,
	MODE_RAG_LIMIT  ,
	MODE_RAG_FULL  ,
	MODE_RAG_UNKNOW
} HDMI_RGB_YUV_RANGE_MODE_T;

typedef enum {
	COLOR_RGB 	= 0x00,
	COLOR_YUV422,
	COLOR_YUV444,
	COLOR_YUV411,
	COLOR_UNKNOW
} HDMI_COLOR_SPACE_T;


typedef enum _HDMI_OSD_MODE{
	HDMI_OSD_MODE_AUTO= 0,
	HDMI_OSD_MODE_HDMI,
	HDMI_OSD_MODE_DVI,
	HDMI_OSD_MODE_NO_SETTING
} HDMI_OSD_MODE;

typedef enum {
	HDMI_PHY_PORT0 = 0,
	HDMI_PHY_PORT1,
	HDMI_PHY_PORT2,
	HDMI_PHY_PORT_NOTUSED
} HDMI_PHY_PORT_T;


typedef enum {
	HDMI_MUX_PORT0 = 0,
	HDMI_MUX_PORT1,
	HDMI_MUX_PORT2,
	HDMI_MUX_PORT3,
	HDMI_MUX_PORT4,
	HDMI_MUX_PORT5,
	HDMI_MUX_PORT6,
	HDMI_MUX_PORT_NOTUSED
} HDMI_MUX_PORT_T;


typedef enum {
	HDMI_EDID_ONCHIP = 0,
	HDMI_EDID_EEPROM,
	HDMI_EDID_I2CMUX,
	HDMI_EDID_DISABLE
} HDMI_EDID_TYPE_T;

typedef enum {
	HDMI_HPD_ONCHIP = 0,
	HDMI_HPD_I2CMUX,
	HDMI_HPD_DISABLE,
} HDMI_HPD_TYPE_T;

typedef enum {
	DDC0 = 0,
	DDC1,
	DDC2,
	DDC3,
	DDC_NOTUSED
} DDC_NUMBER_T;

typedef enum {
	HDMI_AUDIO = 0,

} HDMI_AUDIO_PATH_T;

typedef enum {
	HDMI_MUX_PS331 = 0,
	HDMI_SWITCH_SiI9199 = 1,
	HDMI_MUX_NOUSED,
} HDMI_MUX_IC_T;

typedef struct {
	char (*HPD) (int index, char high);
 	void (*PhyPortEnable) (HDMI_MUX_PORT_T port, char enable);
	char (*EDIDEnable) (int mux_ddc_port, char enable);
	char (*EDIDLoad) (int mux_ddc_port, unsigned char* edid, int len);
}HDMI_MUX_FUNC_T;

typedef struct {

	// for  HDMI port selection
	// for channel selection
	char 			  port_enable;
	HDMI_PHY_PORT_T  phy_port;
	HDMI_MUX_IC_T      mux_ic;
	HDMI_MUX_PORT_T mux_port;
	int 				  mux_ddc_port;
	DDC_NUMBER_T      ddc_selected;
	HDMI_EDID_TYPE_T edid_type;
	char				  edid_initialized;
	char 			  ddcci_enable;
	HDMI_HPD_TYPE_T  HotPlugType;
	int		  HotPlugPin;
	unsigned char          HotPlugPinHighValue;


	unsigned char 	DVIaudioPath;
	unsigned char		HDMIaudioPath;
	unsigned short       DVIaudio_switch_time_10ms;

	// EDID table
	unsigned char     EDID[256];   // EDID may stored in Flash and DDR
	/*DTS EDID Info*/
	int use_dts_edid;
 

	// channel enable for AP configuration
//	unsigned short    VFreqUpperBound;
} HDMI_CHANNEL_T;

// cloud add for magellan CBUS channel setting

typedef struct {
	unsigned char BKsv[5];
	unsigned char Key[320];
} HDCP_KEY_T;

#if (defined ENABLE_HDCPKEY_ENDIAN_SWAP)
typedef struct {
	unsigned char BKsv[5];
	unsigned char Key[328];
} HDCP_KEY_T1;
#endif

typedef enum {
	HDMI_COLORIMETRY_NOSPECIFIED = 0,
	HDMI_COLORIMETRY_601,
	HDMI_COLORIMETRY_709,
	HDMI_COLORIMETRY_XYYCC601,
	HDMI_COLORIMETRY_XYYCC709,
	HDMI_COLORIMETRY_SYCC601,
	HDMI_COLORIMETRY_ADOBE_YCC601,
	HDMI_COLORIMETRY_ADOBE_RGB,
} HDMI_COLORIMETRY_T;

HDMI_COLORIMETRY_T drvif_Hdmi_GetColorimetry(void);

#endif //PHOENIX_HDMI_FUN_H
