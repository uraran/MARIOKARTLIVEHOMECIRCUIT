/*
 * $Id: hello.c,v 1.5 2004/10/26 03:32:21 corbet Exp $
 */
#include "phoenix_mipi_i2c_ctl.h"
#include "v4l2_hdmi_dev.h"
#include "phoenix_hdmiFun.h"
#include "phoenix_hdmiInternal.h"
#include "phoenix_mipi_wrapper.h"
#include "phoenix_hdmi_wrapper.h"
#include "sd_test.h"
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/switch.h>
#include <linux/time.h>

#define MIPI_CAMERA_IN_W 1920//2592
#define MIPI_CAMERA_IN_H 1080//1944

#define MAX_WIDTH 2592
#define MAX_HEIGHT 1944

#define FCC2ASCII(fmt)  ((char *)(&fmt))[0], ((char *)(&fmt))[1], ((char *)(&fmt))[2], ((char *)(&fmt))[3]


//static unsigned int vid_limit = 16;
int hdmi_thread_exist = 0;
int hdmi_stream_on = 0;

int rx_5v_state = 0;

extern char Hdmi_IsbReady(void);
extern int Hdmi_get_b_value(void);
#ifdef CRC_CHECK
extern int QC_CRCCheck(UINT8 type);
extern int HDMI_FormatDetect(void);
#endif
extern void OV5645_2lane_1080P30(void);
extern void OV5645_2lane_1920x1080_raw8(void);
extern void OV5645_2lane_1280x720_raw8(void);
extern void RLE0551C_2048x1536(void);
extern void RLE0551C_1280x720_raw(void);
extern void OV5647_2lane_1280x720_raw8(void);
extern void OV9782_2lane_1280x720_raw8(void);
extern int OV9782_2lane_1280x800(unsigned int color);
extern int OV9782_set_group(struct group_switch_config *config, int stream_on);
extern void OV9782_get_group(struct group_switch_config *config);
extern int OV9782_set_awb(struct awb_gain_config *gain, int stream_on);
extern void OV9782_get_awb(struct awb_gain_config *gain);
extern void register_mipi_i2c_sysfs(struct platform_device *pdev);

int mipi_csi_process(void);

DECLARE_WAIT_QUEUE_HEAD(HDMIRX_WAIT_OSD_COMPLETE);


static const struct of_device_id hdmirx_rtk_dt_ids[] = {
	{ .compatible = "Realtek,rtk119x-mipi-top", },
	{},
};
MODULE_DEVICE_TABLE(of, hdmirx_rtk_dt_ids);

#define RTK_HDMIRX_NAME "rtk-mipi-top"

MODULE_ALIAS("platform:" RTK_HDMIRX_NAME);

MODULE_LICENSE("Dual BSD/GPL");
static LIST_HEAD(hdmi_devlist);
extern void RLE0551C_patterngen_2048x1536(void);
extern void RTS5845(unsigned int h_input_len, unsigned int v_input_len);


struct platform_driver hdmirx_rtk_driver = {
    .probe      = hdmirx_rtk_drv_probe,
    .remove     = hdmirx_rtk_drv_remove,
	.suspend	= hdmirx_rtk_drv_suspend,
	.resume		= hdmirx_rtk_drv_resume,
    .shutdown            = hdmi_platform_shutdown,
    .driver     = {
        .name   = RTK_HDMIRX_NAME,
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(hdmirx_rtk_dt_ids),
    },
};
#define	HDMIRX_PLATFORM_DRIVER		hdmirx_rtk_driver

struct hdmirx_format_desc {
    char *name;
    __u32 fcc;
    char compressed;
};

static const struct hdmirx_format_desc hdmi_rx_SGRBG8_fmt =
{
    .name       = "RAW8",
    .fcc        = V4L2_PIX_FMT_SGRBG8 ,
    .compressed = 0,
};

static const struct hdmirx_format_desc hdmi_rx_SGRBG10_fmt =
{
    .name       = "RAW10",
    .fcc        = V4L2_PIX_FMT_SGRBG10,
    .compressed = 0,
};

#if 0
// XXX: could be used by RTK past cameras, enable them back at will
static const struct hdmirx_format_desc hdmi_rx_NV16_fmt =
{
    .name       = "Y/CbCr 4:2:2",
    .fcc        = V4L2_PIX_FMT_NV16,
    .compressed = 0,
};

static const struct hdmirx_format_desc hdmi_rx_NV12_fmt =
{
    .name       = "YUV 4:2:0 (NV12)",
    .fcc        = V4L2_PIX_FMT_NV12,
    .compressed = 0,
};

static const struct hdmirx_format_desc hdmi_rx_JPEG8_fmt =
{
    .name       = "JPEG8",
    .fcc        = V4L2_PIX_FMT_MJPEG,
    .compressed = 1,
};

static const struct hdmirx_format_desc hdmi_rx_ARGB_fmt =
{
    .name       = "ARGB",
    .fcc        = V4L2_PIX_FMT_RGB32,
    .compressed = 0,
};

static const struct hdmirx_format_desc hdmi_rx_BGR32_fmt =
{
    .name       = "BGRA",
    .fcc        = V4L2_PIX_FMT_BGR32,
    .compressed = 0,
};

static const struct hdmirx_format_desc* enum_fmt_RLE0551C[] =
{
    // XXX: RTK fill this up correctly or drop support
};

static const struct hdmirx_format_desc* enum_fmt_RTS5845[] =
{
    // XXX: RTK fill this up correctly or drop support
};

static const struct hdmirx_format_desc* enum_fmt_OV5645[] =
{
    &hdmi_rx_SGRBG8_fmt
};

static const struct hdmirx_format_desc* enum_fmt_OV5647[] =
{
    &hdmi_rx_SGRBG8_fmt
};
#endif

static const struct hdmirx_format_desc* enum_fmt_OV9782[] =
{
    &hdmi_rx_SGRBG10_fmt, &hdmi_rx_SGRBG8_fmt
};

struct supported_camera_desc
{
    MIPI_CAMERA_TYPE cameraid;
    enum v4l2_buf_type buffertype;
    size_t numfmts;
    const struct hdmirx_format_desc** fmts;
    u16 maxwidth;
    u16 maxheight;
    unsigned int auxiliary_inband_lines;
};

static const struct supported_camera_desc supported_camera[] =
{
#if 0
    {
        // XXX RTK check or drop support
        .cameraid = MIPI_CAMERA_RLE0551C,
        .buffertype = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .numfmts = sizeof(enum_fmt_RLE0551C)/sizeof(enum_fmt_RLE0551C[0]),
        .fmts = &enum_fmt_RLE0551C[0],
        .maxwidth = 1280,
        .maxheight = 720,
        .auxiliary_inband_lines = 0,
    },
    {
        // XXX RTK check or drop support
        .cameraid = MIPI_CAMERA_RTS5845,
        .buffertype = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .numfmts = sizeof(enum_fmt_RTS5845)/sizeof(enum_fmt_RTS5845[0]),
        .fmts = enum_fmt_RTS5845,
        .maxwidth = 1280,
        .maxheight = 720,
        .auxiliary_inband_lines = 0,
    },
    {
        // XXX RTK check or drop support
        .cameraid = MIPI_CAMERA_OV5645,
        .buffertype = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .numfmts = sizeof(enum_fmt_OV5645)/sizeof(enum_fmt_OV5645[0]),
        .fmts = enum_fmt_OV5645,
        .maxwidth = 1280,
        .maxheight = 720,
        .auxiliary_inband_lines = 0,
    },
    {
        // XXX RTK check or drop support
        .cameraid = MIPI_CAMERA_OV5647,
        .buffertype = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .numfmts = sizeof(enum_fmt_OV5647)/sizeof(enum_fmt_OV5647[0]),
        .fmts = enum_fmt_OV5647,
        .maxwidth = 1280,
        .maxheight = 720,
        .auxiliary_inband_lines = 0,
    },
#endif
    {
        .cameraid = MIPI_CAMERA_OV9782,
        .buffertype = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .numfmts = sizeof(enum_fmt_OV9782)/sizeof(enum_fmt_OV9782[0]),
        .fmts = enum_fmt_OV9782,
        .maxwidth = 1280,
        .maxheight = 800,
        .auxiliary_inband_lines = 1,
    },
};

