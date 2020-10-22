//Copyright (C) 2007-2013 Realtek Semiconductor Corporation.
#ifndef __CECRX_RTD1195_REG_H__
#define __CECRX_RTD1195_REG_H__

#include "../cec_rtd1195.h"

#define ID_RTK_CEC_1_CONTROLLER     0x1778

#define CBUS_WRAPPER    (0)

#define CEC1_INT_SCPU_EN                 (0x1<<17)
#define CEC1_INT_ACPU_EN                  (0x1<<16)

// RTCR0
#define CEC_PAD_EN                      (0x01<<17)
#define CEC_PAD_EN_MODE                      (0x01<<16)
#define CEC_HW_RETRY_EN                      (0x01<<12)

// RXCR0

// TXCR0

// TXDR0
                                
// RXTCR0                       

// TXCR0/1

//GDI_POWER_SAVING_MODE
#define CEC_RSEL(x)                      (x & 0x7)
#define CEC_RPU_EN                      (1<<4)
#define IRQ_BY_CEC_RECEIVE_SPECIAL_CMD(x)  ((x&0x1)<<8)
                                
void rtk_cec_1_reset_tx(void);
void rtk_cec_1_reset_rx(void);
int rtk_cec_1_set_mode(rtk_cec* p_this, unsigned char mode);
int rtk_cec_1_set_physical_addr(rtk_cec* p_this, unsigned short phy_addr);
int rtk_cec_1_set_rx_mask(rtk_cec* p_this, unsigned short rx_mask);    

#endif // __CECRX_RTD1195_REG_H__
