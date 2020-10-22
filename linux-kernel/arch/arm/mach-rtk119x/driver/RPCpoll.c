/*
 * $Id: RPCpoll.c,v 1.10 2004/8/4 09:25 Jacky Exp $
 */
#include <generated/autoconf.h>
/*
#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
    #define MODVERSIONS
#endif

#ifdef MODVERSIONS
    #include <linux/modversions.h>
#endif

#ifndef MODULE
#define MODULE
#endif
*/
#include <linux/module.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>     /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/ioctl.h>    /* needed for the _IOW etc stuff used later */
#include <linux/delay.h>

#include <asm/system.h>     /* cli(), *_flags */
#include <asm/uaccess.h>    /* copy_to_user() copy_from_user() */
#include <linux/dma-mapping.h>

#include "RPCDriver.h"
#include "debug.h"

RPC_POLL_Dev *rpc_poll_devices;
#ifdef CONFIG_REALTEK_RPC_KCPU
RPC_POLL_Dev *rpc_poll_kcpu_devices;
#endif
int rpc_poll_is_paused = 0;
int rpc_poll_is_suspend = 0;

static unsigned int *DbgFlag_A = NULL;
static dma_addr_t phy_DbgFlag_A;

RPC_DEV_EXTRA rpc_poll_extra[RPC_NR_DEVS/RPC_NR_PAIR];

int rpc_poll_init(void)
{
        static int is_init = 0;
        int result = 0, i;

    // Create corresponding structures for each device.
    rpc_poll_devices = (RPC_POLL_Dev *)AVCPU2SCPU(RPC_POLL_RECORD_ADDR);

    for (i = 0; i < RPC_INTR_DEV_TOTAL; i++) {
        pr_debug("rpc_poll_device %d addr: %x\n", i, (unsigned)&rpc_poll_devices[i]);
        rpc_poll_devices[i].ringBuf = (char *)(RPC_POLL_DEV_ADDR + i*RPC_RING_SIZE*2);

        // Initialize pointers...
        rpc_poll_devices[i].ringStart = rpc_poll_devices[i].ringBuf;
        rpc_poll_devices[i].ringEnd = rpc_poll_devices[i].ringBuf+RPC_RING_SIZE;
        rpc_poll_devices[i].ringIn = rpc_poll_devices[i].ringBuf;
        rpc_poll_devices[i].ringOut = rpc_poll_devices[i].ringBuf;

        pr_debug("The %dth RPC_POLL_Dev:\n", i);
        pr_debug("RPC ringStart: 0x%8x\n", (int)AVCPU2SCPU(rpc_poll_devices[i].ringStart));
        pr_debug("RPC ringEnd:   0x%8x\n", (int)AVCPU2SCPU(rpc_poll_devices[i].ringEnd));
        pr_debug("RPC ringIn:    0x%8x\n", (int)AVCPU2SCPU(rpc_poll_devices[i].ringIn));
        pr_debug("RPC ringOut:   0x%8x\n", (int)AVCPU2SCPU(rpc_poll_devices[i].ringOut));
        pr_debug("\n");

        rpc_poll_extra[i].nextRpc = rpc_poll_devices[i].ringOut;
        rpc_poll_extra[i].currProc = NULL;

        if (!is_init) {
            rpc_poll_devices[i].ptrSync = kmalloc(sizeof(RPC_SYNC_Struct), GFP_KERNEL);

            // Initialize wait queue...
            //init_waitqueue_head(&(rpc_poll_devices[i].ptrSync->waitQueue));

            // Initialize sempahores...
            init_rwsem(&rpc_poll_devices[i].ptrSync->readSem);
            init_rwsem(&rpc_poll_devices[i].ptrSync->writeSem);

            rpc_poll_extra[i].dev = (void *)&rpc_poll_devices[i];
            INIT_LIST_HEAD(&rpc_poll_extra[i].tasks);
            //tasklet_init(&rpc_poll_extra[i].tasklet, rpc_dispatch, (unsigned long)&rpc_poll_extra[i]);
            spin_lock_init(&rpc_poll_extra[i].lock);
            switch(i){
                case 0: rpc_poll_extra[i].name = "AudioPollWrite"; break;
                case 1: rpc_poll_extra[i].name = "AudioPollRead"; break;
                case 2: rpc_poll_extra[i].name = "Video1PollWrite"; break;
                case 3: rpc_poll_extra[i].name = "Video1PollRead"; break;
                case 4: rpc_poll_extra[i].name = "Video2PollWrite"; break;
                case 5: rpc_poll_extra[i].name = "Video2PollRead"; break;
            }
        }
    }

    is_init = 1;
    rpc_poll_is_paused = 0;
    rpc_poll_is_suspend = 0;

//fail:
    return result;
}

