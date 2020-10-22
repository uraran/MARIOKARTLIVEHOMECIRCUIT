#include <linux/mutex.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/string.h>

#include <linux/delay.h>

#include "clk-factors.h"

struct clk_factors {
	struct clk_hw hw;
	void __iomem *reg;
	struct clk_factors_config *config;
    void (*get_factors) (u32 *rate, u32 parent_rate, void * config, void *data, size_t dataSize);
	spinlock_t *lock;
	struct mutex mlock;
};

#define to_clk_factors(_hw) container_of(_hw, struct clk_factors, hw)

#define SETMASK(len, pos)		(((-1U) >> (31-len))  << (pos))
#define CLRMASK(len, pos)		(~(SETMASK(len, pos)))
#define FACTOR_GET(bit, len, reg)	(((reg) & SETMASK(len, bit)) >> (bit))

#define FACTOR_SET(bit, len, reg, val) \
	(((reg) & CLRMASK(len, bit)) | (val << (bit)))

#define RTK_FACTOR_GET(reg, offset, len, shift) FACTOR_GET(shift,len, readl(reg+offset));

static int clk_factors_set_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long parent_rate);

static unsigned long clk_factors_recalc_rate(struct clk_hw *hw,
					     unsigned long parent_rate)
{
	unsigned long rate;
	struct clk_factors *factors = to_clk_factors(hw);
	struct clk_factors_config *config = factors->config;
    struct clk_factors_data data;

    data.n = RTK_FACTOR_GET(factors->reg, config->noffset, config->nlen, config->nshift);
    data.f = RTK_FACTOR_GET(factors->reg, config->foffset, config->flen, config->fshift);
    data.d = RTK_FACTOR_GET(factors->reg, config->doffset, config->dlen, config->dshift);

	/* Calculate the rate */
    if (config->id == SPLL) {
        int dev = (data.d == 3) ? 4 : (data.d == 2) ? 2 : 1;
        rate = ((parent_rate * (data.n + 0x1)) + ((parent_rate/2048) * data.f )) / dev;
    } else {
        rate = parent_rate * (data.n + 2);
        rate /= (data.f + 1);
        rate /= (data.d + 1);
    }

    return rate;
}

static long clk_factors_round_rate(struct clk_hw *hw, unsigned long rate,
				   unsigned long *parent_rate)
{
	struct clk_factors *factors = to_clk_factors(hw);
	struct clk_factors_config *config = factors->config;
    struct clk_factors_data data;
	factors->get_factors((u32 *)&rate, (u32)*parent_rate, (void *)config, (void *) &data, sizeof(data));
	return rate;
}

#define DIVIDER_LIMIT 900000000 // 900MHz
static int clk_factors_set_rate_limit(struct clk_hw *hw, unsigned long rate,
				unsigned long parent_rate)
{
	struct clk_factors *factors = to_clk_factors(hw);
	struct clk_factors_config *config = factors->config;
    int err = 0;

    if (config->id == SPLL && (is_revinfo_0 || is_revinfo_1)) {
        struct clk_factors_data fData;
        unsigned long target_rate = rate;
        unsigned long get_rate = clk_factors_recalc_rate(hw, parent_rate);
        u32 old_d = RTK_FACTOR_GET(factors->reg, config->doffset, config->dlen, config->dshift);
        factors->get_factors((u32 *)&target_rate, (u32)parent_rate, (void *)config, (void *) &fData, sizeof(fData));

        if (fData.d != old_d) {
            if (    ((target_rate > DIVIDER_LIMIT) && (get_rate < DIVIDER_LIMIT))
                 || ((target_rate < DIVIDER_LIMIT) && (get_rate > DIVIDER_LIMIT))
               )
                err = clk_factors_set_rate(hw, DIVIDER_LIMIT, parent_rate);
        }

        if (err != 0) goto ERROR;

    }

    return clk_factors_set_rate(hw, rate, parent_rate);

ERROR:
    pr_err("%s: ERROR! id:%-2d rate = %d parent_rate = %d\n", __func__,config->id,rate,parent_rate);
    return err;
}

