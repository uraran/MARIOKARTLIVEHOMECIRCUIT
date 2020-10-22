/*
 *  Copyright (C) 2010 Realtek Semiconductors, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __RTK_CRSD_OPS_H
#define __RTK_CRSD_OPS_H
#include <linux/completion.h>

void rtk_crsd_sync(struct rtk_crsd_host *sdport);
int rtk_crsd_wait_opt_end(char*,struct rtk_crsd_host *sdport,unsigned char cmdcode,unsigned char cpu_mode);
int rtk_crsd_int_wait_opt_end(char*,struct rtk_crsd_host *sdport,unsigned char cmdcode,unsigned char cpu_mode);
void rtk_crsd_int_enable_and_waitfor(struct rtk_crsd_host *sdport, u8 cmdcode, unsigned long msec, unsigned long dma_msec);
void rtk_crsd_int_waitfor(struct rtk_crsd_host *sdport, u8 cmdcode, unsigned long msec,unsigned long dma_msec);
void rtk_crsd_op_complete(struct rtk_crsd_host *sdport);
char *rtk_crsd_parse_token(const char *parsed_string, const char *token);
void rtk_crsd_chk_param(u32 *pparam, u32 len, u8 *ptr);
int rtk_crsd_chk_VerA(void);
void rtk_crsd_show_config123(struct rtk_crsd_host *sdport);
void rtk_crsd_set_mis_gpio(u32 gpio_num,u8 dir,u8 level);
void rtk_crsd_set_iso_gpio(u32 gpio_num,u8 dir,u8 level);

int rtk_crsd_fast_write( unsigned int blk_addr,
                    unsigned int data_size,
                    unsigned char * buffer );

int rtk_crsd_fast_read( unsigned int blk_addr,
                   unsigned int data_size,
                   unsigned char * buffer );
struct completion* rtk_crsd_int_enable(struct rtk_crsd_host *sdport, unsigned long msec);
void rtk_crsd_int_enable_and_waitfor(struct rtk_crsd_host *sdport, u8 cmdcode, unsigned long msec, unsigned long dma_msec);
void rtk_crsd_waitfor(struct rtk_crsd_host *sdport);
#endif


/* end of file */



