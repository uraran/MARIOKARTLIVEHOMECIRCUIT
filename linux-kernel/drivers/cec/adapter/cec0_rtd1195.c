/* ------------------------------------------------------------------------- 
   cec_rtk.c  cec driver for Realtek rtk           
   ------------------------------------------------------------------------- 
    Copyright (C) 2012 Kevin Wang <kevin_wang@realtek.com.tw>
----------------------------------------------------------------------------
Update List :
----------------------------------------------------------------------------
    1.0     |   20120608    |   Kevin Wang  | 1) create phase
----------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <asm/io.h>

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <linux/kthread.h>
#include "cec_rtd1195.h"
#include "cectx_rtd1195_reg.h"

#include <RPCDriver.h>
#include <linux/dma-mapping.h>
#include "cec_rpc.h"

#define CONVERT_FOR_AVCPU(x)        ((unsigned int)(x) | 0xA0000000)

#ifdef AO_ON_SCPU
void rtk_ao_kernel_rpc(
int type, 
unsigned char *pArg, unsigned int size_pArg,
unsigned char *pRet, unsigned int size_pRet,
unsigned char *pFuncRet, unsigned int size_pFuncRet);
#endif

#undef cec_info
#undef cec_warning
#define cec_info(format, ...)		 printk("[cec0]" format "\n", ## __VA_ARGS__)	
#define cec_warning(format, ...)     printk(KERN_WARNING"[cec0]" format "\n", ## __VA_ARGS__)

static rtk_cec m_cec;
static void __iomem *cectx_base;
static void __iomem *cectx_irq_reg;
static void __iomem *cectx_iso_base; // clk_en & reset
static void __iomem *sys_pll_bus_base;
static const struct of_device_id cectx_ids[] __initconst = {
	{ .compatible = "Realtek,rtk119x-cec0" },
	{/*Sentinel*/},
};

void cec_reset_tx(void);
void cec_wait_tx_int(void);
void cec_check_rd_reg_jne(int reg_addr, int mask_value, int want_value);

//#define CONFIG_CEC_DBG
//#define CEC_TX_DBG
//#define CEC_RX_DBG

////////////////////////////////////////////////////////////////////////
// CEC Wakeup Function
////////////////////////////////////////////////////////////////////////
//#define CEC_OPCODE_COMPARE_ENABLE
#ifdef CEC_OPCODE_COMPARE_ENABLE
#define IS_OPCODE_COMP_EN()				(read_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE)&0xf)
#define PWR_STS_COMP_EN() 				(read_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE)&0x00000001)
#define PHY_ADDR_COMP_EN() 				(read_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE)&0x00000002)
#define CEC_VER_COMP_EN() 				(read_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE)&0x00000004)
#define VEND_ID_COMP_EN() 				(read_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE)&0x00000008)
#endif

//static unsigned long  __sleepdata rtk_cec_standby_config        	= 0;
//static unsigned char  __sleepdata rtk_cec_standby_logical_addr  = 0xF;
//static unsigned short __sleepdata rtk_cec_standby_physical_addr	= 0xFFFF;
//static unsigned char  __sleepdata rtk_cec_standby_cec_version   	= CEC_VERSION_1_4;    
//static unsigned long  __sleepdata rtk_cec_standby_vendor_id     	= 0x000000;       

static unsigned long   rtk_cec_standby_config        	= 0;
static unsigned char   rtk_cec_standby_logical_addr  	= 0xF;
static unsigned short  rtk_cec_standby_physical_addr	= 0xFFFF;
static unsigned char   rtk_cec_standby_cec_version   	= CEC_VERSION_1_4;    
static unsigned long   rtk_cec_standby_vendor_id     	= 0x000000;    
static unsigned short  rtk_cec_standby_rx_mask 		    = 0;
   
static unsigned int cectx_irq_num;

/*------------------------------------------------------------------
 * Func : rtk_dump_cec_register
 *
 * Desc : dump cec register
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/

void rtk_dump_cec_register(unsigned char *str)
{
		
	printk("%s\n",str);	
	
	printk("CEC0 clock:\n");	
	printk("(%s,%x)=0x%x\n","SOFT_RESET  ",(u32)(cectx_iso_base + RTK_CEC_SOFT_RESET),read_reg32(cectx_iso_base + RTK_CEC_SOFT_RESET));
	printk("(%s,%x)=0x%x\n","CLOCK_ENABLE",(u32)(cectx_iso_base + RTK_CEC_CLOCK_ENABLE),read_reg32(cectx_iso_base + RTK_CEC_CLOCK_ENABLE));
	printk("(%s,%x)=0x%x\n","SYS_PLL_BUS1",(u32)(sys_pll_bus_base + SYS_PLL_BUS1),read_reg32(sys_pll_bus_base + SYS_PLL_BUS1));
	printk("(%s,%x)=0x%x\n","SYS_PLL_BUS3",(u32)(sys_pll_bus_base + SYS_PLL_BUS3),read_reg32(sys_pll_bus_base + SYS_PLL_BUS3));                     
	                 		
	printk("CEC0 basic:\n");
	printk("(%s,%x)=0x%x\n","CEC_TXTCR1",(u32)(cectx_base+ CEC_TXTCR1),read_reg32(cectx_base+ CEC_TXTCR1));	
	printk("(%s,%x)=0x%x\n","CEC_TXTCR0",(u32)(cectx_base+ CEC_TXTCR0),read_reg32(cectx_base+ CEC_TXTCR0));		
	printk("(%s,%x)=0x%x\n","CEC_RXTCR0",(u32)(cectx_base+ CEC_RXTCR0),read_reg32(cectx_base+ CEC_RXTCR0));
	printk("(%s,%x)=0x%x\n","CEC_CR0   ",(u32)(cectx_base+ CEC_CR0)   ,read_reg32(cectx_base+ CEC_CR0));
	printk("(%s,%x)=0x%x\n","CEC_RTCR0 ",(u32)(cectx_base+ CEC_RTCR0) ,read_reg32(cectx_base+ CEC_RTCR0));
	printk("(%s,%x)=0x%x\n","CEC_RXCR0 ",(u32)(cectx_base+ CEC_RXCR0) ,read_reg32(cectx_base+ CEC_RXCR0));
	printk("(%s,%x)=0x%x\n","CEC_TXCR0 ",(u32)(cectx_base+ CEC_TXCR0) ,read_reg32(cectx_base+ CEC_TXCR0));
	printk("(%s,%x)=0x%x\n","CEC_ANALOG",(u32)(cectx_base+ RTK_CEC_ANALOG),read_reg32(cectx_base+ RTK_CEC_ANALOG));	      
	
	printk("CEC0 interrupt:\n");
	printk("(%s,%x)=0x%x\n","SCPU_INT_EN",(u32)(cectx_irq_reg + SCPU_INT_EN),read_reg32(cectx_irq_reg + SCPU_INT_EN));
	printk("(%s,%x)=0x%x\n","ACPU_INT_EN",(u32)(cectx_irq_reg + ACPU_INT_EN),read_reg32(cectx_irq_reg + ACPU_INT_EN));
	
}
/*------------------------------------------------------------------
 * Func : _read_rx_fifo
 *
 * Desc : read rx fifo
 *
 * Parm : cmb : cec message buffer
 *         
 * Retn : N/A  
 *------------------------------------------------------------------*/
int _read_rx_fifo(cm_buff* cmb)
{    
    unsigned long rx_fifo_data[4];
    int len = read_reg32(cectx_base + CEC_RXCR0) & 0x1F;
    int i;    
    
    cec_rx_dbg("[cec0]_read_rx_fifo : rx data len = %d\n", len);
    
    if (cmb_tailroom(cmb) >= len)
    {        
        while(len)
        {            
            unsigned char rx_len = len;
            
            if (rx_len > 16)
                rx_len = 16;
                
            for (i=0; i<rx_len; i+=4)
                rx_fifo_data[i>>2] = __cpu_to_be32(read_reg32(cectx_base + CEC_RXDR1 + i));
                
            cec_rx_dbg("[cec0]cmb->tail=%p, rx_fifo_data=%p, rx_len=%d\n", cmb->tail, rx_fifo_data, rx_len);                
               
            memcpy(cmb->tail, (unsigned char*)rx_fifo_data, rx_len);                    
            
            cmb_put(cmb, rx_len);    
                           
            write_reg32(cectx_base + CEC_TXDR0, RX_SUB_CNT | FIFO_CNT(rx_len));  // fetch next rx data                           
            
            len -= rx_len;
        }
        
        return cmb->len;                
    }
    else
    {
        cec_warning("no more space to read data");
        return -ENOMEM;
    }
}



