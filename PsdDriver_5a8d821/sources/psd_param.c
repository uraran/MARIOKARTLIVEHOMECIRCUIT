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

#include <linux/circ_buf.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include "psd_param.h"
#include "psd_util.h"

#define SIX_AXIS_BUFFER_RING_SIZE 256 //*must* be a power of 2

struct circ_buf g_six_axis_data;

static spinlock_t g_spinlock;

static struct psd_button g_button;
static struct psd_charge_brake_led g_charge_brake_led;
static struct psd_charge_state g_charge_state;
static struct psd_current_mcu_state g_current_mcu_state;
static struct psd_current_mcu_iap_state g_current_mcu_iap_state;
static struct psd_drive_control g_drive_control;
static struct psd_drive_trim g_drive_trim;
static struct psd_mcu_clock_correction g_mcu_clock_correction;
static struct psd_mcu_fw_version g_mcu_fw_version;
static struct psd_mcu_state_request g_mcu_state_request;
static struct psd_six_axis_offset g_six_axis_offset;
static struct psd_voltage g_voltage;
static struct psd_settings g_settings;
static struct psd_init_state g_init_state; 
static void(*g_six_axis_ready_callback)(void) = NULL;


int psd_param_init(void)
{
    spin_lock_init(&g_spinlock);
    g_drive_trim.throttle_percent = 100;
    g_mcu_state_request.state = psd_mcu_state_wakeup;
    g_mcu_state_request.is_via_reboot = false;
    g_init_state.state = PSD_INIT_STATE_NOT_INITIALIZED;
    g_settings.feedback.is_enable = true;
    g_settings.feedback.percent_p = 140;
    g_settings.feedback.permille_i = 3;
    g_settings.feedback.percent_d = 500;
    g_settings.feedback.integral_max = 300;
    g_settings.feedback.feedforward = 30;
    g_settings.fail_safe.is_motor_temp_enable = true;
    g_settings.fail_safe.is_stall_enable = true;

#ifdef FUJI_CONFIG_TYPE_SYSDEV
    g_settings.fail_safe.is_heart_beat_enable = false;
#else
    g_settings.fail_safe.is_heart_beat_enable = true;
#endif

    g_six_axis_data.buf = kmalloc(SIX_AXIS_BUFFER_RING_SIZE * sizeof(struct psd_six_axis), GFP_KERNEL);

    if(g_six_axis_data.buf == NULL)
    {
        PSD_LOG("Fail to allocate memory in psd_param_init\n");
        return -ENOMEM;
    }

    return 0;
}

struct psd_button psd_param_get_button(void)
{
    unsigned long flags;
    struct psd_button button;
    spin_lock_irqsave(&g_spinlock, flags);
    button = g_button;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return button;
}

void psd_param_set_button(const struct psd_button* p_button)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_button = *p_button;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

struct psd_charge_brake_led psd_param_get_charge_brake_led(void)
{
    unsigned long flags;
    struct psd_charge_brake_led charge_brake_led;
    spin_lock_irqsave(&g_spinlock, flags);
    charge_brake_led = g_charge_brake_led;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return charge_brake_led;
}

void psd_param_set_charge_brake_led(const struct psd_charge_brake_led* p_charge_brake_led)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_charge_brake_led = *p_charge_brake_led;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

struct psd_charge_state psd_param_get_charge_state(void)
{
    unsigned long flags;
    struct psd_charge_state charge_state;
    spin_lock_irqsave(&g_spinlock, flags);
    charge_state = g_charge_state;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return charge_state;
}

void psd_param_set_charge_state(const struct psd_charge_state* p_charge_state)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_charge_state = *p_charge_state;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

struct psd_drive_control psd_param_get_drive_control(void)
{
    unsigned long flags;
    struct psd_drive_control drive_control;
    spin_lock_irqsave(&g_spinlock, flags);
    drive_control = g_drive_control;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return drive_control;
}

void psd_param_set_drive_control(const struct psd_drive_control* p_drive_control)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_drive_control = *p_drive_control;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

struct psd_drive_trim psd_param_get_drive_trim(void)
{
    unsigned long flags;
    struct psd_drive_trim drive_trim;
    spin_lock_irqsave(&g_spinlock, flags);
    drive_trim = g_drive_trim;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return drive_trim;
}

void psd_param_set_drive_trim(const struct psd_drive_trim* p_drive_trim)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_drive_trim = *p_drive_trim;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

