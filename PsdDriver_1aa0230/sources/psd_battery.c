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
#include "psd_battery.h"
#include "psd_internal_types.h"
#include "psd_param.h"
#include "psd_physics.h"
#include "psd_types.h"
#include "psd_util.h"

#define PSD_BATTERY_SAMPLE_COUNT_MAX 60

static spinlock_t g_spinlock;
static int g_counter;//次に追加するindex。また、追加済みの数
static u16 g_stop_cv[PSD_BATTERY_SAMPLE_COUNT_MAX];
static int g_battery_level;

static int get_current_counter(void)
{
    return g_counter % PSD_BATTERY_SAMPLE_COUNT_MAX;
}

static int get_count(void)
{
    if(g_counter > PSD_BATTERY_SAMPLE_COUNT_MAX)
    {
        return PSD_BATTERY_SAMPLE_COUNT_MAX;
    }
    else
    {
        return g_counter;
    }
}

static bool is_count_full(void)
{
    if(g_counter >= PSD_BATTERY_SAMPLE_COUNT_MAX)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void increment_counter(void)
{
    ++g_counter;
}

static u16 calculate_stop_cv_ave(int count)
{
    int sum = 0;
    for(int i=0;i<count;i++)
    {
        sum += g_stop_cv[i];
    }
    return sum / count;
}

static int convert_to_battery_level(u16 cv)
{
    const int level_four_cv  = 386;
    const int level_three_cv = 372;
    const int level_two_cv   = 361;
    const int level_one_cv   = 330;

    if(cv >= level_four_cv)
    {
        return 4;
    }
    else if(cv >= level_three_cv)
    {
        return 3;
    }
    else if(cv >= level_two_cv)
    {
        return 2;
    }
    else if(cv >= level_one_cv)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static void update_battery_level(int count)
{
    if(is_count_full())
    {

    }
    else
    {
        //データが揃っていないので、更新はしない
        return;
    }
    
    int battery_level;
    battery_level = convert_to_battery_level(calculate_stop_cv_ave(count));

    struct psd_charge_state charge_state;
    charge_state = psd_param_get_charge_state();
    if(charge_state.cable_state == 1)
    {
        g_battery_level = battery_level;
    }
    else
    {
        //充電していない時は、下降方向の遷移のみ
        if(battery_level >= g_battery_level)
        {

        }
        else
        {
            g_battery_level = battery_level;
        }
    }
}

static u16 calculate_stop_cv(u16 bat_cv, u16 mot_ma)
{
    //Vbat + 0.12 * Imot
    u16 additional_cv = psd_exponential_round(12 * mot_ma, 3);
    return bat_cv + additional_cv;
}

static void set_stop_cv(int index)
{    
    struct psd_util_ratio duty;
    duty = psd_util_ratio(1,1);
    const u16 mot_ma = psd_physics_calculate_throttle_current(duty);
    const u16 bat_cv = psd_physics_get_battery_cv();

    g_stop_cv[index] = calculate_stop_cv(bat_cv, mot_ma);
}

void psd_battery_init(void)
{
    spin_lock_init(&g_spinlock);
    g_counter = 0;
    g_battery_level = 4;
}

void psd_battery_update(void)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    
    set_stop_cv(get_current_counter());
    increment_counter();

    update_battery_level(get_count());
    
    spin_unlock_irqrestore(&g_spinlock, flags);
}

int psd_battery_get_level(void)
{
    int battery_level;
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    battery_level = g_battery_level;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return battery_level;
}
