//Copyright (C) 2007-2015 Realtek Semiconductor Corporation.
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include <linux/dma-mapping.h>
#include <linux/device.h>

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <asm/page.h>

//#include <linux/interrupt.h>
//#include <linux/wait.h>

#include <linux/uaccess.h>

#include <rbus/reg_kmd.h>
#include <mach/rtk_kmd.h>

///////////////////// MACROS /////////////////////////////////

//volatile static struct kmd_dev* dev = NULL;
static struct kmd_dev* dev = NULL;
static struct platform_device*  kmd_device = NULL;

volatile MD_KMQ_INFO *KMQRegInfo = (volatile MD_KMQ_INFO*)KMQ_REG_BASE;


///////////////////// MACROS /////////////////////////////////
#define _md_map_single(p_data, len, dir)      dma_map_single(&kmd_device->dev, p_data, len, dir)
#define _md_unmap_single(p_data, len, dir)    dma_unmap_single(&kmd_device->dev, p_data, len, dir)
//#define md_lock(md)                           spin_lock_irqsave(&md->lock, md->spin_flags)
//#define md_unlock(md)                         spin_unlock_irqrestore(&md->lock, md->spin_flags)

MD_COMPLETION* kmd_alloc_cmd_completion(
    MD_CMD_HANDLE       request_id,
    dma_addr_t          dst,
    unsigned long       dst_len,
    dma_addr_t          src,
    unsigned long       src_len
    );


/*--------------------------------------------------------------------
 * Func : kmd_open
 *
 * Desc : open md engine on KCPU MQ
 *
 * Parm : N/A
 *
 * Retn : N/A
 --------------------------------------------------------------------*/
static void kmd_open(void)
{
    dma_addr_t PhysAddr;
    int i;
    MD_COMPLETION* cmp;

    DBG_PRINT("\n\n\n\n *****************    KMD init module  *********************\n\n\n\n");

    //start MD
    KMQRegInfo->MdCtrl->Value = MQ_GO | MQ_CLR_WRITE_DATA | MQ_CMD_SWAP;

    if (dev == NULL)
    {
        dev = (struct kmd_dev *)kmalloc(sizeof(struct kmd_dev), GFP_KERNEL);

        if (dev == NULL){
            return ;
        }

        memset((void*)dev, 0, sizeof(struct kmd_dev));
    }

    dev->size = MD_COMMAND_ENTRIES * sizeof(u32);
    //dev->CachedCmdBuf = kmalloc(dev->size, GFP_KERNEL); // cached virt address
    //dev->CmdBuf       =(void*) KSEG1ADDR((unsigned long)dev->CachedCmdBuf);  // non cached virt address
    //PhysAddr          = virt_to_phys(dev->CachedCmdBuf);
    dev->CmdBuf = dma_alloc_coherent(NULL, dev->size, (dma_addr_t*)(&PhysAddr), GFP_DMA | GFP_KERNEL);
    printk("[KMD]Allocate cmd_buffer from dma_alloc. addr:%08x, paddr:%08x\n",
      (u32)dev->CmdBuf, (u32)PhysAddr);
    memset((void*)dev->CmdBuf, 0, dev->size);

    dev->v_to_p_offset = (int32_t)((u8 *)dev->CmdBuf - (u32)PhysAddr);

    dev->wrptr    = 0;
    dev->CmdBase  = (void *)PhysAddr;
    dev->CmdLimit = (void *)(PhysAddr + dev->size);
    init_rwsem(&dev->sem);

    INIT_LIST_HEAD(&dev->task_list);
    INIT_LIST_HEAD(&dev->free_task_list);

    for (i=0; i<32; i++)
    {
        cmp = kmd_alloc_cmd_completion(0, 0, 0, 0, 0);
        if (cmp)
            list_add_tail(&cmp->list, &dev->free_task_list);
    }

    // ??
    //*(volatile unsigned int *)IOA_SMQ_CmdRdptr = 1;

    KMQRegInfo->MdCmdBase->Value = (u32)dev->CmdBase;
    KMQRegInfo->MdCmdLimit->Value = (u32)dev->CmdLimit;
    KMQRegInfo->MdCmdReadPtr->Value = (u32)dev->CmdBase;
    KMQRegInfo->MdCmdWritePtr->Value = (u32)dev->CmdBase;
    KMQRegInfo->MdInstCnt->Value = 0;
    KMQRegInfo->MdInts->Value = 0x3e;

    //enable interrupt
    //writel(SMQ_SET_WRITE_DATA | INT_SYNC, IOA_SMQ_INT_ENABLE);

    wmb();

    //start MD
    KMQRegInfo->MdCtrl->Value = MQ_GO | MQ_SET_WRITE_DATA | MQ_CMD_SWAP;
}