/*------------------------------------------------------------------
 * Func : _write_tx_fifo
 *
 * Desc : write data to tx fifo
 *
 * Parm : cmb
 *         
 * Retn : write length
 *------------------------------------------------------------------*/
int _write_tx_fifo(cm_buff* cmb)
{
    int remain = 0x1F - (unsigned char)(read_reg32(cectx_base + CEC_TXCR0) & 0x1F);
    int i;
    int len = cmb->len;
    unsigned long tx_fifo_data[4];    
 
    if (len > remain)
        len = remain;
        
    while(len)
    {
        unsigned char tx_len = len;
        
        if (tx_len >16)
            tx_len = 16;
        
        cec_tx_dbg("[cec0]in _write_tx_fifo : tx data len = %d\n", tx_len);       
#ifdef CEC_TX_DBG
		memset(&tx_fifo_data, 0, sizeof(unsigned long)*4);
#endif
        memcpy(tx_fifo_data, cmb->data, tx_len);
        cmb_pull(cmb, tx_len);                

		//Dump write data
		cec_tx_dbg("[cec0][%s] data[0]=0x%08x,data[1]=0x%08x,data[2]=0x%08x,data[3]=0x%08x\n", \
			__FUNCTION__,tx_fifo_data[0],tx_fifo_data[1],tx_fifo_data[2],tx_fifo_data[3]);

        for (i=0; i<tx_len; i+=4)    
            write_reg32(cectx_base + CEC_TXDR1 + i, __cpu_to_be32(tx_fifo_data[i>>2]));    
         
        write_reg32(cectx_base + CEC_TXDR0, TX_ADD_CNT | FIFO_CNT(tx_len));  // add data to fifo

		cec_tx_dbg("[cec0][%s] TXCR0(0x%08x) TXDR0(0x%08x) TXDR1(0x%08x)\n", \
			__FUNCTION__,read_reg32(cectx_base + CEC_TXCR0),read_reg32(cectx_base + CEC_TXDR0),read_reg32(cectx_base + CEC_TXDR1));
        
        len -= tx_len;
    }
                      
    return len;   
}



/*------------------------------------------------------------------
 * Func : _cmb_tx_complete
 *
 * Desc : complete a tx cmb
 *
 * Parm : p_this : handle of rtk cec 
 *         
 * Retn : N/A  
 *------------------------------------------------------------------*/
void _cmb_tx_complete(cm_buff* cmb)
{
    if (cmb->flags & NONBLOCK)
    {
        kfree_cmb(cmb);
    }
    else
    {
        if (cmb->status==WAIT_XMIT)
            cmb->status = XMIT_ABORT;
            
        wake_up(&cmb->wq);        
    }
}



/*------------------------------------------------------------------
 * Func : rtk_cec_rx_reset
 *
 * Desc : reset rx
 *
 * Parm : p_this : handle of rtk cec  
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static 
void rtk_cec_rx_reset(rtk_cec* p_this)
{       
    write_reg32(cectx_base + CEC_RXCR0, RX_RST);
    wmb();
    write_reg32(cectx_base + CEC_RXCR0, 0);        
    wmb();
    p_this->rcv.state = IDEL;
}



/*------------------------------------------------------------------
 * Func : rtk_cec_tx_work
 *
 * Desc : rtk cec tx function 
 *
 * Parm : p_this : handle of rtk cec 
 *         
 * Retn : N/A  
 *------------------------------------------------------------------*/
void rtk_cec_tx_work(struct work_struct* work)
{    
    rtk_cec* p_this = (rtk_cec*) container_of(work, rtk_cec, xmit.work.work);          
    rtk_cec_xmit* p_xmit = &p_this->xmit;    
    unsigned char dest;        
    unsigned long flags;     
    
    spin_lock_irqsave(&p_this->lock, flags);   
                             
    if (p_xmit->state == XMIT)
    {
        if (read_reg32(cectx_base + CEC_TXCR0) & TX_EN)
        {                                                   
            // xmitting           
            if (read_reg32(cectx_base + CEC_TXCR0) & TX_INT)
            {      
                if (p_xmit->cmb->len)
                {
                    _write_tx_fifo(p_xmit->cmb);
                    
                    if (p_xmit->cmb->len==0)            
                        write_reg32(cectx_base + CEC_TXCR0, read_reg32(cectx_base + CEC_TXCR0) &~TX_CONTINUOUS);   // clear continue bits....                                     
                }                            

                write_reg32(cectx_base + CEC_TXCR0, read_reg32(cectx_base + CEC_TXCR0));   // clear interrupt                               
            }   
            
            if (time_after(jiffies, p_xmit->timeout))
            {                                    
                write_reg32(cectx_base + CEC_TXCR0, 0);                          
                
                p_xmit->cmb->status = XMIT_TIMEOUT;                
                _cmb_tx_complete(p_xmit->cmb);
                        
                p_xmit->cmb   = NULL;
                p_xmit->state = IDEL;
                
                cec_warning("cec tx timeout");
                
                cancel_delayed_work(&p_xmit->work);
            }                     
        }            
        else
        {
            cancel_delayed_work(&p_xmit->work);
            
            if ((read_reg32(cectx_base + CEC_TXCR0) & TX_EOM)==0)
            {
                p_xmit->cmb->status = XMIT_FAIL;                
                cec_warning("cec tx failed");
            }
            else
            {
                p_xmit->cmb->status = XMIT_OK;
                cec_tx_dbg("[cec0]cec tx completed\n");
            }
            
            write_reg32(cectx_base + CEC_TXCR0, 0);			
            _cmb_tx_complete(p_xmit->cmb);
            p_xmit->cmb   = NULL;
            p_xmit->state = IDEL;                     
        }                                                 
    }        
    
    if (p_xmit->state==IDEL)        
    {
        if (p_xmit->enable)
        {                                    
            p_xmit->cmb = cmb_dequeue(&p_this->tx_queue);
            
            if (p_xmit->cmb)
            {                            
                cec_tx_dbg("[cec0]cec tx : cmb = %p, len=%d\n", p_xmit->cmb, p_xmit->cmb->len);
                dest = (p_xmit->cmb->data[0] & 0xf);                               
                cmb_pull(p_xmit->cmb, 1);          
                p_xmit->timeout = jiffies + TX_TIMEOUT;

                // reset tx fifo
                write_reg32(cectx_base + CEC_TXCR0, TX_RST);
                wmb();
                write_reg32(cectx_base + CEC_TXCR0, 0);                                    
                wmb();     
                                       				
                _write_tx_fifo(p_xmit->cmb);                
                                                   
                if (p_xmit->cmb->len==0)
					write_reg32(cectx_base + CEC_TXCR0, TX_ADDR_EN | TX_ADDR(rtk_cec_standby_logical_addr) | DEST_ADDR(dest) | TX_EN | TX_INT_EN);										
                else
					write_reg32(cectx_base + CEC_TXCR0, TX_ADDR(rtk_cec_standby_logical_addr) | DEST_ADDR(dest) | TX_EN | TX_INT_EN | TX_CONTINUOUS);						
                                          
                p_xmit->state = XMIT;
                
                cec_tx_dbg("[cec0]cec tx start\n");

				//cec_wait_tx_int();        
				//cec_check_rd_reg_jne(0xfe00750C, 0x000000C0,0x000000C0);
                
				schedule_delayed_work(&p_xmit->work, TX_TIMEOUT+1);      // queue delayed work for timeout detection
            }
        }
    }
    
    spin_unlock_irqrestore(&p_this->lock, flags);    
}



