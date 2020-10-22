#ifndef PHOENIX_HDMI_INTERNAL_H
#define PHOENIX_HDMI_INTERNAL_H

//#include <stdio.h>
#include "phoenix_hdmiFun.h"
#include "phoenix_hdmiPlatform.h"
#include "phoenix_hdmiHWReg.h"
#include "hdmirx_rpc.h"
#include <linux/types.h>
#include "sd_test.h"
//#include "Add_in_common/hdmiSetForScaler.h"

#ifndef TRUE
#define TRUE true
#define FALSE false
#endif

#ifndef NULL
#define  NULL 0
#endif

//#define HDMI_DEBUG
#ifdef HDMI_DEBUG
#define HDMI_PRINTF(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#else
#define HDMI_PRINTF(fmt, ...)	do {} while(0)
#endif
#define HDMI_LOG(...)   do { } while(0)
#define HDMI_NOT_SUPPORT_SCALER
#define HDMI_LOCK()       do { } while(0)
#define HDMI_UNLOCK()         do { } while(0)
#ifndef ABS
#define ABS(x,y)                ((x > y) ? (x-y) : (y-x))
#endif

#define HDCP_10_DVI     0x91
#define HDCP_11_HDMI    0x93
#define HDMI_HPD_INSRC_TIME    50

#define _AUDIO_128_TIMES        1
#define _AUDIO_256_TIMES        2

#define _AUDIO_MCK_32000        _AUDIO_256_TIMES
#define _AUDIO_MCK_44100        _AUDIO_256_TIMES
#define _AUDIO_MCK_48000        _AUDIO_256_TIMES
#define _AUDIO_MCK_88200        _AUDIO_256_TIMES
#define _AUDIO_MCK_96000        _AUDIO_256_TIMES
#define _AUDIO_MCK_176400       _AUDIO_128_TIMES
#define _AUDIO_MCK_192000       _AUDIO_128_TIMES

#define HDMI_AUDIO_PCM      0
#define HDMI_AUDIO_NPCM 1

#define HDMI_RX_REG_BLOCK_NUM 19

#define HDMI_MAX_FRAME_SIZE 0x400000 //0x80000//0x2200000 //0x880000// ~ 4096*2160*4
#define HDMI_UV_BUF_OFFSET (HDMI_MAX_FRAME_SIZE >> 1) // ~ 1920*1080*2

#define _SYNC_HN_VN _BIT0
#define _SYNC_HP_VN     _BIT1
#define _SYNC_HN_VP     _BIT2
#define _SYNC_HP_VP     _BIT3

#ifndef UINT8
typedef int INT32;
typedef unsigned int UINT32;
typedef unsigned short  UINT16;
typedef char    UINT8;
#endif

typedef enum {
    MAIN_FSM_HDMI_SETUP_VEDIO_PLL = 0,
    MAIN_FSM_HDMI_MEASURE,
    MAIN_FSM_HDMI_MEASURE_ACTIVE_SPACE,
    MAIN_FSM_HDMI_WAIT_READY,
    MAIN_FSM_HDMI_DISPLAY_ON,
} HDMI_MAIN_FSM_T;

typedef enum {
    AUDIO_FSM_AUDIO_START = 0,
    AUDIO_FSM_FREQ_DETECT,
    AUDIO_FSM_AUDIO_WAIT_TO_START,
    AUDIO_FSM_AUDIO_START_OUT,
    AUDIO_FSM_AUDIO_WAIT_PLL_READY,
    AUDIO_FSM_AUDIO_CHECK
} HDMI_AUDIO_FSM_T;


typedef enum {

    AUDIO_FORMAT_LPCM = 0,
    AUDIO_FORMAT_NONLPCM

} HDMI_AUDIO_FORMAT_T;

typedef struct {
    unsigned long ACR_freq;         // Audio Frequency from ACR
    unsigned long AudioInfo_freq;       // Audio Frequency from Audio Info Frame
    unsigned long SPDIF_freq;           // Audio Frequency from SPDIF Channel Status Info
} HDMI_AUDIO_FREQ_T;

