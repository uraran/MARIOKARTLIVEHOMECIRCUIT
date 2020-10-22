#ifndef __I2C_VENUS_H__
#define __I2C_VENUS_H__

#include "../algos/i2c-algo-phoenix.h"
#include "i2c-rtk-priv.h"

typedef struct 
{
    struct i2c_adapter  adap;
    i2c_venus_algo      algo;
    struct list_head    list;
    venus_i2c*          p_phy;          
    int                 port;       // -1 means no specified port associated
    
#ifdef CONFIG_PM 
    struct platform_device* p_dev;
#endif
    
}venus_i2c_adapter;



typedef struct 
{
    unsigned char mode;
    #define I2C_MODE    0
    #define G2C_MODE    1
    
    union {
        struct {
            unsigned char phy_id;
            char          port_id;
        }i2c;
        
        struct {
            unsigned char sda;
            unsigned char scl;
        }g2c;
    };
    
}venus_i2c_adapter_config;


#define SET_I2C_CFG(phy_id, port_id)          ((0x80000000) |((phy_id)<<8) | (port_id))
#define SET_G2C_CFG(sda, scl)                 ((0xC0000000) | ((sda)<<8) | (scl))
#define IS_I2C_CFG(cfg)                       ((cfg & 0xFFFF0000)==0x80000000)
#define IS_G2C_CFG(cfg)                       ((cfg & 0xFFFF0000)==0xC0000000)
#define IS_VALID_CFG(cfg)                     (IS_G2C_MODE(cfg)||IS_I2C_MODE(cfg))
#define GET_I2C_PHY(cfg)                      ((cfg>>8) & 0xFF)
#define GET_I2C_PORT(cfg)                     (cfg & 0xFF)
#define GET_G2C_SDA(cfg)                      ((cfg>>8) & 0xFF)
#define GET_G2C_SCL(cfg)                      (cfg & 0xFF)

#define I2C_CON                 0x0000
#define I2C_TAR                 0x0004
#define I2C_SAR                 0x0008
#define I2C_HS_MADDR            0x000c
#define I2C_DATA_CMD            0x0010
#define I2C_SS_SCL_HCNT         0x0014
#define I2C_SS_SCL_LCNT         0x0018
#define I2C_FS_SCL_HCNT         0x001c
#define I2C_FS_SCL_LCNT         0x0020
#define I2C_INTR_STAT           0x002c
#define I2C_INTR_MASK           0x0030
#define I2C_RAW_INTR_STAT       0x0034
#define I2C_RX_TL               0x0038
#define I2C_TX_TL               0x003c
#define I2C_CLR_INTR            0x0040
#define I2C_CLR_RX_UNDER        0x0044
#define I2C_CLR_RX_OVER         0x0048
#define I2C_CLR_TX_OVER         0x004c
#define I2C_CLR_RD_REQ          0x0050
#define I2C_CLR_TX_ABRT         0x0054
#define I2C_CLR_RX_DONE         0x0058
#define I2C_CLR_ACTIVITY        0x005c
#define I2C_CLR_STOP_DET        0x0060
#define I2C_CLR_START_DET       0x0064
#define I2C_CLR_GEN_CALL        0x0068
#define I2C_ENABLE              0x006c
#define I2C_STATUS              0x0070
#define I2C_TXFLR               0x0074
#define I2C_RXFLR               0x0078
#define I2C_SDA_HOLD            0x007c
#define I2C_TX_ABRT_SOURCE      0x0080
#define I2C_SLV_DATA_NACK_ONLY  0x0084
#define I2C_DMA_CR              0x0088
#define I2C_DMA_TDLR            0x008c
#define I2C_DMA_RDLR            0x0090
#define I2C_SDA_SETUP          	0x0094
#define I2C_ACK_GENERAL_CALL	0x0098
#define I2C_ENABLE_STATUS		0x009c
#define I2C_COMP_PARAM_1        0x00f4
#define I2C_COMP_VERSION        0x00f8
#define I2C_COMP_TYPE           0x00fc

#define ISO_ISR_I2C0	(0x00000001 << 8)
#define ISO_ISR_I2C6	(0x00000001 << 10)

#define MIS_ISR_I2C1     	(0x00000001 << 4)
#define MIS_ISR_I2C2     	(0x00000001 << 26)
#define MIS_ISR_I2C3     	(0x00000001 << 23)
#define MIS_ISR_I2C4     	(0x00000001 << 15)
#define MIS_ISR_I2C5     	(0x00000001 << 14)

#define MIS_I2C1_SDA_DEL	0x0088
#define MIS_I2C2_SDA_DEL	0x008C
#define MIS_I2C3_SDA_DEL	0x0090
#define MIS_I2C4_SDA_DEL	0x0094
#define MIS_I2C5_SDA_DEL	0x0098

#define ISO_I2C0_SDA_DEL	0x0084
#define ISO_I2C6_SDA_DEL	0x00c0

//IC_CON
#define IC_SLAVE_DISABLE    0x0040
#define IC_RESTART_EN       0x0020
#define IC_10BITADDR_MASTER 0x0010
#define IC_10BITADDR_SLAVE  0x0008
#define IC_MASTER_MODE      0x0001

#define IC_SPEED            0x0006
#define SPEED_SS            0x0002
#define SPEED_FS            0x0004
#define SPEED_HS            0x0006

//ID_DATA
#define READ_CMD            0x0100

// INT
#define GEN_CALL_BIT        0x800
#define START_DET_BIT       0x400
#define STOP_DET_BIT        0x200
#define ACTIVITY_BIT        0x100
#define RX_DONE_BIT         0x080
#define TX_ABRT_BIT         0x040
#define RD_REQ_BIT          0x020
#define TX_EMPTY_BIT        0x010
#define TX_OVER_BIT         0x008
#define RX_FULL_BIT         0x004
#define RX_OVER_BIT         0x002
#define RX_UNDER_BIT        0x001

// STATUS
#define ST_RFF_BIT          0x10
#define ST_RFNE_BIT         0x08
#define ST_TFE_BIT          0x04
#define ST_TFNF_BIT         0x02
#define ST_ACTIVITY_BIT     0x01

#endif //__I2C_VENUS_H__
