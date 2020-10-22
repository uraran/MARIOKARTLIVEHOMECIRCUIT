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
#include "psd_log.h"
#include "psd_param.h"
#include "psd_types.h"
#include "psd_util.h"

#define PSD_STEERING_ANGLE_MAX 300
#define PSD_ADC_MAX 4096

s16 psd_physics_get_target_angle(s8 steering)
{
    struct psd_util_ratio ratio;
    s16 angle;
    ratio = psd_util_range_ratio(steering, -PSD_DRIVE_CONTROL_MAX, PSD_DRIVE_CONTROL_MAX);
    angle = psd_util_apply_ratio_to_range(-PSD_STEERING_ANGLE_MAX, PSD_STEERING_ANGLE_MAX, ratio);
    return angle;
}

s16 psd_physics_get_current_angle(void)
{
    struct psd_voltage voltage;
    voltage = psd_param_get_voltage();

    //計算中は、100分の1の単位（centi）を使用
    const u16 PotentioAngleCapacity_c = 33330;//333.3 * 100
    u16 angle_c;
    struct psd_util_ratio ratio;
    
    //ADCを角度に変換
    ratio = psd_util_ratio(voltage.potentio, PSD_ADC_MAX);
    //2Byteに対して、最大1の比率反映のため、桁は溢れない
    angle_c = (u16)psd_util_apply_ratio(PotentioAngleCapacity_c, ratio);

    //角度をADCの特性で補正
    ratio = psd_util_ratio(26, 28);//2.6v / 2.8v
    angle_c = (u16)psd_util_apply_ratio(angle_c, ratio);

    //角度の中央を0度に変更。後段の計算のため、符号と絶対値に分解
    s16 centered_angle_c;
    centered_angle_c = angle_c - 16665;//333.3 / 2 * 100
    int sign = psd_util_sign(centered_angle_c);
    u16 abs_angle_c = abs(centered_angle_c);

    //ポテンショ角とステア角のズレを補正
    ratio = psd_util_ratio(PSD_STEERING_ANGLE_MAX * 10, 3805);//30度と38.05度
    abs_angle_c = psd_util_apply_ratio(abs_angle_c, ratio);
    //100分の1を10分の1に戻す
    u16 abs_angle = psd_exponential_round(abs_angle_c, 1);

    return (s16)(abs_angle * sign);
}

s16 psd_physics_get_offset_angle(s8 trim)
{
    //処理内容は psd_physics_get_target_angle()と同じ
    struct psd_util_ratio ratio = psd_util_range_ratio(trim, -PSD_DRIVE_CONTROL_MAX, PSD_DRIVE_CONTROL_MAX);
    return psd_util_apply_ratio_to_range(-PSD_STEERING_ANGLE_MAX, PSD_STEERING_ANGLE_MAX, ratio);
}

static s16 apply_proportional_factor(s16 control)
{
    control = psd_util_minmax(-PSD_STEERING_ANGLE_MAX, control, PSD_STEERING_ANGLE_MAX);
    
    //係数を取得
    struct psd_util_ratio ratio;
    struct psd_feedback feedback;
    feedback = psd_param_get_feedback_setting();
    feedback.percent_p = psd_util_minmax(0, feedback.percent_p, PSD_INTERNAL_TYPES_P_FACTOR_MAX);
    ratio = psd_util_ratio(feedback.percent_p, 100);//パーセント

    //係数を反映
    int sign = psd_util_sign(control);
    u16 abs_control = abs(control);
    s16 result = (s16)psd_util_apply_ratio(abs_control, ratio);//最大 P_FACTOR_MAX /100 倍なので、s16に収まる
    return result * sign;
}

static s16 apply_integral_factor(s16 control)
{
    control = psd_util_minmax(-PSD_INTERNAL_TYPES_I_MAX, control, PSD_INTERNAL_TYPES_I_MAX);

    //係数を取得
    struct psd_util_ratio ratio;
    struct psd_feedback feedback;
    feedback = psd_param_get_feedback_setting();
    feedback.permille_i = psd_util_minmax(0, feedback.permille_i, PSD_INTERNAL_TYPES_I_FACTOR_MAX);
    ratio = psd_util_ratio(feedback.permille_i, 1000);//パーミル

    //係数を反映
    int sign = psd_util_sign(control);
    u16 abs_control = abs(control);//最大 I_MAX なので、u16に収まる
    s16 result = (s16)psd_util_apply_ratio(abs_control, ratio);//最大 I_FACTOR_MAX / 1000 倍なので、s16 に収まる
    return result * sign;
}

