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

#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/sched/rt.h>
#include "psd_timer.h"
#include "psd_util.h"

static const int TIMER_CYCLE_NS = 4700000;

static struct hrtimer g_hrtimer;
static struct work_struct g_work;
static void (*g_func)(void) = NULL;
struct workqueue_struct *psdtimer_wq;

/*
static void validate_periodic_work(void)
{
    const int ACCEPTABLE_FLUCTUATION_US = 500;
    const int TIMER_CYCLE_US = TIMER_CYCLE_NS / 1000;
    static u32 g_previous_time_us = 0;
    u32 time_us;
    u32 fluc_us;

    time_us = (u32)ktime_to_us(ktime_get());
    fluc_us = abs(time_us - g_previous_time_us - TIMER_CYCLE_US);
    if(fluc_us > ACCEPTABLE_FLUCTUATION_US)
    {
        PSD_LOG("%s large fluctuation found %d us\n", __FUNCTION__, fluc_us);
    }

    g_previous_time_us = time_us;
}
*/

static void workq_cb(struct work_struct* work)
{
    //validate_periodic_work();
    if(g_func != NULL)
    {
        g_func();
    }

    struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
    sched_setscheduler(current , SCHED_FIFO, &param);
}

static enum hrtimer_restart timer_cb(struct hrtimer* timer)
{
    ktime_t current_time;
    queue_work(psdtimer_wq, &g_work);
    current_time = ktime_get();
    hrtimer_forward(timer, current_time, ktime_set(0, TIMER_CYCLE_NS));
    return HRTIMER_RESTART;
}

void psd_timer_init(void (*func)(void))
{
    PSD_LOG("%s\n", __FUNCTION__);
    g_func = func;
    psdtimer_wq = create_singlethread_workqueue("psdtimer");

    INIT_WORK(&g_work, workq_cb);

    hrtimer_init(&g_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    g_hrtimer.function = timer_cb;
    hrtimer_start(&g_hrtimer, ktime_set(0, 0), HRTIMER_MODE_REL);
}

void psd_timer_exit(void)
{
    PSD_LOG("%s\n", __FUNCTION__);
    hrtimer_cancel(&g_hrtimer);
    flush_workqueue(psdtimer_wq);
    destroy_workqueue(psdtimer_wq);
}