int rpc_poll_pause(void)
{
    rpc_poll_is_paused = 1;
    return 0;
}

int rpc_poll_suspend(void)
{
    rpc_poll_is_suspend = 1;
    return 0;
}

int rpc_poll_resume(void)
{
    rpc_poll_is_suspend = 0;
    return 0;
}

void rpc_poll_cleanup(void)
{
        int num = RPC_NR_DEVS/RPC_NR_PAIR, i;

    // Clean corresponding structures for each device.
    if (rpc_poll_devices) {
        // Clean ring buffers.
        for (i = 0; i < num; i++) {
//          if (rpc_poll_devices[i].ringBuf)
//              kfree(rpc_poll_devices[i].ringBuf);
        }

//        kfree(rpc_poll_devices);
    }

    return;
}

int rpc_poll_open(struct inode *inode, struct file *filp)
{
    int minor = MINOR(inode->i_rdev);

    if (minor == 100) {
        filp->f_op = &rpc_ctrl_fops;
        return filp->f_op->open(inode, filp); /* dispatch to specific open */
    }

    /*
     * If private data is not valid, we are not using devfs
     * so use the minor number to select a new f_op
     */
    if (!filp->private_data && (minor%RPC_NR_PAIR != 0)) {
        filp->f_op = rpc_fop_array[minor%RPC_NR_PAIR];
        return filp->f_op->open(inode, filp); /* dispatch to specific open */
    }

    pr_debug("RPC poll open with minor number: %d\n", minor);

        if (!filp->private_data) {
                RPC_PROCESS *proc = kmalloc(sizeof(RPC_PROCESS), GFP_KERNEL);
                if(proc == NULL){
                        pr_err("%s: failed to allocate RPC_PROCESS", __func__);
                        return -ENOMEM;
                }

#ifdef CONFIG_REALTEK_RPC_KCPU
                if (minor >= RPC_NR_DEVS)
                        proc->dev = (RPC_COMMON_Dev *)&rpc_poll_kcpu_devices[(minor-RPC_NR_DEVS)/RPC_NR_PAIR];
                else
#endif
                        proc->dev = (RPC_COMMON_Dev *)&rpc_poll_devices[minor/RPC_NR_PAIR];

                proc->extra = &rpc_poll_extra[minor/RPC_NR_PAIR];
                proc->pid = current->tgid; //current->tgid = process id, current->pid = thread id
                init_waitqueue_head(&proc->waitQueue);
                INIT_LIST_HEAD(&proc->threads);
#ifdef CONFIG_REALTEK_RPC_PROGRAM_REGISTER
                INIT_LIST_HEAD(&proc->handlers);
#endif
                list_add(&proc->list, &proc->extra->tasks);
                pr_debug("%s: Current process pid:%d tgid:%d => %d(%p) for %s(%p)\n", __func__, current->pid, current->tgid, proc->pid, &proc->waitQueue, proc->extra->name, proc->dev);

                filp->private_data = proc;
        }

//    MOD_INC_USE_COUNT;  /* Before we maybe sleep */

    return 0;          /* success */
}

