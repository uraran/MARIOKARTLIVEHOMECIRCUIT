#ifndef PHOENIX_HDMI_HW_REG_H
#define PHOENIX_HDMI_HW_REG_H

#define HW_IS_MAGELLAN

#ifdef HW_IS_MAGELLAN
//#include "rbusHDMIReg.h"
//#include "rbusMiscDDCReg.h"
//#include "rbusMHLCbus.h"
#include "hdmi_rx_mac_reg.h"
#include "hdmi_rx_phy_reg.h"
#include "hdmi_rx_clock_detect_reg.h"
#include "hdmi_rx_ddc_reg.h"

typedef enum {
    HDMI_RX_MAC = 0,
    HDMI_RX_PHY = 1,
    HDMI_RX_CEC = 2,
    HDMI_RX_CLOCK_DETECT = 3,
    HDMI_RX_MHL = 4,
    HDMI_RX_MHL_CBUS_STANDBY = 5, 
    HDMI_RX_MHL_MSC_REG = 6,
    HDMI_RX_MHL_CBUS_LINK = 7, 
    HDMI_RX_MHL_CBUS_MSC = 8, 
    HDMI_RX_MHL_CBUS_MSC_FW = 9, 
    HDMI_RX_MHL_CBUS_DDC = 10, 
    HDMI_RX_MHL_GLOBAL = 11, 
    HDMI_RX_MHL_CBUS_CLK_CTL = 12, 
    HDMI_RX_DDC = 13,
    HDMI_RX_CBUS_DDC = 14,
    HDMI_RX_MIPI = 15,
    HDMI_RX_MIPI_PHY = 16,    
    HDMI_RX_HDMI_WRAPPER = 17,
    HDMI_RX_RBUS_DFE = 18
}HDMI_RX_REG_BASE_TYPE;

//int*    phoenix_pli_getIOAddress(int addr);
//unsigned int phoenix_IoReg_Read32 (unsigned int addr);
//void phoenix_IoReg_Write32 (unsigned int addr, unsigned int value);
//void phoenix_IoReg_Mask32(unsigned int addr, unsigned int andMask, unsigned int orMask);
unsigned int hdmi_rx_reg_read32 (unsigned int addr_offset, HDMI_RX_REG_BASE_TYPE type);
void hdmi_rx_reg_write32 (unsigned int addr_offset, unsigned int value, HDMI_RX_REG_BASE_TYPE type);
void hdmi_rx_reg_mask32(unsigned int addr_offset, unsigned int andMask, unsigned int orMask, HDMI_RX_REG_BASE_TYPE type);
void Hdmi_PhyInit(void);
int DPHDMI_CRCCheck(void);
#else
#endif

#endif //PHOENIX_HDMI_HW_REG_H
