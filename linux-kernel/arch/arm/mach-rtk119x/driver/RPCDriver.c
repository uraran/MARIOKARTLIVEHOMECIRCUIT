/*
 * $Id: RPCDriver.c,v 1.10 2004/8/4 09:25 Jacky Exp $
 */
#include <generated/autoconf.h>
//#define   CONFIG_REALTEK_AVCPU

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>     /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/ioctl.h>    /* needed for the _IOW etc stuff used later */
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
//add by Angus
#include <linux/sched.h>


#include <asm/io.h>
#include <asm/system.h>     /* cli(), *_flags */
#include <asm/uaccess.h>    /* copy_to_user() copy_from_user() */

#include <mach/rtk_ipc_shm.h>

#include "RPCDriver.h"
#include "avcpu.h"
#include "debug.h"
#include "dc2vo/dc2vo.h"

#ifdef CONFIG_RTK_FB
extern spinlock_t gASLock;
extern void dc_irq_handler(void);
#else
static DEFINE_SPINLOCK(gASLock);
#endif

extern int venus_irtx_getscancode(u32, u32*, u32*);

MODULE_LICENSE("Dual BSD/GPL");

void __iomem *rpc_int_base;
void __iomem *rpc_int_flag;
static int  rpc_irq;

static int rpc_event_notify(struct notifier_block *self, unsigned long action, void *data);

__maybe_unused static struct notifier_block rpc_event_notifier = {
    .notifier_call  = rpc_event_notify,
};

struct file_operations *rpc_fop_array[]={
    &rpc_poll_fops, /* poll */
    &rpc_intr_fops  /* intr */
};

void **rpc_data_ptr_array[]={
    (void **)&rpc_poll_devices, /* poll */
    (void **)&rpc_intr_devices  /* intr */
};

int rpc_data_size_array[]={
    sizeof(RPC_POLL_Dev), /* poll */
    sizeof(RPC_INTR_Dev)  /* intr */
};

int         rpc_major   = RPC_MAJOR;
const char *rpc_name    = "RPC";

/*
 * Finally, the module stuff
 */

static struct class *rpc_class;

void rpc_set_flag(uint32_t flag)
{
	struct RTK119X_ipc_shm __iomem *ipc = (void __iomem *)IPC_SHM_VIRT;

	writel(__cpu_to_be32(flag), &(ipc->audio_rpc_flag));// audio RPC flag

//  *((int *)phys_to_virt(0x0000b000)) = flag;      // audio RPC flag
//  *((int *)phys_to_virt(0x0000b0d0)) = flag;      // audio RPC flag
//  *((int *)phys_to_virt(0x0000b0d4)) = flag;      // video RPC flag
//  *((int *)phys_to_virt(0x0000b0d8)) = flag;
}


void rpc_set_ir_wakeup_key(uint32_t uScancode, uint32_t uScancode_msk)
{
	struct RTK119X_ipc_shm __iomem *ipc = (void __iomem *)IPC_SHM_VIRT;

	dbg_info("uScancode = 0x%x , uScancode_msk = 0x%x ",uScancode, uScancode_msk);
	writel(__cpu_to_be32(uScancode_msk), &(ipc->ir_scancode_mask));// audio RPC flag for scancode mask
	writel(__cpu_to_be32(uScancode), &(ipc->ir_wakeup_scancode));// audio RPC flag for scancode key
}

uint32_t rpc_get_flag(void)
{
	struct RTK119X_ipc_shm __iomem *ipc = (void __iomem *)IPC_SHM_VIRT;

	return __be32_to_cpu(readl(&(ipc->audio_rpc_flag)));
}

