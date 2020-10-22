#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/clk/rtk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <rbus/reg.h>

#include "clk-factors.h"
static DEFINE_SPINLOCK(clk_lock);

/**
 * Setup function for gatable oscillator
 */

static int debug    = 0;
static int warning  = 1;
static int info     = 0;
#define dprintk(msg...) if (debug)   { printk(KERN_DEBUG    "D/CLK: " msg); }
#define eprintk(msg...) if (1)       { printk(KERN_ERR      "E/CLK: " msg); }
#define wprintk(msg...) if (warning) { printk(KERN_WARNING  "W/CLK: " msg); }
#define iprintk(msg...) if (info)    { printk(KERN_INFO     "I/CLK: " msg); }

#define RTKSetRange(n,min,max) n = ( n > max )? max: (n < min)? min: n;

static void __init rtk_osc_clk_setup(struct device_node *node)
{
	struct clk *clk;
	struct clk_fixed_rate *fixed;
	//struct clk_gate *gate;
	const char *clk_name = node->name;
	unsigned int rate;

    dprintk("[%s %d]\n",__func__,__LINE__);

	/* allocate fixed-rate and gate clock structs */
	fixed = kzalloc(sizeof(struct clk_fixed_rate), GFP_KERNEL);
	if (!fixed)
		return;

	if (of_property_read_u32(node, "clock-frequency", &rate))
		return;

	fixed->fixed_rate = rate;

	clk = clk_register_composite(NULL, clk_name,    // *dev, *name,
			NULL, 0,                                // **parent_names, num_parents,
			NULL, NULL,                             // *mux_hw, *mux_ops,
			&fixed->hw, &clk_fixed_rate_ops,        // *rate_hw, *rate_ops,
			NULL, NULL,                             // *gate_hw, *gate_ops,
			CLK_IS_ROOT);                           // flags

	if (clk) {
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		clk_register_clkdev(clk, clk_name, NULL);
	} else {
        eprintk("[%s %d]ERROR! \n",__func__,__LINE__);
    }
}


#if 0
struct mux_data {
    u8 shift;
    u8 width;
};

static const __initconst struct mux_data cpu_data = {
    .shift = 16,
    .width = 2,
};

static void __init rtk_mux_clk_setup(struct device_node *node,
        struct mux_data *data)
{
    struct clk *clk;
    const char *clk_name = node->name;
    const char *parents[5];
    unsigned long flags = 0;
    void *reg;
    int i = 0;

    reg = of_iomap(node, 0);

    #if 0
    /* case 1 : 0x0 0x1 0x2 0x3 */
    flags = 0;
    /* case 2 : 0x1 0x2 0x4 0x8 */
    flags = CLK_MUX_INDEX_BIT;
    /* case 3 : 0x1 0x2 0x3 0x4 */
    flags = CLK_MUX_INDEX_ONE;
    #endif

    while (i < 5 && (parents[i] = of_clk_get_parent_name(node, i)) != NULL)
        i++;

    clk = clk_register_mux(NULL, clk_name, parents, i, flags, reg,
            data->shift, data->width,
            0, &clk_lock);

    if (clk) {
        of_clk_add_provider(node, of_clk_src_simple_get, clk);
        clk_register_clkdev(clk, clk_name, NULL);
    }
}
#endif

#define RTK_GATES_MAX_SIZE	32

struct gates_data {
    DECLARE_BITMAP(mask, RTK_GATES_MAX_SIZE);
};

static const __initconst struct gates_data ve_gates_data = {
    .mask = {0x20001000},
};

static const __initconst struct gates_data jpeg_gates_data = {
    .mask = {0x00002000},
};

static void __init rtk_gates_clk_setup(struct device_node *node,
        struct gates_data *data)
{
    struct clk_onecell_data *clk_data;
    const char *clk_parent;
    const char *clk_name;
    void *reg;
    int qty;
    int i = 0;
    int j = 0;
    int ignore = 0;

    reg = of_iomap(node, 0);

    clk_parent = of_clk_get_parent_name(node, 0);

