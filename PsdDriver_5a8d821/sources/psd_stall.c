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

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include "psd_physics.h"
#include "psd_stall.h"
#include "psd_util.h"

static spinlock_t g_spinlock;
static int g_stall_count;
static int g_stall_safe_count;
static bool g_is_safe;

void psd_stall_init(int safe_count)
{
    spin_lock_init(&g_spinlock);
    g_stall_count = 0;
    g_stall_safe_count = safe_count;
    g_is_safe = true;
}

void psd_stall_update(struct psd_util_ratio duty)
{
    u8 stall_rate;
    stall_rate = psd_physics_get_throttle_stall_rate(duty);

    const int SafeRate = 44;
    if(stall_rate > SafeRate)
    {
        unsigned long flags;
        spin_lock_irqsave(&g_spinlock, flags);
        g_stall_count++;
        if(g_stall_count > g_stall_safe_count)
        {
            g_is_safe = false;
        }
        spin_unlock_irqrestore(&g_spinlock, flags);
    }
    else
    {
        unsigned long flags;
        spin_lock_irqsave(&g_spinlock, flags);
        g_stall_count = 0;
        spin_unlock_irqrestore(&g_spinlock, flags);
    }
}

void psd_stall_release(void)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_is_safe = true;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

bool psd_stall_is_safe(void)
{
    bool is_safe;
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    is_safe = g_is_safe;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return is_safe;
}
