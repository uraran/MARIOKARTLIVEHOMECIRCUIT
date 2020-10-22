#ifndef _REG_MISC_H_INCLUDED_
#define _REG_MISC_H_INCLUDED_

typedef struct _rbus_misc_reg
{
	u32		MISC_SYS_reserved_1[64];				/* 0x1801B000 */

	u32		MISC_GPIO[64];							/* 0x1801B100 */
	u32		MISC_UART1[64];							/* 0x1801B200 */
	u32		MISC_reserved_1[128];					/* 0x1801B300 */
	u32		MISC_TC_reserved_1[14];					/* 0x1801B500 */
	u32		MISC_CLK90K_CTRL;						/* 0x1801B538 */
	u32		MISC_SCPU_CLK27M_90K_div;				/* 0x1801B53C */
	u32		MISC_SCPU_CLK90K_LO;					/* 0x1801B540 */
	u32		MISC_SCPU_CLK90K_HI;					/* 0x1801B544 */
	u32		MISC_ACPU_CLK27M_90K_div;				/* 0x1801B548 */
	u32		MISC_ACPU_CLK90K_LO;					/* 0x1801B54C */
	u32		MISC_ACPU_CLK90K_HI;					/* 0x1801B550 */
	u32		MISC_TC_reserved_2[7];
	u32		MISC_PCR_CLK90K_CTRL;					/* 0x1801B570 */
	u32		MISC_PCR_SCPU_CLK27M_90K_div;			/* 0x1801B574 */
	u32		MISC_PCR_SCPU_CLK90K_LO;				/* 0x1801B578 */
	u32		MISC_PCR_SCPU_CLK90K_HI;				/* 0x1801B57C */
	u32		MISC_PCR_ACPU_CLK27M_90K_div;			/* 0x1801B580 */
	u32		MISC_PCR_ACPU_CLK90K_LO;				/* 0x1801B584 */
	u32		MISC_PCR_ACPU_CLK90K_HI;				/* 0x1801B588 */
	u32		MISC_TC_reserved_3[9];
	u32		MISC_TCWCR;								/* 0x1801B5B0 */
	u32		MISC_TCWTR;								/* 0x1801B5B4 */
	u32		MISC_TCWNMI;							/* 0x1801B5B8 */
	u32		MISC_TCWOV;								/* 0x1801B5BC */
	u32		MISC_TCWCR_SWC;							/* 0x1801B5C0 */
	u32		MISC_TCWTR_SWC;							/* 0x1801B5C4 */
	u32		MISC_TCWNMI_SWC;						/* 0x1801B5C8 */
	u32		MISC_TCWOV_SWC;							/* 0x1801B5CC */
	u32		MISC_TC_reserved_4[12];
	u32		MISC_reserved_2[64];					/* 0x1801B600 */
} rbus_misc_reg;

extern rbus_misc_reg *reg_misc;

#define MISC_REG_UART1_PA				0x1801B200

#define MISC_OFFSET						0x0001B000
#define UMSK_ISR_OFFSET					0x00000008
#define ISR_OFFSET						0x0000000C
#define UMSK_ISR_SWC					0x00000010
#define ISR_SWC							0x00000014
#define SETTING_SWC						0x00000018
#define FAST_INT_EN_0					0x0000001C
#define FAST_INT_EN_1					0x00000020
#define FAST_ISR						0x00000024
#define MISC_DBG						0x0000002C
#define MISC_DUMMY						0x00000030

#define TCTVR_OFFSET					0x00000500
#define TCCVR_OFFSET					0x0000050C
#define TCCR_OFFSET						0x00000518
#define TCICR_OFFSET					0x00000524

#define RTC_OFFSET						0x00000600

#define I2C_1_OFFSET					0x00000300
#define I2C_2_OFFSET					0x00000700
#define I2C_3_OFFSET					0x00000900
#define I2C_4_OFFSET					0x00000A00
#define I2C_5_OFFSET					0x00000B00

#define LSADC_OFFSET					0x00000C00

#define GSPI_OFFSET						0x00000D00

#define RBUS_MISC_BASE_PHYS		(RBUS_BASE_PHYS + MISC_OFFSET)
#define RBUS_MISC_PHYS(pa)		(RBUS_BASE_PHYS + MISC_OFFSET + pa)

#define _MISC_IO_ADDR(pa)		(IOMEM(RBUS_BASE_VIRT) + MISC_OFFSET + pa)

#endif