int rpc_poll_release(struct inode *inode, struct file *filp)
{
#ifdef CONFIG_REALTEK_RPC_PROGRAM_REGISTER
        RPC_HANDLER *handler, *hdltmp;
#endif
        int minor = MINOR(inode->i_rdev);

        RPC_PROCESS *proc = filp->private_data;
        RPC_POLL_Dev *dev = (RPC_POLL_Dev *)proc->dev; /* the first listitem */
        RPC_DEV_EXTRA *extra = proc->extra;

        if(extra->currProc == proc){
                pr_debug("%s: clear %s(%p) current process\n", __func__, proc->extra->name, dev);
                update_currProc(extra, NULL);
                if(minor == 2 || minor == 6 || minor == 10){ //poll read device (ugly code)
                        if(!rpc_done(extra)){
                                pr_err("%s: previous rpc hasn't finished, force clear!! ringOut %p => %p\n", __func__,
                                AVCPU2SCPU(dev->ringOut), AVCPU2SCPU(extra->nextRpc));
                                down_write(&dev->ptrSync->readSem);
                                dev->ringOut = extra->nextRpc;
                                up_write(&dev->ptrSync->readSem);
                        }
                }
        }

#ifdef CONFIG_REALTEK_RPC_PROGRAM_REGISTER
        //unregister myself from handler list
        list_for_each_entry_safe(handler, hdltmp, &proc->handlers, list){
                list_del(&handler->list);
                kfree(handler);
        }
#endif

        //remove myself from task list
        list_del(&proc->list);
        kfree(proc);

    pr_debug("RPC poll close with minor number: %d\n", minor);

//    MOD_DEC_USE_COUNT;

    return 0;          /* success */
}

