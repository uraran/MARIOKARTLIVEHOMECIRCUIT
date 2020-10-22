#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/dma-contiguous.h>

#include "debug.h"
#include "rtk_fb.h"
#include "dc2vo/dc2vo.h"

#define		SUPPORT_COLOR_MAP
/*
 * Driver data
 */
#define BYTES_PER_PIXEL	4
#define BITS_PER_PIXEL	(BYTES_PER_PIXEL * 8)

#define TRANSP_SHIFT    24
#define RED_SHIFT		16
#define GREEN_SHIFT		8
#define BLUE_SHIFT		0

#define PALETTE_ENTRIES_NO	16

static int debug    = 0;
static int warning  = 1;
static int info     = 1;
#define dprintk(msg...) if (debug)   { printk(KERN_DEBUG    "D/RTK_FB: " msg); }
#define eprintk(msg...) if (1)       { printk(KERN_ERR      "E/RTK_FB: " msg); }
#define wprintk(msg...) if (warning) { printk(KERN_WARNING  "W/RTK_FB: " msg); }
#define iprintk(msg...) if (info)    { printk(KERN_INFO     "I/RTK_FB: " msg); }

struct rtkfb_platform_data {
	u32		screen_height_mm;
	u32		screen_width_mm;
	u32		xres, yres;
};
/*
 * Default RTK119x FB configuration
 */
static struct rtkfb_platform_data rtk_fb_default_pdata = {
	.xres	= 1280,
	.yres	= 720,
};

/*
 * Here we define the default structs fb_fix_screeninfo and fb_var_screeninfo
 */
static struct fb_fix_screeninfo rtkfb_fix = {
	.id			= "Realtek",
	.type		= FB_TYPE_PACKED_PIXELS,
    .type_aux   = 0,
	.visual		= FB_VISUAL_TRUECOLOR,
	.accel		= FB_ACCEL_NONE,
};

static struct fb_var_screeninfo rtk_fb_var = {
	.bits_per_pixel		= BITS_PER_PIXEL,
    .grayscale          = 0,
	.red				= { RED_SHIFT, 8, 0},
	.green				= { GREEN_SHIFT, 8, 0},
	.blue				= { BLUE_SHIFT, 8, 0},
	.transp				= { TRANSP_SHIFT, 8, 0},
	.activate			= FB_ACTIVATE_NOW,
};

struct rtkfb_drvdata {
	struct fb_info	info;		/* FB driver info record */
	phys_addr_t		regs_phys;	/* phys. address of the control reg */
	void __iomem	*regs;		/* virt. address of the control reg */
	void			*fb_virt;	/* virt. address of the frame buffer */
	dma_addr_t		fb_phys;	/* phys. address of the frame buffer */
    u32             fb_size;
	u32				pseudo_palette[PALETTE_ENTRIES_NO];

    VENUSFB_MACH_INFO video_info;
};

#define to_rtkfb_drvdata(_info)		container_of(_info, struct rtkfb_drvdata, info)

/**
 *      rtkfb_pan_display - NOT a required function. Pans the display.
 *      @var: frame buffer variable screen structure
 *      @info: frame buffer structure that represents a single frame buffer
 *
 *	Pan (or wrap, depending on the `vmode' field) the display using the
 *  	`xoffset' and `yoffset' fields of the `var' structure.
 *  	If the values don't fit, return -EINVAL.
 *
 *      Returns negative errno on error, or zero on success.
 */
__maybe_unused static int rtkfb_pan_display(struct fb_var_screeninfo *var,
			     struct fb_info *info)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
    /*
     * If your hardware does not support panning, _do_ _not_ implement this
     * function. Creating a dummy function will just confuse user apps.
     */

    /*
     * Note that even if this function is fully functional, a setting of
     * 0 in both xpanstep and ypanstep means that this function will never
     * get called.
     */

    struct rtkfb_drvdata *drvdata = to_rtkfb_drvdata(info);
    drvdata->info.var.xoffset         = var->xoffset;
    drvdata->info.var.yoffset         = var->yoffset;
    drvdata->info.var.xres_virtual    = var->xres_virtual;
    drvdata->info.var.yres_virtual    = var->yres_virtual ;
    dprintk(" fbi->fb.var.xoffset      = 0x%08x\n",drvdata->info.var.xoffset     );
    dprintk(" fbi->fb.var.yoffset      = 0x%08x\n",drvdata->info.var.yoffset     );
    dprintk(" fbi->fb.var.xres_virtual = 0x%08x\n",drvdata->info.var.xres_virtual);
    dprintk(" fbi->fb.var.yres_virtual = 0x%08x\n",drvdata->info.var.yres_virtual);
    return DC_Swap_Buffer(info,&drvdata->video_info);
}

