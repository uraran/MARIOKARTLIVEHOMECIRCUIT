#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/pagemap.h>
#include <linux/radix-tree.h>
#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/blkpg.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>
#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/sched.h>
#include <linux/dcache.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/file.h>
#include <linux/console.h>
#include <linux/platform_device.h>
#include "../../../../../drivers/staging/android/ion/ion.h"
#include "../../../../../drivers/staging/android/sw_sync.h"
#include <mach/rtk_ipc_shm.h>

#ifdef CONFIG_REALTEK_RPC
#include <linux/RPCDriver.h>
#endif
#ifdef CONFIG_REALTEK_AVCPU
#include "../avcpu.h"
#endif

#define RTKFB_SET_VSYNC_INT _IOW('F', 206, unsigned int)

#include "../rtk_fb.h"
#include "dc2vo.h"

static int debug    = 0;
static int warning  = 1;
static int info     = 1;
#define dprintk(msg...) if (debug)   { printk(KERN_DEBUG    "D/DC: " msg); }
#define eprintk(msg...) if (1)       { printk(KERN_ERR      "E/DC: " msg); }
#define wprintk(msg...) if (warning) { printk(KERN_WARNING  "W/DC: " msg); }
#define iprintk(msg...) if (info)    { printk(KERN_INFO     "I/DC: " msg); }

#ifndef DC_UNREFERENCED_PARAMETER
#define DC_UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

static int DCINIT = 0;
extern struct ion_device *rtk_phoenix_ion_device;

typedef struct {
    struct fb_info          *pfbi;

    REFCLOCK                *REF_CLK;
    RINGBUFFER_HEADER       *RING_HEADER;
    void                    *RING_HEADER_BASE;
    struct ion_client       *gpsIONClient;
    unsigned int            gAlpha;                     /* [0]:Pixel Alpha    [0x01 ~ 0xFF]:Global Alpha */

    unsigned int            *CLK_ADDR_LOW;
    unsigned int            *CLK_ADDR_HI;

    int64_t                 PTS;
    unsigned long           CTX;
    unsigned int            flags;

    wait_queue_head_t       vsync_wait;
    unsigned int            vsync_timeout_ms;
    rwlock_t                vsync_lock;
	ktime_t                 vsync_timestamp;

    void *                  vo_vsync_flag;              /* VSync enable and notify. (VOut => SCPU) */

    unsigned int            uiRes32Width;
    unsigned int            uiRes32Height;

    struct sw_sync_timeline *timeline;
    int                     timeline_max;

    struct dc_pending_post *onscreen;

    struct list_head        post_list;
    struct mutex            post_lock;
    struct task_struct      *post_thread;
    struct kthread_work     post_work;
    struct kthread_worker   post_worker;

    struct list_head        complete_list;
    struct mutex            complete_lock;
    struct task_struct      *complete_thread;
    struct kthread_work     complete_work;
    struct kthread_worker   complete_worker;

    DC_SYSTEM_TIME_INFO     DC_TIME_INFO;
} DC_INFO;

enum {
    RPC_READY               = (1U << 0),
    ISR_INIT                = (1U << 2),
    WAIT_VSYNC              = (1U << 3),
    CHANGE_RES              = (1U << 4),
    BG_SWAP                 = (1U << 5),
    SUSPEND                 = (1U << 6),
    VSYNC_FORCE_LOCK        = (1U << 7),
};

inline long     dc_wait_vsync_timeout       (DC_INFO *pdc_info);
void            dc_update_vsync_timestamp   (DC_INFO *pdc_info);
static int dc_do_simple_post_config(VENUSFB_MACH_INFO * video_info, void *arg);

#ifdef CONFIG_RTK_RPC
DC_INFO *gpdc_info;
DEFINE_SPINLOCK(gASLock);
//EXPORT_SYSMBOL(gASLock);
EXPORT_SYMBOL(gASLock);
#else
static DEFINE_SPINLOCK(gASLock);
#endif

void DC_Reset_OSD_param(struct fb_info *fb, VENUSFB_MACH_INFO * video_info)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    DC_UNREFERENCED_PARAMETER (fb);
    pdc_info->PTS = 0;
    pdc_info->CTX = 0;
    clkResetPresentation(pdc_info->REF_CLK);
}

int DC_Set_RPCAddr(struct fb_info *fb, VENUSFB_MACH_INFO * video_info, DCRT_PARAM_RPC_ADDR* param)
{

    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    DC_UNREFERENCED_PARAMETER (pdc_info);
    DC_UNREFERENCED_PARAMETER (fb);
    DC_UNREFERENCED_PARAMETER (video_info);

    #if 0
    // Set RefClk Table
    pdc_info->REF_CLK = (REFCLOCK*)
        phys_to_virt((phys_addr_t)ulPhyAddrFilter(param->refclockAddr));
        //ioremap((phys_addr_t)ulPhyAddrFilter(param->refclockAddr),sizeof(pdc_info->REF_CLK));

    // Set CMD Queue (Ring Buffer)
    pdc_info->RING_HEADER = (RINGBUFFER_HEADER*)
        phys_to_virt((phys_addr_t)ulPhyAddrFilter(param->ringPhyAddr));
        //ioremap((phys_addr_t)ulPhyAddrFilter(param->ringPhyAddr),sizeof(pdc_info->RING_HEADER));
    #endif

    dprintk("[%s] REFCLK: phy:0x%08x vir:0x%08x\n", __func__, (u32)param->refclockAddr, (u32)pdc_info->REF_CLK);
    dprintk("[%s] RING_HEADER: phy:0x%08x vir:0x%08x\n", __func__, (u32)param->ringPhyAddr, (u32)pdc_info->RING_HEADER);
    DC_Reset_OSD_param(fb,video_info);

    pdc_info->REF_CLK->mastership.videoMode = AVSYNC_FORCED_MASTER;

    {
        DC_PRESENTATION_INFO pos;
        clkGetPresentation(pdc_info->REF_CLK, &pos);
        iprintk("CLK pts:%lld ctx:%ld\n", pos.videoSystemPTS,pos.videoContext);
        iprintk("drvBeAddr:%x s:%lu id:%lu rw:(%x %x) #rPtr:%lu\n",
                (unsigned int)pli_IPCReadULONG((BYTE*)&pdc_info->RING_HEADER->beginAddr),
                pli_IPCReadULONG((BYTE*)&pdc_info->RING_HEADER->size),
                pli_IPCReadULONG((BYTE*)&pdc_info->RING_HEADER->bufferID),
                (unsigned int)pli_IPCReadULONG( (BYTE*)&pdc_info->RING_HEADER->readPtr[0]),
                (unsigned int)pli_IPCReadULONG( (BYTE*)&pdc_info->RING_HEADER->writePtr),
                pli_IPCReadULONG((BYTE*) &pdc_info->RING_HEADER->numOfReadPtr)
              );
    }

    smp_mb();
    pdc_info->flags |= RPC_READY;
    smp_mb();

    return 0;
}

