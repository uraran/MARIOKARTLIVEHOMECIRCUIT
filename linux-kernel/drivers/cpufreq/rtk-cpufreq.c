

#define pr_fmt(fmt) "cpufreq: " fmt

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/module.h>
#include <linux/suspend.h>

#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/opp.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include "mach/gpio.h"

#include <asm/io.h>
#include <asm/system_misc.h>
#include <rbus/reg.h>

#define RTK_VOLTAGE_STEP_CTL
#define RTK_VIRT_ADDR_MAP(addr) (RBUS_BASE_VIRT + ((unsigned int)addr - RBUS_BASE_PHYS))
#define WRITE_REG_INT32U(addr,val)  writel(val, IOMEM(RTK_VIRT_ADDR_MAP(addr)))
#define READ_REG_INT32U(addr)       readl(IOMEM(RTK_VIRT_ADDR_MAP(addr)))


#define NUM_CPUS	2

static int default_freq = 0;
static int total_freq_num = 0;

static int MaxIndex = -1;
static struct    clk           *armclk = NULL;
static struct    regulator     *vddarm = NULL;
static unsigned long regulator_latency = 1 * 1000 * 1000;
static bool is_suspended = false;
static unsigned long target_cpu_speed[NUM_CPUS];
static unsigned long target_cpu_idx[NUM_CPUS];
static DEFINE_MUTEX(cpufreq_lock);

static bool             virtual_freq_en         = false;
static unsigned long    virtual_freq_threshold  = -1UL;
static unsigned long    virtual_freq_stor       = 0;

struct realtek_1195_dvfs {
    unsigned int vddarm_min;
    unsigned int vddarm_max;
};

#if 1
static unsigned int cpu_transition_latency = 0;
static struct realtek_1195_dvfs *realtek_1195_dvfs_table = NULL;
static struct realtek_1195_dvfs *realtek_1195_vddarm_state = NULL;
static struct cpufreq_frequency_table *realtek_1195_freq_table = NULL;
static bool freq_table_ready = false;
#else
static struct realtek_1195_dvfs realtek_1195_dvfs_table[] = {
    [0] = { 1000000, 1150000 },
    [1] = { 1050000, 1150000 },
    [2] = { 1100000, 1150000 },
    [3] = { 1200000, 1350000 },
    [4] = { 1300000, 1350000 },
};

static struct cpufreq_frequency_table realtek_1195_freq_table[] = {
    { 0, 600000 }, /* { index, frequency(kHz) } */
    { 1, 700000 },
    { 2, 800000 },
    { 3, 900000 },
    { 4,1000000 },
    { 5,1100000 },
    { 0, CPUFREQ_TABLE_END },
};
#endif

static unsigned int rm_threshold_uV = 950000;
void RM_INIT(void)
{
    pr_info("[%s] \n",__func__);
    WRITE_REG_INT32U(0x1801d484, 0x1111111);
    WRITE_REG_INT32U(0x1801d490, 0x11);
}

void RM_ENABLE(void)
{
    pr_info("[%s] \n",__func__);
    WRITE_REG_INT32U(0x1801d480, 0x7f);
    WRITE_REG_INT32U(0x1801d48c, 0x3);
}

void RM_DISABLE(void)
{
    pr_info("[%s] \n",__func__);
    WRITE_REG_INT32U(0x1801d480, 0x0);
    WRITE_REG_INT32U(0x1801d48c, 0x0);
}

#ifdef RTK_VOLTAGE_STEP_CTL
#define MAX_GPIOS 2
struct VCtlData{
    struct realtek_1195_dvfs *rtk_vdd_state;
    int gpio[MAX_GPIOS]; // [0] gpio:15 [1] gpio:16
};

static int  voltage_step_ctl = 0;
static struct VCtlData gVCtl;

