#ifndef __UTIL_H__
#define __UTIL_H__

#include "sysdefs.h"
#include "dvrboot_inc/io.h"
extern int printf(const char *fmt, ...);

#define UBOOT_DDR_OFFSET		0xa0000000	//for RTD299X
//#define UBOOT_DDR_OFFSET		0x20000000	//for pandaboard

#define CP15ISB asm volatile ("mcr     p15, 0, %0, c7, c5, 4" : : "r" (0))
#define CP15DSB asm volatile ("mcr     p15, 0, %0, c7, c10, 4" : : "r" (0))
#define CP15DMB asm volatile ("mcr     p15, 0, %0, c7, c10, 5" : : "r" (0))
#define isb() __asm__ __volatile__ ("" : : : "memory")^M
#define nop() __asm__ __volatile__("mov\tr0,r0\t@ nop\n\t");^M

#define REG8( addr )		  (*(volatile UINT8 *) (addr))
#define REG16( addr )		  (*(volatile UINT16 *)(addr))
#define REG32( addr )		  (*(volatile UINT32 *)(addr))
#define REG64( addr )		  (*(volatile UINT64 *)(addr))

#define CPU_TO_BE32( value ) ( (                ((UINT32)value)  << 24) |   \
                               ((0x0000FF00UL & ((UINT32)value)) <<  8) |   \
                               ((0x00FF0000UL & ((UINT32)value)) >>  8) |   \
                               (                ((UINT32)value)  >> 24)   )
#define BE32_TO_CPU( value ) CPU_TO_BE32( value )

/*
typedef unsigned char		UINT8;
typedef signed char			INT8;
typedef unsigned short		UINT16;
typedef signed short		INT16;
typedef unsigned int		UINT32;
typedef signed int			INT32;
typedef unsigned long long	UINT64;
typedef signed long long	INT64;
typedef UINT8				bool;
*/

#define KUSEG_MSK		  0x80000000
#define KSEG_MSK		  0xE0000000
#define KUSEGBASE		  0x00000000
#define KSEG0BASE		  0x80000000
#define KSEG1BASE		  0xA0000000
#define KSSEGBASE		  0xC0000000
#define KSEG3BASE		  0xE0000000

#define KSEG0(addr)     (((UINT32)(addr) & ~KSEG_MSK)  | KSEG0BASE)
#define KSEG1(addr)     (((UINT32)(addr) & ~KSEG_MSK)  | KSEG1BASE)
#define KSSEG(addr)	(((UINT32)(addr) & ~KSEG_MSK)  | KSSEGBASE)
#define KSEG3(addr)	(((UINT32)(addr) & ~KSEG_MSK)  | KSEG3BASE)
#define KUSEG(addr)	(((UINT32)(addr) & ~KUSEG_MSK) | KUSEGBASE)
#define PHYS(addr) 	((UINT32)(addr)  & ~KSEG_MSK)

#define MAX_MALLOC_SIZE		16*1024*1024
#define MALLOC_BASE		(0xa4000000 - UBOOT_DDR_OFFSET)	//modify by angus, orininal value is 0xa6000000
#define	UARTINFO_TEMT_MASK					0x40
#define	UARTINFO_THRE_MASK					0x20
#define UARTINFO_TRANSMITTER_READY_MASK		(UARTINFO_TEMT_MASK | UARTINFO_THRE_MASK)

/************************************************************************
 *  Public function
 ************************************************************************/
void set_memory(void *dst, UINT8 value, UINT32 size);
void copy_memory(void *dest, void *src, UINT32 size);
void spi_copy_memory(void *dest, void *src, UINT32 size);
int compare_memory(void *s1, void *s2, UINT32 size);
int compare_str(const char *s1, const char *s2);
void sys_dcache_flush_all(void);
void hexdump(const char *str, const void *buf, unsigned int length);
extern void * memset(void * s,int c,size_t count);

#define __align__ __attribute__((aligned (8)))
#endif
