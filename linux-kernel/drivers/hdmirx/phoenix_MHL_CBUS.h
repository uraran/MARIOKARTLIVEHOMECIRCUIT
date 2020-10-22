#ifndef _PHOENIX_MHL_CBUS_H
#define _PHOENIX_MHL_CBUS_H

#include "phoenix_hdmiFun.h"
#include "hdmi_rx_mhl_reg.h"
#include "hdmi_rx_mhl_cbus_sb_reg.h"
#include "hdmi_rx_mhl_msc_reg.h"
#include "hdmi_rx_mhl_cbus_link_reg.h"
#include "hdmi_rx_mhl_cbus_msc_reg.h"
#include "hdmi_rx_mhl_msc_fw_req.h"
#include "hdmi_rx_cbus_ddc_reg.h"
#include "phoenix_hdmiHWReg.h"

#define MHL_HEADER_DDC_CMD          0
#define MHL_HEADER_VENDOR_CMD    1    
#define MHL_HEADER_MSC_CMD          2
//********************************************************************
//Request Send formate 
//********************************************************************
#define MHL_TX_CASE_DATA        				 0     					 //DATA only
#define MHL_TX_CASE_COMMAND        			 1      			//CMD1 only
#define MHL_TX_CASE_COM_DAT        			  2      				//CMD1 + DATA
#define MHL_TX_CASE_COM_OFF_DAT        		  3      		// CMD1 + OFFSET + DATA
#define MHL_TX_CASE_COM_OFF_DAT_COM        4      	//CMD1 +OFFSET +DATA + ... +CMD2
#define MHL_TX_CASE_COM_OFF                        5      				//CMD1 +OFFSET
//*********************************************************************
//Request wait formate 
//*********************************************************************
#define MHL_TX_WAIT_CASE_CMD_NO_WAIT         	0
#define MHL_TX_WAIT_CASE_CMD_ONLY         		1
#define MHL_TX_WAIT_CASE_DATA_ONLY        		2
#define MHL_TX_WAIT_CASE_CMD_DATA        		3
//*********************************************************************
//MSC Translation Layer Control Packets
//*********************************************************************
#define MHL_MSC_CMD_CODE_ACK          			0x33
#define MHL_MSC_CMD_CODE_NACK        			0x34
#define MHL_MSC_CMD_CODE_ABORT        			0x35
#define MHL_MSC_CMD_CODE_WRT_STAT        		0x60
#define MHL_MSC_CMD_CODE_SET_INT        		0x60
#define MHL_MSC_CMD_CODE_READ_DEVCAP        	0x61
#define MHL_MSC_CMD_CODE_GET_STATE        		0x62
#define MHL_MSC_CMD_CODE_GET_VENDOR_ID          0x63
#define MHL_MSC_CMD_CODE_SET_HPD        		0x64
#define MHL_MSC_CMD_CODE_CLR_HPD        		0x65
#define MHL_MSC_CMD_CODE_MSC_MSG        		0x68
#define MHL_MSC_CMD_CODE_GET_SC1_ERR        	0x69
#define MHL_MSC_CMD_CODE_GET_DDC_ERR        	0x6A
#define MHL_MSC_CMD_CODE_GET_MSC_ERR        	0x6B
#define MHL_MSC_CMD_CODE_WRT_BURST        		0x6C
#define MHL_MSC_CMD_CODE_GET_SC3_ERR        	0x6D
#define MHL_MSC_CMD_CODE_EOF          			0x32
#define MHL_MSC_CMD_CODE_NULL        			0x00
#define _NULL_POINTER                            (BYTE *)0xFFFFFFFF
//**********************************************************************
//MSC Message Sub-command
//**********************************************************************
#define MHL_MSG_MSGE                                          0x02
#define MHL_MSG_RCP                                             0x10
#define MHL_MSG_RCPK                                           0x11
#define MHL_MSG_RCPE                                           0x12
#define MHL_MSG_RAP                                             0x20
#define MHL_MSG_RAPK                                           0x21
#define MHL_MSG_UCP                                             0x30
#define MHL_MSG_UCPK                                           0x31
#define MHL_MSG_UCPE                                           0x32

//****************************************************************************
// Definitions of MHL Packets
//****************************************************************************
#define _MHL_SUCCESS                             0
#define _MHL_FAIL                                    1
#define _MHL_ABORT_FAIL                         2
#define _MHL_ABORT_REPLY                        3
//****************************************************************************
// Definitions of MHL High level API
//****************************************************************************