static const size_t supported_camera_nbelms = sizeof(supported_camera) / sizeof(supported_camera[0]);

/*************************************
        EDID DTS Definition
**************************************/
static const char *hdmi_rx_edid_tbl_node[] = {
     "Realtek,edid-tbl0",
     "Realtek,edid-tbl1",
     "Realtek,edid-tbl2",
     "Realtek,edid-tbl3",
     "Realtek,edid-tbl4",
     "Realtek,edid-tbl5",
     "Realtek,edid-tbl6",
     "Realtek,edid-tbl7",
     "Realtek,edid-tbl8",
     "Realtek,edid-tbl9",
     "Realtek,edid-tbl10",
     "Realtek,edid-tbl11",
     "Realtek,edid-tbl12",
     "Realtek,edid-tbl13",
     "Realtek,edid-tbl14",
     "Realtek,edid-tbl15",
     "Realtek,edid-tbl16",
     "Realtek,edid-tbl17",
     "Realtek,edid-tbl18",
     "Realtek,edid-tbl19",
     "Realtek,edid-tbl20",
     "Realtek,edid-tbl21",
     "Realtek,edid-tbl22",
     "Realtek,edid-tbl23",
     "Realtek,edid-tbl24",
     "Realtek,edid-tbl25",
     "Realtek,edid-tbl26",
     "Realtek,edid-tbl27",
     "Realtek,edid-tbl28",
     "Realtek,edid-tbl29",
     "Realtek,edid-tbl30",
     "Realtek,edid-tbl31"
};


HDMI_SW_BUF_CTL hdmi_sw_buf_ctl;

static struct task_struct *hdmi_hw_tsk;

static int mipi_top_thread(void *arg);

int out_color_to_bpp(MIPI_OUT_COLOR_SPACE_T output_color)
{
    switch(output_color){
    case OUT_RAW8:      //V4L2_PIX_FMT_SGRBG8
        return 16;
    case OUT_RAW10:     //V4L2_PIX_FMT_SGRBG10
        return 16;
    case OUT_YUV420:    //V4L2_PIX_FMT_NV12
        return 12;
    case OUT_YUV422:    //
    case OUT_JPEG8:     //V4L2_PIX_FMT_MJPEG
        return 16;
    default:            //all other ARGB formats
        return 32;
    }
}

MIPI_OUT_COLOR_SPACE_T V4L2_format_parse(unsigned int pixelformat)
{
    switch(pixelformat){
        case V4L2_PIX_FMT_SGRBG8:
            return OUT_RAW8;
        case V4L2_PIX_FMT_SGRBG10:
            return OUT_RAW10;
        case V4L2_PIX_FMT_NV12:
            return OUT_YUV420;
        case V4L2_PIX_FMT_MJPEG:
            return OUT_JPEG8;
        case V4L2_PIX_FMT_RGB32:
            if((Rreg32(0x1801A200) & 0xFFFF) == 0x6329){ //for 1195 input 422 format, Y in G component, Cb/Cr in R component
                //return OUT_RBGA;    //YUV422 to YUYV workaround
                return OUT_ABRG;    //YUV422 to NV12 workaround, Y Cb x x  Y Cr x x  Y Cb x x  Y Cr x x
            }else{
                return OUT_BGRA;    // byte 0~3: ARGB
            }
        case V4L2_PIX_FMT_BGR32:
            return OUT_ARGB;    // byte 0~3: BGRA, for HAL_PIXEL_FORMAT_BGRA_8888
        case V4L2_PIX_FMT_YUYV:
            return OUT_YUV422;  //FIXME: YUYV => YUV422?
        default:
            HDMIRX_ERROR("Unsupported pixel format: %c%c%c%c", FCC2ASCII(pixelformat));
            return OUT_UNKNOW;
    }

    return  OUT_UNKNOW;
}

static int hdmirx_queue_init(struct v4l2_hdmi_dev *dev, struct vb2_queue *q, enum v4l2_memory memory)
{
    if(q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) //already initialized, release first
        vb2_queue_release(q);
    q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    q->io_modes = VB2_MMAP | VB2_USERPTR; //| VB2_DMABUF | VB2_READ;
    q->drv_priv = dev;
    q->buf_struct_size = sizeof(struct hdmi_buffer);
    q->ops = &hdmi_qops;
    q->timestamp_type = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;

    if(memory == V4L2_MEMORY_USERPTR){
        q->mem_ops = &vb2_vmalloc_memops;
    }else if(memory == V4L2_MEMORY_MMAP){
        q->mem_ops = &vb2_dma_contig_memops;
    }else{
        HDMIRX_ERROR("[%s] Invalid io mode:%d",__FUNCTION__,memory);
        return -EINVAL;
    }

    return vb2_queue_init(q);
}

static inline const struct supported_camera_desc* v4l2_hdmi_get_camera(unsigned int camera_type)
{
    size_t i;
    for(i=0; i<supported_camera_nbelms; i++) {
        if (supported_camera[i].cameraid == camera_type) {
            return &supported_camera[i];
        }
    }
    return NULL;
}