static int clk_factors_set_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long parent_rate)
{
	struct clk_factors *factors = to_clk_factors(hw);
	struct clk_factors_config *config = factors->config;
    struct clk_factors_data newData, oldData;
	unsigned long flags = 0;

    oldData.n = RTK_FACTOR_GET(factors->reg, config->noffset, config->nlen, config->nshift);
    oldData.f = RTK_FACTOR_GET(factors->reg, config->foffset, config->flen, config->fshift);
    oldData.d = RTK_FACTOR_GET(factors->reg, config->doffset, config->dlen, config->dshift);

	factors->get_factors((u32 *)&rate, (u32)parent_rate, (void *)config, (void *) &newData, sizeof(newData));

    if (oldData.n == newData.n && oldData.f == newData.f && oldData.d == newData.d) {
        return 0;
    } else {
	/*
        printk(KERN_INFO " [clk] frequency change: [%10d > %10d] id:%-2d n:[%2d > %2d] f:[%4d > %4d] d:[%d > %d]\n",
                clk_factors_recalc_rate(hw,parent_rate), rate, config->id,
                oldData.n, newData.n, oldData.f, newData.f, oldData.d, newData.d);
	*/
    }

#if 1
    mutex_lock(&factors->mlock);
#else
	if (factors->lock)
		spin_lock_irqsave(factors->lock, flags);
#endif

    if (config->id == SPLL) {
        // Set Divider
        if (newData.d > oldData.d) {
            u32 reg = readl((factors->reg + config->doffset));
            reg = FACTOR_SET(config->dshift, config->dlen, reg, newData.d);
            writel(reg, (factors->reg + config->doffset));
        }
    }

    {
        if (newData.n < oldData.n) {
            u32 reg = readl((factors->reg + config->noffset));
            reg = FACTOR_SET(config->nshift, config->nlen, reg, newData.n);
            writel(reg, (factors->reg + config->noffset));
        }
        if (config->id == SPLL) {
            if (newData.f < oldData.f) {
                u32 reg = readl((factors->reg + config->foffset));
                reg = FACTOR_SET(config->fshift, config->flen, reg, newData.f);
                writel(reg, (factors->reg + config->foffset));
            }
        } else {
            if (newData.f > oldData.f) {
                u32 reg = readl((factors->reg + config->foffset));
                reg = FACTOR_SET(config->fshift, config->flen, reg, newData.f);
                writel(reg, (factors->reg + config->foffset));
            }
        }
        if (config->id != SPLL) {
            if (newData.d > oldData.d) {
                u32 reg = readl((factors->reg + config->doffset));
                reg = FACTOR_SET(config->dshift, config->dlen, reg, newData.d);
                writel(reg, (factors->reg + config->doffset));
            }
        }

    }

    {
        if (newData.n > oldData.n) {
            u32 reg = readl((factors->reg + config->noffset));
            reg = FACTOR_SET(config->nshift, config->nlen, reg, newData.n);
            writel(reg, (factors->reg + config->noffset));
        }
        if (config->id == SPLL) {
            if (newData.f > oldData.f) {
                u32 reg = readl((factors->reg + config->foffset));
                reg = FACTOR_SET(config->fshift, config->flen, reg, newData.f);
                writel(reg, (factors->reg + config->foffset));
            }
        } else {
            if (newData.f < oldData.f) {
                u32 reg = readl((factors->reg + config->foffset));
                reg = FACTOR_SET(config->fshift, config->flen, reg, newData.f);
                writel(reg, (factors->reg + config->foffset));
            }
        }
        if (config->id != SPLL) {
            if (newData.d < oldData.d) {
                u32 reg = readl((factors->reg + config->doffset));
                reg = FACTOR_SET(config->dshift, config->dlen, reg, newData.d);
                writel(reg, (factors->reg + config->doffset));
            }
        }

    }

    if (config->id == SPLL) {
        {
            // Set OS Step
            u32 offset = config->noffset; // 0x18000104
            const u32 step = 3; // bit [9:0]
            const u32 shift = 0;
            const u32 len = 9;
            u32 reg = readl((factors->reg + offset));
            reg = FACTOR_SET(shift, len, reg, step);
            writel(reg, (factors->reg + offset));
        }
        {
            // Set OC Mode
            enum {
                LowSpeedMode            = 0,
                HightSpeedMode          = 1,
                AdaptiveHighSpeedMode   = 2,
            } REG_SEL_OC_MODE;
            u32 offset = config->noffset + 0x4; // 0x18000108
            u32 reg = readl((factors->reg + offset));
            reg = FACTOR_SET(25, 1, reg, LowSpeedMode);

            // Trigger OC Change
            reg &= 0xfffffffe;
            writel(reg, (factors->reg + offset));
            reg |= 1;
            writel(reg, (factors->reg + offset));
            //__delay((rate >> 20) * 500 / 2);
            //udelay(1000);
            usleep_range(500,600);
        }
        {
            // Set Divider
            if (newData.d < oldData.d) {
                u32 reg = readl((factors->reg + config->doffset));
                reg = FACTOR_SET(config->dshift, config->dlen, reg, newData.d);
                writel(reg, (factors->reg + config->doffset));
                //udelay(10);
            }
        }
    } else if (config->id == VPLL || config->id == APLL) {
        const u32 offset = 4;
        u32 reg = 5;
        writel(reg, (factors->reg + offset));
        reg = 7;
        writel(reg, (factors->reg + offset));
        msleep(10);
        reg = 3;
        writel(reg, (factors->reg + offset));
    }

#if 0

	/* Fetch the register value */
    //FIXME:: It's NOT INTEGRITY (reg0: d | reg1: n,f)
    reg0 = readl((factors->reg + config->doffset));
    reg1 = readl((factors->reg + config->noffset));
    enum {
        LowSpeedMode = 0,
        HightSpeedMode = 1,
        AdaptiveHighSpeedMode = 2
    } REG_SEL_OC_MODE;
    /*
     * REG_SEL_OC_MODE
     * 2'b00:   Low speed mode
     * 2'b01:   Hight speed mode
     * 2'b1x:   Adaptive high speed mode
     */
    /*
     * OC_EN
     * Enable overclock or uncerclock
     */
    reg2 = readl((factors->reg + config->oc_mod_offset));
    //printk("[%s %d] reg2 = 0x%08x \n",__func__,__LINE__,reg2);

	/* Set up the new factors - macros do not do anything if width is 0 */
	reg0 = FACTOR_SET(config->dshift, config->dwidth, reg0, data.d);
	reg1 = FACTOR_SET(config->nshift, config->nwidth, reg1, data.n);
	reg1 = FACTOR_SET(config->fshift, config->fwidth, reg1, data.f);

	//reg2 = FACTOR_SET(config->oc_mod_shift, config->oc_mod_width, reg2,0x1);
	reg2 = FACTOR_SET(config->oc_mod_shift, config->oc_mod_width, reg2,AdaptiveHighSpeedMode);
	//reg2 = FACTOR_SET(config->oc_en_shift, config->oc_en_width, reg2,0x0);
    reg2 &= 0xfffffffe;

	/* Apply them now */
	writel(reg0, (factors->reg + config->doffset));
	writel(reg1, (factors->reg + config->noffset));

    //printk("[%s %d] reg2 = 0x%08x \n",__func__,__LINE__,reg2);
	writel(reg2, (factors->reg + config->oc_mod_offset));
    reg2 |= 1;
    //printk("[%s %d] reg2 = 0x%08x \n",__func__,__LINE__,reg2);
	writel(reg2, (factors->reg + config->oc_mod_offset));

	/* delay 500us so pll stabilizes */
	__delay((rate >> 20) * 500 / 2);
#endif

#if 1
    mutex_unlock(&factors->mlock);
#else
	if (factors->lock)
		spin_unlock_irqrestore(factors->lock, flags);
#endif

	return 0;
}

