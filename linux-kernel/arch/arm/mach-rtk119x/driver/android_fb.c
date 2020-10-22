/*
 *  android_fb.c
 * 
 *  Copyright 2007, The Android Open Source Project
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */


/*
 * ANDROID PORTING GUIDE: FRAME BUFFER DRIVER TEMPLATE
 *
 * This template is designed to provide the minimum frame buffer
 * functionality necessary for Android to display properly on a new
 * device.  The PGUIDE_FB macros are meant as pointers indicating
 * where to implement the hardware specific code necessary for the new
 * device.  The existence of the macros is not meant to trivialize the
 * work required, just as an indication of where the work needs to be
 * done.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/dma-contiguous.h>
#include <asm/div64.h>

#include "debug.h"
#include "rtk_fb.h"
#include "dc2vo/dc2vo.h"

#ifdef CONFIG_REALTEK_AVCPU
#include "avcpu.h"
#endif

#define USE_RTK_FB_DEVICE_TREE 1
#define USE_ION_ALLOCATE_FB

#ifdef USE_ION_ALLOCATE_FB
#include "../../../../drivers/staging/android/ion/ion.h"
#include "../../../../drivers/staging/android/uapi/rtk_phoenix_ion.h"
#endif

#define ANDROID_NUMBER_OF_FPS 60

/* Android currently only uses bgra8888 in the hardware framebuffer */
#define ANDROID_BYTES_PER_PIXEL 4

/* Android will use double buffer in video if there is enough */
//#define ANDROID_NUMBER_OF_BUFFERS 3
static int ANDROID_NUMBER_OF_BUFFERS = 3;

/* Modify these macros to suit the hardware */

#define PGUIDE_FB_ROTATE 
	/* Do what is necessary to cause the rotation */

#define PGUIDE_FB_PAN 
	/* Do what is necessary to cause the panning */

#define PGUIDE_FB_PROBE_FIRST 
	/* Do any early hardware initialization */

#define PGUIDE_FB_PROBE_SECOND
	/* Do any later hardware initialization */

#define PGUIDE_FB_WIDTH 320
	/* Return the width of the screen */

#define PGUIDE_FB_HEIGHT 240
	/* Return the heighth of the screen */

#define PGUIDE_FB_SCREEN_BASE 0
	/* Return the virtual address of the start of fb memory */

#define PGUIDE_FB_SMEM_START PGUIDE_FB_SCREEN_BASE
	/* Return the physical address of the start of fb memory */

#define PGUIDE_FB_REMOVE 
	/* Do any hardware shutdown */

static int debug    = 0;
//#define dprintk(msg...) if (debug)   { printk(KERN_DEBUG    "D/RTK_FB: " msg); }
#define dprintk(msg...) if (debug)   { dbg_info(KERN_DEBUG    "D/RTK_FB: " msg); }

struct rtk_fb {
	struct fb_info fb;
	int rotation;
	u32			cmap[16];

    /* RTK: dc2vo */
    VENUSFB_MACH_INFO video_info;
#ifdef USE_ION_ALLOCATE_FB
	struct ion_client *fb_client;
#endif
};

static inline u32 convert_bitfield(int val, struct fb_bitfield *bf)
{
	unsigned int mask = (1 << bf->length) - 1;

	return (val >> (16 - bf->length) & mask) << bf->offset;
}

#ifdef CONFIG_REALTEK_AVCPU
static int fb_avcpu_event_notify(struct notifier_block *self, unsigned long action, void *data)
{
    struct fb_info *info = (struct fb_info *)registered_fb[0];
	struct rtk_fb *fb = container_of(info, struct rtk_fb, fb);
    return DC_avcpu_event_notify(action, &fb->video_info);
}

__maybe_unused static struct notifier_block fb_avcpu_event_notifier = {
    .notifier_call  = fb_avcpu_event_notify,
};
#endif