static long v4l2_hdmi_do_ioctl(struct file *file, unsigned int cmd, void *arg)
{
    long ret = 0;
    struct video_device *vdev = video_devdata(file);
    struct v4l2_hdmi_dev *dev = video_drvdata(file);

	//pr_err("[HDMI RX] ioctl TYPE(0x%x) NR(%u) SIZE(%u)\n", _IOC_TYPE(cmd),_IOC_NR(cmd),_IOC_SIZE(cmd));

    switch (cmd) {
        case VIDIOC_QUERYCAP:
            {
                struct v4l2_capability *cap = arg;

                //HDMIRX_INFO(" ioctl VIDIOC_QUERYCAP\n");
                memset(cap, 0, sizeof *cap);

                strlcpy(cap->driver, "v4l2_mipi_top", sizeof cap->driver);
                strlcpy(cap->card, vdev->name, sizeof cap->card);
                strlcpy(cap->bus_info, "MIPI", sizeof cap->bus_info);
                cap->version = LINUX_VERSION_CODE;
                cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
                cap->device_caps = cap->capabilities;
                break;
            }
        case VIDIOC_CROPCAP:
            {
                struct v4l2_cropcap *ccap = arg;

                if (hdmi.mipi_input_type) {
                    // HDMI RX
                    ccap->bounds.left = 0;
                    ccap->bounds.top = 0;
                    ccap->bounds.width = 1920;
                    ccap->bounds.height = 1080;
                    ccap->defrect = ccap->bounds;
                    ccap->pixelaspect.numerator = 1;
                    ccap->pixelaspect.denominator = 1;
                } else {
                    // Camera
                    const struct supported_camera_desc* camera;
                    camera = v4l2_hdmi_get_camera(hdmi.mipi_camera_type);
                    if (!camera)
                        return -EINVAL;

                    ccap->bounds.left = 0;
                    ccap->bounds.top = 0;
                    ccap->bounds.width = camera->maxwidth;
                    ccap->bounds.height = camera->maxheight;
                    ccap->defrect = ccap->bounds;
                    ccap->pixelaspect.numerator = 1;
                    ccap->pixelaspect.denominator = 1;
                }
                break;
            }
        case VIDIOC_G_CROP:
        case VIDIOC_S_CROP:
            {
                return -EINVAL;
            }
        case VIDIOC_ENUM_FMT:
            {
                struct v4l2_fmtdesc *fmt = arg;

                if (hdmi.mipi_input_type)
                {
                    // XXX: RTK fill in for HDMIRX output format
                } else {
                    const struct hdmirx_format_desc* format;
                    const struct supported_camera_desc* camera;
                    __u32 index = fmt->index;

                    // prepare output
                    memset(fmt, 0, sizeof(*fmt));
                    camera = v4l2_hdmi_get_camera(hdmi.mipi_camera_type);
                    if (!camera || index >= camera->numfmts)
                        return -EINVAL;

                    format = camera->fmts[index];

                    fmt->index = index;
                    fmt->type = camera->buffertype;
                    fmt->flags = format->compressed ? V4L2_FMT_FLAG_COMPRESSED : 0;
                    strlcpy(fmt->description, format->name, sizeof fmt->description);
                    fmt->description[sizeof fmt->description - 1] = 0;
                    fmt->pixelformat = format->fcc;
                }
                break;
            }
        case VIDIOC_ENUM_FRAMESIZES:
            {
                struct v4l2_frmsizeenum *fsize = arg;
                int i=0;
                //struct hdmi_rx_frame *frame;

                //HDMIRX_INFO(" ioctl VIDIOC_ENUM_FRAMESIZES");
                /* Look for the given pixel format */
                for(i=0; i<HDMI_MEASURE_RETRY_TIMES; i++)
                {
                    if(hdmi_ioctl_struct.measure_ready == 1)
                        break;
                    HDMI_DELAYMS(10);
                }
                if ((fsize->index != 0)||((hdmi_ioctl_struct.measure_ready == 0)&&(mipi_top.src_sel == 1)))
                    return -EINVAL;

                if(mipi_top.src_sel == 1)
                {
                    int v_len;

					for(i=0; i<HDMI_MEASURE_RETRY_TIMES; i++)
					{
						if(hdmi_ioctl_struct.measure_ready == 1)
                        	break;
						HDMI_DELAYMS(10);
					}

					if ((fsize->index != 0)||(hdmi_ioctl_struct.measure_ready == 0))
                    	return -EINVAL;

                    if(!hdmi.tx_timing.progressive)
                        v_len = (hdmi.tx_timing.v_act_len << 1);
                    else
                        v_len = (hdmi.tx_timing.v_act_len);

                    fsize->type = V4L2_FRMSIZE_TYPE_CONTINUOUS;
                    fsize->stepwise.min_width = (hdmi.tx_timing.h_act_len)/32;
                    fsize->stepwise.max_width = hdmi.tx_timing.h_act_len;
                    fsize->stepwise.step_width = 1;
                    fsize->stepwise.min_height = v_len/32;
                    fsize->stepwise.max_height = v_len;
                    fsize->stepwise.step_height = 1;
                    HDMIRX_INFO(" Act_hor:%u,Act_ver:%u",hdmi.tx_timing.h_act_len,hdmi.tx_timing.v_act_len);
                }
                else
                {
                    const struct supported_camera_desc* camera;
                    camera = v4l2_hdmi_get_camera(hdmi.mipi_camera_type);
                    if (!camera)
                        return -EINVAL;

                    fsize->type = V4L2_FRMSIZE_TYPE_CONTINUOUS;
                    fsize->stepwise.min_width = camera->maxwidth/32;
                    fsize->stepwise.max_width = camera->maxwidth;
                    fsize->stepwise.step_width = 1;
                    fsize->stepwise.min_height = camera->maxheight/32;
                    fsize->stepwise.max_height = camera->maxheight;
                    fsize->stepwise.step_height = 1;
                }
                break;
            }
        case VIDIOC_ENUM_FRAMEINTERVALS:
            {
                struct v4l2_frmivalenum *fival = arg;

                if (fival->index != 0)
                    return -EINVAL;

                fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
                fival->discrete.numerator = 1;
                if(mipi_top.src_sel == 1)
                {
					unsigned int vic;
					vic = drvif_Hdmi_AVI_VIC();
					if(hdmi_vic_table[vic].interlace)
						fival->discrete.denominator = hdmi_vic_table[vic].fps/2;
					else
						fival->discrete.denominator = hdmi_vic_table[vic].fps;
                }
                else
                    fival->discrete.denominator = 60;
                break;
            }
		case VIDIOC_G_FMT:
		{
			struct v4l2_format *fmt = arg;

			/* HDMIRX_INFO("ioctl VIDIOC_G_FMT"); */
			fmt->fmt.pix.width = mipi_csi.h_output_len;
			fmt->fmt.pix.height = mipi_csi.v_output_len;

			if (mipi_csi.output_color == OUT_RAW10)
				fmt->fmt.pix.pixelformat = V4L2_PIX_FMT_SGRBG10;
			else if (mipi_csi.output_color == OUT_RAW8)
				fmt->fmt.pix.pixelformat = V4L2_PIX_FMT_SGRBG8;
			else
				fmt->fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;

			break;
		}
        case VIDIOC_S_FMT:
        {
            const struct supported_camera_desc* camera;
            struct v4l2_format *fmt;
            int fccfound;
            size_t i;

            /* HDMI RX S_FMT is unsupported */
            if (hdmi.mipi_input_type)
                return -EINVAL;

            camera = v4l2_hdmi_get_camera(hdmi.mipi_camera_type);
            if (!camera)
                return -EINVAL;

            fmt = arg;

            if (camera->maxwidth && (camera->maxwidth < fmt->fmt.pix.width)) {
                HDMIRX_ERROR("error requesting width %d > device maximum %d", fmt->fmt.pix.width, camera->maxwidth);
                return -EINVAL;
            }
            if (camera->maxheight && ((camera->maxheight+camera->auxiliary_inband_lines) < fmt->fmt.pix.height)) {
                HDMIRX_ERROR("error requesting height %d > device maximum %d", fmt->fmt.pix.height, camera->maxheight);
                return -EINVAL;
            }
            fccfound = 0;
            for (i=0; i<camera->numfmts; ++i) {
                if (camera->fmts[i]->fcc == fmt->fmt.pix.pixelformat) {
                    fccfound = 1;
                }
            }
            if (!fccfound) {
                HDMIRX_ERROR("error requesting unsupported pixel format %08x", fmt->fmt.pix.pixelformat);
                return -EINVAL;
            }

            HDMIRX_INFO("[HDMI RX] VIDIOC_S_FMT, %dx%d, color=0x%x",
                    fmt->fmt.pix.width, fmt->fmt.pix.height, dev->outfmt);

            // init the device structure members
            dev->width = fmt->fmt.pix.width;
            dev->height = fmt->fmt.pix.height;
            dev->outfmt = V4L2_format_parse(fmt->fmt.pix.pixelformat);
            dev->bpp = out_color_to_bpp(dev->outfmt);

            // init the MIPI structure members
            mipi_top.mipi_init = 0;
            mipi_csi.h_output_len = fmt->fmt.pix.width;
            mipi_csi.v_output_len = fmt->fmt.pix.height;
            mipi_csi.output_color = dev->outfmt;
            mipi_csi.pitch = rx_pitch_measurement(mipi_csi.h_output_len, mipi_csi.output_color);

            break;
		}
		case VIDIOC_G_PARM:
		{
			struct v4l2_streamparm *sp;
			unsigned int fps;

			sp = arg;

			fps = OV9782_get_frame_rate();

			if (sp->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
				sp->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
				sp->parm.capture.capturemode = 0;
				sp->parm.capture.timeperframe.numerator = 1;
				sp->parm.capture.timeperframe.denominator = fps;
				sp->parm.capture.extendedmode = 0;
				sp->parm.capture.readbuffers = 0;
			} else {
				sp->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
				sp->parm.output.outputmode = 0;
				sp->parm.output.timeperframe.numerator = 1;
				sp->parm.output.timeperframe.denominator = fps;
			}

			break;
		}
        case VIDIOC_S_PARM:
		{
			struct v4l2_streamparm *sp;
			unsigned int fps;
			unsigned int numerator;
			unsigned int denominator;

			sp = arg;

			if (sp->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
				numerator = sp->parm.capture.timeperframe.numerator;
				denominator = sp->parm.capture.timeperframe.denominator;
			} else {
				numerator = sp->parm.output.timeperframe.numerator;
				denominator = sp->parm.output.timeperframe.denominator;
			}

			fps = denominator/numerator;

			OV9782_set_frame_rate(fps, hdmi_stream_on);
			break;
		}
        /* Buffers & streaming */
        case VIDIOC_REQBUFS:
            {
                //HDMIRX_INFO(" ioctl VIDIOC_REQBUFS");
                struct vb2_queue *vq ;//= &dev->vb_hdmidq;
                struct v4l2_requestbuffers *rb = (struct v4l2_requestbuffers *)arg;

                //HDMIRX_INFO(" ioctl VIDIOC_REQBUFS count=%d,type=%d,memory=%d",rb->count,rb->type,rb->memory);

                vq = &dev->vb_hdmidq;
                ret = hdmirx_queue_init(dev, vq, rb->memory);
                if(ret){
                    pr_err("hdmirx_queue_init failed: %ld\n", ret);
                    return ret;
                }
                ret = vb2_reqbufs(vq, rb);
                if(ret){
                    pr_err("vb2_reqbufs failed: %ld\n", ret);
                    return ret;
                }
                return ret;
            }
        case VIDIOC_QUERYBUF:
            {
                struct v4l2_buffer *buf = arg;

                //HDMIRX_INFO(" ioctl VIDIOC_QUERYBUF index(%u) type(%u) length(%u)",buf->index,buf->type,buf->length);

                ret = vb2_querybuf(&dev->vb_hdmidq, buf);

				if(ret<0)
					HDMIRX_ERROR(" ioctl VIDIOC_QUERYBUF fail");
                return ret;
            }
         case VIDIOC_QBUF:
            {
                struct v4l2_buffer *buf = arg;

                //HDMIRX_INFO(" ioctl VIDIOC_QBUF");

                ret = vb2_qbuf(&dev->vb_hdmidq, buf);

				if(ret<0)
					HDMIRX_ERROR(" ioctl VIDIOC_QBUF fail");
                return ret;
            }
        case VIDIOC_DQBUF:
            {
                struct v4l2_buffer *buf = arg;

                //HDMIRX_INFO(" ioctl VIDIOC_DQBUF");

                ret = vb2_dqbuf(&dev->vb_hdmidq, buf, file->f_flags & O_NONBLOCK);
                if(ret == 0)
                    atomic_dec(&dev->hdmidq.rcnt);

				if (mipi_top.src_sel==1)
				{
                	if (Hdmi_IsbReady()==TRUE)
                	{
                    	UINT32 b;
                    	b =  Hdmi_get_b_value();
                    	if ((HDMI_ABS(b, hdmi_ioctl_struct.b_pre) > 50))
                    	{
                        	ret = -ENODEV;
                    	}
                	}
				}
                //avoid userspace seeing broken frames on hotplug or switching tv system
                if(unlikely(ret == 0 && hdmi_stream_on == 0))
                    ret = -EINVAL;

                return ret;
            }
        case VIDIOC_STREAMON:
            {
                HDMIRX_INFO(" ioctl VIDIOC_STREAMON");

                ret = vb2_streamon(&dev->vb_hdmidq, dev->vb_hdmidq.type);
                if (ret < 0)
                    HDMIRX_ERROR("VIDIOC_STREAMON fail");
                return ret;
            }
        case VIDIOC_STREAMOFF:
            {
                HDMIRX_INFO(" ioctl VIDIOC_STREAMOFF");

                vb2_streamoff(&dev->vb_hdmidq, dev->vb_hdmidq.type);

                return 0;
            }
        case VIDIOC_QUERY_MIPI_IN_TYPE:
            {
                struct v4l2_mipi_in_type *in_type = arg;

                pr_info("[HDMI RX] ioctl VIDIOC_MIPI_IN_TYPE\n");
                memset(in_type, 0, sizeof *in_type);
                in_type->input_type = mipi_top.src_sel;
                break;
            }
        case VIDIOC_SET_MIPI_IN_TYPE:
            {
                struct v4l2_mipi_in_type *in_type = arg;

                //pr_info("[HDMI RX] ioctl VIDIOC_SET_MIPI_IN_TYPE\n");
                break;
            }
        case VIDIOC_SET_EDID_PHYSICAL_ADDR:
            {
                //For CTS 9-5 physical address, call from TX service
                pr_info("[HDMI RX] ioctl VIDIOC_SET_EDID_PHYSICAL_ADDR\n");
                int *phy_addr = arg;
                unsigned char byteMSB,byteLSB;
                byteMSB = ((*phy_addr)>>8)&0xFF;
                byteLSB = (*phy_addr)&0xFF;

                if(mipi_top.hdmi_rx_init)
                    drvif_Hdmi_HPD(0, 0);

                HdmiRx_change_edid_physical_addr(byteMSB, byteLSB);

                if(mipi_top.hdmi_rx_init)
                {
                    HDMI_DELAYMS(1000);
                    drvif_Hdmi_HPD(0, 1);
                }
                break;
            }
		case VIDIOC_ENABLE_RX_HDCP:
			{
				pr_info("[HDMI RX] ioctl VIDIOC_ENABLE_RX_HDCP\n");
				struct rx_hdcp_key *rx_key = arg;
				HdmiRx_enable_hdcp(rx_key->ksv, rx_key->private_key);
				break;
			}
		case VIDIOC_SET_GROUP_SWITCH:
		{
			int ret_val;
			struct group_switch_config *config = arg;

			ret_val = OV9782_set_group(config, hdmi_stream_on);
			if (ret_val < 0) {
				HDMIRX_ERROR("VIDIOC_SET_GROUP_SWITCH fail");
				ret = -EIO;
			}
			break;
		}
		case VIDIOC_GET_GROUP_SWITCH:
		{
			struct group_switch_config *config = arg;

			OV9782_get_group(config);
			break;
		}
		case VIDIOC_SET_AWB:
		{
			int ret_val;
			struct awb_gain_config *gain = arg;

			ret_val = OV9782_set_awb(gain, hdmi_stream_on);
			if (ret_val < 0) {
				HDMIRX_ERROR("VIDIOC_SET_AWB fail");
				ret = -EIO;
			}
			break;
		}
		case VIDIOC_GET_AWB:
		{
			struct awb_gain_config *gain = arg;

			OV9782_get_awb(gain);
			break;
		}
        default:
            {
                pr_err("Unknown ioctl 0x%08x\n", cmd);
                return -ENOTTY;
            }
   }
    return ret;
}

