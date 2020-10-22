#include "v4l2_hdmi_dev.h"
#include "phoenix_mipi_wrapper.h"
#include "phoenix_hdmi_wrapper.h"
#include "phoenix_hdmiHWReg.h"
#include "phoenix_hdmiInternal.h"
#include "hdmirx_rpc.h"
#include "sd_test.h"
#include "sd_test_hdmi_reg.h"
#include "sd_test_vo_reg.h"
#include "phoenix_mipi_i2c_ctl.h"

#include <linux/ratelimit.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/kthread.h>
//#include <asm-generic/io.h>
//#include <asm/io.h>
//#include <../arch/arm/include/asm/memory.h>
#include <asm/memory.h>

extern unsigned int hdmi_rx_output_format(unsigned int color);
extern wait_queue_head_t HDMIRX_WAIT_OSD_COMPLETE;

int hdmi_interlace_polarity_check = 0;
int hdmi_isr_count = 0;
extern int hdmi_stream_on;

void stop_mipi_process(void)
{
    //Reset MIPI, MIPI will be disabled and all MIPI register set to defalut value
    Wreg32(0x18000000, Rreg32(0x18000000)&0xBFFFFFFF);//set MIPI reset bit
	HDMI_DELAYUS(1);
	Wreg32(0x18000000, Rreg32(0x18000000)|BIT(30));	// release MIPI reset bit
	set_hdmirx_wrapper_control_0(-1, 0,-1,-1,-1,-1);//Stop DMA
    atomic_set(&hdmi_sw_buf_ctl.read_index, 0);
    atomic_set(&hdmi_sw_buf_ctl.write_index, 0);
    atomic_set(&hdmi_sw_buf_ctl.fill_index, 0);
    hdmi_sw_buf_ctl.pre_frame_done = -1;

    hdmi_sw_buf_ctl.use_v4l2_buffer = 0;
    hdmi_stream_on = 0;
    mipi_top.mipi_init = 0;
}

void set_mipi_env(void)
{
    HDMIRX_INFO("set_mipi_env");
    Wreg32(0x18000000, Rreg32(0x18000000)|BIT(30));	// release MIPI reset bit
    HDMI_DELAYMS(1);
    Wreg32(0x1800000c, Rreg32(0x1800000c)|BIT(27));	// enable MIPI clock
    HDMI_DELAYMS(1);
    Wreg32(0x18000044, 0x5);

	// on 1195
	hdmi_rx_reg_write32(MIPI_WRAPPER_APHY_REG15_reg, SEL_DIV_RG(DEMUX16), HDMI_RX_MIPI_PHY);//SEL_DIV_RG
	hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG4_reg, SL_RG(fourlane) | Sw_rst(disable), HDMI_RX_MIPI_PHY);//Lane mux sel & soft reset
#if CAMERA_4_LANE
	 // 4 lane
	hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_PWDB_reg, L3(enable) | L2(enable) | L1(enable) | L0(enable), HDMI_RX_MIPI_PHY);//Lane1 & Lane2 power on
	hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG0_reg,
		Div_sel(div_bit_16) | Lane3(enable) | Lane2(enable) | Lane1(enable) | Lane0(enable), HDMI_RX_MIPI_PHY);//16bit data width, Lane1 & Lane2 enable
#else
	// 2 lane
	hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_PWDB_reg, L1(enable) | L0(enable), HDMI_RX_MIPI_PHY);//Lane1 & Lane2 power on
	hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG0_reg, Div_sel(div_bit_16) | Lane1(enable) | Lane0(enable), HDMI_RX_MIPI_PHY);//16bit data width, Lane1 & Lane2 enable
#endif
	hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG9_reg, Lane3_clk_edge(posedge) | Lane2_clk_edge(posedge) |
							Lane1_clk_edge(posedge) | Lane0_clk_edge(posedge), HDMI_RX_MIPI_PHY);//Lane CLK edge

#if CAMERA_4_LANE
	if(hdmi.mipi_camera_type == MIPI_CAMERA_RTS5845)
		hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG1_reg, yuv_src_sel(SEL_UYVY) | dec_format(YUV422_dec_format), HDMI_RX_MIPI_PHY);// SEL_UYVY //SEL_YUYV & Dec data format for YUV422
	else
		hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG1_reg, yuv_src_sel(SEL_YUYV) | dec_format(YUV422_dec_format), HDMI_RX_MIPI_PHY);// SEL_UYVY //SEL_YUYV & Dec data format for YUV422
