/*
 * $Id: RPCkern.c,v 1.10 2004/8/4 09:25 Jacky Exp $
 */
#include <generated/autoconf.h>

#include <linux/module.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>     /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/ioctl.h>    /* needed for the _IOW etc stuff used later */
#include <linux/sched.h>
#include <linux/freezer.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#include <asm/io.h>
#include <asm/system.h>     /* cli(), *_flags */
#include <asm/uaccess.h>    /* copy_to_user() copy_from_user() */

#include "RPCDriver.h"
#include "debug.h"

#define TIMEOUT 5*HZ

static struct radix_tree_root kernel_rpc_tree = RADIX_TREE_INIT(GFP_ATOMIC);
//static struct semaphore kernel_rpc_sem;
DECLARE_RWSEM(kernel_rpc_sem);

//static DECLARE_MUTEX(kernel_rpc_sem);

RPC_KERN_Dev *rpc_kern_devices;
struct task_struct *rpc_kthread[RPC_NR_KERN_DEVS/RPC_NR_PAIR] = {0};
static wait_queue_head_t rpc_wq[RPC_NR_KERN_DEVS/RPC_NR_PAIR];
int rpc_kern_is_paused = 0;
int rpc_kern_is_suspend = 0;
static int complete_condition[RPC_NR_KERN_DEVS/RPC_NR_PAIR];
static struct mutex rpc_kern_lock[RPC_NR_KERN_DEVS/RPC_NR_PAIR];
static int rpc_kernel_thread(void *p);

int rpc_kern_init(void)
{
    static int is_init = 0;
    int result = 0, num, i;

    // Create corresponding structures for each device.
    rpc_kern_devices = (RPC_KERN_Dev *)AVCPU2SCPU(RPC_KERN_RECORD_ADDR);

    num = RPC_NR_KERN_DEVS;
    for (i = 0; i < num; i++) {
        pr_debug("rpc_kern_device %d addr: %x\n", i, (unsigned)&rpc_kern_devices[i]);
        rpc_kern_devices[i].ringBuf = (char *)(RPC_KERN_DEV_ADDR + i*RPC_RING_SIZE);

        // Initialize pointers...
        rpc_kern_devices[i].ringStart = rpc_kern_devices[i].ringBuf;
        rpc_kern_devices[i].ringEnd = rpc_kern_devices[i].ringBuf+RPC_RING_SIZE;
        rpc_kern_devices[i].ringIn = rpc_kern_devices[i].ringBuf;
        rpc_kern_devices[i].ringOut = rpc_kern_devices[i].ringBuf;

        pr_debug("The %dth RPC_KERN_Dev:\n", i);
        pr_debug("RPC ringStart: 0x%8x\n", (int)AVCPU2SCPU(rpc_kern_devices[i].ringStart));
        pr_debug("RPC ringEnd:   0x%8x\n", (int)AVCPU2SCPU(rpc_kern_devices[i].ringEnd));
        pr_debug("RPC ringIn:    0x%8x\n", (int)AVCPU2SCPU(rpc_kern_devices[i].ringIn));
        pr_debug("RPC ringOut:   0x%8x\n", (int)AVCPU2SCPU(rpc_kern_devices[i].ringOut));
        pr_debug("\n");

        if (!is_init) {
            rpc_kern_devices[i].ptrSync = kmalloc(sizeof(RPC_SYNC_Struct), GFP_KERNEL);

            // Initialize wait queue...
            init_waitqueue_head(&(rpc_kern_devices[i].ptrSync->waitQueue));

            // Initialize sempahores...
            init_rwsem(&rpc_kern_devices[i].ptrSync->readSem);
            init_rwsem(&rpc_kern_devices[i].ptrSync->writeSem);
        }

        if (i%RPC_NR_PAIR == 1) {
            if (rpc_kthread[i/RPC_NR_PAIR] == 0)
                rpc_kthread[i/RPC_NR_PAIR] = kthread_run(rpc_kernel_thread, (void *)i, "rpc-%d", i);
        }
    }
    if (!is_init) {
        for (i = 0; i < RPC_NR_KERN_DEVS/RPC_NR_PAIR; i++)
        {
            init_waitqueue_head(&(rpc_wq[i]));
            mutex_init(&rpc_kern_lock[i]);
        }
    }
    is_init = 1;
    rpc_kern_is_paused = 0;
    rpc_kern_is_suspend = 0;

    return result;
}

int rpc_kern_pause(void)
{
    rpc_kern_is_paused = 1;

    return 0;
}

int rpc_kern_suspend(void)
{
    rpc_kern_is_suspend = 1;
    return 0;
}

int rpc_kern_resume(void)
{
    rpc_kern_is_suspend = 0;
    return 0;
}

