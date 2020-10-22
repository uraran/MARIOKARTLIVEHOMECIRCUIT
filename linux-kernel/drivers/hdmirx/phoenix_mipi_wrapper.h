#ifndef PHOENIX_MIPI_WRAPPER_H
#define PHOENIX_MIPI_WRAPPER_H

#include <linux/irqreturn.h>

#define CAMERA_4_LANE 0

// HDMI MIPI
// HDMI Register Address base 0x18004100
//reg
#define MIPI_WRAPPER_INDEX0_reg 0x001C
#define MIPI_WRAPPER_TYPE_reg  0x002C
#define MIPI_WRAPPER_SA2_reg  0x000C
#define MIPI_WRAPPER_SA2_UV_reg  0x00E8
#define MIPI_WRAPPER_MIPI_SIZE1_reg  0x0014
#define MIPI_WRAPPER_MIPI_SIZE2_reg  0x0018
#define MIPI_WRAPPER_MIPI_reg  0x0000
#define MIPI_WRAPPER_MIPI_INT_ST_reg  0x0028
#define MIPI_WRAPPER_MIPI_SA0_reg  0x0004
#define MIPI_WRAPPER_MIPI_SA0_UV_reg  0x00E0
#define MIPI_WRAPPER_MIPI_RESET_reg  0x00EC
#define MIPI_WRAPPER_MIPI_SA1_reg  0x0008
#define MIPI_WRAPPER_MIPI_SA1_UV_reg  0x00E4
#define MIPI_WRAPPER_MIPI_SIZE0_reg  0x0010
#define MIPI_WRAPPER_SCALER_HSI_reg  0x0060
#define MIPI_WRAPPER_SCALER_HSD_reg  0x0064
#define MIPI_WRAPPER_SCALER_HSYNC0_reg  0x0070
#define MIPI_WRAPPER_SCALER_HSYNC1_reg  0x0074
#define MIPI_WRAPPER_SCALER_HSYNC2_reg  0x0078
#define MIPI_WRAPPER_SCALER_HSYNC3_reg  0x007c
#define MIPI_WRAPPER_SCALER_HSYNC4_reg  0x0080
#define MIPI_WRAPPER_SCALER_HSYNC5_reg  0x0084
#define MIPI_WRAPPER_SCALER_HSYNC6_reg  0x0088
#define MIPI_WRAPPER_SCALER_HSYNC7_reg  0x008C
#define MIPI_WRAPPER_SCALER_HSCC0_reg  0x0090
#define MIPI_WRAPPER_SCALER_HSCC1_reg  0x0094
#define MIPI_WRAPPER_SCALER_HSCC2_reg  0x0098
#define MIPI_WRAPPER_SCALER_HSCC3_reg  0x009c
#define MIPI_WRAPPER_SCALER_HSCC4_reg  0x00A0
#define MIPI_WRAPPER_SCALER_HSCC5_reg  0x00A4
#define MIPI_WRAPPER_SCALER_HSCC6_reg  0x00A8
#define MIPI_WRAPPER_SCALER_HSCC7_reg  0x00AC
#define MIPI_WRAPPER_SCALER_VSI_reg  0x0068
#define MIPI_WRAPPER_SCALER_VSD_reg  0x006C
#define MIPI_WRAPPER_SCALER_VSYC0_reg  0x00B0
#define MIPI_WRAPPER_SCALER_VSYC1_reg  0x00B4
#define MIPI_WRAPPER_SCALER_VSYC2_reg  0x00B8
#define MIPI_WRAPPER_SCALER_VSYC3_reg  0x00BC
#define MIPI_WRAPPER_SCALER_VSCC0_reg  0x00C0
#define MIPI_WRAPPER_SCALER_VSCC1_reg  0x00C4
#define MIPI_WRAPPER_SCALER_VSCC2_reg  0x00C8
#define MIPI_WRAPPER_SCALER_VSCC3_reg  0x00CC
#define MIPI_WRAPPER_CONSTANT_ALPHA_reg  0x00D0
#define MIPI_WRAPPER_CS_TRANS0_reg  0x0030
#define MIPI_WRAPPER_CS_TRANS1_reg  0x0034
#define MIPI_WRAPPER_CS_TRANS2_reg  0x0038
#define MIPI_WRAPPER_CS_TRANS3_reg  0x003C
#define MIPI_WRAPPER_CS_TRANS4_reg  0x0040
#define MIPI_WRAPPER_CS_TRANS5_reg  0x0044
#define MIPI_WRAPPER_CS_TRANS6_reg  0x0048
#define MIPI_WRAPPER_CS_TRANS7_reg  0x004C
#define MIPI_WRAPPER_CS_TRANS8_reg  0x0050
#define MIPI_WRAPPER_CS_TRANS9_reg  0x0054
#define MIPI_WRAPPER_CS_TRANS10_reg  0x0058
#define MIPI_WRAPPER_CS_TRANS11_reg  0x005C