#define  _MHL_OK                             0
#define  _MHL_NG                             1

//****************************************************************************
// Definitions of MSC ERROR CODE 
//****************************************************************************
#define  _MHL_ERR_CODE_NO                                              0x0
#define  _MHL_ERR_CODE_Retry_EX                                     0x1
#define  _MHL_ERR_CODE_PRO_ER                                       0x2
#define  _MHL_ERR_CODE_TIME_OUT                                   0x4
#define  _MHL_ERR_CODE_INVALID_OP                                0x8
#define  _MHL_ERR_CODE_BAD_OFF                                    0x10
#define  _MHL_ERR_CODE_BUSY                                          0x20

//****************************************************************************
// Definitions of DEVICE Register Space
//0x00~0x0F    Capability
//0x20~0x2F    Interrupt
//0x30 ~0x3F    Status
//0x40 ~0x7F    Scratchpad
//****************************************************************************
//0x20~0x2F    Interrupt
#define  _MHL_REG_RCHANGE_INT                                        0x20
#define  DCAP_CHG                                                             (1 <<0)
#define  DSCR_CHG                                                             (1 <<1)
#define  REQ_WRT                                                               (1 <<2)
#define  GRT_WRT                                                                (1 <<3)
#define  _3D_WRT                                                                 (1 <<4)
//0x30 ~0x3F    Status
#define _MHL_REG_CONNECTED_RDY                                   0x30
#define   CONNECT_DCAP_RDY                                            (1 <<0)

#define _MHL_REG_LINK_MODE                                            0x31
#define  LINK_MODE_PATH_EN                                             (1 <<3)
#define  LINK_MODE_CLK_PP_MODE                                      (2<<0)
#define  LINK_MODE_CLK_24BIT_MODE                                 (3<<0)
//0x40 ~0x7F    Scratchpad
#define _MHL_REG_SCRATCH_START                                     0x40

//****************************************************************************
#define  _MHL_3D_VIC_HB                          0x00
#define  _MHL_3D_VIC_LB                           0x10
#define  _MHL_3D_DTD_HB                          0x00
#define  _MHL_3D_DTD_LB                           0x11

//-------------------------------------------------------
//  Definitions of RCP Key Codes
//-------------------------------------------------------
#define _MHL_RCP_VOLUME_UP                      			0x41
#define _MHL_RCP_VOLUME_DOWN                    		0x42
#define _MHL_RCP_MUTE                           				0x43
#define _MHL_RCP_MUTE_FUNCTION                  		0x65
#define _MHL_RCP_RESTORE_VOLUME_FUNCTION        	0x66
//-------------------------------------------------------
//  Definitions of clock mode 
//-------------------------------------------------------
#define _M2PLL_CLK      1
#define _ACTIVE_CLK      0

//--------------------------------------------------
//  MHL Version Define
//--------------------------------------------------
#define _MHL_VERSION_1_2                        0
#define _MHL_VERSION_2_0                        1

#define _MHL_VERSION                                            _MHL_VERSION_2_0

// realtek 765    ==0x2fd
#define _MHL_ADOPTER_ID_H                                   0x02
#define _MHL_ADOPTER_ID_L                                    0xfd

//--------------------------------------------------
//  MHL Linker layer timing setting
//--------------------------------------------------
#define R_bus_clk                		202.5             // unit MHZ
#define T_BIT_CBUS_MIN      		800               // unit n sec
#define T_BIT_CBUS_MAX     		1200             // unit n sec
#define T_BIT_CBUS_AVE      		1000             // unit n sec
#define T_CBUS_ACK_0         		500              // unit n sec
#define T_CBUS_ACK_0_max         600               // unit n sec
#define T_CBUS_ACK_0_min          400               // unit n sec
#define T_CBUS_ACK_FALL    		530              // unit n sec
#define T_TX_bit_time           		1000            // unit n sec
#define T_drive_H_AVE                  250             // unit n sec
#define T_link_timeout                  2000            // unit n sec