int Get_RefClock(struct fb_info *fb, VENUSFB_MACH_INFO * video_info, REFCLOCK* pclk)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    REFCLOCK* clk = pdc_info->REF_CLK; 
    DC_UNREFERENCED_PARAMETER (pdc_info);
    DC_UNREFERENCED_PARAMETER (fb);
    DC_UNREFERENCED_PARAMETER (video_info);
    if(clk == NULL) return -EAGAIN;
    else {
        unsigned long PliReadMastership = (unsigned long)   pli_IPCReadULONG        ((BYTE*)&clk->mastership      );
        pclk->mastership.systemMode     = (unsigned char)   ((PliReadMastership & 0xff000000) >> 24);
        pclk->mastership.videoMode      = (unsigned char)   ((PliReadMastership & 0x00ff0000) >> 16);
        pclk->mastership.audioMode      = (unsigned char)   ((PliReadMastership & 0x0000ff00) >> 8);
        pclk->mastership.masterState    = (unsigned char)   ((PliReadMastership & 0x000000ff) >> 0);
        pclk->AO_Underflow              = (unsigned long)   pli_IPCReadULONG        ((BYTE*)&clk->AO_Underflow    );
        pclk->GPTSTimeout               = (long long)       pli_IPCReadULONGLONG    ((BYTE*)&clk->GPTSTimeout     );
        pclk->RCD                       = (long long)       pli_IPCReadULONGLONG    ((BYTE*)&clk->RCD             );
        pclk->RCD_ext                   = (unsigned long)   pli_IPCReadULONG        ((BYTE*)&clk->RCD_ext         );
        pclk->VO_Underflow              = (unsigned long)   pli_IPCReadULONG        ((BYTE*)&clk->VO_Underflow    );
        pclk->audioContext              = (unsigned long)   pli_IPCReadULONG        ((BYTE*)&clk->audioContext    );
        pclk->videoContext              = (unsigned long)   pli_IPCReadULONG        ((BYTE*)&clk->videoContext    );
        pclk->masterGPTS                = (long long)       pli_IPCReadULONGLONG    ((BYTE*)&clk->masterGPTS      );
        pclk->audioSystemPTS            = (long long)       pli_IPCReadULONGLONG    ((BYTE*)&clk->audioSystemPTS  );
        pclk->videoSystemPTS            = (long long)       pli_IPCReadULONGLONG    ((BYTE*)&clk->videoSystemPTS  );
        pclk->audioRPTS                 = (long long)       pli_IPCReadULONGLONG    ((BYTE*)&clk->audioRPTS       );
        pclk->videoRPTS                 = (long long)       pli_IPCReadULONGLONG    ((BYTE*)&clk->videoRPTS       );
    }
    return 0;
}

int DC_Get_BufferAddr(struct fb_info *fb, VENUSFB_MACH_INFO * video_info,DCRT_PARAM_BUF_ADDR* param)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    DC_UNREFERENCED_PARAMETER (pdc_info);
    DC_UNREFERENCED_PARAMETER (video_info);
    //TODO: Info from fb framebuffer info
#if 1
    memset(param, 0, sizeof(param));
    param->width  = fb->var.xres;
    param->height = fb->var.yres;
    return 0;
#else
    DC_NOHW_DEVINFO *psDevInfo;
    psDevInfo = GetAnchorPtr();
    if( param->buf_id <= DC_NOHW_MAX_BACKBUFFERS)
    {
        param->buf_Paddr = psDevInfo->asBackBuffers[param->buf_id].sSysAddr.uiAddr | 0xa0000000;
        param->buf_Vaddr = (unsigned int)psDevInfo->asBackBuffers[param->buf_id].sCPUVAddr;
        param->buf_size = psDevInfo->sSysDims.ui32Height * psDevInfo->sSysDims.ui32ByteStride;
        param->width = psDevInfo->sSysDims.ui32Width;
        param->height = psDevInfo->sSysDims.ui32Height;
        param->format =  psDevInfo->sSysFormat.pixelformat; //PVRSRV_PIXEL_FORMAT
        DCRT_DEBUG("get BUF_ADDR 0x%x 0x%x id:%d %u %d wh(%u %u)\n", param->buf_Paddr,
                param->buf_Vaddr,
                param->buf_id, param->buf_size, param->format, param->width, param->height);
    }
    else
    {
        memset(param, 0, sizeof(param));
    }
    return 0;
#endif
}

int DC_Wait_Vsync(struct fb_info *fb, VENUSFB_MACH_INFO * video_info, unsigned long long *nsecs)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    DC_UNREFERENCED_PARAMETER (pdc_info);
    DC_UNREFERENCED_PARAMETER (fb);
    DC_UNREFERENCED_PARAMETER (video_info);

    // WIAT VSYNC EVENT
    dc_wait_vsync_timeout(pdc_info);

    // WRITE NSECS TO USER
    *nsecs = ktime_to_ns(ktime_get());
    return 0;
}

int DC_Set_RateInfo(struct fb_info *fb, VENUSFB_MACH_INFO * video_info,DCRT_PARAM_RATE_INFO* param)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    DC_UNREFERENCED_PARAMETER (pdc_info);
    DC_UNREFERENCED_PARAMETER (fb);
    DC_UNREFERENCED_PARAMETER (video_info);
    pdc_info->CLK_ADDR_LOW = (unsigned int*)ioremap((phys_addr_t)ulPhyAddrFilter(param->clockAddrLow) ,0x4);
    pdc_info->CLK_ADDR_HI =  (unsigned int*)ioremap((phys_addr_t)ulPhyAddrFilter(param->clockAddrHi)   ,0x4);
    //pdc_info->CLK_ADDR_LOW = (unsigned int*)phys_to_virt((phys_addr_t)ulPhyAddrFilter(param->clockAddrLow));
    //pdc_info->CLK_ADDR_HI =  (unsigned int*)phys_to_virt((phys_addr_t)ulPhyAddrFilter(param->clockAddrHi));
    iprintk("DC rInfo: pdc_info->CLK_ADDR_LOW=0x%x pdc_info->CLK_ADDR_HI=0x%x\n",
            (unsigned int)pdc_info->CLK_ADDR_LOW, (unsigned int)pdc_info->CLK_ADDR_HI);
    return 0;
}

int DC_Get_Clock_Map_Info(struct fb_info *fb, VENUSFB_MACH_INFO * video_info,DC_CLOCK_MAP_INFO * param)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    DC_UNREFERENCED_PARAMETER (pdc_info);
    DC_UNREFERENCED_PARAMETER (fb);
    DC_UNREFERENCED_PARAMETER (video_info);
#if 1
    wprintk("[%s] WE ARE NOT SUPPORT!\n",__func__);
    memset(param,0,sizeof(DC_CLOCK_MAP_INFO));
#else
    if(pdc_info->CLK_ADDR_LOW == NULL || pdc_info->CLK_ADDR_HI == NULL) return -EAGAIN;
    else {
        DC_CLOCK_MAP_INFO info;
        info.HiOffset   = ((unsigned int)pdc_info->CLK_ADDR_HI) &  (PAGE_SIZE-1);
        info.LowOffset  = ((unsigned int)pdc_info->CLK_ADDR_LOW) & (PAGE_SIZE-1);
        memcpy(param,&info,sizeof(DC_CLOCK_MAP_INFO));
    }
#endif
    return 0;
}

