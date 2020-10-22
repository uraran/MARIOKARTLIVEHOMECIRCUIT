/* ------------------------------------------------------------------------- */
/* i2c-venus.c i2c-hw access for Realtek Venus DVR I2C                       */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2005 Chih-pin Wu <wucp@realtek.com.tw>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    Version 1.0 written by wucp@realtek.com.tw
    Version 2.0 modified by Frank Ting(frank.ting@realtek.com.tw)(2007/06/21)   
-------------------------------------------------------------------------     
    1.4     |   20081016    | Multiple I2C Support
-------------------------------------------------------------------------     
    1.5     |   20090423    | Add Suspen/Resume Feature    
-------------------------------------------------------------------------*/    
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/wait.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/list.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <../../../arch/arm/mach-rtk119x/include/platform.h>
#include <../../../arch/arm/mach-rtk119x/include/venus-i2c-slave.h>
#include "../algos/i2c-algo-phoenix.h"
#include "i2c-phoenix-priv.h"
#include "i2c-phoenix.h"

//#define VENUS_I2C_IRQ           3
//#define VENUS_I2C_IRQ           164	//TODO : temp for i2c5
int venus_i2c_irq[7]={200,164,186,183,175,174,202};


#define VENUS_MASTER_7BIT_ADDR  0x24
#define MAX_I2C_ADAPTER         8

LIST_HEAD(venus_i2c_list);
extern char *parse_token(const char *parsed_string, const char *token);

////////////////////////////////////////////////////////////////////

#ifdef CONFIG_I2C_DEBUG_BUS
#define i2c_debug(fmt, args...)     printk(KERN_INFO fmt, ## args)
#else
#define i2c_debug(fmt, args...)
#endif

/*------------------------------------------------------------------
 * Func : i2c_venus_xfer
 *
 * Desc : start i2c xfer (read/write)
 *
 * Parm : p_msg : i2c messages 
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/  
void i2c_venus_dump_msg(const struct i2c_msg* p_msg)
{
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                          
    printk("msg->addr  = %02x\n",p_msg->addr);
    printk("msg->flags = %04x\n",p_msg->flags);
    printk("msg->len   = %d  \n",p_msg->len);
    printk("msg->buf   = %p  \n",p_msg->buf);    
}


#define IsReadMsg(x)        (x.flags & I2C_M_RD)
#define IsGPIORW(x)         (x.flags & I2C_GPIO_RW)
#define IsSameTarget(x,y)   ((x.addr == y.addr) && !((x.flags ^ y.flags) & (~I2C_M_RD)))

/*------------------------------------------------------------------
 * Func : i2c_venus_xfer
 *
 * Desc : start i2c xfer (read/write)
 *
 * Parm : adapter : i2c adapter
 *        msgs    : i2c messages
 *        num     : nessage counter
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/  
static int 
i2c_venus_xfer(
    void*                   dev_id, 
    struct i2c_msg*         msgs, 
    int                     num
    )
{
    venus_i2c_adapter* p_adp = (venus_i2c_adapter*) dev_id;
    venus_i2c* p_this = (venus_i2c*) p_adp->p_phy;
    int ret = 0;
    int i = 0;
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                                  
    if (p_adp->port>=0 && p_this->set_port(p_this, p_adp->port)<0)
        return -EACCES;            

#ifdef EDID_4BLOCK_SUPPORT
    if(p_adp->port==0 && num==3)
    {
        if(msgs[0].addr==0x30 && msgs[1].addr==0x50)
        {
            ret = p_this->read_edid_seg(p_this, msgs[0].buf[0]/*seg_point*/, msgs[1].buf[0]/*offset*/,msgs[2].buf,msgs[2].len);
            if(ret<0)
                goto err_occur;
            else
                return 3;
        }
    }