struct psd_power_sync_led psd_param_get_power_sync_led(void)
{
    unsigned long flags;
    struct psd_current_mcu_state current_mcu_state;
    spin_lock_irqsave(&g_spinlock, flags);
    current_mcu_state = g_current_mcu_state;
    spin_unlock_irqrestore(&g_spinlock, flags);

    struct psd_power_sync_led power_sync_led;
    switch(current_mcu_state.state)
    {
        case psd_mcu_state_wakeup :
            power_sync_led.state = PSD_PS_LED_BRINK_LOW;
            return power_sync_led;
        case psd_mcu_state_connected :
            power_sync_led.state = PSD_PS_LED_ON;
            return power_sync_led;
        case psd_mcu_state_pairing :
            power_sync_led.state = PSD_PS_LED_BRINK_HIGH;
            return power_sync_led;
        default :
            PSD_LOG("invalid mcu state in %s\n" ,__FUNCTION__);
            power_sync_led.state = PSD_PS_LED_OFF;
            return power_sync_led;
    }
}

void psd_param_set_current_mcu_state(const struct psd_current_mcu_state* p_current_mcu_state)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_current_mcu_state = *p_current_mcu_state;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

struct psd_current_mcu_state psd_param_get_current_mcu_state(void)
{
	unsigned long flags;
    struct psd_current_mcu_state state;
    spin_lock_irqsave(&g_spinlock, flags);
    state = g_current_mcu_state;
    spin_unlock_irqrestore(&g_spinlock, flags);

    return state;
}

void psd_param_set_current_mcu_iap_state(const struct psd_current_mcu_iap_state* p_current_mcu_iap_state)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_current_mcu_iap_state = *p_current_mcu_iap_state;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

struct psd_current_mcu_iap_state psd_param_get_current_mcu_iap_state(void)
{
    unsigned long flags;
    struct psd_current_mcu_iap_state state;
    spin_lock_irqsave(&g_spinlock, flags);
    state = g_current_mcu_iap_state;
    spin_unlock_irqrestore(&g_spinlock, flags);

    return state;
}

struct psd_mcu_state_request psd_param_peek_mcu_state_request(void)
{
    unsigned long flags;
    struct psd_mcu_state_request mcu_state_request;
    spin_lock_irqsave(&g_spinlock, flags);
    mcu_state_request = g_mcu_state_request;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return mcu_state_request;
}

struct psd_mcu_state_request psd_param_pop_mcu_state_request(void)
{
    unsigned long flags;
    struct psd_mcu_state_request mcu_state_request;
    spin_lock_irqsave(&g_spinlock, flags);
    mcu_state_request = g_mcu_state_request;
    if(g_mcu_state_request.is_via_reboot)
    {
        //リブート指示は、MCUへの到達確認ができない。
        //dirtyフラグを落とさないことで、リブートがかかるまでリブート指示をかけ続ける
    }
    else
    {
        g_mcu_state_request.is_dirty = false;
    }
    spin_unlock_irqrestore(&g_spinlock, flags);
    return mcu_state_request;
}

static void set_mcu_state_request(enum psd_mcu_state state, bool is_via_reboot)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    if(g_mcu_state_request.is_dirty)
    {
        //定期アップデート間隔内での多重要求に当たるため、優先度評価
        if(g_mcu_state_request.state == psd_mcu_state_standby)
        {
            //シャットダウン中は変更不可
        }
        else if(g_mcu_state_request.is_via_reboot)
        {
            //リブート中は変更不可
        }
        else
        {
            g_mcu_state_request.state = state;
        }
    }
    else
    {
        //多重要求では無いため、上書き
        g_mcu_state_request.state = state;
        g_mcu_state_request.is_via_reboot = is_via_reboot;
        g_mcu_state_request.is_dirty = true;
    }
    spin_unlock_irqrestore(&g_spinlock, flags);
}

void psd_param_set_power_sync_led(const struct psd_power_sync_led* p_power_sync_led)
{
    switch(p_power_sync_led->state)
    {
        case PSD_PS_LED_ON:
            set_mcu_state_request(psd_mcu_state_connected, false);
            return;
        case PSD_PS_LED_BRINK_LOW:
            set_mcu_state_request(psd_mcu_state_wakeup, false);
            return;
        case PSD_PS_LED_BRINK_HIGH:
            set_mcu_state_request(psd_mcu_state_pairing, false);
            return;
        default:
            PSD_LOG("invalid param in %s\n", __FUNCTION__);
            return;
    }
}

