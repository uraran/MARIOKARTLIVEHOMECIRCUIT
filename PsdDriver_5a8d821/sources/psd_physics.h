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

#ifndef PSD_PHYSICS_H_
#define PSD_PHYSICS_H_

#include <linux/types.h>
#include "psd_util.h"

s16 psd_physics_get_target_angle(s8 steering);//単位はデシ度
s16 psd_physics_get_current_angle(void);
s16 psd_physics_get_offset_angle(s8 trim);

u16 psd_physics_get_battery_cv(void);//単位はcV。0～300程

int psd_physics_calculate_steering_duty(struct psd_util_ratio* p_ratio, s8 steering, s8 trim);
int psd_physics_calculate_spring_steering_duty(struct psd_util_ratio* p_ratio, s8 steering);
int psd_physics_calculate_throttle_duty(struct psd_util_ratio* p_ratio, s8 throttle, u8 trim);

u16 psd_physics_calculate_steering_current(struct psd_util_ratio duty);//単位はmA
u16 psd_physics_calculate_throttle_current(struct psd_util_ratio duty);//単位はmA。0～2000程

u16 psd_physics_get_throttle_torque(struct psd_util_ratio duty);//単位はuNm。0～5800程
u32 psd_physics_get_throttle_rpm(struct psd_util_ratio duty);//単位は 回転/分。0～8300程
int psd_physics_get_throttle_heat_change(struct psd_util_ratio duty);//ミリ単位
u8 psd_physics_get_throttle_stall_rate(struct psd_util_ratio duty);//単位はパーセント

#endif