/* set the software color map.  Probably doesn't need modifying. */
static int
rtk_fb_setcolreg(unsigned int regno, unsigned int red, unsigned int green,
		 unsigned int blue, unsigned int transp, struct fb_info *info)
{
        struct rtk_fb  *fb = container_of(info, struct rtk_fb, fb);

	if (regno < 16) {
		fb->cmap[regno] = convert_bitfield(transp, &fb->fb.var.transp) |
				  convert_bitfield(blue, &fb->fb.var.blue) |
				  convert_bitfield(green, &fb->fb.var.green) |
				  convert_bitfield(red, &fb->fb.var.red);
		return 0;
	}
	else {
		return 1;
	}
}

/* check var to see if supported by this device.  Probably doesn't
 * need modifying.
 */
static int rtk_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	if((var->rotate & 1) != (info->var.rotate & 1)) {
		if((var->xres != info->var.yres) ||
		   (var->yres != info->var.xres) ||
		   (var->xres_virtual != info->var.yres) ||
		   (var->yres_virtual > 
		    info->var.xres * ANDROID_NUMBER_OF_BUFFERS) ||
		   (var->yres_virtual < info->var.xres )) {
			return -EINVAL;
		}
	}
	else {
		if((var->xres != info->var.xres) ||
		   (var->yres != info->var.yres) ||
		   (var->xres_virtual != info->var.xres) ||
		   (var->yres_virtual > 
		    info->var.yres * ANDROID_NUMBER_OF_BUFFERS) ||
		   (var->yres_virtual < info->var.yres )) {
			return -EINVAL;
		}
	}
	if((var->xoffset != info->var.xoffset) ||
	   (var->bits_per_pixel != info->var.bits_per_pixel) ||
	   (var->grayscale != info->var.grayscale)) {
		return -EINVAL;
	}
	return 0;
}


/* Handles screen rotation if device supports it. */
static int rtk_fb_set_par(struct fb_info *info)
{
	struct rtk_fb *fb = container_of(info, struct rtk_fb, fb);
	if(fb->rotation != fb->fb.var.rotate) {
		info->fix.line_length = 
		  info->var.xres * ANDROID_BYTES_PER_PIXEL;
		fb->rotation = fb->fb.var.rotate;
        dbg_info("[%s %d] TODO: FB_ROTATE\n",__func__,__LINE__);
		//PGUIDE_FB_ROTATE;
	}
	return 0;
}

__maybe_unused static int rtkfb_blank(int blank_mode, struct fb_info *info)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
    /* ... */
    return 0;
}

/* Pan the display if device supports it. */
static int rtk_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
    static int rtk_fb_pan_init = 0;
	struct rtk_fb *fb    __attribute__ ((unused)) 
	    = container_of(info, struct rtk_fb, fb);

	/* Set the frame buffer base to something like:
	   fb->fb.fix.smem_start + fb->fb.var.xres * 
	   ANDROID_BYTES_PER_PIXEL * var->yoffset
	*/
    dprintk("[%s %d]\n",__func__,__LINE__);
    fb->fb.var.xoffset         = var->xoffset;
    fb->fb.var.yoffset         = var->yoffset;
    fb->fb.var.xres_virtual    = var->xres_virtual;
    fb->fb.var.yres_virtual    = var->yres_virtual ;
    dprintk(" fbi->fb.var.xoffset      = 0x%08x\n",fb->fb.var.xoffset     );
    dprintk(" fbi->fb.var.yoffset      = 0x%08x\n",fb->fb.var.yoffset     );
    dprintk(" fbi->fb.var.xres_virtual = 0x%08x\n",fb->fb.var.xres_virtual);
    dprintk(" fbi->fb.var.yres_virtual = 0x%08x\n",fb->fb.var.yres_virtual);
    if (rtk_fb_pan_init <= 5)
    {
        rtk_fb_pan_init++;
        msleep(16);
        return 0;
    }

    return DC_Swap_Buffer(info,&fb->video_info);
}

static int rtkfb_mmap(struct fb_info * info, struct vm_area_struct *vma)
{
    struct rtk_fb *fb    __attribute__ ((unused))
        = container_of(info, struct rtk_fb, fb);
    unsigned long start;
    unsigned long mmio_pgoff;
    u32 len;
    dprintk("[%s %d]\n",__func__,__LINE__);

    start = info->fix.smem_start;
    len =   info->fix.smem_len;
    mmio_pgoff = PAGE_ALIGN((start & ~PAGE_MASK) + len) >> PAGE_SHIFT;
    if (vma->vm_pgoff >= mmio_pgoff) {
        if (info->var.accel_flags) {
            return -EINVAL;
        }

        vma->vm_pgoff -= mmio_pgoff;
        start = info->fix.mmio_start;
        len = info->fix.mmio_len;
    }
    //vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    dprintk("start = 0x%08lx , len = 0x%08x\n",start,len);
    return vm_iomap_memory(vma, start, len);

}