void psd_param_set_shutdown(const struct psd_shutdown* p_shutdown)
{
    switch(p_shutdown->command)
    {
        case PSD_SHUTDOWN_DOWN:
            set_mcu_state_request(psd_mcu_state_standby, false);
            return;
        case PSD_SHUTDOWN_REBOOT:
            set_mcu_state_request(psd_mcu_state_wakeup, true);
            return;
        default:
            PSD_LOG("invalid param in %s\n", __FUNCTION__);
    }
}

void psd_param_set_iap(void)
{
    set_mcu_state_request(psd_mcu_state_iap, false);
}

struct psd_mcu_fw_version psd_param_get_mcu_fw_version(void)
{
    unsigned long flags;
    struct psd_mcu_fw_version version;
    spin_lock_irqsave(&g_spinlock, flags);
    version = g_mcu_fw_version;
    spin_unlock_irqrestore(&g_spinlock, flags);

    return version;
}

void psd_param_set_mcu_fw_version(const struct psd_mcu_fw_version* p_mcu_fw_version)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_mcu_fw_version = *p_mcu_fw_version;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

static void apply_six_axis_offset(struct psd_six_axis data[], size_t count)
{
    struct psd_six_axis_offset offset = {0};
    offset = psd_param_get_six_axis_offset();

    const int s16_min = -32768;
    const int s16_max =  32767;
    for(int i=0;i<count;i++)
    {
        data[i].acceleration_x = 
        (s16)psd_util_minmax(s16_min, data[i].acceleration_x + offset.acceleration_x, s16_max);
        
        data[i].acceleration_y = 
        (s16)psd_util_minmax(s16_min, data[i].acceleration_y + offset.acceleration_y, s16_max);

        data[i].acceleration_z = 
        (s16)psd_util_minmax(s16_min, data[i].acceleration_z + offset.acceleration_z, s16_max);

        data[i].angular_velocity_x = 
        (s16)psd_util_minmax(s16_min, data[i].angular_velocity_x + offset.angular_velocity_x, s16_max);

        data[i].angular_velocity_y = 
        (s16)psd_util_minmax(s16_min, data[i].angular_velocity_y + offset.angular_velocity_y, s16_max);

        data[i].angular_velocity_z = 
        (s16)psd_util_minmax(s16_min, data[i].angular_velocity_z + offset.angular_velocity_z, s16_max);
    }
}

size_t psd_param_get_six_axis(struct psd_six_axis* p_six_axis_buffer, size_t max_count)
{
    spin_lock(&g_spinlock);

    int entry_count = CIRC_CNT(g_six_axis_data.head, g_six_axis_data.tail, SIX_AXIS_BUFFER_RING_SIZE);
    int count = psd_util_min(max_count, entry_count);

    if (count == 0) {
        spin_unlock(&g_spinlock);
        return 0;
    }
    int copy_count = psd_util_min(count,
        CIRC_CNT_TO_END(g_six_axis_data.head, g_six_axis_data.tail, SIX_AXIS_BUFFER_RING_SIZE));

    struct psd_six_axis *buf = (struct psd_six_axis*)g_six_axis_data.buf;

    memcpy(p_six_axis_buffer, &buf[g_six_axis_data.tail],
           copy_count * sizeof(struct psd_six_axis));

    if (copy_count < count) {
        memcpy(p_six_axis_buffer+copy_count, &buf[0],
               (count - copy_count) * sizeof(struct psd_six_axis));
    }

    g_six_axis_data.tail = (g_six_axis_data.tail + count) & (SIX_AXIS_BUFFER_RING_SIZE - 1);

    spin_unlock(&g_spinlock);

    apply_six_axis_offset(p_six_axis_buffer, count);

    return count;
}

size_t psd_param_get_six_axis_count(void)
{
    int entry_count = 0;

    spin_lock(&g_spinlock);
    entry_count = CIRC_CNT(g_six_axis_data.head, g_six_axis_data.tail, SIX_AXIS_BUFFER_RING_SIZE);
    spin_unlock(&g_spinlock);

    return entry_count;
}

