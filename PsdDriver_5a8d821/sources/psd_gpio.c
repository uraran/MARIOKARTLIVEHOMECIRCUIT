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

#include <linux/gpio.h>
#include <asm/gpio.h>
#include "psd_gpio.h"
#include "psd_util.h"

int psd_gpio_init(void)
{
    int result;
    result = gpio_request_one(PSD_GPIO_MD_SR, GPIOF_OUT_INIT_LOW, NULL);
    if(result != 0)
    {
        PSD_LOG("gpio_request_one(%d) = %d\n",PSD_GPIO_MD_SR, result);
        return result;
    }

    result = gpio_request_one(PSD_GPIO_MD_PH_A, GPIOF_OUT_INIT_LOW, NULL);
    if(result != 0)
    {
        PSD_LOG("gpio_request_one(%d) = %d\n",PSD_GPIO_MD_PH_A, result);
        return result;
    }

    result = gpio_request_one(PSD_GPIO_MD_PH_A_DP, GPIOF_OUT_INIT_LOW, NULL);
    if(result != 0)
    {
        PSD_LOG("gpio_request_one(%d) = %d\n",PSD_GPIO_MD_PH_A_DP, result);
        return result;
    }

    result = gpio_request_one(PSD_GPIO_MD_EN_A, GPIOF_OUT_INIT_LOW, NULL);
    if(result != 0)
    {
        PSD_LOG("gpio_request_one(%d) = %d\n",PSD_GPIO_MD_EN_A, result);
        return result;
    }

    result = gpio_request_one(PSD_GPIO_MD_EN_IN_A, GPIOF_IN, NULL);
    if(result != 0)
    {
        PSD_LOG("gpio_request_one(%d) = %d\n",PSD_GPIO_MD_EN_IN_A, result);
        return result;
    }

    result = gpio_request_one(PSD_GPIO_MD_PH_B, GPIOF_OUT_INIT_LOW, NULL);
    if(result != 0)
    {
        PSD_LOG("gpio_request_one(%d) = %d\n",PSD_GPIO_MD_PH_B, result);
        return result;
    }

    result = gpio_request_one(PSD_GPIO_MD_EN_B, GPIOF_OUT_INIT_LOW, NULL);
    if(result != 0)
    {
        PSD_LOG("gpio_request_one(%d) = %d\n",PSD_GPIO_MD_EN_B, result);
        return result;
    }

    result = gpio_request_one(PSD_GPIO_MD_EN_IN_B, GPIOF_IN, NULL);
    if(result != 0)
    {
        PSD_LOG("gpio_request_one(%d) = %d\n",PSD_GPIO_MD_EN_IN_B, result);
        return result;
    }

    return 0;
}

void psd_gpio_exit(void)
{
    gpio_free(PSD_GPIO_MD_SR);
    gpio_free(PSD_GPIO_MD_PH_A);
    gpio_free(PSD_GPIO_MD_PH_A_DP);
    gpio_free(PSD_GPIO_MD_EN_A);
    gpio_free(PSD_GPIO_MD_EN_IN_A);
    gpio_free(PSD_GPIO_MD_PH_B);
    gpio_free(PSD_GPIO_MD_EN_B);
    gpio_free(PSD_GPIO_MD_EN_IN_B);
}

void psd_gpio_set_pin(int pin, int value)
{
    gpio_set_value(pin, value);
}