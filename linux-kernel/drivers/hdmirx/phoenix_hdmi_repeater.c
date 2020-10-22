#include "phoenix_hdmiPlatform.h"
#include "phoenix_hdmi_wrapper.h"
#include "phoenix_mipi_wrapper.h"
#include "phoenix_hdmiHWReg.h"
#include "phoenix_hdmiFun.h"

void hdmi_repeater(HDMI_CONST unsigned char* value ,unsigned char num)
{
    int i;
    unsigned int tmp_value, tmp_ap_value;
    
    // close hdmi wrapper isr
    set_hdmirx_wrapper_interrupt_en(0,0,0);
    hdmi_ioctl_struct.measure_ready = 0;    
    // close mipi/hdmi wrapper
    tmp_value = hdmi_rx_reg_read32(MIPI_WRAPPER_MIPI_reg, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_reg, (tmp_value & 0xFFFFFFFE), HDMI_RX_MIPI);
    set_hdmirx_wrapper_control_0(-1, 0,-1,-1,-1, 0);
    
    // write BKsv
    hdmi_rx_reg_write32(HDMI_HDCP_AP_VADDR, 0x43, HDMI_RX_MAC);
    while(num--)
        hdmi_rx_reg_write32(HDMI_HDCP_DP_VADDR, *(value++), HDMI_RX_MAC);

    // set new status : repeater
    tmp_value = hdmi_rx_reg_read32(HDMI_HDCP_CR_VADDR, HDMI_RX_MAC);
    tmp_value |= 0x40; // set bit 6 to 1
    hdmi_rx_reg_write32(HDMI_HDCP_CR_VADDR, tmp_value, HDMI_RX_MAC);

    // hot plug
    for (i=0; i<HDMI_MAX_CHANNEL; i++)
    {
        drvif_Hdmi_HPD(i , 0);
        HDMI_DELAYMS(200);
        drvif_Hdmi_HPD(i , 1);
    }
    
    // set HDCP_AP : AP1 -> 0x40
    tmp_value = hdmi_rx_reg_read32(HDMI_HDCP_AP_VADDR, HDMI_RX_MAC);
    tmp_value = (tmp_value & 0xFFFFFF00) | 0x40; 
    hdmi_rx_reg_write32(HDMI_HDCP_AP_VADDR, tmp_value, HDMI_RX_MAC);
    // read HDCP_DP:DP1, and set HDCP_DP:bit 6 -> 1
    tmp_value = hdmi_rx_reg_read32(HDMI_HDCP_DP_VADDR, HDMI_RX_MAC);
    tmp_value |= 0x40; // set bit 6 to 1
    // set HDCP_AP : AP1 -> 0x40 again
    tmp_ap_value = hdmi_rx_reg_read32(HDMI_HDCP_AP_VADDR, HDMI_RX_MAC);
    tmp_ap_value = (tmp_ap_value & 0xFFFFFF00) | 0x40; 
    hdmi_rx_reg_write32(HDMI_HDCP_AP_VADDR, tmp_ap_value, HDMI_RX_MAC);
    // set HDCP_DP:DP1
    hdmi_rx_reg_write32(HDMI_HDCP_DP_VADDR, tmp_value, HDMI_RX_MAC);
    
    // enable hdmi wrapper isr
    set_hdmirx_wrapper_interrupt_en(0,0,1);
    // open mipi/ hdmi wrapper
    tmp_value = hdmi_rx_reg_read32(MIPI_WRAPPER_MIPI_reg, HDMI_RX_MIPI);
    hdmi_rx_reg_write32(MIPI_WRAPPER_MIPI_reg, (tmp_value | 0x1), HDMI_RX_MIPI);
    set_hdmirx_wrapper_control_0(-1, -1,-1,-1,-1, 1);
}

   
