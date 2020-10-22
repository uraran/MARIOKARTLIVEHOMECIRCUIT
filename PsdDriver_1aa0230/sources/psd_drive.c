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
#include "psd_deviation.h"
#include "psd_gpio.h"
#include "psd_log.h"
#include "psd_param.h"
#include "psd_park.h"
#include "psd_physics.h"
#include "psd_pwm.h"
#include "psd_stall.h"
#include "psd_temperature.h"
#include "psd_types.h"
#include "psd_util.h"

static void update_deviation_history(s8 steering, s8 trim)
{
    s16 target_angle  = psd_physics_get_target_angle(steering);
    s16 current_angle = psd_physics_get_current_angle();
    current_angle += psd_physics_get_offset_angle(trim);
    psd_deviation_update(current_angle, target_angle);
}

static bool is_acceptable_steering_position(s8 steering, s8 trim)
{
    s16 target_angle  = psd_physics_get_target_angle(steering);
    s16 current_angle = psd_physics_get_current_angle();
    current_angle += psd_physics_get_offset_angle(trim);

    if(abs(current_angle - target_angle) < 4)
    {
        //0.4度以下のズレは、合格とする。制御しない
        return true;
    }
    else
    {
        return false;
    }
}

static void set_steering(s8 steering, s8 trim)
{
    int sign;
    struct psd_util_ratio ratio;

    if(is_acceptable_steering_position(steering, trim))
    {
        sign = 0;
        ratio.raw = 0;
    }
    else
    {
        update_deviation_history(steering, trim);
        sign = psd_physics_calculate_steering_duty(&ratio, steering, trim);
    }

    psd_log_set(psd_log_id_steering_control, steering);
    psd_log_set(psd_log_id_steering_duty, sign * psd_util_apply_ratio(100, ratio));
    psd_log_set(psd_log_id_steering_intensity_oc, psd_physics_calculate_steering_current(ratio));

    if(sign > 0)
    {
        psd_gpio_set_pin(PSD_GPIO_MD_EN_B, 1);
        psd_gpio_set_pin(PSD_GPIO_MD_PH_B, 1);
        psd_pwm_set_duty(1, ratio);
    }
    else if(sign < 0)
    {
        psd_gpio_set_pin(PSD_GPIO_MD_EN_B, 1);
        psd_gpio_set_pin(PSD_GPIO_MD_PH_B, 0);
        psd_pwm_set_duty(1, ratio);
    }
    else
    {
        psd_gpio_set_pin(PSD_GPIO_MD_EN_B, 1);
        psd_gpio_set_pin(PSD_GPIO_MD_PH_B, 0);
        psd_pwm_set_duty(1, ratio);
    }
}

static void set_spring_steering(s8 steering)
{
    int sign;
    struct psd_util_ratio ratio;

    sign = psd_physics_calculate_spring_steering_duty(&ratio, steering);
    psd_log_set(psd_log_id_steering_control, steering);
    psd_log_set(psd_log_id_steering_duty, sign * psd_util_apply_ratio(100, ratio));
    psd_log_set(psd_log_id_steering_intensity_oc, psd_physics_calculate_steering_current(ratio));

    if(sign > 0)
    {
        psd_gpio_set_pin(PSD_GPIO_MD_EN_B, 1);
        psd_gpio_set_pin(PSD_GPIO_MD_PH_B, 1);
        psd_pwm_set_duty(1, ratio);
    }
    else if(sign < 0)
    {
        psd_gpio_set_pin(PSD_GPIO_MD_EN_B, 1);
        psd_gpio_set_pin(PSD_GPIO_MD_PH_B, 0);
        psd_pwm_set_duty(1, ratio);
    }
    else
    {
        psd_gpio_set_pin(PSD_GPIO_MD_EN_B, 0);
        psd_gpio_set_pin(PSD_GPIO_MD_PH_B, 0);
        psd_pwm_set_duty(1, ratio);
    }
}