typedef struct {
    int        freq;
    unsigned char coeff;
    unsigned char rate;
} HDMI_AUDIO_PLL_COEFF_T;


typedef struct
{
    atomic_t read_index;
    atomic_t write_index;
    atomic_t fill_index;
    int use_v4l2_buffer;
    int pre_frame_done;
    int ISR_FLIP;
}HDMI_SW_BUF_CTL;

typedef struct  {
    unsigned int width;
    unsigned int height;
    unsigned int fps;
	unsigned int interlace;
}HDMI_VIC_TABLE_T;

extern HDMI_VIC_TABLE_T hdmi_vic_table[];

#define HDMI_SW_BUF_NUM 20
#define HDMI_HW_BUF_NUM 20

typedef struct {

    unsigned char N;
    unsigned char M;
    unsigned char O;
    unsigned char S;
    unsigned char S1;
    unsigned int D_HighByte;
    unsigned int D_LowByte;
    const char *sample_rate;

} HDMI_AUDIO_PLL_PARAM_T;


typedef enum {

    HDMIRX_DETECT_FAIL = 0,
    HDMIRX_DETECT_SUCCESS,
    HDMIRX_DETECT_AVMUTE,
    HDMIRX_CHECK_MODE,
    HDMIRX_RELEASED,

} HDMIRX_APSTATUS_T;


typedef struct {
    HDMIRX_APSTATUS_T apstatus;
    unsigned int idle_count;
} HDMIRX_PHY_IDLE_PATCH_T;



typedef struct {
     short sum_c;
     unsigned char overflow:1;
     unsigned char underflow:1;
     unsigned char audio_disable:1;
     unsigned char pll_unstable:1;
     unsigned int n;
     unsigned int cts;

} AUDIO_DEBUG_INFO_T;

typedef struct {
    AUDIO_DEBUG_INFO_T *buffer;
    int      ring_start;
    int  ring_end;
    HDMI_BOOL enable;
} HDMI_AUDIO_PLL_BUFFER_T;




typedef struct {
    unsigned short IVTotal;
    unsigned short IHTotal;
    unsigned short IVFreq;
    short   IVSTA_ADJ;
    short   IHSTA_ADJ;
    short   IVHEIGHT_ADJ;
    short   IHWIDTH_ADJ;
} HDMI_TIMING_COMPENSATION_T;

//
typedef enum {

    HDMI_MS_MODE_ONESHOT =0,
    HDMI_MS_MODE_CONTINUOUS_INIT,
    HDMI_MS_MODE_CONTINUOUS_CHECK,
    HDMI_MS_MODE_ONESHOT_INIT,
    HDMI_MS_MODE_ONESHOT_GET_RESULT,
    HDMI_MS_MODE_ONESHOT_POLARITY_FIXED,
} HDMI_MS_MODE_T;

typedef enum {

    HDMI_3DEYE_LEFT,
    HDMI_3DEYE_RIGHT,
} HDMI_3DEYE_T;

#if 0
typedef enum {

    HDMI_3DFMT_2D = 0,
    HDMI_3DFMT_3D_FRAME_PACKING,
    HDMI_3DFMT_3D_TOP_BOTTOM,
    HDMI_3DFMT_3D_SIDE_BY_SIDE,
    HDMI_3DFMT_3D_LINE_ALTERNATIVE,

} HDMI_3DFMT_T;
#endif
typedef enum {

    HDMI_COLOR_DEPTH_8B =0,
    HDMI_COLOR_DEPTH_10B,
    HDMI_COLOR_DEPTH_12B,
    HDMI_COLOR_DEPTH_16B,

} HDMI_COLOR_DEPTH_T;

typedef struct {
    unsigned char SM;
    unsigned char SN;
    unsigned char RatioM;
    unsigned char RatioN;
} VIDEO_DPLL_RATIO_T;