// We don't need parameter f_pos here...
ssize_t rpc_poll_read(struct file *filp, char *buf, size_t count,
                loff_t *f_pos)
{
        RPC_PROCESS *proc = filp->private_data;
        RPC_POLL_Dev *dev = (RPC_POLL_Dev *)proc->dev; /* the first listitem */
        RPC_DEV_EXTRA *extra = proc->extra;
        int temp, size;
        size_t r;
        ssize_t ret = 0;
        char *ptmp;
        int rpc_ring_size = dev->ringEnd - dev->ringStart;

        //pr_debug("%s:%d thread:%s pid:%d tgid:%d device:%s\n", __func__, __LINE__, current->comm, current->pid, current->tgid, extra->name);
        if (rpc_poll_is_paused) {
            dbg_err("RPCpoll: someone access rpc poll during the pause...\n");
            pr_err("%s:%d buf:%p count:%u EAGAIN\n", __func__, __LINE__, buf, count);
            msleep(1000);
            return -EAGAIN;
        }

    while (rpc_poll_is_suspend) {
        pr_warn("RPCpoll: someone access rpc poll during the suspend!!!...\n");
        msleep(1000);
    }

        wmb();
        down_write(&dev->ptrSync->readSem);

        if (need_dispatch(extra)) {
            tasklet_disable(&extra->tasklet); //wait until tasklet finish before disable it
            spin_lock_bh(&extra->lock);
            if (need_dispatch(extra)) {
                rpc_dispatch((unsigned long)extra);
            }
            spin_unlock_bh(&extra->lock);
            tasklet_enable(&extra->tasklet);
        }

        //pr_debug("%s: dev:%s(%p) currProc:%p\n", __func__, extra->name, dev, extra->currProc);
        if ((extra->currProc != proc) || (ring_empty((RPC_COMMON_Dev *)dev))) {
                if (unlikely(!(filp->f_flags & O_NONBLOCK))){
                        //pr_warn("%s:%d:%s Warning: pid:%d use blocking mode with poll buffer!\n", __func__, __LINE__, extra->name, current->pid);
                }
                goto out; //return anyway
        }

        if (dev->ringIn > dev->ringOut)
                size = dev->ringIn - dev->ringOut;
        else
                size = rpc_ring_size + dev->ringIn - dev->ringOut;

        //pr_debug("%s: %s: count:%d size:%d\n", __func__, extra->name, count, size);
        //peek_rpc_struct(__func__, dev);
        if (count > size)
                count = size;

        temp = dev->ringEnd - dev->ringOut;
        if (temp >= count) {
#ifdef MY_COPY
                r = my_copy_user((int *)buf, (int *)AVCPU2SCPU(dev->ringOut), count);
#else
                r = copy_to_user((int *)buf, (int *)AVCPU2SCPU(dev->ringOut), count);
#endif
                if(r) {
                        pr_err("%s:%d buf:%p count:%u EFAULT\n", __func__, __LINE__, buf, count);
                        ret = -EFAULT;
                        goto out;
                }
                ret += count;
                ptmp = dev->ringOut + ((count+3) & 0xfffffffc);
                if (ptmp == dev->ringEnd)
                        dev->ringOut = dev->ringStart;
                else
                        dev->ringOut = ptmp;

                //pr_debug("RPC Read is in 1st kind...\n");
        } else {
#ifdef MY_COYP
                r = my_copy_user((int *)buf, (int *)AVCPU2SCPU(dev->ringOut), temp);
#else
                r = copy_to_user((int *)buf, (int *)AVCPU2SCPU(dev->ringOut), temp);
#endif
                if(r) {
                        pr_err("%s:%d buf:%p count:%u EFAULT\n", __func__, __LINE__, buf, count);
                        ret = -EFAULT;
                        goto out;
                }
                count -= temp;

#ifdef MY_COPY
                r = my_copy_user((int *)(buf+temp), (int *)AVCPU2SCPU(dev->ringStart), count);
#else
                r = copy_to_user((int *)(buf+temp), (int *)AVCPU2SCPU(dev->ringStart), count);
#endif
                if(r) {
                        pr_err("%s:%d buf:%p count:%u EFAULT\n", __func__, __LINE__, buf, count);
                        ret = -EFAULT;
                        goto out;
                }
                ret += (temp + count);
                dev->ringOut = dev->ringStart+((count+3) & 0xfffffffc);

                //pr_debug("RPC Read is in 2nd kind...\n");
        }

        tasklet_disable(&extra->tasklet); //wait until tasklet finish before disable it
        spin_lock_bh(&extra->lock);
        if (rpc_done(extra)) {
                pr_debug("%s: Previous RPC is done, unregister myself\n", __func__);
                update_currProc(extra, NULL);
        }

        //process next rpc command if any
        if (need_dispatch(extra)) {
            rpc_dispatch((unsigned long)extra);
        }
        spin_unlock_bh(&extra->lock);
        tasklet_enable(&extra->tasklet);

        pr_debug("%s:%d buf:%p count:%u actual:%d\n", __func__, __LINE__, buf, count, ret);
out:
        up_write(&dev->ptrSync->readSem);
        wmb();
        //pr_debug("RPC poll ringOut pointer is : 0x%8x\n", (int)AVCPU2SCPU(dev->ringOut));
        //pr_debug("%s:%d pid:%d reads %d bytes\n", extra->name, __LINE__, current->pid, ret);
        return ret;
}