long v4l2_hdmi_ioctl(struct file *file,
             unsigned int cmd, unsigned long arg)
{
    if(0)
    {
        pr_info("v4l2_hdmi_ioctl(");
        v4l_printk_ioctl(NULL, cmd);
        pr_info(")\n");
    }
    return video_usercopy(file, cmd, arg, v4l2_hdmi_do_ioctl);
}

int hdmi_queue_setup(struct vb2_queue *vq, const struct v4l2_format *fmt,
                unsigned int *nbuffers, unsigned int *nplanes,
                unsigned int sizes[], void *alloc_ctxs[])
{
    struct v4l2_hdmi_dev *dev = vb2_get_drv_priv(vq);
    struct hdmi_dmaqueue *dma_q = &dev->hdmidq;
    unsigned long size;

	HDMIRX_INFO("[%s] outfmt(%u) width(%u) height(%u)",__FUNCTION__, dev->outfmt, dev->width, dev->height + dev->auxiliary_inband_lines);

    atomic_set(&dma_q->qcnt, 0);
    atomic_set(&dma_q->rcnt, 0);

    if(dev->outfmt >= OUT_ARGB)
        size = dev->width * dev->height * dev->bpp / 8;
    else
        size = roundup16(dev->width) * roundup16(dev->height+dev->auxiliary_inband_lines) * dev->bpp / 8;

    if (size == 0)
        return -EINVAL;

    if (0 == *nbuffers)
        *nbuffers = 32;

    *nplanes = 1;

    sizes[0] = size;

    /*
     * videobuf2-vmalloc allocator is context-less so no need to set
     * alloc_ctxs array.
     */
    if(vq->memory == V4L2_MEMORY_MMAP){
        void *ret = vb2_dma_contig_init_ctx(&dev->vdev.dev);
        if (IS_ERR(ret))
            return PTR_ERR(ret);
        alloc_ctxs[0] = ret;
    }

    HDMIRX_INFO("[%s] count=%d, size=%ld",__func__, *nbuffers, size);
    return 0;
}

