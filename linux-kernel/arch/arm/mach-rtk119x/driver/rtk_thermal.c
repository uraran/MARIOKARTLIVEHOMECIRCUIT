#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/thermal.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/cpu_cooling.h>
#include <linux/slab.h>

#define RTK_THERMAL_LINEAR_TEST 0

#define THERMAL_NUMBER 2
#define SET_THERMAL_PWDB1(en)           ((en &  0x1U) << 15)
#define SET_THERMAL_SBG1(vt)            ((vt &  0x7U) << 12)
#define SET_THERMAL_SDATA1(sw)          ((sw & 0x7fU) <<  5)
#define SET_THERMAL_SINL1(sl)           ((sl &  0x3U) <<  3)
#define SET_THERMAL_SOS1(sl)            ((sl &  0x7U) <<  0)

#define GET_THERMAL_CMP_OUT1(reg)       ((reg & ( 0x1U << 16)) >> 16)
#define GET_THERMAL_PWDB1(reg)          ((reg & ( 0x1U << 15)) >> 15)
#define GET_THERMAL_SBG1(reg)           ((reg & ( 0x7U << 12)) >> 12)
#define GET_THERMAL_SDATA1(reg)         ((reg & (0x7fU <<  5)) >>  5)
#define GET_THERMAL_SINL1(reg)          ((reg & ( 0x3U <<  3)) >>  3)
#define GET_THERMAL_SOS1(reg)           ((reg & ( 0x7U <<  0)) >>  0)

#define GET_THERMAL_CALIBRATION_0(reg)  ((reg & (0x7fU << 25)) >> 25)
#define GET_THERMAL_CALIBRATION_1(reg)  ((reg & (0x7fU << 18)) >> 18)

struct rtk_thermal_ops;

enum FlagMode {
    SET,
    CLEAR,
    ASSIGN
};

enum rtk_thermal_flags_t {
    TREND_URGENT            = 0x1U << 0,
    TRIP_SHUTDOWN           = 0x1U << 1,
    THERMAL_0_EN            = 0x1U << 2,
    THERMAL_1_EN            = 0x1U << 3,
};

#define THERMAL_MASK (THERMAL_0_EN | THERMAL_1_EN)

struct rtk_thermal_priv {
    struct thermal_zone_device      *rtk_thermal;
    struct thermal_cooling_device   *rtk_cool_dev;
    enum thermal_device_mode        mode;
    int                             numTrip;
#if RTK_THERMAL_LINEAR_TEST
    int                             tripStep;
    int                             tripHot;
    int                             tripShutdown;
#else
    unsigned long                   *tripTemp;
    unsigned long                   *tripState;
#endif
    unsigned int                    compareDelayUs;
    int                             monitoringRateMs;
    unsigned long                   flags;

    void __iomem                    *thermal_base[THERMAL_NUMBER];
    void __iomem                    *calibration_base;
    unsigned long                   calibrationValue[THERMAL_NUMBER];
    struct rtk_thermal_ops          *ops;
};

struct rtk_thermal_ops {
    /* Initialize the sensor */
    void (*init_sensor)(struct rtk_thermal_priv *);
    void (*post_sensor)(struct rtk_thermal_priv *);
};

void modifyFlags(struct rtk_thermal_priv *priv, unsigned int value, enum FlagMode mode)
{
    switch (mode) {
        case SET:
            priv->flags |= value;
            break;
        case CLEAR:
            priv->flags &= ~value;
            break;
        case ASSIGN:
            priv->flags = value;
            break;
        default:
            printk("[%s] ERROR! value = 0x%x, mode = %d\n",__FUNCTION__,value,mode);
    }
}

