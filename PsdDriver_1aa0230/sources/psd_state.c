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
#include <linux/delay.h>
#include "psd_battery.h"
#include "psd_drive.h"
#include "psd_heart_beat.h"
#include "psd_mcu_update.h"
#include "psd_param.h"
#include "psd_spi.h"
#include "psd_types.h"
#include "psd_util.h"

static void test(void)
{
    static unsigned long count = 0;
    if(count == 0)
    {
        PSD_LOG("initial update\n");
#ifdef FUJI_BOARD_TYPE_EP
        PSD_LOG("FUJI_BOARD_TYPE_EP \n");
#else
        PSD_LOG("FUJI_BOARD_TYPE_XX \n");
#endif
    }
    count++;
}

static void update_param_from_mcu(void)
{
    //設定の変化から、変更のトリガを生成するために、毎回設定する
    bool is_enable;
    is_enable = psd_param_is_heart_beat_fail_safe_setting_enable();
    psd_heart_beat_enable(is_enable);

    if(!psd_heart_beat_is_alive())
    {
        return;
    }

    while(psd_spi_check_data())
    {
        ndelay(200000);
    }
}

static void set_mcu_state(void)
{
    struct psd_mcu_state_request mcu_state_request;
    struct psd_current_mcu_state mcu_state;

    mcu_state_request = psd_param_peek_mcu_state_request();
    mcu_state = psd_param_get_current_mcu_state();
    if(mcu_state_request.is_dirty)
    {
        //requestの変更があったため、実行
        ndelay(100000);//SPI通信が連続すると、MCU側で受信できないため、delay
        psd_spi_set_state();
    }
    else if(mcu_state_request.state != mcu_state.state)
    {
        //state差があった（下層要因でステートが変わった）ため、実行
        ndelay(100000);//SPI通信が連続すると、MCU側で受信できないため、delay
        psd_spi_set_state();
    }
    else
    {
        //条件外では何もしない
    }
}

static void set_motor_state(void)
{
    psd_drive_update();
}

static void update_battery_state(void)
{
    static int count = 0;
    if(count % 10 == 0)
    {
        psd_battery_update();
    }
    count++;
}

void psd_state_update(void)
{
    test();
    update_param_from_mcu();

    struct psd_current_mcu_state mcu_state = psd_param_get_current_mcu_state();

    // mcu が IAP の時は定期処理で state の更新は行わない
    if(mcu_state.state != psd_mcu_state_iap)
    {
        set_mcu_state();
    }
    else
    {
        // IAP の時は IAP 用のコマンド処理をここで実行する
        ndelay(100000);//SPI通信が連続すると、MCU側で受信できないため、delay
        psd_mcu_update_execute();
    }
    set_motor_state();
    update_battery_state();
}