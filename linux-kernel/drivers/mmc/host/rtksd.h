/*
 *  Copyright (C) 2010 Realtek Semiconductors, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __RTKSD_H
#define __RTKSD_H

#include <rbus/reg_mmc.h>               //liao
#include "../mmc_debug.h"               //liao

/*
#define NONE			"\033[m"
#define RED				"\033[0;32;31m"
#define LIGHT_RED		"\033[1;31m"
#define GREEN			"\033[0;32;32m"
#define LIGHT_GREEN		"\033[1;32m"
#define BLUE			"\033[0;32;34m"
#define LIGHT_BLUE		"\033[1;34m"
#define DARY_GRAY		"\033[1;30m"
#define CYAN			"\033[0;36m"
#define LIGHT_CYAN		"\033[1;36m"
#define PURPLE			"\033[0;35m"
#define LIGHT_PURPLE	"\033[1;35m"
#define BROWN			"\033[0;33m"
#define YELLOW			"\033[1;33m"
#define LIGHT_GRAY		"\033[0;37m"
#define WHITE			"\033[1;37m"

#define ATTR_OFF		"\033[0m"
#define BOLD			"\033[1m"
#define UNDERSCORE		"\033[4m"
#define BLINK			"\033[5m"
#define REVERSE			"\033[7m"
#define CONCEALED		"\033[8m"
*/

#define MMC_EXT_READ_SINGLE			48
#define MMC_EXT_WRITE_SINGLE		49
#define MMC_EXT_READ_MULTIPLE		58
#define MMC_EXT_WRITE_MULTIPLE		59

//debug
//#define CRSD_TUNE_DBG
//#define CRSD_DBG
//#define CRSD_REG_DBG


#if defined(CRSD_TUNE_DBG) 
#define RTKSDPRINTF_WARM(fmt, args...)	printk(KERN_WARNING "[%26s %4d]"fmt,__FUNCTION__,__LINE__,## args)
#define RTKSDPRINTF_NOTC(fmt, args...)	printk(KERN_NOTICE "[%26s %4d]"fmt,__FUNCTION__,__LINE__,## args)
#define RTKSDPRINTF(fmt, args...)   	
#define RedSDPRINTF(fmt, args...)		
#define BluSDPRINTF(fmt, args...)		
#define CyanSDPRINTF(fmt, args...)		
#define RTKSDPRINTF_DBG(fmt, args...)	
#elif defined(CRSD_DBG) 
#define RTKSDPRINTF_WARM(fmt, args...)	printk(KERN_WARNING "[%26s %4d]"fmt,__FUNCTION__,__LINE__,## args)
#define RTKSDPRINTF_NOTC(fmt, args...)	printk(KERN_NOTICE "[%26s %4d]"fmt,__FUNCTION__,__LINE__,## args)
#define RTKSDPRINTF(fmt, args...)   	printk(KERN_INFO "[%26s %4d]"fmt,__FUNCTION__,__LINE__,## args)
#define RedSDPRINTF(fmt, args...)		printk(KERN_INFO "\033[0;32;31m" "[%26s %4d]"  fmt "\033[m",__FUNCTION__,__LINE__,## args)
#define BluSDPRINTF(fmt, args...)		printk(KERN_INFO "\033[0;32;34m" "[%26s %4d]"  fmt "\033[m",__FUNCTION__,__LINE__,## args)
#define CyanSDPRINTF(fmt, args...)		printk(KERN_INFO "\033[0;36m" "[%26s %4d]"  fmt "\033[m",__FUNCTION__,__LINE__,## args)
#define RTKSDPRINTF_DBG(fmt, args...)	printk(KERN_DEBUG "[%26s %4d]"fmt,__FUNCTION__,__LINE__,## args)
#else
#define RTKSDPRINTF_WARM(fmt, args...)
#define RTKSDPRINTF_NOTC(fmt, args...)	
#define RTKSDPRINTF(fmt, args...)
#define RedSDPRINTF(fmt, args...)
#define BluSDPRINTF(fmt, args...)
#define CyanSDPRINTF(fmt, args...)
#define RTKSDPRINTF_DBG(fmt, args...)
#endif

#ifdef CRSD_REG_DBG
#define REG_SDPRINTF(fmt, args...)		printk(KERN_NOTICE fmt,## args)
#else
#define REG_SDPRINTF(fmt, args...)
#endif




/* cmd1 sector mode */
#define MMC_SECTOR_ADDR         0x40000000
/*
 * Clock rates
 */
