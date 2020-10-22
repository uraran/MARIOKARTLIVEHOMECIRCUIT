#include <linux/regulator/machine.h>
#include <linux/i2c/twl.h>
#include <linux/mfd/rtp5901-mfd.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/i2c.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>

//#include <mach/gpio.h>
//#include <mach/iomux.h>
//#include <mach/sram.h>
#include <asm/io.h>

#ifdef CONFIG_MFD_RTP5901
int rtp5901_ic_type = RTP5901_IC_TYPE_A;
uint8_t rtp5901_regu_init_vol[RTP5901_ID_REGULATORS_NUM] = {0, 0x02, 0x0F, 0x0F, 0x1E, 0x14, 0x0F, 0x20, 0x18};

static int rtp5901_regu_vol[RTP5901_ID_REGULATORS_NUM] = {0, 1200000, 3300000, 3300000, 3300000, 1200000, 1200000, 1500000, 3100000};
static int rtp5901_regu_stb_vol[RTP5901_ID_REGULATORS_NUM] = {0, 1200000, 3300000, 3300000, 3000000, 900000, 900000, 1500000, 3100000};
static int rtp5901_regu_stb_en[RTP5901_ID_REGULATORS_NUM] = {0, 0, 0, 0, 0, 0, 1, 0, 0};
static struct regulator_init_data rtp5901_init_rdatas[];
static struct rtp5901_board rtp5901_data;
/*
static int __init rtp5901_regu_vol_setup(char *str)
{
	char *p_st = str;
	char *p_en = NULL;
	int i;

	for (i = 0; i < RTP5901_ID_REGULATORS_NUM; i++) {
		rtp5901_regu_vol[i] = simple_strtoul(p_st, &p_en, 10);
		if (p_en == NULL || p_en[0] != ',')
			break;
		p_st = p_en + 1;
	}

	return 1;
}
__setup("rtp5901_regu_vol=", rtp5901_regu_vol_setup);

static int __init rtp5901_regu_stb_vol_setup(char *str)
{
	char *p_st = str;
	char *p_en = NULL;
	int i;

	for (i = 0; i < RTP5901_ID_REGULATORS_NUM; i++) {
		rtp5901_regu_stb_vol[i] = simple_strtoul(p_st, &p_en, 10);
		if (p_en == NULL || p_en[0] != ',')
			break;
		p_st = p_en + 1;
	}

	return 1;
}
__setup("rtp5901_regu_stb_vol=", rtp5901_regu_stb_vol_setup);

static int __init rtp5901_regu_stb_en_setup(char *str)
{
	char *p_st = str;
	char *p_en = NULL;
	int i;

	for (i = 0; i < RTP5901_ID_REGULATORS_NUM; i++) {
		rtp5901_regu_stb_en[i] = simple_strtoul(p_st, &p_en, 10);
		if (p_en == NULL || p_en[0] != ',')
			break;
		p_st = p_en + 1;
	}

	return 1;
}
__setup("rtp5901_regu_stb_en=", rtp5901_regu_stb_en_setup);*/

int rtp5901_pre_init(struct rtp5901_mfd_chip *chip)
{
	int ret, i;

	printk("%s,line=%d\n", __func__,__LINE__);
	for (i = 0; i < RTP5901_ID_REGULATORS_NUM; i++) {
		printk(KERN_ERR "rtp5901_regu_vol[%d] = %d\n", i, rtp5901_regu_vol[i]);
		printk(KERN_ERR "rtp5901_regu_stb_vol[%d] = %d\n", i, rtp5901_regu_stb_vol[i]);
		printk(KERN_ERR "rtp5901_regu_stb_en[%d] = %d\n", i, rtp5901_regu_stb_en[i]);
	}

	// config rk sleep pin
#if 0
	ret = rk_sleep_config();
	if (ret < 0)
		return ret;
#endif

	rtp5901_ic_type = RTP5901_IC_TYPE_A;

	/* stage 1 */
	ret = register_rw_init(chip);
	if (ret < 0)
		return ret;

	/* stage 3 */
	ret = interrupts_init(chip);
	if (ret < 0)
		return ret;

	/* stage 4 */
	ret = uv_init(chip);
	if (ret < 0)
		return ret;

	/* stage 5 */
	ret = ldo_init(chip);
	if (ret < 0)
		return ret;

	/* stage 6 */
	ret = buck_init(chip);
	if (ret < 0)
		return ret;

	/* stage 7 */
	ret = power_stb_init(chip);
	if (ret < 0)
		return ret;

	/* stage 8 */
	ret = system_global_init(chip);
	if (ret < 0)
		return ret;

	return 0;
}