// We don't need parameter f_pos here...
ssize_t rpc_poll_write(struct file *filp, const char *buf, size_t count,
                loff_t *f_pos)
{
        RPC_PROCESS *proc = filp->private_data;
        RPC_POLL_Dev *dev = (RPC_POLL_Dev *)proc->dev; /* the first listitem */
        RPC_DEV_EXTRA *extra = proc->extra;
        RPC_DEV_EXTRA *rextra = extra + 1;
        int temp, size;
        size_t r;
        ssize_t ret = 0;
        char *ptmp;
        int rpc_ring_size = dev->ringEnd - dev->ringStart;

        if (rpc_poll_is_paused) {
                pr_warn("RPCpoll: someone access rpc poll during the pause...\n");
                msleep(1000);
                return -EAGAIN;
        }

    while (rpc_poll_is_suspend) {
        pr_warn("RPCpoll: someone access rpc poll during the suspend!!!...\n");
        msleep(1000);
    }

        wmb();
        down_write(&dev->ptrSync->writeSem);

#if 1
        /* Threads that share the same file descriptor should have the same tgid
         * However, with uClibc pthread library, pthread_create() creates threads with pid == tgid
         * So the tgid is not real tgid, we have to maintain the thread list that we can lookup later
         */
        if(current->pid != proc->pid){
                RPC_PROCESS *rproc;
                list_for_each_entry(rproc, &rextra->tasks, list){
                        //find the entry in read device task list with the same pid
                        if(rproc->pid == proc->pid){
                                update_thread_list(rproc);
                                break;
                        }
                }
        }
#endif

        if (ring_empty((RPC_COMMON_Dev *)dev))
                size = 0;   // the ring is empty
        else if (dev->ringIn > dev->ringOut)
                size = dev->ringIn - dev->ringOut;
        else
                size = rpc_ring_size + dev->ringIn - dev->ringOut;

        pr_debug("%s: count:%d space:%d\n", extra->name, count, rpc_ring_size - size - 1);
        pr_debug("%s: pid:%d tgid:%d\n", extra->name, current->pid, current->tgid);

        if (count > (rpc_ring_size - size - 1))
                goto out;

        temp = dev->ringEnd - dev->ringIn;
        if (temp >= count) {
#ifdef MY_COPY
                r = my_copy_user((int *)AVCPU2SCPU(dev->ringIn), (int *)buf, count);
#else
                r = copy_from_user((int *)AVCPU2SCPU(dev->ringIn), (int *)buf, count);
#endif
                if(r) {
                        ret = -EFAULT;
                        goto out;
                }
                ret += count;
                ptmp = dev->ringIn + ((count+3) & 0xfffffffc);

                //moidfy by Angus
                asm("DSB");

                if (ptmp == dev->ringEnd)
                        dev->ringIn = dev->ringStart;
                else
                        dev->ringIn = ptmp;

                //pr_debug("RPC Write is in 1st kind...\n");
        } else {
#ifdef MY_COPY
                r = my_copy_user((int *)AVCPU2SCPU(dev->ringIn), (int *)buf, temp);
#else
                r = copy_from_user((int *)AVCPU2SCPU(dev->ringIn), (int *)buf, temp);
#endif
                if(r) {
                        ret = -EFAULT;
                        goto out;
                }
                count -= temp;

#ifdef MY_COPY
                r = my_copy_user((int *)AVCPU2SCPU(dev->ringStart), (int *)(buf+temp), count);
#else
                r = copy_from_user((int *)AVCPU2SCPU(dev->ringStart), (int *)(buf+temp), count);
#endif
                if(r) {
                        ret = -EFAULT;
                        goto out;
                }
                ret += (temp + count);

                //moidfy by Angus
                asm("DSB");

                dev->ringIn = dev->ringStart+((count+3) & 0xfffffffc);

                //pr_debug("RPC Write is in 2nd kind...\n");
        }

        //peek_rpc_struct(extra->name, dev);

out:
        pr_debug("RPC poll ringIn pointer is : 0x%8x\n", (int)AVCPU2SCPU(dev->ringIn));
        up_write(&dev->ptrSync->writeSem);
        wmb();
        return ret;
}

long rpc_poll_ioctl(/*struct inode *inode, */struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
        int ret = 0;

    while (rpc_poll_is_suspend) {
        pr_warn("RPCpoll: someone access rpc poll during the suspend!!!...\n");
        msleep(1000);
    }

        switch(cmd) {
#ifdef CONFIG_REALTEK_RPC_PROGRAM_REGISTER
        case RPC_IOCTHANDLER:
        {
                int found;
                RPC_PROCESS *proc = filp->private_data;
                RPC_HANDLER *handler;

                pr_debug("%s:%d : Register handler for programID:%lu\n", __func__, __LINE__, arg);
                found = 0;
                list_for_each_entry(handler, &proc->handlers, list){
                        if(handler->programID == arg){
                                found = 1;
                                break;
                        }
                }

                if(found) break;

                //not found, add to handler list
                handler = kmalloc(sizeof(RPC_HANDLER), GFP_KERNEL);
                if(handler == NULL){
                        pr_err("%s: failed to allocate RPC_HANDLER\n", __func__);
                        return -ENOMEM;
                }
                handler->programID = arg;
                list_add(&handler->list, &proc->handlers);
                pr_debug("%s:%d %s: Add handler pid:%d for programID:%lu\n", __func__, __LINE__, proc->extra->name, proc->pid, arg);
                break;
        }
#endif
        default:
                pr_warn("%s:%d unsupported ioctl cmd:%x arg:%lx\n", __func__, __LINE__, cmd, arg);
                return -ENOTTY;
        }
        return ret;
}