#define RTKSD_CLOCKRATE_MAX			48000000
#define RTKSD_BASE_DIV_MAX			0x100
#define RTKSD_CLK_STEP				32 // 16MHZ
#define RTKSD_MAX_CLK_PLL3_SD		147 // 0x56:(86+3=89), 0xB6:(182+3=185)

#define crsd_readb(offset)        readb(IOMEM(offset))
#define crsd_writeb(val, offset)  do { 		\
				writeb(val, IOMEM(offset));	\
				} while(0);
#define crsd_readl(offset)        readl(IOMEM(offset))
#define crsd_writel(val, offset)  do { 			\
				writel(val, IOMEM(offset));		\
				} while(0)


#define CLEAR_CR_CARD_STATUS(reg_addr)      \
do {                                \
    crsd_writel(0xffffffff, reg_addr);    \
} while (0)

#define CLEAR_ALL_CR_CARD_STATUS(io_base)        \
do {                                \
    int i = 0;  \
    for(i=0; i<5; i++)  \
        CLEAR_CR_CARD_STATUS(io_base+20+ (i*4));  \
} while (0)

/*  ===== macros and funcitons for Realtek CR ===== */
/* for SD/MMC usage */
#define ON                      0
#define OFF                     1
#define GPIO_OUT    1
#define GPIO_IN     0
#define GPIO_HIGH   1
#define GPIO_LOW    0

/* MMC configure1, for SD_CONFIGURE1 */
#define SD30_FIFO_RST               (1<<4)
#define SD1_R0                      (SDCLK_DIV | SDCLK_DIV_256 | RST_RDWR_FIFO)

/* MMC configure3 , for SD_CONFIGURE3 */
#define SD30_CLK_STOP               (1<<4)
#define SD2_R0                      (RESP_CHK_EN | ADDR_BYTE_MODE)

/* MMC configure2, for SD_CONFIGURE2, response type */
#define SD_R0                   (RESP_TYPE_NON|CRC7_CHK_DIS)
#define SD_R1                   (RESP_TYPE_6B)
#define SD_R1b                  (RESP_TYPE_6B)
#define SD_R2                   (RESP_TYPE_17B)
#define SD_R3                   (RESP_TYPE_6B|CRC7_CHK_DIS)
#define SD_R4                   (RESP_TYPE_6B)
#define SD_R5                   (RESP_TYPE_6B)
#define SD_R6                   (RESP_TYPE_6B)
#define SD_R7                   (RESP_TYPE_6B)
#define SD_R_NO                 (0xFF)

/* rtflags */
#define RTKCR_FCARD_DETECTED    (1<<0)      /* Card is detected */
#define RTKCR_FCARD_SELECTED    (1<<1)      /* Card is detected */
#define RTKCR_USER_PARTITION    (1<<2)      /* card is working on normal partition */


#define RTKCR_FCARD_POWER       (1<<4)      /* Card is power on */
#define RTKCR_FHOST_POWER       (1<<5)      /* Host is power on */

struct rtk_crsd_host {
    struct mmc_host     *mmc;           /* MMC structure */
    volatile u32		rtflags;        /* Driver states */
    u8                  ins_event;
    u8                  cmd_opcode;

#define EVENT_NON		    0x00
#define EVENT_INSER		    0x01
#define EVENT_REMOV		    0x02
#define EVENT_USER		    0x10

    u8                  reset_event;
    struct mmc_request  *mrq;            /* Current request */
    u8                  wp;
    struct rtk_crsd_host_ops *ops;
    struct semaphore	sem;
    struct semaphore	sem_op_end;

    void __iomem        *base_crsd;
    u32 base_io;
    void __iomem        *base_pll;
    u32 base_pll_io;
    void __iomem        *base_sysbrdg;
    u32 base_sysbrdg_io;
    void __iomem        *base_emmc;
    u32 base_emmc_io;
    void __iomem        *base_sdio;
    u32 base_sdio_io;




    spinlock_t          lock;
    unsigned int        ns_per_clk;
    struct delayed_work cmd_work;
    struct tasklet_struct req_end_tasklet;