    /* Worst-case size approximation and memory allocation */
    qty = find_last_bit(data->mask, RTK_GATES_MAX_SIZE);
    clk_data = kmalloc(sizeof(struct clk_onecell_data), GFP_KERNEL);
    if (!clk_data)
        return;
    clk_data->clks = kzalloc((qty+1) * sizeof(struct clk *), GFP_KERNEL);
    if (!clk_data->clks) {
        kfree(clk_data);
        return;
    }

    for_each_set_bit(i, data->mask, RTK_GATES_MAX_SIZE) {
        of_property_read_string_index(node, "clock-output-names",
                j, &clk_name);
        #if 0
        /* No driver claims this clock, but it should remain gated */
        ignore = !strcmp("ahb_sdram", clk_name) ? CLK_IGNORE_UNUSED : 0;
        #endif
        clk_data->clks[i] = clk_register_gate(NULL, clk_name,
                clk_parent, ignore,
                reg + 4 * (i/32), i % 32,
                0, &clk_lock);
        WARN_ON(IS_ERR(clk_data->clks[i]));
        j++;
    }

    /* Adjust to the real max */
    clk_data->clk_num = i;
    iprintk("[%s] clk_data->clk_num = %d \n", __func__, clk_data->clk_num);

    of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
}

#define MAX_TEMP_NUM 10
struct temp_data {
    u32 target;
    u32 parent_rate;
    struct clk_factors_data factors_data;
};

static struct temp_data temp_factors_data [MAX_TEMP_NUM];

static unsigned int temp_id = 0;

void push_factors_data(u32 target, u32 parent_rate, struct clk_factors_data *factors_data)
{
    temp_factors_data[temp_id].target = target;
    temp_factors_data[temp_id].parent_rate = parent_rate;
    temp_factors_data[temp_id].factors_data = *factors_data;
    temp_id ++;
    if (temp_id >= MAX_TEMP_NUM) temp_id = 0;
}

int find_factors_data(u32 target, u32 parent_rate, struct clk_factors_data *factors_data)
{
    int i;
    for (i = 0;i <MAX_TEMP_NUM;i++)
        if (temp_factors_data[temp_id].target == target
                && temp_factors_data[temp_id].parent_rate == parent_rate) {
            *factors_data = temp_factors_data[temp_id].factors_data;
            return 0;
        }
    return -1;
}

/* 
 * Calculates n, f, d for SCPU PLL
 * SCPU PLL rate is calculated as follows
 * rate = parent_rate * ( n + 1 + f/2048) / d;
 * parent_rate is always 27Mhz
 */

