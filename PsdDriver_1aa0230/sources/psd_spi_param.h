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

#ifndef PSD_SPI_PARAM_H_
#define PSD_SPI_PARAM_H_

void psd_spi_param_generate_cd(void* p_buffer, size_t size);
bool psd_spi_param_receive_cd(const void* p_buffer, size_t size);

void psd_spi_param_generate_ss(void* p_buffer, size_t size);
void psd_spi_param_generate_flash_erase(void* p_buffer, size_t size);
void psd_spi_param_generate_flash_write(void* p_buffer, size_t size, u32 address, u8* data, u8 data_size);
void psd_spi_param_generate_flash_verify(void* p_buffer, size_t size, u16 data_size, u32 crc);
void psd_spi_param_generate_iap_reboot(void* p_buffer, size_t size);

#endif
