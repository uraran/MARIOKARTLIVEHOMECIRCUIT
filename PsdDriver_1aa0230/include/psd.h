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

#ifndef PSD_H_
#define PSD_H_

#include <linux/ioctl.h>
#include "psd_types.h"

#define PSD_IOC_TYPE 'P'

#define PSD_IOC_RD_BATTERY                _IOR(PSD_IOC_TYPE, 1, struct psd_battery)
#define PSD_IOC_RD_BUTTON                 _IOR(PSD_IOC_TYPE, 2, struct psd_button)
#define PSD_IOC_WR_CHARGE_BRAKE_LED       _IOR(PSD_IOC_TYPE, 3, struct psd_charge_brake_led)
#define PSD_IOC_WR_DRIVE_CONTROL          _IOW(PSD_IOC_TYPE, 4, struct psd_drive_control)
#define PSD_IOC_WR_DRIVE_TRIM             _IOW(PSD_IOC_TYPE, 5, struct psd_drive_trim)
#define PSD_IOC_RD_INTERNAL_DATA          _IOR(PSD_IOC_TYPE, 6, struct psd_internal_data)
#define PSD_IOC_RD_POWER_SYNC_LED         _IOR(PSD_IOC_TYPE, 7, struct psd_power_sync_led)
#define PSD_IOC_WR_POWER_SYNC_LED         _IOW(PSD_IOC_TYPE, 7, struct psd_power_sync_led)
#define PSD_IOC_WR_SHUTDOWN               _IOW(PSD_IOC_TYPE, 8, struct psd_shutdown)
#define PSD_IOC_WR_IAP                    _IOW(PSD_IOC_TYPE, 11, struct psd_iap)
#define PSD_IOC_RD_MCU_STATUS             _IOR(PSD_IOC_TYPE, 12, struct psd_mcu_status)
#define PSD_IOC_RD_MCU_UPDATE_STATUS      _IOR(PSD_IOC_TYPE, 13, struct psd_mcu_update_state)
#define PSD_IOC_WR_MCU_SEND_UPDATE_IMAGE  _IOW(PSD_IOC_TYPE, 14, struct psd_mcu_fw_data)
#define PSD_IOC_RD_MCU_FW_VERSION         _IOR(PSD_IOC_TYPE, 16, struct psd_mcu_fw_version)
#define PSD_IOC_RD_VOLTAGE                _IOR(PSD_IOC_TYPE, 17, struct psd_voltage)
#define PSD_IOC_RD_MOTOR_TEMPERATURE      _IOR(PSD_IOC_TYPE, 18, struct psd_motor_temperature)
#define PSD_IOC_HEART_BEAT                _IO(PSD_IOC_TYPE, 19)
#define PSD_IOC_RD_PARK_STATE             _IOR(PSD_IOC_TYPE, 20, struct psd_park_state)
#define PSD_IOC_RD_INIT_STATE             _IOR(PSD_IOC_TYPE, 21, struct psd_init_state)

#define PSD_SIX_AXIS_IOC_TYPE 'S'

#define PSD_SIX_AXIS_IOC_WR_OFFSET        _IOW(PSD_SIX_AXIS_IOC_TYPE, 1, struct psd_six_axis_offset)

#endif