static int rtkSetVoltage(struct realtek_1195_dvfs * dvfs, unsigned int freqsnew)
{
    int i;
    int newVal[MAX_GPIOS];
    if (!voltage_step_ctl || dvfs == gVCtl.rtk_vdd_state)
        goto done;

    if (gVCtl.gpio[0] < 0 || gVCtl.gpio[1] < 0 || dvfs == NULL)
        goto error;

    if (dvfs->vddarm_min ==  1000000) {
        newVal[0] = 0;
        newVal[1] = 0;
    } else if (dvfs->vddarm_min == 1100000) {
        newVal[0] = 0;
        newVal[1] = 1;
    } else if (dvfs->vddarm_min == 1200000) {
        newVal[0] = 1;
        newVal[1] = 1;
    } else
        goto error;

    if ((dvfs->vddarm_min == 900000) && (freqsnew < 450000))
        RM_ENABLE();

#if 0
    for (i=0;i<MAX_GPIOS;i++)
        gpio_set_value(gVCtl.gpio[i],newVal[i]);
#else
    if (gVCtl.rtk_vdd_state == NULL
            || dvfs->vddarm_min < gVCtl.rtk_vdd_state->vddarm_min) {
        gpio_set_value(gVCtl.gpio[0],newVal[0]);

        // Fix Voltage Overshoot
        if (gVCtl.rtk_vdd_state != NULL
                && gVCtl.rtk_vdd_state->vddarm_min == 1200000
                && dvfs->vddarm_min == 900000)
            udelay(20);

        gpio_set_value(gVCtl.gpio[1],newVal[1]);
    } else {
        gpio_set_value(gVCtl.gpio[1],newVal[1]);

        // Fix Voltage Overshoot
        if (gVCtl.rtk_vdd_state != NULL
                && gVCtl.rtk_vdd_state->vddarm_min == 900000
                && dvfs->vddarm_min == 1200000)
            udelay(20);

        gpio_set_value(gVCtl.gpio[0],newVal[0]);
    }
#endif

    if (gVCtl.rtk_vdd_state != NULL && gVCtl.rtk_vdd_state->vddarm_min == 900000 && freqsnew >= 450000)
        RM_DISABLE();

    pr_debug("[%s] Set Vddr:%d gpio[0]:%d gpio[1]:%d\n",__func__,dvfs->vddarm_min,newVal[0],newVal[1]);

    gVCtl.rtk_vdd_state = dvfs;
done:
    return 0;
error:
    pr_err("[%s %d] dvfs=0x%08x vddarm_min %d \n",__func__,__LINE__,(unsigned int)dvfs,dvfs->vddarm_min);
    return -1;
}
#endif

static int realtek_1195_cpufreq_verify_speed(struct cpufreq_policy *policy)
{
    return cpufreq_frequency_table_verify(policy, realtek_1195_freq_table);
}

static unsigned int realtek_1195_cpufreq_get_speed(unsigned int cpu)
{
    if (cpu >= NUM_CPUS)
        return 0;

    if (virtual_freq_en && virtual_freq_stor > virtual_freq_threshold)
        return virtual_freq_stor;

    return clk_get_rate(armclk) / 1000;
}

static unsigned long realtek_1195_cpufreq_get_max_freq_by_threshold(unsigned long threshold)
{
    struct cpufreq_frequency_table *freq = realtek_1195_freq_table;
    unsigned long target_freq = 0;
    unsigned long min_freq = -1UL;
    while (freq->frequency != CPUFREQ_TABLE_END) {
        if (freq->frequency == CPUFREQ_ENTRY_INVALID) {
            freq++;
            continue;
        }

        if (freq->frequency < min_freq)
            min_freq = freq->frequency;

        if (freq->frequency <= threshold && freq->frequency > target_freq)
            target_freq = freq->frequency;
        freq++;
    }

    if (target_freq < min_freq)
        target_freq = min_freq;

    return target_freq;
}

