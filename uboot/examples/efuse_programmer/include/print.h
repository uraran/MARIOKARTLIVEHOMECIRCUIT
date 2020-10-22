#ifndef __PRINT_H__
#define __PRINT_H__

#include "sysdefs.h"
#include "dvrboot_inc/io.h"
extern int printf(const char *fmt, ...);

#define UBOOT_DDR_OFFSET		0xa0000000	//for RTD299X
//#define UBOOT_DDR_OFFSET		0x20000000	//for pandaboard

#define MAX_MALLOC_SIZE		16*1024*1024
#define MALLOC_BASE		(0xa4000000 - UBOOT_DDR_OFFSET)	//modify by angus, orininal value is 0xa6000000
#define	UARTINFO_TEMT_MASK					0x40
#define	UARTINFO_THRE_MASK					0x20
#define UARTINFO_TRANSMITTER_READY_MASK		(UARTINFO_TEMT_MASK | UARTINFO_THRE_MASK)

/* UART0 */
#if 1
#define  UART0_REG_BASE		0xb8062300  //see "RTD299X_ISO_MIS_OFF_arch_spec.doc"
#else
#define  UART0_REG_BASE		0xB801B100  //see "RTD299X_ISO_MIS_OFF_arch_spec.doc"
#endif
#define U0RBR_THR_DLL		(UART0_REG_BASE + 0x0)
#define U0IER_DLH		(UART0_REG_BASE + 0x4)
#define U0IIR_FCR		(UART0_REG_BASE + 0x8)
#define U0LCR			(UART0_REG_BASE + 0xc)
#define U0MCR			(UART0_REG_BASE + 0x10)
#define U0LSR			(UART0_REG_BASE + 0x14)
#define U0MSR			(UART0_REG_BASE + 0x18)
#define U0SCR			(UART0_REG_BASE + 0x1c)

#define U0FAR			(UART0_REG_BASE + 0x70)
#define U0TFR			(UART0_REG_BASE + 0x74)
#define U0RFW			(UART0_REG_BASE + 0x78)
#define U0USR			(UART0_REG_BASE + 0x7c)
#define U0TFL			(UART0_REG_BASE + 0x80)
#define U0RFL			(UART0_REG_BASE + 0x84)
#define U0SRR			(UART0_REG_BASE + 0x88)
#define U0SBCR			(UART0_REG_BASE + 0x8c)
#define U0BCR			(UART0_REG_BASE + 0x90)
#define U0SDMAM			(UART0_REG_BASE + 0x94)
#define U0SFE			(UART0_REG_BASE + 0x98)
#define U0SRT			(UART0_REG_BASE + 0x9c)
#define U0STET			(UART0_REG_BASE + 0xa0)
#define U0HTX			(UART0_REG_BASE + 0xa4)
#define U0DMASA			(UART0_REG_BASE + 0xa8)
#define U0CPR			(UART0_REG_BASE + 0xf4)
#define U0UCV			(UART0_REG_BASE + 0xf8)
#define U0CTR			(UART0_REG_BASE + 0xfc)
//end by angus

#define CODE_VAILD_MAGIC_NUMBER 0xF9E8D7C6

typedef int (*PrintFuncPtr_t)(const char *fmt, ...);
typedef void (*Flush_Dcache_AllPtr_t)(void);

extern PrintFuncPtr_t rtprintf;
extern Flush_Dcache_AllPtr_t rtflush_dcache_all;

//#define printf(fmt, args...)

#define ENABLE_UART_FUNC
//#define ENABLE_PRINTF

#ifdef ENABLE_PRINTF
#define print_uart printf
#endif
/************************************************************************
 *  Public function
 ************************************************************************/
void sync(void);
void init_uart(void);

#ifdef ENABLE_UART_FUNC
void print_uart(char *message);
#endif
void print_val(UINT32 val, UINT32 len);

void sys_dcache_flush_all(void);
void put_char(char ch);
unsigned int hexstr2int(char *s);
int IsHexStr(char *s);
void hexdump(const char *str, const void *buf, unsigned int length);
extern void * memset(void * s,int c,size_t count);
#endif