static int rtk_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    struct rtk_fb *fb    __attribute__ ((unused))
        = container_of(info, struct rtk_fb, fb);
    return DC_Ioctl(info,(void *)&fb->video_info,cmd,arg);
}

static struct fb_ops rtk_fb_ops = {
	.owner          = THIS_MODULE,
    .fb_blank       = rtkfb_blank,
	.fb_check_var   = rtk_fb_check_var,
	.fb_set_par     = rtk_fb_set_par,
	.fb_setcolreg   = rtk_fb_setcolreg,
	.fb_pan_display = rtk_fb_pan_display,
    .fb_ioctl       = rtk_fb_ioctl,
    .fb_mmap        = rtkfb_mmap,

	/* These are generic software based fb functions */
#ifdef CONFIG_FB_CFB_FILLRECT
	.fb_fillrect    = cfb_fillrect,
#endif
#ifdef CONFIG_FB_CFB_COPYAREA
	.fb_copyarea    = cfb_copyarea,
#endif
#ifdef CONFIG_FB_CFB_IMAGEBLIT
	.fb_imageblit   = cfb_imageblit,
#endif
};


static int rtk_fb_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct rtk_fb *fb;
	size_t framesize;
	uint32_t width, height;
	uint32_t fps;
	struct resource res;
	dma_addr_t		fb_phys;	/* phys. address of the frame buffer */
	uint32_t fb_size;
	const u32 *prop;
	int size;
#ifdef USE_ION_ALLOCATE_FB
	extern struct ion_device *rtk_phoenix_ion_device;
	struct ion_handle *handle;
	ion_phys_addr_t dat;
	size_t len;
	uint32_t allocate_size = 0;
	uint32_t fb_cnt;
#endif

	fb = kzalloc(sizeof(*fb), GFP_KERNEL);
	if(fb == NULL) {
		ret = -ENOMEM;
		goto err_fb_alloc_failed;
	}

#if USE_RTK_FB_DEVICE_TREE
#ifndef USE_ION_ALLOCATE_FB
	ret = of_address_to_resource(pdev->dev.of_node, 0, &res);
	if (ret) {
		dbg_err("Invalid address");
        goto err_fb_alloc_failed;
	}
#endif

	prop = of_get_property(pdev->dev.of_node, "resolution", &size);
	if ((prop) && (size >= sizeof(u32)*2)) {
		int x,y;
		x = of_read_number(prop, 1);
		y = of_read_number(prop, 2);
		width = x;
		height = y;
	} else {
		width = 1280;
		height = 720;
        dbg_warn("[%s %s] Use default w:%d h:%d\n",__FILE__,__func__,width,height);
    }

#ifdef USE_ION_ALLOCATE_FB
	fb->fb_client = NULL;

	prop = of_get_property(pdev->dev.of_node, "buffer-cnt", &size);
	if ((prop) && (size >= sizeof(u32)*1)) {
		fb_cnt = of_read_number(prop, 1);
	} else {
		fb_cnt = 3;
		dbg_warn("[%s %s] Use default fb_cnt:%d\n",__FILE__,__func__,fb_cnt);
	}

#define MEMORY_ALIGN( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))
    allocate_size = MEMORY_ALIGN((width * height * ANDROID_BYTES_PER_PIXEL * fb_cnt), PAGE_SIZE);

	fb->fb_client = ion_client_create(rtk_phoenix_ion_device, "android_fb");
	handle = ion_alloc(fb->fb_client, allocate_size, PAGE_SIZE, RTK_PHOENIX_ION_HEAP_MEDIA_MASK, RTK_ION_FLAG_MASK);
	if (IS_ERR(handle))
	{
		ret = -ENOMEM;
		dbg_err("[%s %s] ion_alloc allocation error size=%d\n", __FILE__, __func__, allocate_size);
		goto err_fb_set_var_failed;
	}

	if(ion_phys(fb->fb_client, handle, &dat, &len) != 0)
	{
		dbg_err("[%s %s] ion_phys error size=%d\n", __FILE__, __func__, allocate_size);
		return -1;
	}

	res.start = dat;
	res.end = dat + allocate_size - 1; //let like as of_address_to_resource(pdev->dev.of_node, 0, &res);