int DC_Get_Clock_Info(struct fb_info *fb, VENUSFB_MACH_INFO * video_info,REFCLOCK * RefClock)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    DC_UNREFERENCED_PARAMETER (pdc_info);
    DC_UNREFERENCED_PARAMETER (fb);
    DC_UNREFERENCED_PARAMETER (video_info);
    return Get_RefClock(fb,video_info,RefClock);
}

int DC_Reset_RefClock_Table(struct fb_info *fb, VENUSFB_MACH_INFO * video_info,unsigned int option)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    REFCLOCK* clk = pdc_info->REF_CLK; 
    DC_UNREFERENCED_PARAMETER (pdc_info);
    DC_UNREFERENCED_PARAMETER (fb);
    DC_UNREFERENCED_PARAMETER (video_info);
    if( clk != NULL)
    {
        if(option & ResetOption_videoSystemPTS)
            pli_IPCWriteULONGLONG(  (BYTE*)&clk->videoSystemPTS     , -1);
        if(option & ResetOption_audioSystemPTS)
            pli_IPCWriteULONGLONG(  (BYTE*)&clk->audioSystemPTS     , -1);
        if(option & ResetOption_videoRPTS)
            pli_IPCWriteULONGLONG(  (BYTE*)&clk->videoRPTS          , -1);
        if(option & ResetOption_audioRPTS)
            pli_IPCWriteULONGLONG(  (BYTE*)&clk->audioRPTS          , -1);
        if(option & ResetOption_videoContext)
            pli_IPCWriteULONG(      (BYTE*)&clk->videoContext       ,(unsigned long)-1);
        if(option & ResetOption_audioContext)
            pli_IPCWriteULONG(      (BYTE*)&clk->audioContext       ,(unsigned long)-1);
        if(option & ResetOption_videoEndOfSegment)
            pli_IPCWriteULONG(      (BYTE*)&clk->videoEndOfSegment  ,(long)-1);
        //if(option & ResetOption_RCD)
        //    pli_IPCWriteULONGLONG(  (BYTE*)&clk->RCD                ,-1);
        return 0;
    }
    else return -1;
}

int DC_Get_System_Time_Info(struct fb_info *fb, VENUSFB_MACH_INFO * video_info, DC_SYSTEM_TIME_INFO * param)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    DC_UNREFERENCED_PARAMETER (pdc_info);
    DC_UNREFERENCED_PARAMETER (fb);
    DC_UNREFERENCED_PARAMETER (video_info);
    pdc_info->DC_TIME_INFO.PTS              =                   pdc_info->PTS;
    pdc_info->DC_TIME_INFO.CTX              =                   pdc_info->CTX;
    pdc_info->DC_TIME_INFO.RefClockAddr     = (unsigned int)    pdc_info->REF_CLK;
    pdc_info->DC_TIME_INFO.ClockAddr_HI     = (unsigned int)    pdc_info->CLK_ADDR_HI;
    pdc_info->DC_TIME_INFO.ClockAddr_LOW    = (unsigned int)    pdc_info->CLK_ADDR_LOW;
    pdc_info->DC_TIME_INFO.WAIT_ISR         =                   0;//atomic_read(&pdc_info->ISR_FLIP);
    pdc_info->DC_TIME_INFO.RTK90KClock      = 0;
    pdc_info->DC_TIME_INFO.RTK90KClock      = ((unsigned long long)*(unsigned int *) pdc_info->CLK_ADDR_HI)<<32;
    pdc_info->DC_TIME_INFO.RTK90KClock     |= (*(unsigned int *) pdc_info->CLK_ADDR_LOW);

    if(Get_RefClock(fb,video_info,&pdc_info->DC_TIME_INFO.RefClock) != 0) return -EAGAIN;
    memcpy(param,&pdc_info->DC_TIME_INFO,sizeof(DC_SYSTEM_TIME_INFO));
    return 0;
}

int DC_Get_Surface(struct fb_info *fb, VENUSFB_MACH_INFO * video_info, DCRT_PARAM_SURFACE* param)
{
#if 1
    DC_UNREFERENCED_PARAMETER (fb);
    DC_UNREFERENCED_PARAMETER (video_info);
    DC_UNREFERENCED_PARAMETER (param);
    wprintk("[%s] WE ARE NOT SUPPORT!\n",__func__);
    return 0;
#else
    int err=0;
    int idx = 0;
    DC_NOHW_DEVINFO *psDevInfo = (DC_NOHW_DEVINFO *)gpvAnchor;
    DCRT_VSYNC_FLIP_ITEM *psFlipItem= NULL;
    unsigned long ulLockFlags;

    if( psDevInfo == NULL || psDevInfo->psSwapChain == NULL) {
        err = -ENOENT;
        goto DC_Get_Surface_End;
    }
    else if( psDevInfo->ui32BufferSize < param->buf_size) {
        err = -EINVAL;
        goto DC_Get_Surface_End;
    }
    spin_lock_irqsave(&psDevInfo->psSwapChainLock, ulLockFlags);
    if( psDevInfo->ulInsertIndex != psDevInfo->ulRemoveIndex)
    {
        idx = psDevInfo->ulInsertIndex;
    }
    else { //==, use last one
        idx = psDevInfo->ulInsertIndex;
    }
    psFlipItem = &psDevInfo->psVSyncFlips[idx];
    {
        unsigned int srcAddr  = (unsigned int )phys_to_virt(psFlipItem->phyAddr);
        unsigned int dstAddr = (unsigned int )phys_to_virt(param->buf_Paddr);
        if( md_memcpy( (void*)dstAddr, (void*)srcAddr,  param->buf_size, true) != 0 )
        {  //md copy err
            err = -EAGAIN;
        }
    }
    spin_unlock_irqrestore(&psDevInfo->psSwapChainLock, ulLockFlags);
DC_Get_Surface_End:
    if( err) {
        //****   ****//
    }
    return err;
#endif
}

int DC_Set_ION_Share_Memory(struct fb_info *fb, VENUSFB_MACH_INFO * video_info, DC_ION_SHARE_MEMORY * param)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    DC_UNREFERENCED_PARAMETER (fb);
    DC_UNREFERENCED_PARAMETER (video_info);
    if (pdc_info->gpsIONClient == NULL)
        pdc_info->gpsIONClient = ion_client_create(rtk_phoenix_ion_device,"dc2vo");

    if (param->sfd_refclk != 0) {
        struct ion_handle *refclk_handle;
        refclk_handle = ion_import_dma_buf(pdc_info->gpsIONClient,param->sfd_refclk);
        pdc_info->REF_CLK = ion_map_kernel(pdc_info->gpsIONClient,refclk_handle);
        dprintk("[%s] refclk sfd:%d handle:%d vAddr:0x%08x\n",__func__,
                param->sfd_refclk, (int) refclk_handle, (u32) pdc_info->REF_CLK);
    }
    if (param->sfd_rbHeader!= 0) {
        struct ion_handle *rbHeader_handle;
        rbHeader_handle = ion_import_dma_buf(pdc_info->gpsIONClient,param->sfd_rbHeader);
        pdc_info->RING_HEADER =  ion_map_kernel(pdc_info->gpsIONClient,rbHeader_handle);
        dprintk("[%s] rbHeader sfd:%d handle:%d vAddr:0x%08x\n",__func__,
                param->sfd_rbHeader, (int) rbHeader_handle, (u32) pdc_info->RING_HEADER);
    }
    if (param->sfd_rbBase != 0) {
        struct ion_handle *rbBase_handle;
        rbBase_handle = ion_import_dma_buf(pdc_info->gpsIONClient,param->sfd_rbBase);
        pdc_info->RING_HEADER_BASE = ion_map_kernel(pdc_info->gpsIONClient,rbBase_handle);
        dprintk("[%s] rbHeaderBase sfd:%d handle:%d vAddr:0x%08x\n",__func__,
                param->sfd_rbBase, (int) rbBase_handle, (u32) pdc_info->RING_HEADER_BASE);
    }
    return 0;
}