void thermal_get(struct rtk_thermal_priv *priv, unsigned int thermalID, unsigned long *temp)
{
    void __iomem *base;
    unsigned int compareDelayUs = priv->compareDelayUs;
    unsigned long calibration_value;

    if (IS_ERR_OR_NULL(temp) || IS_ERR_OR_NULL(priv))
        return;

    if (thermalID >= THERMAL_NUMBER) {
        *temp = -1UL;
        return;
    } else {
        base    = priv->thermal_base[thermalID];
        calibration_value = priv->calibrationValue[thermalID];
    }

    // THERMAL ENABLE
    {
        writel( SET_THERMAL_PWDB1(0x1)      |
                SET_THERMAL_SBG1(0x4)       |
                SET_THERMAL_SDATA1(0x40)    |
                SET_THERMAL_SINL1(0x1)      |
                SET_THERMAL_SOS1(0x4)       ,
                base);

        usleep_range(compareDelayUs,compareDelayUs+10);
    }

    // THERMAL DETECT
    {
        int i;
        for (i=6; i>=0; i--) {
            u32 reg = readl(base);
            u32 cmp_data = (0x1U << i); // 7'b
            writel(reg | SET_THERMAL_SDATA1(cmp_data), base);
            usleep_range(compareDelayUs,compareDelayUs+10);
            reg = readl(base);
            if (GET_THERMAL_CMP_OUT1(reg) == 0)
                writel(reg & ~SET_THERMAL_SDATA1(cmp_data),base);
        }
    }

    // GET TEMPERATURE
    {
        u32 reg = readl(base);
        unsigned long long temperature = GET_THERMAL_SDATA1(reg);
        temperature *= 1000;
        //temperature = ((temperature*165)/128)-40-7;
        temperature -= calibration_value;
        temperature = (temperature*165)/128;
        if (calibration_value != 0)
            temperature += 25 * 1000;
        else
            temperature -= 47 * 1000;
        *temp = (unsigned long) temperature;
    }
}

static int rtk_thermal_get_temp(struct thermal_zone_device *thermal,
        unsigned long *temp)
{
    struct rtk_thermal_priv *priv = thermal->devdata;

    unsigned long temp_temp = 0;
    int i;

    *temp = 0;

    if (!(priv->flags & THERMAL_MASK))
        return -EINVAL;

    for (i=0;i<THERMAL_NUMBER;i++) {
        if (        ((priv->flags & THERMAL_0_EN) && (i == 0))
                ||  ((priv->flags & THERMAL_1_EN) && (i == 1))) {
            thermal_get(priv, i, &temp_temp);
            if (temp_temp > *temp)
                *temp = temp_temp;
        }
    }

    return 0;
}

static int rtk_thermal_get_trip_type(struct thermal_zone_device *thermal,
        int trip, enum thermal_trip_type *type)
{
    struct rtk_thermal_priv *priv = thermal->devdata;

    if (trip >= priv->numTrip)
        return -EINVAL;

    if ((priv->flags & TRIP_SHUTDOWN) && (trip + 1 == priv->numTrip))
        *type = THERMAL_TRIP_CRITICAL;
    else
        *type = THERMAL_TRIP_PASSIVE;

    return 0;
}

static int rtk_thermal_get_trip_value(struct rtk_thermal_priv *priv, int trip)
{
    if (trip >= priv->numTrip)
        return -EINVAL;

#if RTK_THERMAL_LINEAR_TEST
    return priv->tripHot + (trip * priv->tripStep);
#else
    return priv->tripTemp[trip];
#endif
}

static int rtk_thermal_get_trip_temp(struct thermal_zone_device *thermal,
        int trip, unsigned long *temp)
{
    struct rtk_thermal_priv *priv = thermal->devdata;

    if (trip >= priv->numTrip)
        return -EINVAL;

    *temp = rtk_thermal_get_trip_value(priv, trip);

    return 0;
}

static int rtk_thermal_get_crit_temp(struct thermal_zone_device *thermal,
        unsigned long *temp)
{
    struct rtk_thermal_priv *priv = thermal->devdata;
    /* shutdown zone */
    return rtk_thermal_get_trip_temp(thermal, priv->numTrip - 1, temp);
}

static int rtk_thermal_get_trend(struct thermal_zone_device *thermal,
        int trip, enum thermal_trend *trend)
{
    struct rtk_thermal_priv *priv = thermal->devdata;
    unsigned long trip_temp;
    int ret;

    ret = rtk_thermal_get_trip_temp(thermal, trip, &trip_temp);
    if (ret < 0)
        return ret;

    if (thermal->temperature >= trip_temp)
        if (priv->flags & TREND_URGENT)
            *trend = THERMAL_TREND_RAISE_FULL;
        else
            *trend = THERMAL_TREND_RAISING;
    else
        if (priv->flags & TREND_URGENT)
            *trend = THERMAL_TREND_DROP_FULL;
        else
            *trend = THERMAL_TREND_DROPPING;

    if (thermal->temperature >= trip_temp) {
        dev_info(&thermal->device, "temperature(%d C) tripTemp(%lu C) calibration(%lu %lu)\n",
                thermal->temperature/1000, trip_temp/1000,
                priv->calibrationValue[0]/1000, priv->calibrationValue[1]/1000);
    }

    return 0;
}
static int rtk_thermal_get_mode(struct thermal_zone_device *thermal,
        enum thermal_device_mode *mode)
{
    struct rtk_thermal_priv *priv = thermal->devdata;