/**
 *      rtkfb_blank - NOT a required function. Blanks the display.
 *      @blank_mode: the blank mode we want.
 *      @info: frame buffer structure that represents a single frame buffer
 *
 *      Blank the screen if blank_mode != FB_BLANK_UNBLANK, else unblank.
 *      Return 0 if blanking succeeded, != 0 if un-/blanking failed due to
 *      e.g. a video mode which doesn't support it.
 *
 *      Implements VESA suspend and powerdown modes on hardware that supports
 *      disabling hsync/vsync:
 *
 *      FB_BLANK_NORMAL = display is blanked, syncs are on.
 *      FB_BLANK_HSYNC_SUSPEND = hsync off
 *      FB_BLANK_VSYNC_SUSPEND = vsync off
 *      FB_BLANK_POWERDOWN =  hsync and vsync off
 *
 *      If implementing this function, at least support FB_BLANK_UNBLANK.
 *      Return !0 for any modes that are unimplemented.
 *
 */
__maybe_unused static int rtkfb_blank(int blank_mode, struct fb_info *info)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
    /* ... */
    return 0;
}

/* ------------ Accelerated Functions --------------------- */

/*
 * We provide our own functions if we have hardware acceleration
 * or non packed pixel format layouts. If we have no hardware
 * acceleration, we can use a generic unaccelerated function. If using
 * a pack pixel format just use the functions in cfb_*.c. Each file
 * has one of the three different accel functions we support.
 */

/**
 *      rtkfb_fillrect - REQUIRED function. Can use generic routines if
 *		 	 non acclerated hardware and packed pixel based.
 *			 Draws a rectangle on the screen.
 *
 *      @info: frame buffer structure that represents a single frame buffer
 *	@region: The structure representing the rectangular region we
 *		 wish to draw to.
 *
 *	This drawing operation places/removes a retangle on the screen
 *	depending on the rastering operation with the value of color which
 *	is in the current color depth format.
 */
void rtkfb_fillrect(struct fb_info *p, const struct fb_fillrect *region)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
/*	Meaning of struct fb_fillrect
 *
 *	@dx: The x and y corrdinates of the upper left hand corner of the
 *	@dy: area we want to draw to.
 *	@width: How wide the rectangle is we want to draw.
 *	@height: How tall the rectangle is we want to draw.
 *	@color:	The color to fill in the rectangle with.
 *	@rop: The raster operation. We can draw the rectangle with a COPY
 *	      of XOR which provides erasing effect.
 */
}

/**
 *      rtkfb_copyarea - REQUIRED function. Can use generic routines if
 *                       non acclerated hardware and packed pixel based.
 *                       Copies one area of the screen to another area.
 *
 *      @info: frame buffer structure that represents a single frame buffer
 *      @area: Structure providing the data to copy the framebuffer contents
 *	       from one region to another.
 *
 *      This drawing operation copies a rectangular area from one area of the
 *	screen to another area.
 */
void rtkfb_copyarea(struct fb_info *p, const struct fb_copyarea *area)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
/*
 *      @dx: The x and y coordinates of the upper left hand corner of the
 *      @dy: destination area on the screen.
 *      @width: How wide the rectangle is we want to copy.
 *      @height: How tall the rectangle is we want to copy.
 *      @sx: The x and y coordinates of the upper left hand corner of the
 *      @sy: source area on the screen.
 */
}


/**
 *      rtkfb_imageblit - REQUIRED function. Can use generic routines if
 *                        non acclerated hardware and packed pixel based.
 *                        Copies a image from system memory to the screen.
 *
 *      @info: frame buffer structure that represents a single frame buffer
 *      @image: structure defining the image.
 *
 *      This drawing operation draws a image on the screen. It can be a
 *	mono image (needed for font handling) or a color image (needed for
 *	tux).
 */
void rtkfb_imageblit(struct fb_info *p, const struct fb_image *image)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
/*
 *      @dx: The x and y coordinates of the upper left hand corner of the
 *	@dy: destination area to place the image on the screen.
 *      @width: How wide the image is we want to copy.
 *      @height: How tall the image is we want to copy.
 *      @fg_color: For mono bitmap images this is color data for
 *      @bg_color: the foreground and background of the image to
 *		   write directly to the frmaebuffer.
 *	@depth:	How many bits represent a single pixel for this image.
 *	@data: The actual data used to construct the image on the display.
 *	@cmap: The colormap used for color images.
 */

