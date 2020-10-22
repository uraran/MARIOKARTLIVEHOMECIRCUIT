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
#include "psd_park.h"
#include "psd_physics.h"
#include "psd_util.h"

static spinlock_t g_spinlock;
static s8 g_throttle;
static bool g_is_throttle_stop;

void psd_park_init(void)
{
    spin_lock_init(&g_spinlock);
    g_is_throttle_stop = false;
}

void psd_park_update(bool is_throttle_stop, s8 throttle)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_throttle = throttle;
    g_is_throttle_stop = is_throttle_stop;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

bool psd_park_is_parking(void)
{
    s8 throttle;
    bool is_throttle_stop;
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    throttle = g_throttle;
    is_throttle_stop = g_is_throttle_stop;
    spin_unlock_irqrestore(&g_spinlock, flags);

    if(is_throttle_stop)
    {
        return true;
    }
    else
    {
        if(abs(throttle) > 30)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
}
