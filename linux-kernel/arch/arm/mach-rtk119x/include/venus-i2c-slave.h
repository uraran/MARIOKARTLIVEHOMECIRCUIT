#ifndef __VENUS_I2C_SLAVE_H__
#define __VENUS_I2C_SLAVE_H__

typedef struct 
{    
    unsigned short sar;     // slave address
    
    unsigned long  private_data;    // private data

    /*---------------------------------------------------------------------- 
     * Func : handle_command 
     *
     * Desc : This function will be called as the i2c hardware received 
     *        message form remote i2c master     
     *
     * Parm : id     : i2c adapter index
     *        cmd    : command comes form i2c master
     *        len    : length of command (in bytes)
     *
     * Retn : 0 : successed, others: failed
     *----------------------------------------------------------------------*/ 
    int (*handle_command)(int id, unsigned char* cmd, unsigned char len);

    /*---------------------------------------------------------------------- 
     * Func : read_char 
     *
     * Desc : This function will be called as the i2c hardware got read request
     *        form remote i2c master.
     *
     * Parm : id     : i2c adapter index     
     *
     * Retn : data to return
     *----------------------------------------------------------------------*/ 
    unsigned char (*read_data)(int id);   // read data form i2c slave
    
}venus_i2c_slave_driver;


int venus_i2c_register_slave_driver(unsigned char id, venus_i2c_slave_driver* p_drv);


void venus_i2c_unregister_slave_driver(unsigned char id, venus_i2c_slave_driver* p_drv);


int venus_i2c_slave_enable(unsigned char id, unsigned char on);


#endif // __VENUS_I2C_SLAVE_H__