static int realtek_1195_update_cpu_speed(struct cpufreq_policy *policy, int idx)
{
    int ret = 0;
    struct cpufreq_freqs freqs;
    struct realtek_1195_dvfs *dvfs;

    mutex_lock(&cpufreq_lock);

    freqs.old = realtek_1195_cpufreq_get_speed(0);
    freqs.new = realtek_1195_freq_table[idx].frequency;
    freqs.flags = 0;

    if (freqs.old == freqs.new) {
        mutex_unlock(&cpufreq_lock);
        return ret;
    }

    cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);

#if 1
    freqs.old = realtek_1195_cpufreq_get_speed(0);
#endif

    //local_irq_disable();

#ifdef RTK_VOLTAGE_STEP_CTL
    if (voltage_step_ctl && vddarm) {
        pr_err("[%s] ERROR! voltage_step_ctl and regulator(vddarm) at the same time enable! please check the devices tree.\n",__func__);
        regulator_put(vddarm);
        vddarm = NULL;
    }
#endif

    dvfs = &realtek_1195_dvfs_table[realtek_1195_freq_table[idx].index];
    if (freqs.new > freqs.old) {
        if (vddarm) {
#ifdef CONFIG_REGULATOR

            ret = regulator_set_voltage(vddarm,
                    dvfs->vddarm_min,
                    dvfs->vddarm_max);

            //udelay((regulator_latency/1000)/2);
            udelay(50);

            if ((dvfs->vddarm_min > rm_threshold_uV) && (freqs.new >= 450000) )
                if (realtek_1195_vddarm_state != NULL && realtek_1195_vddarm_state->vddarm_min <= rm_threshold_uV)
                    RM_DISABLE();

            if (ret != 0)
                pr_err("Failed to set VDDARM for %dkHz: %d\n",
                        freqs.new, ret);
            else
                realtek_1195_vddarm_state = dvfs;
#endif
        } else {
#ifdef RTK_VOLTAGE_STEP_CTL
            rtkSetVoltage(dvfs, freqs.new);
            udelay(50);
#endif
        }
    }

    if (virtual_freq_en && freqs.new >= virtual_freq_threshold) {
        ret = clk_set_rate(armclk, realtek_1195_cpufreq_get_max_freq_by_threshold(virtual_freq_threshold) * 1000);
    } else
        ret = clk_set_rate(armclk, freqs.new * 1000);

    virtual_freq_stor = freqs.new;

    if (ret < 0) {
        pr_err("Failed to set rate %dkHz: %d\n",
                freqs.new, ret);
        goto err;
    }

    if (freqs.new < freqs.old) {
        if (vddarm) {
#ifdef CONFIG_REGULATOR

            if ((dvfs->vddarm_min <= rm_threshold_uV) && (freqs.new < 450000) )
                if (realtek_1195_vddarm_state == NULL || realtek_1195_vddarm_state->vddarm_min > rm_threshold_uV)
                    RM_ENABLE();

            //udelay((regulator_latency/1000)/2);
            udelay(50);

            ret = regulator_set_voltage(vddarm,
                    dvfs->vddarm_min,
                    dvfs->vddarm_max);
            if (ret != 0)
                pr_err("Failed to set VDDARM for %dkHz: %d\n",
                        freqs.new, ret);
            else
                realtek_1195_vddarm_state = dvfs;
#endif
        } else {
#ifdef RTK_VOLTAGE_STEP_CTL
            udelay(50);
            rtkSetVoltage(dvfs, freqs.new);
#endif
        }
    }

err:

    mutex_unlock(&cpufreq_lock);

    //local_irq_enable();

    cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
    return ret;
}

static unsigned long realtek_1195_cpu_highest_speed(void)
{
    unsigned long rate = 0;
    int i;

    for_each_online_cpu(i)
        rate = max(rate, target_cpu_speed[i]);
    return rate;
}

static unsigned long realtek_1195_cpu_highest_idx(void)
{
    unsigned long idx = 0;
    int i;

    for_each_online_cpu(i)
        idx = max(idx, target_cpu_idx[i]);
    return idx;
}

