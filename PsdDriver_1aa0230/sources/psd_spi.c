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

#include <linux/spi/spi.h>
#include "psd_spi.h"
#include "psd_spi_param.h"
#include "psd_util.h"

#define PSD_SPI_TX_SIZE    8
#define PSD_SPI_RX_SIZE    128
#define PSD_SPI_CD_TX_SIZE 8
#define PSD_SPI_CD_RX_SIZE 128
#define PSD_SPI_SS_TX_SIZE 8

// for IAP
#define PSD_SPI_IAP_FLASH_ERASE_TX_SIZE 8
#define PSD_SPI_IAP_WRITE_TX_SIZE 136
#define PSD_SPI_IAP_VERIFY_TX_SIZE 8

static int probed_cb(struct spi_device* p_spi_device);
static int removed_cb(struct spi_device* p_spi_device);

static struct spi_board_info g_spi_board_info = {
    .modalias = "psd_spi",
    .max_speed_hz = 14000000,
    .bus_num = 0,
    .chip_select = 0,
    .mode = SPI_MODE_0,
};

static struct spi_device_id g_spi_device_id[] = {
    {"psd_spi", 0},
    {},
};

static struct spi_driver g_spi_driver = {
    .driver = {
        .name = "psd_spi",
        .owner = THIS_MODULE,
    },
    .id_table = g_spi_device_id,
    .probe  = probed_cb,
    .remove = removed_cb,
};

struct spi_data
{
    struct spi_device* p_spi_device;
    unsigned char tx[PSD_SPI_TX_SIZE];
    unsigned char rx[PSD_SPI_RX_SIZE];
};

static struct spi_data* g_p_spi_data;

static int setup_bus(struct spi_device* p_spi_device)
{
    p_spi_device->max_speed_hz = g_spi_board_info.max_speed_hz;
    p_spi_device->mode = g_spi_board_info.mode;
    p_spi_device->bits_per_word = 8;
    int result = spi_setup(p_spi_device);
    if(result == 0)
    {
        return 0;
    }
    else
    {
        PSD_LOG("spi_setup returned error\n");
        return -ENODEV;
    }
}

static int setup_data(struct spi_device* p_spi_device)
{
    g_p_spi_data = kzalloc(sizeof(struct spi_data), GFP_KERNEL);
    if(g_p_spi_data == NULL)
    {
        PSD_LOG("kzalloc returned error\n");
        return -ENODEV;
    }
    else
    {
        g_p_spi_data->p_spi_device = p_spi_device;
        return 0;
    }
}

static int probed_cb(struct spi_device* p_spi_device)
{
    PSD_LOG("%s\n",__FUNCTION__);
    int result;

    result = setup_bus(p_spi_device);
    if(result != 0)
    {
        return result;
    }

    return setup_data(p_spi_device);
}

static int removed_cb(struct spi_device* p_spi_device)
{
    PSD_LOG("%s\n",__FUNCTION__);
    kfree(g_p_spi_data);
    return 0;
}

static void remove_device(const struct spi_master* p_spi_master)
{
    PSD_LOG("%s\n", __FUNCTION__);
    struct device* p_device;
    char str[128];
    snprintf(str, sizeof(str), "%s.%u", dev_name(&p_spi_master->dev), g_spi_board_info.chip_select);
    p_device = bus_find_device_by_name(&spi_bus_type, NULL, str);//spi_bus_typeの出所が不明
    if(p_device != NULL)
    {
        PSD_LOG("%s removed\n", &str[0]);
        device_del(p_device);
    }
}

static void setup_txfer_for_check_data(struct spi_transfer* p_xfer)
{
    p_xfer->bits_per_word = 8;
    p_xfer->cs_change = 0;
    p_xfer->delay_usecs = 1;
    p_xfer->speed_hz = 14000000;

    psd_spi_param_generate_cd(&g_p_spi_data->tx[0], PSD_SPI_CD_TX_SIZE);

    p_xfer->tx_buf = g_p_spi_data->tx;
    p_xfer->rx_buf = NULL;
    p_xfer->len = PSD_SPI_CD_TX_SIZE;
}

