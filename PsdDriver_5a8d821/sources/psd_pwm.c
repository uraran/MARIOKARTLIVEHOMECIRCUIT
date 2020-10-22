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

#include <linux/platform_device.h>
#include <linux/pwm.h>
#include "psd_pwm.h"
#include "psd_util.h"

#define PSD_PWM_DEVICE_COUNT_MAX 2
struct device* g_p_device[PSD_PWM_DEVICE_COUNT_MAX];

static const struct of_device_id g_of_device_id[] = {
    {.compatible = "Realtek,motor0",},
    {.compatible = "Realtek,motor1",},
    {},
};

#define PSD_PWM_NAME_SIZE 6
static u8 get_device_index(const struct platform_device* p_platform_device)
{
    char name[PSD_PWM_DEVICE_COUNT_MAX][PSD_PWM_NAME_SIZE] = {"motor0", "motor1"};

    for(int i=0;i < PSD_PWM_DEVICE_COUNT_MAX;i++)
    {
        int result;
        result = strncmp(&p_platform_device->name[0], &name[i][0], PSD_PWM_NAME_SIZE);
        if(result == 0)
        {
            return i;
        }
    }
    return 0xFF;
}

static int probed_cb(struct platform_device* p_platform_device)
{
    PSD_LOG("%s name = %s\n",__FUNCTION__, p_platform_device->name);

    u8 index;
    index = get_device_index(p_platform_device);
    if(index >= PSD_PWM_DEVICE_COUNT_MAX)
    {
        PSD_LOG("invalid device index in %s. pwm = %d\n", __FUNCTION__, index);
        return -1;
    }

    g_p_device[index] = &p_platform_device->dev;

    struct pwm_device* p_device;
    p_device = pwm_get(g_p_device[index], NULL);

    pwm_enable(p_device);

    int period_ns;
    period_ns = pwm_get_period(p_device);
    PSD_LOG("pwm_period_ns = %d\n", period_ns);

    int duty_ns = 0;
    pwm_config(p_device, duty_ns, period_ns);

    pwm_put(p_device);
    return 0;
}

static int removed_cb(struct platform_device* p_platform_device)
{
    PSD_LOG("%s\n",__FUNCTION__);
    return 0;
}

static struct platform_driver g_pwm_driver = {
    .probe  = probed_cb,
    .remove = removed_cb,
    .driver = {
        .name   = "psd_pwm",
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(g_of_device_id),
    },
};

int psd_pwm_init(void)
{
    PSD_LOG("%s\n",__FUNCTION__);
    return platform_driver_probe(&g_pwm_driver, probed_cb);
}

void psd_pwm_exit(void)
{
    PSD_LOG("%s\n",__FUNCTION__);
    platform_driver_unregister(&g_pwm_driver);
}

void psd_pwm_set_duty(u8 device_index, struct psd_util_ratio ratio)
{
    if(device_index >= PSD_PWM_DEVICE_COUNT_MAX)
    {
        PSD_LOG("invalid device index in %s. pwm = %d\n", __FUNCTION__, device_index);
        return;
    }

    struct pwm_device* p_device;
    p_device = pwm_get(g_p_device[device_index], NULL);

    int period_ns;
    period_ns = pwm_get_period(p_device);

    int duty_ns;
    duty_ns = psd_util_apply_ratio(period_ns, ratio);
    pwm_config(p_device, duty_ns, period_ns);
    pwm_put(p_device);
}