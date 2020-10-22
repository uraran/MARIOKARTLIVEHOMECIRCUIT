#ifndef PHOENIX_HDMI_WRAPPER_H
#define PHOENIX_HDMI_WRAPPER_H

// HDMI MIPI
// HDMI Register Address base 0x1800FF00

typedef enum{
    PHOENIX_RX_TYPE_SELECT_HDMI_IN = 0,
    PHOENIX_RX_TYPE_SELECT_MHL_IN = 1
}PHOENIX_RX_TYPE_SELECT;

//reg
#define HDMIRX_WRAPPER_DEN_BUF01_reg    0x0000
#define HDMIRX_WRAPPER_DEN_BUF23_reg    0x0004
#define HDMIRX_WRAPPER_ACTIVE_PIXELS_reg    0x000C
#define HDMIRX_WRAPPER_INTERRUPT_EN_reg 0x0018
#define HDMIRX_WRAPPER_INTERRUPT_STATUS_reg 0x001C
#define HDMIRX_WRAPPER_HOR_THRESHOLD_reg    0x0008
#define HDMIRX_WRAPPER_CONTROL_0_reg    0x0020
#define HDMIRX_WRAPPER_MONITOR_1_reg    0x0014

//mask
#define VER_ERR_EN_MASK 0x04
#define HOR_ERR_EN_MASK 0x02
#define POLARITY_DETECT_MASK 0x01
#define FIFI_STAGE_MASK 0x0800
#define FW_DMA_EN_MASK  0x0400
#define POLARITY_SET_EN_MASK    0x0200
#define POLARITY_SET_MASK   0x0100
#define ADDR_H_MASK 0x0F0
#define YUV_FMT_MASK    0x00E
#define CONTROL_0_HDMIRX_EN_MASK    0x001
#define DEN_BUF01_MASK   0x1FFF0000
#define LINE_BUF0_MASK  0x0FFF

//set bit
#define HDMIRX_WRAPPER_INTERRUPT_EN_VER_ERR_EN(x)   (x << 2)
#define HDMIRX_WRAPPER_INTERRUPT_EN_HOR_ERR_EN(x)   (x << 1)
#define HDMIRX_WRAPPER_INTERRUPT_EN_POLARITY_DETECT_MASK(x) (x)
#define HDMIRX_WRAPPER_CONTROL_0_FIFO_STAGE_SET(x)  (x << 11)
#define HDMIRX_WRAPPER_CONTROL_0_FW_DMA_EN_SET(x)   (x << 10)
#define HDMIRX_WRAPPER_CONTROL_0_POLARITY_SET_EN_SET(x) (x << 9)
#define HDMIRX_WRAPPER_CONTROL_0_POLARITY_SET(x)   (x << 8) 
#define HDMIRX_WRAPPER_CONTROL_0_ADDR_H(x)  (x << 4)
#define HDMIRX_WRAPPER_CONTROL_0_YUV_FMT_SET(x) (x << 1)
#define HDMIRX_WRAPPER_CONTROL_0_HDMIRX_EN(x)   (x)

#define HDMI_MIN_SUPPORT_H_PIXEL    0x40

unsigned int hdmirx_wrapper_get_active_line(void);
unsigned int hdmirx_wrapper_get_active_pixel(void);
void hdmi_related_wrapper_init(void);
void set_hdmirx_wrapper_control_0(int fifo_stage,int fw_dma_en,int polarity_set_en,int polarity_set,int yuv_fmt,int hdmirx_en);
void set_hdmirx_wrapper_active_pixels(unsigned v_pixel, unsigned h_pixel);
void set_hdmirx_wrapper_interrupt_en(int ver_err_en,int hor_err_en,int polarity_detect_en);
void set_hdmirx_wrapper_hor_threshold(unsigned int h_threshold);

#endif //PHOENIX_HDMI_WRAPPER_H