static void setup_txfer_for_state_set(struct spi_transfer* p_xfer)
{
    p_xfer->bits_per_word = 8;
    p_xfer->cs_change = 0;
    p_xfer->delay_usecs = 1;
    p_xfer->speed_hz = 14000000;

    psd_spi_param_generate_ss(&g_p_spi_data->tx[0], PSD_SPI_SS_TX_SIZE);

    p_xfer->tx_buf = g_p_spi_data->tx;
    p_xfer->rx_buf = NULL;
    p_xfer->len = PSD_SPI_SS_TX_SIZE;
}

static void setup_txfer_for_iap_flash_erase(struct spi_transfer* p_xfer)
{
    p_xfer->bits_per_word = 8;
    p_xfer->cs_change = 0;
    p_xfer->delay_usecs = 1;
    p_xfer->speed_hz = 14000000;

    psd_spi_param_generate_flash_erase(&g_p_spi_data->tx[0], PSD_SPI_IAP_FLASH_ERASE_TX_SIZE);

    p_xfer->tx_buf = g_p_spi_data->tx;
    p_xfer->rx_buf = NULL;
    p_xfer->len = PSD_SPI_IAP_FLASH_ERASE_TX_SIZE;
}

void setup_txfer_for_iap_write(struct spi_transfer* p_xfer, u32 address, u8* data, u8 size)
{
    p_xfer->bits_per_word = 8;
    p_xfer->cs_change = 0;
    p_xfer->delay_usecs = 1;
    p_xfer->speed_hz = 14000000;

    psd_spi_param_generate_flash_write(&g_p_spi_data->tx[0], PSD_SPI_IAP_WRITE_TX_SIZE, address, data, size);

    p_xfer->tx_buf = g_p_spi_data->tx;
    p_xfer->rx_buf = NULL;
    p_xfer->len = PSD_SPI_IAP_WRITE_TX_SIZE;
}

void setup_txfer_for_iap_verify(struct spi_transfer* p_xfer, u16 data_size, u32 crc)
{
    p_xfer->bits_per_word = 8;
    p_xfer->cs_change = 0;
    p_xfer->delay_usecs = 1;
    p_xfer->speed_hz = 14000000;

    psd_spi_param_generate_flash_verify(&g_p_spi_data->tx[0], PSD_SPI_IAP_VERIFY_TX_SIZE, data_size, crc);

    p_xfer->tx_buf = g_p_spi_data->tx;
    p_xfer->rx_buf = NULL;
    p_xfer->len = PSD_SPI_IAP_VERIFY_TX_SIZE;
}

void setup_txfer_for_iap_reboot(struct spi_transfer* p_xfer)
{
    p_xfer->bits_per_word = 8;
    p_xfer->cs_change = 0;
    p_xfer->delay_usecs = 1;
    p_xfer->speed_hz = 14000000;

    psd_spi_param_generate_iap_reboot(&g_p_spi_data->tx[0], PSD_SPI_IAP_VERIFY_TX_SIZE);

    p_xfer->tx_buf = g_p_spi_data->tx;
    p_xfer->rx_buf = NULL;
    p_xfer->len = PSD_SPI_IAP_VERIFY_TX_SIZE;
}

static void setup_rxfer_for_check_data(struct spi_transfer* p_xfer)
{
    p_xfer->bits_per_word = 8;
    p_xfer->cs_change = 0;
    p_xfer->delay_usecs = 1;
    p_xfer->speed_hz = 14000000;

    memset(&g_p_spi_data->rx[0], 0, PSD_SPI_CD_RX_SIZE);
    p_xfer->tx_buf = NULL;
    p_xfer->rx_buf = g_p_spi_data->rx;
    p_xfer->len = PSD_SPI_CD_RX_SIZE;
}

bool psd_spi_check_data(void)
{
    struct spi_transfer txfer = {0};
    struct spi_transfer rxfer = {0};
    setup_txfer_for_check_data(&txfer);
    setup_rxfer_for_check_data(&rxfer);

    struct spi_message message;
    spi_message_init(&message);
    spi_message_add_tail(&txfer, &message);
    spi_message_add_tail(&rxfer, &message);

    int result;
    result = spi_sync(g_p_spi_data->p_spi_device, &message);
    if(result != 0)
    {
        PSD_LOG("spi_sync error in %s\n", __FUNCTION__);
        return 0;
    }
    else
    {
        /*for(int i=0;i<PSD_SPI_CD_TX_SIZE;i++)
        {
            PSD_LOG("txbuf[%d] = %d\n", i, g_p_spi_data->tx[i]);
        }
        for(int i=0;i<PSD_SPI_CD_RX_SIZE;i++)
        {
            PSD_LOG("rxbuf[%d] = %d\n", i, g_p_spi_data->rx[i]);
        }*/
        return psd_spi_param_receive_cd(&g_p_spi_data->rx[0], PSD_SPI_CD_RX_SIZE);
    }
}