#endif

    for (i = 0; i < num; i++) 
    {                    
        ret = p_this->set_tar(p_this, msgs[i].addr, ADDR_MODE_7BITS);
        
        if (ret<0)
            goto err_occur;
            
        switch(msgs[i].flags & I2C_M_SPEED_MASK)
        {           
        case I2C_M_FAST_SPEED:
            p_this->set_spd(p_this, 400);
			break;
        case I2C_M_HIGH_SPEED:
            p_this->set_spd(p_this, 800);
			break;
        case I2C_M_LOW_SPEED:            
            p_this->set_spd(p_this, 50);
            break;
        case I2C_M_LOW_SPEED_80:
            p_this->set_spd(p_this, 80);
			break;
        case I2C_M_LOW_SPEED_66:
            p_this->set_spd(p_this, 66);
			break;
        case I2C_M_LOW_SPEED_33:
            p_this->set_spd(p_this, 33);
			break;
        case I2C_M_LOW_SPEED_10:
            p_this->set_spd(p_this, 10);
			break;
        default:            
        case I2C_M_NORMAL_SPEED:
            p_this->set_spd(p_this, 100);
 			break;
        }                   
       
        p_this->set_guard_interval(p_this, (msgs[i].flags & I2C_M_NO_GUARD_TIME) ? 0 : 10);

        if (IsReadMsg(msgs[i]))            
        {                   
            	if(0)// (IsGPIORW(msgs[i]))
                ret = p_this->gpio_read(p_this, NULL, 0, msgs[i].buf, msgs[i].len);
            else
                ret = p_this->read(p_this, NULL, 0, msgs[i].buf, msgs[i].len);
        }                            
        else
        {             
            if ((i < (num-1)) && IsReadMsg(msgs[i+1]) && IsSameTarget(msgs[i], msgs[i+1]))
            {
                // Random Read = Write + Read (same addr)                
                if(0) //(IsGPIORW(msgs[i]))
                    ret = p_this->gpio_read(p_this, msgs[i].buf, msgs[i].len, msgs[i+1].buf, msgs[i+1].len);                        
                else                    
                    ret = p_this->read(p_this, msgs[i].buf, msgs[i].len, msgs[i+1].buf, msgs[i+1].len);                        
                i++;
            }
            else
            {   
                // Single Write
                if(0)// (IsGPIORW(msgs[i]))
                    ret = p_this->gpio_write(p_this, msgs[i].buf, msgs[i].len, (i==(num-1)) ? WAIT_STOP : NON_STOP);
                else                    
                    ret = p_this->write(p_this, msgs[i].buf, msgs[i].len, (i==(num-1)) ? WAIT_STOP : NON_STOP);
            }            
        }
        
        if (ret < 0)        
            goto err_occur;                          
    }     

    return i;
    
////////////////////    
err_occur:        
    
        
    printk("-----------------------------------------\n");        
          
    switch(ret)
    {
    case -ECMDSPLIT:
        printk("[I2C%d] Xfer fail - MSG SPLIT (%d/%d)\n", p_this->id, i,num);
        break;                                            
                                        
    case -ETXABORT:             
            
        printk("[I2C%d] Xfer fail - TXABORT (%d/%d), Reason=%04x\n",p_this->id, i,num, p_this->get_tx_abort_reason(p_this));        
        break;                  
                                
    case -ETIMEOUT:              
        printk("[I2C%d] Xfer fail - TIMEOUT (%d/%d)\n", p_this->id, i,num);        
        break;                  
                                
    case -EILLEGALMSG:           
        printk("[I2C%d] Xfer fail - ILLEGAL MSG (%d/%d)\n",p_this->id, i,num);
        break;
    
    case -EADDROVERRANGE:    
        printk("[I2C%d] Xfer fail - ADDRESS OUT OF RANGE (%d/%d)\n",p_this->id, i,num);
        break;
        
    default:        
        printk("[I2C%d] Xfer fail - Unkonwn Return Value (%d/%d)\n", p_this->id, i,num);
        break;
    }    
    
    i2c_venus_dump_msg(&msgs[i]);
    
    printk("-----------------------------------------\n");        
        
    ret = -EACCES;        
    return ret;
}