    if (priv)
        *mode = priv->mode;

    return 0;
}

static int rtk_thermal_set_mode(struct thermal_zone_device *thermal,
        enum thermal_device_mode mode)
{
    struct rtk_thermal_priv *priv = thermal->devdata;

    if (!priv->rtk_thermal) {
        dev_notice(&thermal->device, "thermal zone not registered\n");
        return 0;
    }

    mutex_lock(&priv->rtk_thermal->lock);

    if (mode == THERMAL_DEVICE_ENABLED)
        priv->rtk_thermal->polling_delay = priv->monitoringRateMs;
    else
        priv->rtk_thermal->polling_delay = 0;

    mutex_unlock(&priv->rtk_thermal->lock);

    priv->mode = mode;

    thermal_zone_device_update(priv->rtk_thermal);
    dev_info(&thermal->device, "thermal polling set for duration=%d msec\n",
            priv->rtk_thermal->polling_delay);

    return 0;
}

static int rtk_thermal_bind(struct thermal_zone_device *thermal,
        struct thermal_cooling_device *cdev)
{
    struct rtk_thermal_priv *priv = thermal->devdata;
    unsigned long max_state, upper;
    int err = 0;
    int ret = 0;
    int i;

    dev_info(&thermal->device, "[%s] (%p %p)\n",__func__,priv->rtk_cool_dev,cdev);

    if (IS_ERR_OR_NULL(priv))
        return -ENODEV;

    /* check if this is the cooling device we registered */
    //if (priv->rtk_cool_dev != cdev)
    //	return 0;

    cdev->ops->get_max_state(cdev, &max_state);

    dev_info(&thermal->device, "[%s] max_state = %lu\n",__func__,max_state);

    for (i=0; i < priv->numTrip; i++) {

#if RTK_THERMAL_LINEAR_TEST
        if (i == (priv->numTrip -1))
            upper   = max_state - 1;
        else
            upper = i * (max_state / priv->numTrip);
#else
        upper = priv->tripState[i];
#endif

        /* Simple thing, two trips, one passive another critical */
        ret = thermal_zone_bind_cooling_device(thermal, i, cdev,
                upper, 0);

        dev_info(&cdev->device, "%s bind to %d: %d-%s upper:%lu\n", cdev->type,
                i, ret, ret ? "fail" : "succeed",upper);

        if(ret)
            err = ret;
    }

    if (err)
        dev_err(&thermal->device, "Failed to bind cooling device\n");

    return ret;
}

static int rtk_thermal_unbind(struct thermal_zone_device *thermal,
        struct thermal_cooling_device *cdev)
{
    struct rtk_thermal_priv *priv = thermal->devdata;

    if (IS_ERR_OR_NULL(priv))
        return -ENODEV;

    /* check if this is the cooling device we registered */
    if (priv->rtk_cool_dev != cdev)
        return 0;

    /* Simple thing, two trips, one passive another critical */
    return thermal_zone_unbind_cooling_device(thermal, 0, cdev);
}

static struct thermal_zone_device_ops ops = {
    .get_temp       = rtk_thermal_get_temp,
    .get_trend      = rtk_thermal_get_trend,
    .bind           = rtk_thermal_bind,
    .unbind         = rtk_thermal_unbind,
    .get_mode       = rtk_thermal_get_mode,
    .set_mode       = rtk_thermal_set_mode,
    .get_trip_type  = rtk_thermal_get_trip_type,
    .get_trip_temp  = rtk_thermal_get_trip_temp,
    .get_crit_temp  = rtk_thermal_get_crit_temp,
};

static void rtk119x_thermal_init_sensor(struct rtk_thermal_priv *priv)
{
    if (IS_ERR_OR_NULL(priv))
        return;

    modifyFlags(priv, THERMAL_0_EN, SET);
    modifyFlags(priv, THERMAL_1_EN, SET);

#if RTK_THERMAL_LINEAR_TEST
    priv->numTrip           = 8;
    priv->tripHot           = 50000;
    priv->tripShutdown      = 80000;
#endif
    priv->monitoringRateMs  = 250 * 2;
    priv->compareDelayUs    = 15;
}

