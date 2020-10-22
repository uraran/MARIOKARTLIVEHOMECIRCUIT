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

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include "psd_drive.h"
#include "psd_internal_types.h"
#include "psd_mcu_update.h"
#include "psd_param.h"
#include "psd_spi.h"
#include "psd_types.h"
#include "psd_util.h"

#define MCU_FW_MAX_TRANSFER_SIZE 126

static const u32 g_fw_image_address = 0x08001000;

static u8 g_fw_image_buffer[MCU_FW_MAX_TRANSFER_SIZE];
static u8 g_fw_image_write_size;
static u32 g_toral_fw_image_crc;
static u8* g_fw_buffer_pointer;
static u16 g_fw_total_size;
static bool g_is_allocated = false;
static u16 g_transferrd_size = 0;
static u16 g_transfer_number = 0;
static bool g_is_fw_transfer_completed = false;
static u16 g_write_finish_size = 0;
static bool g_is_fw_write_completed = false;

#define OPTIMIZE_LV     1

__u32 crc32b(__u8 *data, __u32 size)
{
    register __u32 len = size;
    register __u32 k = 0;
    register __u32 polynominal = 0xEDB88320;
    register __u32 crc = 0xFFFFFFFF;
    register __u8 *p = data;

    while (len--) {
        crc ^= *(volatile __u8 *)p++;
        for (k=0; k<8; k++) {
#if   OPTIMIZE_LV == 0
            if (crc & 1)
                crc = (crc >> 1) ^ polynominal;
            else
                crc = crc >> 1;
#elif OPTIMIZE_LV == 1
            crc = (crc >> 1) ^ (crc & 1) * polynominal;
#endif
        }
    }

    return crc ^ 0xFFFFFFFF;
}

void psd_mcu_update_append_fw(struct psd_mcu_fw_data data)
{
    // 最初のデータの時に領域をアロケート
    if(data.fw_transfer_number == 0 && !g_is_allocated)
    {
        PSD_LOG("vmalloc =%d\n",MCU_FW_MAX_SIZE);
        g_fw_buffer_pointer = vmalloc(MCU_FW_MAX_SIZE);
        if(g_fw_buffer_pointer != NULL)
        {
            PSD_LOG("vmalloc success\n");
        }
        else
        {
            PSD_LOG("vmalloc fail\n");
        }
        
        g_fw_total_size = data.fw_total_size;
        PSD_LOG("total size = %d \n", g_fw_total_size);
        g_is_allocated = true;
    }

	if(g_transfer_number == data.fw_transfer_number)
    {
        PSD_LOG("transfer size %d\n", data.fw_transfer_size);
        PSD_LOG("transferred data %d / %d \n", g_transferrd_size, g_fw_total_size);
        for (int i = 0 ; i < data.fw_transfer_size; i++)
        {
    	    g_fw_buffer_pointer[i + g_transferrd_size] = data.fw_image[i];
        }

        g_transferrd_size += data.fw_transfer_size;
        g_transfer_number = data.fw_transfer_number + 1;
    }
    else
    {
        PSD_LOG("wrong number %d\n", data.fw_transfer_number);
    }
    
    // FW の転送が終わったら、各種情報のチェック
    if(g_transferrd_size == g_fw_total_size)
    {
        g_toral_fw_image_crc = ((g_fw_buffer_pointer[5] << 24) | (g_fw_buffer_pointer[4] << 16) | (g_fw_buffer_pointer[3] << 8) | (g_fw_buffer_pointer[2]));

        PSD_LOG("Transferred size = %d byte\n", g_transferrd_size);
        PSD_LOG("Mcu FW Size in header = %d byte\n", (g_fw_buffer_pointer[1] << 8 | g_fw_buffer_pointer[0]));
        PSD_LOG("Mcu FW CRC in header  = %x \n", g_toral_fw_image_crc);
        PSD_LOG("caluculated Mcu CRC32 = %x \n", crc32b((g_fw_buffer_pointer + 8), g_transferrd_size - 8));

        // FW の転送完了フラグを立てる
        g_is_fw_transfer_completed = true;
    }

    // 最後まで転送できたら、アップデートスタート
}

void psd_mcu_update_execute(void)
{
    // FW の転送が完了している時だけ実行する
    if(g_is_fw_transfer_completed)
    {
        struct psd_current_mcu_iap_state state;

        // ステートの確認
        state = psd_param_get_current_mcu_iap_state();

        switch(state.state)
        {
        case psd_mcu_iap_state_idle:
            // 初期状態なので FW の Erase から始める
            psd_spi_iap_flash_erase();
            break;

        case psd_mcu_iap_state_erasing:
            // FW の消去中。ログだけ出す。
            PSD_LOG("Erasing Mcu FW\n");
            break;

        case psd_mcu_iap_state_erased:
        case psd_mcu_iap_state_wrote:

            if(!g_is_fw_write_completed)
            {
                PSD_LOG("Write Mcu FW\n");
                // 消去が完了 or 書き込み完了なら次の FW の書き込みを開始する
                // 転送バッファにデータをコピー
                g_fw_image_write_size = MCU_FW_MAX_TRANSFER_SIZE;
                if(g_fw_total_size - MCU_FW_HEADER_SIZE < g_write_finish_size + MCU_FW_MAX_TRANSFER_SIZE)
                {
                    g_fw_image_write_size = g_fw_total_size - MCU_FW_HEADER_SIZE - g_write_finish_size;
                    g_is_fw_write_completed = true;
                }

                for (int i = 0; i < g_fw_image_write_size; i++)
                {
                    g_fw_image_buffer[i] = g_fw_buffer_pointer[i + MCU_FW_HEADER_SIZE + g_write_finish_size];
                }

                psd_spi_iap_write(g_fw_image_address + g_write_finish_size, g_fw_image_buffer, g_fw_image_write_size);
                g_write_finish_size += g_fw_image_write_size;
            }
            else
            {
                // FW の書き込み完了状態ならベリファイを実施する
                psd_spi_iap_verify(g_fw_total_size - MCU_FW_HEADER_SIZE, g_toral_fw_image_crc);
            }
            
            break;

        case psd_mcu_iap_state_writing:
            // FW の書き込み中。ログだけ出す。
            PSD_LOG("Writing Mcu FW\n");
            break;

        case psd_mcu_iap_state_verifying:
            // FW のベリファイ中。ログだけ出す。
            PSD_LOG("Verifying Mcu FW\n");
            break;

        case psd_mcu_iap_state_verified:
            // FW のベリファイ完了。リブートする。
            PSD_LOG("Reboot Mcu FW\n");
            //vfree(g_fw_buffer_pointer);//リブートが落ちると多重開放になるので、解放しない
            psd_spi_iap_reboot();
            break;

        case psd_mcu_iap_state_error_fw_size:
            printk(KERN_ERR "Mcu FW size Error\n");
            psd_spi_iap_reboot();
            break;
        
        case psd_mcu_iap_state_error_fw_crc32:
            printk(KERN_ERR "Mcu FW crc Error\n");
            psd_spi_iap_reboot();
            break;

        default:
            printk(KERN_ERR "FW State Error %d\n", state.state);
            psd_spi_iap_reboot();
            break;
        }
    }
}