    struct timer_list   timer;
    struct timer_list   plug_timer;
    struct completion   *int_waiting;
    struct device       *dev;
    int                 irq;
    u8                  *tmp_buf;
    dma_addr_t          tmp_buf_phy_addr;
    dma_addr_t          paddr;
#ifdef MONI_MEM_TRASH
    u8                  *cmp_buf;
    dma_addr_t          phy_addr;
#endif
    //int                 gpio_card_detect;
    //int                 gpio_write_protect;
    int                 gpio_card_power;    
    u32                 test_count;
    volatile u32		int_status;
    volatile u32		int_status_old;
    volatile u32		sd_status1;
    volatile u32		sd_status2;
    volatile u32		bus_status;
    volatile u32		dma_trans;
    volatile u32		sd_trans;
    volatile u32		gpio_isr_info;
    u32                 tmout;
    unsigned int  		magic_num;
};

struct rtk_crsd_host_ops {
    irqreturn_t (*func_irq)(int irq, void *dev);
    int (*re_init_proc)(struct mmc_card *card);
    int (*card_det)(struct rtk_crsd_host *sdport);
    void (*card_power)(struct rtk_crsd_host *sdport,u8 status);
	void (*chk_card_insert)(struct rtk_crsd_host *rtkhost);
	void (*set_crt_muxpad)(struct rtk_crsd_host *rtkhost);
	void (*set_clk)(struct rtk_crsd_host *rtkhost,u32 mmc_clk);
    void (*reset_card)(struct rtk_crsd_host *rtkhost);
    void (*reset_host)(struct rtk_crsd_host *rtkhost);
    void (*bus_speed_down)(struct rtk_crsd_host *sdport);
    u32 (*get_cmdcode)(u32 opcode );
    u32 (*get_r1_type)(u32 opcode );
    u32 (*chk_cmdcode)(struct mmc_command* cmd);
    u32 (*chk_r1_type)(struct mmc_command* cmd);

};

struct crsd_cmd_pkt {
    struct mmc_host     *mmc;       /* MMC structure */
    struct rtk_crsd_host   *sdport;
    struct mmc_command  *cmd;    /* cmd->opcode; cmd->arg; cmd->resp; cmd->data */
    struct mmc_data     *data;
    unsigned char       *dma_buffer;
    u16                 byte_count;
    u16                 block_count;

    u32                 flags;
    s8                  rsp_para1;
    s8                  rsp_para2;
    s8                  rsp_para3;
    u8                  rsp_len;
    u32                 timeout;
};

/* *** Realtek CRT register *** */
//#define CRT_SYS_CLKSEL          0xb8000204
//#define CRT_SYS_SRST2           0xb8000108
//#define CRT_SYS_SRST3           0xb800010c
//#define CRT_SYS_CLKEN2          0xb8000118
//#define CRT_SYS_CLKEN3          0xb800011c
//#define CRT_NF_CR_PMUX          0xb800030c
//#define CRT_MUXPAD5             0xb8000364
//#define CRT_PFUNC_NF0           0xb8000390
//#define CRT_PFUNC_NF1           0xb8000394
//#define CRT_PFUNC_NF2           0xb8000398
//#define GPIO_BASE               0xb8010000
//#define CRT_MUXPAD4             0xb8000360
//#define CRT_MUXPAD10            0xb8000378
//#define PLL_HDMI2               0xb8000194

/* *** Realtek CRT register &&& */

/* CRT_SYS_CLKSEL setting bit *** */
//#define EMMC_CLKSEL_SHT        (12)
//#define SD_CLKSEL_SHT          (20)

#define EMMC_CLKSEL_MASK        (0x07<<EMMC_CLKSEL_SHT)
#define SD_CLKSEL_MASK          (0x07<<SD_CLKSEL_SHT)
#define EMMC_CLOCK_SPEED_GAP    (0x03<<EMMC_CLKSEL_SHT)
#define SD_CLOCK_SPEED_GAP      (0x03<<SD_CLKSEL_SHT)

//#define CARD_SWITCHCLOCK_20MHZ_B    (0x00UL)    //(0x04)
//#define CARD_SWITCHCLOCK_25MHZ_B    (0x01UL)    //(0x05)
//#define CARD_SWITCHCLOCK_33MHZ_B    (0x02UL)    //(0x06)
//#define CARD_SWITCHCLOCK_40MHZ_B    (0x03UL)    //(0x00)
//#define CARD_SWITCHCLOCK_49MH_B     (0x04UL)    //(0x07)
//#define CARD_SWITCHCLOCK_50MHZ_B    (0x05UL)    //(0x01)
//#define CARD_SWITCHCLOCK_66MHZ_B    (0x06UL)    //(0x02)
//#define CARD_SWITCHCLOCK_98MH_B     (0x07UL)    //(0x03)

