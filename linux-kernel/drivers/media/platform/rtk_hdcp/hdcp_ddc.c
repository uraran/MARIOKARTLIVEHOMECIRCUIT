/* Copyright (C) 2007-2014 Realtek Semiconductor Corporation. 
 * hdcp_ddc.c
 *
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include "hdcp.h"


int ddc_write(int len,unsigned char start,unsigned char *buf)
{	
	struct i2c_adapter *p_adap;	
	unsigned char bus_id =1;
	unsigned char *data;
	//int i;
	
	data = kzalloc((len+1)*sizeof(unsigned char), GFP_KERNEL);
	
	data[0]= start;
	memcpy(data+1,buf,len);
	
#if 0	
	for(i=0;i<len+1;i++)
	 	HDCP_DEBUG("data[%d]=0x%x \n",i,data[i]);
		
	for(i=0;i<len;i++)
	 	HDCP_DEBUG("buf[%d]=0x%x\n",i,buf[i]);	
#endif
		
	struct i2c_msg msgs[] = {
		{
			.addr	= 0x3A,
			.flags	= 0,
			.len	= len+1,
			.buf	= data,
		}
	};
	
	if ((p_adap = i2c_get_adapter(bus_id))==NULL){
        HDCP_ERROR("get adapter %d failed\n", bus_id);        
        return -ENODEV;
    }

	if (i2c_transfer(p_adap, msgs, 1) == 1)
		return 0;

	
	return -ECOMM;	
}	
	
int ddc_read(int len,unsigned char start,unsigned char *buf)
{
	struct i2c_adapter *p_adap;	
	unsigned char bus_id=1;
	
	struct i2c_msg msgs[] = {
		{
			.addr	= 0x3A,
			.flags	= 0,
			.len	= 1,
			.buf	= &start,
		}, {
			.addr	= 0x3A,
			.flags	= I2C_M_RD,
			.len	= len,
			.buf	= buf,
		}
	};
	
	if ((p_adap = i2c_get_adapter(bus_id))==NULL){
        HDCP_ERROR("hdcp get adapter %d failed\n", bus_id);        
        return -ENODEV;
    }

	if (i2c_transfer(p_adap, msgs, 2) == 2)
		return 0;

	
	return -ECOMM;
}