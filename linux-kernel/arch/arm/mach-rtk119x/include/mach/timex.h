/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2012 by Chuck Chen <ycchen@realtek.com>
 *
 */

#ifndef _ASM_ARM_TIMERX_H_
#define _ASM_ARM_TIMERX_H_

#include <mach/system.h>

#if 0
struct timer_reg
{
    unsigned int tvr;
    unsigned int cvr;
    unsigned int cr;
    unsigned int icr;
    unsigned char isr_bit;
};

#define TIMERINFO_BASE_CLOCK     TIMER_CLOCK
#define TIMERINFO_BASE_UNIT      37      // 37ns
#define TIMERINFO_SEC            1000000000/TIMERINFO_BASE_UNIT
#define TIMERINFO_MSEC           1000000/TIMERINFO_BASE_UNIT
#define TIMERINFO_USEC           1000/TIMERINFO_BASE_UNIT

#define CLOCK_TICK_RATE	TIMERINFO_BASE_CLOCK	// CLOCK_TICK_RATE is obsoleted. Give it a fake value and go

enum hwtimer_commands {
	HWT_START = 0x80,	// Start a timer/counter
	HWT_STOP,			// Stop a timer/counter
	HWT_PAUSE,			// Pause a timer/counter
	HWT_RESUME,			// Restart a timer/counter from last value
	HWT_INT_ENABLE,		// Enable timer/counter interrupt
	HWT_INT_DISABLE,	// Disable timer/counter interrupt
	HWT_INT_CLEAR,		// Clear timer/counter interrupt pending bit
};

enum hwtimer_mode {
	COUNTER = 0,
	TIMER,
};

#define TIMERINFO_TIMER_ENABLE              0x80000000
#define TIMERINFO_TIMER_MODE     	        0x40000000
#define TIMERINFO_TIMER_PAUSE               0x20000000
#define TIMERINFO_INTERRUPT_ENABLE          0x80000000

int create_timer(unsigned char id, unsigned int interval, unsigned char mode);
int timer_set_mode(unsigned char id, unsigned char mode);
int timer_get_value(unsigned char id);
int timer_set_value(unsigned char id, unsigned int value);
int timer_set_target(unsigned char id, unsigned int value);
int timer_control(unsigned char id, unsigned int cmd);

void rtk_timer_init(void);
#endif

#endif	//_ASM_MACH_TIMERX_H_