#if 0
int rk_sleep_config(void)
{
	#ifdef CONFIG_RK_CONFIG
	if(sram_gpio_init(get_port_config(pmic_slp).gpio, &pmic_sleep) < 0) {
		printk(KERN_ERR "sram_gpio_init failed\n");
		return -EINVAL;
	}
	if(port_output_init(pmic_slp, 0, "pmic_slp") < 0) {
		printk(KERN_ERR "port_output_init failed\n");
		return -EINVAL;
	}
	#else
	if(sram_gpio_init(PMU_POWER_SLEEP, &pmic_sleep) < 0) {
		printk(KERN_ERR "sram_gpio_init failed\n");
		return -EINVAL;
	}

	gpio_request(PMU_POWER_SLEEP, "NULL");
	gpio_direction_output(PMU_POWER_SLEEP, GPIO_HIGH);
	#endif
	return 0;
}
#endif

/* init continuous r/w */
int register_rw_init(struct rtp5901_mfd_chip *chip)
{
	int ret;
	ret = rtp5901_update_bits(chip, RTP5901_REG_SYS_PAD_CTL, 0x80, 0xc0);
	if (ret < 0) {
		printk(KERN_ERR "Unable to config RTP5901_REG_SYS_PAD_CTL reg\n");
		return ret;
	}
	return 0;
}

/* DC4 OCP */
int dc4_ocp_init(struct rtp5901_mfd_chip *chip)
{
	int ret;
	ret = rtp5901_set_bits(chip, RTP5901_REG_DC_OCP_SET_02, 0x70);
	if (ret < 0) {
		printk(KERN_ERR "Unable to config RTP5901_REG_DC_OCP_SET_02 reg\n");
		return ret;
	}
	return 0;
}

/* initial all interrupts */
int interrupts_init(struct rtp5901_mfd_chip *chip)
{
	int ret;
	if (chip->init_irqs) {
		ret = chip->init_irqs(chip);
		if (ret)
			return ret;
	}
	return 0;
}

/* config UV threshold and UV_PD_EN */
int uv_init(struct rtp5901_mfd_chip *chip)
{
	int ret;
	ret = rtp5901_clr_bits(chip, RTP5901_REG_SYS_UVOV_EN, 0x10);
	if (ret < 0) {
		printk(KERN_ERR "Failed to clr RTP5901_REG_SYS_UVOV_EN\n");
		return ret;
	}

	ret = rtp5901_clr_bits(chip, RTP5901_REG_SYS_UVOV_SET, 0x30);
	if (ret < 0) {
		printk(KERN_ERR "Failed to clr RTP5901_REG_SYS_UVOV_SET\n");
		return ret;
	}

	ret = rtp5901_set_bits(chip, RTP5901_REG_PWR_OFF_CFG, 0x0C);
	if (ret < 0) {
		printk(KERN_ERR "Unable to config RTP5901_REG_PWR_OFF_CFG reg\n");
		return ret;
	}
	return 0;
}

/* LDO1 off, LDO2,3,4,5 Dischargen Disable for power saving */
int ldo_init(struct rtp5901_mfd_chip *chip)
{
	int ret;
	uint8_t val;
	val = 0x42;
	ret = rtp5901_reg_write(chip, RTP5901_REG_LDO1_CTL, &val);
        if (ret < 0) {
                printk(KERN_ERR "Unable to write RTP5901_REG_LDO1_CTL reg\n");
                return ret;
        }

	val = 0x00;
	ret = rtp5901_reg_write(chip, RTP5901_REG_LDO_PD_MODE, &val);
        if (ret < 0) {
                printk(KERN_ERR "Unable to write RTP5901_REG_LDO_PD_MODE reg\n");
                return ret;
        }
	return 0;
}

