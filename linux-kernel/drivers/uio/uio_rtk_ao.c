#include <linux/device.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/dma-mapping.h>
#include <linux/miscdevice.h>
//#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/uio_driver.h>

#include "../staging/android/ion/ion.h"
#include "../staging/android/uapi/rtk_phoenix_ion.h"
#include "ao_reg/ao_regdef.h"
#include "ao_reg/hdmi_reg.h"
#include "uio_rtk_ao.h"
#include "snd-realtek_common.h"

///////////
//define //
///////////

#define FW_TRACE_EN 1

#define MAX_MMAP_SEC 4  // for HW ioremap
#define MMAP_0_BASE     (0x18000000)
#define MMAP_0_SIZE     (0x1000)
#define MMAP_1_BASE     (0x18006000)
#define MMAP_1_SIZE     (0x1000)
#define MMAP_2_BASE     (0x1800D000)
#define MMAP_2_SIZE     (0x1000)
#define MMAP_3_BASE     (0x18071200)
#define MMAP_3_SIZE     (0x1000)
#define MMAP_RPC_SIZE   (0x1000)
#define RPC_BUF_SIZE    (MMAP_RPC_SIZE - sizeof(AUDIO_RINGBUF_PTR))
#define S_OK 0x10000000
#define VENUSOUT_FLASH_INPIN_NUM    3

#if FW_TRACE_EN
#define FW_TRACE(format, ...) printk(KERN_ALERT format, ##__VA_ARGS__);
#else
#define FW_TRACE(format, ...) 
#endif
#define FW_PRINT(format, ...) printk(KERN_ALERT format, ##__VA_ARGS__);
#define FW_DBG() printk(KERN_ALERT "¡¹ %s %d\n", __FUNCTION__, __LINE__);
#define AP_memcpy memcpy

////////////////////////
// function prototype //
////////////////////////
static int rtk_ao_probe (struct platform_device *pdev);
static int rtk_ao_remove (struct platform_device *pdev);
int ion_share_fd_2_user(struct ion_client* client,struct ion_handle* handle);

////////////////////
// data structure //
////////////////////
typedef struct 
{
	unsigned long base;
	unsigned long limit;
	unsigned long cp;
	unsigned long rp;
	unsigned long wp;
} AUDIO_RINGBUF_PTR;

typedef struct  {
    void __iomem* base_addr_virt[MAX_MMAP_SEC];
    struct uio_info uio_ao_info;
    unsigned char *pRPCBuf_virt;
    unsigned char *pRPCBuf_phy;
    AUDIO_RINGBUF_PTR *pRPCRingBuf; // share with user space
    AUDIO_RINGBUF_PTR nRPCRingBuf_virt;
    struct semaphore kernel_rpc_lock;
} UIO_RTK_AO;

#ifdef ION_TEST_MEMORY_EN
extern struct ion_device* rtk_phoenix_ion_device;
static struct ion_client *p_ion_client = NULL;
static struct ion_handle* pHandle = NULL;
static unsigned int *gpTest = NULL;
static int gTestFd = -1;
#define AUDIO_ION_FLAG              (ION_FLAG_NONCACHED |ION_FLAG_SCPUACC | ION_FLAG_ACPUACC)
#endif

//////////////////////
// global variables //
//////////////////////
static UIO_RTK_AO *gpRtkAO = NULL;
static int probe_done = 0;

static const struct of_device_id rtkao_dt_ids[] = {
     { .compatible = "Realtek,uio_rtk_ao", },
     {},
};

static struct platform_driver rtk_ao_driver = {
	.probe		= rtk_ao_probe,
	.remove		= rtk_ao_remove,
	.driver		= {
		.name	= "rtkao",
		.owner	= THIS_MODULE,
		.of_match_table = rtkao_dt_ids,
	},
};

//////////////
// function //
//////////////

long ring_add(unsigned long ring_base,unsigned long ring_limit,unsigned long ptr, int bytes)
{
	ptr += bytes;
	if (ptr >= ring_limit)
	{
		ptr = ring_base + (ptr-ring_limit);
	}
	return ptr;
}