/*------------------------------------------------------------------
 * Func : i2c_venus_set_speed
 *
 * Desc : set speed of venus i2c
 *
 * Parm : dev_id : i2c adapter
 *        KHz    : speed of i2c adapter 
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/  
static int 
i2c_venus_set_speed(
    void*                   dev_id, 
    int                     KHz
    )
{
    venus_i2c_adapter* p_adp = (venus_i2c_adapter*) dev_id;
    venus_i2c* p_this = (venus_i2c*) p_adp->p_phy;   
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                              
    if (p_this)
        return p_this->set_spd(p_this, KHz);
            
    return -1;          
}

    

//////////////////////////////////////////////////////////////////////////////////////////////
// Platform Device Interface
//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_PM

#define VENUS_I2C_NAME          "Venus_I2C"

static int venus_i2c_probe(struct device *dev)
{
    struct platform_device  *pdev = to_platform_device(dev);
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                          
    return strncmp(pdev->name,VENUS_I2C_NAME, strlen(VENUS_I2C_NAME));
}


static int venus_i2c_remove(struct device *dev)
{
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                          
    // we don't need to do anything for it...
    return 0;
}
 
static void venus_i2c_shutdown(struct device *dev)
{
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                          
    // we don't need to do anything for it...    
}
/*
static int venus_i2c_pm_suspend(struct device *dev)
{
    struct platform_device* pdev   = to_platform_device(dev);  
    venus_i2c*              p_this = (venus_i2c*)  pdev->id;       
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                                  
    p_this->suspend(p_this);                    
        
    return 0;    
}


static int venus_i2c_pm_resume(struct device *dev)
{    
    struct platform_device* pdev   = to_platform_device(dev);  
    venus_i2c*              p_this = (venus_i2c*) pdev->id;       
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                                  
    p_this->resume(p_this);
        
    return 0;    
}
*/
static const struct dev_pm_ops venus_i2c_pm_ops = {
    .suspend    = NULL,//TODO: venus_i2c_pm_suspend,
    .resume     = NULL,//TODO: venus_i2c_pm_resume,
#ifdef CONFIG_HIBERNATION
    .freeze     = venus_i2c_pm_suspend,
    .thaw       = venus_i2c_pm_resume,
    .poweroff   = venus_i2c_pm_suspend,
    .restore    = venus_i2c_pm_resume,
#endif
};

static struct platform_driver venus_i2c_platform_drv = 
{
    .driver = {
        .name     = VENUS_I2C_NAME,
        .bus      = &platform_bus_type, 
        .probe    = venus_i2c_probe,
        .remove   = venus_i2c_remove,
        .shutdown = venus_i2c_shutdown,
        .pm       = &venus_i2c_pm_ops,
    }
};

#endif

//////////////////////////////////////////////////////////////////////////////////////////////
// Device Attribute
//////////////////////////////////////////////////////////////////////////////////////////////




static int venus_i2c_show_speed(struct device *dev, struct device_attribute *attr, char* buf)
{    
    struct i2c_adapter* p_adap = to_i2c_adapter(dev);
    i2c_venus_algo*     p_algo = (i2c_venus_algo*) p_adap->algo_data;
    venus_i2c*          p_this = (venus_i2c*) p_algo->dev_id;       
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                                  
    if (p_this)    
        return sprintf(buf, "%d\n", p_this->spd);    
                    
    return 0;    
}



int venus_i2c_store_speed(struct device *dev, struct device_attribute *attr, char* buf,  size_t count)
{    
    struct i2c_adapter* p_adap = to_i2c_adapter(dev);
    i2c_venus_algo*     p_algo = (i2c_venus_algo*) p_adap->algo_data;
    venus_i2c*          p_this = (venus_i2c*) p_algo->dev_id;       
    int                 spd;
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                                          
    if (p_this && sscanf(buf,"%d\n", &spd)==1)     
        p_algo->set_speed(p_this, spd);
        
    return count;    
}



DEVICE_ATTR(speed, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP, venus_i2c_show_speed, venus_i2c_store_speed);



//////////////////////////////////////////////////////////////////////////////////////////////
// Module
//////////////////////////////////////////////////////////////////////////////////////////////



/*------------------------------------------------------------------
 * Func : i2c_venus_register_adapter
 *
 * Desc : register i2c_adapeter 
 *
 * Parm : p_phy       : pointer of i2c phy
 *         
 * Retn : 0 : success, others failed
 *------------------------------------------------------------------*/  
