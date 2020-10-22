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

#ifndef PSD_UTIL_H_
#define PSD_UTIL_H_

#include <linux/types.h>

#ifdef FUJI_CONFIG_TYPE_SYSDEV
#define PSD_LOG(format, ...) printk(KERN_DEBUG "[psd] " format, ##__VA_ARGS__)
#else
#define PSD_LOG(...) (void)0
#endif

struct psd_util_ratio
{
    u32 raw;
};

//比率の精度は0.1% (0.001)
//比率適用時は、小数点以下を四捨五入
struct psd_util_ratio psd_util_ratio(u16 input, u16 divider);
struct psd_util_ratio psd_util_range_ratio(s16 input, s16 min, s16 max);
u32 psd_util_apply_ratio(u16 input, struct psd_util_ratio ratio);
s16 psd_util_apply_ratio_to_range(s16 min, s16 max, struct psd_util_ratio ratio);
u8 psd_util_percent(u16 input, u16 divider);

//10^n 倍されているinputを、四捨五入しつつ1倍に戻す
u16 psd_exponential_round(u16 input, u8 exponent_ten);

int psd_util_min(int a, int b);
int psd_util_max(int a, int b);
int psd_util_minmax(int min, int input, int max);
int psd_util_sign(int input);
u8 psd_util_get_crc8(const void* p_input, size_t size);
void psd_util_init_crc8(void);

u16 psd_util_convert_multi_byte(u8 ms_byte, u8 ls_byte);
u16 psd_util_convert_multi_byte_with_cap(u8 ms_byte_cap, u8 ms_byte, u8 ls_byte);

#endif