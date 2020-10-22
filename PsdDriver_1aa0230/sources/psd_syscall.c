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

#include <linux/slab.h>
#include <asm/uaccess.h>
#include "psd.h"
#include "psd_battery.h"
#include "psd_heart_beat.h"
#include "psd_mcu_update.h"
#include "psd_param.h"
#include "psd_park.h"
#include "psd_physics.h"
#include "psd_syscall.h"
#include "psd_temperature.h"
#include "psd_util.h"


#define WRITE_STR_SIZE 64

static int get_battery(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_battery battery = {0};
    struct psd_charge_state charge_state = {0};
    struct psd_voltage voltage = {0};
    charge_state = psd_param_get_charge_state();
    voltage = psd_param_get_voltage();
    battery.cable_state = charge_state.cable_state;
    battery.enumeration_state = charge_state.enumeration_state;
    battery.charge_state = charge_state.charge_state;
    battery.level = psd_battery_get_level();

    result = copy_to_user((void __user *)arg, &battery, sizeof(battery));
    if (result == 0)
    {
        return sizeof(battery);
    }
    else
    {
        return -EFAULT;
    }
}

static int get_button(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_button button = {0};
    button = psd_param_get_button();
    result = copy_to_user((void __user *)arg, &button, sizeof(button));
    if (result == 0)
    {
        return sizeof(button);
    }
    else
    {
        return -EFAULT;
    }
}

static int set_charge_brake_led(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_charge_brake_led led = {0};
    result = copy_from_user(&led, (void __user *)arg, sizeof(led));
    if (result == 0)
    {
        led.rate = psd_util_min(200, led.rate);//mcu仕様上、200が最大
        psd_param_set_charge_brake_led(&led);
        return sizeof(led);
    }
    else
    {
        return -EFAULT;
    }
}

static int set_drive_control(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_drive_control drive_control = {0};
    result = copy_from_user(&drive_control, (void __user *)arg, sizeof(drive_control));
    if (result == 0)
    {
        drive_control.steering = psd_util_minmax(-PSD_DRIVE_CONTROL_MAX, drive_control.steering, PSD_DRIVE_CONTROL_MAX);
        drive_control.throttle = psd_util_minmax(-PSD_DRIVE_CONTROL_MAX, drive_control.throttle, PSD_DRIVE_CONTROL_MAX);
        psd_param_set_drive_control(&drive_control);
        return sizeof(drive_control);
    }
    else
    {
        return -EFAULT;
    }
}

static int set_drive_trim(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_drive_trim drive_trim = {0};
    result = copy_from_user(&drive_trim, (void __user *)arg, sizeof(drive_trim));
    if (result == 0)
    {
        drive_trim.steering_offset = psd_util_minmax(-PSD_DRIVE_CONTROL_MAX, drive_trim.steering_offset, PSD_DRIVE_CONTROL_MAX);
        drive_trim.throttle_percent = psd_util_min(drive_trim.throttle_percent, 100);
        psd_param_set_drive_trim(&drive_trim);
        return sizeof(drive_trim);
    }
    else
    {
        return -EFAULT;
    }
}

static int get_internal_data(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_internal_data internal_data = {0};
    
    internal_data.voltage = psd_param_get_voltage();
    internal_data.steering_degree = psd_physics_get_current_angle();
    internal_data.battery_cv = psd_physics_get_battery_cv();

    {
        if(psd_param_get_mcu_clock_correction().is_enable)
        {
            internal_data.clock_correction = PSD_CLOCK_C_ENABLED;
        }
        else
        {
            internal_data.clock_correction = PSD_CLOCK_C_DISABLED;
        }
    }

    {
        struct psd_drive_control drive_control;
        struct psd_drive_trim drive_trim;
        struct psd_util_ratio ratio;
        int sign;
        drive_control = psd_param_get_drive_control();
        drive_trim = psd_param_get_drive_trim();
        sign = psd_physics_calculate_throttle_duty(&ratio, drive_control.throttle, drive_trim.throttle_percent);
        internal_data.throttle_ma = psd_physics_calculate_throttle_current(ratio);
        internal_data.throttle_pwm = sign * psd_util_apply_ratio(100, ratio);
        internal_data.throttle_rotation_per_min = psd_physics_get_throttle_rpm(ratio);
        internal_data.throttle_torque_unm = psd_physics_get_throttle_torque(ratio);
        internal_data.throttle_temp_raw = psd_temperature_get_raw();
        internal_data.throttle_stall_rate = psd_physics_get_throttle_stall_rate(ratio);

        struct psd_feedback feedback;
        feedback = psd_param_get_feedback_setting();
        if(feedback.is_enable)
        {
            sign = psd_physics_calculate_steering_duty(&ratio, drive_control.steering, drive_trim.steering_offset);
            internal_data.steering_ma = psd_physics_calculate_steering_current(ratio);
            internal_data.steering_pwm = sign * psd_util_apply_ratio(100, ratio);
        }
        else
        {
            sign = psd_physics_calculate_spring_steering_duty(&ratio, drive_control.steering);
            internal_data.steering_ma = psd_physics_calculate_steering_current(ratio);
            internal_data.steering_pwm = sign * psd_util_apply_ratio(100, ratio);
        }
    }
    
    result = copy_to_user((void __user *)arg, &internal_data, sizeof(internal_data));
    if (result == 0)
    {
        return sizeof(internal_data);
    }
    else
    {
        return -EFAULT;
    }
}