/*------------------------------------------------------------------
 * Func : rtk_cec_tx_start
 *
 * Desc : start rx
 *
 * Parm : p_this : handle of rtk cec  
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static 
void rtk_cec_tx_start(rtk_cec* p_this)
{   
    unsigned long flags;    
    spin_lock_irqsave(&p_this->lock, flags);    
                
    if (!p_this->xmit.enable)
    {                        
        cec_info("cec tx start");
        p_this->xmit.enable = 1;
        p_this->xmit.state  = IDEL;
        p_this->xmit.cmb    = NULL;          
 
        INIT_DELAYED_WORK(&p_this->xmit.work, rtk_cec_tx_work);
    }    
    
    spin_unlock_irqrestore(&p_this->lock, flags);
}


/*------------------------------------------------------------------
 * Func : rtk_cec_tx_stop
 *
 * Desc : stop tx
 *
 * Parm : p_this : handle of rtk cec  
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static 
void rtk_cec_tx_stop(rtk_cec* p_this)
{   
    cm_buff* cmb;
    unsigned long flags;    
    spin_lock_irqsave(&p_this->lock, flags);    
                
    if (p_this->xmit.enable)
    {   
        cec_info("cec tx stop");
            
        rtk_cec_rx_reset(p_this);        
                    
        if (p_this->xmit.cmb)
            _cmb_tx_complete(p_this->xmit.cmb);
            
        while((cmb = cmb_dequeue(&p_this->tx_queue)))
            _cmb_tx_complete(cmb);
                        
        p_this->xmit.enable = 0;
        p_this->xmit.state  = IDEL;
        p_this->xmit.cmb    = NULL;
    }    
    
    spin_unlock_irqrestore(&p_this->lock, flags);
}


extern int rtk_cec_xmit_message(rtk_cec* p_this, cm_buff* cmb, unsigned long flags);

/*------------------------------------------------------------------
 * Func : rtk_cec_standby_message_handler
 *
 * Desc : rtk cec message handler when cec works under standby mode
 *
 * Parm : p_this : handle of rtk cec 
 *        init   : message initator
 *        dest   : message destination
 *        opcode : opcode of message
 *        param  : param of messsage
 *        len    : length of message parameter
 *         
 * Retn : N/A  
 *------------------------------------------------------------------*/
void rtk_cec_standby_message_handler(
    rtk_cec*             p_this, 
    unsigned char           init,    
    unsigned char           dest,
    unsigned char           opcode,
    unsigned char*          param,
    unsigned char           len
    )
{        
    cm_buff* cmb = NULL;
    
    unsigned char dev_type_map[15] = 
    {
        CEC_DEVICE_TV, 
        CEC_DEVICE_RECORDING_DEVICE, 
        CEC_DEVICE_RECORDING_DEVICE,
        CEC_DEVICE_TUNER,
        CEC_DEVICE_PLAYBACK_DEVICE,
        CEC_DEVICE_AUDIO_SYSTEM,
        CEC_DEVICE_TUNER,
        CEC_DEVICE_TUNER,
        CEC_DEVICE_PLAYBACK_DEVICE,
        CEC_DEVICE_RECORDING_DEVICE,
        CEC_DEVICE_TUNER,
        CEC_DEVICE_PLAYBACK_DEVICE,
        CEC_DEVICE_RESERVED,
        CEC_DEVICE_RESERVED,
        CEC_DEVICE_TV
    };  
    
    //cec_info("rtk_cec_standby_message_handler : init = %x, dest=%x, opcode=%x, len=%d", init, dest, opcode, len);   
    
    switch(opcode)
    {                
    case CEC_MSG_GIVE_DEVICE_POWER_STATUS:
        
        if (rtk_cec_standby_config & STANBY_RESPONSE_GIVE_POWER_STATUS && len==0 && init!=0xF)
        { 

#ifdef CEC_OPCODE_COMPARE_ENABLE
				if(PWR_STS_COMP_EN())
					break;
#endif			
            if ((cmb=alloc_cmb(3)))
            {                
                cmb->data[0] = (dest<<4) | init; 
                cmb->data[1] = CEC_MSG_REPORT_POWER_STATUS;  
                cmb->data[2] = CEC_POWER_STATUS_STANDBY;
                cmb_put(cmb,3);
            }
        }
        break;

    case CEC_MSG_GIVE_PHYSICAL_ADDRESS:
        
        if (rtk_cec_standby_config & STANBY_WAKEUP_BY_SET_STREAM_PATH && len==0 && dest !=0xF)
        {       

#ifdef CEC_OPCODE_COMPARE_ENABLE
				if(PHY_ADDR_COMP_EN())
					break;
#endif					
            if ((cmb=alloc_cmb(5)))
            {                
                cmb->data[0] = (dest<<4) | 0xF; 
                cmb->data[1] = CEC_MSG_REPORT_PHYSICAL_ADDRESS;  
                cmb->data[2] = (rtk_cec_standby_physical_addr >> 8) & 0xFF;
                cmb->data[3] = (rtk_cec_standby_physical_addr) & 0xFF;
                cmb->data[4] = dev_type_map[dest];
                cmb_put(cmb, 5);
            }
        }
        break;
        
    case CEC_MSG_STANDBY:
        break;  
                      
    default:
        // send feature abort         
        if (init!=0xF && dest !=0xF)
        {
            if ((cmb=alloc_cmb(4)))
            {                
                cmb->data[0] = (dest<<4) | init;
                cmb->data[1] = CEC_MSG_FEATURE_ABORT;
                cmb->data[2] = opcode;        // FEATURE ABORT          
                cmb->data[3] = CEC_ABORT_NOT_IN_CORECT_MODE;
                cmb_put(cmb, 4);
            }
        }          
    }
    
    if (cmb)
        rtk_cec_xmit_message(p_this, cmb, NONBLOCK);
}



/*------------------------------------------------------------------
 * Func : rtk_cec_rx
 *
 * Desc : rtk cec rx function 
 *
 * Parm : work_struct : handle of work structure
 *         
 * Retn : N/A  
 *------------------------------------------------------------------*/