static void rtk_get_spll_factors(u32 *rate, u32 parent_rate, void *config, void *data, size_t dataSize)
{
    struct clk_factors_config   *clk_config = (struct clk_factors_config *) config;
    struct clk_factors_data     *output_data = (struct clk_factors_data *)data;
    struct clk_factors_data     limit_table = {.result = 0};
    u8 d_limit = 0;
    u32 target = *rate;
    RTKSetRange(target, clk_config->min_freq, clk_config->max_freq);
    dprintk("[%s] target = %d parent_rate = %d\n",__func__,target,parent_rate);

    if (parent_rate == 0) {
        eprintk("[%s] ERROR! parent_rate = 0; set default 27MHz",__func__);
        parent_rate = 27000000;
    }

    if (clk_config->nmax > (clk_config->max_freq/parent_rate))
        clk_config->nmax = clk_config->max_freq/parent_rate;

#if 0
    if (!find_factors_data(target,parent_rate, &limit_table))
        goto DONE;
#endif

    /* Assume the target is 1GHz, we will find the number which is the most closest to
     * and smaller than 1GHz. */
    //for (d_limit = clk_config->dmin; d_limit <= clk_config->dmax; d_limit ++)
    {
        u32 div,n_limit,f_limit,result;

        if (is_revinfo_0 || is_revinfo_1) {
            if (target <= (600 * 1000 * 1000))
                d_limit = 2; // /2
            else
                d_limit = 0;
        } else {
            if (target <= (350 * 1000 * 1000))
                d_limit = 3; // /4
            else if (target <= (700 * 1000 * 1000))
                d_limit = 2; // /2
        }

        if (d_limit < clk_config->dmin)
            d_limit = clk_config->dmin;
        else if (d_limit > clk_config->dmax)
            d_limit = clk_config->dmax;

        /*
         * d_limit : PLLSCPU DIVIDER
         *  00 : /1
         *  01 : /1
         *  10 : /2
         *  11 : /4
         *
         * +--------+    +------+     +---------+
         * | Parent |--->| SPLL |---->| DIVIDER |---> SCPU
         * +--------+    +------+     +---------+
         *
         */

        div = (d_limit == 3) ? 4 : (d_limit == 2) ? 2 : 1;
#if 1
        for (n_limit = clk_config->nmin; n_limit <= clk_config->nmax; n_limit ++) {
            for (f_limit = clk_config->fmin; f_limit <= clk_config->fmax; f_limit ++) {
                result = (parent_rate/div) * (n_limit + 0x1);
                result += ((parent_rate/2048)/div) * f_limit;
                if (limit_table.result < result && result <= target) {
                    limit_table.result  = (u32)  result;
                    limit_table.d       = (u8)   d_limit;
                    limit_table.n       = (u32)  n_limit;
                    limit_table.f       = (u32)  f_limit;
                }
                if (target == result) goto COMPUTE_DONE;
            }
        }
#else
        n_limit = (target * div)/parent_rate;
        RTKSetRange(n_limit, clk_config->nmin, clk_config->nmax);
        do {
            temp_n_rate = (parent_rate * n_limit)/div;

            f_limit = ((target - temp_n_rate) * div)/(parent_rate/2048);
            RTKSetRange(f_limit, clk_config->fmin, clk_config->fmax);
            temp_f_rate =  ((parent_rate/2048)*f_limit)/div;

        } while ((target < (temp_n_rate + temp_f_rate)) && ((--n_limit) > clk_config->nmin));

        result = ((parent_rate * n_limit) + ((parent_rate/2048)*f_limit))/div;

        if (limit_table.result < result && result <= target) {
            limit_table.result  = (u32)  result;
            limit_table.d       = (u8)   d_limit;
            limit_table.n       = (u32)  n_limit;
            limit_table.f       = (u32)  f_limit;
        }

        if (target == result) break;
#endif
    }
COMPUTE_DONE:

    dprintk("[%s] n:%d f:%d d:%d \n",__func__,limit_table.n,limit_table.f,limit_table.d);
    push_factors_data(target, parent_rate, &limit_table);
DONE:

    iprintk("[%s] output rate = %d (target = %d)\n",__func__,limit_table.result,target);

    *rate   = limit_table.result;
    *output_data   = limit_table;
}

/* 
 * Calculates n, f, d for PLL
 * SCPU PLL rate is calculated as follows
 * rate = parent_rate * ( n + 2 ) / ( f + 1 ) / ( d + 1 );
 * parent_rate is always 27Mhz
 */

