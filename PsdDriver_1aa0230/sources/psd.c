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
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "psd.h"
#include "psd_battery.h"
#include "psd_gpio.h"
#include "psd_heart_beat.h"
#include "psd_param.h"
#include "psd_park.h"
#include "psd_pwm.h"
#include "psd_six_axis.h"
#include "psd_spi.h"
#include "psd_stall.h"
#include "psd_state.h"
#include "psd_syscall.h"
#include "psd_temperature.h"
#include "psd_timer.h"

MODULE_LICENSE("GPL v2");

#define DRIVER_NAME "PsdDevice"

static const unsigned int MINOR_BASE = 0;
static const unsigned int MINOR_COUNT  = 1;

static unsigned int g_device_major_num;
static struct cdev g_character_device;
static struct class* g_p_device_class = NULL;

static long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    //PSD_LOG("%s: cmd = %d\n",__FUNCTION__, cmd);
    return psd_syscall_ioctl(cmd, arg);
}

static int device_open(struct inode *inode, struct file *file)
{
    PSD_LOG("%s\n",__FUNCTION__);
    return 0;
}

static int device_close(struct inode *inode, struct file *file)
{
    PSD_LOG("%s\n",__FUNCTION__);
    return 0;
}

static void update(void)
{
    psd_state_update();
}

static ssize_t device_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    PSD_LOG("%s\n",__FUNCTION__);
    int result;
    result = psd_syscall_read(buf, count);
    return result;
}

static ssize_t device_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    PSD_LOG("%s\n",__FUNCTION__);
    int result;
    result = psd_syscall_write(buf, count);
    return count;
}

static struct file_operations s_device_fops = {
    .open    = device_open,
    .release = device_close,
    .read    = device_read,
    .write   = device_write,
    .unlocked_ioctl = device_ioctl,
    .compat_ioctl   = device_ioctl,
};

static int create_device(void)
{
    //メジャー番号の確保と、マイナー番号の予約
    int result;
    dev_t device;
    result = alloc_chrdev_region(&device, MINOR_BASE, MINOR_COUNT, DRIVER_NAME);
    if(result != 0)
    {
        PSD_LOG("alloc_chrdev_region = %d\n", result);
        return -1;
    }
    //メジャー番号のみを抽出
    g_device_major_num = MAJOR(device);
    device = MKDEV(g_device_major_num, MINOR_BASE);

    //カーネルに登録
    cdev_init(&g_character_device, &s_device_fops);
    g_character_device.owner = THIS_MODULE;
    result = cdev_add(&g_character_device, device, MINOR_COUNT);
    if(result != 0)
    {
        PSD_LOG("cdev_add = %d\n", result);
        unregister_chrdev_region(device, MINOR_COUNT);//alloc_chrdev_region()に対応
        return -1;
    }

    //デバイスファイルの生成
    g_p_device_class = class_create(THIS_MODULE, "psd");
    if(IS_ERR(g_p_device_class))
    {
        PSD_LOG("class_create\n");
        cdev_del(&g_character_device);//cdev_add()に対応
        unregister_chrdev_region(device, MINOR_COUNT);//alloc_chrdev_region()に対応
        return -1;
    }
    for(int i = MINOR_BASE; i < MINOR_BASE + MINOR_COUNT; i++)
    {
        device_create(g_p_device_class, NULL, MKDEV(g_device_major_num, i), NULL, "psd%d", i);
    }

    return 0;
}

static void remove_device(void)
{
    dev_t device = MKDEV(g_device_major_num, MINOR_BASE);
    
    //デバイスファイルの削除
    for(int i=MINOR_BASE; i < MINOR_BASE + MINOR_COUNT; ++i)
    {
        device_destroy(g_p_device_class, MKDEV(g_device_major_num, i));
    }
    class_destroy(g_p_device_class);

    //カーネルからの取り除きと、番号登録の取り消し
    cdev_del(&g_character_device);
    unregister_chrdev_region(device, MINOR_COUNT);
}

static int device_init(void)
{
    {
        struct timespec currentTimeSpec;
        currentTimeSpec = ktime_to_timespec(ktime_get());
        printk(KERN_INFO "[BootTime_PsddStart] %ld.%09ld\n", currentTimeSpec.tv_sec, currentTimeSpec.tv_nsec);
    }

    int result;

    result = create_device();
    if(result != 0)
    {
        return result;
    }

    result = psd_pwm_init();
    if(result != 0)
    {
        return result;
    }

    result = psd_spi_init();
    if(result != 0)
    {
        return result;
    }

    result = psd_gpio_init();
    if(result != 0)
    {
        return result;
    }

    result = psd_param_init();
    if(result != 0)
    {
        return result;
    }

    psd_battery_init();
    psd_park_init();
    psd_temperature_init();
    psd_stall_init(30);
    psd_heart_beat_init();
    psd_timer_init(update);
    psd_six_axis_init();

    return 0;
}

static void device_exit(void)
{
    PSD_LOG("%s\n",__FUNCTION__);

    psd_timer_exit();
    psd_six_axis_exit();
    psd_gpio_exit();
    psd_spi_exit();
    psd_pwm_exit();
    remove_device();
}

module_init(device_init);
module_exit(device_exit);
