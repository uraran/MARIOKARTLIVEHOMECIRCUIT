#ifndef _HDMITX_H_
#define _HDMITX_H_

#include <drm/drm_edid.h>
#include <drm/drm_crtc.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#define __RTK_HDMI_GENERIC_DEBUG__  0

#if __RTK_HDMI_GENERIC_DEBUG__
#define HDMI_DEBUG(format, ...) printk("[HDMITx_DBG] " format "\n", ## __VA_ARGS__)
#else
#define HDMI_DEBUG(format, ...) 
#endif

#define HDMI_ERROR(format, ...) printk(KERN_ERR "[HDMITx_ERR] " format "\n", ## __VA_ARGS__)
#define HDMI_INFO(format, ...) printk(KERN_WARNING "[HDMITx] " format "\n", ## __VA_ARGS__)

#define wr_reg(x,y)                     writel(y,(volatile unsigned int*)(x))
#define rd_reg(x)                       readl((volatile unsigned int*)(x))

#define field_get(val, start, end) 		(((val) & field_mask(start, end)) >> (end))
#define field_mask(start, end)    		(((1 << (start - end + 1)) - 1) << (end))
#define setbits(base,offset, Mask) 		wr_reg( (base+offset), ( rd_reg(base+offset) | Mask))
#define readbits(value, Mask) 			((value >> (Mask)) & 0x1)
#define clearbits(base,offset, Mask)	wr_reg((base+offset),(rd_reg(base+offset) & ~(Mask)))

#define MAX_ELD_BYTES	128

enum {
	Vid_1920x1080p_60Hz =16,
    Vid_1920x1080p_50Hz =31,
};

enum HDMI_MODE {
	HDMI_MODE_UNDEF= 0,
	HDMI_MODE_HDMI= 1,
	HDMI_MODE_DVI= 2,
	HDMI_MODE_MAX
};

enum TMDS_MODE {
	TMDS_MODE_UNKNOW = 0,
	TMDS_HDMI_DISABLED = 1,
	TMDS_HDMI_ENABLED = 2,
	TMDS_MHL_ENABLED = 3,
	TMDS_MODE_MAX
};

enum HDMI_EXTENDED_MODE {
	EXTENDED_MODE_RESERVED = 0,
	EXTENDED_MODE_3840_2160_30HZ = 1,
	EXTENDED_MODE_3840_2160_25HZ = 2,
	EXTENDED_MODE_3840_2160_24HZ = 3,
	EXTENDED_MODE_4096_2160_24HZ = 4,
	EXTENDED_MODE_MAX
};

#define RTK_EDID_DIGITAL_DEPTH_6       (1 << 0)
#define RTK_EDID_DIGITAL_DEPTH_8       (1 << 1)
#define RTK_EDID_DIGITAL_DEPTH_10      (1 << 2)
#define RTK_EDID_DIGITAL_DEPTH_12      (1 << 3)
#define RTK_EDID_DIGITAL_DEPTH_14      (1 << 4)
#define RTK_EDID_DIGITAL_DEPTH_16      (1 << 5)

struct raw_edid {unsigned char edid_data[256];};

struct Audio_desc
{
	unsigned char	coding_type;
	unsigned char	channel_count;
	unsigned char	sample_freq_all;
	unsigned char	sample_size_all;
	unsigned char	max_bit_rate_divided_by_8KHz;
};

struct Audio_Data
{
	char ADB_length;	// Audio Data Block
	struct Audio_desc ADB[10];
	char SADB_length;	// Speaker Allocation Data Block
	unsigned char SADB[3];	
};

struct sink_capabilities_t {
	
	unsigned int hdmi_mode;
	//basic
	unsigned int est_modes;
	
	//audio
	//unsigned int max_channel_cap;
	//unsigned int sampling_rate_cap;	
	unsigned char eld[MAX_ELD_BYTES];
	struct Audio_Data audio_data;
	
	//Vendor-Specific Data Block(VSDB)
	unsigned char cec_phy_addr[2];
	bool support_AI;  // needs info from ACP or ISRC packets
	bool DC_Y444;	// 4:4:4 in deep color modes
	unsigned char color_space;  
	bool dvi_dual;	//DVI Dual Link Operation
	int max_tmds_clock;	/* in MHz */
	bool latency_present[2];
	unsigned char video_latency[2];	/* [0]: progressive, [1]: interlaced */
	unsigned char audio_latency[2];
	
	bool _3D_present;
	__u16 structure_all;
	unsigned char _3D_vic[16];
		
	//video
	struct drm_display_info display_info;
	__u64 vic;
	unsigned char extended_vic;

};


typedef struct{
	struct sink_capabilities_t sink_cap;
	bool sink_cap_available;
	unsigned char *edid_ptr;	
}asoc_hdmi_t;


struct hdmitx_switch_data{
	int	state;
	int	irq;
	int pin;
	struct work_struct work;	
};

struct ext_edid{
	int	extension;	
	int current_blk;
	unsigned char data[2*EDID_LENGTH];
};

#define HDMI_IOCTL_MAGIC 0xf1
#define HDMI_CHECK_LINK_STATUS			_IOR (HDMI_IOCTL_MAGIC,2,  int)
#define HDMI_CHECK_Rx_Sense				_IOR (HDMI_IOCTL_MAGIC,11, int)
#define HDMI_GET_EXT_BLK_COUNT			_IOR (HDMI_IOCTL_MAGIC,12, int)
#define HDMI_GET_EXTENDED_EDID			_IOWR(HDMI_IOCTL_MAGIC,13, struct ext_edid)
#define HDMI_QUERY_DISPLAY_STANDARD		_IOR (HDMI_IOCTL_MAGIC,14, int)

/* HDMI ioctl */
enum {
    HDMI_GET_SINK_CAPABILITY,
    HDMI_GET_RAW_EDID,
    HDMI_GET_LINK_STATUS,   
    HDMI_GET_VIDEO_CONFIG,       
    HDMI_SEND_AVMUTE,
    HDMI_CONFIG_TV_SYSTEM,
    HDMI_CONFIG_AVI_INFO,
    HDMI_SET_FREQUNCY,
    HDMI_SEND_AUDIO_MUTE,
    HDMI_SEND_AUDIO_VSDB_DATA,
    HDMI_SEND_AUDIO_EDID2,
	HDMI_CHECK_TMDS_SRC,	
};

enum HDMI_ERROR_CODE {
	HDMI_ERROR_RESERVED = 0,
	HDMI_ERROR_NO_MEMORY = 1,
	HDMI_ERROR_I2C_ERROR = 2,
	HDMI_ERROR_HPD = 3,
	HDMI_ERROR_INVALID_EDID = 4,
	//HDMI_ERROR_CBUS_DDC_ERROR = 5,
	HDMI_ERROR_MAX
};

//for auto detecting rx sense.
//#define HDMI_5V_HPD   	0x1
//#define HDMI_Rx_Sense   0x2

enum HDMI_RX_SENSE_STATUS {
	HDMI_RX_SENSE_OFF = 0,
	HDMI_RX_SENSE_ON = 1,
	HDMI_RX_SENSE_MAX
};

#define USE_ION_AUDIO_HEAP

#endif /* _HDMITX_H_ */