static int realtek_1195_cpufreq_set_target(struct cpufreq_policy *policy,
        unsigned int target_freq,
        unsigned int relation)
{
    int ret;
    unsigned int idx;

    if (is_suspended) {
        ret = -EBUSY;
        pr_err("W/[%s] Suspend!!! return %d\n",__func__,ret);
        return ret;
    }

    ret = cpufreq_frequency_table_target(policy, realtek_1195_freq_table,
            target_freq, relation, &idx);

    if (ret != 0)
        return ret;

    target_cpu_speed[policy->cpu] = realtek_1195_freq_table[idx].frequency;
    target_cpu_idx[policy->cpu] = idx;

    //ret = realtek_1195_update_cpu_speed(policy,idx);
    ret = realtek_1195_update_cpu_speed(policy,realtek_1195_cpu_highest_idx());

    return ret;
}

#ifdef CONFIG_REGULATOR
static void __init realtek_1195_cpufreq_config_regulator(void)
{
    int count, v, i, found;
    struct cpufreq_frequency_table *freq;
    struct realtek_1195_dvfs *dvfs;

#ifdef CONFIG_REGULATOR_RTP5901
    return;
#endif

    count = regulator_count_voltages(vddarm);
    if (count < 0) {
        pr_err("Unable to check supported voltages\n");
    }

    freq = realtek_1195_freq_table;
    while (count > 0 && freq->frequency != CPUFREQ_TABLE_END) {
        if (freq->frequency == CPUFREQ_ENTRY_INVALID) {
            freq++;
            continue;
        }

        dvfs = &realtek_1195_dvfs_table[freq->index];
        found = 0;

        for (i = 0; i < count; i++) {
            v = regulator_list_voltage(vddarm, i);
            if (v >= dvfs->vddarm_min && v <= dvfs->vddarm_max)
                found = 1;
        }

        if (!found) {
            pr_debug("%dkHz unsupported by regulator\n",
                    freq->frequency);
            freq->frequency = CPUFREQ_ENTRY_INVALID;
        }

        freq++;
    }

    /* Guess based on having to do an I2C/SPI write; in future we
     * will be able to query the regulator performance here. */
    // regulator_latency = 1 * 1000 * 1000;
}
#endif