void rtk_cec_rx_work(struct work_struct * work)
{          
    rtk_cec* p_this = (rtk_cec*) container_of(work, rtk_cec, rcv.work.work);              
    rtk_cec_rcv* p_rcv = &p_this->rcv;            
    unsigned long flags;            
    unsigned long rx_event = read_reg32(cectx_base + CEC_RXCR0);
     
    spin_lock_irqsave(&p_this->lock, flags);       
       
    if (!p_rcv->enable)
    {
        if (p_rcv->state==RCV)
        {
            cec_rx_dbg("[cec0]force stop\n");            
            write_reg32(cectx_base + CEC_RXCR0, 0);
            kfree_cmb(p_rcv->cmb);
            p_rcv->cmb   = NULL;
            p_rcv->state = IDEL;
            
            // reset cec0
         //   write_reg32(cectx_base + CEC_RXCR0, RX_RST);                
         //   wmb();
         //   write_reg32(cectx_base + CEC_RXCR0, 0);        
         //   wmb();        
              
        }        
        goto end_proc;
    }    
                    
    if (p_rcv->state==RCV)
    {                   
#ifdef CEC_OPCODE_COMPARE_ENABLE    
        if ((rx_event & RX_EN) && (!IS_OPCODE_COMP_EN()))
#else
        if (rx_event & RX_EN)
#endif
        {                           
            if (rx_event & RX_INT)
            {                                               
                if (_read_rx_fifo(p_rcv->cmb)<0)
                {
                    cec_rx_dbg("[cec0]read rx fifo failed, return to rx\n");                    
                    write_reg32(cectx_base + CEC_RXCR0, 0);
                    p_rcv->state = IDEL;                       
                }                            
                 
                write_reg32(cectx_base + CEC_RXCR0, RX_INT);
            }            
        }
        else
        {               
            cec_rx_dbg("[cec0]rx_stop (0x%08x)\n", read_reg32(cectx_base + CEC_RXCR0));
            
            if ((rx_event & RX_EOM) && _read_rx_fifo(p_rcv->cmb))
            {                                                                 
                if (rx_event & BROADCAST_ADDR)
                    *cmb_push(p_rcv->cmb, 1) = (((rx_event & INIT_ADDR_MASK)>>INIT_ADDR_SHIFT)<<4) | 0xF;
                else
                    *cmb_push(p_rcv->cmb, 1) = (((rx_event & INIT_ADDR_MASK)>>INIT_ADDR_SHIFT)<<4) |
                                               ((read_reg32(cectx_base + CEC_CR0) & LOGICAL_ADDR_MASK) >> LOGICAL_ADDR_SHIFT);
                                     
                if (p_this->status.standby_mode) 
                {
                    rtk_cec_standby_message_handler(       // process messag
                        p_this, 
                        p_rcv->cmb->data[0] >> 4, 
                        p_rcv->cmb->data[0] & 0xF,
                        p_rcv->cmb->data[1],  
                        &p_rcv->cmb->data[2],
                        p_rcv->cmb->len -2);                  
                }
                else
                {
                    cmb_queue_tail(&p_this->rx_queue, p_rcv->cmb);                        
                    p_rcv->cmb = NULL;
                
                    wake_up_interruptible(&p_this->rcv.wq);
                }
            }

            p_rcv->state = IDEL;               
        }         
    }
    
    if (p_rcv->state==IDEL)
    {           
        if (!p_rcv->enable)
            goto end_proc;
             
        if (!p_rcv->cmb)   
        {            
            if (cmb_queue_len(&p_this->rx_free_queue))            
                p_rcv->cmb = cmb_dequeue(&p_this->rx_free_queue);
            else
            {
                cec_warning("WARNING, rx over run");
                p_rcv->cmb = cmb_dequeue(&p_this->rx_queue);        // reclaim form rx fifo
            }
                
            if (p_rcv->cmb==NULL)
            {
                cec_warning("FATAL, something wrong, no rx buffer left");
                goto end_proc;
            }
        }
                              
        cmb_purge(p_rcv->cmb);
        cmb_reserve(p_rcv->cmb, 1);
                
        write_reg32(cectx_base + CEC_RXCR0, RX_RST);                
        wmb();
        write_reg32(cectx_base + CEC_RXCR0, 0);        
        wmb();        
                                                                                                              
        write_reg32(cectx_base + CEC_RXCR0, RX_EN | RX_INT_EN);
        cec_rx_dbg("[cec0]rx_restart (0x%08x)\n", read_reg32(cectx_base + CEC_RXCR0));                        
        p_rcv->state = RCV;                
    }
    
end_proc:

    spin_unlock_irqrestore(&p_this->lock, flags);                      
}




/*------------------------------------------------------------------
 * Func : rtk_cec_rx_start
 *
 * Desc : start rx
 *
 * Parm : p_this : handle of rtk cec  
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static 
void rtk_cec_rx_start(rtk_cec* p_this)
{   
    unsigned long flags;    
    spin_lock_irqsave(&p_this->lock, flags);    
                
    if (!p_this->rcv.enable)
    {                       
        cec_info("rx start");
        
        p_this->rcv.enable = 1;
        p_this->rcv.state  = IDEL;
        p_this->rcv.cmb    = NULL;      
        
        INIT_DELAYED_WORK(&p_this->rcv.work, rtk_cec_rx_work);
                
        schedule_delayed_work(&p_this->rcv.work, 0);                  
    }    
    
    spin_unlock_irqrestore(&p_this->lock, flags);
}



/*------------------------------------------------------------------
 * Func : rtk_cec_rx_stop
 *
 * Desc : stop rx
 *
 * Parm : p_this : handle of rtk cec  
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static 
void rtk_cec_rx_stop(rtk_cec* p_this)
{   
    cm_buff* cmb;
    unsigned long flags;    
        
    spin_lock_irqsave(&p_this->lock, flags);    
    
    if (p_this->rcv.enable)
    {           
        cec_info("rx stop");        
        
        write_reg32(cectx_base + CEC_RXCR0, RX_RST);
        wmb();
        write_reg32(cectx_base + CEC_RXCR0, 0);        
        wmb();
                    
        if (p_this->rcv.cmb)
        {
            cmb_purge(p_this->rcv.cmb);
            cmb_reserve(p_this->rcv.cmb, 1);
            cmb_queue_tail(&p_this->rx_free_queue, p_this->rcv.cmb);
        }    
            
        while((cmb = cmb_dequeue(&p_this->rx_queue)))
        {
            cmb_purge(cmb);
            cmb_reserve(cmb, 1);
            cmb_queue_tail(&p_this->rx_free_queue, cmb);
        }
        
        p_this->rcv.enable = 0;
        p_this->rcv.state  = IDEL;
        p_this->rcv.cmb    = NULL;
        
        wake_up_interruptible(&p_this->rcv.wq);
    }    
    spin_unlock_irqrestore(&p_this->lock, flags);
}



/*------------------------------------------------------------------
 * Func : rtk_cec_isr
 *
 * Desc : isr of rtk cec
 *
 * Parm : p_this : handle of rtk cec 
 *        dev_id : handle of the rtk_cec 
 *         
 * Retn : IRQ_NONE, IRQ_HANDLED
 *------------------------------------------------------------------*/
static 
irqreturn_t rtk_cec_isr(
    int                     this_irq, 
    void*                   dev_id    
    )
{            
    rtk_cec* p_this = (rtk_cec*) dev_id;
    irqreturn_t ret = IRQ_NONE;            

    
    if (read_reg32(cectx_base + CEC_TXCR0) & TX_INT) {
        rtk_cec_tx_work(&p_this->xmit.work.work);    
        ret = IRQ_HANDLED;
        
    }
    
    if (read_reg32(cectx_base + CEC_RXCR0) & RX_INT) { 
        rtk_cec_rx_work(&p_this->rcv.work.work);
        ret = IRQ_HANDLED;
       
    }    
    return ret;    

}

/*------------------------------------------------------------------
 * Func : rtk_cec_init
 *
 * Desc : init a rtk cec adapter
 *
 * Parm : N/A
 *         
 * Retn : handle of cec module 
 *------------------------------------------------------------------*/