/* BUCK init */
int buck_init(struct rtp5901_mfd_chip *chip)
{
    int ret;
    uint8_t val;
    //DC1,2,3,4 config
    ret = rtp5901_clr_bits(chip, RTP5901_REG_DC1_CTL_03, 0x03);
    if (ret < 0) {
        printk(KERN_ERR "Failed to clear RTP5901_REG_DC1_CTL_03\n");
        return ret;
    }

    ret = rtp5901_clr_bits(chip, RTP5901_REG_DC2_CTL_03, 0x03);
    if (ret < 0) {
        printk(KERN_ERR "Failed to clear RTP5901_REG_DC2_CTL_03\n");
        return ret;
    }

    ret = rtp5901_clr_bits(chip, RTP5901_REG_DC3_CTL_03, 0x03);
    if (ret < 0) {
        printk(KERN_ERR "Failed to clear RTP5901_REG_DC3_CTL_03\n");
        return ret;
    }

    //BUCK DVS
    val = 0x3F;
    ret = rtp5901_reg_write(chip, RTP5901_REG_DUMMY_FF, &val);
    if (ret < 0) {
        printk(KERN_ERR "Unable to write RTP5901_REG_DUMMY_FF reg\n");
        return ret;
    }

    val = 0x20;
    ret = rtp5901_reg_write(chip, RTP5901_REG_DUMMY_5A, &val);
    if (ret < 0) {
        printk(KERN_ERR "Unable to write RTP5901_REG_DUMMY_5A reg\n");
        return ret;
    }

    val = 0xA0;
    ret = rtp5901_reg_write(chip, RTP5901_REG_DUMMY_A5, &val);
    if (ret < 0) {
        printk(KERN_ERR "Unable to write RTP5901_REG_DUMMY_A5 reg\n");
        return ret;
    }

    val = 0x30;
    ret = rtp5901_reg_write(chip, RTP5901_REG_DVS_CTRL, &val);
    if (ret < 0) {
        printk(KERN_ERR "Unable to write RTP5901_REG_DVS_CTRL reg\n");
        return ret;
    }

    //BUCK4
    /**********************************/
#define RTP5901_REG_PFM_PWM_CTRL_DC_PWM_FORCE(x)  (1U << (x-1))
    /**********************************/
    val = 0x08;
#if 1
    val |= RTP5901_REG_PFM_PWM_CTRL_DC_PWM_FORCE(2);
    val |= RTP5901_REG_PFM_PWM_CTRL_DC_PWM_FORCE(4);
#endif
    ret = rtp5901_reg_write(chip, RTP5901_REG_PFM_PWM_CTRL, &val);
    if (ret < 0) {
        printk(KERN_ERR "Unable to write RTP5901_REG_PFM_PWM_CTRL reg\n");
        return ret;
    }
    val = 0x00;
#if 1
    val |= RTP5901_REG_PFM_PWM_CTRL_DC_PWM_FORCE(2);
    val |= RTP5901_REG_PFM_PWM_CTRL_DC_PWM_FORCE(4);
#endif
    ret = rtp5901_reg_write(chip, RTP5901_REG_PFM_PWM_CTRL, &val);
    if (ret < 0) {
        printk(KERN_ERR "Unable to write RTP5901_REG_PFM_PWM_CTRL reg\n");
        return ret;
    }

    val = 0x07;
    ret = rtp5901_reg_write(chip, RTP5901_REG_DC_UVOV_PD_PMU, &val);
    if (ret < 0) {
        printk(KERN_ERR "Unable to write RTP5901_REG_DC_UVOV_PD_PMU reg\n");
        return ret;
    }

#if 1
    /**********************************/
#define RTP5901_REG_DVS_STEP_CTRL_DC_DVS_STEP_MASK(id) (0x3U << (id -1))
#define RTP5901_REG_DVS_STEP_CTRL_DC_DVS_STEP(val,id,mod) ((val & ~RTP5901_REG_DVS_STEP_CTRL_DC_DVS_STEP_MASK(2*id-1)) | (mod << (2*id -2)))
    /**********************************/

    ret = rtp5901_reg_read(chip, RTP5901_REG_DVS_STEP_CTRL, &val);
    if (ret < 0) {
        printk(KERN_ERR "Unable to read RTP5901_REG_DVS_STEP_CTRL reg\n");
        return ret;
    }

    
    val = RTP5901_REG_DVS_STEP_CTRL_DC_DVS_STEP(val,2,1);
    ret = rtp5901_reg_write(chip, RTP5901_REG_DVS_STEP_CTRL, &val);

    if (ret < 0) {
        printk(KERN_ERR "Unable to write RTP5901_REG_DVS_STEP_CTRL reg\n");
        return ret;
    }

#endif
    return 0;
}