static bool update_stall_safety(struct psd_util_ratio ratio, s8 throttle)
{
    struct psd_fail_safe fail_safe;
    fail_safe = psd_param_get_fail_safe_setting();
    if(fail_safe.is_stall_enable)
    {
        //フェールセーフ有効のため、後続
    }
    else
    {
        //フェールセーフ無効のため、安全として返す
        return true;
    }

    static int count = 0;
    count++;
    if(count % 20 == 0)
    {
        //20回に1回(4.7ms周期なので、約94ms)ストール状態を更新
        //30回のストール(約 2.82s)でフェールセーフ
        psd_stall_update(ratio);
    }

    if(throttle == 0)
    {
        psd_stall_release();
    }

    return psd_stall_is_safe();
}

static bool update_temperature_safety(struct psd_util_ratio ratio)
{
    static int count = 0;
    count++;
    if(count % 10 == 0)
    {
        psd_temperature_update(ratio);
    }

    struct psd_fail_safe fail_safe;
    fail_safe = psd_param_get_fail_safe_setting();
    if(fail_safe.is_motor_temp_enable)
    {
        //フェールセーフ有効のため、後続
    }
    else
    {
        //フェールセーフ無効のため、安全として返す
        return true;
    }

    if(psd_temperature_is_safe())
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool update_throttle_safety(struct psd_util_ratio ratio, s8 throttle)
{
    bool is_stall_safe = update_stall_safety(ratio, throttle);
    bool is_temperature_safe = update_temperature_safety(ratio);

    return is_stall_safe && is_temperature_safe;
}

static void set_throttle(s8 throttle, u8 trim)
{
    int sign;
    struct psd_util_ratio ratio;

    sign = psd_physics_calculate_throttle_duty(&ratio, throttle, trim);
    psd_log_set(psd_log_id_throttle_control, throttle);
    psd_log_set(psd_log_id_throttle_duty, sign * psd_util_apply_ratio(100, ratio));
    psd_log_set(psd_log_id_throttle_intensity_oc, psd_physics_calculate_throttle_current(ratio));

    bool is_safe = update_throttle_safety(ratio, throttle);
    psd_park_update(!is_safe, throttle);
    if(!is_safe)
    {
        ratio = psd_util_ratio(0, 1);//入力を0に
    }

    if(sign > 0)
    {
        //正転
        psd_gpio_set_pin(PSD_GPIO_MD_EN_A, 1);
        psd_gpio_set_pin(PSD_GPIO_MD_PH_A, 0);
        psd_gpio_set_pin(PSD_GPIO_MD_PH_A_DP, 0);
        psd_pwm_set_duty(0, ratio);
    }
    else if(sign < 0)
    {
        //逆転
        psd_gpio_set_pin(PSD_GPIO_MD_EN_A, 1);
        psd_gpio_set_pin(PSD_GPIO_MD_PH_A, 1);
        psd_gpio_set_pin(PSD_GPIO_MD_PH_A_DP, 1);
        psd_pwm_set_duty(0, ratio);
    }
    else
    {
        //ブレーキ
        psd_gpio_set_pin(PSD_GPIO_MD_EN_A, 1);
        psd_gpio_set_pin(PSD_GPIO_MD_PH_A, 0);
        psd_gpio_set_pin(PSD_GPIO_MD_PH_A_DP, 0);
        psd_pwm_set_duty(0, ratio);
    }
}

void psd_drive_update(void)
{
    psd_gpio_set_pin(PSD_GPIO_MD_SR, 1);

    struct psd_drive_control drive_control;
    struct psd_drive_trim drive_trim;
    drive_control = psd_param_get_drive_control();
    drive_trim = psd_param_get_drive_trim();

    struct psd_feedback feedback;
    feedback = psd_param_get_feedback_setting();
    if(feedback.is_enable)
    {
        set_steering(drive_control.steering, drive_trim.steering_offset);
    }
    else
    {
        set_spring_steering(drive_control.steering);
    }

    set_throttle(drive_control.throttle, drive_trim.throttle_percent);

    psd_log_set(psd_log_id_steering_angle, psd_physics_get_current_angle());
    psd_log_set(psd_log_id_batv, psd_physics_get_battery_cv() * 10);
    psd_log_flush();
}