extern HDMI_CONST VIDEO_DPLL_RATIO_T dpll_ratio[];

typedef enum {
    HDMI_AUDIO_N_CTS_TREND_BOUND = 0,
    HDMI_AUDIO_TREND_BOUND,
    HDMI_AUDIO_N_CTS,
} HDMI_AUDIO_TRACK_MODE;

typedef enum {
    OUT_RAW8    = 0x0,
    OUT_RAW10     = 0x1,
    OUT_YUV422    = 0x2,
    OUT_YUV420    = 0x3,
    OUT_JPEG8      = 0x4,
    OUT_ARGB  = 0x10,
    OUT_ARBG  = 0x11,
    OUT_AGRB  = 0x12,
    OUT_AGBR  = 0x13,
    OUT_ABRG  = 0x14,
    OUT_ABGR  = 0x15,
    OUT_RGBA  = 0x16,
    OUT_RBGA  = 0x17,
    OUT_GRBA  = 0x18,
    OUT_GBRA  = 0x19,
    OUT_BRGA  = 0x1A,
    OUT_BGRA  = 0x1B,
    OUT_UNKNOW
} MIPI_OUT_COLOR_SPACE_T;

typedef struct {

    HDMI_COLOR_DEPTH_T depth;
    HDMI_COLOR_SPACE_T color;
    HDMI_COLORIMETRY_T  colorimetry;
    char progressive;          // 0 --> interlaced ; 1 --> progressive
    HDMI_3D_T hdmi_3dformat;

    unsigned int v_act_len;
    unsigned int h_act_len;
    unsigned int polarity;
    unsigned int mode_id;
    int pitch;

    //audio
    unsigned int spdif_freq;

    // output format
    MIPI_OUT_COLOR_SPACE_T output_color;
    unsigned int output_v;
    unsigned int output_h;
    // to vo info
    unsigned int phyAddr;

    // quincunx
    unsigned quincunx_en;
    unsigned quincunx_mode;

} HDMI_TIMING_T;

/*

    HDMI library information


*/

typedef struct {
    int lane_num;
    unsigned int h_input_len;
    unsigned int v_input_len;
    unsigned int h_output_len;
    unsigned int v_output_len;
    MIPI_OUT_COLOR_SPACE_T output_color;
    int pitch;
    int auxiliary_inband_lines;
} MIPI_CSI_INFO_T;

typedef struct {
    unsigned int Info;          // [0:1] Color Space
                                            // [2]   IsInterlace
                                            // [3]    is hdmi/dvi
                                            // [4:7] color depth
                                            // [8]    current channel

    unsigned char FSM;      // [0:3] Video FSM
                                            // [4:7] Audio FSM

    unsigned short b;
#if (defined ENABLE_HDCPKEY_ENDIAN_SWAP)
    HDCP_KEY_T1 * hdcpkey;
#else
    HDCP_KEY_T * hdcpkey;
#endif
	unsigned char hdcpkey_enable;

    HDMI_CHANNEL_T *channel[HDMI_MAX_CHANNEL];
       HDMI_TIMING_T  tx_timing;
    unsigned int timer;
    int      fast_boot_source;

    // for HDMIRX to VO
    struct task_struct         *kthread_hdmi2vo;
    REFCLOCK                *REF_CLK;
    RINGBUFFER_HEADER       *RING_HEADER;
    unsigned long           CTX;

	// GPIO number
	int gpio_rx_5v;
	int gpio_hpd_ctrl;
	unsigned int power_saving;//Turn off RX when cable unplugged, need to do init when plugged

    // mipi_camera_setting
    unsigned int mipi_camera_type;
    unsigned int mipi_input_type;

    struct v4l2_hdmi_dev *dev;
} HDMI_INFO_T;

typedef struct {
    char hdmi_rx_init;
//    char camera_init;
    char mipi_init;
    unsigned int src_sel; // 0:mipi ; 1:hdmi_rx
    HDMI_INFO_T *p_hdmi;
    MIPI_CSI_INFO_T *p_mipi_csi;
} MIPI_TOP_INFO;

