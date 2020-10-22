#ifndef __I2C_VENUS_H__
#define __I2C_VENUS_H__


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


#endif //__I2C_VENUS_H__