// HDMI MIPI PHY
// HDMI Register Address base 0x18004000

#define MIPI_WRAPPER_APHY_REG15_reg  0x003C
#define MIPI_WRAPPER_APHY_REG19_reg 0x004C
#define MIPI_WRAPPER_DPHY_PWDB_reg 0x0074
#define MIPI_WRAPPER_DPHY_REG0_reg  0x0080
#define MIPI_WRAPPER_DPHY_REG1_reg 0x008C
#define MIPI_WRAPPER_DPHY_REG2_reg  0x0090
#define MIPI_WRAPPER_DPHY_REG4_reg 0x0098
#define MIPI_WRAPPER_DPHY_REG6_reg  0x00A0
#define MIPI_WRAPPER_DPHY_REG7_reg  0x00A4
#define MIPI_WRAPPER_DPHY_REG8_reg  0x00A8
#define MIPI_WRAPPER_DPHY_REG9_reg  0x00AC
#define MIPI_WRAPPER_DPHY_REG11_reg  0x00B4

//mask
#define MIPI_TYPE_MASK 0x01
#define SRC_WIDTH_PIC_MASK  0x1FFF0000
#define SRC_WIDTH_VIDEO_MASK  0x1FFF

//set bit
#define MIPI_WRAPPER_TYPE_SET(x) (x)
//mipi_dphy_pwdb
#define L0(x)             (x & 0x1)
#define L1(x)             ((x & 0x1) << 1)
#define L2(x)             ((x & 0x1) << 2)
#define L3(x)             ((x & 0x1) << 3)
//mipi_dphy_reg0
#define Lane0(x)            (x & 0x1)
#define Lane1(x)            ((x & 0x1) << 1)
#define Lane2(x)            ((x & 0x1) << 2)
#define Lane3(x)            ((x & 0x1) << 3)
#define Div_sel(x)          ((x & 0x3) << 6)
enum {
	dvi_bit_8   =	   0x0,
	div_bit_16  =    0x1,	
	div_bit_32  =    0x2,	
};
//mipi_dphy_reg1
#define dec_format(x)       (x & 0x7) 
enum {
	RAW8_dec_format     =	   0x0,
	RAW10_dec_format    =    0x1,	
	YUV422_dec_format   =    0x2,	
	JPEG_dec_format     =    0x3,
	RAW12_dec_format    =    0x4,
};

#define yuv_src_sel(x)      ((x & 0x3) << 4) 
enum {
	SEL_UYVY   =	  0x0,
	SEL_VYUY   =    0x1,	
	SEL_YVYU   =    0x2,	
	SEL_YUYV   =    0x3,
};
//mipi_dphy_reg2
#define D_type(x)           (x & 0xff)  
enum {
	DTYPE_RAW8    =    0x2A,
	DTYPE_RAW10   =    0x2B,
	DTYPE_YUY8    =	   0x1E,
	DTYPE_MJPEG   =    0x31,	
}; 
//mipi_dphy_reg4
#define Sw_rst(x)           (x & 0x1)  
enum {
	disable    =	   0x0,
	enable     =     0x1,	
}; 
#define SL_RG(x)            ((x & 0x1) << 3) 
enum {
	Onelane      =	   0x0,
	fourlane     =     0x1,	
}; 
//mipi_dphy_reg6
#define DVld_Lane0(x)           (x & 0xf)  
#define DVld_Lane1(x)           ((x & 0xf) << 4) 