extern MIPI_TOP_INFO mipi_top;
extern HDMI_INFO_T hdmi;
extern MIPI_CSI_INFO_T mipi_csi;

#define GET_HDMI_COLOR_SPACE()          ((HDMI_COLOR_SPACE_T) (hdmi.Info & (_BIT0 | _BIT1)))
#define GET_HDMI_ISINTERLACE()          ((hdmi.Info & (_BIT2)) >> 2)
#define GET_ISHDMI()                        ((HDMI_DVI_MODE_T)((hdmi.Info & _BIT3) >> 3))
#define GET_HDMI_CHANNEL()              ((hdmi.Info & (_BIT8 | _BIT9)) >> 8)
#define GET_HDMI_CD()                   ((hdmi.Info & (0xf0)) >> 4)
#define GET_HDMI_DEONLY_MODE()      ((hdmi.Info & _BIT10) >> 10)
#define GET_HDMI_AVMUTE_MODE()      ((hdmi.Info & _BIT11) >> 11)
#define GET_HDMI_AUDIO_LAYOUT()     ((hdmi.Info & _BIT12) >> 12)
#define GET_IS_FIRST_CHECK_MODE()       ((hdmi.Info & _BIT13) >> 13)
#define GET_IS_FIRST_DETECT_MODE()  ((hdmi.Info & _BIT14) >> 14)
#define GET_DVI_AUDIO_PATH_SETUP()  ((hdmi.Info & _BIT15) >> 15)
#define GET_HDMI_AUDIO_PATH_SETUP()     ((hdmi.Info & _BIT16) >> 16)
#define GET_HDMI_AUDIO_TYPE()           ((hdmi.Info & _BIT17) >> 17)
#define GET_HDMI_HBR_MODE()         ((hdmi.Info & _BIT18) >> 18)
#define GET_HDMI_BOOT_FIRST_RUN()           ((hdmi.Info & _BIT19) >> 19)
#define GET_HDMI_FASTER_BOOT_EN()           ((hdmi.Info & _BIT20) >> 20)
#define GET_HDMI_FASTER_SWITCH_EN()     ((hdmi.Info & _BIT21) >> 21)
#define GET_HDMI_RGB_Q_RANGE()      ((HDMI_RGB_YUV_RANGE_MODE_T)(hdmi.Info & (_BIT22|_BIT23) )>> 22)
#define GET_HDMI_OSD_MODE()                 ((hdmi.Info & (_BIT29 | _BIT30)) >> 29)
#define GET_HDMI_VIDEO_FSM()            (hdmi.FSM & 0x0F)
#define GET_HDMI_AUDIO_FSM()            ((hdmi.FSM & 0xF0) >> 4)

#define SET_HDMI_COLOR_SPACE(x)     (hdmi.Info = ((hdmi.Info & ~ (_BIT0 | _BIT1))  | (((unsigned int) x & 0x03))))
#define SET_HDMI_CD(x)                  (hdmi.Info = ((hdmi.Info & ~(0xf0))  | (((unsigned int) x & 0x0f) << 4)))
#define SET_HDMI_ISINTERLACE(x)     do { hdmi.Info = ((hdmi.Info & (~_BIT2))  | (((unsigned int) x & 0x01) << 2)); \
                                             hdmi.tx_timing.progressive = (x == 0);} while(0)