static void rtk_get_pll_factors(u32 *rate, u32 parent_rate, void *config, void *data, size_t dataSize)
{
    struct clk_factors_config   *clk_config = (struct clk_factors_config *) config;
    struct clk_factors_data     *output_data = (struct clk_factors_data *)data;
    struct clk_factors_data     limit_table = {.result = 0};
    u8 d_limit;
    u32 target = *rate;
    RTKSetRange(target, clk_config->min_freq, clk_config->max_freq);
    dprintk("[%s] target = %d parent_rate = %d\n",__func__,target,parent_rate);

    if (parent_rate == 0) {
        eprintk("[%s] ERROR! parent_rate = 0; set default 27MHz",__func__);
        parent_rate = 27000000;
    }

    if (clk_config->nmax > (clk_config->max_freq/parent_rate))
        clk_config->nmax = clk_config->max_freq/parent_rate;

    for (d_limit = clk_config->dmin; d_limit <= clk_config->dmax; d_limit ++) {
        u32 div,n_limit,f_limit,result;

        /* PLLSCPU PRE-DIVIDER
         * 00 : /1
         * 01 : /2
         * 10 : /4
         * 11 : /8
         */
        //div = 0x1 << d_limit;
        div = d_limit;
        for (n_limit = clk_config->nmin; n_limit <= clk_config->nmax; n_limit ++) {
            for (f_limit = clk_config->fmin; f_limit <= clk_config->fmax; f_limit ++) {
                result = parent_rate * (n_limit + 0x2);
                result /= (f_limit + 1);
                result /= (d_limit + 1);
                if (limit_table.result < result && result <= target) {
                    limit_table.result  = (u32)  result;
                    limit_table.d       = (u8)   d_limit;
                    limit_table.n       = (u32)  n_limit;
                    limit_table.f       = (u32)  f_limit;
                }
                if (target == result) goto COMPUTE_DONE;
            }
        }
    }

    COMPUTE_DONE:

    dprintk("[%s] n:%d f:%d d:%d \n",__func__,limit_table.n,limit_table.f,limit_table.d);

    iprintk("[%s] output rate = %d (target = %d)\n",__func__,limit_table.result,target);

    *rate   = limit_table.result;
    *output_data   = limit_table;
}

/* Setup function for factor clocks
 */

struct factors_data {
	struct clk_factors_config *table;
    void (*getter) (u32 *rate, u32 parent_rate, void * config, void *data, size_t dataSize);
};

static struct clk_factors_config spll_config = {
    .id         = SPLL,
    .noffset    = 0x104,    // 0x18000104
    .nlen       = 7,
    .nshift     = 21,
    .foffset    = 0x104,    // 0x18000104
    .flen       = 10,
    .fshift     = 10,
    .doffset    = 0x30,     // 0x18000030
    .dlen       = 1,
    .dshift     = 7,
};

static struct clk_factors_config vpll_config = {
    .id         = VPLL,
    .noffset    = 0,          // 0x18000114
    .nlen       = 6,
    .nshift     = 11,
    .foffset    = 0,          // 0x18000114
    .flen       = 1,
    .fshift     = 18,
    .doffset    = 0,          // 0x18000114
    .dlen       = 1,
    .dshift     = 24,
};

static struct clk_factors_config apll_config = {
    .id         = APLL,
    .noffset    = 0,          // 0x1800010c
    .nlen       = 6,
    .nshift     = 12,
    .foffset    = 0,          // 0x1800010c
    .flen       = 1,
    .fshift     = 19,
    .doffset    = 0,          // 0x1800010c
    .dlen       = 1,
    .dshift     = 24,
};

static const __initconst struct factors_data spll_data = {
	.table = &spll_config,
	.getter = rtk_get_spll_factors,
};

static const __initconst struct factors_data vpll_data = {
	.table = &vpll_config,
	.getter = rtk_get_pll_factors,
};

static const __initconst struct factors_data apll_data = {
	.table = &apll_config,
	.getter = rtk_get_pll_factors,
};

static void __init rtk_factors_clk_setup(struct device_node *node,
					   struct factors_data *data)
{
	struct clk *clk;
	const char *clk_name = node->name;
	const char *parent;
	void *reg;
    struct clk_factors_config *config = data->table;

	reg = of_iomap(node, 0);

	parent = of_clk_get_parent_name(node, 0);

    if (of_property_read_u32(node, "pll-max-frequency", &(config->max_freq))
      ||of_property_read_u32(node, "pll-min-frequency", &(config->min_freq)))
        return;
    iprintk("[%s] pll-max-frequency = %d   pll-min-frequency = %d \n",__func__,config->max_freq,config->min_freq);