int DC_Set_Buffer_Info(struct fb_info *fb, VENUSFB_MACH_INFO * video_info, DC_BUFFER_INFO * param)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    DC_UNREFERENCED_PARAMETER (fb);
    DC_UNREFERENCED_PARAMETER (video_info);
    if (pdc_info == NULL || param == NULL)
        goto err;

    if (param->enable)
        pdc_info->flags |= CHANGE_RES;
    else
        pdc_info->flags &= ~CHANGE_RES;

    pdc_info->uiRes32Width = param->width;
    pdc_info->uiRes32Height = param->height;

    dprintk("[%s] enable:%d width:%d height:%d\n",__func__,param->enable,param->width,param->height);
    return 0;
err:
    eprintk("[%s] ERROR!",__func__);
    return -1;
}

int DC_Ioctl (struct fb_info *fb, VENUSFB_MACH_INFO * video_info,unsigned int cmd, unsigned int arg)
{
    int retval = 0;
#define goERROR(tag) {eprintk("ERROR! CMD = %u LINE = %d tag = %d",cmd,__LINE__,tag); goto ERROR;}
    switch (cmd) {
        case DC2VO_GET_BUFFER_ADDR       :
            {
                DCRT_PARAM_BUF_ADDR param;
                if (copy_from_user(&param, (void *)arg, sizeof(param)) != 0)    goERROR(0);
                retval = DC_Get_BufferAddr(fb,video_info,&param);
                if (copy_to_user((void *)arg, &param, sizeof(param)) != 0)      goERROR(0);
                break;
            }
        case DC2VO_SET_RING_INFO         :
            {
                DCRT_PARAM_RPC_ADDR param;
                if (copy_from_user(&param, (void *)arg, sizeof(param)) != 0)    goERROR(0);
                retval = DC_Set_RPCAddr(fb,video_info,&param);
                break;
            }
        case DC2VO_SET_OUT_RATE_INFO     :
            {
                DCRT_PARAM_RATE_INFO param;
                if (copy_from_user(&param, (void *)arg, sizeof(param)) != 0)    goERROR(0);
                if( param.param_size != sizeof(DCRT_PARAM_RATE_INFO) )          goERROR(0);
                retval = DC_Set_RateInfo(fb,video_info,&param);
                break;
            }
        case DC2VO_GET_CLOCK_MAP_INFO    :
            {
                DC_CLOCK_MAP_INFO  param;
                if(DC_Get_Clock_Map_Info(fb,video_info,&param) != 0)           goERROR(0);
                if(copy_to_user((void *)arg,&param,sizeof(DC_CLOCK_MAP_INFO)) != 0) goERROR(0);
                break;
            }
        case DC2VO_GET_CLOCK_INFO        :
            {
                REFCLOCK RefClock;
                if(DC_Get_Clock_Info(fb,video_info,&RefClock) != 0)            goERROR(0);
                if(copy_to_user((void *)arg,&RefClock,sizeof(REFCLOCK)) != 0)   goERROR(0);
                break;
            }
        case DC2VO_RESET_CLOCK_TABLE     :
            {
                unsigned int option = 0;
                if(copy_from_user(&option,(void *)arg,sizeof(option)) != 0)     goERROR(0);
                if(DC_Reset_RefClock_Table(fb,video_info,option) != 0)         goERROR(0);
                break;
            }
        case FBIO_WAITFORVSYNC           :
 	     	{
                unsigned long long nsecs;
                if (DC_Wait_Vsync(fb, video_info, &nsecs) != 0)                goERROR(0);
                break;
            }
        case DC2VO_WAIT_FOR_VSYNC        :
            {
                unsigned long long nsecs;
                if (DC_Wait_Vsync(fb, video_info, &nsecs) != 0)                goERROR(0);
                if(copy_to_user((void *)arg,&nsecs,sizeof(nsecs)) != 0)         goERROR(0);
                break;
            }
        case DC2VO_GET_SURFACE           :
            {
                DCRT_PARAM_SURFACE param;
                if (copy_from_user(&param, (void *)arg, sizeof(param)) != 0)    goERROR(0);
                if( param.param_size != sizeof(DCRT_PARAM_SURFACE) )            goERROR(0);
                retval = DC_Get_Surface(fb, video_info, &param);
                break;
            }
        case DC2VO_GET_SYSTEM_TIME_INFO  :
            {
                DC_SYSTEM_TIME_INFO param;
                DC_Get_System_Time_Info(fb,video_info,&param);
                if(copy_to_user((void *)arg,&param,sizeof(DC_SYSTEM_TIME_INFO)) != 0) goERROR(0);
                break;
            }
        case DC2VO_GET_MAX_FRAME_BUFFER  :
            {
                unsigned int MAX_FRAME_BUFFER = DC_NOHW_MAX_BACKBUFFERS;
                if (copy_to_user((void *)arg, &MAX_FRAME_BUFFER, sizeof(MAX_FRAME_BUFFER)) != 0) goERROR(0);
                break;
            }
        case RTKFB_SET_VSYNC_INT         :
            {
                /*
                 * 1: enable vsync
                 * 0: disable vsync
                 */
                break;
            }
        case DC2VO_SET_ION_SHARE_MEMORY  :
            {
                DC_ION_SHARE_MEMORY param;
                if (copy_from_user(&param, (void *)arg, sizeof(param)) != 0)    goERROR(0);
                if (DC_Set_ION_Share_Memory(fb, video_info, &param) != 0)       goERROR(0);
                break;
            }
        case DC2VO_SET_BUFFER_INFO       :
            {
                DC_BUFFER_INFO param;
                if (copy_from_user(&param, (void *)arg, sizeof(param)) != 0)    goERROR(0);
                if (DC_Set_Buffer_Info(fb, video_info, &param) != 0)            goERROR(0);
                break;
            }
        case DC2VO_SET_BG_SWAP           :
            {
                DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
                unsigned int swap;
                if (pdc_info == NULL)                                           goERROR(0);
                if (copy_from_user(&swap, (void *)arg, sizeof(swap)) != 0)      goERROR(0);

                if (swap)
                    pdc_info->flags |= BG_SWAP;
                else
                    pdc_info->flags &= ~BG_SWAP;

                break;
            }
        case DC2VO_SET_VSYNC_FORCE_LOCK   :
            {
                DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
                unsigned int lock;
                if (pdc_info == NULL)                                           goERROR(0);
                if (copy_from_user(&lock, (void *)arg, sizeof(lock)) != 0)      goERROR(0);

                if (lock)
                    pdc_info->flags |= VSYNC_FORCE_LOCK;
                else
                    pdc_info->flags &= ~VSYNC_FORCE_LOCK;

                break;
            }
        case DC2VO_SET_GLOBAL_ALPHA      :
            {
                DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
                unsigned int alpha;
                if (pdc_info == NULL)                                           goERROR(0);
                if (copy_from_user(&alpha, (void *)arg, sizeof(alpha)) != 0)    goERROR(0);

                if (alpha <= 0xFF)
                    pdc_info->gAlpha = alpha;
                else
                    pdc_info->gAlpha = 0; /* 0 : Pixel Alpha */

                break;
            }
        case DC2VO_SIMPLE_POST_CONFIG    :
            {
                if (dc_do_simple_post_config(video_info, (void *)arg))          goERROR(0);
                break;
            }
        case DC2VO_SET_SYSTEM_TIME_INFO  :
        case DC2VO_SET_BUFFER_ADDR       :
        case DC2VO_SET_DISABLE           :
        case DC2VO_SET_MODIFY            :
            {
                dprintk("[%s %d] CMD = %u \n",__func__,__LINE__,cmd);
                break;
            }
        default:
            retval = -EAGAIN;
    }
    return retval;
ERROR:
#undef goERROR
    return -EFAULT;
}