void psd_spi_set_state(void)
{
    struct spi_transfer txfer = {0};
    setup_txfer_for_state_set(&txfer);

    struct spi_message message;
    spi_message_init(&message);
    spi_message_add_tail(&txfer, &message);

    int result;
    result = spi_sync(g_p_spi_data->p_spi_device, &message);
    if(result != 0)
    {
        PSD_LOG("spi_sync error in %s\n", __FUNCTION__);
    }
}

void psd_spi_iap_flash_erase(void)
{
    struct spi_transfer txfer = {0};
    setup_txfer_for_iap_flash_erase(&txfer);

    struct spi_message message;
    spi_message_init(&message);
    spi_message_add_tail(&txfer, &message);

    int result;
    result = spi_sync(g_p_spi_data->p_spi_device, &message);
    if(result != 0)
    {
        PSD_LOG("spi_sync error in %s\n", __FUNCTION__);
    }
}

void psd_spi_iap_write(u32 address, u8* data, u8 size)
{
    struct spi_transfer txfer = {0};
    setup_txfer_for_iap_write(&txfer, address, data, size);

    struct spi_message message;
    spi_message_init(&message);
    spi_message_add_tail(&txfer, &message);

    int result;
    result = spi_sync(g_p_spi_data->p_spi_device, &message);
    if(result != 0)
    {
        PSD_LOG("spi_sync error in %s\n", __FUNCTION__);
    }
}

void psd_spi_iap_verify(u16 data_size, u32 crc)
{
    struct spi_transfer txfer = {0};
    setup_txfer_for_iap_verify(&txfer, data_size, crc);

    struct spi_message message;
    spi_message_init(&message);
    spi_message_add_tail(&txfer, &message);

    PSD_LOG("send verify in %s\n", __FUNCTION__);

    int result;
    result = spi_sync(g_p_spi_data->p_spi_device, &message);
    if(result != 0)
    {
        PSD_LOG("spi_sync error in %s\n", __FUNCTION__);
    }
}

void psd_spi_iap_reboot(void)
{
    struct spi_transfer txfer = {0};
    setup_txfer_for_iap_reboot(&txfer);

    struct spi_message message;
    spi_message_init(&message);
    spi_message_add_tail(&txfer, &message);

    PSD_LOG("send verify in %s\n", __FUNCTION__);

    int result;
    result = spi_sync(g_p_spi_data->p_spi_device, &message);
    if(result != 0)
    {
        PSD_LOG("spi_sync error in %s\n", __FUNCTION__);
    }
}

int psd_spi_init(void)
{
    PSD_LOG("%s\n", __FUNCTION__);
    spi_register_driver(&g_spi_driver);
    struct spi_master* p_spi_master;
    struct spi_device* p_spi_device;
    p_spi_master = spi_busnum_to_master(g_spi_board_info.bus_num);
    if(p_spi_master == NULL)
    {
        PSD_LOG("spi_busnum_to_master null\n");
        spi_unregister_driver(&g_spi_driver);
        return -1;
    }

    p_spi_device = spi_new_device(p_spi_master, &g_spi_board_info);
    if(p_spi_device == NULL)
    {
        PSD_LOG("spi_new_device null\n");
        spi_unregister_driver(&g_spi_driver);
        return -1;
    }
    return 0;
}

void psd_spi_exit(void)
{
    PSD_LOG("%s\n", __FUNCTION__);
    struct spi_master* p_spi_master;
    p_spi_master = spi_busnum_to_master(g_spi_board_info.bus_num);
    if(p_spi_master == NULL)
    {
        PSD_LOG("spi_busnum_to_master null\n");
    }
    else
    {
        remove_device(p_spi_master);
    }
    spi_unregister_driver(&g_spi_driver);
}