int power_stb_init(struct rtp5901_mfd_chip *chip)
{
	int ret, i;
	uint8_t val;
	ret = rtp5901_set_bits(chip, RTP5901_REG_EXTEN_EN, 0x80);
	if (ret < 0) {
		printk(KERN_ERR "Unable to config RTP5901_REG_EXTEN_EN reg\n");
		return ret;
	}

	val = 0x00;
	for (i = 1; i < RTP5901_ID_REGULATORS_NUM; i++) {
		if (rtp5901_regu_stb_en[i] == 1) {
			if (i <= RTP5901_ID_LDO5)
				val |= 0x01 << (i + 3);
			else
				val |= 0x01 << (i - 5);
		}
	}
	ret = rtp5901_reg_write(chip, RTP5901_REG_POWER_STB_EN, &val);
	if (ret < 0) {
		printk(KERN_ERR "Unable to write RTP5901_REG_POWER_STB_EN reg\n");
		return ret;
	}

	ret = rtp5901_set_bits(chip, RTP5901_REG_GPIO0_CTRL, 0x08);
	if (ret < 0) {
		printk(KERN_ERR "Unable to config RTP5901_REG_GPIO0_CTRL reg\n");
		return ret;
	}
	return 0;
}

int system_global_init(struct rtp5901_mfd_chip *chip)
{
	int ret;
	uint8_t val;
	ret = rtp5901_clr_bits(chip, RTP5901_REG_SYS_CLK_EN, 0x08);
	if (ret < 0) {
		printk(KERN_ERR "Failed to disable rtc output\n");
		return ret;
	}

	ret = rtp5901_update_bits(chip, RTP5901_REG_EXTEN_EN, 0x04, 0x05);
	if (ret < 0) {
		printk(KERN_ERR "Unable to config RTP5901_REG_EXTEN_EN reg\n");
		return ret;
	}

	ret = rtp5901_update_bits(chip, RTP5901_REG_PWRHOLD, 0xA0, 0xF0);
        if (ret < 0) {
                printk(KERN_ERR "Failed to set RTP5901_REG_PWRHOLD\n");
                return ret;
        }

	val = 0x58;
	ret = rtp5901_reg_write(chip, RTP5901_REG_GLOBAL_CFG1, &val);
        if (ret < 0) {
                printk(KERN_ERR "Unable to write RTP5901_REG_GLOBAL_CFG1 reg\n");
                return ret;
        }

	ret = rtp5901_clr_bits(chip, RTP5901_REG_GLOBAL_CFG2, 0x02);
	if (ret < 0) {
		printk(KERN_ERR "Failed to clear RTP5901_REG_GLOBAL_CFG2\n");
		return ret;
	}

	val = 0x00;
	ret = rtp5901_reg_write(chip, RTP5901_REG_DEBUGO_EXT_MUX1, &val);
        if (ret < 0) {
                printk(KERN_ERR "Unable to write RTP5901_REG_DEBUGO_EXT_MUX1 reg\n");
                return ret;
        }
	return 0;
}