/*
 * The generic function, cfb_imageblit, expects that the bitmap scanlines are
 * padded to the next byte.  Most hardware accelerators may require padding to
 * the next u16 or the next u32.  If that is the case, the driver can specify
 * this by setting info->pixmap.scan_align = 2 or 4.  See a more
 * comprehensive description of the pixmap below.
 */
}

/**
 *	rtkfb_rotate -  NOT a required function. If your hardware
 *			supports rotation the whole screen then
 *			you would provide a hook for this.
 *
 *      @info: frame buffer structure that represents a single frame buffer
 *	@angle: The angle we rotate the screen.
 *
 *      This operation is used to set or alter the properities of the
 *	cursor.
 */
void rtkfb_rotate(struct fb_info *info, int angle)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
/* Will be deprecated */
}

/**
 *	rtkfb_sync - NOT a required function. Normally the accel engine
 *		     for a graphics card take a specific amount of time.
 *		     Often we have to wait for the accelerator to finish
 *		     its operation before we can write to the framebuffer
 *		     so we can have consistent display output.
 *
 *      @info: frame buffer structure that represents a single frame buffer
 *
 *      If the driver has implemented its own hardware-based drawing function,
 *      implementing this function is highly recommended.
 */
int rtkfb_sync(struct fb_info *info)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
	return 0;
}

int rtkfb_set_par(struct fb_info *info)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
	return 0;
}

int rtkfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct rtkfb_drvdata *drvdata = to_rtkfb_drvdata(info);
    return DC_Ioctl(info,(void *)&drvdata->video_info,cmd,arg);
}
static int rtkfb_mmap(struct fb_info *fbi, struct vm_area_struct *vma)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
	struct rtkfb_drvdata *drvdata = to_rtkfb_drvdata(fbi);
	struct fb_info *info = &drvdata->info;
	unsigned long start;
	unsigned long mmio_pgoff;
	u32 len;
	start = info->fix.smem_start;
	len =   info->fix.smem_len;
	mmio_pgoff = PAGE_ALIGN((start & ~PAGE_MASK) + len) >> PAGE_SHIFT;
#if 1
	if (vma->vm_pgoff >= mmio_pgoff) {
		if (info->var.accel_flags) {
			return -EINVAL;
		}

		vma->vm_pgoff -= mmio_pgoff;
		start = info->fix.mmio_start;
		len = info->fix.mmio_len;
	}
#endif
	//vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	//fb_pgprotect(file, vma, start);

    dprintk("start = 0x%08x , len = 0x%08x\n",start,len);
	return vm_iomap_memory(vma, start, len);

}
static int rtkfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
                                     u_int transp, struct fb_info *info)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
    if (regno >= 256)       /* no. of hw registers */
        return 1;
    /*
     * Program hardware... do anything you want with transp
     */

    /* grayscale works only partially under directcolor */
    if (info->var.grayscale) {
        /* grayscale = 0.30*R + 0.59*G + 0.11*B */
        red = green = blue = (red * 77 + green * 151 + blue * 28) >> 8;
    }

    /* Directcolor:
     *   var->{color}.offset contains start of bitfield
     *   var->{color}.length contains length of bitfield
     *   {hardwarespecific} contains width of RAMDAC
     *   cmap[X] is programmed to (X << red.offset) | (X << green.offset) | (X << blue.offset)
     *   RAMDAC[X] is programmed to (red, green, blue)
     *
     * Pseudocolor:
     *   uses offset = 0 && length = RAMDAC register width.
     *   var->{color}.offset is 0
     *   var->{color}.length contains widht of DAC
     *   cmap is not used
     *   RAMDAC[X] is programmed to (red, green, blue)
     *
     * Truecolor:
     *   does not use DAC. Usually 3 are present.
     *   var->{color}.offset contains start of bitfield
     *   var->{color}.length contains length of bitfield
     *   cmap is programmed to (red << red.offset) | (green << green.offset) |
     *                     (blue << blue.offset) | (transp << transp.offset)
     *   RAMDAC does not exist
     */
    #define CNVT_TOHW(val,width) ((((val)<<(width))+0x7FFF-(val))>>16)
    switch (info->fix.visual) {
    case FB_VISUAL_TRUECOLOR:
    case FB_VISUAL_PSEUDOCOLOR:
        red = CNVT_TOHW(red, info->var.red.length);
        green = CNVT_TOHW(green, info->var.green.length);
        blue = CNVT_TOHW(blue, info->var.blue.length);
        transp = CNVT_TOHW(transp, info->var.transp.length);
        break;
    case FB_VISUAL_DIRECTCOLOR:
        red = CNVT_TOHW(red, 8);        /* expect 8 bit DAC */
        green = CNVT_TOHW(green, 8);
        blue = CNVT_TOHW(blue, 8);
        /* hey, there is bug in transp handling... */
        transp = CNVT_TOHW(transp, 8);
        break;
    }
    #undef CNVT_TOHW
    /* Truecolor has hardware independent palette */
    if (info->fix.visual == FB_VISUAL_TRUECOLOR) {
        u32 v;

        if (regno >= 16)
            return 1;

        v = (red << info->var.red.offset) |
            (green << info->var.green.offset) |
            (blue << info->var.blue.offset) |
            (transp << info->var.transp.offset);
        switch (info->var.bits_per_pixel) {
        case 8:
            break;
        case 16:
            ((u32 *) (info->pseudo_palette))[regno] = v;
            break;
        case 24:
        case 32:
            ((u32 *) (info->pseudo_palette))[regno] = v;
            break;
        }
        return 0;
    }
    return 0;

}