static int 
__init i2c_venus_register_adapter(
    venus_i2c*              p_phy,
    int                     port
    ) 
{        
    venus_i2c_adapter *p_adp = kmalloc(sizeof(venus_i2c_adapter), GFP_KERNEL);
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                              
    if (p_adp)
    {
        memset(p_adp, 0, sizeof(venus_i2c_adapter));               
        
        // init i2c_adapter data structure
        p_adp->adap.owner      = THIS_MODULE;
        p_adp->adap.class      = I2C_CLASS_HWMON;
    //    p_adp->adap.id         = 0x00; 
    	sprintf(p_adp->adap.name, "%s I2C %d bus", p_phy->model_name, p_phy->id);	
        p_adp->adap.algo_data  = &p_adp->algo;
        
        // init i2c_algo data structure    
        p_adp->algo.dev_id     = (void*) p_adp;
        p_adp->algo.masterXfer = i2c_venus_xfer;
        p_adp->algo.set_speed  = i2c_venus_set_speed;
        p_adp->p_phy           = p_phy;
        p_adp->port            = port;
        
#ifdef CONFIG_PM
        p_adp->p_dev = platform_device_register_simple(VENUS_I2C_NAME, (unsigned int) p_phy->id, NULL, 0);       
#endif
           
        // add bus
        i2c_venus_add_bus(&p_adp->adap);
        
        // add to list
        INIT_LIST_HEAD(&p_adp->list);
        
        list_add_tail(&p_adp->list, &venus_i2c_list);
        
        device_create_file(&p_adp->adap.dev, &dev_attr_speed);                  
    }
    
    return (p_adp) ? 0 : -1;
}


/*------------------------------------------------------------------
 * Func : i2c_venus_unregister_adapter
 *
 * Desc : remove venus i2c adapeter 
 *
 * Parm : p_adap      : venus_i2c_adapter data structure
 *         
 * Retn : 0 : success, others failed
 *------------------------------------------------------------------*/  
static 
void i2c_venus_unregister_adapter(
    venus_i2c_adapter*      p_adap
    )
{
    i2c_venus_algo* p_algo = NULL;
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                              
    if (p_adap)
    {
        list_del(&p_adap->list);                                        // remove if from the list
        
        p_algo = (i2c_venus_algo*) p_adap->adap.algo_data;
    
        device_remove_file(&p_adap->adap.dev, &dev_attr_speed);         // remove device attribute
    
        i2c_venus_del_bus(&p_adap->adap);                               // remove i2c bus
    
        destroy_venus_i2c_handle(p_adap->p_phy);        
        
#ifdef CONFIG_PM         
        if (p_adap->p_dev)
            platform_device_unregister(p_adap->p_dev);  
#endif        
        kfree(p_adap);
    }
}



#ifdef CONFIG_I2C_VENUS_GPIO_I2C