#else
	if (mipi_csi.output_color == OUT_RAW10)
		hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG1_reg,
			yuv_src_sel(SEL_UYVY) | dec_format(RAW10_dec_format), HDMI_RX_MIPI_PHY);
	else if (mipi_csi.output_color == OUT_RAW8)
		hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG1_reg,
			yuv_src_sel(SEL_UYVY) | dec_format(RAW8_dec_format), HDMI_RX_MIPI_PHY);
	else
		hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG1_reg,
			yuv_src_sel(SEL_YUYV) | dec_format(YUV422_dec_format), HDMI_RX_MIPI_PHY);// SEL_UYVY //SEL_YUYV & Dec data format for YUV422
#endif

	if (mipi_csi.output_color == OUT_RAW10)
		hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG2_reg, D_type(DTYPE_RAW10), HDMI_RX_MIPI_PHY);
	else if (mipi_csi.output_color == OUT_RAW8)
		hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG2_reg, D_type(DTYPE_RAW8), HDMI_RX_MIPI_PHY);
	else
		hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG2_reg, D_type(DTYPE_YUY8), HDMI_RX_MIPI_PHY);//MIPI data type YUV422 8bit


#if CAMERA_4_LANE
    if(hdmi.mipi_camera_type == MIPI_CAMERA_RTS5845)
    {
        hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG6_reg  , DVld_Lane1(0x6) | DVld_Lane0(0x6), HDMI_RX_MIPI_PHY);        // 0x6
        hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG7_reg  , DVld_Lane3(0x6) | DVld_Lane2(0x6), HDMI_RX_MIPI_PHY);
    }
    else if(hdmi.mipi_camera_type == MIPI_CAMERA_RLE0551C)
    {
        hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG6_reg  , DVld_Lane1(0x5) | DVld_Lane0(0x5), HDMI_RX_MIPI_PHY);
        hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG7_reg  , DVld_Lane3(0x5) | DVld_Lane2(0x5), HDMI_RX_MIPI_PHY);
    }
#else
	 hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG6_reg  , DVld_Lane1(0x5) | DVld_Lane0(0x5), HDMI_RX_MIPI_PHY);
#endif
}

void set_video_DDR_start_addr(struct v4l2_hdmi_dev *dev)
{
    unsigned int addr1y, addr1uv, addr2y, addr2uv;
    unsigned int offset;
    unsigned long flags = 0;
    struct hdmi_dmaqueue *hdmidq = &dev->hdmidq;
    if(mipi_top.src_sel == 1)
    {
        offset = roundup16(hdmi.tx_timing.pitch) * roundup16(hdmi.tx_timing.output_v);
        HDMIRX_INFO("pitch=%x,output_v=%x,offset=%x",hdmi.tx_timing.pitch,hdmi.tx_timing.output_v,offset);
    }
    else
        offset = roundup16(mipi_csi.pitch) * roundup16(mipi_csi.v_output_len + mipi_csi.auxiliary_inband_lines);

    spin_lock_irqsave(&dev->slock, flags);
    hdmidq->hwbuf[0] = list_entry(hdmidq->active.next, struct hdmi_buffer, list);
    list_del(&hdmidq->hwbuf[0]->list);
    //pr_info("%s: del buf[%d] from queue\n", __func__, hdmidq->hwbuf[0]->vb.v4l2_buf.index);
    hdmidq->hwbuf[1] = list_entry(hdmidq->active.next, struct hdmi_buffer, list);
    list_del(&hdmidq->hwbuf[1]->list);
    //pr_info("%s: del buf[%d] from queue\n", __func__, hdmidq->hwbuf[1]->vb.v4l2_buf.index);
    atomic_sub(2, &hdmidq->qcnt);

    addr1y = hdmidq->hwbuf[0]->phys;
    addr1uv = hdmidq->hwbuf[0]->phys + offset;
    addr2y = hdmidq->hwbuf[1]->phys;
    addr2uv = hdmidq->hwbuf[1]->phys + offset;
	set_video_dest_addr(addr1y, addr1uv, addr2y, addr2uv);
	spin_unlock_irqrestore(&dev->slock, flags);

    HDMIRX_INFO("[%s] addr1y(0x%08x) addr1uv(0x%08x) addr2y(0x%08x) addr2uv(0x%08x)",
				__FUNCTION__,addr1y,addr1uv,addr2y,addr2uv);
}