#endif

	prop = of_get_property(pdev->dev.of_node, "fps", &size);
	if ((prop) && (size >= sizeof(u32)*1)) {
		fps = of_read_number(prop, 1);
	} else {
        fps = ANDROID_NUMBER_OF_FPS;
        dbg_warn("[%s %s] Use default fps:%d\n",__FILE__,__func__,fps);
    }

	fb_phys = res.start;
	fb_size = res.end - res.start;
    fb_size += 1;
    dbg_info("[%s %s] [%d x %d] addr:0x%08x size:0x%08x\n",__FILE__,__func__,
            width,height,fb_phys,fb_size);

	dev_set_drvdata(&pdev->dev, fb);

    DC_Init(&fb->video_info, &fb->fb);

#else /* else of USE_RTK_FB_DEVICE_TREE */
	platform_set_drvdata(pdev, fb);

	PGUIDE_FB_PROBE_FIRST;
	width = PGUIDE_FB_WIDTH;
	height = PGUIDE_FB_HEIGHT;
#endif /* end of USE_RTK_FB_DEVICE_TREE */

	fb->fb.fbops		= &rtk_fb_ops;

	/* These modes are the ones currently required by Android */

	fb->fb.flags		= FBINFO_FLAG_DEFAULT;
	fb->fb.pseudo_palette	= fb->cmap;
	fb->fb.fix.type		= FB_TYPE_PACKED_PIXELS;
	fb->fb.fix.visual = FB_VISUAL_TRUECOLOR;
	fb->fb.fix.line_length = width * ANDROID_BYTES_PER_PIXEL;
	fb->fb.fix.accel	= FB_ACCEL_NONE;
	fb->fb.fix.ypanstep = 1;

    ANDROID_NUMBER_OF_BUFFERS = (fb_size)/(fb->fb.fix.line_length * height);

	fb->fb.var.xres		= width;
	fb->fb.var.yres		= height;
	fb->fb.var.xres_virtual	= width;
	fb->fb.var.yres_virtual	= height * ANDROID_NUMBER_OF_BUFFERS;
	fb->fb.var.activate	= FB_ACTIVATE_NOW;
	fb->fb.var.height	= height;
	fb->fb.var.width	= width;

#if 0
	fb->fb.var.bits_per_pixel = 16;
	fb->fb.var.red.offset = 11;
	fb->fb.var.red.length = 5;
	fb->fb.var.green.offset = 5;
	fb->fb.var.green.length = 6;
	fb->fb.var.blue.offset = 0;
	fb->fb.var.blue.length = 5;
#else
	fb->fb.var.bits_per_pixel = 32;
	fb->fb.var.transp.offset = 24;
	fb->fb.var.transp.length = 8;
	fb->fb.var.red.offset = 16;
	fb->fb.var.red.length = 8;
	fb->fb.var.green.offset = 8;
	fb->fb.var.green.length = 8;
	fb->fb.var.blue.offset = 0;
	fb->fb.var.blue.length = 8;
#endif

        switch(width)
        {
            case 1920:
                fb->fb.var.vsync_len = 1;
                fb->fb.var.hsync_len = 2;
                //fb->fb.var.pixclock = 80185;
                fb->fb.var.width = 0;
                fb->fb.var.height = 0;
                break;
            case 1280:
                fb->fb.var.vsync_len = 11;
                fb->fb.var.hsync_len = 3;
                //fb->fb.var.pixclock = 17856;
                fb->fb.var.width = 0;
                fb->fb.var.height = 0;
                break;
            default:
                break;
        }

        {
            long long temp = (fb->fb.var.xres+fb->fb.var.vsync_len) * (fb->fb.var.yres+fb->fb.var.hsync_len);
            long long temp2 = 1000000000000;
            do_div(temp2, temp);
            fb->fb.var.pixclock = temp2;
            fb->fb.var.pixclock /= fps;
        }


