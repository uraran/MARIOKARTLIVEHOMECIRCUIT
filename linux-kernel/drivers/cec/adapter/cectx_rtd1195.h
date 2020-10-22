//Copyright (C) 2007-2013 Realtek Semiconductor Corporation.
#ifndef __RTD1195_CECTX_H__
#define __RTD1195_CECTX_H__

#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/completion.h>
#include "../core/cec.h"
#include "../core/cm_buff.h"


#define ID_RTK_CEC_CONTROLLER     0x1778

#define ISO_IRQ        		57
#define RX_RING_LENGTH  	16
#define TX_RING_LENGTH  	16

typedef struct {
    unsigned char       enable : 1;
    unsigned char       state  : 7;    
    cm_buff*            cmb;    
    unsigned long       timeout;    
    struct delayed_work work;        
}rtk_cec_xmit;


typedef struct {
    unsigned char       enable : 1;
    unsigned char       state  : 7;
    cm_buff*            cmb;
    wait_queue_head_t   wq;
    struct delayed_work work;
}rtk_cec_rcv;


enum 
{
    IDEL,
    XMIT,        
    RCV    
};

enum 
{    
    RCV_OK,
    RCV_FAIL,    
};

#define CEC_MODE_OFF        0
#define CEC_MODE_ON         1
#define CEC_MODE_STANDBY    2   

typedef struct 
{
    struct {
        unsigned char   init   : 1;
        unsigned char   enable : 1;   
        unsigned char   standby_mode : 1;   
    }status;
    
    rtk_cec_xmit   xmit;    
    rtk_cec_rcv    rcv;
    cm_buff_head      tx_queue;    
    cm_buff_head      rx_queue;    
    cm_buff_head      rx_free_queue;                        
    spinlock_t        lock;    

}rtk_cec;


#endif //__RTD1195_CECTX_H__