void set_mipi_type(unsigned int type)
{
    unsigned int reg_val;

    reg_val = hdmi_rx_reg_read32(MIPI_WRAPPER_TYPE_reg, HDMI_RX_MIPI);

    reg_val = reg_val & (MIPI_TYPE_MASK ^ 0xFFFFFFFF);
    reg_val = reg_val | MIPI_WRAPPER_TYPE_SET(type);

    hdmi_rx_reg_write32(MIPI_WRAPPER_TYPE_reg, reg_val, HDMI_RX_MIPI);
}

void set_pic_dest_addr(int addry, int addruv)
{
    if(addry >= 0)
    {
        hdmi_rx_reg_write32(MIPI_WRAPPER_SA2_reg, addry, HDMI_RX_MIPI);
    }
    if(addruv >= 0)
    {
        hdmi_rx_reg_write32(MIPI_WRAPPER_SA2_UV_reg, addruv, HDMI_RX_MIPI);
    }
}

void set_pic_dest_size(unsigned int dst_width, unsigned int pitch)
{
    unsigned int reg_val;

    reg_val = ((dst_width & 0x1FFF) << 16) | (pitch & 0xFFFF);
    hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_SIZE1_reg, reg_val, HDMI_RX_MIPI);
}

void set_pic_src_size(unsigned int src_width)
{
    unsigned int reg_val;

    reg_val = hdmi_rx_reg_read32(MIPI_WRAPPER_MIPI_SIZE2_reg, HDMI_RX_MIPI);
    reg_val = reg_val & (SRC_WIDTH_PIC_MASK ^ 0xFFFFFFFF);
    reg_val = reg_val | (src_width << 16);
    hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_SIZE2_reg, reg_val, HDMI_RX_MIPI);
}

void set_video_dest_addr(int addr1y, int addr1uv, int addr2y, int addr2uv)
{
    if(addr1y >= 0)
    {
        hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_SA0_reg, addr1y, HDMI_RX_MIPI);
    }
    if(addr1uv >= 0)
    {
        hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_SA0_UV_reg, addr1uv, HDMI_RX_MIPI);
    }
    if(addr2y >= 0)
    {
        hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_SA1_reg, addr2y, HDMI_RX_MIPI);
    }
    if(addr2uv >= 0)
    {
        hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_SA1_UV_reg, addr2uv, HDMI_RX_MIPI);
    }
}

void set_video_dest_size(unsigned int dst_width, unsigned int pitch)
{
    unsigned int reg_val;

    reg_val = ((dst_width & 0x1FFF) << 16) | (pitch & 0xFFFF);
    hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_SIZE0_reg, reg_val, HDMI_RX_MIPI);
}

void set_video_src_size(unsigned int src_width)
{
    unsigned int reg_val;

    reg_val = hdmi_rx_reg_read32(MIPI_WRAPPER_MIPI_SIZE2_reg, HDMI_RX_MIPI);
    reg_val = reg_val & (SRC_WIDTH_VIDEO_MASK ^ 0xFFFFFFFF);
    reg_val = reg_val | src_width;
    hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_SIZE2_reg, reg_val, HDMI_RX_MIPI);
}

