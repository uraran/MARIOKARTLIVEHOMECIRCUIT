#ifndef V4L2_HDMI_DEV_H
#define V4L2_HDMI_DEV_H

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/font.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <media/videobuf2-vmalloc.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-event.h>
#include <media/v4l2-common.h>
#include <media/videobuf2-memops.h>
#include <linux/platform_device.h>
#include <asm/page.h>
#include <linux/dma-contiguous.h>
#include <asm/memory.h>
#include <linux/device.h>
#include <linux/fb.h>
#include <linux/switch.h>

#include "phoenix_hdmiInternal.h"

#define __RTK_HDMI_RX_DEBUG__	0

#if __RTK_HDMI_RX_DEBUG__
#define HDMIRX_DEBUG(format, ...) pr_err("[HDMI RX DBG]" format "\n", ## __VA_ARGS__)
#else
#define HDMIRX_DEBUG(format, ...)
#endif

#define HDMIRX_ERROR(format, ...) printk(KERN_ERR "[HDMI RX ERR]" format "\n", ## __VA_ARGS__)
#define HDMIRX_INFO(format, ...) printk(KERN_WARNING "[HDMI RX]" format "\n", ## __VA_ARGS__)

#ifndef VB2_DMABUF
#define VB2_DMABUF     (1 << 4)
#endif

//#define V4L2_HDMI_RX_FIX_FPS_NUM  2
#define HDMI_MEASURE_RETRY_TIMES 10

typedef enum{
PIXELSIZE_INDEX_420,
PIXELSIZE_INDEX_422
}VIDEO_FORMAT_PIXELSIZE_INDEX;

#define HDMIRX_FB_IRQ (5)
#define HDMIRX_HAS_BIT(addr, bit)  ((*(volatile uint32_t *)addr) &(bit))
#define HDMIRX_VO_SYNC_ADDR  (0xA0000104)
#define hdmirx_endian_swap_32(a) (((a)>>24) | (((a)>>8) & 0xFF00) | (((a)<<8) & 0xFF0000) | (((a)<<24) & 0xFF000000))
#define HDMIRX_VO_SET_NOTIFY            (hdmirx_endian_swap_32(1L << 0))
#define HDMIRX_VO_FEEDBACK_NOTIFY   (hdmirx_endian_swap_32(1L << 1))
#define HDMIRX_RESET_BIT(addr,bit)  ((*(volatile uint32_t *)addr) &= (~(bit)))
#define HDMIRX_SET_BIT(addr,bit)  ((*(volatile uint32_t *)addr) |= (bit))

#define HDMIRX_FB_IOC_MAGIC        'f'
#define HDMIRX_FB_SET_RING_INFO              _IO     (HDMIRX_FB_IOC_MAGIC,  8)

typedef struct HDMIRX_PARAM_RPC_ADDR
{
    unsigned int ringPhyAddr;
    unsigned int refclockAddr;
    int param_size;	//[2]-th pos

} HDMIRX_PARAM_RPC_ADDR;



struct hdmi_dmaqueue {
    atomic_t               rcnt;    //number of buffers already processed by hw
    atomic_t               qcnt;    //number of buffers waiting for hw process
    struct list_head       active;
    struct hdmi_buffer     *hwbuf[2];
    /* thread for generating video stream*/
    struct task_struct         *kthread_v4l2;
    wait_queue_head_t          wq_v4l2;
};

/* buffer for one video frame */
struct hdmi_buffer {
    /* common v4l buffer stuff -- must be first */
    struct vb2_buffer   vb;
    unsigned long       phys;
    struct list_head    list;
};

struct hdmi_fb_info {
    struct fb_info        fb;
    struct device        *dev;
};

struct v4l2_hdmi_dev {
    struct list_head           hdmi_devlist;
    struct v4l2_device     v4l2_dev;
    struct v4l2_ctrl_handler   ctrl_handler;
    struct video_device    vdev;
    struct switch_dev sdev;