long buf_memcpy2_ring(long base, long limit, long ptr, char* buf, long size)
{
	if (ptr + size <= limit)
	{					
		AP_memcpy((char*)ptr,buf,size);			
	}
	else
	{
		int i = limit-ptr;
		AP_memcpy((char*)ptr,(char*)buf,i);
		AP_memcpy((char*)base,(char*)(buf+i),size-i);
	}
	
	ptr += size;
	if (ptr >= limit)
	{
		ptr = base + (ptr-limit);
	}	
	return ptr;
}

unsigned long ring_memcpy2_buf(char* buf,unsigned long base,unsigned long limit,unsigned long ptr,unsigned long size)
{
	if (ptr + size <= limit)
	{					
		AP_memcpy(buf,(char*)ptr,size);			
	}
	else
	{
		int i = limit-ptr;
		AP_memcpy((char*)buf,(char*)ptr,i);
		AP_memcpy((char*)(buf+i),(char*)base,size-i);
	}
	
	ptr += size;
	if (ptr >= limit)
	{
		ptr = base + (ptr-limit);
	}	
	return ptr;
}

#ifdef ION_TEST_MEMORY_EN
static void test_ion_init()
{
    ion_phys_addr_t dat;
    int  len;
    p_ion_client = ion_client_create(rtk_phoenix_ion_device, "uio254");
    pHandle = ion_alloc(p_ion_client, 4096, 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, AUDIO_ION_FLAG);
    if(IS_ERR(pHandle))
    {
        FW_PRINT("Err @ %s %d\n", __FUNCTION__, __LINE__);
    }
    if(ion_phys(p_ion_client, pHandle, &dat, &len) != 0) {
        FW_PRINT("Err @ %s %d\n", __FUNCTION__, __LINE__);
    }
    gpTest = ion_map_kernel(p_ion_client, pHandle);
    FW_PRINT("¡¹ test ion virt %x phy %x\n", gpTest, dat);
    *gpTest = 0x12341234;
}
void test_ion_get_fd(unsigned int arg)
{
    gTestFd = ion_share_dma_buf_fd(p_ion_client, pHandle);
    FW_PRINT("¡¹ test ion fd %x\n", gTestFd);
    __put_user(gTestFd, (int __user *)arg);
}
EXPORT_SYMBOL(test_ion_get_fd);
#endif