void psd_param_add_six_axis(const struct psd_six_axis six_axis[], int count)
{
    if (count == 0) return;

    spin_lock(&g_spinlock);

    int free_space = CIRC_SPACE(g_six_axis_data.head, g_six_axis_data.tail, SIX_AXIS_BUFFER_RING_SIZE);

    if (free_space < count)
    {
        int dropped = count - free_space;
        g_six_axis_data.tail =
            (g_six_axis_data.tail + dropped) & (SIX_AXIS_BUFFER_RING_SIZE - 1);
    }


    int copy_count = psd_util_min(count,
        CIRC_SPACE_TO_END(g_six_axis_data.head, g_six_axis_data.tail, SIX_AXIS_BUFFER_RING_SIZE));

    struct psd_six_axis *buf = (struct psd_six_axis*)g_six_axis_data.buf;

    memcpy(&buf[g_six_axis_data.head], six_axis,
           copy_count * sizeof(struct psd_six_axis));

    if (copy_count < count) {
        memcpy(&buf[0], &six_axis[copy_count],
               (count - copy_count) * sizeof(struct psd_six_axis));
    }

    g_six_axis_data.head = (g_six_axis_data.head + count) & (SIX_AXIS_BUFFER_RING_SIZE - 1);
    spin_unlock(&g_spinlock);

    if(g_six_axis_ready_callback)
    {
        g_six_axis_ready_callback();
    }
}

void psd_param_set_six_axis_ready_callback(void (*callback)(void))
{
    g_six_axis_ready_callback = callback;
}

struct psd_six_axis_offset psd_param_get_six_axis_offset(void)
{
    unsigned long flags;
    struct psd_six_axis_offset six_axis_offset;
    spin_lock_irqsave(&g_spinlock, flags);
    six_axis_offset = g_six_axis_offset;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return six_axis_offset;
}

void psd_param_set_six_axis_offset(const struct psd_six_axis_offset* p_six_axis_offset)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_six_axis_offset = *p_six_axis_offset;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

struct psd_voltage psd_param_get_voltage(void)
{
    unsigned long flags;
    struct psd_voltage voltage;
    spin_lock_irqsave(&g_spinlock, flags);
    voltage = g_voltage;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return voltage;
}

void psd_param_set_voltage(const struct psd_voltage* p_voltage)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_voltage = *p_voltage;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

struct psd_feedback psd_param_get_feedback_setting(void)
{
    unsigned long flags;
    struct psd_feedback feedback;
    spin_lock_irqsave(&g_spinlock, flags);
    feedback = g_settings.feedback;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return feedback;
}

void psd_param_set_feedback_setting(const struct psd_feedback* p_feedback)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_settings.feedback = *p_feedback;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

struct psd_drive_log psd_param_get_drive_log_setting(void)
{
    unsigned long flags;
    struct psd_drive_log drive_log;
    spin_lock_irqsave(&g_spinlock, flags);
    drive_log = g_settings.drive_log;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return drive_log;
}

void psd_param_set_drive_log_setting(const struct psd_drive_log* p_drive_log)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_settings.drive_log = *p_drive_log;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

struct psd_fail_safe psd_param_get_fail_safe_setting(void)
{
    unsigned long flags;
    struct psd_fail_safe fail_safe;
    spin_lock_irqsave(&g_spinlock, flags);
    fail_safe = g_settings.fail_safe;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return fail_safe;
}

void psd_param_set_fail_safe_setting(const struct psd_fail_safe* p_fail_safe)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_settings.fail_safe = *p_fail_safe;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

bool psd_param_is_heart_beat_fail_safe_setting_enable(void)
{
    unsigned long flags;
    bool is_enable;
    spin_lock_irqsave(&g_spinlock, flags);
    is_enable = g_settings.fail_safe.is_heart_beat_enable;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return is_enable;
}

void psd_param_set_heart_beat_fail_safe_setting(bool is_enable)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_settings.fail_safe.is_heart_beat_enable = is_enable;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

struct psd_init_state psd_param_get_init_state(void)
{
    unsigned long flags;
    struct psd_init_state init_state;
    spin_lock_irqsave(&g_spinlock, flags);
    init_state = g_init_state;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return init_state;
}

void psd_param_set_init_state(const struct psd_init_state* p_init_state)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_init_state = *p_init_state;
    spin_unlock_irqrestore(&g_spinlock, flags);
}

struct psd_mcu_clock_correction psd_param_get_mcu_clock_correction(void)
{
    unsigned long flags;
    struct psd_mcu_clock_correction mcu_clock_correction;
    spin_lock_irqsave(&g_spinlock, flags);
    mcu_clock_correction = g_mcu_clock_correction;
    spin_unlock_irqrestore(&g_spinlock, flags);
    return mcu_clock_correction;
}

void psd_param_set_clock_correction(const struct psd_mcu_clock_correction* p_mcu_clock_correction)
{
    unsigned long flags;
    spin_lock_irqsave(&g_spinlock, flags);
    g_mcu_clock_correction = *p_mcu_clock_correction;
    spin_unlock_irqrestore(&g_spinlock, flags);
}