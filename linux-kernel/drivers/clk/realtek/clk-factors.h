#ifndef __MACH_REALTEK_CLK_FACTORS_H
#define __MACH_REALTEK_CLK_FACTORS_H

#include <linux/clk-provider.h>
#include <linux/clkdev.h>

/*  Fscpu = Fosc*(NCODE_T_SCPU+ FCODE_T_SCPU/2048) / PLLSCPU_N
 *  n : NCODE_T_SCPU
 *  f : FCODE_T_SCPU
 *  d : PLLSCPU_N
 */

typedef enum {
    SPLL = 1,
    VPLL,
    APLL,
} PLLID;

struct clk_factors_config {
    PLLID id;

    u32 nshift;
    u32 nlen;
    u32 noffset;
    u32 nmax;
    u32 nmin;

    u32 fshift;
    u32 flen;
    u32 foffset;
    u32 fmax;
    u32 fmin;

    u32 dshift;
    u32 dlen;
    u32 doffset;
    u32 dmax;
    u32 dmin;

    u32 max_freq;
    u32 min_freq;

    u32 reserve2;
    u32 reserve3;
};

struct clk_factors_data {
    u32 result;
    u32 n;
    u32 f;
    u32  d;
};

struct clk *clk_register_factors(struct device *dev, const char *name,
				 const char *parent_name,
				 unsigned long flags, void __iomem *reg,
				 struct clk_factors_config *config,
                 void (*getter) (u32 *rate, u32 parent_rate, void * config, void *data, size_t dataSize),
				 spinlock_t *lock);
#endif
