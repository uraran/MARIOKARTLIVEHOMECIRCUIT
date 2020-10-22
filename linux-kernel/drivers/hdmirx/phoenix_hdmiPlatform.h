#ifndef PHOENIX_HDMI_PLATFORM_H
#define PHOENIX_HDMI_PLATFORM_H

#define HDMI_CONST					const
#include "phoenix_hdmiHWReg.h"
#include "phoenix_hdmiInternal.h"
#include <linux/delay.h>
#include <linux/hardirq.h>

//------------------------------------------
// Definitions of Bits
//------------------------------------------
#define _ZERO                       0x00
#define _BIT0                       0x01
#define _BIT1                       0x02
#define _BIT2                       0x04
#define _BIT3                       0x08
#define _BIT4                       0x10
#define _BIT5                       0x20
#define _BIT6                       0x40
#define _BIT7                       0x80
#define _BIT8                       0x0100
#define _BIT9                       0x0200
#define _BIT10                      0x0400
#define _BIT11                      0x0800
#define _BIT12                      0x1000
#define _BIT13                      0x2000
#define _BIT14                      0x4000
#define _BIT15                      0x8000
#define _BIT16                      0x10000
#define _BIT17                      0x20000
#define _BIT18                      0x40000
#define _BIT19                      0x80000
#define _BIT20                      0x100000
#define _BIT21                      0x200000
#define _BIT22                      0x400000
#define _BIT23                      0x800000
#define _BIT24                      0x1000000
#define _BIT25                      0x2000000
#define _BIT26                      0x4000000
#define _BIT27                      0x8000000
#define _BIT28                      0x10000000
#define _BIT29                      0x20000000
#define _BIT30                      0x40000000
#define _BIT31                      0x80000000

unsigned char HdmiIsAudioSpdifIn(void);
void Hdmi_VideoOutputDisable(void);
void HdmiGetFastBootStatus(void);
void HdmiSetPreviousSource(int channel);
void drvif_Hdmi_HPD(unsigned char channel_index, char high);

//#define hdmi_in(addr, type)	        hdmi_rx_reg_read32(addr, type)
//#define hdmi_out(addr, value, type)     hdmi_rx_reg_write32(addr, value, type)
//#define hdmi_mask(addr, andmask, ormask, type)    hdmi_rx_reg_mask32(addr, andmask, ormask, type)
//#define hdmiport_out(addr, value)                                    phoenix_IoReg_Write32(addr, value)
//#define hdmiport_mask(addr, andmask, ormask)   	       phoenix_IoReg_Mask32(addr, andmask, ormask)
//#define hdmiport_in(addr)           			                     phoenix_IoReg_Read32(addr)

//#define rtd_outl(x, y)     								phoenix_IoReg_Write32(x,y)
//#define rtd_inl(x)     									phoenix_IoReg_Read32(x)
//#define rtd_maskl(x, y, z)     							phoenix_IoReg_Mask32(x,y,z)

#define HDMI_DELAYUS(x)    do{ udelay(x); }while(0)
#define HDMI_DELAYMS(x)    do{ if(in_atomic()) mdelay(x); else msleep(x); }while(0)
#define HDMI_COPY 											memcpy
#define HDMI_DISABLE_TIMER_ISR()							do { } while(0)
#define HDMI_ENABLE_TIMER_ISR()							do { } while(0)
#define HDMI_ABS(x, y)	 									((x > y) ? (x-y) : (y-x))
#define HDMI_IS_AUDIO_SPDIF_PATH()				(HdmiIsAudioSpdifIn())

#define INTERVAL(t001)    do{ \
        struct timeval t002; \
        long interval; \
        do_gettimeofday(&t002); \
        interval = t002.tv_usec - t001.tv_usec; \
        pr_info("%s:%d %ldus\n", __func__, __LINE__, interval); \
        t001 = t002; \
}while(0)

#endif //PHOENIX_HDMI_PLATFORM_H
