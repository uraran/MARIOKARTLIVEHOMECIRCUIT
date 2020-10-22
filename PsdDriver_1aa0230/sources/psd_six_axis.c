/* --------------------------------------------------------------------------
 * Sensors and Motors driver
 * Copyright (C) 2020 Nintendo Co, Ltd
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * ----------------------------------------------------------------------- */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include "psd.h"
#include "psd_param.h"
#include "psd_six_axis.h"
#include "psd_types.h"
#include "psd_util.h"

#define PSD_SIX_AXIS_DRIVER_NAME "psd_sixaxis"
static const unsigned int PSD_SIX_AXIS_MINOR  = 0;
static const unsigned int PSD_SIX_AXIS_MINOR_COUNT  = 1;
static int g_six_axis_major = 0;

struct six_axis_dev {
    struct cdev cdev;
    struct device* dev;
    struct class* class;
    int open_count;
    spinlock_t lock;
    wait_queue_head_t read_wq;
};

static struct six_axis_dev *g_six_axis_device = NULL;

static int g_six_axis_devno;

static int six_axis_device_open(struct inode *inode, struct file *file)
{
    spin_lock(&g_six_axis_device->lock);
    if(g_six_axis_device->open_count > 0)
    {
        // Single-open device
        spin_unlock(&g_six_axis_device->lock);
        return  -EBUSY;
    }
    g_six_axis_device->open_count++;
    spin_unlock(&g_six_axis_device->lock);

    return 0;
}

static int six_axis_device_release(struct inode *inode, struct file *file)
{
    spin_lock(&g_six_axis_device->lock);
    g_six_axis_device->open_count--;
    spin_unlock(&g_six_axis_device->lock);
    return 0;
}

static ssize_t six_axis_device_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct psd_six_axis* p_six_axis_buffer;
    int result;

    size_t six_axis_max_count = count / sizeof(struct psd_six_axis);

    size_t six_axis_buffer_size = six_axis_max_count * sizeof(struct psd_six_axis);
    p_six_axis_buffer = kmalloc(six_axis_buffer_size, GFP_KERNEL);

    size_t six_axis_count = psd_param_get_six_axis(p_six_axis_buffer, six_axis_max_count);

    size_t six_axis_used_size = six_axis_count * sizeof(struct psd_six_axis);

    result = copy_to_user((void __user *)buf, p_six_axis_buffer, six_axis_used_size);
    kfree(p_six_axis_buffer);

    if (result == 0)
    {
        return six_axis_used_size;
    }
    else
    {
        return -EFAULT;
    }
}
static unsigned int six_axis_device_poll(struct file *filp, poll_table *wait)
{
    poll_wait(filp, &g_six_axis_device->read_wq, wait);

    unsigned int mask = 0;
    if(psd_param_get_six_axis_count() > 0)
    {
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}


static int set_six_axis_offset(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_six_axis_offset six_axis_offset = {0};
    result = copy_from_user(&six_axis_offset, (void __user *)arg, sizeof(six_axis_offset));
    if (result == 0)
    {
        psd_param_set_six_axis_offset(&six_axis_offset);
        return sizeof(six_axis_offset);
    }
    else
    {
        return -EFAULT;
    }
}

static void psd_six_axis_data_ready(void)
{
    wake_up_interruptible(&g_six_axis_device->read_wq);
}

static long six_axis_device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch(cmd)
    {
        case PSD_SIX_AXIS_IOC_WR_OFFSET:
            return set_six_axis_offset(cmd, arg);
        default:
            PSD_LOG("Unsupported ioctl %u in %s.\n", cmd, __FUNCTION__);
            return -EINVAL;
    }
}

static struct file_operations s_six_axis_device_fops = {
    .open    = six_axis_device_open,
    .release = six_axis_device_release,
    .read    = six_axis_device_read,
    .poll    = six_axis_device_poll,
    .unlocked_ioctl = six_axis_device_ioctl,
    .compat_ioctl   = six_axis_device_ioctl,
};

int psd_six_axis_init(void)
{
    int result;
    dev_t dev = 0;

    result = alloc_chrdev_region(&dev, PSD_SIX_AXIS_MINOR, PSD_SIX_AXIS_MINOR_COUNT, PSD_SIX_AXIS_DRIVER_NAME);
    if(result)
    {
        PSD_LOG("Fail to allocate chrdev for six axis device");
        return result;
    }

    g_six_axis_device = kmalloc(sizeof(struct six_axis_dev), GFP_KERNEL);
    if(!g_six_axis_device)
    {
        result = -ENOMEM;
        goto fail;
    }
    memset(g_six_axis_device, 0, sizeof(struct six_axis_dev));
    
    spin_lock_init(&g_six_axis_device->lock);


    g_six_axis_major = MAJOR(dev);
    
    g_six_axis_devno = MKDEV(g_six_axis_major, PSD_SIX_AXIS_MINOR);

    cdev_init(&g_six_axis_device->cdev, &s_six_axis_device_fops);
    g_six_axis_device->cdev.owner = THIS_MODULE;
    init_waitqueue_head(&g_six_axis_device->read_wq);
    psd_param_set_six_axis_ready_callback(psd_six_axis_data_ready);

    result = cdev_add (&g_six_axis_device->cdev, g_six_axis_devno, 1);
    if (result)
    {
        PSD_LOG("Fail to create kernel device for six axis device\n");
        goto fail;
    }

    g_six_axis_device->class = class_create(THIS_MODULE, "psd_six_axis");
    if(!g_six_axis_device->class)
    {
        PSD_LOG("Fail to create class for six axis device\n");
        goto fail;
    }

    g_six_axis_device->dev = device_create(g_six_axis_device->class, NULL, g_six_axis_devno, NULL, "psd_six_axis");
    if(!g_six_axis_device->dev)
    {
        PSD_LOG("Fail to create user device for six axis device\n");
        goto fail;
    }

    return 0;

    fail:
    psd_six_axis_exit();
    return result;
}

void psd_six_axis_exit(void)
{
    if(g_six_axis_device->dev)
    {
        device_destroy(g_six_axis_device->class, g_six_axis_devno);
        g_six_axis_device->dev = NULL;
    }

    if(g_six_axis_device->class)
    {
        class_destroy(g_six_axis_device->class);
        g_six_axis_device->class = NULL;
    }

    if (g_six_axis_device){
        cdev_del(&g_six_axis_device->cdev);
        kfree(g_six_axis_device);
        g_six_axis_device = NULL;
    }

    unregister_chrdev_region(g_six_axis_devno, PSD_SIX_AXIS_MINOR_COUNT);
}
