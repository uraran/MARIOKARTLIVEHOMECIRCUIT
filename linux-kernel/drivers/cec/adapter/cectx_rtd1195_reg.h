//Copyright (C) 2007-2013 Realtek Semiconductor Corporation.
#ifndef __CECTX_RTD1195_REG_H__
#define __CECTX_RTD1195_REG_H__

#include "cec_rtd1195.h"

#define ID_RTK_CEC_CONTROLLER     0x1778

#define ISO_IRQ        		57

#define RTK_CEC_ANALOG         			    (0xE0)
#define CEC_ANALOG							RTK_CEC_ANALOG
#define CEC_ANALOG_REG_CEC_RPU_EN_MASK		(0x1<<3)



#define SOFT_RESET1_reg		                0x18000000
#define ISO_ISR_reg							0x18007000

					
#define SYS_PLL_BUS1						0x164
#define SYS_PLL_BUS1_pllbus_n_shift         (25)
#define SYS_PLL_BUS1_pllbus_m_shift         (18)
#define SYS_PLL_BUS1_pllbus_n_mask          (0x06000000)
#define SYS_PLL_BUS1_pllbus_m_mask          (0x01FC0000)

#define SYS_PLL_BUS3                        0x16c
#define SYS_PLL_BUS3_pllbus_o_shift         (3)
#define SYS_PLL_BUS3_pllbus_o_mask          (0x00000008)

#define CEC_INT_EN                      RTK_CEC_INT_EN
#define CEC_INT_EN_MASK                 (0x1<<22)
#define CEC_INT_EN_VAL                  (0x1<<22)

#define SCPU_INT_EN                      (0)
#define ACPU_INT_EN                      (8)

#define SOFT_RESET_rstn_cec0_mask        		(0x00000004)
#define CLOCK_ENABLE_clk_en_misc_cec0_mask      (0x00000004)

// RTCR0
#define CEC_EOM_ON						(0x01<<12)

// RXCR0

// TXCR0

// TXDR0
                                
// RXTCR0                       

// TXCR0/1
                                
#endif // __CECTX_RTD1195_REG_H__
