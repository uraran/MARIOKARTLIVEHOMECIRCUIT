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

#ifndef PSD_TYPES_H_
#define PSD_TYPES_H_

#include <linux/types.h>

#define PSD_BATTERY_STATES_OFF  0
#define PSD_BATTERY_STATES_ON   1
struct psd_battery
{
    __u8 cable_state;
    __u8 enumeration_state;
    __u8 charge_state;
    __u8 level;
};

#define PSD_BUTTON_OFF        0
#define PSD_BUTTON_ON         1
struct psd_button
{
    __u8 state;
};

#define PSD_DRIVE_CONTROL_MAX 127
struct psd_drive_control
{
    __s8 throttle;
    __s8 steering;
};

struct psd_drive_trim
{
    __u8 throttle_percent;
    __s8 steering_offset;
};

struct psd_voltage
{
    __u16 throttle;
    __u16 steering;
    __u16 potentio;
    __u16 battery;
    __u16 temperature;
    __u8 _pad[6];
};


#define PSD_CLOCK_C_DISABLED 0
#define PSD_CLOCK_C_ENABLED  1
struct psd_internal_data
{
    __s32 throttle_ma;
    __s32 steering_ma;
    __u32 throttle_rotation_per_min;
    __s32 throttle_temp_raw;
    __s16 steering_degree;
    __u16 battery_cv;
    __u16 throttle_torque_unm;
    __s8 throttle_pwm;
    __s8 steering_pwm;
    __u8 clock_correction;
    __u8 throttle_stall_rate;
    __u8 _pad[6];
    struct psd_voltage voltage;
};

#define PSD_PS_LED_OFF        0
#define PSD_PS_LED_ON         1
#define PSD_PS_LED_BRINK_LOW  2
#define PSD_PS_LED_BRINK_HIGH 3
struct psd_power_sync_led
{
    __u8 state;
};

#define PSD_CB_LED_RATE_MAX 200
struct psd_charge_brake_led
{
    __u8 rate;
};

#define PSD_SHUTDOWN_DOWN   0
#define PSD_SHUTDOWN_REBOOT 1
struct psd_shutdown
{
    __u8 command;
};

struct psd_six_axis
{
    __u64 time_stamp_us;
    __s16 acceleration_x;
    __s16 acceleration_y;
    __s16 acceleration_z;
    __s16 angular_velocity_x;
    __s16 angular_velocity_y;
    __s16 angular_velocity_z;
    __u8  _pad[4];
};

struct psd_six_axis_offset
{
    __s16 acceleration_x;
    __s16 acceleration_y;
    __s16 acceleration_z;
    __s16 angular_velocity_x;
    __s16 angular_velocity_y;
    __s16 angular_velocity_z;
};

#define PSD_IAP_DOWN   0
#define PSD_IAP_REBOOT 1
struct psd_iap
{
    __u8 command;
};

#define PSD_MCU_STATE_STANBY     0x00
#define PSD_MCU_STATE_WAKEUP     0x01
#define PSD_MCU_STATE_CONNECTED  0x02
#define PSD_MCU_STATE_PAIRING    0x03
#define PSD_MCU_STATE_IAP        0xF0
#define PSD_MCU_STATE_RESET      0xFF
struct psd_mcu_status
{
    __u8 state;
};

#define PSD_MCU_UPDATE_STATE_IAP_IDLE           0x00
#define PSD_MCU_UPDATE_STATE_IAP_ERASING        0x01
#define PSD_MCU_UPDATE_STATE_IAP_ERASED         0x02
#define PSD_MCU_UPDATE_STATE_IAP_WRITING        0x03
#define PSD_MCU_UPDATE_STATE_IAP_WROTE          0x04
#define PSD_MCU_UPDATE_STATE_IAP_VERIFYING      0x05
#define PSD_MCU_UPDATE_STATE_IAP_VERIFIED       0x06
#define PSD_MCU_UPDATE_STATE_IAP_ERROR_CRC8     0x07
#define PSD_MCU_UPDATE_STATE_IAP_ERROR_ERASING  0x08
#define PSD_MCU_UPDATE_STATE_IAP_ERROR_WRITING  0x09
#define PSD_MCU_UPDATE_STATE_IAP_ERROR_FW_SIZE  0x0a
#define PSD_MCU_UPDATE_STATE_IAP_ERROR_FW_CRC32 0x0b
struct psd_mcu_update_state
{
    __u8 state;
};

struct psd_mcu_update_fw_data
{
    __u32 address;
    __u8 size;
    __u8 data[126];
};

struct psd_mcu_update_fw_verify_data
{
    __u16 data_size;
    __u32 crc;
};

#define MCU_FW_HEADER_SIZE 8
#define MCU_FW_MAX_SIZE (28 * 1024 + MCU_FW_HEADER_SIZE)

struct psd_mcu_fw_data
{
    __u16 fw_total_size;
    __u16 fw_transfer_size;
    __u8  fw_transfer_number;
    __u8  fw_image[1000];
};

struct psd_mcu_fw_version
{
    __u8  major;
    __u8  minor;
};

struct psd_motor_temperature
{
    __s32 percent;
};

#define PSD_PARK_STATE_NOT_PARKING 0
#define PSD_PARK_STATE_PARKING 1
struct psd_park_state
{
    __u8 state;
};

#define PSD_INIT_STATE_NOT_INITIALIZED 0
#define PSD_INIT_STATE_INITIALIZED  1
struct psd_init_state
{
    __u8 state;
};

#endif
