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

#ifndef PSD_INTERNAL_TYPES_H_
#define PSD_INTERNAL_TYPES_H_

#include <linux/types.h>

enum psd_mcu_state
{
    psd_mcu_state_standby   = 0x00,
    psd_mcu_state_wakeup    = 0x01,
    psd_mcu_state_connected = 0x02,
    psd_mcu_state_pairing   = 0x03,
    psd_mcu_state_iap       = 0xF0,
    psd_mcu_state_reset     = 0xFF,
};

struct psd_current_mcu_state
{
    enum psd_mcu_state state;
};

struct psd_mcu_state_request
{
    enum psd_mcu_state state;
    bool is_via_reboot;
    bool is_dirty;
};

enum psd_mcu_iap_state
{
    psd_mcu_iap_state_idle           = 0x00,
    psd_mcu_iap_state_erasing        = 0x01,
    psd_mcu_iap_state_erased         = 0x02,
    psd_mcu_iap_state_writing        = 0x03,
    psd_mcu_iap_state_wrote          = 0x04,
    psd_mcu_iap_state_verifying      = 0x05,
    psd_mcu_iap_state_verified       = 0x06,
    psd_mcu_iap_state_error_crc8     = 0x07,
    psd_mcu_iap_state_error_erasing  = 0x08,
    psd_mcu_iap_state_error_writing  = 0x09,
    psd_mcu_iap_state_error_fw_size  = 0x0A,
    psd_mcu_iap_state_error_fw_crc32 = 0x0B,
};

struct psd_current_mcu_iap_state
{
    enum psd_mcu_iap_state state;
};

struct psd_charge_state
{
    u8 cable_state;
    u8 enumeration_state;
    u8 charge_state;
};

#define PSD_INTERNAL_TYPES_P_FACTOR_MAX 1000
#define PSD_INTERNAL_TYPES_I_FACTOR_MAX 1000
#define PSD_INTERNAL_TYPES_D_FACTOR_MAX 10000
#define PSD_INTERNAL_TYPES_I_MAX 10000
#define PSD_INTERNAL_TYPES_FF_MAX 1000
struct psd_feedback
{
    bool is_enable;
    u16 percent_p;
    u16 permille_i;
    u16 percent_d;
    u16 integral_max;
    u16 feedforward;
};

struct psd_drive_log
{
    bool is_enable;
};

struct psd_fail_safe
{
    bool is_motor_temp_enable;
    bool is_stall_enable;
    bool is_heart_beat_enable;
};

struct psd_settings
{
    struct psd_feedback feedback;
    struct psd_drive_log drive_log;
    struct psd_fail_safe fail_safe;
};

struct psd_mcu_clock_correction
{
    bool is_enable;
};

#endif