static int rpc_event_notify(struct notifier_block *self, unsigned long action, void *data)
{
#ifdef CONFIG_REALTEK_AVCPU
    switch (action) {
        case AVCPU_SUSPEND:
            pr_debug("[RPC]: AVCPU_SUSPEND...\n");
            //rpc_poll_pause();
            rpc_poll_suspend();
            //rpc_intr_pause();
            rpc_intr_suspend();
            //rpc_kern_pause();
            rpc_kern_suspend();
            mdelay(10);
            break;
        case AVCPU_RESUME:
            pr_debug("[RPC]: AVCPU_RESUME...\n");
            rpc_poll_resume();
            rpc_intr_resume();
            rpc_kern_resume();
            break;
        case AVCPU_RESET_PREPARE:
            pr_debug("[RPC]: AVCPU_RESET_PREPARE...\n");
            rpc_poll_pause();
            rpc_intr_pause();
            rpc_kern_pause();
            mdelay(10);
            rpc_set_flag(0x00000000);
            break;
        case AVCPU_RESET_DONE:
            pr_debug("[RPC]: AVCPU_RESET_DONE...\n");
            rpc_poll_init();
            rpc_intr_init();
            rpc_kern_init();

            // clear the inter-processor interrupts
            //modify by Angus
            //*((int *)REG_SB2_CPU_INT) = 0x0000007e;
DC_SET_BIT(rpc_int_flag,RPC_AUDIO_SET_NOTIFY);

            writel(RPC_INT_AS, rpc_int_base+RPC_SB2_INT);
            writel(RPC_INT_SA, rpc_int_base+RPC_SB2_INT);

// Enable the interrupt from system to audio & video
writel((RPC_INT_SA | RPC_INT_WRITE_1), rpc_int_base+RPC_SB2_INT_EN);

            rpc_set_flag(0xffffffff);

            break;
    }
#endif
    return 0;
}

#ifdef MY_COPY
int my_copy_user(int *des, int *src, int size)
{
    char *csrc, *cdes;
    int i;

    pr_info("%s(des:%p, src:%p, size:%d) pid:%d tid:%d comm:%s\n", __func__, des, src, size, current->tgid, current->pid, current->comm);
    if((unsigned long)des < 0xc0000000 && access_ok(VERIFY_WRITE, des, size) == 0)
        BUG();
    if((unsigned long)src < 0xc0000000 && access_ok(VERIFY_READ, src, size) == 0)
        BUG();
    if (((int)src & 0x3) || ((int)des & 0x3))
        pr_warn("my_copy_user: unaligned happen...\n");

    while (size >= 4) {
        *des++ = *src++;
        size -= 4;
    }

    csrc = (char *)src;
    cdes = (char *)des;
    for (i = 0; i < size; i++)
        cdes[i] = csrc[i];

    return 0;
}
#endif

irqreturn_t rpc_isr(int irq, void *dev_id)
{
    int itr;
    //pr_debug("irq number %d...\n", irq);

    itr = readl(rpc_int_base+RPC_SB2_INT);

#ifdef CONFIG_RTK_FB
    dc_irq_handler();
#endif
#if 1
    if ( !DC_HAS_BIT(rpc_int_flag,RPC_AUDIO_FEEDBACK_NOTIFY)) {
        if (itr & (RPC_INT_AS)) {
            //to clear interrupt, set bit[0] to 0 then we can clear A2S int
            writel(RPC_INT_AS, rpc_int_base+RPC_SB2_INT);
        }
        #if 1
        return IRQ_HANDLED;
        #else
        return IRQ_NONE;
        #endif
    } else {
        //unsigned long flags;
        //spin_lock_irqsave(&gASLock,flags);
        DC_RESET_BIT(rpc_int_flag,RPC_AUDIO_FEEDBACK_NOTIFY);
        //spin_unlock_irqrestore(&gASLock,flags);
    }
#endif

    if (itr & (RPC_INT_AS)) {
        while (itr & (RPC_INT_AS)) {

            // ack the interrupt
            if (itr & RPC_INT_AS) {
                //to clear interrupt, set bit[0] to 0 then we can clear A2S int
                writel(RPC_INT_AS, rpc_int_base+RPC_SB2_INT);
                if (rpc_intr_devices[RPC_INTR_DEV_AS_ID1].ringIn != rpc_intr_devices[RPC_INTR_DEV_AS_ID1].ringOut)
                    tasklet_schedule(&(rpc_intr_extra[RPC_INTR_DEV_AS_ID1].tasklet));
                if (rpc_kern_devices[RPC_KERN_DEV_AS_ID1].ringIn != rpc_kern_devices[RPC_KERN_DEV_AS_ID1].ringOut)
                    wake_up_interruptible(&(rpc_kern_devices[RPC_KERN_DEV_AS_ID1].ptrSync->waitQueue));
            }
            itr = readl(rpc_int_base+RPC_SB2_INT);
        }
    } else {
//      pr_warn("Not RPC interrupt...\n");
        #if 1
        return IRQ_HANDLED;
        #else
        return IRQ_NONE;
        #endif
    }

    return IRQ_HANDLED;
}

__maybe_unused static int rtk_rpc_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct RTK119X_ipc_shm __iomem *ipc = (void __iomem *)IPC_SHM_VIRT;

    if (WARN_ON(!np))
        dbg_err("Could not found device node");

    rpc_int_base = of_iomap(np, 0);
