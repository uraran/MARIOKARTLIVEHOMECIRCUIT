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

#ifndef PSD_SPI_H_
#define PSD_SPI_H_

int psd_spi_init(void);
void psd_spi_exit(void);
bool psd_spi_check_data(void);
void psd_spi_set_state(void);
void psd_spi_iap_flash_erase(void);
void psd_spi_iap_write(u32 address, u8* data, u8 size);
void psd_spi_iap_verify(u16 data_size, u32 crc);
void psd_spi_iap_reboot(void);

#endif