    spinlock_t                 slock;
    struct mutex           mutex; /* Protects queue */

    struct hdmi_dmaqueue       hdmidq;

    /* video capture */
//    const struct vivi_fmt      *fmt;
    unsigned int           width;
    unsigned int           height;
    unsigned int           outfmt;
    unsigned int		   bpp;
    unsigned int           auxiliary_inband_lines;
    struct vb2_queue       vb_hdmidq;
};

struct v4l2_hdmi_video_queue {
	struct vb2_queue queue;
	struct mutex mutex;			/* Protects queue */

	unsigned int flags;
	unsigned int buf_used;

	spinlock_t irqlock;			/* Protects irqqueue */
	struct list_head irqqueue;
};

long v4l2_hdmi_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
int v4l2_mipi_top_open(struct file *filp);
int hdmi_queue_setup(struct vb2_queue *vq, const struct v4l2_format *fmt,
				unsigned int *nbuffers, unsigned int *nplanes,
				unsigned int sizes[], void *alloc_ctxs[]);
int hdmi_buffer_prepare(struct vb2_buffer *vb);
int hdmi_buffer_finish(struct vb2_buffer *vb);
void hdmi_buffer_queue(struct vb2_buffer *vb);
int hdmi_start_streaming(struct vb2_queue *vq, unsigned int count);
int hdmi_stop_streaming(struct vb2_queue *vq);
void hdmi_unlock(struct vb2_queue *vq);
void hdmi_lock(struct vb2_queue *vq);
int hdmi_start_generating(struct v4l2_hdmi_dev *dev);
int hdmirx_rtk_drv_probe(struct platform_device *pdev);
int hdmirx_rtk_drv_remove(struct platform_device *pdev);
int hdmirx_rtk_drv_suspend(struct platform_device *pdev, pm_message_t state);
int hdmirx_rtk_drv_resume(struct platform_device *);
void hdmi_platform_shutdown(struct platform_device *);

static const struct v4l2_file_operations hdmi_fops = {
	.owner          = THIS_MODULE,
	.open           = v4l2_mipi_top_open,
	.release        = vb2_fop_release,
	.read           = vb2_fop_read,
	.poll           = vb2_fop_poll,
	.unlocked_ioctl = v4l2_hdmi_ioctl, /* V4L2 ioctl handler */
	.mmap           = vb2_fop_mmap,
};

static const struct vb2_ops hdmi_qops = {
    .queue_setup        = hdmi_queue_setup,
    .buf_prepare        = hdmi_buffer_prepare,
    .buf_finish         = hdmi_buffer_finish,
    .buf_queue          = hdmi_buffer_queue,
    .start_streaming    = hdmi_start_streaming,
    .stop_streaming     = hdmi_stop_streaming,
    .wait_prepare       = hdmi_unlock,
    .wait_finish        = hdmi_lock,
};

struct mipi_vb2_vmalloc_buf {
       void                            *vaddr;
       struct page                     **pages;
       struct vm_area_struct           *vma;
       int                             write;
       unsigned long                   size;
       unsigned int                    n_pages;
       atomic_t                        refcount;
       struct vb2_vmarea_handler       handler;
       struct dma_buf                  *dbuf;
};

static const struct video_device hdmi_template = {
	.name		= "v4l2_mipi_top",
	.fops           = &hdmi_fops,
//	.ioctl_ops 	= &vivi_ioctl_ops,
	.release	= video_device_release_empty,
};

int hdmirx_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
int hdmirx_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);
int hdmirx_fb_mmap(struct fb_info *info, struct vm_area_struct *vma);
int hdmirx_fb_open(struct fb_info *info, int user);
int hdmirx_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
int hdmirx_fb_release(struct fb_info *info, int user);
int hdmirx_fb_set_par(struct fb_info *info);
int hdmirx_fb_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info *info);

#endif // V4L2_HDMI_DEV_H