static int get_power_sync_led(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_power_sync_led led = {0};
    led = psd_param_get_power_sync_led();
    result = copy_to_user((void __user *)arg, &led, sizeof(led));
    if (result == 0)
    {
        return sizeof(led);
    }
    else
    {
        return -EFAULT;
    }
}

int get_mcu_state(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_current_mcu_state current_status = {0};
    struct psd_mcu_status status = {0};
    current_status = psd_param_get_current_mcu_state();
    status.state = current_status.state;
    result = copy_to_user((void __user *)arg, &status, sizeof(status));
    if (result == 0)
    {
        return sizeof(status);
    }
    else
    {
        return -EFAULT;
    }
}

int get_mcu_update_state(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_current_mcu_iap_state current_status = {0};
    struct psd_mcu_update_state status = {0};
    current_status = psd_param_get_current_mcu_iap_state();
    status.state = current_status.state;
    result = copy_to_user((void __user *)arg, &status, sizeof(status));
    if (result == 0)
    {
        return sizeof(status);
    }
    else
    {
        return -EFAULT;
    }
}

static int set_power_sync_led(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_power_sync_led led = {0};
    result = copy_from_user(&led, (void __user *)arg, sizeof(led));
    if (result == 0)
    {
        psd_param_set_power_sync_led(&led);
        return sizeof(led);
    }
    else
    {
        return -EFAULT;
    }
}

static int set_shutdown(unsigned int cmd, unsigned long arg)
{
    printk(KERN_ERR "shutdown request received.\n");
    int result;
    struct psd_shutdown shutdown = {0};
    result = copy_from_user(&shutdown, (void __user *)arg, sizeof(shutdown));
    if (result == 0)
    {
        psd_param_set_shutdown(&shutdown);
        return sizeof(shutdown);
    }
    else
    {
        return -EFAULT;
    }
}

static int set_iap(void)
{
    psd_param_set_iap();
    return WRITE_STR_SIZE;
}

int set_iap_fw_data_commnad(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_mcu_fw_data fw_data = {0};
    result = copy_from_user(&fw_data, (void __user *)arg, sizeof(fw_data));

    if (result == 0)
    {
        psd_mcu_update_append_fw(fw_data);
        return sizeof(fw_data);
    }
    else
    {
        return -EFAULT;
    }
}

static int get_mcu_fw_version(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_mcu_fw_version version = {0};
    version = psd_param_get_mcu_fw_version();
    result = copy_to_user((void __user *)arg, &version, sizeof(version));
    if (result == 0)
    {
        return sizeof(version);
    }
    else
    {
        return -EFAULT;
    }
}

static int get_voltage(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_voltage voltage = {0};
    
    voltage = psd_param_get_voltage();

    result = copy_to_user((void __user *)arg, &voltage, sizeof(voltage));
    if (result == 0)
    {
        return sizeof(voltage);
    }
    else
    {
        return -EFAULT;
    }
}

static int get_motor_temperature(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_motor_temperature motor_temp = {0};
    
    motor_temp.percent = psd_temperature_get_percent();

    result = copy_to_user((void __user *)arg, &motor_temp, sizeof(motor_temp));
    if (result == 0)
    {
        return sizeof(motor_temp);
    }
    else
    {
        return -EFAULT;
    }
}

static int update_heart_beat(void)
{
    psd_heart_beat_update();
    return 0;
}

static int get_park_state(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_park_state park_state = {0};
    
    if(psd_park_is_parking())
    {
        park_state.state = PSD_PARK_STATE_PARKING;
    }
    else
    {
        park_state.state = PSD_PARK_STATE_NOT_PARKING;
    }

    result = copy_to_user((void __user *)arg, &park_state, sizeof(park_state));
    if (result == 0)
    {
        return sizeof(park_state);
    }
    else
    {
        return -EFAULT;
    }
}

