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

#ifndef PSD_PARAM_H_
#define PSD_PARAM_H_

#include "psd_internal_types.h"
#include "psd_types.h"

int psd_param_init(void);

struct psd_button psd_param_get_button(void);
void psd_param_set_button(const struct psd_button* p_button);

struct psd_charge_brake_led psd_param_get_charge_brake_led(void);
void psd_param_set_charge_brake_led(const struct psd_charge_brake_led* p_charge_brake_led);

struct psd_charge_state psd_param_get_charge_state(void);
void psd_param_set_charge_state(const struct psd_charge_state* p_charge_state);

struct psd_drive_control psd_param_get_drive_control(void);
void psd_param_set_drive_control(const struct psd_drive_control* p_drive_control);

struct psd_drive_trim psd_param_get_drive_trim(void);
void psd_param_set_drive_trim(const struct psd_drive_trim* p_drive_trim);

struct psd_power_sync_led psd_param_get_power_sync_led(void);
void psd_param_set_current_mcu_state(const struct psd_current_mcu_state* p_current_mcu_state);
struct psd_current_mcu_state psd_param_get_current_mcu_state(void);

void psd_param_set_current_mcu_iap_state(const struct psd_current_mcu_iap_state* p_current_mcu_iap_state);
struct psd_current_mcu_iap_state psd_param_get_current_mcu_iap_state(void);

struct psd_mcu_clock_correction psd_param_get_mcu_clock_correction(void);
void psd_param_set_clock_correction(const struct psd_mcu_clock_correction* p_mcu_clock_correction);

struct psd_mcu_state_request psd_param_peek_mcu_state_request(void);
struct psd_mcu_state_request psd_param_pop_mcu_state_request(void);
void psd_param_set_power_sync_led(const struct psd_power_sync_led* p_power_sync_led);
void psd_param_set_shutdown(const struct psd_shutdown* p_shutdown);
void psd_param_set_iap(void);

struct psd_mcu_fw_version psd_param_get_mcu_fw_version(void);
void psd_param_set_mcu_fw_version(const struct psd_mcu_fw_version* p_mcu_fw_version);

size_t psd_param_get_six_axis(struct psd_six_axis* p_six_axis_buffer, size_t max_count);
size_t psd_param_get_six_axis_count(void);
void psd_param_set_six_axis_ready_callback(void (*callback)(void));
void psd_param_add_six_axis(const struct psd_six_axis six_axis[], int count);

struct psd_six_axis_offset psd_param_get_six_axis_offset(void);
void psd_param_set_six_axis_offset(const struct psd_six_axis_offset* p_six_axis_offset);

struct psd_voltage psd_param_get_voltage(void);
void psd_param_set_voltage(const struct psd_voltage* p_voltage);

struct psd_feedback psd_param_get_feedback_setting(void);
void psd_param_set_feedback_setting(const struct psd_feedback* p_feedback);

struct psd_drive_log psd_param_get_drive_log_setting(void);
void psd_param_set_drive_log_setting(const struct psd_drive_log* p_drive_log);

struct psd_fail_safe psd_param_get_fail_safe_setting(void);
void psd_param_set_fail_safe_setting(const struct psd_fail_safe* p_fail_safe);
bool psd_param_is_heart_beat_fail_safe_setting_enable(void);
void psd_param_set_heart_beat_fail_safe_setting(bool is_enable);

struct psd_init_state psd_param_get_init_state(void);
void psd_param_set_init_state(const struct psd_init_state* p_init_state);

#endif