ssize_t rpc_kern_read(int opt, char *buf, size_t count)
{
    RPC_KERN_Dev *dev;
    int temp, size;
    size_t r;
    ssize_t ret = 0;
    char *ptmp;

    dev = &rpc_kern_devices[opt*RPC_NR_PAIR+1];
    pr_debug("read rpc_kern_device: %x\n", (unsigned int)dev);
    down_write(&dev->ptrSync->readSem);

    if (dev->ringIn == dev->ringOut)
        goto out;   // the ring is empty...
    else if (dev->ringIn > dev->ringOut)
        size = dev->ringIn - dev->ringOut;
    else
        size = RPC_RING_SIZE + dev->ringIn - dev->ringOut;

    if (count > size)
        count = size;

    temp = dev->ringEnd - dev->ringOut;
    if (temp >= count) {
#ifdef MY_COPY
        r = my_copy_user((int *)buf, (int *)AVCPU2SCPU(dev->ringOut), count);
#else
        r = ((int *)buf != memcpy((int *)buf, (int *)AVCPU2SCPU(dev->ringOut), count));
#endif
        if(r) {
            ret = -EFAULT;
            goto out;
        }
        ret += count;
        ptmp = dev->ringOut + ((count+3) & 0xfffffffc);
        if (ptmp == dev->ringEnd)
            dev->ringOut = dev->ringStart;
        else
            dev->ringOut = ptmp;

        pr_debug("RPC Read is in 1st kind...\n");
    } else {
#ifdef MY_COPY
        r = my_copy_user((int *)buf, (int *)AVCPU2SCPU(dev->ringOut), temp);
#else
        r = ((int *)buf != memcpy((int *)buf, (int *)AVCPU2SCPU(dev->ringOut), temp));
#endif
        if(r) {
            ret = -EFAULT;
            goto out;
        }
        count -= temp;

#ifdef MY_COPY
        r = my_copy_user((int *)(buf+temp), (int *)AVCPU2SCPU(dev->ringStart), count);
#else
        r = ((int *)(buf+temp) != memcpy((int *)(buf+temp), (int *)AVCPU2SCPU(dev->ringStart), count));
#endif
        if(r) {
            ret = -EFAULT;
            goto out;
        }
        ret += (temp + count);
        dev->ringOut = dev->ringStart+((count+3) & 0xfffffffc);

        pr_debug("RPC Read is in 2nd kind...\n");
    }
out:
    pr_debug("RPC kern ringOut pointer is : 0x%8x\n", (int)AVCPU2SCPU(dev->ringOut));
    up_write(&dev->ptrSync->readSem);
    return ret;
}

ssize_t rpc_kern_write(int opt, const char *buf, size_t count)
{
    RPC_KERN_Dev *dev;
    int temp, size;
    size_t r;
    ssize_t ret = 0;
    char *ptmp;

    dev = &rpc_kern_devices[opt*RPC_NR_PAIR];
    pr_debug("write rpc_kern_device: %x\n", (unsigned int)dev);
    pr_debug("[rpc_kern_write] write rpc_kern_device: caller%x, *buf:0x%x\n", (unsigned int) __read_32bit_caller_register(), *(unsigned int *)buf);
    down_write(&dev->ptrSync->writeSem);

    if (dev->ringIn == dev->ringOut)
        size = 0;   // the ring is empty
    else if (dev->ringIn > dev->ringOut)
        size = dev->ringIn - dev->ringOut;
    else
        size = RPC_RING_SIZE + dev->ringIn - dev->ringOut;

    if (count > (RPC_RING_SIZE - size - 1))
        goto out;

    temp = dev->ringEnd - dev->ringIn;
    if (temp >= count) {
#ifdef MY_COPY
        r = my_copy_user((int *)AVCPU2SCPU(dev->ringIn), (int *)buf, count);
#else
        r = ((int *)AVCPU2SCPU(dev->ringIn) != memcpy((int *)AVCPU2SCPU(dev->ringIn), (int *)buf, count));
#endif
        if(r) {
            ret = -EFAULT;
            goto out;
        }
        ret += count;
        ptmp = dev->ringIn + ((count+3) & 0xfffffffc);

        //modify by Angus
        asm("DSB");

        if (ptmp == dev->ringEnd)
            dev->ringIn = dev->ringStart;
        else
            dev->ringIn = ptmp;

        pr_debug("RPC Write is in 1st kind...\n");
    } else {
#ifdef MY_COPY
        r = my_copy_user((int *)AVCPU2SCPU(dev->ringIn), (int *)buf, temp);
#else
        r = ((int *)AVCPU2SCPU(dev->ringIn) != memcpy((int *)AVCPU2SCPU(dev->ringIn), (int *)buf, temp));
#endif
        if(r) {
            ret = -EFAULT;
            goto out;
        }
        count -= temp;

#ifdef MY_COPY
        r = my_copy_user((int *)AVCPU2SCPU(dev->ringStart), (int *)(buf+temp), count);
#else
        r = ((int *)AVCPU2SCPU(dev->ringStart) != memcpy((int *)AVCPU2SCPU(dev->ringStart), (int *)(buf+temp), count));
#endif
        if(r) {
            ret = -EFAULT;
            goto out;
        }
        ret += (temp + count);

        //modify by Angus
        asm("DSB");

        dev->ringIn = dev->ringStart+((count+3) & 0xfffffffc);

        pr_debug("RPC Write is in 2nd kind...\n");
    }

    //if (opt == RPC_AUDIO)
    if (opt == RPC_AUDIO || opt == RPC_VIDEO)
        writel((RPC_INT_SA | RPC_INT_WRITE_1), rpc_int_base+RPC_SB2_INT);        // audio
        //rtd_outl(REG_SB2_CPU_INT, (RPC_INT_SA | RPC_INT_WRITE_1));
    else
        pr_err("error device number...\n");

out:
    pr_debug("RPC kern ringIn pointer is : 0x%8x\n", (int)AVCPU2SCPU(dev->ringIn));
    up_write(&dev->ptrSync->writeSem);
    return ret;
}