#define CARD_SWITCHCLOCK_25MHZ_B    (0x00UL)    //(0x05)
#define CARD_SWITCHCLOCK_33MHZ_B    (0x01UL)    //(0x06)
#define CARD_SWITCHCLOCK_40MHZ_B    (0x02UL)    //(0x00)
#define CARD_SWITCHCLOCK_50MHZ_B    (0x03UL)    //(0x01)
#define CARD_SWITCHCLOCK_66MHZ_B    (0x04UL)    //(0x02)
#define CARD_SWITCHCLOCK_98MH_B     (0x05UL)    //(0x03)
#define CLOCK_SPEED_GAP          CARD_SWITCHCLOCK_50MHZ_B
#define LOW_SPEED_LMT               CARD_SWITCHCLOCK_33MHZ_B

 
#define CARD_SWITCHCLOCK_60MHZ  (0x00UL)      //(0x00<<12)
#define CARD_SWITCHCLOCK_80MHZ  (0x01UL)      //(0x01<<12)
#define CARD_SWITCHCLOCK_98MHZ  (0x02UL)      //(0x02<<12)
#define CARD_SWITCHCLOCK_98MHZS (0x03UL)      //(0x03<<12)
#define CARD_SWITCHCLOCK_30MHZ  (0x04UL)      //(0x04<<12)
#define CARD_SWITCHCLOCK_40MHZ  (0x05UL)      //(0x05<<12)
#define CARD_SWITCHCLOCK_49MHZ  (0x06UL)      //(0x06<<12)
#define CARD_SWITCHCLOCK_49MHZS (0x07UL)      //(0x07<<12)

#define RCA_SHIFTER             16
#define NORMAL_READ_BUF_SIZE    512      //no matter FPGA & QA

#define SD_CARD_WP              (1<<5)
//#define MS_CARD_EXIST           (1<<3)
#define SD_CARD_EXIST           (1<<2)


/* move from c file *** */
#define BYTE_CNT            0x200
#define RTK_NORMAL_SPEED    0x00
#define RTK_HIGH_SPEED      0x01
#define RTK_1_BITS          0x00
#define RTK_4_BITS          0x10
#define RTK_BITS_MASK       0x30
#define RTK_SPEED_MASK      0x01
#define RTK_PHASE_MASK      0x06

#define R_W_CMD             2   //read/write command
#define INN_CMD             1   //command work chip inside
#define UIN_CMD             0   //no interrupt rtk command

#define RTK_FAIL            3  /* DMA error & cmd parser error */
#define RTK_RMOV            2  /* card removed */
#define RTK_TOUT            1  /* time out include DMA finish & cmd parser finish */
#define RTK_SUCC            0

#define CR_TRANS_OK         0x0
#define CR_TRANSFER_TO      0x1
#define CR_BUF_FULL_TO      0x2
#define CR_DMA_FAIL         0x3
#define CR_TRANSFER_FAIL    0x4

/* send status event */
#define STATE_IDLE          0
#define STATE_READY         1
#define STATE_IDENT         2
#define STATE_STBY          3
#define STATE_TRAN          4
#define STATE_DATA          5
#define STATE_RCV           6
#define STATE_PRG           7
#define STATE_DIS           8

#define GPIO_HI             0x78
#define GPIO_LO             0x87

#define LVL_HI             0x01
#define LVL_LO             0x00

#define POW_CHECK 0
#define POW_FORCE 1

#define rtk_crsd_get_int_sta(io_base,status_addr)                           \
            do {                                                         \
                if (io_base == CR_BASE_ADDR)                             \
                    *(u32 *)status_addr = crsd_readl(io_base+CR_SD_ISR);   \
                else                                                     \
                    *(u32 *)status_addr = crsd_readl(io_base+EMMC_SD_ISR); \
            } while (0);

#define rtk_crsd_get_sd_sta(io_base,status_addr1,status_addr2,bus_status)       \
            do {                                                             \
                    *(u32 *)status_addr1 = crsd_readb(io_base+SD_STATUS1);     \
                    *(u32 *)status_addr2 = crsd_readb(io_base+SD_STATUS2);     \
                    *(u32 *)bus_status = crsd_readb(io_base+SD_BUS_STATUS);    \
            } while (0);

#define rtk_crsd_get_sd_trans(io_base,status_addr) \
            *(u32 *)status_addr = crsd_readb(io_base+SD_TRANSFER)