#if 1
    framesize = fb_size;
#else
	framesize = width * height * 
	  ANDROID_BYTES_PER_PIXEL * ANDROID_NUMBER_OF_BUFFERS;
#endif
	fb->fb.fix.smem_len = framesize;

#if USE_RTK_FB_DEVICE_TREE
	fb->fb.fix.smem_start = fb_phys;
#ifdef USE_ION_ALLOCATE_FB
	fb->fb.screen_base = ion_map_kernel(fb->fb_client, handle);
#else
	fb->fb.screen_base = ioremap(fb_phys, framesize);
#endif
	if (!fb->fb.screen_base) {
		dbg_err("Could not allocate frame buffer memory");
		goto err_fb_set_var_failed;
	}

    /* Avoid black screen */
    //memset(fb->fb.screen_base,0,framesize);

#else /* else of USE_RTK_FB_DEVICE_TREE */
	fb->fb.fix.smem_start = PGUIDE_FB_SMEM_START;
	fb->fb.screen_base = PGUIDE_FB_SCREEN_BASE;
#endif /* end of USE_RTK_FB_DEVICE_TREE */

	ret = fb_set_var(&fb->fb, &fb->fb.var);
	if(ret)
		goto err_fb_set_var_failed;

	//PGUIDE_FB_PROBE_SECOND;

	ret = register_framebuffer(&fb->fb);
	if(ret)
		goto err_register_framebuffer_failed;

#ifdef CONFIG_REALTEK_AVCPU
    unregister_avcpu_notifier(&fb_avcpu_event_notifier);
#endif

	return 0;

err_register_framebuffer_failed:
err_fb_set_var_failed:
    DC_Deinit(&fb->video_info);
	kfree(fb);
err_fb_alloc_failed:
	return ret;
}

static int rtk_fb_suspend(struct device *dev)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct rtk_fb *fb = platform_get_drvdata(pdev);
    return DC_Suspend(&fb->video_info);
}

static int rtk_fb_resume(struct device *dev)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct rtk_fb *fb = platform_get_drvdata(pdev);
    return DC_Resume(&fb->video_info);
}

static int rtk_fb_remove(struct platform_device *pdev)
{
	struct rtk_fb *fb = platform_get_drvdata(pdev);

#ifdef CONFIG_REALTEK_AVCPU
    register_avcpu_notifier(&fb_avcpu_event_notifier);
#endif

#if USE_RTK_FB_DEVICE_TREE
#ifdef USE_ION_ALLOCATE_FB
	if(fb->fb_client != NULL)
	{
		ion_client_destroy(fb->fb_client);
		fb->fb_client = NULL;
	}
#endif
    DC_Deinit(&fb->video_info);
	unregister_framebuffer(&fb->fb);
	kfree(fb);
	dev_set_drvdata(&pdev->dev, NULL);
#else /* else of USE_RTK_FB_DEVICE_TREE */
    PGUIDE_FB_REMOVE;
	kfree(fb);
#endif /* end of USE_RTK_FB_DEVICE_TREE */
	return 0;
}


static struct of_device_id rtk_fb_ids[] = {
	{ .compatible = "Realtek,rtk-fb" },
	{ /* Sentinel */ },
};

MODULE_DEVICE_TABLE(of, rtk_fb_ids);

static const struct dev_pm_ops rtk_fb_pm_ops = {
    .suspend    = rtk_fb_suspend,
    .resume     = rtk_fb_resume,
};

static struct platform_driver rtkfb_of_driver = {
	.remove		= rtk_fb_remove,
	.driver = {
		.name = "rtk-fb",
		.owner = THIS_MODULE,
		.of_match_table = rtk_fb_ids,
#ifdef CONFIG_PM
        .pm = &rtk_fb_pm_ops,
#endif
	},
};

module_platform_driver_probe(rtkfb_of_driver, rtk_fb_probe);
MODULE_LICENSE("GPL");