static int rtkfb_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
    return 0;
}

static int rtkfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
    return 0;
}

static int rtkfb_open(struct fb_info *info, int user)
{
    dprintk("[%s %d]\n",__func__,__LINE__);
    return 0;
}
    /*
     *  Frame buffer operations
     */

static struct fb_ops rtkfb_ops = {
	.owner		= THIS_MODULE,
	.fb_blank	= rtkfb_blank,
	.fb_pan_display	= rtkfb_pan_display,
	.fb_open	= rtkfb_open,
    .fb_ioctl   = rtkfb_ioctl,
    .fb_mmap	= rtkfb_mmap,
	.fb_read	= fb_sys_read,
	.fb_write	= fb_sys_write,
	.fb_check_var	= rtkfb_check_var,
	.fb_cursor	= rtkfb_cursor,		/* Optional !!! */
	.fb_sync	= rtkfb_sync,
	.fb_set_par	= rtkfb_set_par,
#if 0
	.fb_fillrect	= rtkfb_fillrect, 	/* Needed !!! */
	.fb_copyarea	= rtkfb_copyarea,	/* Needed !!! */
	.fb_imageblit	= rtkfb_imageblit,	/* Needed !!! */
	.fb_setcolreg	= rtkfb_setcolreg,
	.fb_release	= rtkfb_release,
	.fb_rotate	= rtkfb_rotate,
#endif
};

/* ------------------------------------------------------------------------- */

    /*
     *  Initialization
     */
static int rtkfb_assign(struct device *dev, struct rtkfb_drvdata *drvdata,
		struct rtkfb_platform_data *pdata)
{
	struct fb_info *info = &drvdata->info;
	int rc = 0;
#if 1
    int fbsize = drvdata->fb_size;
#else
	int fbsize = pdata->xres * pdata->yres * BYTES_PER_PIXEL * 2;
#endif

    dprintk("FB start:0x%08x size:0x%x\n",drvdata->fb_phys,fbsize);
	/* Allocate the framebuffer memory */
	if (drvdata->fb_phys) {
		drvdata->fb_virt = ioremap(drvdata->fb_phys, fbsize);
	}
	else {
		struct page *page;
		page = dma_alloc_from_contiguous(dev, fbsize>>PAGE_SHIFT, get_order(fbsize));
		drvdata->fb_phys = page_to_phys(page);
	}
	if (!drvdata->fb_virt) {
		dbg_err("Could not allocate frame buffer memory");
		goto err_region;
	}

	/* fill fb_info data struct */
	info->device = dev;
	info->screen_base = (void __iomem*)drvdata->fb_virt;
	info->fbops = &rtkfb_ops;
	info->fix = rtkfb_fix;
	info->fix.line_length = pdata->xres * BYTES_PER_PIXEL;
    info->fix.xpanstep = pdata->xres;
    info->fix.ypanstep = pdata->yres;
    info->fix.ywrapstep = pdata->yres;