void set_init_mipi_value(MIPI_REG *mipi_reg)
{
    unsigned int reg_val = 0;

    reg_val = (mipi_reg->dst_fmt << 27) |
                    (mipi_reg->src_fmt << 25) |
                    (mipi_reg->src_sel << 24) |
                    (mipi_reg->seq_en << 23) |
                    (mipi_reg->vs << 22) |
                    (mipi_reg->vs_near << 21) |
                    (mipi_reg->vs_yodd << 20) |
                    (mipi_reg->vs_codd << 19) |
                    (mipi_reg->hs << 18) |
                    (mipi_reg->hs_yodd << 17) |
                    (mipi_reg->hs_codd << 16) |
                    (mipi_reg->yuv_to_rgb << 12) |
                    (mipi_reg->chroma_ds_mode << 11) |
                    (mipi_reg->chroma_ds_en << 10) |
                    (mipi_reg->chroma_us_mode << 9) |
                    (mipi_reg->chroma_us_en << 8) |
                    (mipi_reg->hdmirx_interlace_en << 7) |
                    (mipi_reg->hdmirx_interlace_polarity << 6) |
                    (mipi_reg->int_en4 << 5) |
                    (mipi_reg->int_en3 << 4) |
                    (mipi_reg->int_en2 << 3) |
                    (mipi_reg->int_en1 << 2) |
                    (mipi_reg->int_en0 << 1) |
                    (mipi_reg->en);

    hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_reg, reg_val, HDMI_RX_MIPI);
    HDMIRX_INFO("MIPI_WRAPPER_MIPI_reg=%x",reg_val);
}

void set_hs_scaler(unsigned int hsi_offset, unsigned int hsi_phase, unsigned int hsd_out, unsigned int hsd_delta)
{
    unsigned int reg_val;

    reg_val = ((hsi_offset & 0x07FF)<<16) | ((hsi_phase &0x3FFF)<<2) ;
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSI_reg, reg_val, HDMI_RX_MIPI);

    reg_val = ((hsd_out & 0x07FF)<<20) | (hsd_delta &0x7FFFF) ;
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSD_reg, reg_val, HDMI_RX_MIPI);
    HDMIRX_INFO("hsd_out=%x,hsd_delta=%x",hsd_out,hsd_delta);
}

void set_hs_coeff(void)
{
    unsigned int reg_val;

    //for Y
    reg_val = (0x400<<16) | 0x400 ;
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSYNC0_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSYNC1_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSYNC2_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSYNC3_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSYNC4_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSYNC5_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSYNC6_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSYNC7_reg, reg_val, HDMI_RX_MIPI);

    //for U,V
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSCC0_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSCC1_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSCC2_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSCC3_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSCC4_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSCC5_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSCC6_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_HSCC7_reg, reg_val, HDMI_RX_MIPI);
}

void set_vs_scaler(unsigned int vsi_offset, unsigned int vsi_phase, unsigned int vsd_out, unsigned int vsd_delta)
{
    unsigned int reg_val;

    reg_val = ((vsi_offset & 0x0FFF)<<16) | ((vsi_phase &0x3FFF)<<2) ;
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_VSI_reg, reg_val, HDMI_RX_MIPI);

    reg_val = ((vsd_out & 0x07FF)<<20) | (vsd_delta &0x7FFFF) ;
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_VSD_reg, reg_val, HDMI_RX_MIPI);
    HDMIRX_INFO("vsd_out=%x,vsd_delta=%x",vsd_out,vsd_delta);
}

void set_vs_coeff(void)
{
    unsigned int reg_val;

    reg_val = (0x800<<16) | 0x800 ;
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_VSYC0_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_VSCC0_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_VSYC1_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_VSCC1_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_VSYC2_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_VSCC2_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_VSYC3_reg, reg_val, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_SCALER_VSCC3_reg, reg_val, HDMI_RX_MIPI);
}

void set_alpha(unsigned int alpha)
{
    unsigned int reg_val;

    reg_val = alpha & 0x0FF;
    hdmi_rx_reg_write32(MIPI_WRAPPER_CONSTANT_ALPHA_reg, reg_val, HDMI_RX_MIPI);
}

void set_YUV2RGB_coeff(void)
{
    hdmi_rx_reg_write32(MIPI_WRAPPER_CS_TRANS0_reg,  0x04a80, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_CS_TRANS1_reg,  0x00000, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_CS_TRANS2_reg,  0x072c0, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_CS_TRANS3_reg,  0x04a80, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_CS_TRANS4_reg,  0x1f260, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_CS_TRANS5_reg,  0x1ddd0, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_CS_TRANS6_reg,  0x04a80, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_CS_TRANS7_reg,  0x08760, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_CS_TRANS8_reg,  0x00000, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_CS_TRANS9_reg,  0x0fc20, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_CS_TRANS10_reg, 0x00134, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_CS_TRANS11_reg, 0x0fb7c, HDMI_RX_MIPI);
}

void phoenix_mipi_format_transform(void)
{
    //set RGB
    set_alpha(0xff);
}