int rtk_cec_init(rtk_cec* p_this)
{
	
    cm_buff*  cmb = NULL;
    int i;
	int cec_pre_div = 0x21;    
	int fout,pllbus_n,pllbus_m,pllbus_o;
	
	sys_pll_bus_base = ioremap(SOFT_RESET1_reg, sizeof(unsigned int)+SYS_PLL_BUS3);
		
    pllbus_m = ((read_reg32(sys_pll_bus_base + SYS_PLL_BUS1) & SYS_PLL_BUS1_pllbus_m_mask) >> SYS_PLL_BUS1_pllbus_m_shift);
    pllbus_n = ((read_reg32(sys_pll_bus_base + SYS_PLL_BUS1) & SYS_PLL_BUS1_pllbus_n_mask) >> SYS_PLL_BUS1_pllbus_n_shift);
    pllbus_o = ((read_reg32(sys_pll_bus_base + SYS_PLL_BUS3) & SYS_PLL_BUS3_pllbus_o_mask) >> SYS_PLL_BUS3_pllbus_o_shift);
	fout = 27000000 * (pllbus_m + 2) / (pllbus_n + 1) / (pllbus_o + 1) / 2;
//    cec_pre_div = fout / (162000000/202); // fout/cec_pre_div = 162MHz/202;because CEC clock come from rbus
    

    if (!p_this->status.init)
    {              
        cec_info("Open rtk CEC");
		
		cectx_iso_base = ioremap(ISO_ISR_reg, sizeof(unsigned int) + RTK_CEC_CLOCK_ENABLE);	
		//rtk_dump_cec_register("before");
        
        //SETBITS(cectx_iso_base, RTK_CEC_SOFT_RESET, SOFT_RESET_rstn_cec0_mask); // 0xfe007088: already set by hw_setting. 
        //SETBITS(cectx_iso_base, RTK_CEC_CLOCK_ENABLE, CLOCK_ENABLE_clk_en_misc_cec0_mask); // 0xfe00708C: already set by hw_setting. 
				        
		SETBITS(cectx_base, RTK_CEC_ANALOG, CEC_ANALOG_REG_CEC_RPU_EN_MASK); //0xfe0075E0: enable pull up resistor.
	    
        write_reg32(cectx_base+ CEC_TXTCR1, TX_DATA_LOW(0x1A) | TX_DATA_01(0x23) | TX_DATA_HIGH(0x22));
        
        write_reg32(cectx_base+ CEC_TXTCR0, TX_START_LOW(0x93) | TX_START_HIGH(0x20));                       
         
        write_reg32(cectx_base+ CEC_RXTCR0, RX_START_LOW(0x8C) | RX_START_PERIOD(0xC1) | 
											RX_DATA_SAMPLE(0x2A) | RX_DATA_PERIOD(0x52));
                                
        write_reg32(cectx_base+ CEC_CR0 , CEC_MODE(1) | LOGICAL_ADDR(0x4) | TIMER_DIV(0x14) | 
										  PRE_DIV(cec_pre_div) | UNREG_ACK_EN);   
		          	
        MASK_REG_32(cectx_base,CEC_RTCR0, ~RETRY_NO(5), RETRY_NO(2));		
     
		
		/* reset Rx */
		write_reg32(cectx_base+ CEC_RXCR0,  RX_RST); 
        write_reg32(cectx_base+ CEC_RXCR0,  0);         
       
	   /* reset Tx */
        write_reg32(cectx_base+ CEC_TXCR0,  TX_RST);        
        write_reg32(cectx_base+ CEC_TXCR0,  0);         
        
#ifdef CEC_OPCODE_COMPARE_ENABLE
        write_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE, 0); //disable opcode compare function
#endif	
      
	    spin_lock_init(&p_this->lock);        
        p_this->xmit.state      = IDEL;    
        p_this->xmit.cmb        = NULL;    
        p_this->xmit.timeout    = 0;        
        
        p_this->rcv.state       = IDEL;  
        p_this->rcv.cmb         = NULL;  
        init_waitqueue_head(&p_this->rcv.wq);                    
        
        cmb_queue_head_init(&p_this->tx_queue);
        cmb_queue_head_init(&p_this->rx_queue);
        cmb_queue_head_init(&p_this->rx_free_queue);
        
        for (i=0; i< RX_RING_LENGTH; i++)
        {
            cmb = alloc_cmb(RX_BUFFER_SIZE);
            if (cmb)
                cmb_queue_tail(&p_this->rx_free_queue, cmb);            
        }            

        if (request_irq(cectx_irq_num, rtk_cec_isr, IRQF_SHARED, "RTK CEC", p_this) < 0) 
        {
            cec_warning("open rtk cec failed, unable to request irq#%d", cectx_irq_num);              			
            return -EIO;
        }
				
        p_this->status.init = 1;
    }
	
	//rtk_dump_cec_register("after");
				
    return 0;     
	
}



int rtk_cec_RPC_TOAGENT_PrivateInfo(char *str, unsigned long para) 
{	
#if 1	
	struct AUDIO_RPC_PRIVATEINFO_PARAMETERS *rpc = NULL;
    int ret = -1;
    unsigned long RPC_ret;
    dma_addr_t dat;	
    void *addr = 0;
	
	cec_info("pass %s =%4lx to fw",str, para);
    
    addr = dma_alloc_coherent(NULL, 4096, &dat, GFP_KERNEL);

    if(!addr)
    {
        cec_info("[%s %d fail]\n",__FUNCTION__, __LINE__);
        goto exit;
    }
    rpc = addr;
	
	rpc-> type = htonl(ENUM_PRIVATEINFO_AUDIO_SET_CEC_PARAMETERS);
	rpc-> privateInfo[0] = htonl(rtk_cec_standby_config);
	rpc-> privateInfo[1] = htonl(rtk_cec_standby_logical_addr);
	rpc-> privateInfo[2] = htonl(rtk_cec_standby_physical_addr);
	rpc-> privateInfo[3] = htonl(rtk_cec_standby_cec_version);
	rpc-> privateInfo[4] = htonl(rtk_cec_standby_vendor_id);
	rpc-> privateInfo[5] = htonl(rtk_cec_standby_rx_mask);

#ifdef AO_ON_SCPU
    printk(KERN_ALERT "#### hack @ %s %d\n", __FUNCTION__, __LINE__);
    rtk_ao_kernel_rpc(ENUM_KERNEL_RPC_PRIVATEINFO
        , (unsigned char *)rpc, sizeof(*rpc)
        , (unsigned char *)rpc + sizeof(*rpc), sizeof(long) * 17//sizeof(AUDIO_RPC_PRIVATEINFO_RETURNVAL)
        , (unsigned char *)&RPC_ret, sizeof(RPC_ret));
#else
    if (send_rpc_command(RPC_AUDIO, 
        ENUM_KERNEL_RPC_PRIVATEINFO,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU((dat + sizeof(struct AUDIO_RPC_PRIVATEINFO_PARAMETERS))),
        &RPC_ret)) 
    {
        cec_info("[%s %d RPC fail]\n",__FUNCTION__, __LINE__);
        goto exit;
    }
#endif

    ret = 0;
exit:
    if(addr)
        dma_free_coherent(NULL, 4096, addr, dat);
    return ret;
#endif
        return 0;	
}


/*------------------------------------------------------------------
 * Func : rtk_cec_set_mode
 *
 * Desc : enable ces module
 *
 * Parm : p_this   : handle of rtk cec adapter
 *        mode     : CEC_MODE_OFF : disable CEC module
 *                   CEC_MODE_ON : enable CEC module
 *                   CEC_MODE_OFF : put CEC module into standby mode
 *         
 * Retn : 0 for success, others failed
 *------------------------------------------------------------------*/
int rtk_cec_set_mode(rtk_cec* p_this, unsigned char mode)
{      
    switch(mode)
    {
    case CEC_MODE_OFF:        // disable
        
        cec_info("rtk cec mode : OFF");                                
        write_reg32(cectx_base + CEC_CR0, (read_reg32(cectx_base + CEC_CR0) & ~CTRL_MASK1) | CEC_MODE(0) | LOGICAL_ADDR(0xF));             
        p_this->status.enable = 0;  
        p_this->status.standby_mode = 0;                      
        rtk_cec_rx_stop(p_this);
        rtk_cec_tx_stop(p_this);
       
        break;
         
    case CEC_MODE_ON:

        cec_info("rtk cec mode : On");                                         
        write_reg32(cectx_base + CEC_CR0, (read_reg32(cectx_base + CEC_CR0) & ~CTRL_MASK1) | CEC_MODE(1));
        //write_reg32(cectx_base+ CEC_RXCR0,  RX_EN | RX_INT_EN); 
#ifdef CEC_OPCODE_COMPARE_ENABLE
			write_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE, 0); //disable opcode compare function
#endif				
        rtk_cec_rx_start(p_this);
        rtk_cec_tx_start(p_this);
        p_this->status.enable = 1;
        p_this->status.standby_mode = 0;
            
        break;        
        
    case CEC_MODE_STANDBY:
        
        cec_info("rtk cec mode : Standby (la=%x)", rtk_cec_standby_logical_addr);                                                  
        write_reg32(cectx_base + CEC_CR0, (read_reg32(cectx_base + CEC_CR0) & ~CTRL_MASK1) | 
                              CEC_MODE(1) | 
                              LOGICAL_ADDR(rtk_cec_standby_logical_addr));
        rtk_cec_rx_start(p_this);
        rtk_cec_tx_start(p_this);
        p_this->status.enable = 1;
        p_this->status.standby_mode = 1;                           
        break;
    }	 
    return 0;
}



/*------------------------------------------------------------------
 * Func : rtk_cec_enable
 *
 * Desc : enable ces module
 *
 * Parm : p_this   : handle of rtk cec adapter
 *        on_off   : 0 : disable, others enable
 *         
 * Retn : 0 for success, others failed
 *------------------------------------------------------------------*/
int rtk_cec_enable(rtk_cec* p_this, unsigned char on_off)
{    
    if (on_off)                
    {                          
        rtk_cec_set_mode(p_this, CEC_MODE_ON);   
    }
    else
    {
        if (rtk_cec_standby_config==0 || rtk_cec_standby_logical_addr>=0xF || rtk_cec_standby_physical_addr==0xFFFF)      
            rtk_cec_set_mode(p_this, CEC_MODE_OFF);
        else               
            rtk_cec_set_mode(p_this, CEC_MODE_STANDBY);        // standby mode
    }          
    
    return 0;
}



