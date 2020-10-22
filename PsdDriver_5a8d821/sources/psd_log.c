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
#include "psd_internal_types.h"
#include "psd_log.h"
#include "psd_param.h"
#include "psd_util.h"

static int g_steering_control;
static int g_steering_manipulation;
static int g_steering_duty;
static int g_steering_intensity_oc;
static int g_throttle_control;
static int g_throttle_manipulation;
static int g_throttle_duty;
static int g_throttle_intensity_oc;
static int g_steering_angle;
static int g_batv;

void psd_log_set(enum psd_log_id id, int value)
{
    switch(id)
    {
        case psd_log_id_steering_control:
            g_steering_control = value;
            return;
        case psd_log_id_steering_manipulation:
            g_steering_manipulation = value;
            return;
        case psd_log_id_steering_duty:
            g_steering_duty = value;
            return;
        case psd_log_id_steering_intensity_oc:
            g_steering_intensity_oc = value;
            return;
        case psd_log_id_throttle_control:
            g_throttle_control = value;
            return;
        case psd_log_id_throttle_manipulation:
            g_throttle_manipulation = value;
            return;
        case psd_log_id_throttle_duty:
            g_throttle_duty = value;
            return;
        case psd_log_id_throttle_intensity_oc:
            g_throttle_intensity_oc = value;
            return;
        case psd_log_id_steering_angle:
            g_steering_angle = value;
            return;
        case psd_log_id_batv:
            g_batv = value;
            return;
        default:
            return;
    }
}

void psd_log_flush(void)
{
    struct psd_drive_log drive_log;
    drive_log = psd_param_get_drive_log_setting();

    if(drive_log.is_enable)
    {
        PSD_LOG("sc %d sm %d sd %d si %d tc %d tm %d td %d ti %d sa %d bv %d\n",
        g_steering_control,
        g_steering_manipulation,
        g_steering_duty,
        g_steering_intensity_oc,
        g_throttle_control,
        g_throttle_manipulation,
        g_throttle_duty,
        g_throttle_intensity_oc,
        g_steering_angle,
        g_batv
        );
    }
}
