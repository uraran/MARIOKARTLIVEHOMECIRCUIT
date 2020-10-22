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
#include "psd_param.h"
#include "psd_types.h"
#include "psd_util.h"

#define PSD_DEVIATION_HISTORY_COUNT_MAX 10
struct deviation_history
{
    s16 actual_angle[PSD_DEVIATION_HISTORY_COUNT_MAX];
    s16 target_angle[PSD_DEVIATION_HISTORY_COUNT_MAX];
    u16 count;
    u16 counter;
};

static struct deviation_history g_dh;
static s16 g_deviation_sum = 0;

static u16 get_current_index(void)
{
    if(g_dh.counter == 0)
    {
        if(g_dh.count == 0)
        {
            return 0;//データなし
        }
        else
        {
            return PSD_DEVIATION_HISTORY_COUNT_MAX - 1;
        }
    }
    else
    {
        return g_dh.counter - 1;
    }
}

static u16 get_previous_index(void)
{
    u16 current;
    current = get_current_index();
    if(current != 0)
    {
        return current - 1;
    }
    else if(g_dh.count == PSD_DEVIATION_HISTORY_COUNT_MAX)
    {
        return PSD_DEVIATION_HISTORY_COUNT_MAX - 1;
    }
    else
    {
        return 0;
    }
}

static void update_count_counter(void)
{
    if(g_dh.count < PSD_DEVIATION_HISTORY_COUNT_MAX)
    {
        g_dh.count++;
    }

    if(g_dh.counter < PSD_DEVIATION_HISTORY_COUNT_MAX - 1)
    {
        g_dh.counter++;
    }
    else
    {
        g_dh.counter = 0;
    }
}

static s16 get_deviation(u16 index)
{
    return g_dh.target_angle[index] - g_dh.actual_angle[index];
}

static void update_sum(void)
{
    struct psd_feedback feedback;
    feedback = psd_param_get_feedback_setting();

    g_deviation_sum += get_deviation(get_current_index());
    g_deviation_sum = psd_util_minmax(-feedback.integral_max, g_deviation_sum, feedback.integral_max);
}

void psd_deviation_update(s16 current_angle, s16 target_angle)
{
    g_dh.actual_angle[g_dh.counter] = current_angle;
    g_dh.target_angle[g_dh.counter] = target_angle;
    update_count_counter();
    update_sum();
}

s16 psd_deviation_get_sum(void)
{
    return g_deviation_sum;
}

s16 psd_deviation_get_current(void)
{
    return get_deviation(get_current_index());
}

s16 psd_deviation_get_sub(void)
{
    s16 current;
    s16 previous;
    current = get_deviation(get_current_index());
    previous = get_deviation(get_previous_index());
    return current - previous;
}
