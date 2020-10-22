//#include <stdio.h>
//#include <string.h>
//#include <common.h>
//#include <exports.h>
//#include <linux/types.h>
#include "sysdefs.h"
#include "dvrboot_inc/util.h"
#include "uart.h"
#include "uart_reg.h"
#include "iso_reg.h"
#include "mis_reg.h"

/**
 * memset - Fill a region of memory with the given value
 * @s: Pointer to the start of the area.
 * @c: The byte to fill the area with
 * @count: The size of the area.
 *
 * Do not use memset() to access IO space, use memset_io() instead.
 */
void * memset(void * s,int c,size_t count)
{
	set_memory(s, c, count);
	return s;
}

void set_memory(void *dst, UINT8 value, UINT32 size)
{
	UINT32 i;	
	for (i=0; i<size; i++)
		REG8(((UINT32)dst) + i) = value;
}

void copy_memory(void *dst, void *src, UINT32 size)
{
	UINT32 i;	
	for (i=0; i<size; i++)
		rtd_outb((((UINT32)dst) + i) , rtd_inb(((UINT32)src) + i));
}


int compare_memory(void *s1, void *s2, UINT32 size)
{
	UINT32 i;
	char *p1, *p2;
	
	p1 = (char *)s1;
	p2 = (char *)s2;
	for (i = 0; i < size; i++)
	{
		if (p1[i] != p2[i])
			return (int)p1[i] - (int)p2[i];
	}
	
	return 0;
}

int compare_str(const char *s1, const char *s2)
{
	UINT32 i = 0;

	while (s1[i] != '\0')
	{
		if (s1[i] != s2[i])
			return (int)s1[i] - (int)s2[i];
		i++;
	}
	
	return 0;
}

void watchdog_reset(void)
{	
	REG32(MIS_TCWCR) = 0xa5;	// disable watchdog
	REG32(MIS_TCWTR) = 0x1;		// clear watchdog counter

	// set overflow count
	REG32(MIS_TCWOV) = 0x0;

	REG32(MIS_TCWCR) = 0;		// enable watchdog
}

/************************************************************************
 *

Uboot is a stand-alone application; raise() is an OS call on Linux that
raises a signal (in this case SIGFPE, with the subcode of
division-by-zero).

What you need here is a suitable definition of __div0 for your division
function (__div0 is called if the user attempts to divide by zero).  In
most bare-metal apps like uboot, you simply need a definition of __div0
that returns 0 rather than throwing an exception.

The implementation of __div0 that comes with the arm-none-eabi build of
the tools should do that.  Perhaps you should use that version of the
compiler for building uboot, not the arm-none-linux-gnueabi version.
*
*
************************************************************************/
void __div0 (void)
{
}