static int get_init_state(unsigned int cmd, unsigned long arg)
{
    int result;
    struct psd_init_state init_state = {0};

    init_state = psd_param_get_init_state();
    result = copy_to_user((void __user *)arg, &init_state, sizeof(init_state));
    if (result == 0)
    {
        return sizeof(init_state);
    }
    else
    {
        return -EFAULT;
    }
}

int psd_syscall_ioctl(unsigned int cmd, unsigned long arg)
{
    switch(cmd)
    {
        case PSD_IOC_RD_BATTERY:
            return get_battery(cmd, arg);
        case PSD_IOC_RD_BUTTON:
            return get_button(cmd, arg);
        case PSD_IOC_WR_CHARGE_BRAKE_LED:
            return set_charge_brake_led(cmd, arg);
        case PSD_IOC_WR_DRIVE_CONTROL:
            return set_drive_control(cmd, arg);
        case PSD_IOC_WR_DRIVE_TRIM:
            return set_drive_trim(cmd, arg);
        case PSD_IOC_RD_INTERNAL_DATA:
            return get_internal_data(cmd, arg);
        case PSD_IOC_RD_POWER_SYNC_LED:
            return get_power_sync_led(cmd, arg);
        case PSD_IOC_WR_POWER_SYNC_LED:
            return set_power_sync_led(cmd, arg);
        case PSD_IOC_WR_SHUTDOWN:
            return set_shutdown(cmd, arg);
        case PSD_IOC_WR_IAP:
            return set_iap();
        case PSD_IOC_RD_MCU_STATUS:
            return get_mcu_state(cmd, arg);
        case PSD_IOC_RD_MCU_UPDATE_STATUS:
            return get_mcu_update_state(cmd, arg);
        case PSD_IOC_WR_MCU_SEND_UPDATE_IMAGE:
            return set_iap_fw_data_commnad(cmd, arg);
        case PSD_IOC_RD_MCU_FW_VERSION:
            return get_mcu_fw_version(cmd, arg);
        case PSD_IOC_RD_VOLTAGE:
            return get_voltage(cmd, arg);
        case PSD_IOC_RD_MOTOR_TEMPERATURE:
            return get_motor_temperature(cmd, arg);
        case PSD_IOC_HEART_BEAT:
            return update_heart_beat();
        case PSD_IOC_RD_PARK_STATE:
            return get_park_state(cmd, arg);
        case PSD_IOC_RD_INIT_STATE:
            return get_init_state(cmd, arg);
        default:
            PSD_LOG("unsupported command in %s. cmd = %u\n", __FUNCTION__, cmd);
            return -EFAULT;
    }
}

static char* get_value(const char* p_token, int* p_out, int* p_result)
{
    if(p_token == NULL || p_out == NULL)
    {
        *p_result = -EFAULT;
        return NULL;
    }

    //直近の','の置き換えと、次回向けにtokenを更新
    char* p_next_token;
    p_next_token = strchr(p_token, ',');
    if(p_next_token == NULL)
    {
        //最後の入力。終端が入っている前提
    }
    else
    {
        *p_next_token = '\0';
        p_next_token++;
    }

    if(kstrtoint(p_token, 10, p_out) == 0)
    {
        *p_result = 0;
        return p_next_token;
    }
    else
    {
        *p_result = -EFAULT;
        return NULL;
    }
}

static int enable_feedback(char str[])
{
    char* p_token = &str[1];
    int result;
    int percent_p;
    int permille_i;
    int percent_d;
    int integral_max;
    int feedforward;

    p_token = get_value(p_token, &percent_p, &result);
    if(result != 0)
    {
        return result;
    }

    p_token = get_value(p_token, &permille_i, &result);
    if(result != 0)
    {
        return result;
    }

    p_token = get_value(p_token, &percent_d, &result);
    if(result != 0)
    {
        return result;
    }

    p_token = get_value(p_token, &integral_max, &result);
    if(result != 0)
    {
        return result;
    }

    p_token = get_value(p_token, &feedforward, &result);
    if(result != 0)
    {
        return result;
    }

    percent_p = psd_util_minmax(0, percent_p, PSD_INTERNAL_TYPES_P_FACTOR_MAX);
    permille_i = psd_util_minmax(0, permille_i, PSD_INTERNAL_TYPES_I_FACTOR_MAX);
    percent_d = psd_util_minmax(0, percent_d, PSD_INTERNAL_TYPES_D_FACTOR_MAX);
    integral_max = psd_util_minmax(0, integral_max, PSD_INTERNAL_TYPES_I_MAX);
    feedforward = psd_util_minmax(0, feedforward, PSD_INTERNAL_TYPES_FF_MAX);

    struct psd_feedback feedback = {0};
    feedback.is_enable = true;
    feedback.percent_p = percent_p;
    feedback.permille_i = permille_i;
    feedback.percent_d = percent_d;
    feedback.integral_max = integral_max;
    feedback.feedforward = feedforward;
    psd_param_set_feedback_setting(&feedback);
    return WRITE_STR_SIZE;
}