#ifdef CONFIG_RTK_RPC
void dc_irq_handler(void)
{
    DC_INFO * pdc_info = gpdc_info;
    if (DCINIT == 0)
        return;

    if(!DC_HAS_BIT(pdc_info->vo_vsync_flag, DC_VO_FEEDBACK_NOTIFY))
        return;

    DC_RESET_BIT(pdc_info->vo_vsync_flag,  DC_VO_FEEDBACK_NOTIFY) ;
    dc_update_vsync_timestamp(pdc_info);

    return;
}
EXPORT_SYMBOL(dc_irq_handler);
#else
irqreturn_t dc_irq_handler(int irq, void *dev_id)
{
    VENUSFB_MACH_INFO * video_info = (VENUSFB_MACH_INFO *)dev_id;
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    DC_UNREFERENCED_PARAMETER (pdc_info);

    if(!DC_HAS_BIT(pdc_info->vo_vsync_flag, DC_VO_FEEDBACK_NOTIFY))
        return IRQ_NONE;

    DC_RESET_BIT(pdc_info->vo_vsync_flag,  DC_VO_FEEDBACK_NOTIFY) ;
    dc_update_vsync_timestamp(pdc_info);

    return IRQ_HANDLED;
}
#endif

int Activate_vSync(VENUSFB_MACH_INFO * video_info)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    int result=0;
    DC_UNREFERENCED_PARAMETER (pdc_info);
    dprintk("%s:%d DEBUG!!!! \n", __func__, __LINE__);
#ifndef CONFIG_RTK_RPC
    if( !(pdc_info->flags & ISR_INIT))
    {
        result = request_irq(DCRT_IRQ, dc_irq_handler, IRQF_SHARED, "dc2vo", (void *) video_info);
        if(result)
        {
            eprintk("DC: irq ins fail %i\n", DCRT_IRQ);
            return result;
        }
        else {
            dprintk("DC irq Ins\n");
            smp_mb();
            pdc_info->flags |= ISR_INIT;
            smp_mb();
        }
    }
#endif
    spin_lock_irq(&gASLock);
    DC_SET_BIT(pdc_info->vo_vsync_flag, DC_VO_SET_NOTIFY);
    spin_unlock_irq(&gASLock);
    return result;
}

int DeInit_vSync(VENUSFB_MACH_INFO * video_info)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    unsigned long ulLockFlags;
    static spinlock_t  vSyncLock;
    DC_UNREFERENCED_PARAMETER (pdc_info);
    spin_lock_irqsave(&vSyncLock, ulLockFlags);
    if( pdc_info->flags & ISR_INIT ) {
#ifndef CONFIG_RTK_RPC
        free_irq(DCRT_IRQ, (void *) video_info);
#endif
        smp_mb();
        pdc_info->flags &= ~ISR_INIT;
        smp_mb();
        dprintk("DC irqUn\n");
    }
    spin_unlock_irqrestore(&vSyncLock, ulLockFlags);
    return 0;
}

long dc_wait_vsync_timeout(DC_INFO *pdc_info)
{
	unsigned long   flags;
    ktime_t         timestamp;
    long            timeout;

	read_lock_irqsave(&pdc_info->vsync_lock, flags);
    timestamp = pdc_info->vsync_timestamp;
	read_unlock_irqrestore(&pdc_info->vsync_lock, flags);

    timeout = wait_event_interruptible_timeout(pdc_info->vsync_wait,
            !ktime_equal(timestamp, pdc_info->vsync_timestamp),
            msecs_to_jiffies(pdc_info->vsync_timeout_ms));

    if (!timeout) {
        unsigned int flag = pli_IPCReadULONG((BYTE*)pdc_info->vo_vsync_flag);
        eprintk("[%s %d] wait vsync timeout! vo_vsync_flag:0x%08x\n",
                __func__, __LINE__, flag);
    }

    return timeout;
}

void dc_update_vsync_timestamp(DC_INFO *pdc_info)
{
	unsigned long flags;

	write_lock_irqsave(&pdc_info->vsync_lock, flags);
    pdc_info->vsync_timestamp = ktime_get();
	write_unlock_irqrestore(&pdc_info->vsync_lock, flags);

	wake_up_interruptible_all(&pdc_info->vsync_wait);
}

static void dc_fence_wait(DC_INFO * pdc_info, struct sync_fence *fence)
{
    /* sync_fence_wait() dumps debug information on timeout.  Experience
       has shown that if the pipeline gets stuck, a short timeout followed
       by a longer one provides useful information for debugging. */
    int err = sync_fence_wait(fence, DC_SHORT_FENCE_TIMEOUT);
    if (err >= 0)
        return;

    if (err == -ETIME)
        err = sync_fence_wait(fence, DC_LONG_FENCE_TIMEOUT);

    if (err < 0)
        wprintk("error waiting on fence: %d\n", err);
}

void dc_buffer_dump(struct dc_buffer *buf)
{
    if (!buf) {
        eprintk("buffer is null!\n");
        return;
    }
    wprintk("buffer:%p id:%d ov_engine:%d fmt:%d offset:0x%x ctx:%d [%d %d %d %d] [%d %d %d %d]\n",
            buf, buf->id, buf->overlay_engine, buf->format, buf->offset, buf->context,
            buf->sourceCrop.left,
            buf->sourceCrop.top,
            buf->sourceCrop.right,
            buf->sourceCrop.bottom,
            buf->displayFrame.left,
            buf->displayFrame.top,
            buf->displayFrame.right,
            buf->displayFrame.bottom);
}

void dc_buffer_cleanup(struct dc_buffer *buf)
{
    if (buf->acquire.fence)
        sync_fence_put(buf->acquire.fence);

    kfree(buf);
}

void dc_post_cleanup(DC_INFO *pdc_info, struct dc_pending_post *post)
{
	size_t i;
	for (i = 0; i < post->config.n_bufs; i++)
		dc_buffer_cleanup(&post->config.bufs[i]);

#if 0
    if (post->release.fence)
        sync_fence_put(post->release.fence);
#endif

	kfree(post);
}