	info->pseudo_palette = drvdata->pseudo_palette;
	info->flags = FBINFO_DEFAULT;
#if 0
	info->flags = 0;
	info->monspecs.hfmin  = 30000;
	info->monspecs.hfmax  = 70000;
	info->monspecs.vfmin  = 50;
	info->monspecs.vfmax  = 60;
#endif
	info->var = rtk_fb_var;
	info->var.height = pdata->screen_height_mm;
	info->var.width = pdata->screen_width_mm;
	info->var.xres = pdata->xres;
	info->var.yres = pdata->yres;
	info->var.xres_virtual  = pdata->xres;
	//info->var.yres_virtual  = ((fbsize / info->fix.line_length) / (pdata->yres)) * pdata->yres;
	info->var.yres_virtual  = pdata->yres * 3;
    info->var.xoffset       = 0;
    info->var.yoffset       = 0;
    info->var.nonstd        = 8;

	info->fix.smem_start = drvdata->fb_phys;
	//info->fix.smem_len = info->fix.line_length * info->var.yres_virtual * 2;
	info->fix.smem_len = fbsize;

    info->var.pixclock = 1000*(1000*1000*1000 / (pdata->xres * pdata->yres * 60));

	/* allocate a colour map */
#ifdef SUPPORT_COLOR_MAP
	rc = fb_alloc_cmap(&drvdata->info.cmap, PALETTE_ENTRIES_NO, 0);
	if (rc) {
		dbg_err("Fail to allocate colormap (%d entries)", PALETTE_ENTRIES_NO);
		goto err_cmap;
	}
#endif
	/* register */
	rc = register_framebuffer(&drvdata->info);
	if (rc) {
		dbg_err("Could not register frame buffer");
		goto err_regfb;
	}
	return 0;	/* success */

err_regfb:
#ifdef SUPPORT_COLOR_MAP
	fb_dealloc_cmap(&drvdata->info.cmap);
err_cmap:
#endif

err_region:
	kfree(drvdata);
	dev_set_drvdata(dev, NULL);
	return rc;
}

static int rtkfb_release(struct device *dev)
{
	struct rtkfb_drvdata *drvdata = dev_get_drvdata(dev);

    DC_Deinit(&drvdata->video_info);

	unregister_framebuffer(&drvdata->info);

	kfree(drvdata);
	dev_set_drvdata(dev, NULL);

	return 0;
}

static int rtkfb_probe (struct platform_device *pdev)
{
	struct rtkfb_drvdata		*drvdata;
	struct rtkfb_platform_data	pdata;
	struct resource res;
	const u32 *prop;
	int size, rc;

	dbg_info("%s %s",__FILE__, __func__);

	/* Copy with the default pdata */
	pdata = rtk_fb_default_pdata;

    /* Allocate driver info and par */
	drvdata = kzalloc(sizeof(*drvdata), GFP_KERNEL);
	if (WARN_ON(!drvdata)) {
		dbg_warn("Could not allocate device private record");
		return -ENOMEM;
	}

	rc = of_address_to_resource(pdev->dev.of_node, 0, &res);
	if (rc) {
		dbg_err("Invalid address");
		goto err;
	}
	prop = of_get_property(pdev->dev.of_node, "resolution", &size);
	if ((prop) && (size >= sizeof(u32)*2)) {
		int x,y;
		x = of_read_number(prop, 1);
		y = of_read_number(prop, 2);
		pdata.xres = x;
		pdata.yres = y;
	}

	drvdata->fb_phys = res.start;
	drvdata->fb_size = res.end - res.start;

	dev_set_drvdata(&pdev->dev, drvdata);

    DC_Init(&drvdata->video_info);

	return rtkfb_assign(&pdev->dev, drvdata, &pdata);

err:
	kfree(drvdata);
    return -ENODEV;
}

    /*
     *  Cleanup
     */
static int rtkfb_remove(struct platform_device *pdev)
{
    return rtkfb_release(&pdev->dev);
}

static struct of_device_id rtk_fb_ids[] = {
	{ .compatible = "Realtek,rtk-fb" },
	{ /* Sentinel */ },
};
MODULE_DEVICE_TABLE(of, rtk_fb_ids);

static struct platform_driver rtkfb_of_driver = {
	.remove		= rtkfb_remove,
	.driver = {
		.name = "rtk-fb",
		.owner = THIS_MODULE,
		.of_match_table = rtk_fb_ids,
	},
};

module_platform_driver_probe(rtkfb_of_driver, rtkfb_probe);

MODULE_LICENSE("GPL");