static int apply_differential_factor(s16 control)
{
    //係数を取得
    struct psd_util_ratio ratio;
    struct psd_feedback feedback;
    feedback = psd_param_get_feedback_setting();
    feedback.percent_d = psd_util_minmax(0, feedback.percent_d, PSD_INTERNAL_TYPES_D_FACTOR_MAX);
    ratio = psd_util_ratio(feedback.percent_d, 100);//パーセント

    //係数を反映
    int sign = psd_util_sign(control);
    u16 abs_control = abs(control);
    int result = psd_util_apply_ratio(abs_control, ratio);//最大 D_FACTOR_MAX / 100 倍なので、intに収まる
    return result * sign;
}

u16 psd_physics_get_battery_cv(void)
{
    struct psd_voltage voltage;
    voltage = psd_param_get_voltage();

    struct psd_util_ratio ratio;
    const u16 vdda_cv = 260;//VDDA 2.6vを基準。centi単位を使用
    u16 result_cv;

    ratio = psd_util_ratio(voltage.battery, PSD_ADC_MAX);
    //2Byteに対して、最大1の比率反映のため、桁は溢れない
    result_cv = (u16)psd_util_apply_ratio(vdda_cv, ratio);

    result_cv *= 2;// 回路で1/2に分圧されているので、元に戻す
    return result_cv;
}

static struct psd_util_ratio convert_to_duty_by_batv(u16 motor_cv)
{
    //電池電圧を元に、比率を算出
    const u16 battery_cv = psd_physics_get_battery_cv();
    motor_cv = psd_util_min(motor_cv, battery_cv);
    return psd_util_ratio(motor_cv, battery_cv);
}

static u16 convert_to_steering_motor_cv(struct psd_util_ratio ratio)
{
    const int moter_cv_max = 300;//3.0v
    int motor_cv = psd_util_apply_ratio(moter_cv_max, ratio);
    return (u16)psd_util_min(motor_cv, moter_cv_max);
}

static u16 convert_to_throttle_motor_cv(struct psd_util_ratio ratio)
{
#ifdef FUJI_BOARD_TYPE_EP
    const int moter_cv_max = 300;//3.00v
#else
    const int moter_cv_max = 314;//3.14v
#endif
    int motor_cv = psd_util_apply_ratio(moter_cv_max, ratio);
    return (u16)psd_util_min(motor_cv, moter_cv_max);
}

static int apply_feedforward_control(int control)
{
    struct psd_feedback feedback;
    feedback = psd_param_get_feedback_setting();
    
    if(control > 0)
    {
        control += feedback.feedforward;
    }
    else if(control < 0)
    {
        control -= feedback.feedforward;
    }

    return control;
}

int psd_physics_calculate_steering_duty(struct psd_util_ratio* p_ratio, s8 steering, s8 trim)
{
    steering = psd_util_minmax(-PSD_DRIVE_CONTROL_MAX, steering, PSD_DRIVE_CONTROL_MAX);
    trim = psd_util_minmax(-PSD_DRIVE_CONTROL_MAX, trim, PSD_DRIVE_CONTROL_MAX);

    //P制御
    s16 proportional_control = apply_proportional_factor(psd_deviation_get_current());

    //I制御
    s16 integral_control = apply_integral_factor(psd_deviation_get_sum());

    //D制御
    int differential_control = apply_differential_factor(psd_deviation_get_sub());

    //制御の合計
    int control = proportional_control + integral_control + differential_control;
    
    //フィードフォワード制御
    control = apply_feedforward_control(control);
    psd_log_set(psd_log_id_steering_manipulation, control);
    
    control = psd_util_minmax(-PSD_STEERING_ANGLE_MAX * 2, control, PSD_STEERING_ANGLE_MAX * 2);
    //以降、符号を分離して処理
    int sign = psd_util_sign(control);
    u16 abs_control = abs(control);

    if(abs_control == 0)
    {
        p_ratio->raw = 0;
        return 0;
    }
    
    //モータ印加電圧に変換
    u16 steering_cv;
    steering_cv = convert_to_steering_motor_cv(psd_util_ratio(abs_control, PSD_STEERING_ANGLE_MAX * 2));    

    //バッテリ電圧で補正しつつ、ratio化
    *p_ratio = convert_to_duty_by_batv(steering_cv);

    return sign;
}