long rpc_ctrl_ioctl(/*struct inode *inode, */struct file *filp,
        unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    while (rpc_poll_is_suspend) {
        pr_warn("RPCpoll: someone access rpc poll during the suspend!!!...\n");
        msleep(1000);
    }

    switch(cmd)
    {
        case RPC_IOCTRESET:
            {
                pr_info("[RPC]start reset...\n");
                rpc_poll_init();
                rpc_intr_init();
                rpc_kern_init();

                // clear the inter-processor interrupts
                //modify by Angus
                //*((int *)REG_SB2_CPU_INT) = 0x0000007e;
                writel(RPC_INT_AS, rpc_int_base+RPC_SB2_INT);
                writel(RPC_INT_SA, rpc_int_base+RPC_SB2_INT);

                rpc_set_flag(0xffffffff);

                pr_info("[RPC]done...\n");
                break;
            }
        case RPC_IOCTRGETDBGREG_A:
            {
                RPC_DBG_FLAG dFlag;
                if (copy_from_user(&dFlag, (void __user *)arg, sizeof(dFlag)))
                    return -EFAULT;
                if (dFlag.op == RPC_DBGREG_SET)
                {
                    //pr_info("\033[0;31;31mIOCTL RPC DEBUG AUDIO op %s Value %x Addr %x \033[m\n", "SET", dFlag.flagValue, dFlag.flagAddr);
                    memcpy(DbgFlag_A, &dFlag.flagValue, sizeof(unsigned int));
                    //*DbgFlag_A = dFlag.flagValue;
                }
                else
                {
                    if(DbgFlag_A == NULL)
                    {
                        DbgFlag_A = dma_alloc_coherent(NULL, sizeof(unsigned int), &phy_DbgFlag_A, GFP_KERNEL);
                        //*DbgFlag_A = 0;
                        memset(DbgFlag_A, 0, sizeof(unsigned int));
                        pr_info("DbgFlag_A %x\n", *DbgFlag_A);
                    }
                    dFlag.flagValue = (unsigned int)*DbgFlag_A;
                    dFlag.flagAddr = phy_DbgFlag_A;
                    //pr_info("\033[0;31;31mIOCTL RPC DEBUG AUDIO op %s Value %x Addr %x \033[m\n", "GET", dFlag.flagValue, dFlag.flagAddr);

                    if (copy_to_user((void __user *)arg, &dFlag, sizeof(dFlag)))
                        return -EFAULT;
                }
                break;
            }
        default:
            pr_warn("[RPC]: error ioctl command...\n");
            break;
    }

    return ret;
}

int rpc_ctrl_open(struct inode *inode, struct file *filp)
{
    pr_info("[RPC]open for RPC ioctl...\n");

    return 0;          /* success */
}

struct file_operations rpc_poll_fops = {
    //    llseek:     scull_llseek,
    .unlocked_ioctl=    rpc_poll_ioctl,
    .read=              rpc_poll_read,
    .write=             rpc_poll_write,
    .open=              rpc_poll_open,
    .release=           rpc_poll_release,
};

struct file_operations rpc_ctrl_fops = {
    .unlocked_ioctl=    rpc_ctrl_ioctl,
    .open=              rpc_ctrl_open,
};

