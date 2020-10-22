#include "phoenix_hdmi_wrapper.h"
#include "phoenix_mipi_wrapper.h"
#include "phoenix_hdmiHWReg.h"
#include "phoenix_hdmiInternal.h"
#include "v4l2_hdmi_dev.h"
#include "sd_test.h"
#include <linux/printk.h>
#include <linux/ratelimit.h>

const unsigned int resolution_format[14][2]=
{
	{ 640, 480},
	{ 720, 480},
	{1280, 720},
	{1920,1080},
	{ 720, 240},
	{1440, 480},
	{ 720, 576},
	{ 720, 288},
	{1440, 576}, 
	{ 720, 400},
	{ 800, 600},
	{1024, 768},	
	{1280, 960},	  
	{1920, 540},
};	
const unsigned int format_number = sizeof(resolution_format)/sizeof(resolution_format[0]);


void set_hdmirx_wrapper_interrupt_en(int ver_err_en,int hor_err_en,int polarity_detect_en)
{
    unsigned int reg_val;
    reg_val = hdmi_rx_reg_read32(HDMIRX_WRAPPER_INTERRUPT_EN_reg, HDMI_RX_HDMI_WRAPPER);

    if(ver_err_en >= 0){
        reg_val = reg_val & (VER_ERR_EN_MASK ^ 0xFFFFFFFF);
        reg_val = reg_val | HDMIRX_WRAPPER_INTERRUPT_EN_VER_ERR_EN(ver_err_en);
    }
    if(hor_err_en >= 0){
        reg_val = reg_val & (HOR_ERR_EN_MASK ^ 0xFFFFFFFF);
        reg_val = reg_val | HDMIRX_WRAPPER_INTERRUPT_EN_HOR_ERR_EN(hor_err_en);
    }
    if(polarity_detect_en >=0){
        reg_val = reg_val & (POLARITY_DETECT_MASK ^ 0xFFFFFFFF);
        reg_val = reg_val | HDMIRX_WRAPPER_INTERRUPT_EN_POLARITY_DETECT_MASK(polarity_detect_en);
    }
    hdmi_rx_reg_write32(HDMIRX_WRAPPER_INTERRUPT_EN_reg, reg_val, HDMI_RX_HDMI_WRAPPER);
}

void set_hdmirx_wrapper_active_pixels(unsigned v_pixel, unsigned h_pixel)
{
    unsigned int reg_val;

    reg_val = ((v_pixel & 0x0FFF) << 16)  | // set_ver_width
                    (h_pixel & 0x1FFF); // set_hor_width
    hdmi_rx_reg_write32(HDMIRX_WRAPPER_ACTIVE_PIXELS_reg, reg_val, HDMI_RX_HDMI_WRAPPER);
}

void set_hdmirx_wrapper_hor_threshold(unsigned int h_threshold)
{
    hdmi_rx_reg_write32(HDMIRX_WRAPPER_HOR_THRESHOLD_reg, h_threshold, HDMI_RX_HDMI_WRAPPER);
}

void set_hdmirx_wrapper_control_0(int fifo_stage,int fw_dma_en,int polarity_set_en,int polarity_set,int yuv_fmt,int hdmirx_en)
{
    unsigned int reg_val;
    reg_val = hdmi_rx_reg_read32(HDMIRX_WRAPPER_CONTROL_0_reg, HDMI_RX_HDMI_WRAPPER);

    if(fifo_stage >= 0){
        reg_val = reg_val & (FIFI_STAGE_MASK ^ 0xFFFFFFFF);
        reg_val = reg_val | HDMIRX_WRAPPER_CONTROL_0_FIFO_STAGE_SET(fifo_stage);
    }
    if(fw_dma_en >= 0){
        reg_val = reg_val & (FW_DMA_EN_MASK ^ 0xFFFFFFFF);
        reg_val = reg_val | HDMIRX_WRAPPER_CONTROL_0_FW_DMA_EN_SET(fw_dma_en);
    }
    if(polarity_set_en >= 0){
        reg_val = reg_val & (POLARITY_SET_EN_MASK ^ 0xFFFFFFFF);
        reg_val = reg_val | HDMIRX_WRAPPER_CONTROL_0_POLARITY_SET_EN_SET(polarity_set_en);
    }
    if(polarity_set >= 0){
        reg_val = reg_val & (POLARITY_SET_MASK ^ 0xFFFFFFFF);
        reg_val = reg_val | HDMIRX_WRAPPER_CONTROL_0_POLARITY_SET(polarity_set);
    }
    reg_val = reg_val & (ADDR_H_MASK ^ 0xFFFFFFFF);
    reg_val = reg_val | HDMIRX_WRAPPER_CONTROL_0_ADDR_H(0x7);
    if(yuv_fmt >= 0){
        reg_val = reg_val & (YUV_FMT_MASK ^ 0xFFFFFFFF);
        reg_val = reg_val | HDMIRX_WRAPPER_CONTROL_0_YUV_FMT_SET(yuv_fmt);
    }
    if(hdmirx_en >= 0){
        reg_val = reg_val & (CONTROL_0_HDMIRX_EN_MASK ^ 0xFFFFFFFF);
        reg_val = reg_val | HDMIRX_WRAPPER_CONTROL_0_HDMIRX_EN(hdmirx_en);
    }
    hdmi_rx_reg_write32(HDMIRX_WRAPPER_CONTROL_0_reg, reg_val, HDMI_RX_HDMI_WRAPPER);
}