#if 0
    rpc_int_flag = ioremap(DC_VO_SYNC_ADDR,sizeof(unsigned int));
#else
    rpc_int_flag = &ipc->vo_int_sync;
#endif
    if (WARN_ON(!rpc_int_base))
        dbg_warn("Could not map registers for %s", np->name);

    rpc_irq = irq_of_parse_and_map(np, 0);
    if (WARN_ON(!rpc_irq))
        dbg_warn("Could not parse IRQ");

    dbg_info("rpc_int_base:%x irq:%d",(uint32_t)rpc_int_base, rpc_irq);
    spin_lock_irq(&gASLock);
    DC_SET_BIT(rpc_int_flag,RPC_AUDIO_SET_NOTIFY);
    spin_unlock_irq(&gASLock);
    if (request_irq(rpc_irq, rpc_isr, IRQF_SHARED, "rpc", (void *)RPC_ID) != 0)
        dbg_err("Can't get assigned irq...");

    // Enable the interrupt from system to audio & video

    writel((RPC_INT_SA | RPC_INT_WRITE_1), rpc_int_base+RPC_SB2_INT_EN);

    rpc_set_flag(0xffffffff);

#ifdef CONFIG_REALTEK_AVCPU
    register_avcpu_notifier(&rpc_event_notifier);
#endif

    return 0;
}

__maybe_unused static int rtk_rpc_remove(struct platform_device *pdev)
{
    return 0;
}

// cyhuang (2011/11/19) +++
//                          Add for new device PM driver.
#ifdef CONFIG_PM
static int venus_rpc_pm_suspend(struct device *dev) {
    // Disable the interrupt from system to audio & video

#if 1
    int MaxCount = 500;
	u32 uScancode = 0x0;
	u32 uScancode_msk = 0x0;

	rpc_set_flag(0xdaedffff); // STOP AUDIO HAS_CHECK

	while ((rpc_get_flag() != 0x0) && (MaxCount > 0)) {
		mdelay(1);
		MaxCount--;
	}

    rpc_event_notify(NULL,AVCPU_SUSPEND,NULL); // STOP SYSTEM RPC

	dbg_info("venus_rpc_pm_suspend begin");

    DC_RESET_BIT(rpc_int_flag,RPC_AUDIO_SET_NOTIFY); // Disable Interrupt

	//venus_irtx_getscancode(116, &uScancode, &uScancode_msk);					// where 116 is linux,code for KEY_POWER
	//rpc_set_ir_wakeup_key(uScancode, uScancode_msk);

	rpc_set_flag(0xdeadffff); // WAIT AUDIO RPC SUSPEND READY

	while ((rpc_get_flag() != 0x0) && (MaxCount > 0)) {
		mdelay(1);
		MaxCount--;
	}


	dbg_info("venus_rpc_pm_suspend end (Wait %d ms) ",(500 - MaxCount));
#else
    writel(RPC_INT_SA, rpc_int_base+RPC_SB2_INT_EN);
    //writel(RPC_INT_SV, (void *)REG_SB2_CPU_INT_EN);
    //writel(RPC_INT_SV2, (void *)REG_SB2_CPU_INT_EN);
    //rtd_outl(REG_SB2_CPU_INT_EN, RPC_INT_SA);
    //rtd_outl(REG_SB2_CPU_INT_EN, RPC_INT_SV);
    //rtd_outl(REG_SB2_CPU_INT_EN, RPC_INT_SV2);
#endif

    return 0;
}

static int venus_rpc_pm_resume(struct device *dev) {
    // Enable the interrupt from system to audio & video
	dbg_info("venus_rpc_pm_resume begin");

#if 1
    DC_SET_BIT(rpc_int_flag,RPC_AUDIO_SET_NOTIFY);
	rpc_set_flag(0xffffffff);
#else
    writel((RPC_INT_SA | RPC_INT_WRITE_1), rpc_int_base+RPC_SB2_INT_EN);
    //writel((RPC_INT_SV | RPC_INT_WRITE_1), (void *)REG_SB2_CPU_INT_EN);
    //writel((RPC_INT_SV2 | RPC_INT_WRITE_1), (void *)REG_SB2_CPU_INT_EN);
    //rtd_outl(REG_SB2_CPU_INT_EN, (RPC_INT_SA | RPC_INT_WRITE_1));
    //rtd_outl(REG_SB2_CPU_INT_EN, (RPC_INT_SV | RPC_INT_WRITE_1));
    //rtd_outl(REG_SB2_CPU_INT_EN, (RPC_INT_SV2 | RPC_INT_WRITE_1));
#endif

    rpc_event_notify(NULL,AVCPU_RESUME,NULL);
    return 0;
}