#define SET_ISHDMI(x)                   (hdmi.Info =    ((hdmi.Info & ~_BIT3)  | (((unsigned int) x & 0x01) << 3)))
#define SET_HDMI_CHANNEL(x)         (hdmi.Info =    ((hdmi.Info & ~(_BIT8 | _BIT9))  | (((unsigned int) x & 0x03) << 8)))
#define SET_HDMI_DEONLY_MODE(x)     (hdmi.Info = ((hdmi.Info & (~_BIT10))  | (((unsigned int) x & 0x01) << 10)))
#define SET_HDMI_AVMUTE_MODE(x)     (hdmi.Info = ((hdmi.Info & (~_BIT11))  | (((unsigned int) x & 0x01) << 11)))
#define SET_HDMI_AUDIO_LAYOUT(x)        (hdmi.Info = ((hdmi.Info & (~_BIT12))  | (((unsigned int) x & 0x01) << 12)))
#define SET_IS_FIRST_CHECK_MODE(x)      (hdmi.Info = ((hdmi.Info & (~_BIT13))  | (((unsigned int) x & 0x01) << 13)))
#define SET_IS_FIRST_DETECT_MODE(x) (hdmi.Info = ((hdmi.Info & (~_BIT14))  | (((unsigned int) x & 0x01) << 14)))
#define SET_DVI_AUDIO_PATH_SETUP(x) (hdmi.Info = ((hdmi.Info & (~_BIT15))  | (((unsigned int) x & 0x01) << 15)))
#define SET_HDMI_AUDIO_PATH_SETUP(x)    (hdmi.Info = ((hdmi.Info & (~_BIT16))  | (((unsigned int) x & 0x01) << 16)))
#define SET_HDMI_AUDIO_TYPE(x)          (hdmi.Info = ((hdmi.Info & (~_BIT17))  | (((unsigned int) x & 0x01) << 17)))
#define SET_HDMI_HBR_MODE(x)            (hdmi.Info = ((hdmi.Info & (~_BIT18))  | (((unsigned int) x & 0x01) << 18)))
#define SET_HDMI_BOOT_FIRST_RUN(x)  (hdmi.Info = ((hdmi.Info & (~_BIT19))  | (((unsigned int) x & 0x01) << 19)))
#define SET_HDMI_FASTER_BOOT_EN(x)          (hdmi.Info = ((hdmi.Info & (~_BIT20))  | (((unsigned int) x & 0x01) << 20)))
#define SET_HDMI_FASTER_SWITCH_EN(x)        (hdmi.Info = ((hdmi.Info & (~_BIT21))  | (((unsigned int) x & 0x01) << 21)))
#define SET_HDMI_RGB_Q_RANGE(x)     (hdmi.Info = ((hdmi.Info & (~(_BIT22|_BIT23)))  | (((unsigned int) x & 0x03) << 22)))
#define SET_HDMI_OSD_MODE(x)            (hdmi.Info = ((hdmi.Info & ~(_BIT29 | _BIT30))  | (((unsigned int) x & 0x03) << 29)))
#define SET_HDMI_VIDEO_FSM(x)           (hdmi.FSM = (hdmi.FSM & 0xF0) | ((unsigned char) x & 0x0F))
#define SET_HDMI_AUDIO_FSM(x)           (hdmi.FSM = (hdmi.FSM & 0x0F) | (((unsigned char) x & 0x0F) << 4))

/*

    HDMI relative HW address marco

*/

#define HDMI_AUDIO_SUPPORT_NON_PCM()        (HDMI_SUPPORT_NONPCM != 0)
#define HDMI_AUDIO_IS_LPCM()  ((hdmi_rx_reg_read32(HDMI_HDMI_SR_VADDR, HDMI_RX_MAC) & _BIT4) == 0)

typedef enum {
    HDMI_ERR_NO = 0,
    HDMI_EER_GENERIC,
    HDMI_ERR_HDMIWRAPPER_NOT_READY,
    HDMI_ERR_HDMIWRAPPER_MS_ERR,
    HDMI_ERR_3D_WRONG_OPMODE,
    HDMI_ERR_3D_NO_MEM,
    HDMI_ERR_4XXTO4XX_WRONG_PARAM,
    HDMI_EER_MEASURE_ACTIVESPACE_FAIL,
    HDMI_EER_VODMA_NOT_READY,
} HDMI_ERR_T;