static void rtk119x_thermal_post_sensor(struct rtk_thermal_priv *priv)
{
    if (IS_ERR_OR_NULL(priv))
        return;

    if (!(priv->flags & THERMAL_MASK)) {
        priv->numTrip = 0;
    }

    {
        u32 reg = readl(priv->calibration_base);
        if (priv->flags & THERMAL_0_EN)
            priv->calibrationValue[0] = GET_THERMAL_CALIBRATION_0(reg) * 1000;
        if (priv->flags & THERMAL_1_EN)
            priv->calibrationValue[1] = GET_THERMAL_CALIBRATION_1(reg) * 1000;
    }

#if RTK_THERMAL_LINEAR_TEST
    if (priv->numTrip > 1)
        priv->tripStep          = (priv->tripShutdown - priv->tripHot) / (priv->numTrip - 1);
#endif
}

static const struct rtk_thermal_ops rtk119x_thermal_ops = {
    .init_sensor = rtk119x_thermal_init_sensor,
    .post_sensor = rtk119x_thermal_post_sensor,
};

static const struct of_device_id rtk_thermal_id_table[] = {
    {
        .compatible = "rtk119x-thermal",
        .data       = &rtk119x_thermal_ops,
    },
    {
        /* sentinel */
    },
};

MODULE_DEVICE_TABLE(of, rtk_thermal_id_table);

static int rtk_thermal_get_cooling_state_by_freq(unsigned int input, unsigned long *output)
{
    int i, j;
    unsigned long max_level = 0;
    unsigned int freq = CPUFREQ_ENTRY_INVALID;
    unsigned int freq_temp = 0;
    int descend = 0;
    struct cpufreq_frequency_table *table =
        cpufreq_frequency_get_table(0);

    if (!table || !output)
        return -EINVAL;

    for (i = 0; table[i].frequency != CPUFREQ_TABLE_END; i++) {
        /* ignore invalid entries */
        if (table[i].frequency == CPUFREQ_ENTRY_INVALID)
            continue;

        /* ignore duplicate entry */
        if (freq == table[i].frequency)
            continue;

        /* get the frequency order */
        if (freq != CPUFREQ_ENTRY_INVALID && descend != -1)
            descend = !!(freq > table[i].frequency);

        freq = table[i].frequency;
        max_level++;
    }
    for (i = 0, j = 0; table[i].frequency != CPUFREQ_TABLE_END; i++) {
        /* ignore invalid entry */
        if (table[i].frequency == CPUFREQ_ENTRY_INVALID)
            continue;

        /* ignore duplicate entry */
        if (freq == table[i].frequency)
            continue;

        /* now we have a valid frequency entry */
        freq = table[i].frequency;

        if (freq <= input && freq > freq_temp) {
            freq_temp = freq;
            *output = descend ? j : (max_level - j - 1);
        }
        j++;
    }

    return (freq_temp == 0)? -EINVAL : 0;
}

static int rtk_thermal_probe_of(struct rtk_thermal_priv *priv, struct platform_device *pdev)
{
    struct device_node *node = pdev->dev.of_node;
    const u32 *prop;
    int size;
    int err,ret;
    int i;

#define THERMAL_COLUMN 2
    prop = of_get_property(node, "thermal-table", &size);
    err = size % (sizeof(u32)*THERMAL_COLUMN);
    if ((prop) && (!err)) {
        priv->numTrip   = size / (sizeof(u32)*THERMAL_COLUMN);
        priv->tripTemp  = (unsigned long *) kzalloc(sizeof(unsigned long) * (priv->numTrip),GFP_KERNEL);
        priv->tripState = (unsigned long *) kzalloc(sizeof(unsigned long) * (priv->numTrip),GFP_KERNEL);

        for (i=0; i<priv->numTrip; i++) {
            unsigned int reqFreq = of_read_number(prop, 2 + (i*THERMAL_COLUMN));
            priv->tripTemp[i] = of_read_number(prop, 1 + (i*THERMAL_COLUMN));
            priv->tripTemp[i] *= 1000;
            ret =  rtk_thermal_get_cooling_state_by_freq(reqFreq, &priv->tripState[i]);
            if (ret) {
                dev_err(&pdev->dev,
                        "[%s] get cooling state error! i:%d ret:%d reqFreq:%d \n",__func__,i,ret,reqFreq);
                priv->tripState[i] = 0;
            }
        }
    } else {
        priv->numTrip   = 0;
        dev_err(&pdev->dev,
                "[%s] thermal-table ERROR! err = %d \n",__func__,err);
        //return -1;
    }

    prop = of_get_property(node, "thermal-trip-shutdown", &size);
    if (prop) {
        unsigned int temp = of_read_number(prop,1);
        if (temp)
            modifyFlags(priv, TRIP_SHUTDOWN, SET);
    }

    prop = of_get_property(node, "thermal0-disable", &size);
    if (prop) {
        unsigned int temp = of_read_number(prop,1);
        if (temp)
            modifyFlags(priv, THERMAL_0_EN, CLEAR);
    }

    prop = of_get_property(node, "thermal1-disable", &size);
    if (prop) {
        unsigned int temp = of_read_number(prop,1);
        if (temp)
            modifyFlags(priv, THERMAL_1_EN, CLEAR);
    }

    prop = of_get_property(node, "thermal-polling-ms", &size);
    if (prop) {
        unsigned int temp = of_read_number(prop,1);
        priv->monitoringRateMs = temp;
    }

    prop = of_get_property(node, "thermal-trend-urgent", &size);
    if (prop) {
        unsigned int temp = of_read_number(prop,1);
        if (temp)
            modifyFlags(priv, TREND_URGENT, SET);
    }

    dev_info(&pdev->dev, "[%s] thermal flags is 0x%lx\n",__func__,priv->flags);

    return 0;
}