static int disable_feedback(void)
{
    struct psd_feedback feedback;
    feedback.is_enable = false;
    feedback.percent_p = 0;
    feedback.permille_i = 0;
    feedback.percent_d = 0;
    feedback.integral_max = 0;
    psd_param_set_feedback_setting(&feedback);
    return WRITE_STR_SIZE;
}

static int set_drive(char str[])
{
    char* p_token = &str[0];
    int result;
    int steering;
    int throttle;

    p_token = get_value(p_token, &steering, &result);
    if(result != 0)
    {
        return result;
    }

    p_token = get_value(p_token, &throttle, &result);
    if(result != 0)
    {
        return result;
    }

    struct psd_drive_control drive_control = {0};
    drive_control.steering = steering;
    drive_control.throttle = throttle;

    psd_param_set_drive_control(&drive_control);

    return WRITE_STR_SIZE;
}

static int enable(char str[])
{
    if(str[1] == 'h')
    {
        printk(KERN_ERR "heart beat enabled.\n");
        psd_param_set_heart_beat_fail_safe_setting(true);
        return WRITE_STR_SIZE;
    }
    else if(str[1] == 'l')
    {
        struct psd_drive_log drive_log = {0};
        drive_log.is_enable = true;
        psd_param_set_drive_log_setting(&drive_log);
        return WRITE_STR_SIZE;
    }
    else
    {
        return WRITE_STR_SIZE;
    }
}

static int disable(char str[])
{
    if(str[1] == 'h')
    {
        printk(KERN_ERR "heart beat disabled.\n");
        psd_param_set_heart_beat_fail_safe_setting(false);
        return WRITE_STR_SIZE;
    }
    else if(str[1] == 's')
    {
        struct psd_fail_safe fail_safe;
        fail_safe = psd_param_get_fail_safe_setting();
        fail_safe.is_stall_enable = false;
        psd_param_set_fail_safe_setting(&fail_safe);
        return WRITE_STR_SIZE;
    }
    else if(str[1] == 't')
    {
        struct psd_fail_safe fail_safe;
        fail_safe = psd_param_get_fail_safe_setting();
        fail_safe.is_motor_temp_enable = false;
        psd_param_set_fail_safe_setting(&fail_safe);
        return WRITE_STR_SIZE;
    }
    else
    {
        return WRITE_STR_SIZE;
    }
}

int psd_syscall_write(const char __user* p_input, size_t size)
{
    if(size < 0 || size > WRITE_STR_SIZE)
    {
        return -EFAULT;
    }

    int result;
    char str[WRITE_STR_SIZE] = "";
    result = copy_from_user(&str[0], p_input, size);
    if(result != 0)
    {
        return -EFAULT;
    }
    str[WRITE_STR_SIZE - 1] = '\0';

    switch(str[0])
    {
        case 'f':
            return enable_feedback(&str[0]);
        case 's':
            return disable_feedback();
        case 'e':
            return enable(&str[0]);
        case 'd':
            return disable(&str[0]);
        default:
            return set_drive(&str[0]);
    }
}

#define READ_STR_SIZE 64

int psd_syscall_read(char __user *p_output, size_t size)
{
    if(size < READ_STR_SIZE)
    {
        return -EFAULT;
    }

    struct psd_drive_control drive_control = {0};
    drive_control = psd_param_get_drive_control();

    struct psd_feedback feedback = {0};
    feedback = psd_param_get_feedback_setting();

    char f_or_s;
    if(feedback.is_enable)
    {
        f_or_s = 'f';
    }
    else
    {
        f_or_s = 's';
    }

    char str[READ_STR_SIZE] = "";
    int write_size;
    write_size = snprintf(&str[0], READ_STR_SIZE, "%c%hu,%hu,%hu,%hu,%hu,%hhd,%hhd",
    f_or_s, feedback.percent_p, feedback.permille_i, feedback.percent_d,
    feedback.integral_max, feedback.feedforward, drive_control.steering, drive_control.throttle);
    if (write_size < 0)
    {
        return -EFAULT;
    }

    int result;
    result = copy_to_user(p_output, &str[0], READ_STR_SIZE);
    if (result != 0)
    {
        return -EFAULT;
    }

    //マルチスレッド非対応
    static bool is_end = false;
    if(is_end)
    {
        is_end = false;
        return 0;
    }
    else
    {
        is_end = true;
        return write_size;
    }
}