/*--------------------------------------------------------------------
 * Func : kmd_close
 *
 * Desc : close kmd engine
 *
 * Parm : N/A
 *
 * Retn : N/A
 --------------------------------------------------------------------*/
static void kmd_close(void)
{
    int count = 0;
    MD_COMPLETION* cmp;

    while(KMQRegInfo->MdCmdWritePtr->Value != KMQRegInfo->MdCmdReadPtr->Value)
    {
        msleep(100);
        if (count++ > 300)
            return;
    }

    KMQRegInfo->MdCtrl->Value = MQ_GO | MQ_CLR_WRITE_DATA;

    if (dev)
    {
        if (dev->CmdBuf)
            dma_free_coherent(NULL, dev->size, dev->CmdBuf, (dma_addr_t)dev->CmdBase);

        while(!list_empty(&dev->task_list))
        {
            cmp = list_entry(dev->task_list.next, MD_COMPLETION, list);
            list_del(&cmp->list);
            kfree(cmp);
        }

        while(!list_empty(&dev->free_task_list))
        {
            cmp = list_entry(dev->free_task_list.next, MD_COMPLETION, list);
            list_del(&cmp->list);
            kfree(cmp);
        }

        kfree(dev);
        dev = NULL;
    }
    return;
}



/*--------------------------------------------------------------------
 * Func : kmd_check_hardware
 *
 * Desc : check hardware status
 *
 * Parm : N/A
 *
 * Retn : N/A
 --------------------------------------------------------------------*/
void kmd_check_hardware(void)
{
    u32 ctrl = KMQRegInfo->MdCtrl->Value;
    u32 status = KMQRegInfo->MdInts->Value;

    if(0 == (ctrl&BIT1)){
        if(ctrl&BIT3){
            printk("[KMD] md status = idle, status:%08x\n", status);
            // something error
            if(status & BIT2)
                printk("[KMD] Fatal Error, KMQ command decoding error\n");
            if(status & BIT1)
                printk("[KMD] Fatal Error, KMQ instrunction error\n");
            // do error handling
        }
        printk("[KMD] restart md\n");
        KMQRegInfo->MdCtrl->Value = MQ_GO | MQ_SET_WRITE_DATA | MQ_CMD_SWAP;
    }
}


/*--------------------------------------------------------------------
 * Func : WriteCmd
 *
 * Desc : Write Command to Move Data Engine
 *
 * Parm : dev   :   md device
 *        pBuf  :   cmd
 *        nLen  :   cmd length in bytes
 *
 * Retn : N/A
 --------------------------------------------------------------------*/
