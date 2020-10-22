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
#include <asm-generic/div64.h>
#include "psd_util.h"

#define PSD_UTIL_RATIO_FACTOR 1000
#define PSD_UTIL_TABLE_SIZE 256
u8 g_table[PSD_UTIL_TABLE_SIZE];

struct psd_util_ratio psd_util_ratio(u16 input, u16 divider)
{
    struct psd_util_ratio ratio = {0};
    if(divider == 0)
    {
        PSD_LOG("invalid param in %s\n" ,__FUNCTION__);
        return ratio;
    }

    ratio.raw = input * PSD_UTIL_RATIO_FACTOR;
    ratio.raw /= divider;
    return ratio;
}

struct psd_util_ratio psd_util_range_ratio(s16 input, s16 min, s16 max)
{
    struct psd_util_ratio ratio = {0};
    if(max <= min)
    {
        //同じ値か、逆転している場合は異常系
        PSD_LOG("invalid param in %s\n" ,__FUNCTION__);
        return ratio;
    }
    if(max < input || input < min)
    {
        //範囲外は異常系
        PSD_LOG("invalid param in %s. input = %d, min = %d, max = %d\n" ,__FUNCTION__, input, min, max);
        return ratio;
    }

    u16 target;
    u16 range;
    target = input - min;
    range = max - min;
    return psd_util_ratio(target, range);
}

u32 psd_util_apply_ratio(u16 input, struct psd_util_ratio ratio)
{
    u64 result;
    result = input * ratio.raw;

    u16 round_adjuster = (u16)(5 * PSD_UTIL_RATIO_FACTOR / 10);// = 0.5 * PSD_UTIL_RATIO_FACTOR
    result += round_adjuster;

    //result /= PSD_UTIL_RATIO_FACTOR;
    do_div(result, PSD_UTIL_RATIO_FACTOR);
    return result;
}

s16 psd_util_apply_ratio_to_range(s16 min, s16 max, struct psd_util_ratio ratio)
{
    if(min > max)
    {
        //逆転している場合は異常系
        PSD_LOG("invalid param in %s\n" ,__FUNCTION__);
        return 0;
    }

    u16 range;
    range = max - min;
    s64 result;
    result = (s64)psd_util_apply_ratio(range, ratio);//[todo]
    result += min;

    //max値で丸める
    if(result > max)
    {
        //range_ratioで生成したratioでは、本来ここには来ない
        return max;
    }
    else
    {
        return result;
    }
}

u8 psd_util_percent(u16 input, u16 divider)
{
    struct psd_util_ratio ratio;
    ratio = psd_util_ratio(input, divider);
    return psd_util_apply_ratio(100, ratio);
}

u16 psd_exponential_round(u16 input, u8 exponent_ten)
{
    if(exponent_ten > 4)
    {
        //桁あふれし得るので、return
        PSD_LOG("invalid param in %s\n" ,__FUNCTION__);
        return 0;
    }

    if(exponent_ten == 0)
    {
        //小数演算が発生するので、return
        return input;
    }

    u16 factor = 1;
    for(int i=0;i<exponent_ten;i++)
    {
        factor *= 10;
    }

    u16 round_adjuster = (u16)(5 * factor / 10);// = 0.5 * factor

    int result;
    result = input + round_adjuster;
    result /= factor;
    return (u16)result;
}

void initialize_table(void)
{
    u32 r;
    for (u32 i = 0; i < PSD_UTIL_TABLE_SIZE; i++)
    {
        r = i;
        for (u32 j = 0; j < 8; j++)
        {
            if (r & 0x80)
            {
                r = (r << 1) ^ 0x07;
            }
            else
            {
                r <<= 1;
            }
        }
        g_table[i] = (u8)(r);
    }
}

u8 get_hash(const void* p_input, size_t size)
{
    u32 r = 0;
    const u8* p_input_8 = (const u8*)p_input;
    for (u32 i = 0; i < size; i++)
    {
        r = g_table[(r ^ p_input_8[i]) & 0xff];
    }
    return (u8)(r);
}

u8 psd_util_get_crc8(const void* p_input, size_t size)
{
    initialize_table();
    return get_hash(p_input, size);
}

int psd_util_min(int a, int b)
{
    if(a < b)
    {
        return a;
    }
    else
    {
        return b;
    }
}

int psd_util_max(int a, int b)
{
    int min;
    min = psd_util_min(a, b);

    if(min == a)
    {
        return b;
    }
    else
    {
        return a;
    }
}

int psd_util_minmax(int min, int input, int max)
{
    return psd_util_max(min, psd_util_min(input, max));
}

int psd_util_sign(int input)
{
    if(input == 0)
    {
        return 0;
    }
    else
    {
        return input / abs(input);
    }
}

u16 psd_util_convert_multi_byte(u8 ms_byte, u8 ls_byte)
{
    u8 raw[2];
    raw[0] = ls_byte;
    raw[1] = ms_byte;
    return *(u16*)&raw[0];
}

u16 psd_util_convert_multi_byte_with_cap(u8 ms_byte_cap, u8 ms_byte, u8 ls_byte)
{
    ms_byte &= ms_byte_cap;
    return psd_util_convert_multi_byte(ms_byte, ls_byte);
}