static const struct dev_pm_ops venus_rpc_pm_ops = {
    .suspend    = venus_rpc_pm_suspend,
    .resume     = venus_rpc_pm_resume,
#ifdef CONFIG_HIBERNATION
    .freeze     =  venus_rpc_pm_suspend,
    .thaw       =  venus_rpc_pm_resume,
    .poweroff   =  venus_rpc_pm_suspend,
    .restore    =  venus_rpc_pm_resume,
#endif
};

#endif

static struct platform_device *venus_rpc_devs = 0;

__maybe_unused static struct of_device_id rtk_rpc_ids[] = {
    { .compatible = "Realtek,rtk-rpc" },
    { /* Sentinel */ },
};

// Change device_driver to platform_driver.
static struct platform_driver venus_rpc_driver = {
//  .probe  = rtk_rpc_probe,
//  .remove = rtk_rpc_remove,
    .driver = {
        .name       = "Venus_RPC",
        .bus        = &platform_bus_type,
    #ifdef CONFIG_PM
        .pm         = &venus_rpc_pm_ops,
    #endif
//      .of_match_table = rtk_rpc_ids,
    },
};
// +++ cyhuang (2011/03/23)

static ssize_t show_tasks(struct device *cd, struct device_attribute *attr, char *buf)
{
    RPC_DEV_EXTRA *extra = (RPC_DEV_EXTRA *)dev_get_drvdata(cd);
    RPC_COMMON_Dev *dev = (RPC_COMMON_Dev *)extra->dev;
    RPC_PROCESS *curr = (RPC_PROCESS *)extra->currProc;
    RPC_PROCESS *proc;
    RPC_THREAD *thread;
    RPC_HANDLER *handler;
    struct task_struct *task;
    ssize_t cnt = 0;

    cnt += scnprintf(buf + cnt, (ssize_t)PAGE_SIZE - cnt, "name:      %s\n", extra->name);
    cnt += scnprintf(buf + cnt, (ssize_t)PAGE_SIZE - cnt, "currProc:  %d\n", curr ? curr->pid : 0);
    cnt += scnprintf(buf + cnt, (ssize_t)PAGE_SIZE - cnt, "nextRpc:   %p\n", extra->nextRpc);
    cnt += scnprintf(buf + cnt, (ssize_t)PAGE_SIZE - cnt, "RingBuf:   %p\n", dev->ringBuf);
    cnt += scnprintf(buf + cnt, (ssize_t)PAGE_SIZE - cnt, "RingStart: %p\n", dev->ringStart);
    cnt += scnprintf(buf + cnt, (ssize_t)PAGE_SIZE - cnt, "RingIn:    %p\n", dev->ringIn);
    cnt += scnprintf(buf + cnt, (ssize_t)PAGE_SIZE - cnt, "RingOut:   %p\n", dev->ringOut);
    cnt += scnprintf(buf + cnt, (ssize_t)PAGE_SIZE - cnt, "RingEnd:   %p\n", dev->ringEnd);
    list_for_each_entry(proc, &extra->tasks, list){
        task = find_task_by_vpid(proc->pid);
        cnt += scnprintf(buf + cnt, (ssize_t)PAGE_SIZE - cnt, "\nProcess:%s\n", task ? task->comm : "N/A");
        list_for_each_entry(handler, &proc->handlers, list){
            cnt += scnprintf(buf + cnt, (ssize_t)PAGE_SIZE - cnt, "\tprogramID:%lu\n", handler->programID);
        }
        cnt += scnprintf(buf + cnt, (ssize_t)PAGE_SIZE - cnt, "\tp:%d\n", proc->pid);
        list_for_each_entry(thread, &proc->threads, list){
            cnt += scnprintf(buf + cnt, (ssize_t)PAGE_SIZE - cnt, "\tt:%d\n", thread->pid);
        }
    }
    return cnt;
}

static DEVICE_ATTR(tasks, S_IRUGO, show_tasks, NULL);