void phoenix_mipi_scale_down(unsigned int src_width,unsigned int src_height,unsigned int dst_width,unsigned int dst_height)
{
    unsigned int delta_num, delta_den, offset, phase;

	if(mipi_top.src_sel == 1)//HDMI RX
	{
    	if(hdmi_rx_reg_read32(HDMI_HDMI_VCR_VADDR, HDMI_RX_MAC) & _BIT6) // interlace mode
       		dst_height = (dst_height>>1);
    }

    if(src_width > dst_width)
    {
        //set hs_scaler
        offset = 0;
        phase = 0;
        delta_num = (src_width / dst_width) << 14;
        delta_den = ((src_width % dst_width)*0x4000) / dst_width ;
        set_hs_scaler(offset,phase,dst_width, (delta_num | delta_den));//offset,phase,out,delta
        set_hs_coeff();
    }
    if(src_height > dst_height)
    {
        //set vs_scaler
        offset = 0;
        phase = 0;
        delta_num = (src_height / dst_height) << 14;
        delta_den = ((src_height % dst_height)*0x4000) / dst_height ;
        set_vs_scaler(offset,phase,dst_height, (delta_num | delta_den));//offset,phase,out,delta
        set_vs_coeff();
    }
}


int hdmirx_inband_format_type_select(void)
{
    if(hdmi.tx_timing.output_color == OUT_RAW8)
        return INBAND_CMD_GRAPHIC_FORMAT_8BIT;
    else if(hdmi.tx_timing.output_color == OUT_YUV422)
        return INBAND_CMD_GRAPHIC_FORMAT_422;
    else if(hdmi.tx_timing.output_color == OUT_YUV420)
        return INBAND_CMD_GRAPHIC_FORMAT_420;
    else if((hdmi.tx_timing.output_color >= OUT_ARGB)&&(hdmi.tx_timing.output_color <= OUT_ABGR))
        return INBAND_CMD_GRAPHIC_FORMAT_ARGB8888;
    else if((hdmi.tx_timing.output_color >= OUT_RGBA)&&(hdmi.tx_timing.output_color <= OUT_BGRA))
        return INBAND_CMD_GRAPHIC_FORMAT_RGBA8888;
    else
        return INBAND_CMD_GRAPHIC_FORMAT_Reserved0 ;// VO not support
}

void setup_mipi(void)
{
    unsigned int color_index;
    static unsigned int pre_v_act_len = 0, pre_h_act_len = 0;
    MIPI_REG mipi_reg;
    unsigned int dst_height;
    HDMI_TIMING_T *timing = &hdmi.tx_timing;
	unsigned int interlace_mode = 0;

	HDMIRX_INFO("[%s]",__FUNCTION__);

    set_mipi_env();

    set_video_dest_size(timing->output_h,timing->pitch);    //width,pitch
    set_video_src_size(timing->h_act_len);  
    //scale down
    phoenix_mipi_scale_down(timing->h_act_len, timing->v_act_len, timing->output_h, timing->output_v);

    hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG11_reg, Esc_lane3_flg(clear)|Esc_lane2_flg(clear)|Esc_lane2_flg(clear)|Esc_lane1_flg(clear)|
           Esc_lane0_flg(clear)|Clk_ulps_flg(clear)|Crc16_err_flg(clear)|Ecc_crt_flg(clear)|Ecc_err_flg(clear), HDMI_RX_MIPI_PHY);

    //set main reg
    if((pre_v_act_len != timing->v_act_len)||(pre_h_act_len != timing->h_act_len))
        hdmi_interlace_polarity_check = 0;
    memset(&mipi_reg, 0, sizeof(MIPI_REG));
    mipi_reg.dst_fmt = timing->output_color;
    mipi_reg.src_fmt = hdmi_rx_output_format(timing->color);
    mipi_reg.src_sel = mipi_top.src_sel;   //0:MIPI 1:HDMI_RX
    mipi_reg.seq_en = 1;    //0:blk 1:seq (for YUV)

	if (mipi_reg.src_sel == 1)
		interlace_mode = hdmi_rx_reg_read32(HDMI_HDMI_VCR_VADDR, HDMI_RX_MAC) & _BIT6;

    if (interlace_mode)
       dst_height = (timing->output_v>>1);
    else
       dst_height = timing->output_v;

    if(timing->v_act_len > dst_height)
        mipi_reg.vs = 1;