/*------------------------------------------------------------------
 * Func : rtk_cec_set_rx_mask
 *
 * Desc : set rx mask rtk ces
 *
 * Parm : p_this    : handle of rtk cec adapter
 *        rx_mask   : rx mask
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
int rtk_cec_set_rx_mask(
    rtk_cec*             p_this,
    unsigned short          rx_mask
    )
{         
    unsigned char log_addr = 0xf;
    int i;

    for (i=0; i<16; i++)
    {
        if (TEST_BIT(rx_mask, i))
        {
            log_addr = i;
            break;
        }
    }
    
    if (rx_mask & ~(BIT_MASK(15) | BIT_MASK(log_addr))) {     
        cec_info("multiple address specified (%04x), only address %x,f are valid", rx_mask, log_addr);           
    }
      
    write_reg32(cectx_base + CEC_CR0, (read_reg32(cectx_base + CEC_CR0) & ~LOGICAL_ADDR_MASK) | LOGICAL_ADDR(log_addr));
    
    cec_info("logical address = %02x", log_addr);  	

    if (log_addr != 0xF)
	{
        rtk_cec_standby_logical_addr = log_addr;         // save the latest vaild logical address
		rtk_cec_RPC_TOAGENT_PrivateInfo("logical address", rtk_cec_standby_logical_addr); 
	}
    
    return 0;            
}    



/*------------------------------------------------------------------
 * Func : rtk_cec_set_physical_addr
 *
 * Desc : set physical address of rtk ces
 *
 * Parm : p_this    : handle of rtk cec adapter
 *        phy_addr  : physical address  
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
int rtk_cec_set_physical_addr(
    rtk_cec*             p_this,
    unsigned short          phy_addr    
    )
{         
    cec_info("physcial address = %04x", phy_addr);    
    
    if (phy_addr!= 0xffff)
        rtk_cec_standby_physical_addr = phy_addr;      // we always keep latest valid physical address during standby
        
	
	rtk_cec_RPC_TOAGENT_PrivateInfo("physical addr",(unsigned long)phy_addr);
	
    return 0;            
}    



/*------------------------------------------------------------------
 * Func : rtk_cec_set_cec_version
 *
 * Desc : set cec version
 *
 * Parm : p_this    : handle of rtk cec adapter
 *        version   : cec version number
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
int rtk_cec_set_cec_version(rtk_cec* p_this, unsigned char version)
{      
    rtk_cec_standby_cec_version = version;
	rtk_cec_RPC_TOAGENT_PrivateInfo("cec version", (unsigned long)version);
    
	return 0; 
}    



/*------------------------------------------------------------------
 * Func : rtk_cec_set_device_vendor_id
 *
 * Desc : set vendor id of rtk ces
 *
 * Parm : p_this    : handle of rtk cec adapter
 *        vendor_id  : device vendor id
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
int rtk_cec_set_device_vendor_id(rtk_cec* p_this, unsigned long vendor_id)
{      
    rtk_cec_standby_vendor_id = vendor_id;
	rtk_cec_RPC_TOAGENT_PrivateInfo("vendor id",vendor_id);
   
   return 0;       
}    



/*------------------------------------------------------------------
 * Func : rtk_cec_set_stanby_mode
 *
 * Desc : set standy mode condition of rtk ces
 *
 * Parm : p_this  : handle of rtk cec adapter
 *        mode    : cec standby mode
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
int rtk_cec_set_stanby_mode(
    rtk_cec*             p_this,
    unsigned long           mode
    )
{         
    rtk_cec_standby_config = mode;
	rtk_cec_RPC_TOAGENT_PrivateInfo("standy mode",mode);
	
    return 0;            
}    

/*------------------------------------------------------------------
 * Func : rtk_cec_set_retry_num
 *
 * Desc : set retry times of rtk cec
 *
 * Parm : p_this  : handle of rtk cec adapter
 *        mun    : retry times
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
int rtk_cec_set_retry_num(
    rtk_cec*         	p_this,
    unsigned long        	num
    )
{         
  	write_reg32(cectx_base + CEC_RTCR0, ((read_reg32(cectx_base + CEC_RTCR0)&~RETRY_NO_MASK)|RETRY_NO(num)));  
    return 0;            
}    



/*------------------------------------------------------------------
 * Func : rtk_cec_xmit_msg
 *
 * Desc : xmit message
 *
 * Parm : p_this   : handle of rtk cec 
 *        cmb      : msg to xmit
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/
int rtk_cec_xmit_message(rtk_cec* p_this, cm_buff* cmb, unsigned long flags)
{    
    int ret = 0;
    
    if (!p_this->xmit.enable)
        return -1;
    
    cmb->flags  = flags;
    cmb->status = WAIT_XMIT;
    
    cmb_queue_tail(&p_this->tx_queue, cmb);
    
    schedule_delayed_work(&p_this->xmit.work, 0);
    
    if ((cmb->flags & NONBLOCK)==0)
    {                
        wait_event_timeout(cmb->wq, cmb->status!=WAIT_XMIT, TX_TIMEOUT + 50);
        
        switch(cmb->status)    
        {
        case XMIT_OK:
            cec_tx_dbg("[cec0]xmit message success\n");
            break;
                            
        case XMIT_ABORT:
            cec_warning("xmit message abort");
            break;        
        
        case XMIT_FAIL:
            cec_warning("xmit message failed");
            break;
        
        default:
        case XMIT_TIMEOUT:    
        case WAIT_XMIT:
            cec_warning("xmit message timeout");            
            break;
        }
            
        ret = (cmb->status==XMIT_OK) ? 0 : -1;
        kfree_cmb(cmb);
    }                    
    
    return ret;
}




/*------------------------------------------------------------------
 * Func : rtk_cec_read_message
 *
 * Desc : read message
 *
 * Parm : p_this : handle of cec device
 *        flags  : flag of current read operation
 * 
 * Retn : cec message
 *------------------------------------------------------------------*/
static cm_buff* rtk_cec_read_message(rtk_cec* p_this, unsigned char flags)
{                
    while(!(flags & NONBLOCK) && p_this->status.enable && !cmb_queue_len(&p_this->rx_queue))
        wait_event_interruptible(p_this->rcv.wq, !p_this->status.enable || cmb_queue_len(&p_this->rx_queue));

    if (p_this->status.enable)
    {
        cm_buff* cmb = cmb_dequeue(&p_this->rx_queue);
    
        if (cmb)
        {   
            cec_rx_dbg("[cec0]got msg %p\n", cmb);
            cmb_queue_tail(&p_this->rx_free_queue, alloc_cmb(RX_BUFFER_SIZE));
            return cmb;
        }        
    }
        
    return NULL;
}


/*------------------------------------------------------------------
 * Func : rtk_cec_uninit
 *
 * Desc : uninit a rtk cec adapter
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
void rtk_cec_uninit(rtk_cec* p_this)
{         
    cec_info("rtk cec closed");    
    free_irq(ISO_IRQ, p_this);
    write_reg32(cectx_base + CEC_CR0, read_reg32(cectx_base + CEC_CR0) & ~CEC_MODE_MASK);    
    write_reg32(cectx_base + CEC_RXCR0, 0);   
    write_reg32(cectx_base + CEC_TXCR0, 0);        
    rtk_cec_enable(p_this, 0);
    p_this->status.init = 0;
}    



/*------------------------------------------------------------------
 * Func : rtk_cec_suspend
 *
 * Desc : suspend a rtk cec adapter
 *
 * Parm : p_this : handle of rtk cec adapter
 * 
 * Retn : 0
 *------------------------------------------------------------------*/