//--------------------------------------------------
// CDF
//--------------------------------------------------
#define CDF_CR_MHL_VER_MAJOR     	 2
#define CDF_CR_MHL_VER_MINOR     	 0
#define CDF_CR_DEV_TYPE          		 1   // 1: sink   2 :source 3: dongle
#define CDF_CR_POW              		 1   // 1 : support vbus out
#define CDF_CR_POW_PLIM           		 1   //0 : 500ma  1 :900ma  2 : 1.5 A
#define CDF_CR_ADOPTER_ID_H      	0x02       //RTK :765
#define CDF_CR_ADOPTER_ID_L      	0xFD
#define CDF_CR_SUPP_RGB444         	1
#define CDF_CR_SUPP_YCBCR444       	1
#define CDF_CR_SUPP_YCBCR422     	1
#define CDF_CR_SUPP_PPIXEL         	0
#define CDF_CR_SUPP_ISLANDS        	1
#define CDF_CR_SUPP_VGA            	1
#define CDF_CR_AUD_2CH             		1
#define CDF_CR_AUD_8CH             		0
#define CDF_CR_VT_GRAPHICS         	0
#define CDF_CR_VT_PHOTO            	0
#define CDF_CR_VT_CINEMA           	0
#define CDF_CR_VT_GAME             		0
#define CDF_CR_LD_DISPLAY          	1
#define CDF_CR_LD_VIDEO            		0
#define CDF_CR_LD_AUDIO            		0
#define CDF_CR_LD_MEDIA            		0
#define CDF_CR_LD_TUNER            		0
#define CDF_CR_LD_RECORD           	0
#define CDF_CR_LD_SPEAKER          	1
#define CDF_CR_LD_GUI              		0
#define CDF_CR_BANDWIDTH           	15   // 15*5  =  75Mhz
#define CDF_CR_RCP_SUPPORT         	1
#define CDF_CR_RAP_SUPPORT         	1
#define CDF_CR_SP_SUPPORT          	1
#define CDF_CR_DEVICE_ID_H         	0
#define CDF_CR_DEVICE_ID_L         	0
#define CDF_CR_SCRATCHPAD_SIZE     16
#define CDF_CR_INT_SIZE            		3
#define CDF_CR_STAT_SIZE           		3
#define CDF_CR_UCP_RECV_SUPPORT    0
#define CDF_CR_UCP_SEND_SUPPORT    0


#define FW0Requester_set_req_en(x)	   		hdmi_rx_reg_mask32(FW0_REQ_00_reg,~(FW0_REQ_00_fw0_req_mask|FW0_REQ_00_fw0_fifo_clr_mask),(FW0_REQ_00_fw0_req(x))|(FW0_REQ_00_fw0_fifo_clr(0)), HDMI_RX_MHL_CBUS_MSC_FW)
#define FW0Requester_set_tx_case(x)	   		hdmi_rx_reg_mask32(FW0_REQ_00_reg,~(FW0_REQ_00_fw0_tx_case_mask|FW0_REQ_00_fw0_fifo_clr_mask),(FW0_REQ_00_fw0_tx_case(x))|(FW0_REQ_00_fw0_fifo_clr(0)), HDMI_RX_MHL_CBUS_MSC_FW)
#define FW0Requester_set_head(x)	   		hdmi_rx_reg_mask32(FW0_REQ_00_reg,~(FW0_REQ_00_fw0_head_mask|FW0_REQ_00_fw0_fifo_clr_mask),(FW0_REQ_00_fw0_head(x))|(FW0_REQ_00_fw0_fifo_clr(0)), HDMI_RX_MHL_CBUS_MSC_FW)
#define FW0Requester_set_cmd1(x)	   		hdmi_rx_reg_mask32(FW0_REQ_01_reg,~(FW0_REQ_01_fw0_cmd1_mask),FW0_REQ_01_fw0_cmd1(x), HDMI_RX_MHL_CBUS_MSC_FW)
#define FW0Requester_set_cmd2(x)	   		hdmi_rx_reg_mask32(FW0_REQ_02_reg,~(FW0_REQ_02_fw0_cmd2_mask),FW0_REQ_02_fw0_cmd2(x), HDMI_RX_MHL_CBUS_MSC_FW)
#define FW0Requester_set_offset(x)	   		hdmi_rx_reg_mask32(FW0_REQ_03_reg,~(FW0_REQ_03_fw0_offset_mask),FW0_REQ_03_fw0_offset(x), HDMI_RX_MHL_CBUS_MSC_FW)
#define FW0Requester_set_data(x)	   		hdmi_rx_reg_mask32(FW0_REQ_04_reg,~(FW0_REQ_04_fw0_data_mask),FW0_REQ_04_fw0_data(x), HDMI_RX_MHL_CBUS_MSC_FW)
#define FW0Requester_set_wait_case(x)	   	hdmi_rx_reg_mask32(FW0_REQ_05_reg,~(FW0_REQ_05_fw0_wait_case_mask),FW0_REQ_05_fw0_wait_case(x), HDMI_RX_MHL_CBUS_MSC_FW)
#define FW0Requester_set_cmd_flag(x)	   	hdmi_rx_reg_mask32(FW0_REQ_05_reg,~(FW0_REQ_05_fw0_cmd_irq_en_mask),FW0_REQ_05_fw0_cmd_event(x), HDMI_RX_MHL_CBUS_MSC_FW)
#define FW0Requester_set_data_flag(x)	   	hdmi_rx_reg_mask32(FW0_REQ_05_reg,~(FW0_REQ_05_fw0_data_irq_en_mask),FW0_REQ_05_fw0_data_event(x), HDMI_RX_MHL_CBUS_MSC_FW)
#define FW0Requester_set_finish_flag(x)   	       hdmi_rx_reg_mask32(FW0_REQ_05_reg,~(FW0_REQ_05_fw0_fin_mask),FW0_REQ_05_fw0_fin(x), HDMI_RX_MHL_CBUS_MSC_FW)
#define FW0Requester_set_error_flag(x)   	       hdmi_rx_reg_mask32(FW0_REQ_05_reg,~(FW0_REQ_05_fw0_rcv_err_mask),FW0_REQ_05_fw0_rcv_err(x), HDMI_RX_MHL_CBUS_MSC_FW)