static
u64 WriteCmd(
    struct kmd_dev*          dev,
    u8*                     pBuf,
    int                     nLen
    )
{
    int i=0;
    u8 *readptr;
    u8 *writeptr;
    u8 *pWptr;
    u8 *pWptrLimit;
    u64 counter64 = 0;

    down_write(&dev->sem);

    kmd_check_hardware();

    writeptr = (u8*)(KMQRegInfo->MdCmdWritePtr->Value);
    // virtual address limitation
    pWptrLimit = (u8 *)dev->CmdLimit + dev->v_to_p_offset;
    //pWptr = writeptr;
    pWptr = (u8 *)dev->CmdBuf + dev->wrptr;

    /*if ((dev->wrptr+nLen) >= dev->size)
    {
        //to cover md copy bus, see mantis 9801 & 9809
        while(1)
        {
            u32 rp = (u32)(KMQRegInfo->MdCmdReadPtr->Value);
            if((u32)writeptr == rp)
                break;
            DBG_PRINT("[KMD] delay for execution.\n");
            udelay(1);
        }
    }*/

    while(1)
    {
        readptr = (u8 *)(KMQRegInfo->MdCmdReadPtr->Value);

        if(readptr <= writeptr)
        {
            readptr += dev->size;
        }
        if((writeptr+nLen) < readptr)
        {
            DBG_PRINT("[KMD] R/W ptr: %p/%p, len: %d.\n",
	                readptr, writeptr, nLen);
            DBG_PRINT("[KMD] CmdBase/CmdLimit: %p/%p, CmdBuf: %p, dev->wrptr: %d.\n",
	                dev->CmdBase, dev->CmdLimit, dev->CmdBuf, dev->wrptr);
            break;
        }
        udelay(1);
    }

    //Start writing command words to the ring buffer.
    for(i=0; i<nLen; i+=sizeof(u32))
    {
        *(u32 *)((u32)pWptr) = endian_swap_32(*(u32 *)(pBuf + i));

        pWptr += sizeof(u32);

        if (pWptr >= pWptrLimit)
            //pWptr -= dev->size;
            pWptr = (u8 *)dev->CmdBuf;
    }

    dev->wrptr += nLen;
    if (dev->wrptr >= dev->size)
        dev->wrptr -= dev->size;

    //convert to physical address
    pWptr -= dev->v_to_p_offset;

    wmb();

    KMQRegInfo->MdCmdWritePtr->Value = (u32)pWptr;
    // Start execution
    DBG_PRINT("[KMD] Start execution.\n");
    KMQRegInfo->MdCtrl->Value = MQ_GO | MQ_SET_WRITE_DATA | MQ_CMD_SWAP;

    counter64 = ++dev->u64IssuedInstCnt;

    up_write(&dev->sem);

    DBG_PRINT("[KMD] MD_Counter: %lld.\n", dev->u64IssuedInstCnt);

    return counter64;
}



/*--------------------------------------------------------------------
 * Func : kmd_checkfinish
 *
 * Desc : check if the specified command finished.
 *
 * Parm : handle   :   md command handle
 *
 * Retn : true : finished, 0 : not finished
 --------------------------------------------------------------------*/
bool kmd_checkfinish(MD_CMD_HANDLE handle)
{
    u32 sw_counter;
    u32 hw_counter;

    if (handle==0)
        return true;

    hw_counter = KMQRegInfo->MdInstCnt->Value & INST_CNT_MASK;
    // Update instruction count
    sw_counter = dev->u64InstCnt & INST_CNT_MASK;
    if(hw_counter >= sw_counter){
        dev->u64InstCnt = 
          (dev->u64InstCnt & ~INST_CNT_MASK_L) + hw_counter;
    }
    else{
        dev->u64InstCnt = 
          (dev->u64InstCnt & ~INST_CNT_MASK_L) + (INST_CNT_MASK_L+1) + hw_counter;
    }

    if(dev->u64InstCnt >= handle){
        return true;
    }
    else{
        kmd_check_hardware(); // command not complete...
        return false;
    }
}



///////////////////////////////////////////////////////////////////////
// Copy Request Management APIs
///////////////////////////////////////////////////////////////////////


/*--------------------------------------------------------------------
 * Func : kmd_alloc_cmd_completion
 *
 * Desc : alloc a completion cmd
 *
 * Parm : N/A
 *
 * Retn : 0 : success, <0 failed
 --------------------------------------------------------------------*/