static int realtek_1195_cpufreq_driver_init(struct cpufreq_policy *policy)
{
    int ret;
    struct cpufreq_frequency_table *freq;

    RM_INIT();

    if (policy->cpu >= NUM_CPUS)
        return -EINVAL;

    if (realtek_1195_freq_table == NULL) {
        pr_err("No frequency information for this CPU\n");
        return -ENODEV;
    }

    if (armclk == NULL)
        armclk = clk_get(NULL, "spll");

    if (IS_ERR(armclk)) {
        pr_err("Unable to obtain ARMCLK: %ld\n",
                PTR_ERR(armclk));
        return PTR_ERR(armclk);
    }

#ifdef CONFIG_REGULATOR
    vddarm = regulator_get(NULL, "vddarm");
    if (IS_ERR(vddarm)) {
        ret = PTR_ERR(vddarm);
        pr_err("Failed to obtain VDDARM: %d\n", ret);
        pr_err("Only frequency scaling available\n");
        vddarm = NULL;
    } else {
        realtek_1195_cpufreq_config_regulator();
    }
#endif

    mutex_lock(&cpufreq_lock);
    if (!freq_table_ready) {
        freq = realtek_1195_freq_table;
        while (freq->frequency != CPUFREQ_TABLE_END) {
            unsigned long r;

            /* Check for frequencies we can generate */
            r = clk_round_rate(armclk, freq->frequency * 1000);
            r /= 1000;
#define TARGET_RANGE 200 //200kHz
            if (((freq->frequency + TARGET_RANGE/2) -r) < TARGET_RANGE) {
                pr_debug("[%s] SET freq->frequency %d => %d \n",__func__,freq->frequency,r);
                freq->frequency = r;
            } else {
                pr_debug("%dkHz unsupported by clock (r = %d)\n",
                        freq->frequency,r);
                freq->frequency = CPUFREQ_ENTRY_INVALID;
            }

#if 0
            /* If we have no regulator then assume startup
             * frequency is the maximum we can support. */
            if (!vddarm && freq->frequency > realtek_1195_cpufreq_get_speed(0))
                freq->frequency = CPUFREQ_ENTRY_INVALID;
#endif

            freq++;
        }
        freq_table_ready = true;
    }
    mutex_unlock(&cpufreq_lock);

    policy->cur = clk_get_rate(armclk) / 1000;
	target_cpu_speed[policy->cpu] = policy->cur;

    pr_debug("[%s] policy->cur = %d \n",__func__,policy->cur);

    /* Datasheet says PLL stabalisation time (if we were to use
     * the PLLs, which we don't currently) is ~300us worst case,
     * but add some fudge.
     */
    //policy->cpuinfo.transition_latency = (500 * 1000) + regulator_latency;
    policy->cpuinfo.transition_latency = cpu_transition_latency + regulator_latency;

    ret = cpufreq_frequency_table_cpuinfo(policy, realtek_1195_freq_table);
    if (ret != 0) {
        pr_err("Failed to configure frequency table: %d\n",
                ret);
#ifdef CONFIG_REGULATOR
        if (vddarm)
            regulator_put(vddarm);
#endif
        clk_put(armclk);
    }

    if (armclk && total_freq_num == 0) {
        clk_set_rate(armclk, default_freq * 1000);
    }

    cpumask_setall(policy->cpus);
    cpufreq_frequency_table_get_attr(realtek_1195_freq_table, policy->cpu);

    return ret;
}

void realtek_1195_cpufreq_voltage_reset(void)
{
#ifdef CONFIG_REGULATOR
    if (vddarm) {
        if (regulator_set_voltage(vddarm,1200000, 1200000) !=0)
            pr_err("[%s] Failed to set VDDARM!",__func__);
        RM_DISABLE();
    }
#endif
}
EXPORT_SYMBOL(realtek_1195_cpufreq_voltage_reset);

static int realtek_1195_cpufreq_notifier_event(struct notifier_block *this,
        unsigned long event, void *ptr)
{
    if (MaxIndex < 0)
        return NOTIFY_OK;

    switch (event) {
        case PM_SUSPEND_PREPARE:
            {
                struct cpufreq_policy *policy = cpufreq_cpu_get(0);
                realtek_1195_update_cpu_speed(policy, MaxIndex);
                cpufreq_cpu_put(policy);
                is_suspended = true;
                realtek_1195_cpufreq_voltage_reset();
                break;
            }
        case PM_POST_RESTORE:
        case PM_POST_SUSPEND:
            {
                struct cpufreq_policy *policy = cpufreq_cpu_get(0);
                is_suspended = false;
                realtek_1195_update_cpu_speed(policy,realtek_1195_cpu_highest_idx());
                cpufreq_cpu_put(policy);
                break;
            }
        default:
            break;
    }

    return NOTIFY_OK;
}

#if 0 //#ifdef CONFIG_PM
static int realtek_1195_cpufreq_suspend(struct cpufreq_policy *policy)
{
    pr_debug("[%s] policy->cur = %d \n",__func__,policy->cur);
    //realtek_1195_update_cpu_speed(policy, 0);
    //is_suspended = true;
    return 0;
}

static int realtek_1195_cpufreq_resume(struct cpufreq_policy *policy)
{
    pr_debug("[%s] policy->cur = %d \n",__func__,policy->cur);
    //is_suspended = false;
    return 0;
}
#endif

static struct freq_attr *realtek_1195_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};