int register_kernel_rpc(unsigned long command, FUNC_PTR ptr)
{
    int error;

    error = radix_tree_preload(GFP_KERNEL);
    if (error == 0) {
        down_write(&kernel_rpc_sem);
        error = radix_tree_insert(&kernel_rpc_tree, command, (void *)ptr);
        if (error)
            pr_err("RPC: register kernel rpc %ld error...\n", command);
        up_write(&kernel_rpc_sem);
        radix_tree_preload_end();
    }

    return error;
}

unsigned long handle_command(unsigned long command, unsigned long param1, unsigned long param2)
{
    FUNC_PTR ptr;
    int ret = 0;

    pr_info("Handle command %lx, param1: %lx, param2: %lx...\n", command, param1, param2);
    down_write(&kernel_rpc_sem);
    ptr = radix_tree_lookup(&kernel_rpc_tree, command);
    if (ptr)
        ret = ptr(param1, param2);
    else
        pr_err("RPC: lookup kernel rpc %ld error...\n", command);
    up_write(&kernel_rpc_sem);

    return ret;
}

static int rpc_kernel_thread(void *p)
{
    char readbuf[sizeof(RPC_STRUCT)+3*sizeof(unsigned long)];
    RPC_KERN_Dev *dev;
    RPC_STRUCT *rpc;
    unsigned long *tmp;

//  daemonize(current->comm);

    dev = &rpc_kern_devices[(int)p];
    while (1) {
//      if (current->flags & PF_FREEZE)
//          refrigerator(PF_FREEZE);
        //try_to_freeze();

//      pr_info(" #@# wait %s %x %x \n", current->comm, dev, dev->waitQueue);
        if (wait_event_interruptible(dev->ptrSync->waitQueue, dev->ringIn != dev->ringOut)) {
            pr_notice("%s got signal or should stop...\n", current->comm);
            continue;
        }
//      pr_info(" #@# wakeup %s \n", current->comm);

        if (kthread_should_stop()) {
            pr_notice("%s exit...\n", current->comm);
            break;
        }

        // read the reply data...
        if (rpc_kern_read(((int)p)/RPC_NR_PAIR, readbuf, sizeof(RPC_STRUCT)) != sizeof(RPC_STRUCT)) {
            pr_err("ERROR in read kernel RPC...\n");
            continue;
        }

        rpc = (RPC_STRUCT *)readbuf;
        tmp = (unsigned long *)(readbuf+sizeof(RPC_STRUCT));
        if (rpc->taskID) {
            // handle the request...
            char replybuf[sizeof(RPC_STRUCT)+2*sizeof(unsigned long)];
            unsigned long ret;
            RPC_STRUCT *rrpc = (RPC_STRUCT *)replybuf;

            // read the payload...
            if (rpc_kern_read(((int)p)/RPC_NR_PAIR, readbuf+sizeof(RPC_STRUCT), 3*sizeof(unsigned long))
                    != 3*sizeof(unsigned long)) {
                pr_err("ERROR in read payload...\n");
                continue;
            }

            ret = handle_command(ntohl(*tmp), ntohl(*(tmp+1)), ntohl(*(tmp+2)));

            // fill the RPC_STRUCT...
            rrpc->programID = htonl(REPLYID);
            rrpc->versionID = htonl(REPLYID);
            rrpc->procedureID = 0;
            rrpc->taskID = 0;
#ifdef RPC_SUPPORT_MULTI_CALLER_SEND_TID_PID
            rrpc->sysTID = 0;
#endif
            rrpc->sysPID = 0;
            rrpc->parameterSize = htonl(2*sizeof(unsigned long));
            rrpc->mycontext = rpc->mycontext;

            // fill the parameters...
            tmp = (unsigned long *)(replybuf+sizeof(RPC_STRUCT));
            *(tmp+0) = rpc->taskID;
            *(tmp+1) = htonl(ret);

            if (rpc_kern_write(((int)p)/RPC_NR_PAIR, replybuf, sizeof(replybuf)) != sizeof(replybuf)) {
                pr_err("ERROR in send kernel RPC...\n");
                return RPC_FAIL;
            }
        } else {
            // read the payload...
            if (rpc_kern_read(((int)p)/RPC_NR_PAIR, readbuf+sizeof(RPC_STRUCT), 2*sizeof(unsigned long))
                    != 2*sizeof(unsigned long)) {
                pr_err("ERROR in read payload...\n");
                continue;
            }

            // parse the reply data...
            *((unsigned long *)ntohl(rpc->mycontext)) = ntohl(*(tmp+1));
            //pr_info("tmp %x opt %d\n", ntohl(*tmp), (int)p/RPC_NR_PAIR);
            complete_condition[((int)p/RPC_NR_PAIR)] = 1;
            wake_up((wait_queue_head_t *)ntohl(*tmp)); // ack the sync...
           
        }
    }

    return 0;
}