int rtk_cec_suspend(rtk_cec* p_this)
{    
	
	if(!p_this->status.enable) //CEC is off 
	{
		printk("[CEC0]function is off when power on, doesn't enable!!\n");
		return 0;
	}
	
    cec_info("rtk cec suspended (la=%x, pa=%04x, standby_mode=%ld)",
            rtk_cec_standby_logical_addr, 
            rtk_cec_standby_physical_addr, 
            rtk_cec_standby_config);
    
    rtk_cec_set_mode(p_this, CEC_MODE_OFF); 

	disable_irq(cectx_irq_num);

    if (rtk_cec_standby_config && rtk_cec_standby_logical_addr<0xF && rtk_cec_standby_physical_addr!=0xFFFF)
    {
        p_this->status.enable = 1;
        p_this->status.standby_mode = 1;   

         write_reg32(cectx_base + CEC_CR0,   (read_reg32(cectx_base + CEC_CR0) & ~(LOGICAL_ADDR(0xF))) | CEC_MODE(1) | LOGICAL_ADDR(rtk_cec_standby_logical_addr));                                                                               

         write_reg32(cectx_base + CEC_RXCR0,  RX_RST); 
        
        write_reg32(cectx_base + CEC_RXCR0,  0);         
        
        write_reg32(cectx_base + CEC_TXCR0,  TX_RST);        
        
        write_reg32(cectx_base + CEC_TXCR0,  0);         
       
#ifdef CEC_OPCODE_COMPARE_ENABLE
		write_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE, 0); //disable

		//-- compare 1
		if(rtk_cec_standby_config&STANBY_RESPONSE_GIVE_POWER_STATUS) 
		{
			write_reg32(cectx_base + GDI_CEC_COMPARE_OPCODE_01, CEC_MSG_GIVE_DEVICE_POWER_STATUS); 
			write_reg32(cectx_base + GDI_CEC_SEND_OPCODE_01, CEC_MSG_REPORT_POWER_STATUS);  	
			write_reg32(cectx_base + GDI_CEC_SEND_OPERAND_NUMBER_01, 1); 								
			write_reg32(cectx_base + GDI_CEC_OPERAND_01, CEC_POWER_STATUS_STANDBY);		
			write_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE, (read_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE)|0x01)); 
		}
		//-- compare 2
		if(rtk_cec_standby_config&STANBY_RESPONSE_GIVE_PHYSICAL_ADDR) 
		{
			write_reg32(cectx_base + GDI_CEC_COMPARE_OPCODE_02, CEC_MSG_GIVE_PHYSICAL_ADDRESS); 
			write_reg32(cectx_base + GDI_CEC_SEND_OPCODE_02, CEC_MSG_REPORT_PHYSICAL_ADDRESS);	
			write_reg32(cectx_base + GDI_CEC_SEND_OPERAND_NUMBER_02, 2);									
			write_reg32(cectx_base + GDI_CEC_OPERAND_04,(rtk_cec_standby_physical_addr >> 8) & 0xFF);	
			write_reg32(cectx_base + GDI_CEC_OPERAND_05,(rtk_cec_standby_physical_addr) & 0xFF);				
			//write_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE, (read_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE)|0x02)); //invalid, disable temporarily
		}
		//-- compare 3	
		if(rtk_cec_standby_config&STANBY_RESPONSE_GET_CEC_VERISON)
		{
			write_reg32(cectx_base + GDI_CEC_COMPARE_OPCODE_03, CEC_MSG_GET_CEC_VERSION); 
			write_reg32(cectx_base + GDI_CEC_SEND_OPCODE_03 , CEC_MSG_CEC_VERSION);				
			write_reg32(cectx_base + GDI_CEC_SEND_OPERAND_NUMBER_03, 1);									
			write_reg32(cectx_base + GDI_CEC_OPERAND_07, rtk_cec_standby_cec_version);	
			write_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE, (read_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE)|0x04)); 
 		}
		//-- compare 4
		if(rtk_cec_standby_config&STANBY_RESPONSE_GIVE_DEVICE_VENDOR_ID) 	
		{
			write_reg32(cectx_base + GDI_CEC_COMPARE_OPCODE_04, CEC_MSG_GIVE_DEVICE_VENDOR_ID); 
			write_reg32(cectx_base + GDI_CEC_SEND_OPCODE_04, CEC_MSG_DEVICE_VENDOR_ID);			
			write_reg32(cectx_base + GDI_CEC_SEND_OPERAND_NUMBER_04, 3);									
			write_reg32(cectx_base + GDI_CEC_OPERAND_10,(rtk_cec_standby_vendor_id >> 16) & 0xFF);	
			write_reg32(cectx_base + GDI_CEC_OPERAND_11,(rtk_cec_standby_vendor_id >> 8) & 0xFF);				
			write_reg32(cectx_base + GDI_CEC_OPERAND_12,(rtk_cec_standby_vendor_id) & 0xFF);			
			write_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE, (read_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE)|0x08)); 
		}
		printk("CEC_COMP_ENABLE = %08x\n", read_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE));
#endif //ifdef CEC_OPCODE_COMPARE_ENABLE  
        write_reg32(cectx_base + CEC_RXCR0,  RX_EN);    // enable RX
       
        cec_info("cec standby enabled");
    }        

    return 0;
}


/*------------------------------------------------------------------
 * Func : rtk_cec_resume
 *
 * Desc : suspend a rtk cec adapter
 *
 * Parm : p_this : handle of rtk cec adapter
 * 
 * Retn : 0
 *------------------------------------------------------------------*/
int rtk_cec_resume(rtk_cec* p_this)
{
	if(!p_this->status.enable) //CEC is off
		return 0;

    cec_info("rtk cec resume");     

	if (read_reg32(cectx_base + CEC_TXCR0) & TX_INT)
		write_reg32(cectx_base + CEC_TXCR0, read_reg32(cectx_base + CEC_TXCR0));// clear tx interrupt


    SETBITS(cectx_base, RTK_CEC_ANALOG, CEC_ANALOG_REG_CEC_RPU_EN_MASK); //0xfe0075E0: enable pull up resistor.

    write_reg32(cectx_base+ CEC_TXTCR1, TX_DATA_LOW(0x1A) | TX_DATA_01(0x23) | TX_DATA_HIGH(0x22));

    write_reg32(cectx_base+ CEC_TXTCR0, TX_START_LOW(0x93) | TX_START_HIGH(0x20));

    write_reg32(cectx_base+ CEC_RXTCR0, RX_START_LOW(0x8C) | RX_START_PERIOD(0xC1) |
										RX_DATA_SAMPLE(0x2A) | RX_DATA_PERIOD(0x52));

    write_reg32(cectx_base+ CEC_CR0 , CEC_MODE(1) | LOGICAL_ADDR(0x4) | TIMER_DIV(0x14) |
            PRE_DIV(0x21) | UNREG_ACK_EN);

    MASK_REG_32(cectx_base,CEC_RTCR0, ~RETRY_NO(5), RETRY_NO(2));

    /* Reset Rx */
    write_reg32(cectx_base+ CEC_RXCR0,  RX_RST);
    write_reg32(cectx_base+ CEC_RXCR0,  0);

    /* Reset Tx */
    write_reg32(cectx_base+ CEC_TXCR0,  TX_RST);
    write_reg32(cectx_base+ CEC_TXCR0,  0);

    if (rtk_cec_standby_config && rtk_cec_standby_logical_addr<0xF && rtk_cec_standby_physical_addr!=0xFFFF)
        rtk_cec_set_mode(p_this, CEC_MODE_ON);
    else
        rtk_cec_set_mode(p_this, CEC_MODE_OFF);
	
    enable_irq(cectx_irq_num);
		    
    return 0;
}



/*------------------------------------------------------------------
 * Func : pm_rtk_cec_suspend
 *
 * Desc : Force rtk cec to enter suspend state.
 *        this function will be called before enter 8051.
 *
 * Parm : None
 * 
 * Retn : 0
 *------------------------------------------------------------------*/
int pm_rtk_cec_suspend(void)
{    
    return rtk_cec_suspend(&m_cec);
}