int hdmi_buffer_prepare(struct vb2_buffer *vb)
{
    struct vb2_queue *vq = vb->vb2_queue;
    struct v4l2_hdmi_dev *dev = vb2_get_drv_priv(vq);
    struct hdmi_buffer *buf = container_of(vb, struct hdmi_buffer, vb);
    unsigned long size;

    if (dev->width  < 48 || dev->width  > MAX_WIDTH ||
        dev->height < 32 || dev->height > MAX_HEIGHT)
        return -EINVAL;

    if(dev->outfmt >= OUT_ARGB)
        size = dev->width * dev->height * dev->bpp / 8;
    else
        size = roundup16(dev->width) * roundup16(dev->height+dev->auxiliary_inband_lines) * dev->bpp / 8;

    if (vb2_plane_size(vb, 0) < size) {
        HDMIRX_ERROR("[%s] data will not fit into plane (%lu < %lu)",__FUNCTION__,vb2_plane_size(vb, 0), size);
        return -EINVAL;
    }

    vb2_set_plane_payload(&buf->vb, 0, size);

    if(buf->phys == 0){
        if(vq->memory == V4L2_MEMORY_USERPTR){
            struct mipi_vb2_vmalloc_buf *vb2_buf =  vb->planes[0].mem_priv;
            unsigned long pfn;

            follow_pfn(vb2_buf->vma, vb->v4l2_planes[0].m.userptr, &pfn);
            buf->phys = __pfn_to_phys(pfn);
        }else if(vq->memory == V4L2_MEMORY_MMAP){
            dma_addr_t *addr = vb2_plane_cookie(vb, 0);
            buf->phys = *addr;
        }
    }

    return 0;
}

int hdmi_buffer_finish(struct vb2_buffer *vb)
{
	struct timespec ts;
	struct hdmi_buffer *buf = container_of(vb, struct hdmi_buffer, vb);
	unsigned char *frame;

	ktime_get_ts(&ts);
	vb->v4l2_buf.timestamp.tv_sec = ts.tv_sec;
	vb->v4l2_buf.timestamp.tv_usec = ts.tv_nsec / 1000;

	vb->v4l2_buf.sequence = atomic_read(&hdmi_sw_buf_ctl.read_index);
	atomic_inc(&hdmi_sw_buf_ctl.read_index);

	/* POCONO-218, EmbeddedMetadata */
	frame = vb2_plane_vaddr(vb, 0);

	if (frame != NULL) {
		/* fine_exposure(9) | coarse_exposure(6) */
		vb->v4l2_buf.reserved2 = (frame[19]<<16) | (frame[13]<<8) | frame[15];
		/* analog_gain(3) */
		vb->v4l2_buf.reserved = frame[7];
	} else {
		pr_err("[MIPI] Get vb2 vaddr fail\n");
	}

	return 0;
}

void hdmi_buffer_queue(struct vb2_buffer *vb)
{
    struct v4l2_hdmi_dev *dev = vb2_get_drv_priv(vb->vb2_queue);
    struct hdmi_buffer *buf = container_of(vb, struct hdmi_buffer, vb);
    struct hdmi_dmaqueue *hdmidq = &dev->hdmidq;
    unsigned long flags = 0;

    spin_lock_irqsave(&dev->slock, flags);
    list_add_tail(&buf->list, &hdmidq->active);
    atomic_inc(&hdmidq->qcnt);
    spin_unlock_irqrestore(&dev->slock, flags);

    //initiate hw buffer addr then enable MIPI wrapper
    if((hdmi_sw_buf_ctl.use_v4l2_buffer == 0) && (atomic_read(&hdmidq->qcnt) >= 2))
    {
        //unsigned int current_offset = 0;
        static int ever_in = 0;
        hdmi_sw_buf_ctl.use_v4l2_buffer = 1;
        set_video_DDR_start_addr(dev);
        if(ever_in == 0)
        {
            atomic_add(2, &hdmi_sw_buf_ctl.write_index);
        }
        ever_in = 1;
    }
}

int hdmi_start_generating(struct v4l2_hdmi_dev *dev)
{
	int ret_val;

	HDMIRX_INFO("[%s]",__FUNCTION__);

	ret_val = 0;
	hdmi_sw_buf_ctl.pre_frame_done = -1;

	//Enable mipi
	if (mipi_top.src_sel == 1)
		setup_mipi();
	else
		ret_val = mipi_csi_process();

	hdmi_stream_on = 1;
	HDMIRX_INFO("[%s] Start",__FUNCTION__);

	if (ret_val < 0)
		return -EIO;
	else
		return 0;
}

int hdmi_start_streaming(struct vb2_queue *vq, unsigned int count)
{
    struct v4l2_hdmi_dev *dev = vb2_get_drv_priv(vq);
    HDMIRX_INFO("[%s]",__FUNCTION__);
    return hdmi_start_generating(dev);
}

void hdmi_stop_generating(struct v4l2_hdmi_dev *dev)
{
    struct hdmi_dmaqueue *dma_q = &dev->hdmidq;

    HDMIRX_INFO("[%s]",__FUNCTION__);

    /* shutdown control thread */
    if (dma_q->kthread_v4l2) {
        kthread_stop(dma_q->kthread_v4l2);
        dma_q->kthread_v4l2 = NULL;
    }

    /*
     * Typical driver might need to wait here until dma engine stops.
     * In this case we can abort imiedetly, so it's just a noop.
     */

    /* Release all active buffers */
    while (!list_empty(&dma_q->active)) {
        struct hdmi_buffer *buf;
        buf = list_entry(dma_q->active.next, struct hdmi_buffer, list);
        list_del(&buf->list);
        vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
        HDMIRX_INFO("Release[%p/%d] done", buf, buf->vb.v4l2_buf.index);
    }
    if(dma_q->hwbuf[0] != NULL){
        vb2_buffer_done(&dma_q->hwbuf[0]->vb, VB2_BUF_STATE_ERROR);
        dma_q->hwbuf[0] = NULL;
        HDMIRX_INFO("Release hwbuf[0] done");
    }
    if(dma_q->hwbuf[1] != NULL){
        vb2_buffer_done(&dma_q->hwbuf[1]->vb, VB2_BUF_STATE_ERROR);
        dma_q->hwbuf[1] = NULL;
        HDMIRX_INFO("Release hwbuf[1] done");
    }
}

int hdmi_stop_streaming(struct vb2_queue *vq)
{
    struct v4l2_hdmi_dev *dev = vb2_get_drv_priv(vq);
    HDMIRX_INFO("[%s]",__FUNCTION__);
    stop_mipi_process();
    hdmi_stop_generating(dev);
    return 0;
}

void hdmi_unlock(struct vb2_queue *vq)
{
    struct v4l2_hdmi_dev *dev = vb2_get_drv_priv(vq);
    mutex_unlock(&dev->mutex);
}

void hdmi_lock(struct vb2_queue *vq)
{
    struct v4l2_hdmi_dev *dev = vb2_get_drv_priv(vq);
    mutex_lock(&dev->mutex);
}