unsigned int hdmirx_wrapper_get_active_line(void)
{
    return (hdmi_rx_reg_read32(HDMIRX_WRAPPER_MONITOR_1_reg, HDMI_RX_HDMI_WRAPPER) & LINE_BUF0_MASK);
}

unsigned int hdmirx_wrapper_get_active_pixel(void)
{
	unsigned int reg01,buf0;
	reg01 = hdmi_rx_reg_read32(HDMIRX_WRAPPER_DEN_BUF01_reg, HDMI_RX_HDMI_WRAPPER);
	buf0 = reg01 & 0x1FFF;
    return buf0;
}

static inline unsigned int check_hdmirx_resolution_match(void)
{
	unsigned int active_pixel,active_line,height;
	unsigned int i,vic;
	HDMI_TIMING_T *timing = &hdmi.tx_timing;

	vic = 0;
	if(IsHDMI())
	{
		vic = drvif_Hdmi_AVI_VIC();
		if(vic>59)
			vic = 0;
	}
	
	active_pixel = 	hdmirx_wrapper_get_active_pixel();
	active_line = hdmirx_wrapper_get_active_line();

	if(hdmi_vic_table[vic].interlace)
		height = active_line*2;
	else
		height = active_line;

	if(active_pixel==hdmi_vic_table[vic].width && height==hdmi_vic_table[vic].height && vic)
	{
		timing->h_act_len = active_pixel;
		timing->v_act_len = active_line;
		HDMIRX_INFO("Check resolution match => Width(%u) Height(%u) VIC(%u)",active_pixel,height,vic);
		return 1;
	}
    else if(vic == 0)//Support standard timings and DVI mode
	{
        for(i=0;i<format_number;i++)
        {
        	if(active_pixel==resolution_format[i][0] && active_line==resolution_format[i][1])
        	{
				timing->h_act_len = active_pixel;
				timing->v_act_len = active_line;
				HDMIRX_INFO("Check resolution match => Width(%u) Height(%u) DIV",active_pixel,height);
	            return 1;
        	}
        }
	}

    HDMIRX_INFO("Check resolution not match => active_pixel(%u) active_line(%u) VIC(%u)",active_pixel,active_line,vic);
    return 0;
}

void hdmirx_wrapper_isr(void)
{
    unsigned int status = hdmi_rx_reg_read32(HDMIRX_WRAPPER_INTERRUPT_STATUS_reg, HDMI_RX_HDMI_WRAPPER);
    unsigned int active_pixel,active_line;
	HDMI_TIMING_T *timing = &hdmi.tx_timing;

	if(likely(status == 0))
	{
        //pr_info("do nothing...\n");
        return;
	}
	else if(status & POLARITY_DETECT_MASK)//POLARITY_DETECT
	{
		set_hdmirx_wrapper_interrupt_en(0,0,0);//Disable interrupt

		if(!check_hdmirx_resolution_match())//Format not match, just re-enable polarity detection
		{
			set_hdmirx_wrapper_interrupt_en(0,0,1);
			return;
		}

        HDMIRX_INFO("polarity detection done! st:%x, hor:%x, act:%x"
                                                       ,hdmi_rx_reg_read32(HDMIRX_WRAPPER_INTERRUPT_STATUS_reg, HDMI_RX_HDMI_WRAPPER)
                                                       ,hdmi_rx_reg_read32(HDMIRX_WRAPPER_DEN_BUF01_reg, HDMI_RX_HDMI_WRAPPER)
                                                       ,hdmi_rx_reg_read32(HDMIRX_WRAPPER_ACTIVE_PIXELS_reg, HDMI_RX_HDMI_WRAPPER));

		set_hdmirx_wrapper_active_pixels(timing->v_act_len, timing->h_act_len);
        set_hdmirx_wrapper_hor_threshold(timing->h_act_len);

        hdmi_ioctl_struct.measure_ready = 1;
		set_hdmirx_wrapper_interrupt_en(1,1,0);//Format match, enable error detection
	}
	else if(status & HOR_ERR_EN_MASK)
	{
		set_hdmirx_wrapper_interrupt_en(0,0,0);

		active_pixel =	hdmirx_wrapper_get_active_pixel();
		HDMIRX_INFO("ISR status:%x reenable polarity detection... pixel(%u)\n", status, active_pixel);
		restartHdmiRxWrapperDetection();

        return;
	}
	else if(status & VER_ERR_EN_MASK)
	{
		set_hdmirx_wrapper_interrupt_en(0,0,0);

		active_line = hdmirx_wrapper_get_active_line();
		HDMIRX_INFO("ISR status:%x reenable polarity detection... line(%u)\n", status, active_line);
		restartHdmiRxWrapperDetection();

        return;
    }
}

void hdmi_related_wrapper_init(void)
{
    //HDMIRX_INFO("[%s]", __FUNCTION__);
    set_mipi_env();

    set_hdmirx_wrapper_interrupt_en(0,0,0); //v,h,polarity
    set_hdmirx_wrapper_control_0(1,0,0,0,0,1);//fifo_stage,fw_dma_en,polarity_set_en,polarity_set,yuv_fmt,hdmirx_en
    set_hdmirx_wrapper_hor_threshold(HDMI_MIN_SUPPORT_H_PIXEL);
}