static struct cpufreq_driver realtek_1195_cpufreq_driver = {
    .owner  = THIS_MODULE,
    .flags  = 0/*CPUFREQ_CONST_LOOPS*/,
    .verify = realtek_1195_cpufreq_verify_speed,
    .target = realtek_1195_cpufreq_set_target,
    .get    = realtek_1195_cpufreq_get_speed,
    .init   = realtek_1195_cpufreq_driver_init,
    .name   = "realtek",
#if 0 //#ifdef CONFIG_PM
    .suspend = realtek_1195_cpufreq_suspend,
    .resume	 = realtek_1195_cpufreq_resume,
#endif
    .attr   = realtek_1195_cpufreq_attr,
};

static struct notifier_block realtek_1195_cpufreq_notifier = {
    .notifier_call = realtek_1195_cpufreq_notifier_event,
};

static int __init realtek_1195_cpufreq_init(void)
{
    register_pm_notifier(&realtek_1195_cpufreq_notifier);
    return cpufreq_register_driver(&realtek_1195_cpufreq_driver);
}

#if 0
late_initcall(realtek_1195_cpufreq_init);
#else
device_initcall(realtek_1195_cpufreq_init);
#endif

static struct of_device_id rtk_dvfs_ids[] = {
	{ .compatible = "Realtek,rtk119x-dvfs" },
	{ /* Sentinel */ },
};

MODULE_DEVICE_TABLE(of, rtk_dvfs_ids);

static void rtk_dvfs_shutdown(struct platform_device *pdev)
{
    realtek_1195_cpufreq_voltage_reset();
}

static int rtk_dvfs_frequency_filter(unsigned int frequency)
{
    if (is_revinfo_0 || is_revinfo_1) {
        if (frequency < 350000)
            return -EINVAL;
        if ((frequency > 400000) && (frequency < 720000))
            return -EINVAL;
    }

    return 0;
}

