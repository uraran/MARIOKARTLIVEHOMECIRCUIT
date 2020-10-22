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

#ifndef PSD_GPIO_H_
#define PSD_GPIO_H_

#define PSD_GPIO_MD_SR      32
#define PSD_GPIO_MD_PH_A    28
#define PSD_GPIO_MD_PH_A_DP 50
#define PSD_GPIO_MD_EN_A    65
#define PSD_GPIO_MD_EN_IN_A 66
#define PSD_GPIO_MD_PH_B    29
#define PSD_GPIO_MD_EN_B    62
#define PSD_GPIO_MD_EN_IN_B 64

int psd_gpio_init(void);
void psd_gpio_exit(void);
void psd_gpio_set_pin(int pin, int value);

#endif