/*------------------------------------------------------------------
 * Func : get_g2c_gpio_config
 *
 * Desc : get config for GPIO i2c
 *
 * Parm : N/A
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/ 
static int __init get_g2c_gpio_config()
{
    int sda = CONFIG_I2C_VENUS_GPIO_I2C0_SDA;
    int scl = CONFIG_I2C_VENUS_GPIO_I2C0_SCL;
    
    char* param= parse_token(platform_info.system_parameters, "G2C0_GPIO");
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                              
    if (param>0)
    { 
        if (sscanf(param, "%u:%u", &sda, &scl)!=2)
        {
            printk("get_g2c_gpio_config failed, unknown G2C0_GPIO parameter %s\n", param);
            return 0xffff;          
        }
    }
    
    return ((sda << 8) | scl);
}

#endif



/*------------------------------------------------------------------
 * Func : get_i2c_gpio_config
 *
 * Desc : get config for I2C0/I2C1
 *
 * Parm : id : I2C id
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/ 
static  __maybe_unused int __init get_i2c_gpio_config(unsigned char id)
{    
    unsigned int sda = 0xFF;
    unsigned int scl = 0xFF;

    char* param = NULL;
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                          
    switch(id)
    {
    case 0:
        param= parse_token(platform_info.system_parameters, "I2C0_GPIO");
        break;
    
    case 1:
        param= parse_token(platform_info.system_parameters, "I2C1_GPIO");
        break;
        
    default:
        printk("get_i2c_gpio_config failed, invalid i2c id %d\n", id);
        return 0xffff;
    }
    
    if (param>0)
    { 
        if (sscanf(param, "%u:%u", &sda, &scl)!=2)
        {
            printk("get_g2c_gpio_config failed, unknown G2C0_GPIO parameter %s\n", param);
            return 0xffff;          
        }
    }
    
    return ((sda << 8) | scl);
}



/*------------------------------------------------------------------
 * Func : parse_i2c_config
 *
 * Desc : get config for I2C0/I2C1
 *
 * Parm : id : I2C id
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/ 
int __init parse_i2c_config(
    char*                       config_str,
    venus_i2c_adapter_config*   p_config, 
    unsigned char               config_count    
    )
{    
    char buff[256];        
    int n_config = 0;
    int i;
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                              
    if (config_str==NULL)
        return 0;        
    
    memset(buff,0, sizeof(buff));     
        
    strncpy(buff,config_str, sizeof(buff));

    config_str = buff;        
                
    for (i=0; i<sizeof(buff); i++)
    {
        if (buff[i]==',')
            buff[i]=' ';
    }
    
    //printk("I2C_CFG=\'%s\'\n", config_str);     
    
    while(n_config < config_count)
    {
        char cfg[32];
        int  val[2];
        memset(cfg, 0, 32);
        
        if (sscanf(config_str, "%s ", cfg)==0)
            break;
            
        //printk("I2C%d_CFG=%s\n", n_config, cfg);        
            
        if (sscanf(cfg, "P%dS%d", &val[0], &val[1])==2)
        {
            p_config->mode = I2C_MODE;
            p_config->i2c.phy_id = (unsigned char) val[0];
            p_config->i2c.port_id = (char) val[1];
            n_config++;
        }
        else if (sscanf(cfg, "%u:%u", &val[0], &val[1])==2)
        {
            p_config->mode = G2C_MODE;
            p_config->g2c.sda = (unsigned char) val[0];
            p_config->g2c.scl = (unsigned char) val[1];                       
            n_config++;
        }
        else
        {
            printk("[I2C] Warning, unknown config %s, ignore it\n", cfg);
            continue;
        }
                
        config_str += strlen(cfg) +1;
        p_config++;
    }
        
    return n_config;
}



/*------------------------------------------------------------------
 * Func : load_i2c_config
 *
 * Desc : load i2c config
 *
 * Parm : id : I2C id
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/ 
/*
int __init load_i2c_config(
    venus_i2c_adapter_config*   p_config, 
    unsigned char               config_count
    )
{   
    unsigned char adapter_count = parse_i2c_config(parse_token(platform_info.system_parameters, "I2C_CFG"), p_config, config_count);
    
    if (adapter_count==0)
    {
        int gpio_config;
        int i;  
    
        // old config mode
        adapter_count = get_venus_i2c_phy_count();
            
        if (adapter_count > config_count)       
            adapter_count = config_count;
    
        for (i=0; i<adapter_count; i++)    
        {
            gpio_config = get_i2c_gpio_config(i);   
            
            if (gpio_config==0xFFFF) 
            {
                p_config[i].mode = I2C_MODE;
                p_config[i].i2c.phy_id = i;
                p_config[i].i2c.port_id = -1;
            }
            else
            {
                p_config[i].mode    = G2C_MODE;
                p_config[i].g2c.sda = (gpio_config>>8) & 0xFF;
                p_config[i].g2c.scl = gpio_config & 0xFF;
            }                
        }
    
#ifdef CONFIG_I2C_VENUS_GPIO_I2C0

        if ((gpio_config = get_g2c_gpio_config())!=0xFFFF)
        {
            if (adapter_count<config_count)            
            {
                p_config[adapter_count].mode    = G2C_MODE;
                p_config[adapter_count].g2c.sda = (gpio_config>>8) & 0xFF;
                p_config[adapter_count].g2c.scl = gpio_config & 0xFF;            
                adapter_count++;
            }
            else
            {
                printk("[G2C] Open G2C failed, out of i2c config entry\n");
            }        
        }
#endif    
    }

    return adapter_count;
}
*/

venus_i2c* find_venus_i2c_by_id(unsigned char id)
{
    venus_i2c_adapter* cur = NULL;
    venus_i2c_adapter* next = NULL;
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                                  
    list_for_each_entry_safe(cur, next, &venus_i2c_list, list)
    {        
        if (cur->p_phy->id==id)            
            return cur->p_phy;        
    }
    
    return NULL;
}


int venus_i2c_register_slave_driver(
    unsigned char           id, 
    venus_i2c_slave_driver* p_drv
    )
{
    venus_i2c* phy = find_venus_i2c_by_id(id);
    venus_i2c_slave_ops ops = 
    {
        .handle_command = p_drv->handle_command,
        .read_data = p_drv->read_data,
    };
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                              
    if (phy)
    {
        phy->set_sar(phy, p_drv->sar, ADDR_MODE_7BITS);
        return phy->register_slave_ops(phy, &ops, p_drv->private_data);
    }
    
    printk("venus_i2c_register_slave_driver failed, can't find adapter %d\n", id);
    return -1;    
}