    #if 0
    if(of_property_read_u32(node, "n-offset",   &(config->noffset))) goto err;
    if(of_property_read_u32(node, "n-len",      &(config->nlen)))  goto err;
    if(of_property_read_u32(node, "n-shift",    &(config->nshift)))  goto err;
    if(of_property_read_u32(node, "f-offset",   &(config->noffset))) goto err;
    if(of_property_read_u32(node, "f-len",      &(config->nlen)))  goto err;
    if(of_property_read_u32(node, "f-shift",    &(config->nshift)))  goto err;
    if(of_property_read_u32(node, "d-offset",   &(config->noffset))) goto err;
    if(of_property_read_u32(node, "d-len",      &(config->nlen)))  goto err;
    if(of_property_read_u32(node, "d-shift",    &(config->nshift)))  goto err;
    #endif

    if(of_property_read_u32(node, "n-max", &(config->nmax))) config->nmax = (0x1U << (config->nlen + 1)) -1;
    if(of_property_read_u32(node, "n-min", &(config->nmin))) config->nmin = 0;
    if(of_property_read_u32(node, "f-max", &(config->fmax))) config->fmax = (0x1U << (config->flen + 1)) -1;
    if(of_property_read_u32(node, "f-min", &(config->fmin))) config->fmin = 0;
    if(of_property_read_u32(node, "d-max", &(config->dmax))) config->dmax = (0x1U << (config->dlen + 1)) -1;
    if(of_property_read_u32(node, "d-min", &(config->dmin))) config->dmin = 0;

    iprintk("[%s] id:%d n(%d:%d) f(%d:%d) d(%d:%d) \n",__func__,
            config->id,
            config->nmin,config->nmax,
            config->fmin,config->fmax,
            config->dmin,config->dmax);

	clk = clk_register_factors(NULL, clk_name, parent, CLK_GET_RATE_NOCACHE, reg,
				   data->table, data->getter, &clk_lock);

	if (clk) {
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
		clk_register_clkdev(clk, clk_name, NULL);
        return;
	}
err:
    eprintk("[%s %d] ERROR! clk(%d) is NULL\n",__func__,__LINE__,config->id);
}

/* Matches for of_clk_init */
static const __initconst struct of_device_id clk_match[] = {
	{.compatible = "realtek,1195-osc-clk", .data = rtk_osc_clk_setup,},
	{.compatible = "realtek,1195-sys-clk", .data = rtk_osc_clk_setup,},
	{}
};

/* Matches for factors clocks */
static const __initconst struct of_device_id clk_factors_match[] = {
	{.compatible = "realtek,1195-spll-clk", .data = &spll_data,},
	{.compatible = "realtek,1195-vpll-clk", .data = &vpll_data,},
	{.compatible = "realtek,1195-apll-clk", .data = &apll_data,},
	{}
};

#if 0
/* Matches for mux clocks */
static const __initconst struct of_device_id clk_mux_match[] = {
	{.compatible = "realtek,1195-cpu-clk", .data = &cpu_data,},
	{}
};
#endif

/* Matches for gate clocks */
static const __initconst struct of_device_id clk_gates_match[] = {
	{.compatible = "realtek,1195-ve-gates-clk", .data = &ve_gates_data,},
	{.compatible = "realtek,1195-jpeg-gates-clk", .data = &jpeg_gates_data,},
	{}
};

static void __init of_realtek_table_clock_setup(const struct of_device_id *clk_match,
					      void *function)
{
	struct device_node *np;
	const struct div_data *data;
	const struct of_device_id *match;
	void (*setup_function)(struct device_node *, const void *) = function;

	for_each_matching_node(np, clk_match) {
		match = of_match_node(clk_match, np);
		data = match->data;
		setup_function(np, data);
	}
}

void __init rtk_init_clocks(void)
{
	/* Register all the simple realtek clocks */
	of_clk_init(clk_match);

	/* Register factor clocks */
	of_realtek_table_clock_setup(clk_factors_match, rtk_factors_clk_setup);

    #if 0
	/* Register mux clocks */
	of_realtek_table_clock_setup(clk_mux_match, rtk_mux_clk_setup);
    #endif

	/* Register gate clocks */
	of_realtek_table_clock_setup(clk_gates_match, rtk_gates_clk_setup);
}