int rtp5901_post_init(struct rtp5901_mfd_chip *chip)
{
	struct regulator *dcdc;
	//struct regulator *ldo;

	printk("%s,line=%d\n", __func__,__LINE__);

#if 0
	g_pmic_type = PMIC_TYPE_RTP5901;
	printk("%s:g_pmic_type=%d\n", __func__, g_pmic_type);
#endif

#if 0
	ldo = regulator_get(NULL, "vdd_12");	//vcc_12
	//regulator_set_voltage(ldo, rtp5901_regu_vol[RTP5901_ID_LDO2], rtp5901_regu_vol[RTP5901_ID_LDO2]);
	//regulator_set_suspend_voltage(ldo, rtp5901_regu_stb_vol[RTP5901_ID_LDO2]);	// set suspend voltage
	regulator_enable(ldo);
	printk("%s set vcc_12=%dmV end\n", __func__, regulator_get_voltage(ldo));
	regulator_put(ldo);
	udelay(100);

	ldo = regulator_get(NULL, "vcc_tp");	//vcc_tp
	//regulator_set_voltage(ldo, rtp5901_regu_vol[RTP5901_ID_LDO3], rtp5901_regu_vol[RTP5901_ID_LDO3]);
	//regulator_set_suspend_voltage(ldo, rtp5901_regu_stb_vol[RTP5901_ID_LDO3]);	// set suspend voltage
	regulator_enable(ldo);
	printk("%s set vcc_tp=%dmV end\n", __func__, regulator_get_voltage(ldo));
	regulator_put(ldo);
	udelay(100);

	ldo = regulator_get(NULL, "vcca_33");	//vcc_33
	//regulator_set_voltage(ldo, rtp5901_regu_vol[RTP5901_ID_LDO4], rtp5901_regu_vol[RTP5901_ID_LDO4]);
	//regulator_set_suspend_voltage(ldo, rtp5901_regu_stb_vol[RTP5901_ID_LDO4]);	// set suspend voltage
	regulator_enable(ldo);
	printk("%s set vcca_33=%dmV end\n", __func__, regulator_get_voltage(ldo));
	regulator_put(ldo);
	udelay(100);

	ldo = regulator_get(NULL, "vcc_wl");	//vcc_wl
	//regulator_set_voltage(ldo, rtp5901_regu_vol[RTP5901_ID_LDO5], rtp5901_regu_vol[RTP5901_ID_LDO5]);
	//regulator_set_suspend_voltage(ldo, rtp5901_regu_stb_vol[RTP5901_ID_LDO5]);	// set suspend voltage
	regulator_enable(ldo);
	printk("%s set vcc_wl=%dmV end\n", __func__, regulator_get_voltage(ldo));
	regulator_put(ldo);
	udelay(100);
#endif
	{
        struct regulator_init_data * pRegulatorDate=  rtp5901_data.rtp5901_pmic_init_data[RTP5901_ID_BUCK1];
	int ret;

	dcdc = regulator_get(NULL, "vdd_cpu");	//vcc_arm
	regulator_set_voltage(dcdc, pRegulatorDate->constraints.min_uV,  pRegulatorDate->constraints.max_uV );
	//regulator_set_suspend_voltage(dcdc, rtp5901_regu_stb_vol[RTP5901_ID_BUCK1]);	// set suspend voltage
	ret = regulator_enable(dcdc);
	if (ret < 0)
		pr_err("%s, regulator_enable fail (ret=%d)\n", __func__, ret);
	printk("%s set vdd_cpu=%dmV end\n", __func__, regulator_get_voltage(dcdc));
	regulator_put(dcdc);
	udelay(100);
	}
#if 0
	dcdc = regulator_get(NULL, "vdd_core");	//vcc_log
	//regulator_set_voltage(dcdc, rtp5901_regu_vol[RTP5901_ID_BUCK2], rtp5901_regu_vol[RTP5901_ID_BUCK2]);
	//regulator_set_suspend_voltage(dcdc, rtp5901_regu_stb_vol[RTP5901_ID_BUCK2]);	// set suspend voltage
	regulator_enable(dcdc);
	printk("%s set vdd_core=%dmV end\n", __func__, regulator_get_voltage(dcdc));
	regulator_put(dcdc);
	udelay(100);

	dcdc = regulator_get(NULL, "vcc_ddr");	//vcc_ddr
	//regulator_set_voltage(dcdc, rtp5901_regu_vol[RTP5901_ID_BUCK3], rtp5901_regu_vol[RTP5901_ID_BUCK3]);
	//regulator_set_suspend_voltage(dcdc, rtp5901_regu_stb_vol[RTP5901_ID_BUCK3]);	// set suspend voltage
	regulator_enable(dcdc);
	printk("%s set vcc_ddr=%dmV end\n", __func__, regulator_get_voltage(dcdc));
	regulator_put(dcdc);
	udelay(100);

	dcdc = regulator_get(NULL, "vcc_io");	//vcc_io
	//regulator_set_voltage(dcdc, rtp5901_regu_vol[RTP5901_ID_BUCK4], rtp5901_regu_vol[RTP5901_ID_BUCK4]);
	//regulator_set_suspend_voltage(dcdc, rtp5901_regu_stb_vol[RTP5901_ID_BUCK4]);	// set suspend voltage
	regulator_enable(dcdc);
	printk("%s set vcc_io=%dmV end\n", __func__, regulator_get_voltage(dcdc));
	regulator_put(dcdc);
	udelay(100);

	printk("%s,line=%d END\n", __func__,__LINE__);
#endif

	return 0;
}

int rtp5901_late_exit(struct rtp5901_mfd_chip *chip)
{
	uint8_t val;

	printk("%s,line=%d\n", __func__,__LINE__);

	// disable extern en
	rtp5901_clr_bits(chip, RTP5901_REG_EXTEN_EN, 0x04);

	// go to active state
	rtp5901_reg_read(chip, RTP5901_REG_FSM_DEBUG, &val);
	if ((val & 0x07) != 0x06)
		rtp5901_set_bits(chip, RTP5901_REG_PMU_STATE_CTL, 0x02);

	printk("%s,line=%d END\n", __func__,__LINE__);

	return 0;
}