static struct sync_fence *dc_sw_complete_fence(DC_INFO *pdc_info)
{
    struct sync_pt *pt;
    struct sync_fence *complete_fence;


    if (!pdc_info->timeline) {
        pdc_info->timeline = sw_sync_timeline_create("rtk_fb");
        if (!pdc_info->timeline)
            return ERR_PTR(-ENOMEM);
        pdc_info->timeline_max = 1;
    }

    dprintk("[%s %d] sw_sync_pt_create (%d)\n", __func__, __LINE__, pdc_info->timeline_max);
    pt = sw_sync_pt_create(pdc_info->timeline, pdc_info->timeline_max);
    pdc_info->timeline_max++;
    if (!pt)
        goto err_pt_create;

    complete_fence = sync_fence_create("rtk_fb", pt);
    if (!complete_fence)
        goto err_fence_create;


    return complete_fence;

err_fence_create:
    eprintk("[%s %d]\n", __func__, __LINE__);
    sync_pt_free(pt);
err_pt_create:
    eprintk("[%s %d]\n", __func__, __LINE__);
    pdc_info->timeline_max--;
    return ERR_PTR(-ENOSYS);
}

static int dc_buffer_import(DC_INFO *pdc_info,
        struct dc_buffer __user *buf_from_user, struct dc_buffer *buf)
{
    struct dc_buffer user_buf;
    int ret = 0;

    if (copy_from_user(&user_buf, buf_from_user, sizeof(user_buf)))
        return -EFAULT;

    memcpy(buf, &user_buf, sizeof(struct dc_buffer));

    if (user_buf.acquire.fence_fd >= 0) {
        buf->acquire.fence = sync_fence_fdget(user_buf.acquire.fence_fd);
        if (!buf->acquire.fence) {
            eprintk("getting fence fd %d failed\n", user_buf.acquire.fence_fd);
            ret = -EINVAL;
            goto done;
        }
    } else
        buf->acquire.fence = (struct sync_fence *) NULL;

    if (buf->overlay_engine > eEngine_MAX) {
        eprintk("invalid overlay engine id mask %u\n", buf->overlay_engine);
        //ret = -EINVAL;
        //goto done;
    }

    dprintk("[%s %d] buf->acquire_fenc : %p\n", __func__, __LINE__, buf->acquire.fence);
done:
    /*
     * if (ret < 0)
     * dc_buffer_cleanup(buf);
     */
    return ret;
}

static void dc_sw_advance_timeline(DC_INFO *pdc_info)
{
#ifdef CONFIG_SW_SYNC
    sw_sync_timeline_inc(pdc_info->timeline, 1);
#else
    BUG();
#endif
}

struct sync_fence *dc_device_post_to_work(DC_INFO * pdc_info,
        struct dc_buffer *bufs, size_t n_bufs)
{
    struct dc_pending_post *cfg;
    struct sync_fence *ret;

    cfg = kzalloc(sizeof(*cfg), GFP_KERNEL);
    if (!cfg) {
        ret = ERR_PTR(-ENOMEM);
        goto err_alloc;
    }

    INIT_LIST_HEAD(&cfg->head);
    cfg->config.n_bufs = n_bufs;
    cfg->config.bufs = bufs;

    mutex_lock(&pdc_info->post_lock);
    ret = dc_sw_complete_fence(pdc_info);
    if (IS_ERR(ret))
        goto err_fence;

    cfg->release.fence = ret;

    list_add_tail(&cfg->head, &pdc_info->post_list);
    queue_kthread_work(&pdc_info->post_worker, &pdc_info->post_work);
    mutex_unlock(&pdc_info->post_lock);
    return ret;

err_fence:
    mutex_unlock(&pdc_info->post_lock);

err_alloc:
    if (cfg) {
        if (cfg->config.bufs)
            kfree(cfg->config.bufs);
        kfree(cfg);
    }
    return ret;
}

struct sync_fence *dc_device_post(DC_INFO *pdc_info,
        struct dc_buffer *bufs, size_t n_bufs)
{
    struct dc_buffer *bufs_copy = NULL;

    bufs_copy = kzalloc(sizeof(bufs_copy[0]) * n_bufs, GFP_KERNEL);
    if (!bufs_copy)
        return ERR_PTR(-ENOMEM);
    memcpy(bufs_copy, bufs, sizeof(bufs_copy[0]) * n_bufs);
    return dc_device_post_to_work(pdc_info, bufs_copy, n_bufs);
}

struct sync_fence *dc_simple_post(DC_INFO *pdc_info,
        struct dc_buffer *buf)
{
    return dc_device_post(pdc_info, buf, 1);
}

static int dc_do_simple_post_config(VENUSFB_MACH_INFO * video_info, void *arg)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    struct dc_simple_post_config __user *cfg = (struct dc_simple_post_config __user *) arg;
    struct sync_fence *complete_fence = NULL;
    int complete_fence_fd;
    struct dc_buffer buf;
    int ret = 0;

    complete_fence_fd = get_unused_fd();
    if (complete_fence_fd < 0) {
        eprintk("[%s %d] complete_fence_fd = %d\n", __func__, __LINE__, complete_fence_fd);
        return complete_fence_fd;
    }

    ret = dc_buffer_import(pdc_info, &cfg->buf, &buf);
    if (ret < 0) {
        eprintk("[%s %d] ret = %d\n", __func__, __LINE__, ret);
        goto err_import;
    }

    complete_fence = dc_simple_post(pdc_info, &buf);
    if (IS_ERR(complete_fence)) {
        ret = PTR_ERR(complete_fence);
        eprintk("[%s %d] complete_fence : %p\n", __func__, __LINE__, complete_fence);
        goto err_put_user;
    }

    sync_fence_install(complete_fence, complete_fence_fd);

    if (put_user(complete_fence_fd, &cfg->complete_fence_fd)) {
        eprintk("[%s %d] put_user complete_fence_fd:%d failed!\n", __func__, __LINE__, complete_fence_fd);
        ret = -EFAULT;
        goto err_put_user;
    }

    return 0;

err_put_user:
    //dc_buffer_cleanup(&buf);
err_import:
    put_unused_fd(complete_fence_fd);
    return ret;
}

int DC_Swap_Buffer(struct fb_info *fb, VENUSFB_MACH_INFO * video_info)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    struct sync_fence *complete_fence = NULL;
    struct dc_buffer buf;

    if (pdc_info->flags & SUSPEND) {
        dc_wait_vsync_timeout(pdc_info);
        return 0;
    }

    if (debug)
        console_unlock();

    buf.id                          = eFrameBuffer;
    buf.overlay_engine              = eEngine_VO;
    buf.offset                      = fb->var.yoffset;
    buf.acquire.fence               = (struct sync_fence *) 0;

    complete_fence = dc_simple_post(pdc_info, &buf);

    if (IS_ERR(complete_fence))
        goto err;

    dc_fence_wait(pdc_info, complete_fence);

    sync_fence_put(complete_fence);

    if (debug)
        console_lock();
    return 0;

err:
    eprintk("[%s %d]!!!!!!! \n", __func__, __LINE__);
    if (debug)
        console_lock();
    return -EFAULT;
}