#define rtk_crsd_get_dma_trans(io_base,status_addr) \
            *(u32 *)status_addr = crsd_readb(io_base+CR_DMA_CTL3)

#define rtk_crsd_clr_int_sta(io_base)                                                                              \
            do {                                                                                                \
                if (io_base == CR_BASE_ADDR)                                                                    \
                    crsd_writel( (crsd_readl(io_base+CR_SD_ISR)|(ISRSTA_INT1|ISRSTA_INT2))|CLR_WRITE_DATA ,io_base+CR_SD_ISR); \
                else if (io_base == EM_BASE_ADDR)                                                               \
                    crsd_writel( (crsd_readl(io_base+EMMC_SD_ISR)|(ISRSTA_INT1|ISRSTA_INT2))|CLR_WRITE_DATA ,io_base+EMMC_SD_ISR); \
                else                                                                                            \
                    crsd_writel( (crsd_readl(io_base+SDIO_SD_ISR)|(SDIO_ISRSTA_INT1|SDIO_ISRSTA_INT2|SDIO_ISRSTA_INT3|SDIO_ISRSTA_INT4))|CLR_WRITE_DATA ,io_base+SDIO_SD_ISR); \
            } while(0);

#define rtk_crsd_hold_int_dec(io_base)    \
            do {                                                                                                \
                if (io_base == CR_BASE_ADDR)                                                                    \
                    crsd_writel( (crsd_readl(io_base+CR_SD_ISREN)|(ISRSTA_INT1EN|ISRSTA_INT2EN))|CLR_WRITE_DATA ,io_base+CR_SD_ISREN); \
                else if (io_base == EM_BASE_ADDR)                                                               \
                    crsd_writel( (crsd_readl(io_base+EMMC_SD_ISREN)|(ISRSTA_INT1EN|ISRSTA_INT2EN))|CLR_WRITE_DATA ,io_base+EMMC_SD_ISREN); \
                else                                                                                            \
                    crsd_writel( (crsd_readl(io_base+SDIO_SD_ISREN)|SDIO_ISRSTA_INT1EN)|CLR_WRITE_DATA ,io_base+SDIO_SD_ISREN); \
            } while(0);

#define rtk_crsd_en_int(io_base)  \
            do {                                                                                                \
                if (io_base == CR_BASE_ADDR)                                                                    \
                    crsd_writel( (crsd_readl(io_base+CR_SD_ISREN)|(ISRSTA_INT1EN|ISRSTA_INT2EN))|WRITE_DATA ,io_base+CR_SD_ISREN); \
                else if (io_base == EM_BASE_ADDR)                                                               \
                    crsd_writel( (crsd_readl(io_base+EMMC_SD_ISREN)|(ISRSTA_INT1EN|ISRSTA_INT2EN))|WRITE_DATA ,io_base+EMMC_SD_ISREN); \
                else                                                                                            \
                    crsd_writel( (crsd_readl(io_base+SDIO_SD_ISREN)|SDIO_ISRSTA_INT1EN)|WRITE_DATA ,io_base+SDIO_SD_ISREN); \
            } while(0);

#define  rtk_crsd_mdelay(x)  \
            set_current_state(TASK_INTERRUPTIBLE); \
            schedule_timeout(msecs_to_jiffies(x))

#define INT_BLOCK_R_GAP 0x200
#define INT_BLOCK_W_GAP 5

static const char *const state_tlb[9] = {
    "STATE_IDLE",
    "STATE_READY",
    "STATE_IDENT",
    "STATE_STBY",
    "STATE_TRAN",
    "STATE_DATA",
    "STATE_RCV",
    "STATE_PRG",
    "STATE_DIS"
};

static const char *const bit_tlb[4] = {
    "1bit",
    "4bits",
    "8bits",
    "unknow"
};

static const char *const clk_tlb[8] = {
    "30MHz",
    "40MHz",
    "49MHz",
    "49MHz",
    "15MHz",
    "20MHz",
    "24MHz",
    "24MHz"
};

enum crsd_clock_speed {
	CRSD_CLOCK_200KHZ = 0,
	CRSD_CLOCK_400KHZ = 1,
	CRSD_CLOCK_6200KHZ = 2,
	CRSD_CLOCK_20000KHZ = 3,
	CRSD_CLOCK_25000KHZ = 4,
	CRSD_CLOCK_50000KHZ = 5,
	CRSD_CLOCK_208000KHZ = 6,
	CRSD_CLOCK_100000KHZ = 7,
};