typedef enum {
    IOCTRL_HDMI_PHY_START = 0,
    IOCTRL_HDMI_PHY_STOP,
    IOCTRL_HDMI_GET_STRUCT,
    IOCTRL_HDMI_SET_TIMER,
    IOCTRL_HDMI_GET_TIMER,
    IOCTRL_HDMI_AUDIO_PLL_SAMPLE_START,
    IOCTRL_HDMI_AUDIO_PLL_SAMPLE_STOP,
    IOCTRL_HDMI_AUDIO_PLL_SAMPLE_DUMP,
    IOCTRL_HDMI_SET_APSTATUS
}  HDMIRX_IOCTL_T;



typedef struct {
    unsigned short b;
    unsigned short b_pre;
    unsigned short b_count;
    unsigned short b_debouce_count;
    UINT8 b_mhl_debounce;
    HDMI_BOOL b_change;
    HDMI_BOOL avi_info_in;
    unsigned short avi_info_miss_cnt;
    unsigned short LG_patch_timer;
    unsigned short timer;
    short    hotplug_timer;
    unsigned short hotplug_delay_count;
    unsigned char DEF_ready;
    int measure_ready;
    int detect_done;
} HDMIRX_IOCTL_STRUCT_T;

extern HDMIRX_IOCTL_STRUCT_T hdmi_ioctl_struct;

typedef struct {
		UINT16 b_upper;
		UINT16 b_lower;
		UINT16 M_code;
		UINT16 N_code;
		UINT8 N_bypass;
		UINT8 MD_adj;
		UINT8 CK_LDOA;
		UINT8 CK_LDOD;
		UINT8 CK_CS;
		UINT8 CK_RS;
		UINT8 EQ_adj;
		UINT8 CDR_bias;
		UINT8 CDR_KP;
		UINT8 CDR_KP2;
		UINT8 CDR_KI;
		UINT8 CDR_KD;
		UINT8 EQ_gain;
		UINT8 EQ_bias;
		UINT8 CK_Icp;
		UINT8 PR;
	//	UINT8 reg_RESERVED_02;
		//UINT8 reg_RESERVED_05;
		//UINT8 reg_ADAPTIVE_EQUALIZER;
		//UINT8 reg_RESERVED_FA;
		char *band_name;
} HDMI_PHY_PARAM_T;

typedef struct {
    UINT16 b_upper;
    UINT16 b_lower;
        UINT16 M_code;
        UINT16 N_code;
        UINT8 N_bypass;
        UINT8 MD_adj;
        UINT8 CK_LDOA;
        UINT8 CK_LDOD;
        UINT8 CK_CS;
        UINT8 CK_RS;
        UINT8 EQ_adj;
        UINT8 CDR_bias;
        UINT8 CDR_KP1;
        UINT8 CDR_KP2;
        UINT8 CDR_KI;
        UINT8 EQ_gain;
        UINT8 EQ_bias;
        UINT8 CK_Icp;
    //UINT8 reg_RXMISC_01;
    //UINT8 reg_RXMISC_02;
    //UINT8 reg_RESERVED_00;
//  UINT8 reg_RESERVED_02;
    //UINT8 reg_RESERVED_05;
    //UINT8 reg_ADAPTIVE_EQUALIZER;
    //UINT8 reg_RESERVED_FA;
    char *band_name;

} MHL_PHY_PARAM_T;


typedef struct
{
   unsigned char edid[256];
} HDMIRX_DTS_EDID_TBL_T;

/****************************************/


extern HDMI_SW_BUF_CTL hdmi_sw_buf_ctl;
extern int hdmi_interlace_polarity_check ;