static struct regulator_consumer_supply ldo1_data[] = {
	{
		.supply = "vdd_rtc_null",
	},
};

static struct regulator_consumer_supply ldo2_data[] = {
	{
		.supply = "vdd_12",
	},
};

static struct regulator_consumer_supply ldo3_data[] = {
	{
		.supply = "vcc_tp",
	},
};

static struct regulator_consumer_supply ldo4_data[] = {
	{
		.supply = "vcca_33",
	},
};

static struct regulator_consumer_supply ldo5_data[] = {
	{
		.supply = "vcc_wl",
	},
};

static struct regulator_consumer_supply buck1_data[] = {
	{
		.supply = "vdd_cpu",
	},
};

static struct regulator_consumer_supply buck2_data[] = {
	{
#if 0
		.supply = "vdd_core",
#else
		.supply = "vddarm",
#endif
	},
};

static struct regulator_consumer_supply buck3_data[] = {
	{
		.supply = "vcc_ddr",
	},
};

static struct regulator_consumer_supply buck4_data[] = {
	{
		.supply = "vcc_io",
	},
};

static struct regulator_init_data rtp5901_init_rdatas[] = {
	[RTP5901_ID_LDO1] = {
		.constraints = {
			.name = "VDD_RTC",
			.min_uV = RTP5901_LDO1_VOL * 1000,
			.max_uV = RTP5901_LDO1_VOL * 1000,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo1_data),
		.consumer_supplies = ldo1_data,
	},
	[RTP5901_ID_LDO2] = {
		.constraints = {
			.name = "VDD_12",
			.min_uV = 1000000,
			.max_uV = 2500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
//			.initial_state = PM_SUSPEND_STANDBY,
//			.state_standby = {
//				.uV = , 	// set suspend vol
//				.enable = 1,	// set suspend enable
//			},
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo2_data),
		.consumer_supplies = ldo2_data,
	},
	[RTP5901_ID_LDO3] = {
		.constraints = {
			.name = "VCC_TP",
			.min_uV = 1800000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo3_data),
		.consumer_supplies = ldo3_data,
	},
	[RTP5901_ID_LDO4] = {
		.constraints = {
			.name = "VCCA_33",
			.min_uV = 1800000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo4_data),
		.consumer_supplies = ldo4_data,
	},
	[RTP5901_ID_LDO5] = {
		.constraints = {
			.name = "VCC_WL",
			.min_uV = 1800000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo5_data),
		.consumer_supplies = ldo5_data,
	},
	[RTP5901_ID_BUCK1] = {
		.constraints = {
			.name = "VDD_ARM",
			.min_uV = 700000,
			.max_uV = 2275000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(buck1_data),
		.consumer_supplies = buck1_data,
	},
	[RTP5901_ID_BUCK2] = {
		.constraints = {
			.name = "VDD_LOG",
			.min_uV = 700000,
			.max_uV = 2275000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
            .always_on = 1,
		},
		.num_consumer_supplies = ARRAY_SIZE(buck2_data),
		.consumer_supplies = buck2_data,
	},
	[RTP5901_ID_BUCK3] = {
		.constraints = {
			.name = "VCC_DDR",
			.min_uV = 700000,
			.max_uV = 2275000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(buck3_data),
		.consumer_supplies = buck3_data,
	},
	[RTP5901_ID_BUCK4] = {
		.constraints = {
			.name = "VCC_IO",
			.min_uV = 1700000,
			.max_uV = 3500000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(buck4_data),
		.consumer_supplies = buck4_data,
	},
};

#if 0
// suspend and resume
void __sramfunc board_pmu_rtp5901_suspend(void)
{
	#ifdef CONFIG_CLK_SWITCH_TO_32K
	sram_gpio_set_value(pmic_sleep, GPIO_LOW);
	#endif
}
void __sramfunc board_pmu_rtp5901_resume(void)
{
	#ifdef CONFIG_CLK_SWITCH_TO_32K
	sram_gpio_set_value(pmic_sleep, GPIO_HIGH);
	sram_32k_udelay(10000);
	#endif
}
#endif

static struct rtp5901_board rtp5901_data = {
	.pre_init = rtp5901_pre_init,
	.post_init = rtp5901_post_init,
	.late_exit = rtp5901_late_exit,
	// Regulators
#if 0
	.rtp5901_pmic_init_data[RTP5901_ID_LDO1] = NULL,					// 0
	.rtp5901_pmic_init_data[RTP5901_ID_LDO2] = &rtp5901_init_rdatas[RTP5901_ID_LDO2],	// 1
	.rtp5901_pmic_init_data[RTP5901_ID_LDO3] = &rtp5901_init_rdatas[RTP5901_ID_LDO3],	// 2
	.rtp5901_pmic_init_data[RTP5901_ID_LDO4] = &rtp5901_init_rdatas[RTP5901_ID_LDO4],	// 3
	.rtp5901_pmic_init_data[RTP5901_ID_LDO5] = &rtp5901_init_rdatas[RTP5901_ID_LDO5],	// 4
	.rtp5901_pmic_init_data[RTP5901_ID_BUCK1] = &rtp5901_init_rdatas[RTP5901_ID_BUCK1],	// 5
	.rtp5901_pmic_init_data[RTP5901_ID_BUCK2] = &rtp5901_init_rdatas[RTP5901_ID_BUCK2],	// 6
	.rtp5901_pmic_init_data[RTP5901_ID_BUCK3] = &rtp5901_init_rdatas[RTP5901_ID_BUCK3],	// 7
	.rtp5901_pmic_init_data[RTP5901_ID_BUCK4] = &rtp5901_init_rdatas[RTP5901_ID_BUCK4],	// 8
#else
    /* Default for 5903 */
	.rtp5901_pmic_init_data[RTP5901_ID_LDO1] = NULL,
	.rtp5901_pmic_init_data[RTP5901_ID_LDO2] = NULL,
	.rtp5901_pmic_init_data[RTP5901_ID_LDO3] = &rtp5901_init_rdatas[RTP5901_ID_LDO3],
	.rtp5901_pmic_init_data[RTP5901_ID_LDO4] = NULL,
	.rtp5901_pmic_init_data[RTP5901_ID_LDO5] = &rtp5901_init_rdatas[RTP5901_ID_LDO5],
	.rtp5901_pmic_init_data[RTP5901_ID_BUCK1] = &rtp5901_init_rdatas[RTP5901_ID_BUCK1],
	.rtp5901_pmic_init_data[RTP5901_ID_BUCK2] = &rtp5901_init_rdatas[RTP5901_ID_BUCK2],
	.rtp5901_pmic_init_data[RTP5901_ID_BUCK3] = &rtp5901_init_rdatas[RTP5901_ID_BUCK3],
	.rtp5901_pmic_init_data[RTP5901_ID_BUCK4] = &rtp5901_init_rdatas[RTP5901_ID_BUCK4],
#endif
};

static int rtk_mfd_rtp5901_probe(struct platform_device *pdev)
{
    int err = 0;
    int of_irq = 0;
    //int of_irq_base = 0;
    int of_gpio_base = 0;
    int i;
    {
        int num_gpios = of_gpio_count(pdev->dev.of_node);
        if (num_gpios) {
            for (i=0;i<num_gpios;i++) {
                int gpio = of_get_gpio_flags(pdev->dev.of_node, i, NULL);
                if (gpio < 0) {
                    printk("[%s ] could not get gpio from of \n",__FUNCTION__);
                }
                if (!gpio_is_valid(gpio)) {
                    printk("[%s ] gpio %d is not valid\n",__FUNCTION__, gpio);
                }
                if(gpio_request(gpio, pdev->dev.of_node->name)) {
                    printk("[%s ] could not request gpio, %d\n",__FUNCTION__, gpio);
                }
                //gVCtl.gpio[i] = gpio;
            }
        } else {
            printk("[%s] voltage step ctl ERROR! err %d tag %d\n",__func__,err,2);
        }

        of_irq = irq_of_parse_and_map(pdev->dev.of_node, 0); //parsing irq num from device tree
        if (!of_irq) {
            printk("gpio test: fail to parse of irq.\n");
        }

    }
#ifdef CONFIG_GPIO_RTP5901
    {
        int size;
        const u32 *prop = of_get_property(pdev->dev.of_node, "gpio_base", &size);
        if (prop) {
            of_gpio_base = of_read_number(prop,1);
        }

        if (of_gpio_base <= 0) {
            of_gpio_base = 81;
            printk("[%s %d] gpio_base not find! set default : %d\n",__func__,__LINE__,of_gpio_base);
        }
    }
#endif
    {
#define REGULATOR_COLUMN 3
        int size;
        const u32 *prop = of_get_property(pdev->dev.of_node, "regulator-table", &size);
        err = size % (sizeof(u32)*REGULATOR_COLUMN);
        if ((prop) && (!err)) {
            int counter = size / (sizeof(u32)*REGULATOR_COLUMN);
            if (counter > RTP5901_ID_REGULATORS_NUM)
                counter = RTP5901_ID_REGULATORS_NUM;
            for (i=0;i<counter;i+=1) {
                int enable = of_read_number(prop, 1 + (i*REGULATOR_COLUMN));
                struct regulator_init_data * pRegulatorDate = rtp5901_data.rtp5901_pmic_init_data[i];

                if (!pRegulatorDate)
                    continue;

                if (!enable) {
                    rtp5901_data.rtp5901_pmic_init_data[i] = NULL;
                    continue;
                }

                pRegulatorDate->constraints.min_uV = of_read_number(prop, 2 + (i*REGULATOR_COLUMN));
                pRegulatorDate->constraints.max_uV = of_read_number(prop, 3 + (i*REGULATOR_COLUMN));
            }
        } else {
            printk("[%s] regulator-table ERROR! err = %d \n",__func__,err);
        }
    }

    rtp5901_data.irq        = (unsigned) of_irq;
#if 1
    rtp5901_data.irq_base   = 0;
#else
    // NOT READY!!!
    rtp5901_data.irq_base   = irq_alloc_descs(of_irq, 0, RTP5901_IRQ_NUM, 0);
#endif
    rtp5901_data.gpio_base  = of_gpio_base;

    printk("[rtp5901] irq        = %d\n",rtp5901_data.irq        );
    printk("[rtp5901] irq_base   = %d\n",rtp5901_data.irq_base   );
    printk("[rtp5901] gpio_base  = %d\n",rtp5901_data.gpio_base  );

    return err;
}

static struct of_device_id rtk_mfd_rtp5901_ids[] = {
	{ .compatible = "Realtek,rtk119x-rtp5901" },
	{ /* Sentinel */ },
};

MODULE_DEVICE_TABLE(of, rtk_mfd_rtp5901_ids);

static struct platform_driver realtek_rtp5901_platdrv = {
    .driver = {
        .name   = "rtk119x-mfd-rtp5901",
        .owner  = THIS_MODULE,
        .of_match_table = rtk_mfd_rtp5901_ids,
    },
    .probe      = rtk_mfd_rtp5901_probe,
};

#if 0
module_platform_driver_probe(realtek_rtp5901_platdrv, rtk_mfd_rtp5901_probe);

static int __init rtp5901_mfd_init_1195(void)
{
    struct i2c_adapter *adapter = i2c_get_adapter(0);
#else
static int __init rtp5901_mfd_init_1195(void)
{
    struct i2c_adapter *adapter = i2c_get_adapter(0);
    int ret = platform_driver_probe(&realtek_rtp5901_platdrv, rtk_mfd_rtp5901_probe);
    if (ret) {
        printk("[%s:%d] rtp5901 prop ERROR!!!!!\n",__func__,__LINE__);
        return -ENODEV;
    }
#endif

    if (rtp5901_data.irq == 0) {
        printk("[%s:%d] rtp5901 not find!\n",__func__,__LINE__);
        return -ENODEV;
    }

    if (adapter == NULL) {
        /* Eek, no such I2C adapter!  Bail out. */
        printk("ERROR!!!!! [%s:%d] i2c adapter 0 NOT FIND!!!!\n",__func__,__LINE__);
        return -ENODEV;
    } else {
        struct i2c_board_info info;
        struct i2c_client * client;

        memset(&info,0,sizeof(info));
        strlcpy(info.type, "rtp5901_mfd", I2C_NAME_SIZE);
        info.platform_data = &rtp5901_data;
        info.addr = RTP5901_ADDRESS;	// i2c address hufikyu
        client = i2c_new_device(adapter, &info);
        if (!client) {
            printk("ERROR!!!!! [%s:%d] new i2c devices failed!\n",__func__,__LINE__);
            return -ENODEV;
        }
    }
    return 0;
}

#if 0
late_initcall(rtp5901_mfd_init_1195);
#else
fs_initcall(rtp5901_mfd_init_1195);
#endif
#endif