int psd_physics_calculate_spring_steering_duty(struct psd_util_ratio* p_ratio, s8 steering)
{
    steering = psd_util_minmax(-PSD_DRIVE_CONTROL_MAX, steering, PSD_DRIVE_CONTROL_MAX);
    psd_log_set(psd_log_id_steering_manipulation, steering);

    if(steering == 0)
    {
        p_ratio->raw = 0;
        return 0;
    }

    //以降、符号を分離して処理
    int sign = psd_util_sign(steering);
    u16 abs_steering = abs(steering);

    //モータ印加電圧に変換
    u16 steering_cv;
    steering_cv = convert_to_steering_motor_cv(psd_util_ratio(abs_steering, PSD_DRIVE_CONTROL_MAX));    

    //バッテリ電圧で補正しつつ、ratio化
    *p_ratio = convert_to_duty_by_batv(steering_cv);

    return sign;
}

int psd_physics_calculate_throttle_duty(struct psd_util_ratio* p_ratio, s8 throttle, u8 trim)
{
    throttle = psd_util_minmax(-PSD_DRIVE_CONTROL_MAX, throttle, PSD_DRIVE_CONTROL_MAX);
    trim = psd_util_min(trim, 100);

    struct psd_util_ratio ratio;
    ratio = psd_util_ratio(trim, 100);
    throttle = psd_util_apply_ratio(throttle, ratio);
    psd_log_set(psd_log_id_throttle_manipulation, throttle);

    if(throttle == 0)
    {
        p_ratio->raw = 0;
        return 0;
    }

   //以降、符号を分離して処理
    int sign = psd_util_sign(throttle);
    u16 abs_throttle = abs(throttle);

    //モーター印加電圧に変換
    u16 throttle_cv;
    throttle_cv = convert_to_throttle_motor_cv(psd_util_ratio(abs_throttle, PSD_DRIVE_CONTROL_MAX));

    //バッテリ電圧で補正しつつ、ratio化
    *p_ratio = convert_to_duty_by_batv(throttle_cv);

    return sign;
}

static u16 get_steering_base_current(void)
{
    //基準となる電流値
    struct psd_util_ratio ratio;
    const u16 VOLTAGE = 2600;//2.6V * 1000
    const u16 RESISTANCE = 330;//0.33ohm * 1000;
    ratio = psd_util_ratio(VOLTAGE, RESISTANCE);
    u16 current = 1000;//mA単位
    current = (u16)psd_util_apply_ratio(current, ratio);
    return current;
}

u16 psd_physics_calculate_steering_current(struct psd_util_ratio duty)
{
    u16 current;
    current = get_steering_base_current();

    //電流値にAD値の比率を適用
    struct psd_util_ratio ratio;
    ratio = psd_util_ratio(psd_param_get_voltage().steering, 4096);
    current = (u16)psd_util_apply_ratio(current, ratio);

    //電流値にdutyの逆数を適用
    u16 denominator;
    denominator = (u16)psd_util_apply_ratio(1000, duty);
    if(denominator == 0)
    {
        return 0;
    }
    ratio = psd_util_ratio(1000, denominator);
    current = (u16)psd_util_apply_ratio(current, ratio);

    return current;
}

static u16 get_throttle_base_current(void)
{
    //基準となる電流値
    struct psd_util_ratio ratio;
    const u16 VOLTAGE = 2600;//2.6V * 1000
    const u16 RESISTANCE = 51;//0.051ohm * 1000;
    ratio = psd_util_ratio(VOLTAGE, RESISTANCE);
    u16 current = 1000;//mA単位
    current = (u16)psd_util_apply_ratio(current, ratio);
    return current;
}

u16 psd_physics_calculate_throttle_current(struct psd_util_ratio duty)
{
    u16 current;
    current = get_throttle_base_current();

    //電流値にAD値の比率を適用
    struct psd_util_ratio ratio;
    ratio = psd_util_ratio(psd_param_get_voltage().throttle, 4096);
    current = (u16)psd_util_apply_ratio(current, ratio);

    //電流値にdutyの逆数を適用
    u16 denominator;
    denominator = (u16)psd_util_apply_ratio(1000, duty);
    if(denominator == 0)
    {
        return 0;
    }
    ratio = psd_util_ratio(1000, denominator);
    current = (u16)psd_util_apply_ratio(current, ratio);

    return current;
}