void hdmi_rx_process(void)
{
    int ret;
    static int state = 0;

    //pr_info("%s:%d\n", __func__, __LINE__);
    if(mipi_top.hdmi_rx_init == 0)
    {
        set_hdmirx_wrapper_control_0(-1,-1,-1,-1,-1,1);
        // hdmi rx init
        HdmiTable_Init();

        drvif_Hdmi_Init();

        drvif_Hdmi_InitSrc(0);

        mipi_top.hdmi_rx_init = 1;

        state = 0;
    }

#ifdef CRC_CHECK
    HDMI_FormatDetect();
    if(hdmi_ioctl_struct.b)
    {
        int result;

        result = QC_CRCCheck(0); // type=0: HDMI; type=1: MHL

        if(result)
            HDMIRX_INFO("CRC PASS");
        else
            HDMIRX_INFO("CRC FAIL!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    }
#endif
    switch(state)
    {
        case 0:
        	if(hdmi_ioctl_struct.DEF_ready)
        	{
				if (drvif_Hdmi_DetectMode() == PHOENIX_MODE_SUCCESS)
					state++;
            }
            break;
        case 1:
            ret = drvif_Hdmi_CheckMode();
            if(ret > 0 && hdmi_ioctl_struct.detect_done)
            {
				HDMIRX_INFO("Check mode is false, restart HDMI Rx wrapper detection ret(%d)\n",ret);
				state = 0;
				restartHdmiRxWrapperDetection();
            }
			else if(hdmi_ioctl_struct.detect_done == 0)
			{
				HDMIRX_INFO("Check mode is fine, but detect_done=0 ret(%d)\n",ret);
				state =0;
			}
            break;
        default:
            state = 0;
            break;
    }

}


int mipi_csi_process(void)
{
    int ret_val;
    MIPI_REG mipi_reg;

    ret_val =0;
    if (mipi_top.mipi_init == 1)
        goto exit;

    // mipi csi init
    HDMIRX_INFO("mipi_csi_process starting, in:%dx%d aux:%d csp:%d out:%dx%d\n", mipi_csi.h_input_len, mipi_csi.v_input_len, mipi_csi.auxiliary_inband_lines, mipi_csi.output_color, mipi_csi.h_output_len, mipi_csi.v_output_len);

	switch (hdmi.mipi_camera_type) {
	case MIPI_CAMERA_RLE0551C:
		RLE0551C_2048x1536();
		break;
	case MIPI_CAMERA_RTS5845:
		RTS5845(mipi_csi.h_input_len, mipi_csi.v_input_len);
		break;
	case MIPI_CAMERA_OV5645:
		OV5645_2lane_1280x720_raw8();
		break;
	case MIPI_CAMERA_OV5647:
		OV5647_2lane_1280x720_raw8();
		break;
	case MIPI_CAMERA_OV9782:
		ret_val = OV9782_2lane_1280x800(mipi_csi.output_color);
		break;
	default:
		HDMIRX_ERROR("Unknown MIPI type");
	}

    set_mipi_env();

    //pr_info("[%d,%d,%d,%d,%d]\n",mipi_csi.h_output_len,mipi_csi.v_output_len,mipi_csi.output_color,mipi_csi.h_input_len,mipi_csi.v_input_len);
    HDMIRX_INFO("pitch=%d, len=%d, color=%d\n",mipi_csi.pitch,mipi_csi.v_output_len+mipi_csi.auxiliary_inband_lines, mipi_csi.output_color);

    set_video_dest_size(mipi_csi.h_output_len, mipi_csi.pitch);    //width,pitch
    set_video_src_size(mipi_csi.h_input_len);
    //scale down
    if(mipi_csi.output_color !=  OUT_RAW8 && mipi_csi.output_color !=  OUT_RAW10 && mipi_csi.auxiliary_inband_lines == 0) {
        phoenix_mipi_scale_down(mipi_csi.h_input_len, mipi_csi.v_input_len, mipi_csi.h_output_len, mipi_csi.v_output_len);
        HDMIRX_INFO("scaler setup");
    }

    hdmi_rx_reg_write32(MIPI_WRAPPER_DPHY_REG11_reg, Esc_lane3_flg(clear)|Esc_lane2_flg(clear)|Esc_lane2_flg(clear)|Esc_lane1_flg(clear)|
           Esc_lane0_flg(clear)|Clk_ulps_flg(clear)|Crc16_err_flg(clear)|Ecc_crt_flg(clear)|Ecc_err_flg(clear), HDMI_RX_MIPI_PHY);

    memset(&mipi_reg, 0, sizeof(MIPI_REG));

    mipi_reg.dst_fmt = mipi_csi.output_color;

	if (mipi_csi.output_color == OUT_RAW10)
		mipi_reg.src_fmt = 0x01; /* RAW10 */
	else if (mipi_csi.output_color == OUT_RAW8)
		mipi_reg.src_fmt = 0x0;  /* RAW8 */
	else
		mipi_reg.src_fmt = 0x02; /* YUV_FMT_YUV422 */

    mipi_reg.src_sel = 0;   //0:MIPI 1:HDMI_RX
    mipi_reg.seq_en = 1;    //0:blk 1:seq (for YUV)
    if(mipi_csi.output_color !=  OUT_RAW8 && mipi_csi.output_color !=  OUT_RAW10 && mipi_csi.auxiliary_inband_lines == 0 && mipi_csi.v_input_len > mipi_csi.v_output_len)
        mipi_reg.vs = 1;
//    mipi_reg.vs_near = 0;
//    mipi_reg.vs_yodd = 0;
//    mipi_reg.vs_codd = 0;
    if(mipi_csi.output_color !=  OUT_RAW8 && mipi_csi.output_color !=  OUT_RAW10 && mipi_csi.auxiliary_inband_lines == 0 && mipi_csi.h_input_len > mipi_csi.h_output_len)
        mipi_reg.hs = 1;
//    mipi_reg.hs_yodd = 0;
//    mipi_reg.hs_codd = 0;

    if(mipi_csi.output_color > OUT_JPEG8)
    {
        mipi_reg.yuv_to_rgb = 1;
        set_alpha(0xff);
        set_YUV2RGB_coeff();
	}

    if((mipi_reg.vs) || (mipi_reg.hs))
    {
        mipi_reg.chroma_us_mode = 0;    //0:repeat 1:avg
        mipi_reg.chroma_us_en = 1;
        if((mipi_reg.dst_fmt == OUT_YUV422)||(mipi_reg.dst_fmt == OUT_YUV420))
        {
            mipi_reg.chroma_ds_mode = 0;    //0:drop 1:avg
            mipi_reg.chroma_ds_en = 1;
        }
    }
    mipi_reg.int_en4 = 1;
    mipi_reg.int_en3 = 1;
    mipi_reg.int_en2 = 1;
    mipi_reg.int_en1 = 1;
    mipi_reg.int_en0 = 1;

    //if(hdmi_stream_on)
        mipi_reg.en = 1;
    //else
        //mipi_reg.en = 0;

    HDMIRX_DEBUG("set_init_mipi_value\n");
    set_init_mipi_value(&mipi_reg);

    mipi_top.mipi_init = 1;
    HDMIRX_INFO("[MIPI INIT 1]\n");
exit:
    return ret_val;
}

static inline void update_hdmirx_switch_state(struct v4l2_hdmi_dev *dev)
{
    if(hdmi_ioctl_struct.measure_ready != switch_get_state(&dev->sdev))
	{
		pr_err("\033[0;31mswitch hdmi rx state to %d\033[m\n", hdmi_ioctl_struct.measure_ready);
		switch_set_state(&dev->sdev, hdmi_ioctl_struct.measure_ready);
	}
}

static int mipi_top_thread(void *arg)
{
	unsigned int i;
    struct v4l2_hdmi_dev *dev = arg;

    memset(&mipi_top, 0, sizeof(MIPI_TOP_INFO));

    mipi_top.src_sel = hdmi.mipi_input_type;
    mipi_top.p_hdmi = &hdmi;
    mipi_top.p_mipi_csi = &mipi_csi;

    pr_devel("Enter mipi_top_thread \n");
    for(;;) {
        if (kthread_should_stop()) break;

        if (mipi_top.src_sel == 1){
			if(rx_5v_state)
			{
				hdmi_rx_process();
				for(i=0;i<3;i++)
				{
					update_hdmirx_switch_state(dev);
					usleep_range(70000, 75000);//70ms~75ms
					rtd_hdmiPhy_ISR();//Execute once every 10 msec(ideal case)
				}
			}
			else
			{
				if(switch_get_state(&dev->sdev))
				{
					pr_err("\033[0;31mswitch hdmi rx state to %d\033[m\n", hdmi_ioctl_struct.detect_done);
					switch_set_state(&dev->sdev, 0);
				}

				HDMI_DELAYMS(250);
			}
        }else{
            //mipi_csi_process();
            //sd_test_while_loop();
			usleep_range(150000, 200000);
        }
    }
    pr_devel("Leave mipi_top_thread\n");

    return 0;
}

int v4l2_mipi_top_open(struct file *filp)
{
    struct video_device *vdev = video_devdata(filp);
    struct v4l2_fh *fh = kzalloc(sizeof(*fh), GFP_KERNEL);
//    int ret;

    filp->private_data = fh;
    if (fh == NULL)
        return -ENOMEM;
    v4l2_fh_init(fh, vdev);
    v4l2_fh_add(fh);

    return 0;
}

irqreturn_t hdmi_irq_handler(int irq, void *dev_id)
{
    HDMI_SW_BUF_CTL * video_info = (HDMI_SW_BUF_CTL *)dev_id;
    irqreturn_t  retVal = IRQ_NONE;
    if(! HDMIRX_HAS_BIT(HDMIRX_VO_SYNC_ADDR, HDMIRX_VO_FEEDBACK_NOTIFY) )
        return IRQ_NONE;
    else
        HDMIRX_RESET_BIT(HDMIRX_VO_SYNC_ADDR,  HDMIRX_VO_FEEDBACK_NOTIFY) ;

    if( video_info->ISR_FLIP <=0 )
        goto HDMIRX_IRQ_END;

    wake_up(&HDMIRX_WAIT_OSD_COMPLETE);

HDMIRX_IRQ_END:

    retVal =  IRQ_HANDLED;

    return retVal;
}

static int __init hdmirx_init(void)
{
    int retval = 0;

    HDMIRX_INFO("driver init");
    retval = platform_driver_register(&HDMIRX_PLATFORM_DRIVER);
    HDMIRX_INFO("driver init done");

    return retval;
}

void __iomem *hdmi_rx_base[HDMI_RX_REG_BLOCK_NUM]; // see define in HDMI_RX_REG_BASE_TYPE


int hdmirx_rtk_drv_remove(struct platform_device *pdev)
{
    struct fb_info *info = platform_get_drvdata(pdev);

    HDMIRX_INFO("driver remove\n");
    if (info) {
        unregister_framebuffer(info);
        framebuffer_release(info);
    }
    return 0;
}

int hdmirx_rtk_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	HDMIRX_INFO(" Enter %s",__FUNCTION__);

	kthread_stop(hdmi_hw_tsk);

	if(mipi_top.hdmi_rx_init)
		drvif_Hdmi_Release();
	set_hdmirx_wrapper_control_0(-1,0,-1,-1,-1,0);//Turn off RX
	set_hdmirx_wrapper_interrupt_en(0,0,0);// Disable wrapper interrupt
	stop_mipi_process();
	mipi_top.hdmi_rx_init = 0;
	hdmi_ioctl_struct.measure_ready = 0;
	hdmi_ioctl_struct.detect_done = 0;
	SET_HDMI_VIDEO_FSM(MAIN_FSM_HDMI_SETUP_VEDIO_PLL);
	SET_HDMI_AUDIO_FSM(AUDIO_FSM_AUDIO_START);

	if(switch_get_state(&hdmi.dev->sdev))
	{
		pr_err("\033[0;31mswitch hdmi rx state to %d\033[m\n", hdmi_ioctl_struct.detect_done);
		switch_set_state(&hdmi.dev->sdev, 0);
	}

	HDMIRX_INFO(" Exit %s",__FUNCTION__);
	return 0;
}

int hdmirx_rtk_drv_resume(struct platform_device *pdev)
{
	HDMIRX_INFO("Enter %s",__FUNCTION__);

	hdmi_hw_tsk = kthread_run(mipi_top_thread, hdmi.dev, "mipi_hw_thread");

	HDMIRX_INFO("Exit %s",__FUNCTION__);
	return 0;
}

void hdmi_platform_shutdown(struct platform_device * p_dev)
{
    (void)p_dev;
}

static ssize_t show_hdmirx_bufcnt(struct device *cd, struct device_attribute *attr, char *buf)
{
	struct video_device *vfd = container_of(cd, struct video_device, dev);
	struct v4l2_hdmi_dev *dev = (struct v4l2_hdmi_dev *)video_get_drvdata(vfd);
	struct hdmi_dmaqueue *hdmidq = &dev->hdmidq;

	return sprintf(buf, "qcnt:%d rcnt:%d\n", atomic_read(&hdmidq->qcnt), atomic_read(&hdmidq->rcnt));
}

static ssize_t show_hdmirx_video_info(struct device *cd, struct device_attribute *attr, char *buf)
{
    char *type[] = { "MIPI", "HDMIRx" };
    char *stat[] = { "NotReady", "Ready" };
    char *cs[] = { "RGB", "YUV422", "YUV444", "YUV411" };
    char *sm[] = { "Interlaced", "Progressive" };

    int status = 0;
    int mode = 0;
    int color = 0;
	int fps = 0;
    int width = 0;
    int height = 0;
	unsigned int vic = 0;

    if(mipi_top.src_sel == 0){ //mipi camera
        status = 1;
        mode = 1;
        color = 1;
        fps = 60;
        width = mipi_csi.h_input_len;
        height = mipi_csi.v_input_len;
    }else if(mipi_top.src_sel == 1 && hdmi_ioctl_struct.detect_done){ //hdmi rx
        status = hdmi_ioctl_struct.detect_done;
        mode = hdmi.tx_timing.progressive;
        color = hdmi.tx_timing.color;
        width = hdmi.tx_timing.h_act_len;
        height = hdmi.tx_timing.v_act_len;
        if(mode == 0) //interlaced mode
            height *= 2;

		vic = drvif_Hdmi_AVI_VIC();
		fps = hdmi_vic_table[vic].fps;
    }

    //pr_info("%s: %d %d %d %dx%d %u %d\n", __func__, status, mode, color, width, height, vic, fps);
    return sprintf(buf, "Type:%s\nStatus:%s\nWidth:%u\nHeight:%u\nScanMode:%s\nColor:%s\nFps:%u\n",
        type[mipi_top.src_sel], stat[status], width, height, sm[mode], cs[color], fps);
}

static ssize_t show_hdmirx_audio_info(struct device *cd, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "Freq:%u\n", hdmi.tx_timing.spdif_freq);
}