static int rtk_ao_probe (struct platform_device *pdev)
{
	int i=0;
    struct uio_info *uio_ao_info = &gpRtkAO->uio_ao_info;

	gpRtkAO->pRPCBuf_virt = dma_alloc_coherent(&pdev->dev, MMAP_RPC_SIZE, &gpRtkAO->pRPCBuf_phy, GFP_KERNEL);
    gpRtkAO->pRPCRingBuf = (AUDIO_RINGBUF_PTR *)(gpRtkAO->pRPCBuf_virt + RPC_BUF_SIZE);
    gpRtkAO->nRPCRingBuf_virt.base = gpRtkAO->nRPCRingBuf_virt.wp
        = gpRtkAO->nRPCRingBuf_virt.rp = (unsigned long)gpRtkAO->pRPCBuf_virt;
    gpRtkAO->nRPCRingBuf_virt.limit = gpRtkAO->nRPCRingBuf_virt.base + RPC_BUF_SIZE;

    // init RPC ringbuf
    gpRtkAO->pRPCRingBuf->base = gpRtkAO->pRPCRingBuf->wp = gpRtkAO->pRPCRingBuf->rp = gpRtkAO->pRPCBuf_phy;
    gpRtkAO->pRPCRingBuf->limit = gpRtkAO->pRPCRingBuf->base + RPC_BUF_SIZE;

	uio_ao_info->mem[i].name = "AO RPC";
	uio_ao_info->mem[i].addr = gpRtkAO->pRPCBuf_phy;   // physical
	uio_ao_info->mem[i].size = ALIGN(MMAP_RPC_SIZE, PAGE_SIZE);
	uio_ao_info->mem[i].internal_addr = gpRtkAO->pRPCBuf_virt;    // virtual
	uio_ao_info->mem[i].memtype = UIO_MEM_PHYS;
    FW_TRACE("[uio_ao mmap %d phy %x virt %x size %x]\n", i, uio_ao_info->mem[i].addr, uio_ao_info->mem[i].internal_addr, uio_ao_info->mem[i].size);

    i++;
    uio_ao_info->mem[i].name = "AO_HW_SEC0";
	uio_ao_info->mem[i].addr = MMAP_0_BASE;   // physical
	uio_ao_info->mem[i].size = ALIGN(MMAP_0_SIZE, PAGE_SIZE);
	uio_ao_info->mem[i].internal_addr = gpRtkAO->base_addr_virt[i-1];    // virtual
	uio_ao_info->mem[i].memtype = UIO_MEM_PHYS;
    FW_TRACE("[uio_ao mmap %d phy %x virt %x size %x]\n", i, uio_ao_info->mem[i].addr, uio_ao_info->mem[i].internal_addr, uio_ao_info->mem[i].size);

    i++;
	uio_ao_info->mem[i].name = "AO_HW_SEC1";
	uio_ao_info->mem[i].addr = MMAP_1_BASE;   // physical
	uio_ao_info->mem[i].size = ALIGN(MMAP_1_SIZE, PAGE_SIZE);
	uio_ao_info->mem[i].internal_addr = gpRtkAO->base_addr_virt[i-1];    // virtual
	uio_ao_info->mem[i].memtype = UIO_MEM_PHYS;
    FW_TRACE("[uio_ao mmap %d phy %x virt %x size %x]\n", i, uio_ao_info->mem[i].addr, uio_ao_info->mem[i].internal_addr, uio_ao_info->mem[i].size);

    i++;
	uio_ao_info->mem[i].name = "AO_HW_SEC2";
	uio_ao_info->mem[i].addr = MMAP_2_BASE;   // physical
	uio_ao_info->mem[i].size = ALIGN(MMAP_2_SIZE, PAGE_SIZE);
	uio_ao_info->mem[i].internal_addr = gpRtkAO->base_addr_virt[i-1];    // virtual
	uio_ao_info->mem[i].memtype = UIO_MEM_PHYS;
    FW_TRACE("[uio_ao mmap %d phy %x virt %x size %x]\n", i, uio_ao_info->mem[i].addr, uio_ao_info->mem[i].internal_addr, uio_ao_info->mem[i].size);

    i++;
	uio_ao_info->mem[i].name = "AO_HW_SEC3";
	uio_ao_info->mem[i].addr = MMAP_3_BASE;   // physical
	uio_ao_info->mem[i].size = ALIGN(MMAP_3_SIZE, PAGE_SIZE);
	uio_ao_info->mem[i].internal_addr = gpRtkAO->base_addr_virt[i-1];    // virtual
	uio_ao_info->mem[i].memtype = UIO_MEM_PHYS;
    FW_TRACE("[uio_ao mmap %d phy %x virt %x size %x]\n", i, uio_ao_info->mem[i].addr, uio_ao_info->mem[i].internal_addr, uio_ao_info->mem[i].size);

#ifdef CONFIG_UIO_ASSIGN_MINOR
	uio_ao_info->minor = 254;
#endif
    uio_ao_info->version = "0.0.1";
    uio_ao_info->name = "RTK-AO";
    probe_done = 1;

	if (uio_register_device(&pdev->dev, uio_ao_info))
	{
        kfree(uio_ao_info);
        FW_PRINT("Err @ %s %d\n", __FUNCTION__, __LINE__);
		return -ENODEV;
	}

	platform_set_drvdata(pdev, uio_ao_info);

#ifdef ION_TEST_MEMORY_EN
    test_ion_init();
#endif

    return 0;
}