u16 psd_physics_get_throttle_torque(struct psd_util_ratio duty)
{
    const u16 is = 26500;//2.65     * 10^4 A
    const u16 io = 1190; //0.119    * 10^4 A
    const u16 id = is - io;//       * 10^4 A
    const u16 ts = 7800;   //0.0078 * 10^6 Nm
    const u16 im = psd_physics_calculate_throttle_current(duty);//mA
    struct psd_util_ratio ratio;

    //左項の計算
    ratio = psd_util_ratio(im, id);
    u32 left_term = psd_util_apply_ratio(ts, ratio);
    //tsの10^6とidの10^4倍が打ち消しあい、10^2が残る
    //mAの分と、10^2に10^1を加えて、マイクロ単位（最小単位は10マイクロ）とする
    left_term *= 10;

    //右項の計算
    ratio = psd_util_ratio(io, id);
    u32 right_term = psd_util_apply_ratio(ts, ratio);
    //tsの10^6とidの10^4倍が打ち消しあい、10^2が残る
    //ioの10^4と上記で、10^6。マイクロ単位となる

    right_term = psd_util_min(left_term, right_term);

    return left_term - right_term;
}

u32 psd_physics_get_throttle_rpm(struct psd_util_ratio duty)
{
    const u16 no = 8300;//rpm
    const u16 ts = 78;  //0.0078 * 10^4 Nm
    const u16 torque = psd_physics_get_throttle_torque(duty);//mNm
    const u16 voltage = psd_util_apply_ratio(psd_physics_get_battery_cv(), duty);//cV
    struct psd_util_ratio ratio;

    //左項の計算
    ratio = psd_util_ratio(no, ts);
    u32 left_term = psd_util_apply_ratio(torque, ratio);
    //トルクのu(10^6)とtsの10^-4を戻す
    left_term /= 100;//値が大きいので切り捨て

    //右項の計算
    ratio = psd_util_ratio(no, 3);
    u32 right_term = psd_util_apply_ratio(voltage, ratio);
    //cVを戻す
    right_term /= 100;//桁が大きいので、切捨て

    return right_term - left_term;//元の式に合わせて、逆順で計算
}

int psd_physics_get_throttle_heat_change(struct psd_util_ratio duty)
{
    const u16 current = psd_exponential_round(psd_physics_calculate_throttle_current(duty), 1);//0-200 cA
    const u16 voltage = psd_util_apply_ratio(psd_physics_get_battery_cv(), duty);//0-300 cV
    u16 watt_in = current * voltage;//10^4
    watt_in /= 10;//10^3単位にする。桁が大きいので、切捨て

    const u16 crpm = psd_physics_get_throttle_rpm(duty) / 100;//円周率の有効桁確保用に、こちらを1/100に
    const u16 torque = psd_physics_get_throttle_torque(duty);//uNm
    u32 watt_out = 314 * 2 * crpm * torque;//3.14 * 2 * T * N 単位は 10^6
    
    watt_out /= 60;
    watt_out /= 1000;//watt_inと単位を揃える。10^3
    
    //outがinより大きくなる事は無いので、最大inに揃える
    watt_out = psd_util_min(watt_in, watt_out);

    int heat_change;
    heat_change = watt_in - watt_out;
    heat_change -= 1080;//1.080

    //PSD_LOG("watt_in %d, watt_out %d, heat_change %d\n", watt_in, watt_out, heat_change);

    return heat_change; 
}

u8 psd_physics_get_throttle_stall_rate(struct psd_util_ratio duty)
{
    const u16 ts = 78;  //0.0078 * 10^4 Nm
    const u16 voltage = psd_util_apply_ratio(psd_physics_get_battery_cv(), duty);//cV

    if(voltage == 0)
    {
        return 0;
    }

    struct psd_util_ratio ratio;
    ratio = psd_util_ratio(voltage, 3);

    u16 stall_torque;
    stall_torque = (u16)psd_util_apply_ratio(ts, ratio);//最大 78 * 300 / 3 uNm

    u16 torque;
    torque = psd_physics_get_throttle_torque(duty);

    ratio = psd_util_ratio(torque, stall_torque);
    return (u8)psd_util_apply_ratio(100, ratio);
}