static int rtk_dvfs_probe(struct platform_device *pdev)
{
    const u32 *prop;
    int size;
    int err;
    int i;
    struct cpufreq_frequency_table *freq;

    prop = of_get_property(pdev->dev.of_node, "transition_latency", &size);
    err = size % (sizeof(u32));
	if ((prop) && (!err)) {
        cpu_transition_latency = of_read_number(prop, 1);
	} else {
        pr_err("[%s] transition_latency ERROR! err = %d \n",__func__,err);
        goto error;
    }

    prop = of_get_property(pdev->dev.of_node, "virtual-freq-threshold", &size);
    err = size % (sizeof(u32));
	if ((prop) && (!err)) {
        virtual_freq_en = true;
        virtual_freq_threshold = of_read_number(prop, 1);
        pr_info("[%s] virtual-freq-threshold = %d\n", __func__, virtual_freq_threshold);
	} else {
        virtual_freq_en = false;
    }

#define FREQUENCY_COLUMN 2
	prop = of_get_property(pdev->dev.of_node, "frequency-table", &size);
    err = size % (sizeof(u32)*FREQUENCY_COLUMN);
	if ((prop) && (!err)) {
        int counter = size / (sizeof(u32)*FREQUENCY_COLUMN);
	total_freq_num = counter;
        if (realtek_1195_freq_table == NULL) {
            realtek_1195_freq_table = (struct cpufreq_frequency_table*)
                kzalloc(sizeof(struct cpufreq_frequency_table) * (counter+1),GFP_KERNEL);

            MaxIndex = -1;
            freq = realtek_1195_freq_table;
            for (i=0;i<counter;i+=1) {
                freq->index = of_read_number(prop, 1 + (i*FREQUENCY_COLUMN));
                freq->frequency = of_read_number(prop, 2 + (i*FREQUENCY_COLUMN));
                if (rtk_dvfs_frequency_filter(freq->frequency) != 0)
                    continue;
                MaxIndex++;
                freq++;
		default_freq = freq->frequency;
            }
            freq->index = 0;
            freq->frequency = CPUFREQ_TABLE_END;
        }
	} else {
        pr_err("[%s] frequency-table ERROR! err = %d \n",__func__,err);
        goto error;
    }

    prop = of_get_property(pdev->dev.of_node, "rm-threshold-uV", &size);
    if (prop) {
        int temp_rm_threshold_uV = of_read_number(prop,1);
        if (temp_rm_threshold_uV > 0 && temp_rm_threshold_uV < 2000000) {
            rm_threshold_uV = temp_rm_threshold_uV;
        } else
            pr_err("[%s] rm-threshold-uV ERROR! %d \n",__func__,temp_rm_threshold_uV);
    }

#define VOLTAGE_COLUMN 3
	prop = of_get_property(pdev->dev.of_node, "voltage-table", &size);
    err = size % (sizeof(u32)*VOLTAGE_COLUMN);
	if ((prop) && (!err)) {
        int counter = size / (sizeof(u32)*VOLTAGE_COLUMN);

        if (realtek_1195_dvfs_table == NULL) {
            realtek_1195_dvfs_table = (struct realtek_1195_dvfs *)
                kzalloc(sizeof(struct realtek_1195_dvfs) * counter,GFP_KERNEL);

            for (i=0;i<counter;i+=1) {
                int index = of_read_number(prop, 1 + (i*VOLTAGE_COLUMN));
                realtek_1195_dvfs_table[index].vddarm_min = of_read_number(prop, 2 + (i*VOLTAGE_COLUMN));
                realtek_1195_dvfs_table[index].vddarm_max = of_read_number(prop, 3 + (i*VOLTAGE_COLUMN));
            }
        }
	} else {
        pr_err("[%s] voltage-table ERROR! err = %d \n",__func__,err);
        goto error;
    }

#ifdef RTK_VOLTAGE_STEP_CTL
    prop = of_get_property(pdev->dev.of_node, "voltage-step-ctl", &size);
    if (prop) {
        int enable = of_read_number(prop,1);
        if (enable == 1) {
            voltage_step_ctl = 1;
            gVCtl.rtk_vdd_state = NULL;
        }
    }

    if (voltage_step_ctl) {
        int num_gpios = of_gpio_count(pdev->dev.of_node);
        if (num_gpios == MAX_GPIOS) {
            for (i=0;i<num_gpios;i++) {
                int gpio = of_get_gpio_flags(pdev->dev.of_node, i, NULL);
                if (gpio < 0) {
                    printk("[%s ] could not get gpio from of \n",__FUNCTION__);
                    voltage_step_ctl = 0;
                    break;
                }
                if (!gpio_is_valid(gpio)) {
                    printk("[%s ] gpio %d is not valid\n",__FUNCTION__, gpio);
                    voltage_step_ctl = 0;
                    break;
                }
                if(gpio_request(gpio, pdev->dev.of_node->name)) {
                    printk("[%s ] could not request gpio, %d\n",__FUNCTION__, gpio);
                    voltage_step_ctl = 0;
                    break;
                }
                gVCtl.gpio[i] = gpio;
            }
            if (voltage_step_ctl == 0) {
                pr_err("[%s] voltage step ctl ERROR! err %d tag %d\n",__func__,err,1);
            }
        } else {
            voltage_step_ctl = 0;
            pr_err("[%s] voltage step ctl ERROR! err %d tag %d\n",__func__,err,2);
        }

    }
#endif

error:
    return err;
}

static struct platform_driver realtek_cpufreq_platdrv = {
    .driver = {
        .name   = "rtk119x-cpufreq",
        .owner  = THIS_MODULE,
        .of_match_table = rtk_dvfs_ids,
    },
    .probe      = rtk_dvfs_probe,
    .shutdown   = rtk_dvfs_shutdown,
};

module_platform_driver_probe(realtek_cpufreq_platdrv, rtk_dvfs_probe);