void venus_i2c_unregister_slave_driver(
    unsigned char           id, 
    venus_i2c_slave_driver* p_drv
    )
{
    venus_i2c* phy = find_venus_i2c_by_id(id);
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                              
    if (phy)
        phy->register_slave_ops(phy, NULL, 0);    
    else
        printk("venus_i2c_unregister_slave_driver failed, can't find adapter %d\n", id);        
}


int venus_i2c_slave_enable(unsigned char id, unsigned char on)
{
    venus_i2c* phy = find_venus_i2c_by_id(id);            
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                              
    if (phy==NULL)
        printk("venus_i2c_slave_enable failed, can't find adapter %d\n", id);        
        
    return (phy) ? phy->slave_mode_enable(phy, on) : -1;    
}


/*------------------------------------------------------------------
 * Func : i2c_venus_module_init
 *
 * Desc : init venus i2c module
 *
 * Parm : N/A
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/  
static int 
__init i2c_venus_module_init(void) 
{        
    venus_i2c_adapter_config adapter_config[MAX_I2C_ADAPTER];    
//    unsigned char adapter_count = I2C_PHY_CNT;
    unsigned char adapter_count = 7;
    int i;
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                              
    memset(adapter_config, 0, sizeof(adapter_config));

//    adapter_count = load_i2c_config(adapter_config, MAX_I2C_ADAPTER);          // load i2c config...
//    adapter_count = 1;
    
    for (i=0; i<adapter_count; i++)
    {        
        venus_i2c* p_this;    
//	i=5;


        adapter_config[i].mode = I2C_MODE;
        adapter_config[i].i2c.phy_id = i;
        adapter_config[i].i2c.port_id = 0;



         
//        if (adapter_config[i].mode==I2C_MODE)  
        {
            printk("[I2C%d] mode=i2c, phy=%d, port=%d\n", i, adapter_config[i].i2c.phy_id, adapter_config[i].i2c.port_id);
            //p_this = create_venus_i2c_handle(adapter_config[i].i2c.phy_id, VENUS_MASTER_7BIT_ADDR,
            //            ADDR_MODE_7BITS, SPD_MODE_SS, VENUS_I2C_IRQ);                                             
            p_this = create_venus_i2c_handle(adapter_config[i].i2c.phy_id, VENUS_MASTER_7BIT_ADDR,
                        ADDR_MODE_7BITS, SPD_MODE_SS, venus_i2c_irq[i]);                                             

             
            if (p_this==NULL)
	            printk("[I2C%d] p_this is NULL, FAIL!!!!!!!!!!!!!!\n", i);			
//                continue;

            if (p_this->init(p_this)<0 || i2c_venus_register_adapter(p_this, adapter_config[i].i2c.port_id)<0)
                destroy_venus_i2c_handle(p_this);                        
        }
/*
        else
        {
            printk("[I2C%d], mode=g2c, sda=%d, scl=%d\n", i, adapter_config[i].g2c.sda, adapter_config[i].g2c.scl);
            p_this = create_venus_g2c_handle(i,adapter_config[i].g2c.sda, adapter_config[i].g2c.scl);
             
            if (p_this==NULL)
                continue;

            if (p_this->init(p_this)<0 || i2c_venus_register_adapter(p_this, -1)<0)
                destroy_venus_i2c_handle(p_this);
        }                         
*/
    }

#ifdef CONFIG_PM        
    platform_driver_register(&venus_i2c_platform_drv);
#endif                        

    return 0;
}



/*------------------------------------------------------------------
 * Func : i2c_venus_module_exit
 *
 * Desc : exit venus i2c module
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/  
static void 
__exit i2c_venus_module_exit(void)
{   
    venus_i2c_adapter* cur = NULL;
    venus_i2c_adapter* next = NULL;
    RTK_DEBUG("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);                              
    list_for_each_entry_safe(cur, next, &venus_i2c_list, list)
    {
        i2c_venus_unregister_adapter(cur);
    }     

#ifdef CONFIG_PM
    platform_driver_unregister(&venus_i2c_platform_drv);
#endif           
}



MODULE_AUTHOR("Kevin-Wang <kevin_wang@realtek.com.tw>");
MODULE_DESCRIPTION("I2C-Bus adapter routines for Realtek Venus/Neptune DVR");
MODULE_LICENSE("GPL");

#if 0
module_init(i2c_venus_module_init);
#else
subsys_initcall(i2c_venus_module_init);
#endif
module_exit(i2c_venus_module_exit);