// Destructor of this module.
void RPC_cleanup_module(void)
{
    int i;

    if (venus_rpc_devs != 0) {
        if (venus_rpc_devs->dev.driver != 0)
            platform_driver_unregister(&venus_rpc_driver);
        platform_device_unregister(venus_rpc_devs);
        venus_rpc_devs = 0;
    }

    /* call the cleanup functions for friend devices */
    rpc_poll_cleanup();
    rpc_intr_cleanup();

    for (i = 0; i < RPC_NR_DEVS; i++) {
        RPC_DEV_EXTRA *extra = ((i % RPC_NR_PAIR) == 0) ? &rpc_poll_extra[i/RPC_NR_PAIR] : &rpc_intr_extra[i/RPC_NR_PAIR];
        device_remove_file(extra->sdev, &dev_attr_tasks);
        device_destroy(rpc_class, MKDEV(rpc_major, i));
    }
    device_destroy(rpc_class, MKDEV(rpc_major, 100));
    class_destroy(rpc_class);
    /* cleanup_module is never called if registering failed */
    unregister_chrdev(rpc_major, rpc_name);

#ifdef CONFIG_REALTEK_AVCPU
    unregister_avcpu_notifier(&rpc_event_notifier);
#endif

    free_irq(rpc_irq, (void *)RPC_ID);

    pr_debug("Goodbye, RPC~\n");
}

static char *rpc_devnode(struct device *dev, mode_t *mode)
{
    *mode = 0666;
    return NULL;
}

// Constructor of this module.
int RPC_init_module(void)
{
    int result;
    int i;
    struct RTK119X_ipc_shm __iomem *ipc = (void __iomem *)IPC_SHM_VIRT;

    /* register rpc_poll_fops as default file operation */
    result = register_chrdev(rpc_major, rpc_name, &rpc_poll_fops);
    if (result < 0) {
        pr_debug("Can't get major %d\n", RPC_MAJOR);
        return result;
    }
    if (rpc_major == 0) rpc_major = result; /* dynamic */
    pr_debug("RPC major number: %d\n", rpc_major);
    dbg_info("RPC major number: %d", rpc_major);

    /* At this point call the init function for any kind device */
    if ( (result = rpc_poll_init()) )
        goto fail;
    if ( (result = rpc_intr_init()) )
        goto fail;
    if ( (result = rpc_kern_init()) )
        goto fail;

    rpc_class = class_create(THIS_MODULE, "rpc");
    if (IS_ERR(rpc_class))
        return PTR_ERR(rpc_class);

    rpc_class->devnode = rpc_devnode;
    for (i = 0; i < RPC_NR_DEVS; i++) {
        RPC_DEV_EXTRA *extra = ((i % RPC_NR_PAIR) == 0) ? &rpc_poll_extra[i/RPC_NR_PAIR] : &rpc_intr_extra[i/RPC_NR_PAIR];
        struct device *dev = device_create(rpc_class, NULL, MKDEV(rpc_major, i), NULL, "rpc%d", i);
        device_create_file(dev, &dev_attr_tasks);
        extra->sdev = dev;
        dev_set_drvdata(dev, extra);
    }
    device_create(rpc_class, NULL, MKDEV(rpc_major, 100), NULL, "rpc100");

    rpc_int_base = (void __iomem*)0xfe01a104;
#if 0
    rpc_int_flag = ioremap(DC_VO_SYNC_ADDR,sizeof(unsigned int));
#else
    rpc_int_flag = &ipc->vo_int_sync;
#endif
    rpc_irq = 65;   // CPU interrupt
    spin_lock_irq(&gASLock);
    DC_SET_BIT(rpc_int_flag,RPC_AUDIO_SET_NOTIFY);
    spin_unlock_irq(&gASLock);
    if (request_irq(rpc_irq, rpc_isr, IRQF_SHARED, "rpc", (void *)RPC_ID) != 0)
        dbg_err("Can't get assigned irq...");

    venus_rpc_devs = platform_device_register_simple("Venus_RPC", -1, NULL, 0);
    if ((result = platform_driver_register(&venus_rpc_driver)) != 0) {
        dbg_err("Can't register RPC device driver...");
        goto fail;
    }
    // Enable the interrupt from system to audio & video

    writel((RPC_INT_SA | RPC_INT_WRITE_1), rpc_int_base+RPC_SB2_INT_EN);

    rpc_set_flag(0xffffffff);

#ifdef CONFIG_REALTEK_AVCPU
    register_avcpu_notifier(&rpc_event_notifier);
#endif

    return 0; /* succeed */

fail:
    pr_debug("RPC error number: %d\n", result);
    RPC_cleanup_module();
    return result;
}

module_init(RPC_init_module);
module_exit(RPC_cleanup_module);