void drvif_Hdmi_Release(void) ;
HDMI_DVI_MODE_T IsHDMI(void);
void drvif_Hdmi_Init(void);
unsigned char drvif_Hdmi_AVI_VIC(void);
char HdmiMuxHPD(int index, char high);
char HdmiMuxEDIDLoad(int ddc_index, unsigned char* EDID, int len);
char HdmiMuxEDIDEnable(int ddc_index, char enable);
void HdmiMux_PhyPortEnable(HDMI_MUX_PORT_T port, char enable);
unsigned char get_HDMI_AFD(void);
int hdmi_onms_measure(HDMI_TIMING_T *timing) ;
UINT8 Hdmi_HdcpPortRead(UINT8 addr);
HDMI_COLORIMETRY_T Hdmi_GetColorimetry(void);
bool Hdmi_3DInfo_IsChange(void);
void drvif_Hdmi_LoadEDID(unsigned char channel_index, unsigned char *EDID, int len) ;
HDMI_BOOL Hdmi_MeasureTiming(HDMI_TIMING_T *timing, int b);
HDMI_BOOL Hdmi_GetInterlace(HDMI_MS_MODE_T mode);
unsigned char drvif_Hdmi_GetColorDepth(void);
HDMI_3D_T Hdmi_Get3DInfo(HDMI_MS_MODE_T mode);
HDMI_BOOL drvif_Hdmi_GetInterlace(void);
void HdmiMuxSel(HDMI_MUX_IC_T mux_ic);
//unsigned int Hdmi_GetVSyncCountInit(void);
//void HdmiISRInit(void);
void Hdmi_WaitVsync(int num);
void HdmiTable_Init(void);
void HdmiRx_Save_DTS_EDID_Table_Init(HDMIRX_DTS_EDID_TBL_T * dts_edid, int max_ddc_channel);
void drvif_Hdmi_InitSrc(unsigned char channel);
void rtd_hdmiPhy_ISR(void);
int rtd_hdmiRx_cmd(UINT8 cmd,  void * arg);

void HdmiGetStruct(HDMIRX_IOCTL_STRUCT_T* ptr);
void set_gpio6_output_low(void);
void set_gpio6_output_high(void);
void HdmiRx_change_edid_physical_addr(unsigned char byteMSB, unsigned char byteLSB);
void HdmiRx_enable_hdcp(unsigned char *bksv, unsigned char *device_key);
char drvif_EDIDLoad(DDC_NUMBER_T ddc_index, unsigned char* EDID, int len);
char drvif_EDIDEnable(DDC_NUMBER_T ddc_index, char enable);
void drvif_EDID_DDC12_AUTO_Enable(DDC_NUMBER_T ddc_index,char enable);
HDMI_BOOL drvif_Hdmi_regTable(HDMI_TABLE_TYPE index, void *ptr);
void HdmiSetAPStatus(HDMIRX_APSTATUS_T apstatus);
void hdmi_rx_size_measurement(void);
char EDIDIsValid(unsigned char* EDID);
int HdmiMeasureVedioClock(void);
void SetupTMDSPhy(UINT32 b, char force);
//void HdmiISREnable(char nEnable);

HDMI_3D_T drvif_HDMI_GetReal3DFomat(void);
HDMI_ERR_T Hdmi_CheckConditionChange(void);
void restartHdmiRxWrapperDetection(void);
HDMI_DVI_MODE_T drvif_IsHDMI(void);
int drvif_Hdmi_CheckMode(void);
HDMI_BOOL drvif_Hdmi_DetectMode(void);
unsigned char Hdmi_AudioPLLSetting(int freq, HDMI_AUDIO_TRACK_MODE track_mode);
unsigned int rx_pitch_measurement(unsigned int output_h, MIPI_OUT_COLOR_SPACE_T output_color);
//void HdmiSetScalerColor(void) ;
HDMI_COLOR_SPACE_T Hdmi_GetColorSpace(void);

#define HDMI_WAIT_IVS()                 do {} while(0);//(Scaler_DispGetInputInfo(SLR_INPUT_DISPLAY) ? WaitFor_IVS2() : WaitFor_IVS1())

void    inline  Hdmi_AudioOutputDisable(void);

#endif //PHOENIX_HDMI_INTERNAL_H