#define FW0Requester_CLR_error_code()   	       hdmi_rx_reg_mask32(CBUS_MSC_0A_reg,~(CBUS_MSC_0A_msc_err_code_mask),CBUS_MSC_0A_msc_err_code_mask, HDMI_RX_MHL_CBUS_MSC)

#define FW0Requester_get_finish_flag()   	       FW0_REQ_05_get_fw0_fin(hdmi_rx_reg_read32(FW0_REQ_05_reg, HDMI_RX_MHL_CBUS_MSC_FW))
#define FW0Requester_get_error_flag()   	       FW0_REQ_05_get_fw0_rcv_err(hdmi_rx_reg_read32(FW0_REQ_05_reg, HDMI_RX_MHL_CBUS_MSC_FW))
#define FW0Requester_get_cmd_flag()   	       FW0_REQ_05_get_fw0_cmd_event(hdmi_rx_reg_read32(FW0_REQ_05_reg, HDMI_RX_MHL_CBUS_MSC_FW))
#define FW0Requester_get_data_flag()   	       FW0_REQ_05_get_fw0_data_event(hdmi_rx_reg_read32(FW0_REQ_05_reg, HDMI_RX_MHL_CBUS_MSC_FW))
#define FW0Requester_get_cmd()   	       	FW0_REQ_06_get_fw0_cmd_rcv(hdmi_rx_reg_read32(FW0_REQ_06_reg, HDMI_RX_MHL_CBUS_MSC_FW))
#define FW0Requester_get_data()   	       	FW0_REQ_07_get_fw0_data_rcv(hdmi_rx_reg_read32(FW0_REQ_07_reg, HDMI_RX_MHL_CBUS_MSC_FW))
#define FW0Requester_get_error_code()   	       CBUS_MSC_0A_get_msc_err_code(hdmi_rx_reg_read32(CBUS_MSC_0A_reg, HDMI_RX_MHL_CBUS_MSC))

typedef enum {
    MHL_CLK_TYPE_Reserved = 0,
    MHL_CLK_TYPE_packedpixel_mode = 2,
    MHL_CLK_TYPE_normal_clock_mode = 3
}MHL_CLK_TYPE;

void Cbus_Power(char enable);
HDMI_BOOL drvif_MHL_DetectMode(void);
void UserMHLMscCHECK_MODE(void);
int phoenix_detect_MHL_in(void);
void CBUS_InitialPHYSettings(void);
void MHLInitialMACSettings(void);
void MHLTmdsInitial(void);
void CBUSLINKClk(UINT8  bmode);
void CBUS_TIMER_Handle_ISR(void);
void MHLCBUS_ISR(void);
UINT8 MHLNormalPreDetect(void);

#endif //_PHOENIX_MHL_CBUS_H
