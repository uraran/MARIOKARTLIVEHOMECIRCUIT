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

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include "psd_param.h"
#include "psd_util.h"

struct check_data_mosi
{
    u8 command_type;
    u8 led_brightness;
    u8 fail_safe;
    u8 reserve[4];
    u8 crc;
};

void psd_spi_param_generate_cd(void* p_buffer, size_t size)
{
    if(size != sizeof(struct check_data_mosi))
    {
        PSD_LOG("size error in %s\n", __FUNCTION__);
        return;
    }

    struct check_data_mosi* p_data;
    p_data = (struct check_data_mosi*)p_buffer;

    struct psd_charge_brake_led charge_brake_led;
    charge_brake_led = psd_param_get_charge_brake_led();

    memset(p_data, 0, size);
    p_data->command_type = 0x01;
    p_data->led_brightness = charge_brake_led.rate;
    if(psd_param_is_heart_beat_fail_safe_setting_enable())
    {
        p_data->fail_safe = 1;
    }
    else
    {
        p_data->fail_safe = 0;
    }
    p_data->crc = psd_util_get_crc8(p_data, size - 1);
}

struct check_data_six_axis
{
    u8 time_stamp[2];
    u8 gyro_x[2];
    u8 gyro_y[2];
    u8 gyro_z[2];
    u8 acc_x[2];
    u8 acc_y[2];
    u8 acc_z[2];
};

struct check_data_bit_field
{
    u8 enumeration : 1;
    u8 state : 4;
    u8 cable : 1;
    u8 chg   : 1;
    u8 pow   : 1;
};

#define SPI_SIX_AXIS_COUNT_MAX 8

struct check_data_miso
{
    union
    {
        struct
        {
            u8 magic_num[2];
            u8 cycle_count;
            u8 fw_version_major;
            u8 fw_version_minor;
            struct check_data_bit_field bit_field;
            u8 adc_vbat[2];
            u8 adc_vs[2];
            u8 adc_vp[2];
            u8 adc_tmp[2];
            u8 six_axis_count;
            struct check_data_six_axis six_axis[SPI_SIX_AXIS_COUNT_MAX];
            u8 crc;
        };
        struct
        {
            u8 magic_num[2];
            u8 status;
            u8 offset2[124];
            u8 crc;
        } iap;
        u8 raw[128];
    };
};

static void update_button(const struct check_data_miso* p_data)
{
    struct psd_button button = {0};
    button.state = p_data->bit_field.pow;
    psd_param_set_button(&button);
}

static void update_charge_state(const struct check_data_miso* p_data)
{
    struct psd_charge_state charge_state = {0};
    charge_state.charge_state = p_data->bit_field.chg;
    charge_state.cable_state = p_data->bit_field.cable;
    charge_state.enumeration_state = p_data->bit_field.enumeration;
    psd_param_set_charge_state(&charge_state);
}

static void update_mcu_state(const struct check_data_miso* p_data)
{
    struct psd_current_mcu_state current_mcu_state = {0};
    switch(p_data->bit_field.state)
    {
        case 0 :
            current_mcu_state.state = psd_mcu_state_standby;
            break;
        case 1 :
            current_mcu_state.state = psd_mcu_state_wakeup;
            break;
        case 2 :
            current_mcu_state.state = psd_mcu_state_connected;
            break;
        case 3 :
            current_mcu_state.state = psd_mcu_state_pairing;
            break;
        default:
            PSD_LOG("state error in %s. state = %d\n", __FUNCTION__, p_data->bit_field.state);
            current_mcu_state.state = psd_mcu_state_standby;
    }
    psd_param_set_current_mcu_state(&current_mcu_state);    
}

static void update_mcu_fw_version(const struct check_data_miso* p_data)
{
    struct psd_mcu_fw_version version = {0};
    version.major = p_data->fw_version_major;
    version.minor = p_data->fw_version_minor;
    psd_param_set_mcu_fw_version(&version);
}

