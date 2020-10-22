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
#include "psd_temperature.h"
#include "psd_util.h"

static spinlock_t g_spinlock;
static int g_temperature;
static bool g_is_safe;
static const int SafeLimit      = 6894000;
static const int CoolDownTarget = 3447000;

void psd_temperature_init(void)
{
    spin_lock_init(&g_spinlock);
    g_temperature = 0;
    g_is_safe = true;
}

void psd_temperature_update(struct psd_util_ratio duty)
{
    int change = psd_physics_get_throttle_heat_change(duty);
    unsigned long flags;

    spin_lock_irqsave(&g_spinlock, flags);
    g_temperature += change;
    g_temperature = psd_util_max(g_temperature, 0);

    if(g_is_safe)
    {
        if(g_temperature > SafeLimit)
        {
            g_is_safe = false;
        }
    }
    else
    {
        if(g_temperature < CoolDownTarget)
        {
            g_is_safe = true;
        }
    }

    spin_unlock_irqrestore(&g_spinlock, flags);
}

int psd_temperature_get_raw(void)
{
    int temperature;
    unsigned long flags;

    spin_lock_irqsave(&g_spinlock, flags);
    temperature = g_temperature;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return temperature;
}

int psd_temperature_get_percent(void)
{
    int temperature;
    unsigned long flags;

    spin_lock_irqsave(&g_spinlock, flags);
    temperature = g_temperature;
    spin_unlock_irqrestore(&g_spinlock, flags);

    temperature = psd_util_min(temperature, SafeLimit);
    temperature /= 1000;
    int limit = SafeLimit / 1000;

    return psd_util_percent(temperature, limit);
}

bool psd_temperature_is_safe(void)
{
    bool is_safe;
    unsigned long flags;

    spin_lock_irqsave(&g_spinlock, flags);
    is_safe = g_is_safe;
    spin_unlock_irqrestore(&g_spinlock, flags);

    return is_safe;
}