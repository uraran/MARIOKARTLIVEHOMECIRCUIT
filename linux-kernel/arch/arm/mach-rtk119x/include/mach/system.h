/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2012 by Chuck Chen <ycchen@realtek.com>
 *
 */

#ifndef _ASM_MACH_SYSTME_H_
#define _ASM_MACH_SYSTEM_H_

#include <asm/io.h>
#include <rbus/reg.h>
#include "cpu.h"

#if 0
#include <rbus/crt_reg.h>
#include <mach/iomap.h>
#include "serial.h"

#define GET_IC_VERSION()	rtd_inl(SC_VERID_reg)
#define VERSION_A			0x62270000
#define VERSION_B			0x62270001

//Magellan support 8 timers, timer0 for VCPU, timer1 for ACPU, timer2 for SCPU
#define SYSTEM_TIMER 		2
//#define TIMER_CLOCK			50000000	//FPGA
#define TIMER_CLOCK		27000000
#define MAX_HWTIMER_NUM		8

// linux will do the remapping of rbus so that you have to read/write rbus via rtd_xxx series
// or just tells a correct address to the system
#define GET_MAPPED_RBUS_ADDR(x)		(x - 0xb8000000 + RBUS_BASE_VIRT)
#define SYSTEM_SCU_BASE				GET_MAPPED_RBUS_ADDR(0xb801e000)
#define SYSTEM_GIC_DIST_BASE		GET_MAPPED_RBUS_ADDR(0xb801f000)
#define SYSTEM_GIC_CPU_BASE			GET_MAPPED_RBUS_ADDR(0xb801e100)
#define SYSTEM_PL310_BASE			GET_MAPPED_RBUS_ADDR(0xb801d000)

#define GET_RETURN_ADDR()               \
({ int __res;                               \
    __asm__ __volatile__(                   \
        "mov\t%0, lr\n\t"         \
        : "=r" (__res));                \
    __res;                              \
})
#else
#define GET_MAPPED_RBUS_ADDR(x)		(x - 0x18000000 + RBUS_BASE_VIRT)
#endif

#endif