static u16 concatenate_steering_bit(u8 vbat, u8 vs, u8 vp, u8 temp)
{
    u8 cap = 0b11100000;
    u8 msbyte = 0;
    u8 lsbyte = 0;

    vbat &= cap;
    vs   &= cap;
    vp   &= cap;
    temp &= cap;

    lsbyte |= temp>>5;
    lsbyte |= vp>>2;
    lsbyte |= vs<<1;
    msbyte |= vs>>7;
    msbyte |= vbat>>4;

    return psd_util_convert_multi_byte(msbyte, lsbyte);
}

static void update_voltage(const struct check_data_miso* p_data)
{
    struct psd_voltage voltage = {0};
    u8 cap = 0b00001111;
    voltage.throttle = psd_util_convert_multi_byte_with_cap(cap, p_data->adc_vs[1], p_data->adc_vs[0]);
    voltage.potentio = psd_util_convert_multi_byte_with_cap(cap, p_data->adc_vp[1], p_data->adc_vp[0]);
    voltage.battery  = psd_util_convert_multi_byte_with_cap(cap, p_data->adc_vbat[1], p_data->adc_vbat[0]);
    voltage.temperature = psd_util_convert_multi_byte_with_cap(cap, p_data->adc_tmp[1], p_data->adc_tmp[0]);
    voltage.steering = concatenate_steering_bit(p_data->adc_vbat[1], p_data->adc_vs[1], p_data->adc_vp[1], p_data->adc_tmp[1]);
    psd_param_set_voltage(&voltage);
}