void rtk_ao_kernel_rpc(
int type, 
unsigned char *pArg, unsigned int size_pArg,
unsigned char *pRet, unsigned int size_pRet,
unsigned char *pFuncRet, unsigned int size_pFuncRet)
{
#define RPC_HEADER_SIZE 12
    unsigned int total_size;
    unsigned int header[3];
    volatile AUDIO_RINGBUF_PTR* pRPCRingBuf = gpRtkAO->pRPCRingBuf;
    down_interruptible(&gpRtkAO->kernel_rpc_lock);
    int count = 0;
    if(probe_done == 0)
    {
        FW_PRINT("ERR gpRtkAO = NULL @ %s %d\n", __FUNCTION__, __LINE__);
        goto exit;
    }
//    FW_PRINT("[ao-uio rpc %d %d %d start]\n", type, size_pArg, size_pRet);
    header[0] = type;
    header[1] = size_pArg;
    header[2] = size_pRet;
    gpRtkAO->nRPCRingBuf_virt.wp = buf_memcpy2_ring(gpRtkAO->nRPCRingBuf_virt.base
        , gpRtkAO->nRPCRingBuf_virt.limit, gpRtkAO->nRPCRingBuf_virt.wp, (char *)header, RPC_HEADER_SIZE);
    gpRtkAO->nRPCRingBuf_virt.wp = buf_memcpy2_ring(gpRtkAO->nRPCRingBuf_virt.base
        , gpRtkAO->nRPCRingBuf_virt.limit, gpRtkAO->nRPCRingBuf_virt.wp, (char *)pArg, size_pArg);

    total_size = RPC_HEADER_SIZE + size_pArg + size_pRet;
    pRPCRingBuf->wp = ring_add(pRPCRingBuf->base, pRPCRingBuf->limit, pRPCRingBuf->wp, total_size);

    while(1)
    {
        if(pRPCRingBuf->wp != pRPCRingBuf->rp)
        {
            mdelay(3);
//            msleep(3);
            if(count % (1000 / 3) == 0 && count != 0)
                FW_PRINT("wait k-rpc %d %x %x %d\n", type, pRPCRingBuf->wp, pRPCRingBuf->rp, count);
            count ++;
        }
        else
            break;
    }

    gpRtkAO->nRPCRingBuf_virt.wp = ring_memcpy2_buf((char *)pRet, gpRtkAO->nRPCRingBuf_virt.base
        , gpRtkAO->nRPCRingBuf_virt.limit, gpRtkAO->nRPCRingBuf_virt.wp, size_pRet);

//    FW_PRINT("[ao-uio rpc %d %d %d done]\n", type, size_pArg, size_pRet);
    *((int *)pFuncRet) = S_OK;

exit:
    up(&gpRtkAO->kernel_rpc_lock);
}
EXPORT_SYMBOL(rtk_ao_kernel_rpc);

static int rtk_ao_remove (struct platform_device *pdev)
{
    return 0;
}

static int __init uio_rtk_ao_init(void)
{
    FW_TRACE("[+] @ %s\n", __FUNCTION__);
    struct device_node *rtkao_node;
    int ret = -EFAULT;
    int i;

    // check device tree node
	rtkao_node = of_find_matching_node(NULL, rtkao_dt_ids);
    if(!rtkao_node)
    {
        FW_PRINT("Err @ %s %d\n", __FUNCTION__, __LINE__);
        goto exit;
    }

    // init
    gpRtkAO = kzalloc(sizeof(*gpRtkAO), GFP_KERNEL);
    for(i=0 ; i<MAX_MMAP_SEC ; ++i)
        gpRtkAO->base_addr_virt[i] = of_iomap(rtkao_node, i);
    sema_init(&gpRtkAO->kernel_rpc_lock, 1);

    ret = platform_driver_register(&rtk_ao_driver);
exit:
    FW_TRACE("[-] @ %s\n", __FUNCTION__);
    return ret;
}

static int __exit uio_rtk_ao_exit(void)
{
    if(gpRtkAO->pRPCBuf_virt)
    {
        kfree(gpRtkAO->pRPCBuf_virt);
    }
    if(gpRtkAO)
    {
        kfree(gpRtkAO);
        gpRtkAO = NULL;
    }
	platform_driver_unregister(&rtk_ao_driver);
    return 0;
}

module_init(uio_rtk_ao_init);
module_exit(uio_rtk_ao_exit);