MD_COMPLETION* kmd_alloc_cmd_completion(
    MD_CMD_HANDLE       request_id,
    dma_addr_t          dst,
    unsigned long       dst_len,
    dma_addr_t          src,
    unsigned long       src_len
    )
{
    MD_COMPLETION* cmp;

    if (list_empty(&dev->free_task_list))
    {
        cmp = (MD_COMPLETION*) kmalloc(sizeof(MD_COMPLETION), GFP_KERNEL);
        memset(cmp, 0, sizeof(MD_COMPLETION));
    }
    else
    {
        // Get from preallocate task list
        cmp = list_entry(dev->free_task_list.next, MD_COMPLETION, list);
        list_del(&cmp->list);
        memset(cmp, 0, sizeof(MD_COMPLETION));
        cmp->flags = 1; // pool
    }

    if (cmp)
    {
        INIT_LIST_HEAD(&cmp->list);

        cmp->request_id = request_id;
        cmp->dst        = dst;
        cmp->dst_len    = dst_len;
        cmp->src        = src;
        cmp->src_len    = src_len;
    }

    return cmp;
}



/*-------------------------------------------------------------------- 
 * Func : kmd_release_cmd_completion
 *
 * Desc : alloc a completion cmd
 *
 * Parm : N/A
 *
 * Retn : 0 : success, <0 failed
 --------------------------------------------------------------------*/
void kmd_release_cmd_completion(
    MD_COMPLETION*      cmp
    )
{
    if (cmp->flags)
    {
        INIT_LIST_HEAD(&cmp->list); // reinit list
        list_add_tail(&cmp->list, &dev->free_task_list); // move to free list
    }
    else
        kfree(cmp);
}



/*--------------------------------------------------------------------
 * Func : kmd_add_cmd_completion
 *
 * Desc : add a completion cmd
 *
 * Parm : N/A
 *
 * Retn : 0 : success, <0 failed
 --------------------------------------------------------------------*/
int kmd_add_cmd_completion(
    MD_CMD_HANDLE       request_id,
    dma_addr_t          dst,
    unsigned long       dst_len,
    dma_addr_t          src,
    unsigned long       src_len
    )
{
    MD_COMPLETION* cmp = NULL;
    down_write(&dev->sem);
    cmp = kmd_alloc_cmd_completion(request_id, dst, dst_len, src, src_len);

    if (cmp)
    {
        list_add_tail(&cmp->list, &dev->task_list);

        MD_COMPLETION_DBG("add complete req = %llu, src=%08x, len=%zu, dst=%08x, len=%zu\n",
                cmp->request_id, cmp->src, cmp->src_len, cmp->dst, cmp->dst_len);

        up_write(&dev->sem);
        return 0;
    }

    up_write(&dev->sem);
    return -1;
}



/*--------------------------------------------------------------------
 * Func : kmd_cmd_complete
 *
 * Desc : do cmd completion.
 *
 * Parm : N/A
 *
 * Retn : MD_CMD
 --------------------------------------------------------------------*/
void kmd_cmd_complete(
    MD_COMPLETION*      cmp
    )
{
    MD_COMPLETION_DBG("complete req = %llu, src=%08x, len=%zu, dst=%08x, len=%zu\n",
            cmp->request_id, cmp->src, cmp->src_len, cmp->dst, cmp->dst_len);

    if (cmp->src && cmp->src_len)
        _md_unmap_single(cmp->src, cmp->src_len, DMA_TO_DEVICE);

    if (cmp->dst && cmp->dst_len)    
        _md_unmap_single(cmp->dst, cmp->dst_len, DMA_FROM_DEVICE);
        
    kmd_release_cmd_completion(cmp);
}





/*--------------------------------------------------------------------
 * Func : kmd_check_copy_tasks
 *
 * Desc : check copy requests.
 *
 * Parm :
 *
 * Retn :
 --------------------------------------------------------------------*/
void kmd_check_copy_tasks()
{
    struct list_head* cur;
    struct list_head* next;
    MD_COMPLETION* cmp = NULL;

    down_write(&dev->sem);

    list_for_each_safe(cur, next, &dev->task_list)
    {
        cmp = list_entry(cur, MD_COMPLETION, list);
        MD_COMPLETION_DBG("check tastk (%llu)\n", cmp->request_id);

        if (!kmd_checkfinish(cmp->request_id))
            break;

        list_del(&cmp->list); // Detatch form the task list
        kmd_cmd_complete(cmp);
    }
    up_write(&dev->sem);
}