//mipi_dphy_reg7
#define DVld_Lane2(x)           (x & 0xf)  
#define DVld_Lane3(x)           ((x & 0xf) << 4)  
//mipi_dphy_reg8
#define Lane0_sel(x)          (x & 0x3) 
#define Lane1_sel(x)          ((x & 0x3) << 2)
#define Lane2_sel(x)          ((x & 0x3) << 4)
#define Lane3_sel(x)          ((x & 0x3) << 6)
enum {
	MIPI_DATA0   =	  0x0,
	MIPI_DATA1   =    0x1,	
	MIPI_DATA2   =    0x2,	
	MIPI_DATA3   =    0x3,
};
//mipi_dphy_reg9
#define Lane0_clk_edge(x)          (x & 0x1) 
#define Lane1_clk_edge(x)          ((x & 0x1) << 1)
#define Lane2_clk_edge(x)          ((x & 0x1) << 2)
#define Lane3_clk_edge(x)          ((x & 0x1) << 3)
enum {
	posedge   =	  0x0,
	negedge   =	  0x1,	
};
//mipi_dphy_reg11
#define Ecc_err_flg(x)          (x & 0x1) 
#define Ecc_crt_flg(x)          ((x & 0x1) << 1)
#define Crc16_err_flg(x)        ((x & 0x1) << 2)
#define Clk_ulps_flg(x)         ((x & 0x1) << 3)
#define Esc_lane0_flg(x)        ((x & 0x1) << 4) 
#define Esc_lane1_flg(x)        ((x & 0x1) << 5)
#define Esc_lane2_flg(x)        ((x & 0x1) << 6)
#define Esc_lane3_flg(x)        ((x & 0x1) << 7)
enum {
	clear   =	  0x1,	
};
//mipi_aphy_reg15
#define SEL_DIV_RG(x)          (x & 0x3)
enum {
	DEMUX8   =	   0x0,
	DEMUX16  =     0x1,	
	DEMUX32  =     0x2,	
};

typedef struct {
    unsigned int     dst_fmt;
    unsigned int     src_fmt;
    unsigned int     src_sel;
    unsigned int     seq_en;
    unsigned int     vs;
    unsigned int     vs_near;
    unsigned int     vs_yodd;
    unsigned int     vs_codd;
    unsigned int     hs;
    unsigned int     hs_yodd;
    unsigned int     hs_codd;
    unsigned int     reserved_0;
    unsigned int     yuv_to_rgb;
    unsigned int     chroma_ds_mode;
    unsigned int     chroma_ds_en;
    unsigned int     chroma_us_mode;
    unsigned int     chroma_us_en;
    unsigned int     hdmirx_interlace_en;
    unsigned int     hdmirx_interlace_polarity;
    unsigned int     reserved_1;
    unsigned int     int_en4;
    unsigned int     int_en3;
    unsigned int     int_en2;
    unsigned int     int_en1;
    unsigned int     int_en0;
    unsigned int     en;
}MIPI_REG;

#define HDMIRX_NOHW_MAX_BACKBUFFERS (4)

#define roundup16(x)    roundup(x, 16)

int hdmi_v4l2_buf_copy(void *vbuf, unsigned int pixelindex);
void set_alpha(unsigned int alpha);
void set_YUV2RGB_coeff(void);
void set_video_dest_size(unsigned int dst_width, unsigned int pitch);
void set_video_src_size(unsigned int src_width);
void phoenix_mipi_scale_down(unsigned int src_width,unsigned int src_height,unsigned int dst_width,unsigned int dst_heigh);
void set_init_mipi_value(MIPI_REG *mipi_reg);
void set_mipi_env(void);
void set_mipi_type(unsigned int type);
void set_video_dest_addr(int addr1y, int addr1uv, int addr2y, int addr2uv);
irqreturn_t phoenix_mipi_isr(int irq, void* dev_id);
//void tmp_mipi_isr(void);
void set_video_DDR_start_addr(struct v4l2_hdmi_dev *dev);
void stop_mipi_process(void);
void setup_mipi(void);

#endif