static const u32 const clk_2_hz[8] = {
    10000000,
    12000000,
    15000000,
    20000000,
    24000000,
    30000000,
    40000000,
    48000000
};

/* data read cmd */
static const u8 const opcode_r_type[16] = {
    0,0,0,0,0,1,1,0,0,0,0,0,1,1,1,0
};

/* data write cmd */
static const u8 const opcode_w_type[16] = {
    1,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0
};

/* data xfer cmd */
static const u8 const opcode_d_type[16] = {
    1,1,1,0,0,1,1,0,0,1,1,0,1,1,1,0
};

static const char *const cmdcode_tlb[16] = {
    "N_W",      /* 0 */
    "AW3",      /* 1 */
    "AW4",      /* 2 */
    "UNK",      /* 3 */
    "UNK",      /* 4 */
    "AR3",      /* 5 */
    "AR4",      /* 6 */
    "UNK",      /* 7 */
    "SGR",      /* 8 */
    "AW1",      /* 9 */
    "AW2",      /* 10 */
    "UNK",      /* 11 */
    "N_R",      /* 12 */
    "AR1",      /* 13 */
    "AR2",      /* 14 */
    "UNK",      /* 15 */
};

#define card_sta_err_mask ((1<<31)|(1<<30)|(1<<29)|(1<<28)|(1<<27)|(1<<26)|(1<<24)|(1<<23)|(1<<22)|(1<<21)|(1<<20)|(1<<19)|(1<<18)|(1<<17)|(1<<16)|(1<<15)|(1<<13)|(1<<7))

//#define CMD25_USE_SD_AUTOWRITE2 // jinn - for SD card write performance

#define CMD25_WO_STOP_COMMAND // jinn - for SD card write performance

/* Only ADTC type cmd use */
static const unsigned char rtk_crsd_cmdcode[60][2] = {
    {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, //0~3
    {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_NORMALREAD ,SD_R1 }, {SD_CMD_UNKNOW ,SD_R0 }, //4~7
    {SD_CMD_UNKNOW ,SD_R1 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R1 }, //8~11
    {SD_CMD_UNKNOW ,SD_R0 }, {SD_NORMALREAD ,SD_R1 }, {SD_CMD_UNKNOW ,SD_R1 }, {SD_CMD_UNKNOW ,SD_R0 }, //12~15
    {SD_CMD_UNKNOW ,SD_R0 }, {SD_AUTOREAD2  ,SD_R1 }, {SD_AUTOREAD1  ,SD_R1 }, {SD_CMD_UNKNOW,SD_R1 }, //16~19
    {SD_AUTOWRITE1 ,SD_R1 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_NORMALREAD ,SD_R1 }, {SD_CMD_UNKNOW ,SD_R0 }, //20~23
#ifndef CMD25_USE_SD_AUTOWRITE2
	{SD_AUTOWRITE2 ,SD_R1 }, {SD_AUTOWRITE1 ,SD_R1 }, {SD_NORMALWRITE,SD_R1 }, {SD_NORMALWRITE,SD_R1 }, //24~27
#else
    {SD_AUTOWRITE2 ,SD_R1 }, {SD_AUTOWRITE2 ,SD_R1 }, {SD_NORMALWRITE,SD_R1 }, {SD_NORMALWRITE,SD_R1 }, //24~27
#endif
    {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_NORMALREAD ,SD_R1 }, {SD_NORMALREAD ,SD_R1 }, //28~31
    {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, //32~35
    {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, //36~39
    {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_NORMALREAD ,SD_R1 }, {SD_CMD_UNKNOW ,SD_R0 }, //40~43
    {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, //44~47
    {SD_AUTOREAD2  ,SD_R1 }, {SD_AUTOWRITE2 ,SD_R1 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_NORMALREAD ,SD_R1 }, //48~51
    {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_CMD_UNKNOW ,SD_R0 }, //52~55
    {SD_AUTOREAD2  ,SD_R1 }, {SD_CMD_UNKNOW ,SD_R0 }, {SD_AUTOREAD2  ,SD_R1 }, {SD_AUTOWRITE2 ,SD_R1 }  //56~59
};

/* remove from c file &&& */

/* rtk function definition */
//#define ENABLE_SD_INT_MODE                 //enable 119x mmc interrupt



struct timing_phase_path {
	int start;
	int end;
	int mid;
	int len;
};

#endif