static bool extract_clock_correct_bit(u8 vbat)
{
    u8 cap = 0b00010000;
    vbat &= cap;
    if(vbat>>4 == 1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void update_mcu_clock_correction(const struct check_data_miso* p_data)
{
    struct psd_mcu_clock_correction mcu_clock_correction = {0};
    mcu_clock_correction.is_enable = extract_clock_correct_bit(p_data->adc_vbat[1]);
    psd_param_set_clock_correction(&mcu_clock_correction);
}

static int remove_duplicated_data(struct psd_six_axis six_axis[], int count)
{
    static u64 previous_ts_us = 0;
    struct psd_six_axis result_six_axis[SPI_SIX_AXIS_COUNT_MAX];
    int result_count = 0;

    for(int i=0;i<count;i++)
    {
        s16 delta_ts_us = (u16) six_axis[i].time_stamp_us - (u16) previous_ts_us;

        if(delta_ts_us <= 0)
        {
            // timestamp in the past, skip it
        }
        else
        {
            if(delta_ts_us >= 900)
            {
                PSD_LOG("Sample lost. Delta ts: %d us\n", delta_ts_us);
            }

            result_six_axis[result_count] = six_axis[i];
            result_count++;
            previous_ts_us = six_axis[i].time_stamp_us;
        }
    }

    for(int i=0;i<result_count;i++)
    {
        six_axis[i] = result_six_axis[i];
    }

    return result_count;
}

static bool form_time_stamp(struct psd_six_axis six_axis[], int count)
{
    // Convert 16 bits MCU timestamps to 64 bits SOC timestamps
    // All timestamps are in microseconds
    // As MCU clock may be slightly different from the SCO clock,
    // some time scaling has to by applied.
    // Also to avoid any long term drift, some small corrections
    // can be done to keep the sample timestamps close to the
    // soc clock

    static u16 previous_input_ts = 0;
    static u64 previous_output_ts = 0;
    static u64 previous_target_ts = 0;

    static u64 previous_soc_ts = 0;
    
    static u64 global_mcu_delta_ts_sum = 10000; // Init to avoid division by 0 at firsts calls. Use big value to avoid init instabilities.
    static u64 global_soc_delta_ts_sum = 10000; // Init to avoid division by 0 at firsts calls. Use big value to avoid init instabilities.
    static int average_delta_ts_sample_count = -1;

    u64 current_soc_ts = ktime_to_us(ktime_get());
    u64 local_soc_delta_ts_sum = current_soc_ts - previous_soc_ts;

    u64 local_mcu_delta_ts_sum = 0;

    if (average_delta_ts_sample_count == -1)
    {
        // To initialize, we skip call with 0 sample as the imu stream not started wet.
        // We also skip SPI_SIX_AXIS_COUNT_MAX samples calls as there are probably old sample.
        // This allow to make the MCU and SOC timestamps to be the closest possible.
        if(count > 0 && count < SPI_SIX_AXIS_COUNT_MAX)
        {
            // First valid call, init the timestamps
            // To have correct initialization, some timestamp must be present be not all, meaning the soc is uptodate
            average_delta_ts_sample_count = 0;
            previous_output_ts = current_soc_ts;
            previous_target_ts = current_soc_ts;
            previous_soc_ts = current_soc_ts;
            previous_input_ts = six_axis[count -1].time_stamp_us;
            PSD_LOG("Six axis timestamp initialized\n");
        }
        return false;
    }

    for(int i=0;i<count;i++)
    {
        s16 delta_input_ts = (u16) six_axis[i].time_stamp_us - (u16) previous_input_ts;
        previous_input_ts = six_axis[i].time_stamp_us;

        local_mcu_delta_ts_sum += delta_input_ts;

        // Apply time scale with integer round
        u64 delta_output_ts = div_u64(2 * delta_input_ts * global_soc_delta_ts_sum + global_mcu_delta_ts_sum, 2 * global_mcu_delta_ts_sum);

        u64 output_ts = previous_output_ts + delta_output_ts;
        u64 target_ts = previous_target_ts + delta_output_ts;

        // Find error to soc clock and compute a small correction if needed
        s64 ts_error = target_ts - output_ts;
        s32 correction = div_s64(ts_error, 64);

        u64 corrected_output_ts = output_ts + correction;

        previous_output_ts = corrected_output_ts;
        six_axis[i].time_stamp_us = corrected_output_ts;
        previous_target_ts = target_ts;
    }

    if(count < SPI_SIX_AXIS_COUNT_MAX)
    {
        // Lerp previous_target_ts slowly with current_soc_ts because the soc scheduling can be very impressive
        // The update is done only if the MCU ring buffer is empty to be sure to sync only if the SOC is up to date
        previous_target_ts = div_u64(previous_target_ts * 19 + current_soc_ts, 20);
    }

    if (average_delta_ts_sample_count > 5000)
    {
        // The history is big enough, make some place for the new samples
        global_mcu_delta_ts_sum -= div_u64(global_mcu_delta_ts_sum * count, average_delta_ts_sample_count);
        global_soc_delta_ts_sum -= div_u64(global_soc_delta_ts_sum * count, average_delta_ts_sample_count);
        average_delta_ts_sample_count -= count;
    }

    average_delta_ts_sample_count += count;

    global_mcu_delta_ts_sum += local_mcu_delta_ts_sum;
    global_soc_delta_ts_sum += local_soc_delta_ts_sum;

    previous_soc_ts = current_soc_ts;
    return true;
}

static void update_six_axis_history(const struct check_data_miso* p_data)
{
    struct psd_six_axis six_axis[SPI_SIX_AXIS_COUNT_MAX];
    int count = psd_util_min(SPI_SIX_AXIS_COUNT_MAX, p_data->six_axis_count);
    for(int i=0;i<count;i++)
    {
        const struct check_data_six_axis* p_axis = &p_data->six_axis[i];
        six_axis[i].time_stamp_us = psd_util_convert_multi_byte(p_axis->time_stamp[1], p_axis->time_stamp[0]);
        six_axis[i].acceleration_x = (s16)psd_util_convert_multi_byte(p_axis->acc_x[1], p_axis->acc_x[0]);
        six_axis[i].acceleration_y = (s16)psd_util_convert_multi_byte(p_axis->acc_y[1], p_axis->acc_y[0]);
        six_axis[i].acceleration_z = (s16)psd_util_convert_multi_byte(p_axis->acc_z[1], p_axis->acc_z[0]);
        six_axis[i].angular_velocity_x = (s16)psd_util_convert_multi_byte(p_axis->gyro_x[1], p_axis->gyro_x[0]);
        six_axis[i].angular_velocity_y = (s16)psd_util_convert_multi_byte(p_axis->gyro_y[1], p_axis->gyro_y[0]);
        six_axis[i].angular_velocity_z = (s16)psd_util_convert_multi_byte(p_axis->gyro_z[1], p_axis->gyro_z[0]);
    }
    count = remove_duplicated_data(&six_axis[0], count);

    if(!form_time_stamp(&six_axis[0], count))
    {
        // Timestamp system not initialized yep, drop samples
        count = 0;
    }

    psd_param_add_six_axis(&six_axis[0], count);
}

static void update_init_state(void)
{
    struct psd_init_state init_state = {0};
    init_state.state = PSD_INIT_STATE_INITIALIZED;
    psd_param_set_init_state(&init_state);
}

static bool is_valid(const struct check_data_miso* p_data)
{
    static u8 g_previous_cycle_count = 0xFF;//初回の重複判定に引っかからないように、0以外にしている

    if(p_data->magic_num[0] != 0xA5 || p_data->magic_num[1] != 0x5A)
    {
        PSD_LOG("magic num error in data_check\n");
        return false;
    }

    bool is_valid_cycle_count = ((u8)(g_previous_cycle_count + 1) == p_data->cycle_count);
    g_previous_cycle_count = p_data->cycle_count;
    if(!is_valid_cycle_count)
    {
        PSD_LOG("%s cycle count error. count = %d\n", __FUNCTION__, g_previous_cycle_count);
        return false;
    }

    return true;
}

bool is_iap(const struct check_data_miso* p_data)
{
    if(p_data->iap.magic_num[0] == 0xA5 && p_data->iap.magic_num[1] == 0x5A)
    {
        PSD_LOG("not iap mode(%d)\n", __LINE__);
        return false;
    }

    if(p_data->iap.magic_num[0] != 0xC3 || p_data->iap.magic_num[1] != 0x3C)
    {
        PSD_LOG("not iap mode(%d)\n", __LINE__);
        return false;
    }

    return true;
}

static void check_one_time(const struct check_data_miso* p_data)
{
    static bool g_is_first_time = true;
    if(g_is_first_time)
    {
        struct timespec currentTimeSpec;
        currentTimeSpec = ktime_to_timespec(ktime_get());
        printk(KERN_INFO "[BootTime_PsddInitialized] %ld.%09ld\n", currentTimeSpec.tv_sec, currentTimeSpec.tv_nsec);

        printk(KERN_INFO "MCU FW version %d.%d\n", p_data->fw_version_major, p_data->fw_version_minor);
        g_is_first_time = false;
    }
}

static void update_mcu_state_by_iap(const struct check_data_miso* p_data)
{
    PSD_LOG("IAP Mode in %s\n", __FUNCTION__);
    struct psd_current_mcu_state current_mcu_state = {0};
    current_mcu_state.state = psd_mcu_state_iap;
    psd_param_set_current_mcu_state(&current_mcu_state);

    PSD_LOG("IAP Status (%d)\n", p_data->iap.status);
    struct psd_current_mcu_iap_state current_mcu_iap_state = {0};
    current_mcu_iap_state.state = p_data->iap.status;
    psd_param_set_current_mcu_iap_state(&current_mcu_iap_state);
}

static bool check_crc(const struct check_data_miso* p_data)
{
    u8 crc = p_data->crc;
    u8 culc_crc = psd_util_get_crc8(p_data, sizeof(struct check_data_miso) - 1);
    if(crc == culc_crc)
    {
        return true;
    }
    else
    {
        PSD_LOG("crc error. crc = %d, ccrc= %d\n", crc, culc_crc);
        return false;
    }
}

static bool update_by_cd(const struct check_data_miso* p_data)
{
    if(!check_crc(p_data))
    {
        return false;
    }

    check_one_time(p_data);
    update_button(p_data);
    update_charge_state(p_data);
    update_mcu_state(p_data);
    update_mcu_fw_version(p_data);
    update_voltage(p_data);
    update_mcu_clock_correction(p_data);
    update_six_axis_history(p_data);
    update_init_state();

    //MCU側のバッファに余りがあるか否かを返し、再呼び出しの判定に使う
    return p_data->six_axis_count == SPI_SIX_AXIS_COUNT_MAX;
}

static void update_by_iap(struct check_data_miso* p_data)
{
    update_mcu_state_by_iap(p_data);
    update_init_state();
}

bool psd_spi_param_receive_cd(const void* p_buffer, size_t size)
{
    if(size != sizeof(struct check_data_miso))
    {
        PSD_LOG("size error in %s\n", __FUNCTION__);
        return 0;
    }

    struct check_data_miso* p_data;
    p_data = (struct check_data_miso*)p_buffer;
    if(is_valid(p_data))
    {
        return update_by_cd(p_data);
    }

    if(is_iap(p_data))
    {
        update_by_iap(p_data);
        return 0;
    }

    return 0;
}

struct state_set_mosi
{
    u8 command_type;
    u8 state;
    u8 reboot_flag;
    u8 reserved[4];
    u8 crc;
};

void psd_spi_param_generate_ss(void* p_buffer, size_t size)
{
    if(size != sizeof(struct state_set_mosi))
    {
        PSD_LOG("size error in %s\n", __FUNCTION__);
        return;
    }

    struct state_set_mosi* p_data;
    p_data = (struct state_set_mosi*)p_buffer;

    struct psd_mcu_state_request state_request;
    state_request = psd_param_pop_mcu_state_request();

    memset(p_data, 0, size);
    p_data->command_type = 0x02;
    p_data->state = state_request.state;
    p_data->reboot_flag = state_request.is_via_reboot;
    p_data->crc = psd_util_get_crc8(p_data, size - 1);
}

void psd_spi_param_generate_flash_erase(void* p_buffer, size_t size)
{
    if(size != 8)
    {
        PSD_LOG("size error in %s\n", __FUNCTION__);
        return;
    }

    u8* p_data;
    p_data = (u8*)p_buffer;

    memset(p_buffer, 0, size);
    p_data[0] = 0x03; // FLASH_ERASE Command
    p_data[7] = psd_util_get_crc8(p_buffer, size - 1);
}

void psd_spi_param_generate_flash_write(void* p_buffer, size_t size, u32 address, u8* data, u8 data_size)
{
    if(size != 136)
    {
        PSD_LOG("size error in %s\n", __FUNCTION__);
        return;
    }

    u8* p_data;
    p_data = (u8*)p_buffer;

    memset(p_buffer, 0, size);
    p_data[0] = 0x04; // FLASH_WRITE Command
    p_data[1] = (u8)(address & 0xff);     // data address 
    p_data[2] = (u8)(address >> 8 & 0xff);// data address
    p_data[3] = (u8)(address >> 16 & 0xff);// data address
    p_data[4] = (u8)(address >> 24 & 0xff);// data address
    p_data[5] = data_size;
    p_data[7] = psd_util_get_crc8(&p_data[0], 7);
    for(int i = 0; i < data_size; i++)
    {
        p_data[8 + i] = data[i];
    }
    p_data[135] = psd_util_get_crc8(&p_data[8], size - 8 - 1);

    PSD_LOG("tranfered data = %x %x %x %x %x %x %x %x %x\n", p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5], p_data[6], p_data[7], p_data[8]);
}

void psd_spi_param_generate_flash_verify(void* p_buffer, size_t size, u16 data_size, u32 crc)
{
    if(size != 8)
    {
        PSD_LOG("size error in %s\n", __FUNCTION__);
        return;
    }

    u8* p_data;
    p_data = (u8*)p_buffer;

    memset(p_buffer, 0, size);
    p_data[0] = 0x05; // FLASH_VERIFY Command
    p_data[1] = (u8)(data_size & 0xff);     // data size 
    p_data[2] = (u8)(data_size >> 8 & 0xff);// data size
    p_data[3] = (u8)(crc & 0xff);// data address
    p_data[4] = (u8)(crc >> 8 & 0xff);// data address
    p_data[5] = (u8)(crc >> 16 & 0xff);// data address
    p_data[6] = (u8)(crc >> 24 & 0xff);// data address
    p_data[7] = psd_util_get_crc8(&p_data[0], 7);
}

void psd_spi_param_generate_iap_reboot(void* p_buffer, size_t size)
{
    if(size != 8)
    {
        PSD_LOG("size error in %s\n", __FUNCTION__);
        return;
    }

    u8* p_data;
    p_data = (u8*)p_buffer;

    memset(p_buffer, 0, size);
    p_data[0] = 0x02; // STATE_SET
    p_data[1] = 0x01; // wakeup 
    p_data[7] = psd_util_get_crc8(&p_data[0], 7);
}