/*--------------------------------------------------------------------
 * Func : kmd_copy_sync
 *
 * Desc : sync md copy request
 *
 * Parm : md_copy_t
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
int kmd_copy_sync(MD_COPY_HANDLE handle)
{
    unsigned int aaa = 0;

    kmd_check_copy_tasks();

    if (!list_empty(&dev->task_list))
    {
        MD_COMPLETION_DBG("sync copy (%llu)\n", handle);

        while(!kmd_checkfinish(handle))
        {
            if (aaa++ > 100000)
            {
                printk("[KMD] WARNING, command not complete, req_id = %llu, hw_counter=%lu",
                  handle, (unsigned long)KMQRegInfo->MdInstCnt->Value & INST_CNT_MASK);
                aaa = 0;
            }
            udelay(1);
        }
        if(aaa > 100)
        {
            printk("[KMD]CMD time %dus, req_id = %llu, hw_counter=%lu\n",
              aaa, handle, (unsigned long)KMQRegInfo->MdInstCnt->Value & INST_CNT_MASK);
        }

        kmd_check_copy_tasks();
    }

    return 0;
}



///////////////////////////////////////////////////////////////////////
// Asynchronous APIs
///////////////////////////////////////////////////////////////////////


/*--------------------------------------------------------------------
 * Func : kmd_copy_start
 *
 * Desc : doing memory copy via md
 *
 * Parm : req   :   md_copy_request
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
MD_COPY_HANDLE kmd_copy_start(
    void*                   dst,
    void*                   src,
    int                     len,
    int                     dir
    )
{
    unsigned long   tmp;
    unsigned long   remain = len;
    dma_addr_t      dma_dst, dma_src;
    dma_addr_t      dst_tmp, src_tmp;
    MD_COPY_HANDLE  request_id = 0;
    u32             cmd[4];

    dma_dst = _md_map_single(dst, len, DMA_FROM_DEVICE);
    dma_src = _md_map_single(src, len, DMA_TO_DEVICE);

    dst_tmp = dma_dst;
    src_tmp = dma_src;

    while(remain)
    {
        //to cover md copy bus, see mantis 9801 & 9809
        int bytes_to_next_8bytes_align = (8 - ((u32)src & 0x7)) & 0x7;

        if(((remain - bytes_to_next_8bytes_align) & 0xFF) == 8)
            tmp = 8;
        else
            tmp = remain;

        if (tmp > 1024 * 1024)
            tmp = 1024 * 1024;

        cmd[0] = MOVE_DATA_MD_1B | MOVE_DATA_SS;

        if(!dir)
            cmd[0] |= 1 << 7; //backward

        cmd[1] = (u32)dst_tmp;
        cmd[2] = (u32)src_tmp;
        cmd[3] = (u32)(tmp & 0xFFFFF);

        request_id = kmd_write_command((const char *)cmd, sizeof(cmd));
        remain  -= tmp;
        dst_tmp += tmp;
        src_tmp += tmp;
    }

    kmd_add_cmd_completion(request_id, dma_dst, len, dma_src, len);

    return request_id;
}



/*--------------------------------------------------------------------
 * Func : kmd_memset_start
 *
 * Desc : doing memory set via md
 *
 * Parm : dst     :   Destination
 *        data    :   data to set
 *        len     :   number of bytes
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
MD_CMD_HANDLE kmd_memset_start(
    void*                   dst,
    u8                      data,
    int                     len
    )
{
    unsigned long  remain = len;
    MD_COPY_HANDLE request_id = 0;
    dma_addr_t dma_dst = _md_map_single(dst, len, DMA_FROM_DEVICE);
    dma_addr_t dst_tmp = dma_dst;
    u32 cmd[4];

    while(remain)
    {
        // input_sel to choose cmd[2][7:0]
        cmd[0] = MOVE_DATA_MD_1B | MOVE_DATA_SS | (1 << 6);
        cmd[1] = (u32)dst_tmp;
        cmd[2] = (u32)data;

        if (remain < 1024 * 1024)
        {
            cmd[3] = (u32)remain;
            dst_tmp+= remain;
            remain = 0;
        }
        else
        {
            cmd[3]  = 0;
            dst_tmp+= (1024 * 1024);
            remain -= (1024 * 1024);
        }

        request_id =  (MD_CMD_HANDLE) kmd_write_command((const char *)cmd, sizeof(cmd));
    }

    kmd_add_cmd_completion(request_id, dma_dst, len, 0, 0);

    return request_id;
}

///////////////////////////////////////////////////////////////////////
// Synchronous APIs
///////////////////////////////////////////////////////////////////////


/*--------------------------------------------------------------------
 * Func : kmd_memcpy
 *
 * Desc : doing memory copy via md
 *
 * Parm : lpDst   :   Destination
 *        lpSrc   :   Source
 *        len     :   number of bytes
 *        forward :   1 : Src to Dest, 0 Dest to Src
 *
 * Retn : 0 : succeess, others failed
 --------------------------------------------------------------------*/