int send_rpc_command(int opt, unsigned long command, unsigned long param1, unsigned long param2, unsigned long *retvalue)
{
    char sendbuf[sizeof(RPC_STRUCT)+3*sizeof(unsigned long)];
    RPC_STRUCT *rpc = (RPC_STRUCT *)sendbuf;
    unsigned long *tmp;

    if (rpc_kern_is_paused) {
        pr_warn("RPCkern: someone access rpc kern during the pause...\n");
        return RPC_FAIL;
    }

    mutex_lock(&rpc_kern_lock[opt]);

    while (rpc_kern_is_suspend) {
        pr_warn("RPCkern: someone access rpc poll during the suspend!!!...\n");
        msleep(1000);
    }

    if (rpc_kthread[opt] == 0) {
        pr_warn("RPCkern: %s is disabled...\n", rpc_kthread[opt]->comm);
        mutex_unlock(&rpc_kern_lock[opt]);
        return RPC_FAIL;
    }
 
    //pr_info(" #@# sendbuf: %d cmd %x param1 %x param2 %x\n", sizeof(sendbuf), command, param1, param2);
    // fill the RPC_STRUCT...
    rpc->programID = htonl(KERNELID);
    rpc->versionID = htonl(KERNELID);
    rpc->procedureID = 0;
    rpc->taskID = htonl((unsigned long)&rpc_wq[opt]);
#ifdef RPC_SUPPORT_MULTI_CALLER_SEND_TID_PID
    rpc->sysTID = 0;
#endif
    rpc->sysPID = 0;
    rpc->parameterSize = htonl(3*sizeof(unsigned long));
    rpc->mycontext = htonl((unsigned long)retvalue);

    // fill the parameters...
    tmp = (unsigned long *)(sendbuf+sizeof(RPC_STRUCT));
//  pr_info(" aaa: %x bbb: %x \n", sendbuf, tmp);
    *tmp = htonl(command);
    *(tmp+1) = htonl(param1);
    *(tmp+2) = htonl(param2);

    complete_condition[opt] = 0;
    if (rpc_kern_write(opt, sendbuf, sizeof(sendbuf)) != sizeof(sendbuf)) {
        pr_err("ERROR in send kernel RPC...\n");
        mutex_unlock(&rpc_kern_lock[opt]);
        return RPC_FAIL;
    }

    // wait the result...
    //if (!sleep_on_timeout(&rpc_wq[opt], TIMEOUT)) {
    if (!wait_event_timeout(rpc_wq[opt], complete_condition[opt], TIMEOUT)) {
        pr_err("kernel rpc timeout -> disable %s...\n", rpc_kthread[opt]->comm);
        WARN(1, " #@# sendbuf: size%d cmd:%lx param1:%lx param2:%lx\n", sizeof(sendbuf), command, param1, param2);

        kthread_stop(rpc_kthread[opt]);
        rpc_kthread[opt] = 0;
        mutex_unlock(&rpc_kern_lock[opt]);
        return RPC_FAIL;
    } else {
//      pr_info(" #@# ret: %d \n", *retvalue);
        mutex_unlock(&rpc_kern_lock[opt]);
        return RPC_OK;
    }
}

EXPORT_SYMBOL(send_rpc_command);