static int dc_prepare_framebuffer(DC_INFO *pdc_info, struct dc_buffer *buffer)
{
    struct fb_info *fb  = pdc_info->pfbi;


    fb->var.yoffset     = buffer->offset;

    buffer->phyAddr     = fb->fix.smem_start
                            + fb->fix.line_length * fb->var.yoffset
                            + fb->var.xoffset *  (fb->var.bits_per_pixel / 8);

    buffer->stride      = fb->fix.line_length;
    buffer->format      = (pdc_info->flags & BG_SWAP)? INBAND_CMD_GRAPHIC_FORMAT_RGBA8888 : INBAND_CMD_GRAPHIC_FORMAT_ARGB8888_LITTLE;

    if (pdc_info->flags & CHANGE_RES && pdc_info->uiRes32Width > 0 && pdc_info->uiRes32Height > 0) {
        buffer->width   = pdc_info->uiRes32Width;
        buffer->height  = pdc_info->uiRes32Height;
    } else {
        buffer->width   = fb->var.xres;
        buffer->height  = fb->var.yres;
    }

    if (pdc_info->CTX == -1)
        pdc_info->CTX = 0;
    else
        pdc_info->CTX++;

    buffer->context = pdc_info->CTX;
    return 0;
}

static int dc_queue_vo_buffer(DC_INFO *pdc_info, struct dc_buffer *buffer)
{
    VIDEO_GRAPHIC_PICTURE_OBJECT obj;

    if (!(pdc_info->flags & RPC_READY)) {
        eprintk("[%s %d] pdc_info->RING_HEADER = 0x%08x\n",
                __func__, __LINE__, (unsigned int)pdc_info->RING_HEADER);
        buffer->id = eFrameBufferSkip;
        return 0;
    }

    if (pdc_info->flags & SUSPEND) {
        buffer->id = eFrameBufferSkip;
        return 0;
    }

    obj.header.type     = VIDEO_GRAPHIC_INBAND_CMD_TYPE_PICTURE_OBJECT;
    obj.header.size     = sizeof(VIDEO_GRAPHIC_PICTURE_OBJECT);
    obj.context         = (unsigned int) buffer->context;
    obj.PTSH            = 0;
    obj.PTSL            = 0;
    obj.colorkey        = -1;
    obj.alpha           = pdc_info->gAlpha;
    obj.x               = 0;
    obj.y               = 0;
    obj.format          = buffer->format;
    obj.width           = buffer->width;
    obj.height          = buffer->height;
    obj.pitch           = buffer->stride;
    obj.address         = buffer->phyAddr;
    obj.address_right   = 0;
    obj.pitch_right     = 0;
    obj.picLayout       = INBAND_CMD_GRAPHIC_2D_MODE;
    obj.dataByte1       = 0;
    obj.dataByte2       = 0;
    obj.dataByte3       = 0;

    if (ICQ_WriteCmd(&obj, pdc_info->RING_HEADER, pdc_info->RING_HEADER_BASE)) {
        eprintk("[%s %d]ERROR!! Write CMD Error!\n",__FUNCTION__, __LINE__);
        return -EAGAIN;
    }

    return 0;
}

static int dc_queue_framebuffer(DC_INFO *pdc_info, struct dc_buffer *buffer)
{

    dprintk("[%s %d]\n", __func__, __LINE__);

    if(dc_prepare_framebuffer(pdc_info, buffer))
        goto err;

    if(dc_queue_vo_buffer(pdc_info, buffer))
        goto err;

    return 0;

err:
    return -EAGAIN;
}

static int dc_wait_context_ready(DC_INFO *pdc_info, unsigned int context)
{
    u32 refContext = 0, maxWaitVsync = 10;

    do {
        refContext = pli_IPCReadULONG((BYTE*)&pdc_info->REF_CLK->videoContext);

        if ((refContext > (context + 1)) && (refContext != -1U))
            wprintk("refContext:%d context:%d \n", refContext, context);

        if (refContext >= context)
            break;

        if (pdc_info->flags & SUSPEND)
            break;

        if (!dc_wait_vsync_timeout(pdc_info))
            goto err;

    } while (--maxWaitVsync);

    if (!maxWaitVsync)
        goto err;

    return 0;
err:
    eprintk("[%s %d] context:%d refContext:%d waitVsyncTimes %d\n",
            __func__, __LINE__, context, refContext, 10 - maxWaitVsync);
    return -EAGAIN;
}

static int dc_vo_post(DC_INFO *pdc_info, struct dc_buffer *buf)
{
    int ret = 0;

    if (buf->id == eFrameBuffer) {
        ret = dc_queue_framebuffer(pdc_info, buf);
    } else if (buf->id == eFrameBufferSkip) {
        dprintk("eFrameBufferSkip!");
    } else
        eprintk("buffer id (%u) is not ready!",buf->id)

    if (ret)
        goto err;

    return 0;
err:
    dc_buffer_dump(buf);
    return ret;
}

static int dc_vo_complete(DC_INFO *pdc_info, struct dc_buffer *buf)
{
    int ret = 0;

    if (buf->id == eFrameBuffer) {
        if ((buf->context > 0) && !(pdc_info->flags & VSYNC_FORCE_LOCK))
            ret = dc_wait_context_ready(pdc_info, buf->context-1);
        else
            ret = dc_wait_context_ready(pdc_info, buf->context);
    } else if (buf->id == eFrameBufferSkip) {
        dprintk("eFrameBufferSkip!");
    } else
        eprintk("buffer id (%u) is not ready!",buf->id)

    if (ret)
        goto err;

    return 0;
err:
    dc_buffer_dump(buf);
    return ret;
}

static void dc_post_work_func(struct kthread_work *work)
{
    DC_INFO *pdc_info =
        container_of(work, DC_INFO, post_work);
    struct dc_pending_post *post, *next;
    struct list_head saved_list;

    mutex_lock(&pdc_info->post_lock);
    memcpy(&saved_list, &pdc_info->post_list, sizeof(saved_list));
    list_replace_init(&pdc_info->post_list, &saved_list);
    mutex_unlock(&pdc_info->post_lock);

    dprintk("[%s %d]\n", __func__, __LINE__);

    list_for_each_entry_safe(post, next, &saved_list, head) {
        int i;

        for (i = 0; i < post->config.n_bufs; i++) {
            struct sync_fence *fence =
                post->config.bufs[i].acquire.fence;
            if (fence)
                dc_fence_wait(pdc_info, fence);
        }

        for (i = 0; i < post->config.n_bufs; i++) {
            struct dc_buffer *buffer = &post->config.bufs[i];
            if (buffer->overlay_engine == eEngine_VO) {
                dc_vo_post(pdc_info, buffer);
            } else
                eprintk("overlay_engine (%u) is not ready!",buffer->overlay_engine)
        }

        mutex_lock(&pdc_info->complete_lock);
        INIT_LIST_HEAD(&post->head);
        list_add_tail(&post->head, &pdc_info->complete_list);
        queue_kthread_work(&pdc_info->complete_worker, &pdc_info->complete_work);
        mutex_unlock(&pdc_info->complete_lock);
    }
}

