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
rtk_cec* irene_p_this;
static rtk_cec m_cec;
static void __iomem *cectx_base;
static void __iomem *cectx_iso_base;
static void __iomem *cectx_irq_reg;
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

static unsigned long  rtk_cec_standby_config        	= 0;
static unsigned char  rtk_cec_standby_logical_addr  = 0xF;
static unsigned short rtk_cec_standby_physical_addr	= 0xFFFF;
static unsigned char  rtk_cec_standby_cec_version   	= CEC_VERSION_1_4;    
static unsigned long  rtk_cec_standby_vendor_id     	= 0x000000;       

#define IRENE_CEC0_THREAD_ISR_TEST
#ifdef IRENE_CEC0_THREAD_ISR_TEST
//static int ops_enable(cec_device* dev, unsigned char on_off);
static struct task_struct *cec_hw_tsk = NULL;
static int cec_hw_data;
int rtk_cec_set_mode(rtk_cec* p_this, unsigned char mode);
int rtk_cec_set_physical_addr(
    rtk_cec*             p_this,
    unsigned short          phy_addr    
    );
int rtk_cec_set_rx_mask(
    rtk_cec*             p_this,
    unsigned short          rx_mask
    );    
#else
static unsigned int cectx_irq_num;
#endif
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
    
    cec_rx_dbg("_read_rx_fifo : rx data len = %d\n", len);
    
    if (cmb_tailroom(cmb) >= len)
    {        
        while(len)
        {            
            unsigned char rx_len = len;
            
            if (rx_len > 16)
                rx_len = 16;
                
            for (i=0; i<rx_len; i+=4)
                rx_fifo_data[i>>2] = __cpu_to_be32(read_reg32(cectx_base + CEC_RXDR1 + i));
                
            cec_rx_dbg("cmb->tail=%p, rx_fifo_data=%p, rx_len=%d\n", cmb->tail, rx_fifo_data, rx_len);                
               
            memcpy(cmb->tail, (unsigned char*)rx_fifo_data, rx_len);                    
            
            cmb_put(cmb, rx_len);    
                           
            write_reg32(cectx_base + CEC_TXDR0, RX_SUB_CNT | FIFO_CNT(rx_len));  // fetch next rx data                           
            
            len -= rx_len;
        }
        
        return cmb->len;                
    }
    else
    {
        cec_warning("no more space to read data\n");
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
        
        //cec_tx_dbg("in _write_tx_fifo : tx data len = %d\n", tx_len);       
        printk("in _write_tx_fifo : tx data len = %d\n", tx_len);         
        
        memcpy(tx_fifo_data, cmb->data, tx_len);
        cmb_pull(cmb, tx_len);                
                    
        for (i=0; i<tx_len; i+=4)    
            write_reg32(cectx_base + CEC_TXDR1 + i, __cpu_to_be32(tx_fifo_data[i>>2]));    
         
        write_reg32(cectx_base + CEC_TXDR0, TX_ADD_CNT | FIFO_CNT(tx_len));  // add data to fifo
        
        printk("[in _write_tx_fifo 750C=%x,7510=%x,7514=%x]\n",read_reg32(0xfe00750c),read_reg32(0xfe007510),read_reg32(0xfe007514));
        //printk("[7500=%x,7504=%x,7508=%x]\n",read_reg32(0xfe007500),read_reg32(0xfe007504),read_reg32(0xfe007508));
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
              
    printk("[p_xmit->state=%d,p_xmit->enable=%d]\n",p_xmit->state,p_xmit->enable);                  
    if (p_xmit->state == XMIT)
    {
        printk("1.XMIT:len = %d\n",p_xmit->cmb->len);   
        if (read_reg32(cectx_base + CEC_TXCR0) & TX_EN)
        {                                                   
            // xmitting
            printk("2.XMIT:len = %d\n",p_xmit->cmb->len); 
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
                
                cec_warning("cec tx timeout\n");
                
                cancel_delayed_work(&p_xmit->work);
            }                     
        }            
        else
        {
            cancel_delayed_work(&p_xmit->work);
            
            if ((read_reg32(cectx_base + CEC_TXCR0) & TX_EOM)==0)
            {
                p_xmit->cmb->status = XMIT_FAIL;                
                cec_warning("cec tx failed\n");
            }
            else
            {
                p_xmit->cmb->status = XMIT_OK;
                cec_tx_dbg("cec tx completed\n");
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
                cec_tx_dbg("cec tx : cmb = %p, len=%d\n", p_xmit->cmb, p_xmit->cmb->len);
                dest = (p_xmit->cmb->data[0] & 0xf);    
                //printk("dest = %d\n",dest);                            
                cmb_pull(p_xmit->cmb, 1);          
                p_xmit->timeout = jiffies + TX_TIMEOUT;

                // reset tx fifo
//                write_reg32(cectx_base + CEC_TXCR0, TX_RST);
//                wmb();
//                write_reg32(cectx_base + CEC_TXCR0, 0);                                    
//                wmb();     
                
                cec_reset_tx();                           
                //printk("IDEL:len = %d,timeout=%d,reg=%x\n",p_xmit->cmb->len,p_xmit->timeout,(cectx_base + CEC_TXCR0));                                                                                      
                _write_tx_fifo(p_xmit->cmb);                
                                                   
                if (p_xmit->cmb->len==0)
                {
                    write_reg32(cectx_base + CEC_TXCR0, TX_ADDR_EN | TX_ADDR(4) | DEST_ADDR(dest) | TX_EN | TX_INT_EN);
                    //printk("write 0 :%x,%x\n",(volatile unsigned int*)(cectx_base + CEC_TXCR0),(volatile unsigned int*)cectx_base + CEC_TXCR0);
                    //printk("750C=%x\n",read_reg32(0xfe00750c));
                    //write_reg32((cectx_base + CEC_TXCR0), (TX_ADDR_EN | TX_ADDR(4) | DEST_ADDR(dest) | TX_EN | TX_INT_EN));
                    //printk("write 1 :%x,%x\n",(cectx_base + CEC_TXCR0),(TX_ADDR(4) | DEST_ADDR(dest) | TX_EN | TX_INT_EN));
                }
                else
                {
                    write_reg32(cectx_base + CEC_TXCR0, TX_ADDR(4) | DEST_ADDR(dest) | TX_EN | TX_INT_EN | TX_CONTINUOUS);
                //    printk("write 2 :%x,%x\n",(cectx_base + CEC_TXCR0),(TX_ADDR(4) | DEST_ADDR(dest) | TX_EN | TX_INT_EN | TX_CONTINUOUS));
                    
                }
                             
                p_xmit->state = XMIT;
                
                cec_tx_dbg("cec txx start\n");
                printk("[_write_tx_fifo 750C=%x,7510=%x,7514=%x]\n",read_reg32(0xfe00750c),read_reg32(0xfe007510),read_reg32(0xfe007514));
//        write_reg32(0xfe00753C, 0x001A2322);//0x00192323);
//        write_reg32(0xfe007538, 0x00009320);//0x00009320);
//        write_reg32(0xfe007534, 0x8cBc2b56);
//        write_reg32(0xfe007500, 0x44142180);//0x41050580);
//        write_reg32(0xfe007504, 0x00000002);
//        write_reg32(0xfe007508, 0x00009000);
//        write_reg32(0xfe00750c, 0x0014b000);
                printk("[_write_tx_fifo 750C=%x,7510=%x,7514=%x]\n",read_reg32(0xfe00750c),read_reg32(0xfe007510),read_reg32(0xfe007514));
        cec_wait_tx_int();
        
        cec_check_rd_reg_jne(0xfe00750C, 0x000000C0,0x000000C0);
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
        cec_info("cec tx start\n");
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
        cec_info("cec tx stop\n");
            
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
    
    //cec_info("rtk_cec_standby_message_handler : init = %x, dest=%x, opcode=%x, len=%d\n", init, dest, opcode, len);   
    
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
    printk("[p_rcv->state=%d]\n",p_rcv->state);          
    if (!p_rcv->enable)
    {
        if (p_rcv->state==RCV)
        {
            cec_rx_dbg("force stop\n");            
            write_reg32(cectx_base + CEC_RXCR0, 0);
            kfree_cmb(p_rcv->cmb);
            p_rcv->cmb   = NULL;
            p_rcv->state = IDEL;
            
            // reset cec0
            write_reg32(cectx_base + CEC_RXCR0, RX_RST);                
            wmb();
            write_reg32(cectx_base + CEC_RXCR0, 0);        
            wmb();        
            printk("[4.p_rcv->state=%d]\n",p_rcv->state);    
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
                    cec_rx_dbg("read rx fifo failed, return to rx\n");                    
                    write_reg32(cectx_base + CEC_RXCR0, 0);
                    p_rcv->state = IDEL;
                    printk("[3.p_rcv->state=%d]\n",p_rcv->state);    
                }                            
                 
                write_reg32(cectx_base + CEC_RXCR0, RX_INT);
            }            
        }
        else
        {               
            cec_rx_dbg("rx_stop (0x%08x)\n", read_reg32(cectx_base + CEC_RXCR0));
            
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
            printk("[2.p_rcv->state=%d]\n",p_rcv->state);                
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
                cec_warning("[CEC] WARNING, rx over run\n");
                p_rcv->cmb = cmb_dequeue(&p_this->rx_queue);        // reclaim form rx fifo
            }
                
            if (p_rcv->cmb==NULL)
            {
                cec_warning("[CEC] FATAL, something wrong, no rx buffer left\n");
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
        cec_rx_dbg("rx_restart (0x%08x)\n", read_reg32(cectx_base + CEC_RXCR0));                        
        p_rcv->state = RCV;   
        printk("[1.p_rcv->state=%d]\n",p_rcv->state);         
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
        cec_info("rx start\n");
        
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
        cec_info("rx stop\n");        
        
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
#define CEC_0_DRIVER
//#define CEC_1_DRIVER

void cec_reset_rx(void)
{
#ifdef CEC_0_DRIVER
    write_reg32(cectx_base + RTK_CEC_RXCR0, 0x4040);// 0xfe007508
    write_reg32(cectx_base + RTK_CEC_RXCR0, 0x0);
#endif    
#ifdef CEC_1_DRIVER    
    write_reg32(0xfe03720c, 0x4040);
    write_reg32(0xfe03720c, 0x0);
#endif
}

void cec_reset_tx(void)
{
#ifdef CEC_0_DRIVER    
    write_reg32(cectx_base+ RTK_CEC_TXCR0, 0x4040); // 0xfe00750c
    write_reg32(cectx_base+ RTK_CEC_TXCR0, 0x0);
#endif
#ifdef CEC_1_DRIVER    
    write_reg32(0xfe03720c, 0x4040);
    write_reg32(0xfe03720c, 0x0);
#endif
}

void cec_reset_tx_rx(void)
{
    cec_reset_tx();
    cec_reset_rx();
}

void cec_wait_tx_int(void)
{
    int tmp_value;
    
    return;
    printk("[in while loop]\n");
    while(1)
    {
        static int tmp_count = 0;
        
#ifdef CEC_0_DRIVER
        tmp_value = read_reg32(0xfe00750C);
#endif
#ifdef CEC_1_DRIVER
        tmp_value = read_reg32(0xfe03720c);
#endif        
        if((tmp_value & 0x40))
        {
#ifdef CEC_0_DRIVER
            printk("[wait tx success~~750C=%x,7504=%x]\n",tmp_value,read_reg32(0xfe007504)); 
#endif
#ifdef CEC_1_DRIVER
            printk("[wait tx success~~720C=%x,7204=%x]\n",tmp_value,read_reg32(0xfe037204)); 
#endif
            break;
        }
        tmp_count++;
        if(tmp_count > 20)
        {
#ifdef CEC_0_DRIVER
            printk("[wait tx~~750C=%x,7504=%x]\n",tmp_value,read_reg32(0xfe007504)); 
#endif
#ifdef CEC_1_DRIVER
            printk("[wait tx~~720C=%x,7204=%x]\n",tmp_value,read_reg32(0xfe037204)); 
#endif
            tmp_count = 0;
        }
    }
    printk("[leave while loop]\n");
}

void cec_wait_rx_int(void)
{
    int tmp_value;
    
    return;
    printk("[in while loop]\n");
    while(1)
    {
        static int tmp_count = 0;
        
#ifdef CEC_0_DRIVER
        tmp_value = read_reg32(0xfe007508);
#endif
#ifdef CEC_1_DRIVER
        tmp_value = read_reg32(0xfe037208);
#endif        
        if((tmp_value & 0x40))
        {
#ifdef CEC_0_DRIVER
            printk("[wait rx success~~7508=%x,7504=%x]\n",tmp_value,read_reg32(0xfe007504)); 
#endif
#ifdef CEC_1_DRIVER
            printk("[wait rx success~~7208=%x,7204=%x]\n",tmp_value,read_reg32(0xfe037204)); 
#endif
            break;
        }
        tmp_count++;
        if(tmp_count > 20)
        {
#ifdef CEC_0_DRIVER
            printk("[wait rx~~7508=%x,7504=%x]\n",tmp_value,read_reg32(0xfe007504)); 
#endif
#ifdef CEC_1_DRIVER
            printk("[wait rx~~7208=%x,7204=%x]\n",tmp_value,read_reg32(0xfe037204)); 
#endif
            tmp_count = 0;
        }
    }
    printk("[leave while loop]\n");
}

void cec_check_rd_reg_jne(int reg_addr, int mask_value, int want_value)
{
    int tmp_value = read_reg32(reg_addr); // CEC_TxCR0
    return;
    if((tmp_value & mask_value) != want_value)
    {
#ifdef CEC_0_DRIVER 
        printk("[EOM CHECK fail~~ 750C=%x,7504=%x]\n",tmp_value,read_reg32(0xfe007504)); 
#endif
#ifdef CEC_1_DRIVER
        printk("[EOM CHECK fail~~ 720c=%x,7204=%x]\n",tmp_value,read_reg32(0xfe037204)); 
#endif        
    }
}

#ifdef IRENE_CEC0_THREAD_ISR_TEST 
void init_cec_start(void)
#if 0
{
    printk("[in init_cec_start 1]\n");
    write_reg32(0xfe007088, 0xFFFFFFFF);
    write_reg32(0xfe00708C, 0xFFFFFFFF);
    printk("[in init_cec_start 2]\n");
    // for DD test
    //write_reg32(0xfe007024, 0x88889);
    //write_reg32(0xfe000404, 0x4208);
    //write_reg32(0xfe037500, 0x80000);
    //write_reg32(0xfe000404, 0x48E3);
    // for DD end
    //cec_test_prologue
    {
        //cec_pad_en();
        cec_reset_tx_rx();
        write_reg32(0xfe0075E0, 0x0000000c);
#ifdef CEC_0_DRIVER
        write_reg32(0xfe00753C, 0x001A2322);//0x00192323);
        write_reg32(0xfe007538, 0x00009320);//0x00009320);
        write_reg32(0xfe007534, 0x8cBc2b56);
        write_reg32(0xfe007500, 0x44142180);//0x41050580);
        write_reg32(0xfe007504, 0x00000002);
        write_reg32(0xfe007508, 0x00009000);
        write_reg32(0xfe00750c, 0x0014b000);
        cec_wait_tx_int();
        
        cec_check_rd_reg_jne(0xfe00750C, 0x000000C0,0x000000C0);
#endif
#ifdef CEC_1_DRIVER
        write_reg32(0xfe03723C, 0x001A2322);
        write_reg32(0xfe037238, 0x00009320);
        write_reg32(0xfe037234, 0x8cBc2B56);
        write_reg32(0xfe037200, 0x44B92180);
        write_reg32(0xfe037204, 0x00030002);
        write_reg32(0xfe037208, 0x00009000);
        write_reg32(0xfe03720c, 0x0014b000);
        cec_wait_tx_int();
        
        cec_check_rd_reg_jne(0xfe03720c, 0x000000C0,0x000000C0);
#endif
    }
    printk("[in init_cec_start 3]\n");
#ifdef CEC_0_DRIVER  
    cec_reset_tx_rx();
    // send message with dest_addr == 0x0 src== 4
    write_reg32(0xfe007514, 0x91000000); // CEC_TxDR1
    write_reg32(0xfe007510, 0x00000041);
    write_reg32(0xfe00750C, 0x00149000); // CEC_TxCR0
    cec_wait_tx_int();
    cec_wait_rx_int();
    printk("[in init_cec_start 4]\n");
    // sucessfully sent the eom
    cec_check_rd_reg_jne(0xfe00750C, 0x000000C0,0x000000C0);
    printk("[in init_cec_start 5]\n"); 
#endif 
#ifdef CEC_1_DRIVER  
    cec_reset_tx_rx();
    write_reg32(0xfe037214, 0x91000000); // CEC_TxDR1
    write_reg32(0xfe037210, 0x00000041);
    write_reg32(0xfe03720c, 0x00149000); // CEC_TxCR0
    
    cec_wait_tx_int();
    cec_wait_rx_int();
    cec_check_rd_reg_jne(0xfe037208, 0x000000C0,0x000000C0);
    printk("[in init_cec_start 6]\n");
#endif    
}
#endif
#if 1
{
    cm_buff* cmb = NULL;
    
    //ops_enable();
    rtk_cec_standby_config		 	= (STANBY_RESPONSE_GIVE_POWER_STATUS |STANBY_RESPONSE_GET_CEC_VERISON);
    rtk_cec_standby_logical_addr  	= 0x0;
    rtk_cec_standby_physical_addr 	= 0x0000; 
    
    irene_p_this->rcv.enable = 0;
    irene_p_this->xmit.enable = 0;
    rtk_cec_set_mode(irene_p_this, CEC_MODE_ON);   
    
    rtk_cec_set_physical_addr(irene_p_this, 0x1000);
    rtk_cec_set_rx_mask(irene_p_this, 0x10);
    
    printk("[in init_cec_start 1]\n");  
    
    //send polling message
    if ((cmb=alloc_cmb(1)))
    {                
        cmb->data[0] = (4<<4) | 0; // from 4(playback device) to 0(TV)
        cmb_put(cmb,1);
    }
    if (cmb)
        rtk_cec_xmit_message(irene_p_this, cmb, NONBLOCK);
    
    // send get menu language    
    cmb = NULL;
    if ((cmb=alloc_cmb(2)))
    {                
        cmb->data[0] = (4<<4) | 0; // from 4(playback device) to 0(TV)
        cmb->data[1] = 0x91;  
        cmb_put(cmb,2);
    } 
    if (cmb)
        rtk_cec_xmit_message(irene_p_this, cmb, NONBLOCK);   
} 
#endif
#endif //IRENE_CEC0_THREAD_ISR_TEST

#ifndef IRENE_CEC0_THREAD_ISR_TEST
static 
irqreturn_t rtk_cec_isr(
    int                     this_irq, 
    void*                   dev_id    
    )
{            
    rtk_cec* p_this = (rtk_cec*) dev_id;
    irqreturn_t ret = IRQ_NONE;            
#else
void rtk_cec_isr(void)
{
    rtk_cec* p_this = (rtk_cec*) irene_p_this;
#endif
    printk("cec0 INT !!\n");
#ifdef IRENE_CEC0_THREAD_ISR_TEST
    printk("In rtk_cec_isr \n");
    init_cec_start();
    for(;;) {
        if (kthread_should_stop()) break;
#endif    
    if (read_reg32(cectx_base + CEC_TXCR0) & TX_INT) {
		printk("cec TX_INT \n!!");
        rtk_cec_tx_work(&p_this->xmit.work.work);    
#ifndef IRENE_CEC0_THREAD_ISR_TEST
        ret = IRQ_HANDLED;
#endif        
    }
    
    if (read_reg32(cectx_base + CEC_RXCR0) & RX_INT) { 
		printk("cec RX_INT \n!!");
        rtk_cec_rx_work(&p_this->rcv.work.work);
#ifndef IRENE_CEC0_THREAD_ISR_TEST        
        ret = IRQ_HANDLED;
#endif        
    }
#ifdef IRENE_CEC0_THREAD_ISR_TEST    
    }
#else    
    return ret;    
#endif                      
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
    //int fout,pllbus_n,pllbus_m,pllbus_o,
	int cec_pre_div = 0x21;
    //pllbus_m = ((read_reg32(SYS_PLL_BUS1_reg) & SYS_PLL_BUS1_pllbus_m_mask) >> SYS_PLL_BUS1_pllbus_m_shift);
    //pllbus_n = ((read_reg32(SYS_PLL_BUS1_reg) & SYS_PLL_BUS1_pllbus_n_mask) >> SYS_PLL_BUS1_pllbus_n_shift) + 1;
    //pllbus_o = ((read_reg32(SYS_PLL_BUS3_reg) & SYS_PLL_BUS3_pllbus_o_mask) >> SYS_PLL_BUS3_pllbus_o_shift) + 1;
    //fout = 27000000 * (pllbus_m + 2 + 2) / (pllbus_n + 1) / (pllbus_o + 1);
    //cec_pre_div = fout / (114000000/202); // fout/cec_pre_div = 114MHz/202;because CEC clock come from rbus
    
    if (!p_this->status.init)
    {              
        cec_info("Open rtk CEC\n");       
        printk("[In rtk_cec_init]\n");		
        write_reg32(cectx_iso_base + RTK_CEC_SOFT_RESET, 0xFFFFFFFF); // 0xfe007088
        write_reg32(cectx_iso_base + RTK_CEC_CLOCK_ENABLE, 0xFFFFFFFF); // 0xfe00708C
        cec_reset_tx_rx();
        write_reg32(cectx_base+ RTK_CEC_ANALOG, 0x0000000c); // 0xfe0075E0
        
        write_reg32(cectx_base+ CEC_TXTCR1, TX_DATA_LOW(0x1A) | TX_DATA_01(0x23) | TX_DATA_HIGH(0x22));
        
        write_reg32(cectx_base+ CEC_TXTCR0, TX_START_LOW(0x93) | TX_START_HIGH(0x20));                       
         
        write_reg32(cectx_base+ CEC_RXTCR0, RX_START_LOW(0x8C) | RX_START_PERIOD(0xBC) | 
                                RX_DATA_SAMPLE(0x2B) | RX_DATA_PERIOD(0x56));                       
                                
        write_reg32(cectx_base+ CEC_CR0 ,    CEC_MODE(1) |  
                                LOGICAL_ADDR(0x4) | TIMER_DIV(0x14) | 
                                PRE_DIV(cec_pre_div) | UNREG_ACK_EN);   
		          	
        write_reg32(cectx_base+ CEC_RTCR0,  RETRY_NO(2)); 
        		
        write_reg32(cectx_base+ CEC_RXCR0,  RX_EN | RX_INT_EN); 
        
//        write_reg32(cectx_base+ CEC_RXCR0,  0);         
       
//        write_reg32(cectx_base+ CEC_TXCR0,  TX_RST);        
        
//        write_reg32(cectx_base+ CEC_TXCR0,  0);         
        
#ifdef CEC_OPCODE_COMPARE_ENABLE
//        write_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE, 0); //disable opcode compare function
#endif	

//        write_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE, 0); //disable opcode compare function
 //       write_reg32(cectx_base + CEC_ANALOG,  0xb );
      
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
		
#ifndef IRENE_CEC0_THREAD_ISR_TEST
        if (request_irq(cectx_irq_num, rtk_cec_isr, IRQF_SHARED, "RTK CEC", p_this) < 0) 
        {
            cec_warning("cec : open rtk cec failed, unable to request irq#%d\n", cectx_irq_num);              			
            return -EIO;
        }
#else  
        {
            irene_p_this = p_this;
            cec_hw_tsk = kthread_create((void *)rtk_cec_isr, &cec_hw_data, "cec_hw_thread");
            if (IS_ERR(cec_hw_tsk)) {
                //ret = PTR_ERR(cec_hw_tsk);
                cec_hw_tsk = NULL;
                printk("cec_hw_tsk thread fail\n");
                return -EIO;
            }
            wake_up_process(cec_hw_tsk);
        }
#endif        
        cec_info("cec : enable CEC SCPU interrupt\n");                          
        write_reg32( cectx_irq_reg , (read_reg32(cectx_irq_reg) & ~CEC_INT_EN_MASK) | CEC_INT_EN_VAL);         
        p_this->status.init = 1;
    }
	
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
        
        cec_info("rtk cec mode : OFF\n");                                
        write_reg32(cectx_base + CEC_CR0, (read_reg32(cectx_base + CEC_CR0) & ~CTRL_MASK1) | CEC_MODE(0) | LOGICAL_ADDR(0xF));             
        p_this->status.enable = 0;  
        p_this->status.standby_mode = 0;                      
        rtk_cec_rx_stop(p_this);
        rtk_cec_tx_stop(p_this);
        
        break;
         
    case CEC_MODE_ON:

        cec_info("rtk cec mode : On\n");                                         
        write_reg32(cectx_base + CEC_CR0, (read_reg32(cectx_base + CEC_CR0) & ~CTRL_MASK1) | CEC_MODE(1));
        write_reg32(cectx_base+ CEC_RXCR0,  RX_EN | RX_INT_EN); 
#ifdef CEC_OPCODE_COMPARE_ENABLE
			write_reg32(cectx_base + GDI_CEC_OPCODE_ENABLE, 0); //disable opcode compare function
#endif				
        rtk_cec_rx_start(p_this);
        rtk_cec_tx_start(p_this);
        p_this->status.enable = 1;
        p_this->status.standby_mode = 0;
            
        break;        
        
    case CEC_MODE_STANDBY:
        
        cec_info("rtk cec mode : Standby (la=%x)\n", rtk_cec_standby_logical_addr);                                                  
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
        cec_info("cec : multiple address specified (%04x), only address %x,f are valid\n", rx_mask, log_addr);           
    }
      
    write_reg32(cectx_base + CEC_CR0, (read_reg32(cectx_base + CEC_CR0) & ~LOGICAL_ADDR_MASK) | LOGICAL_ADDR(log_addr));
    
    cec_info("cec : logical address = %02x\n", log_addr);    

    if (log_addr != 0xF)
        rtk_cec_standby_logical_addr = log_addr;         // save the latest vaild logical address
    
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
    cec_info("cec : physcial address = %04x\n", phy_addr);    
    
    if (phy_addr!= 0xffff)
        rtk_cec_standby_physical_addr = phy_addr;      // we always keep latest valid physical address during standby
        
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
            cec_tx_dbg("cec : xmit message success\n");
            break;
                            
        case XMIT_ABORT:
            cec_warning("cec : xmit message abort\n");
            break;        
        
        case XMIT_FAIL:
            cec_warning("cec0 : xmit message failed\n");
            break;
        
        default:
        case XMIT_TIMEOUT:    
        case WAIT_XMIT:
            cec_warning("cec : xmit message timeout\n");            
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
            cec_rx_dbg("got msg %p\n", cmb);
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
    cec_info("rtk cec closed\n");    
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
	
    cec_info("rtk cec suspended (la=%x, pa=%04x, standby_mode=%ld)\n",
            rtk_cec_standby_logical_addr, 
            rtk_cec_standby_physical_addr, 
            rtk_cec_standby_config);
    
    rtk_cec_set_mode(p_this, CEC_MODE_OFF); 
    
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
        
        cec_info("cec standby enabled\n");
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
    cec_info("rtk cec resume\n");       
         
    //p_this->status.enable = 0;  
    p_this->status.standby_mode = 0;     
            
    if (rtk_cec_standby_config && rtk_cec_standby_logical_addr<0xF && rtk_cec_standby_physical_addr!=0xFFFF)
        rtk_cec_set_mode(p_this, CEC_MODE_STANDBY);    
    else
        rtk_cec_set_mode(p_this, CEC_MODE_OFF);
    
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
	write_reg32(cectx_base + CEC_RXTCR0, RX_START_LOW(140) | RX_START_PERIOD(188) | 
										RX_DATA_SAMPLE(42) | RX_DATA_PERIOD(82));						
	write_reg32(cectx_base + CEC_TXTCR0, TX_START_LOW(148) | TX_START_HIGH(32)); 					  
	write_reg32(cectx_base + CEC_TXTCR1, TX_DATA_LOW(24) | TX_DATA_01(36) | TX_DATA_HIGH(36));

	//enable Rx
	write_reg32(cectx_base + CEC_RXCR0,  RX_EN);	// enable RX
}



////////////////////////////////////////////////////////////////////////
// CEC Driver Interface                                               //
////////////////////////////////////////////////////////////////////////
#if 0
struct device_node * test_cec0_register_dev(cec_device* device, cec_driver* driver)
{
	struct device_node __maybe_unused *cectx_node;

	printk("[test_cec0_register_dev]\n");
	cectx_node = of_find_matching_node(NULL, cectx_ids);
	if (!cectx_node)
		printk(KERN_ALERT "no cectx_node\n");
		
		
    if (0)//(device->id!=ID_RTK_CEC_CONTROLLER)
        return NULL;     
		
	cectx_base = of_iomap(cectx_node , 0);	
	cectx_iso_base = of_iomap(cectx_node , 1);	
		
	cectx_irq_reg = of_iomap(cectx_node , 1);		

	printk("[In probe ,BASE: %x,%x,%x]\n",(int)cectx_base,(int)cectx_iso_base,(int)cectx_irq_reg);
    
    return cectx_node;
}
#endif

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
	printk("[cec_0_ops_probe]\n");
	cectx_node = of_find_matching_node(NULL, cectx_ids);
	if (!cectx_node)
		printk("no cectx_node\n");
	else
        printk("has cectx_node\n");	
		
    if (dev->id!=ID_RTK_CEC_CONTROLLER)
    {
        cec_device_en[0] = 0;
        return -ENODEV;     
    }	
	of_property_read_u32(cectx_node, "module-enable", &(module_en));
	
	if(!module_en)
	    return -ENODEV; 	
	cec_device_en[0] = 1;
	cectx_base = of_iomap(cectx_node , 0);	
	cectx_iso_base = of_iomap(cectx_node , 1);	
		
	cectx_irq_reg = of_iomap(cectx_node , 1);		

	printk("[In probe ,BASE: %x,%x,%x]\n",(int)cectx_base,(int)cectx_iso_base,(int)cectx_irq_reg);
#ifndef IRENE_CEC0_THREAD_ISR_TEST
	/* Request IRQ */
	cectx_irq_num = irq_of_parse_and_map(cectx_node, 0);
	if (!cectx_irq_num) {
		printk(KERN_ALERT "CEC Tx: fail to parse of irq.\n");
		return -ENXIO;
	}
#endif
	//cec_info("[%s] %s  %d irq number = %d \n", __FILE__,__FUNCTION__,__LINE__,cectx_irq_num);	 
	
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
    cec_info("%s cec module init\n", CEC_MODEL_NAME);                                                                                              

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