static int rtk_thermal_probe(struct platform_device *pdev)
{
    const struct of_device_id *match;
    struct device_node *node = pdev->dev.of_node;
    struct rtk_thermal_priv *priv;
    struct cpumask mask_val;
    int ret;

    match = of_match_device(rtk_thermal_id_table, &pdev->dev);
    if (!match)
        return -ENODEV;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->calibration_base  = of_iomap(node, 0);
    priv->thermal_base[0]   = of_iomap(node, 1);
    priv->thermal_base[1]   = of_iomap(node, 2);

    priv->ops = (struct rtk_thermal_ops *)match->data;

    if (priv->ops->init_sensor)
        priv->ops->init_sensor(priv);

    ret = rtk_thermal_probe_of(priv,pdev);
    if (ret < 0)
        return ret;

    if (priv->ops->post_sensor)
        priv->ops->post_sensor(priv);

    priv->rtk_thermal = thermal_zone_device_register("rtk_thermal", priv->numTrip, 0,
            priv, &ops, NULL, priv->monitoringRateMs, priv->monitoringRateMs);
    if (IS_ERR(priv->rtk_thermal)) {
        dev_err(&pdev->dev,
                "Failed to register thermal zone device\n");
        return PTR_ERR(priv->rtk_thermal);
    }

    cpumask_set_cpu(0, &mask_val);
    cpumask_set_cpu(1, &mask_val);
    priv->rtk_cool_dev = cpufreq_cooling_register(&mask_val);
    if (IS_ERR_OR_NULL(priv->rtk_cool_dev)) {
        int i;
        for(i=0;i<100;i++)
            dev_err(&pdev->dev,
                    "Failed to register cpufreq cooling device\n");
        return PTR_ERR(priv->rtk_cool_dev);
    }

    priv->rtk_thermal->polling_delay = priv->monitoringRateMs;

    priv->mode = THERMAL_DEVICE_ENABLED;

    platform_set_drvdata(pdev, priv->rtk_thermal);

    return 0;
}

static int rtk_thermal_exit(struct platform_device *pdev)
{
    struct thermal_zone_device *rtk_thermal =
        platform_get_drvdata(pdev);
    struct rtk_thermal_priv *priv = rtk_thermal->devdata;

    thermal_zone_device_unregister(priv->rtk_thermal);

    cpufreq_cooling_unregister(priv->rtk_cool_dev);

    platform_set_drvdata(pdev, NULL);
    return 0;
}

static struct platform_driver rtk_thermal_driver = {
    .probe = rtk_thermal_probe,
    .remove = rtk_thermal_exit,
    .driver = {
        .name = "realtek_thermal",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(rtk_thermal_id_table),
    },
};

//module_platform_driver(rtk_thermal_driver);
//module_platform_driver_probe(rtk_thermal_driver, rtk_thermal_probe);

static int __init rtk_thermal_probe_init(void)
{
    return platform_driver_probe(&rtk_thermal_driver, rtk_thermal_probe);
}

late_initcall(rtk_thermal_probe_init);