static void dc_complete_work_func(struct kthread_work *work)
{
    DC_INFO *pdc_info =
        container_of(work, DC_INFO, complete_work);
    struct dc_pending_post *post, *next;
    struct list_head saved_list;

    mutex_lock(&pdc_info->complete_lock);
    memcpy(&saved_list, &pdc_info->complete_list, sizeof(saved_list));
    list_replace_init(&pdc_info->complete_list, &saved_list);
    mutex_unlock(&pdc_info->complete_lock);

    list_for_each_entry_safe(post, next, &saved_list, head) {
        int i;
        for (i = 0; i < post->config.n_bufs; i++) {
            struct dc_buffer *buffer = &post->config.bufs[i];
            if (buffer->overlay_engine == eEngine_VO) {
                dc_vo_complete(pdc_info, buffer);
            } else
                eprintk("overlay_engine (%u) is not ready!",buffer->overlay_engine)
        }
        dc_sw_advance_timeline(pdc_info);
        list_del(&post->head);
        if (pdc_info->onscreen)
            dc_post_cleanup(pdc_info, pdc_info->onscreen);
        pdc_info->onscreen = post;
        //dc_post_cleanup(pdc_info, post);
    }
}

int Init_post_Worker(VENUSFB_MACH_INFO * video_info)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;

    INIT_LIST_HEAD(&pdc_info->post_list);
    INIT_LIST_HEAD(&pdc_info->complete_list);
    mutex_init(&pdc_info->post_lock);
    mutex_init(&pdc_info->complete_lock);
    init_kthread_worker(&pdc_info->post_worker);
    init_kthread_worker(&pdc_info->complete_worker);

    pdc_info->timeline = NULL;
    pdc_info->timeline_max = 0;

    pdc_info->post_thread = kthread_run(kthread_worker_fn,
            &pdc_info->post_worker, "rtk_post_worker");

    pdc_info->complete_thread = kthread_run(kthread_worker_fn,
            &pdc_info->complete_worker, "rtk_complete_worker");

    if (IS_ERR(pdc_info->post_thread)) {
        int ret = PTR_ERR(pdc_info->post_thread);
        pdc_info->post_thread = NULL;
        pr_err("%s: failed to run config posting thread: %d\n",
                __func__, ret);
        goto err;
    }

    if (IS_ERR(pdc_info->complete_thread)) {
        int ret = PTR_ERR(pdc_info->complete_thread);
        pdc_info->complete_thread = NULL;
        pr_err("%s: failed to run config complete thread: %d\n",
                __func__, ret);
        goto err;
    }

    init_kthread_work(&pdc_info->post_work, dc_post_work_func);
    init_kthread_work(&pdc_info->complete_work, dc_complete_work_func);

    return 0;

err:
    if (pdc_info->post_thread) {
        flush_kthread_worker(&pdc_info->post_worker);
        kthread_stop(pdc_info->post_thread);
    }

    if (pdc_info->complete_thread) {
        flush_kthread_worker(&pdc_info->complete_worker);
        kthread_stop(pdc_info->complete_thread);
    }
    return -1;
}

int DeInit_post_Worker(VENUSFB_MACH_INFO * video_info)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    if (pdc_info->post_thread) {
        flush_kthread_worker(&pdc_info->post_worker);
        kthread_stop(pdc_info->post_thread);
    }
    if (pdc_info->complete_thread) {
        flush_kthread_worker(&pdc_info->complete_worker);
        kthread_stop(pdc_info->complete_thread);
    }

    if (pdc_info->timeline)
        sync_timeline_destroy( (struct sync_timeline *) pdc_info->timeline);

    return 0;
}

int DC_Init(VENUSFB_MACH_INFO * video_info, struct fb_info *fbi)
{
    int retval = 0;
    if (DCINIT) goto DONE;

    if (video_info->dc_info == NULL) {
        iprintk("ALLOC DC INFO BUFFER!\n");
        video_info->dc_info = kmalloc(sizeof(DC_INFO),GFP_KERNEL);
        // DC_INFO SET DEFAULT
        memset(video_info->dc_info,0,sizeof(DC_INFO));
        {
            DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
            struct RTK119X_ipc_shm __iomem *ipc = (void __iomem *)IPC_SHM_VIRT;

            init_waitqueue_head(&pdc_info->vsync_wait);

            pdc_info->gAlpha                = 0;
            pdc_info->vsync_timeout_ms      = 1000;
            pdc_info->flags                 |= BG_SWAP;
            pdc_info->flags                 &= ~VSYNC_FORCE_LOCK;
            pdc_info->pfbi                  = fbi;
            pdc_info->vo_vsync_flag         = &ipc->vo_int_sync;

#ifdef CONFIG_RTK_RPC
            gpdc_info = (DC_INFO*)video_info->dc_info;
#endif
        }
    } else
        wprintk("ALLOC DC INFO BUFFER ARE ALREADY DONE? (0x%08x)\n", (unsigned int)video_info->dc_info);

    retval = Init_post_Worker(video_info);
    if (retval)
        goto DONE;

    if (Activate_vSync(video_info)) {
        eprintk("[%s %d] ERROR! video_info = 0x%08x\n",__func__,__LINE__,(unsigned int)video_info);
        retval = (int) video_info;
        goto DONE;
    }

    DCINIT = 1;
DONE:
    iprintk("[%s %d]\n", __func__, __LINE__);
    return retval;
}

void DC_Deinit(VENUSFB_MACH_INFO * video_info)
{
    iprintk("[%s %d]\n",__func__,__LINE__);
    DeInit_post_Worker(video_info);
    DeInit_vSync(video_info);
    if (video_info->dc_info != NULL) {
#if 1
        iprintk("[%s %d] NOT TO FREE DC INFO BUFFER!",__func__,__LINE__);
#else
        kfree(video_info->dc_info);
        video_info->dc_info = NULL;
#endif
    } else {
        wprintk("[%s %d] video_info->dc_info == NULL \n",__func__,__LINE__);
    }
    DCINIT = 0;
}

int DC_Suspend (VENUSFB_MACH_INFO * video_info)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    pdc_info->flags |= SUSPEND;
    flush_kthread_worker(&pdc_info->post_worker);
    //flush_kthread_worker(&pdc_info->complete_worker);

    /* Disable the interrup */
    DC_RESET_BIT(pdc_info->vo_vsync_flag, DC_VO_SET_NOTIFY);

    msleep(5);
    return 0;
}

int DC_Resume (VENUSFB_MACH_INFO * video_info)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    DC_SET_BIT(pdc_info->vo_vsync_flag, DC_VO_SET_NOTIFY);
    smp_mb();
    pdc_info->flags         &= ~SUSPEND;
    smp_mb();
    return 0;
}

#ifdef CONFIG_REALTEK_AVCPU
int DC_avcpu_event_notify(unsigned long action, VENUSFB_MACH_INFO * video_info)
{
    DC_INFO * pdc_info = (DC_INFO*)video_info->dc_info;
    switch (action) {
        case AVCPU_RESET_PREPARE:
            smp_mb();
            pdc_info->flags &= ~RPC_READY;
            smp_mb();
            DC_RESET_BIT(pdc_info->vo_vsync_flag, DC_VO_SET_NOTIFY);
            break;
        case AVCPU_RESET_DONE:
            DC_SET_BIT(pdc_info->vo_vsync_flag, DC_VO_SET_NOTIFY);
            break;
        case AVCPU_SUSPEND:
        case AVCPU_RESUME:
            break;
        default:
            break;
    }
    return 0;
}
#endif
