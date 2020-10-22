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

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include "psd_heart_beat.h"

enum alive_state
{
    alive_state_init,//g_beat_timeが有効か判定するため、initステートを設ける
    alive_state_alive,
    alive_state_dead,
};

static spinlock_t g_spinlock;
static ktime_t g_beat_time;
static bool g_is_enable;
static enum alive_state g_alive_state;

static bool locked_is_enable(void)
{
    bool is_enable;
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    is_enable = g_is_enable;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return is_enable;
}

static enum alive_state locked_get_state(void)
{
    enum alive_state state;
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    state = g_alive_state;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return state;
}

void psd_heart_beat_init(void)
{
    spin_lock_init(&g_spinlock);
    g_is_enable = false;
}

void psd_heart_beat_enable(bool is_enable)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    if(g_is_enable)
    {
        if(is_enable)
        {

        }
        else
        {

        }
    }
    else
    {
        if(is_enable)
        {
            g_alive_state = alive_state_init;
        }
        else
        {
            
        }
    }
    g_is_enable = is_enable;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

void psd_heart_beat_update(void)
{
    if(!locked_is_enable())
    {
        return;
    }

    if(locked_get_state() == alive_state_dead)
    {
        //一度dead判定されているので、状態を変えない
        return;
    }

    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_alive_state = alive_state_alive;
    g_beat_time = ktime_get();
    spin_unlock_irqrestore(&g_spinlock, flags);
}

bool psd_heart_beat_is_alive(void)
{
    if(!locked_is_enable())
    {
        return true;
    }

    switch(locked_get_state())
    {
        case alive_state_init:
            return true;
        case alive_state_alive:
            break;
        case alive_state_dead:
            return false;
        default:
            return false;
    }

    //updateからの時間を計算し、規定以上だった場合は、ステートを変える
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);

    static ktime_t interval_time;
    interval_time = ktime_sub(ktime_get(), g_beat_time);
    static int IntervalTimeUsMax = 1000000;
    //PSD_LOG("interval time = %lld\n", ktime_to_us(interval_time));
    if(ktime_to_us(interval_time) > IntervalTimeUsMax)
    {
        printk(KERN_ERR "heart beat time out. app to driver.\n");
        g_alive_state = alive_state_dead;
    }
    spin_unlock_irqrestore(&g_spinlock, flags);

    //false判定は、次回の確認時に行う
    return true;
}