//    mipi_reg.vs_near = 0;
//    mipi_reg.vs_yodd = 0;
//    mipi_reg.vs_codd = 0;
    if(timing->h_act_len > timing->output_h)
        mipi_reg.hs = 1;
//    mipi_reg.hs_yodd = 0;
//    mipi_reg.hs_codd = 0;
    if((Rreg32(0x1801A200) & 0xFFFF) == 0x6329 && timing->color == COLOR_YUV422){ //don't enable yuv_to_rgb on 1195 input 422
       mipi_reg.yuv_to_rgb = 0;
    }else if((timing->color != COLOR_RGB)&&(timing->output_color > OUT_JPEG8)){ //YUV -> RGB
        mipi_reg.yuv_to_rgb = 1;
        set_alpha(0xff);
        set_YUV2RGB_coeff();
    }else if((timing->color == COLOR_RGB)&&(timing->output_color < OUT_JPEG8)){ //RGB -> YUV
        mipi_reg.yuv_to_rgb = 1;
        set_alpha(0xff);
    }
    if((timing->color == COLOR_YUV444) &&
        ((timing->output_color == OUT_YUV422)||(timing->output_color == OUT_YUV420)))
    {
        mipi_reg.chroma_ds_mode = 0;    //0:drop 1:avg
        mipi_reg.chroma_ds_en = 1;
    }
    if((timing->color == COLOR_YUV422)&&
        ((mipi_reg.vs) || (mipi_reg.hs)) )
    {
        mipi_reg.chroma_us_mode = 0;    //0:repeat 1:avg
        mipi_reg.chroma_us_en = 1;
        if((timing->output_color == OUT_YUV422)||(timing->output_color == OUT_YUV420))
        {
            mipi_reg.chroma_ds_mode = 0;    //0:drop 1:avg
            mipi_reg.chroma_ds_en = 1;
        }    
    }

	if (interlace_mode) {
		/* Interlace mode */
		mipi_reg.hdmirx_interlace_en = 1;

		//pr_err("0x1801A200:%x 0x1801A204:%x\n", Rreg32(0x1801A200), Rreg32(0x1801A204));
		if((Rreg32(0x1801A200) & 0xFFFF) == 0x6329) //for 1195, polarity = 0 is actually bottom field first
			mipi_reg.hdmirx_interlace_polarity = 1;
		else
			mipi_reg.hdmirx_interlace_polarity = 0;
	}

    mipi_reg.int_en4 = 1;
    mipi_reg.int_en3 = 1;
    mipi_reg.int_en2 = 1;
    mipi_reg.int_en1 = 1;
    mipi_reg.int_en0 = 1;

	mipi_reg.en = 1;

    set_init_mipi_value(&mipi_reg);
    set_hdmirx_wrapper_control_0(-1, 1, mipi_reg.hdmirx_interlace_en, mipi_reg.hdmirx_interlace_polarity,-1,-1);
    mipi_top.mipi_init = 1;

}


void update_mipi_hw_buffer(int src_index, struct v4l2_hdmi_dev *dev)
{
    unsigned int addry, addruv, offset;
    unsigned long flags = 0;
    struct hdmi_dmaqueue *hdmidq = &dev->hdmidq;

    if(mipi_top.src_sel == 1)
        offset = roundup16(hdmi.tx_timing.pitch) * roundup16(hdmi.tx_timing.output_v);
    else
        offset = roundup16(mipi_csi.pitch) * roundup16(mipi_csi.v_output_len + mipi_csi.auxiliary_inband_lines);

    spin_lock_irqsave(&dev->slock, flags);

    vb2_buffer_done(&hdmidq->hwbuf[src_index]->vb,VB2_BUF_STATE_DONE);
    atomic_inc(&hdmidq->rcnt);

    hdmidq->hwbuf[src_index] = list_entry(hdmidq->active.next, struct hdmi_buffer, list);
    list_del(&hdmidq->hwbuf[src_index]->list);
    atomic_dec(&hdmidq->qcnt);

    spin_unlock_irqrestore(&dev->slock, flags);

    addry = hdmidq->hwbuf[src_index]->phys;
    addruv = hdmidq->hwbuf[src_index]->phys + offset;

    if(src_index == 0)
        set_video_dest_addr(addry, addruv, -1, -1);
    else
        set_video_dest_addr(-1, -1, addry, addruv);

}