static const struct clk_ops clk_factors_ops = {
	.recalc_rate = clk_factors_recalc_rate,
	.round_rate = clk_factors_round_rate,
	//.set_rate = clk_factors_set_rate,
	.set_rate = clk_factors_set_rate_limit,
};

/**
 * clk_register_factors - register a factors clock with
 * the clock framework
 * @dev: device registering this clock
 * @name: name of this clock
 * @parent_name: name of clock's parent
 * @flags: framework-specific flags
 * @reg: register address to adjust factors
 * @config: shift and len of factors n, k, m and p
 * @get_factors: function to calculate the factors for a given frequency
 * @lock: shared register lock for this clock
 */
struct clk *clk_register_factors(struct device *dev, const char *name,
				 const char *parent_name,
				 unsigned long flags, void __iomem *reg,
				 struct clk_factors_config *config,
				 void (*get_factors)(u32 *rate, u32 parent_rate, void * config, void *data, size_t dataSize),
				 spinlock_t *lock)
{
	struct clk_factors *factors;
	struct clk *clk;
	struct clk_init_data init;

	/* allocate the factors */
	factors = kzalloc(sizeof(struct clk_factors), GFP_KERNEL);
	if (!factors) {
		pr_err("%s: could not allocate factors clk\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	init.name = name;
	init.ops = &clk_factors_ops;
	init.flags = flags;
	init.parent_names = (parent_name ? &parent_name : NULL);
	init.num_parents = (parent_name ? 1 : 0);

	/* struct clk_factors assignments */
	factors->reg = reg;
	factors->config = config;
	factors->lock = lock;
	factors->hw.init = &init;
	factors->get_factors = get_factors;
	mutex_init(&factors->mlock);

	/* register the clock */
	clk = clk_register(dev, &factors->hw);

	if (IS_ERR(clk))
		kfree(factors);

	return clk;
}