/*------------------------------------------------------------------
 * Func : rtk_cec_early_enable
 *
 * Desc : Force darwin cec to enable when the system is cold boot.
 *        this function will be called by 8051
 *
 * Parm : None
 * 
 * Retn : None
 *------------------------------------------------------------------*/
 void rtk_cec_early_enable(void)
{    
	//general setup
	write_reg32(cectx_base + CEC_CR0,	CEC_MODE(1) | TEST_MODE_PAD_EN | 
							LOGICAL_ADDR(0) | TIMER_DIV(20) | 
							PRE_DIV(202) | UNREG_ACK_EN);	
	//reset	engine												   
	write_reg32(cectx_base + CEC_RTCR0,	RETRY_NO(5)); 
	write_reg32(cectx_base + CEC_RXCR0,	RX_RST); 
	write_reg32(cectx_base + CEC_RXCR0,	0); 		
	write_reg32(cectx_base + CEC_TXCR0,	TX_RST);		
	write_reg32(cectx_base + CEC_TXCR0,	0); 		

	//timing setup
	write_reg32(cectx_base + CEC_RXTCR0, RX_START_LOW(0x8C) | RX_START_PERIOD(0xC1) | 
										RX_DATA_SAMPLE(0x2A) | RX_DATA_PERIOD(0x52));
	write_reg32(cectx_base + CEC_TXTCR0, TX_START_LOW(148) | TX_START_HIGH(32)); 					  
	write_reg32(cectx_base + CEC_TXTCR1, TX_DATA_LOW(24) | TX_DATA_01(36) | TX_DATA_HIGH(36));

	//enable Rx
	write_reg32(cectx_base + CEC_RXCR0,  RX_EN);	// enable RX
}



////////////////////////////////////////////////////////////////////////
// CEC Driver Interface                                               //
////////////////////////////////////////////////////////////////////////


int cec_tx_device_check(void)
{
    struct device_node __maybe_unused *cectx_node;
    unsigned int module_en = 0;
    int ret = 0;
    
    printk("[cec_tx_device_check]\n");
    cectx_node = of_find_matching_node(NULL, cectx_ids);
    if (!cectx_node)
        printk("no cectx_node\n");	
    else
        printk("has cectx_node\n");
    of_property_read_u32(cectx_node, "module-enable", &(module_en));   
    if(module_en)
    {
        ret = 1;
        printk("TX en\n");
    }
    else
    {
        printk("TX disable\n");
    }
    return ret;             
}

static int ops_probe(cec_device* dev)
{   
	struct device_node __maybe_unused *cectx_node;
	unsigned int module_en = 0;

	cectx_node = of_find_matching_node(NULL, cectx_ids);
	
	if (!cectx_node)
		cec_info("[%s] %s  %d no matching node", __FILE__,__FUNCTION__,__LINE__);	 
		
		
    if (dev->id!=ID_RTK_CEC_CONTROLLER)
    {
        cec_device_en[0] = 0;
        return -ENODEV;     
    }	
	
	/*Check module-enable*/
	of_property_read_u32(cectx_node, "module-enable", &(module_en));
	
	if(!module_en)
	    return -ENODEV; 	
	cec_device_en[0] = 1;

	cectx_base = of_iomap(cectx_node , 0);	

	//cectx_iso_base = of_iomap(cectx_node , 1);	
		
	cectx_irq_reg = of_iomap(cectx_node , 1);		

	printk("[In probe ,BASE: %x,%x]\n",(int)cectx_base,(int)cectx_irq_reg);

	/* Request IRQ */
	cectx_irq_num = irq_of_parse_and_map(cectx_node, 0);
	
	if (!cectx_irq_num) {
		printk(KERN_ALERT "CEC Tx: fail to parse of irq.\n");
		return -ENXIO;
	}
	cec_info("[%s] %s  %d irq number = %d", __FILE__,__FUNCTION__,__LINE__,cectx_irq_num);	 
	
	/* Hw init */
    if (rtk_cec_init(&m_cec)==0)    
        cec_set_drvdata(dev, &m_cec);
      
    return 0; 
}


static void ops_remove(cec_device* dev)
{        
    rtk_cec_uninit((rtk_cec*) cec_get_drvdata(dev));    
}


static int ops_enable(cec_device* dev, unsigned char on_off)
{    

	printk("ops_enable : %02x\n", on_off);
	if((on_off&0xf0) == 0)
	{
		if(on_off&0x0f) //on
		{
			rtk_cec_standby_config		 	= (STANBY_RESPONSE_GIVE_POWER_STATUS |STANBY_RESPONSE_GET_CEC_VERISON);
			rtk_cec_standby_logical_addr  	= 0x0;
			rtk_cec_standby_physical_addr 	= 0x0000; 
		}
		else{
			rtk_cec_standby_config			= 0;
			rtk_cec_standby_logical_addr 	= 0xF;
			rtk_cec_standby_physical_addr	= 0xFFFF;
		}	
	}
    return rtk_cec_enable((rtk_cec*) cec_get_drvdata(dev), (on_off&0x0f));        
}


static int ops_set_rx_mask(cec_device* dev, unsigned short rx_mask)
{      
    return rtk_cec_set_rx_mask((rtk_cec*) cec_get_drvdata(dev), rx_mask);       
}    


static int ops_set_physical_addr(cec_device* dev, unsigned short phy_addr)
{      
    return rtk_cec_set_physical_addr((rtk_cec*) cec_get_drvdata(dev), phy_addr);       
}    


static int ops_set_cev_version(cec_device* dev, unsigned char version)
{      
    return rtk_cec_set_cec_version((rtk_cec*) cec_get_drvdata(dev), version);       
}    


static int ops_set_device_vendor_id(cec_device* dev, unsigned long vendor_id)
{      
    return rtk_cec_set_device_vendor_id((rtk_cec*) cec_get_drvdata(dev), vendor_id);       
}    


static int ops_xmit(cec_device* dev, cm_buff* cmb, unsigned long flags)
{
    return rtk_cec_xmit_message((rtk_cec*) cec_get_drvdata(dev), cmb, flags);        
}


static cm_buff* ops_read(cec_device* dev, unsigned long flags)
{
    return rtk_cec_read_message((rtk_cec*) cec_get_drvdata(dev), flags);     
}


static int ops_set_stanby_mode(cec_device* dev, unsigned long flags)
{       
    return rtk_cec_set_stanby_mode((rtk_cec*) cec_get_drvdata(dev), flags);    
}

static int ops_set_retry_num(cec_device* dev, unsigned long num)
{       
    return rtk_cec_set_retry_num((rtk_cec*) cec_get_drvdata(dev), num);    
}

static int ops_suspend(cec_device* dev)
{       
    printk("[cec_0_ops_suspend]\n");
    return rtk_cec_suspend((rtk_cec*) cec_get_drvdata(dev));    
}


static int ops_resume(cec_device* dev)
{    
    return rtk_cec_resume((rtk_cec*) cec_get_drvdata(dev));
}



static cec_device rtk_cec_controller = 
{
    .id   = ID_RTK_CEC_CONTROLLER,
    .name = "rtk_cec0"    
};




static cec_driver rtk_cec_driver = 
{    
    .name                 = "cec0", // "rtk_cec0"
    .probe                = ops_probe,
    .remove               = ops_remove,        
    .suspend              = ops_suspend,
    .resume               = ops_resume,
    .enable               = ops_enable,
    .xmit                 = ops_xmit,
    .read                 = ops_read,
    .set_rx_mask          = ops_set_rx_mask,    
    .set_physical_addr    = ops_set_physical_addr,
    .set_cec_version      = ops_set_cev_version,
    .set_device_vendor_id = ops_set_device_vendor_id,
    .set_stanby_mode      = ops_set_stanby_mode,
    .set_retry_num			= ops_set_retry_num,
};



/*------------------------------------------------------------------
 * Func : rtk_cec_module_init
 *
 * Desc : rtk cec module init function
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static int __init rtk_cec_module_init(void)
{                                                        
    cec_info("%s cec module init", CEC_MODEL_NAME);                                                                                              

    if (register_cec_driver(&rtk_cec_driver)!=0)
        return -EFAULT;                        
        
    register_cec_device(&rtk_cec_controller);          // register cec device

	
    return 0;
}



/*------------------------------------------------------------------
 * Func : rtk_cec_module_exit
 *
 * Desc : rtk cec module exit function
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static void __exit rtk_cec_module_exit(void)
{
    unregister_cec_driver(&rtk_cec_driver);
}


module_init(rtk_cec_module_init);
module_exit(rtk_cec_module_exit);