void hdmi_hw_buf_update(int src_index, struct v4l2_hdmi_dev *dev)
{
    static int skip_num = 0;
    static int total_num = 0;
    static unsigned long prev_jif = 0;
    int empty = 0;
    unsigned long flags = 0;
    struct hdmi_dmaqueue *hdmidq = &dev->hdmidq;

    if(hdmi_sw_buf_ctl.pre_frame_done == -1)
          hdmi_sw_buf_ctl.pre_frame_done = src_index;

    hdmi_isr_count++;
    if((mipi_top.src_sel == 1) && (hdmi_interlace_polarity_check))
    {
        unsigned int reg_val = hdmi_rx_reg_read32(MIPI_WRAPPER_MIPI_reg, HDMI_RX_MIPI);

        HDMIRX_INFO("Interlace change Change. Drop frame");
        hdmi_interlace_polarity_check = 0;
        reg_val = reg_val & 0xFFFFFFBF; // clear bit 6
        if(hdmi_rx_reg_read32(HDMI_HDMI_VCR_VADDR, HDMI_RX_MAC) & _BIT27)
            reg_val = reg_val | (1<<6);

        hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_reg, reg_val, HDMI_RX_MIPI);
        return;
    }

    if ((hdmi_sw_buf_ctl.use_v4l2_buffer == 0)
        ||(mipi_top.mipi_init == 0))
    {
        //pr_info("[HW UPDATE R]\n");
        return;
    }

    if(unlikely((jiffies - prev_jif) >= (10 * HZ))){
        HDMIRX_INFO("hdmirx skip %d/%d in %lu jiffies", skip_num, total_num, jiffies - prev_jif);
        prev_jif = jiffies;
        total_num = 0;
        skip_num = 0;
    }
    total_num++;

    spin_lock_irqsave(&dev->slock, flags);
    empty = list_empty(&hdmidq->active);
    spin_unlock_irqrestore(&dev->slock, flags);
    if(empty){
        //pr_notice_ratelimited("No active queue to serve ?????? %d-2-%d\n", atomic_read(&hdmidq->rcnt), atomic_read(&hdmidq->qcnt));
        skip_num++;
        return;
    }
    update_mipi_hw_buffer(src_index, dev);

}

extern void hdmirx_wrapper_isr(void);
irqreturn_t phoenix_mipi_isr(int irq, void* dev_id)
{
    unsigned int st_reg_val, reg_val;
    struct v4l2_hdmi_dev *dev = dev_id;
    st_reg_val = hdmi_rx_reg_read32(MIPI_WRAPPER_MIPI_INT_ST_reg, HDMI_RX_MIPI);

    //HDMIRX_INFO("MIPI ISR");

    if(st_reg_val & 0x30) // Buffer is overflow or one frame is dropped
    {
        //clear interrupt status
        hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_INT_ST_reg, st_reg_val, HDMI_RX_MIPI);

        HDMIRX_INFO("MIPI frame dropped");

        return IRQ_HANDLED;
    }

    reg_val = hdmi_rx_reg_read32(MIPI_WRAPPER_TYPE_reg, HDMI_RX_MIPI);

    if(reg_val & 0x01) // Picture mode
    {
        if(st_reg_val & 0x08) // fm_done2:One image is written to mipi_sa2
        {
            //Clear interrupt status
            hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_INT_ST_reg, st_reg_val, HDMI_RX_MIPI);
        }
    }
    else // Preview or video mode
    {
        //Clear interrupt status
        hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_INT_ST_reg, st_reg_val, HDMI_RX_MIPI);
        if(st_reg_val & 0x02)// fm_done0:One image is written to mipi_index0/mipi_sa0
        {
            hdmi_hw_buf_update(0,dev);
        }
        else if(st_reg_val & 0x04)// fm_done1:One image is written to mipi_index1/mipi_sa1
        {
            hdmi_hw_buf_update(1,dev);
        }
    }

    if(mipi_top.src_sel == 1)//HDMI RX
    	hdmirx_wrapper_isr();

    return IRQ_HANDLED;
}