static DEVICE_ATTR(hdmirx_bufcnt, S_IRUGO, show_hdmirx_bufcnt, NULL);
static DEVICE_ATTR(hdmirx_video_info, S_IRUGO, show_hdmirx_video_info, NULL);
static DEVICE_ATTR(hdmirx_audio_info, S_IRUGO, show_hdmirx_audio_info, NULL);

int hdmirx_rtk_drv_probe(struct platform_device *pdev)
{
    struct v4l2_hdmi_dev *dev;
    struct video_device *vfd;
    struct switch_dev *sdev;
    struct vb2_queue *q;
    int ret, i;
    u32 irq;
    /*EDID Info*/
    unsigned int ddc_max_num;
    const u32 *edid_table_pro[32]; /*Maximum Support Table is 32bit wide*/
    int edid_cell_size, j;
    HDMIRX_DTS_EDID_TBL_T dts_edid_table[HDMI_MAX_CHANNEL];

    //initialize global variables
    memset(&mipi_top, 0, sizeof(mipi_top));
    memset(&hdmi, 0, sizeof(hdmi));
    memset(&mipi_csi, 0, sizeof(mipi_csi));
    memset(&hdmi_ioctl_struct, 0, sizeof(hdmi_ioctl_struct));

    if (WARN_ON(!(pdev->dev.of_node)))
    {
        HDMIRX_ERROR("[%s] No device node",__FUNCTION__);
        return -ENODEV;
    }


    for (i=0; i < HDMI_RX_REG_BLOCK_NUM; i++)
    {
        hdmi_rx_base[i] = of_iomap(pdev->dev.of_node, i);
        WARN(!(hdmi_rx_base[i]), "unable to map HDMI RX base registers\n");
        HDMIRX_INFO("Base %d = %x",i,(int)hdmi_rx_base[i]);
    }

    {
        of_property_read_u32(pdev->dev.of_node, "mipi_camera_type", &(hdmi.mipi_camera_type));
        of_property_read_u32(pdev->dev.of_node, "mipi_input_type", &(hdmi.mipi_input_type));
    }
    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;
    snprintf(dev->v4l2_dev.name, sizeof(dev->v4l2_dev.name), "v4l2_mipi_top");
    ret = v4l2_device_register(NULL, &dev->v4l2_dev);
    if (ret)
        goto free_dev;
	hdmi.dev = dev;

	//RX power saving
	if(of_property_read_u32(pdev->dev.of_node, "power-saving", &hdmi.power_saving))
		hdmi.power_saving = 0;

    /***********************************************

                  EDID Table Settings

    ************************************************/
    if (of_property_read_u32(pdev->dev.of_node, "Realtek,ddc-max-num", &ddc_max_num))
    {
       HDMIRX_ERROR("Parsing MAx DDC channels failed");
       ddc_max_num = 1;
    }

    //pr_info("[HDMI Rx] Get DDC Max Num: %d, EDID Table Mask: 0x%08X\n", ddc_max_num, edid_table_mask);
    HDMIRX_INFO("Get DDC Max Num: %d", ddc_max_num);

    if(ddc_max_num > HDMI_MAX_CHANNEL)
    {
      ddc_max_num = HDMI_MAX_CHANNEL;
    }

    for(i = 0; i < ddc_max_num; i++)
    {

       edid_table_pro[i] = of_get_property(pdev->dev.of_node, hdmi_rx_edid_tbl_node[i], &edid_cell_size);
       HDMIRX_INFO("Used EDID table: %s, Edid cell size: %d", hdmi_rx_edid_tbl_node[i], edid_cell_size);
       if(edid_table_pro[i])
       {
          edid_cell_size = edid_cell_size / 4;/*Each Read is 4bytes (1world), so divide it by 4*/
          for(j = 0; j < edid_cell_size; j++)
          {
             dts_edid_table[i].edid[j] = (unsigned char) of_read_number(edid_table_pro[i], 1 + j);
          }
       }
       else
       {
          HDMIRX_ERROR("Parsing EDID table content failed");
       }
    }

    HdmiRx_Save_DTS_EDID_Table_Init(dts_edid_table, ddc_max_num);


////////////////////////////////////////////////////////////////////////////////////////////

    irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
    WARN(!(irq), "can not map HDMI RX\n");
    if (request_irq(irq, phoenix_mipi_isr, IRQF_SHARED , "mipi",  dev))// dev  // (void*)(&mipi_drv_data)dev
    {
        HDMIRX_ERROR("Cannot register IRQ %d", irq);
    }
    else
    {
        HDMIRX_INFO("mipi irq(%d)",irq);
    }

////////////////////////////////////////////////////////////////////////////////////////////
    // set default value
    dev->width = 1920;
    dev->height = 1080;
    dev->bpp = 12; //OUT_YUV420

    /* initialize locks */
    spin_lock_init(&dev->slock);

    /* initialize queue */
    q = &dev->vb_hdmidq;
    memset(q, 0, sizeof(*q));
    ret = hdmirx_queue_init(dev, q, V4L2_MEMORY_USERPTR);
    if (ret)
        goto unreg_dev;
    mutex_init(&dev->mutex);

    /* init video dma queues */
    INIT_LIST_HEAD(&dev->hdmidq.active);

    vfd = &dev->vdev;
    *vfd = hdmi_template;
//    vfd->debug = debug;
    vfd->v4l2_dev = &dev->v4l2_dev;
    vfd->queue = q;
    //vfd->v4l2_dev->dev = MKDEV(140, 0);
    vfd->num = V4L2_FL_USE_FH_PRIO;
    set_bit(V4L2_FL_USE_FH_PRIO, &vfd->flags);
    dma_set_coherent_mask(&vfd->dev, DMA_BIT_MASK(32));
    /*
     * Provide a mutex to v4l2 core. It will be used to protect
     * all fops and v4l2 ioctls.
     */
    vfd->lock = &dev->mutex;
    video_set_drvdata(vfd, dev);
    ret = video_register_device(vfd, VFL_TYPE_GRABBER, 250);
    if (ret < 0)
        goto unreg_dev;

#if 1
	ret = device_create_file(&vfd->dev, &dev_attr_hdmirx_bufcnt);
	if(ret < 0)
		goto unreg_dev;
    ret = device_create_file(&vfd->dev, &dev_attr_hdmirx_video_info);
    if(ret < 0)
        goto unreg_dev;
    ret = device_create_file(&vfd->dev, &dev_attr_hdmirx_audio_info);
    if(ret < 0)
        goto unreg_dev;
#endif

	register_mipi_i2c_sysfs(pdev);

#if 1
    sdev = &dev->sdev;
    memset(sdev, 0, sizeof(*sdev));
    sdev->name = "rx";
    ret = switch_dev_register(sdev);
    if(ret < 0)
        goto unreg_vdev;
    switch_set_state(&dev->sdev, hdmi_ioctl_struct.detect_done);
#endif

    /* Now that everything is fine, let's add it to device list */
    list_add_tail(&dev->hdmi_devlist, &hdmi_devlist);

    // for HDMI sw ctrl buffer
    atomic_set(&hdmi_sw_buf_ctl.read_index, 0);
    atomic_set(&hdmi_sw_buf_ctl.write_index, 0);
    atomic_set(&hdmi_sw_buf_ctl.fill_index, 0);
    hdmi_sw_buf_ctl.use_v4l2_buffer= 0;

    {
        struct supported_camera_desc* camera;
        MIPI_OUT_COLOR_SPACE_T csp;

#if CAMERA_480_OUT
        mipi_csi.h_output_len = 720;
        mipi_csi.v_output_len = 480;
        hdmi.tx_timing.output_h = 720;
        hdmi.tx_timing.output_v = 480;
#else
        mipi_csi.h_output_len = 1920;
        mipi_csi.v_output_len = 1080;
        hdmi.tx_timing.output_h = 1920;
        hdmi.tx_timing.output_v = 1080;
#endif

        camera = v4l2_hdmi_get_camera(hdmi.mipi_camera_type);
        if (camera) {
            mipi_csi.h_input_len = camera->maxwidth;
            mipi_csi.v_input_len = camera->maxheight;
            mipi_csi.auxiliary_inband_lines = camera->auxiliary_inband_lines;
            csp = ((camera->numfmts > 0) ? V4L2_format_parse(camera->fmts[0]->fcc) : OUT_YUV420);
            mipi_csi.output_color = csp;
            hdmi.tx_timing.output_color = csp;
            dev->auxiliary_inband_lines = camera->auxiliary_inband_lines;
        } else {
            mipi_csi.h_input_len = MIPI_CAMERA_IN_W;
            mipi_csi.v_input_len = MIPI_CAMERA_IN_H;
            mipi_csi.auxiliary_inband_lines = 0;
            mipi_csi.output_color = OUT_YUV420;
            hdmi.tx_timing.output_color = OUT_YUV420;
            dev->auxiliary_inband_lines = 0;
        }
    }
    // add thread for HDMI RX
    hdmi_hw_tsk = kthread_create(mipi_top_thread, dev, "mipi_hw_thread");
    if (IS_ERR(hdmi_hw_tsk)) {
        ret = PTR_ERR(hdmi_hw_tsk);
        hdmi_hw_tsk = NULL;
        return -ENODEV;
    }
    wake_up_process(hdmi_hw_tsk);

    return 0;

unreg_sdev:
    switch_dev_unregister(&dev->sdev);
unreg_vdev:
    video_unregister_device(&dev->vdev);
unreg_dev:
//    v4l2_ctrl_handler_free(hdl);
    v4l2_device_unregister(&dev->v4l2_dev);

free_dev:
    kfree(dev);
    return ret;
}

static int hdmi_release(void)
{
    struct v4l2_hdmi_dev *dev;
    struct list_head *list;

    while (!list_empty(&hdmi_devlist)) {
        list = hdmi_devlist.next;
        list_del(list);
        dev = list_entry(list, struct v4l2_hdmi_dev, hdmi_devlist);

        switch_dev_unregister(&dev->sdev);

        device_remove_file(&dev->vdev.dev, &dev_attr_hdmirx_video_info);
        device_remove_file(&dev->vdev.dev, &dev_attr_hdmirx_audio_info);

        video_unregister_device(&dev->vdev);
        v4l2_device_unregister(&dev->v4l2_dev);

        kfree(dev);
    }

    return 0;
}

static void __exit hdmi_exit(void)
{
    hdmi_release();

    HDMIRX_INFO("driver exit");
}

//module_init(hdmirx_test_init);
module_init(hdmirx_init);

module_exit(hdmi_exit);