int kmd_memcpy(
    void*                   dst,
    void*                   src,
    int                     len,
    bool                    forward
    )
{
    MD_CMD_HANDLE mdh;

    if ((mdh = kmd_copy_start(dst, src, len, forward)))
    {
        kmd_copy_sync(mdh); // wait complete
        return 0;
    }

    return -1;
}



/*--------------------------------------------------------------------
 * Func : kmd_memset
 *
 * Desc : doing memory set via md
 *
 * Parm : lpDst   :   Destination 
 *        data    :   data to set
 *        len     :   number of bytes 
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
int kmd_memset(
    void*                   dst,
    u8                      data,
    int                     len
    )
{
    MD_CMD_HANDLE mdh;

    if ((mdh = kmd_memset_start(dst, data, len)))
    {        
        kmd_copy_sync(mdh); // wait complete
        return 0;
    }

    return -1;
}



///////////////////////////////////////////////////////////////////////
// MISC APIs
///////////////////////////////////////////////////////////////////////


/*--------------------------------------------------------------------
 * Func : kmd_write_command
 *
 * Desc : send command to MD
 *
 * Parm : lpDst   :   Destination
 *        data    :   data to set
 *        len     :   number of bytes
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
MD_CMD_HANDLE kmd_write_command(const char *buf, size_t count)
{
    return WriteCmd(dev, (u8 *)buf, count);
}



//////////////////////////////////////////////////////////////////////////////////////////////
// Platform Device Interface
//////////////////////////////////////////////////////////////////////////////////////////////

#define MD_DEV_NAME "rtk_kmd_drv"

#ifdef CONFIG_PM

static int kmd_drv_probe(struct platform_device *dev)
{
    return strncmp(dev->name,MD_DEV_NAME, strlen(MD_DEV_NAME));
}

static int kmd_drv_remove(struct platform_device *dev)
{
    return 0;
}

static int kmd_drv_pm_suspend(struct platform_device *dev, pm_message_t state)
{
    kmd_close();
    return 0;
}

static int kmd_drv_pm_resume(struct platform_device *dev)
{
    kmd_open();
    return 0;
}

static struct platform_driver kmd_drv = {
    .probe    = kmd_drv_probe,
    .remove   = kmd_drv_remove,
    .suspend  = kmd_drv_pm_suspend,
    .resume   = kmd_drv_pm_resume,
    .driver = {
        .name   = MD_DEV_NAME,
        .owner  = THIS_MODULE,
        .bus    = &platform_bus_type,
    },
};

#endif


/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/

static int __init rtk_kmd_init(void)
{
    kmd_open();

    kmd_device = platform_device_register_simple(MD_DEV_NAME, 0, NULL, 0);

#ifdef CONFIG_PM
    platform_driver_register(&kmd_drv);
#endif

    return 0;
}


static void __exit rtk_kmd_exit(void)
{
    platform_device_unregister(kmd_device);

#ifdef CONFIG_PM
    platform_driver_unregister(&kmd_drv);
#endif

    kmd_close();
}

EXPORT_SYMBOL(kmd_memcpy);
EXPORT_SYMBOL(kmd_memset);


module_init(rtk_kmd_init);
module_exit(rtk_kmd_exit);
