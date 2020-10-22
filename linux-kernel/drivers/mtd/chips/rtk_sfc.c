/*
 * MTD chip driver for SPI Serial Flash 
 * w/ Serial Flash Controller Interface
 *
 * Copyright 2005 Chih-pin Wu <wucp@realtek.com.tw>
 * Copyright 2014 Louis Yang  <louis_yang@realtek.com>
 *
 */
//#define CONFIG_MTD_RTK_MD_READ_COMPARE
//#define CONFIG_MTD_RTK_MD_WRITE_COMPARE
//#define DEV_DEBUG
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/mtd/physmap.h>

#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/cfi.h>
#include <linux/mtd/partitions.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/pm.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <mtd/mtd-abi.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <rbus/reg_sys.h>
#include "../mtdcore.h"
#include <asm/system_info.h>
#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
#include <linux/slab.h>
#endif
#include <platform.h>

#define SET_SPEED	(0)

#define SATURN_RW_MD (1)
#ifdef CONFIG_REALTEK_RTICE
extern int rtice_enable;
#endif
#include "rtk_sfc.h"

#ifdef CONFIG_NOR_SCRAMBLE_DATA_SUPPORT
#include "rtk.h"
#include "mcp.h"
#include <linux/slab.h>
#include <asm/mach-rtk_dmp/mcp.h>
#endif
#include <mach/system.h>
#include <mach/memory.h>
#include <rbus/reg.h>
/*
 * Mapping drivers for chip access
 */
#undef CONFIG_MTD_COMPLEX_MAPPINGS

static resource_size_t rtk_nor_base_addr;
static resource_size_t rtk_nor_size;

static int rtk_sfc_attach(struct mtd_info *mtd_info);
static int rtk_spirbus_probe(struct platform_device *pdev);
extern struct map_info sfc_physmap_map;

#ifdef CONFIG_NOR_SCRAMBLE_DATA_SUPPORT
static unsigned char gAESKey[16];
static unsigned int mtd_parts_count;
struct sfc_mtd_enc_info {
	u_char *name;
	u_char *part_type;
	uint64_t size;
	uint64_t offset;
	u_char isEncrypted;
};
static struct sfc_mtd_enc_info *sfc_mtd_enc_parts;
#endif

struct sfc_reg_info {
    unsigned int sfc_sck;
    unsigned int sfc_ce;
    unsigned int sfc_wp;
    unsigned int sfc_pos_latch;
};
static struct sfc_reg_info gSFCReg;


#if  defined(CONFIG_REALTEK_USE_HWSEM_AS_SENTINEL )
	#define SFC_USE_DELAY	(0)
#else
	#define SFC_USE_DELAY	(1)
#endif

#ifdef CONFIG_NOR_SUPPORT_MAX_ERASE_SIZE
	#define NOR_SUPPORT_MAX_ERASE_SIZE	(1)
#else
	#define NOR_SUPPORT_MAX_ERASE_SIZE	(0)
#endif

#ifdef CONFIG_NOR_ENTER_4BYTES_MODE
	#define ENTER_4BYTES_MODE	(1)
#else
	#define ENTER_4BYTES_MODE	(0)
#endif

#ifdef CONFIG_NOR_AUTO_HW_POLLING
	#define SFC_HW_POLL			(1)
#else
	#define SFC_HW_POLL			(0)
#endif

#ifdef CONFIG_MTD_RTK_SFC_MULTI_READ
	#define SFC_MULTI_READ		(1)
#else
	#define SFC_MULTI_READ		(0)
#endif

static int rtk_sfc_size;
static volatile unsigned char *FLASH_BASE;
static volatile unsigned char *FLASH_POLL_ADDR;
module_param(rtk_sfc_size, int, S_IRUGO);

static struct mtd_info descriptor;
static rtk_sfc_info_t rtk_sfc_info;
static long sb2_ctrl_value;
static const char *part_probes[] = { "cmdlinepart", "RedBoot", NULL };
static struct rw_semaphore rw_sem;

unsigned int g_isSegErase = 0;
#if SFC_USE_DELAY
static inline void _sfc_delay(void);
#endif

#define delaymicro 50

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
typedef enum {
	INVALID 			= 0,
	READ_ACCESS 		= 0x10000000,
	READ_COMPLETED		= 0x01000000,
	WRITE_ACCESS 		= 0x20000000,
	WRITE_COMPLETED		= 0x02000000,
	ERASE_ACCESS		= 0x40000000,
	ERASE_COMPLETED		= 0x04000000,

	READ_READY		 	= 0x00000001,
	READ_STAGE_1 		= 0x00000002,
	READ_STAGE_2 		= 0x00000004,
	READ_STAGE_3 		= 0x00000008,
	WRITE_READY			= 0x00000100,
	WRITE_STAGE_1		= 0x00000200,
	WRITE_STAGE_2		= 0x00000400,
	WRITE_STAGE_3		= 0x00000800,
	WRITE_STAGE_4		= 0x00001000,
	WRITE_STAGE_5		= 0x00002000,
} PROGRESS_STATUS;

typedef struct _tagDescriptorTable {
	int from;
	int to;
	int length;
	PROGRESS_STATUS status;
} DESCRIPTOR_TABLE;

#define DBG_ENTRY_NUM 256

static int *dbg_counter = NULL;
static DESCRIPTOR_TABLE *dbg_table = NULL;

//---------------------------------------------------------------------------------------------
static inline void add_debug_entry(int from, int to, int length, PROGRESS_STATUS status) {
	if(dbg_counter && dbg_table) {
		if((*dbg_counter) < (DBG_ENTRY_NUM - 1))
			(*dbg_counter)++;
		else
			(*dbg_counter) = 0;

		dbg_table[*dbg_counter].from = from;
		dbg_table[*dbg_counter].to = to;
		dbg_table[*dbg_counter].length = length;
		dbg_table[*dbg_counter].status = status;
	}
}

//---------------------------------------------------------------------------------------------

static inline void change_status(PROGRESS_STATUS status) {
	if(dbg_counter && dbg_table)
		dbg_table[*dbg_counter].status |= status;
}
#endif

//---------------------------------------------------------------------------------------------

#define dma_cache_sync(a,b,c,d) dma_map_single(NULL,b,c,d)

void inline setSSTWrteStutReg(void)//Only for MXIC 256/128 MB . Add by alexchang 1217-2010
{
	unsigned char tmp;	
	/* EWSR: enable write status register */
    SFC_SYNC;
	printk("[%s]\n",__FUNCTION__);
	if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu() || is_macarthur_cpu()) && ((&rtk_sfc_info)->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000850,SFC_OPCODE);
	}
	else
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x0000050, SFC_OPCODE);
	}

#if SFC_USE_DELAY
			_sfc_delay();
#endif

	REG_WRITE_U32(0x00000000, SFC_CTL);
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
    SFC_SYNC;

	tmp = (unsigned char)*((volatile unsigned char*)FLASH_POLL_ADDR);

}

//---------------------------------------------------------------------------------------------
static unsigned int endian_swap(unsigned int x)
{
        x = (x>>24) |((x<<8) & 0x00FF0000) |((x>>8) & 0x0000FF00) |(x<<24);
	return x;
}

//---------------------------------------------------------------------------------------------

static inline void _before_serial_flash_access(void) {
return;
	SFC_SYNC;

	sb2_ctrl_value = REG_READ_U32(SB2_WRAPPER_CTRL);

	REG_WRITE_U32((sb2_ctrl_value & (~0x03)) | 0x8, SB2_WRAPPER_CTRL);

	SFC_SYNC;

	REG_WRITE_U32((sb2_ctrl_value & (~0x03)) | 0xc, SB2_WRAPPER_CTRL);

	SFC_SYNC;
}
//---------------------------------------------------------------------------------------------

static inline void _after_serial_flash_access(void) {
return;
	SFC_SYNC;

	REG_WRITE_U32((sb2_ctrl_value & (~0x04)) | 0x8, SB2_WRAPPER_CTRL);

	SFC_SYNC;

	REG_WRITE_U32(sb2_ctrl_value | 0x8, SB2_WRAPPER_CTRL);

	SFC_SYNC;
}
//---------------------------------------------------------------------------------------------
#if 0
static void setAsyFIFOReg(void)
{
	REG_WRITE_U32(0x15, SFC_FAST_RD);
    REG_WRITE_U32(0x8000, SFC_SCK_TAP);
}
#endif
//---------------------------------------------------------------------------------------------

#if SFC_USE_DELAY
static inline void _sfc_delay(void) {
	udelay(delaymicro);
//        __asm__ __volatile__ ("sync;");
//        if (sched_log_flag & 0x1)
//           log_event(0);
/*
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
        __asm__ __volatile__ ("nop;");
*/
//        if (sched_log_flag & 0x1)
//           log_event(1);

//udelay(delaymicro);
}

#endif
//---------------------------------------------------------------------------------------------

static inline void _switch_to_read_mode(rtk_sfc_info_t *sfc_info, rtk_sfc_read_mode_t mode) {

	unsigned int sfc_opcode;
	unsigned int sfc_ctl;
	unsigned int tmp;

	/* restore opcode to read */
	if (is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu() || is_macarthur_cpu()) {
#if SFC_MULTI_READ
		if ((sfc_info->attr & RTK_SFC_ATTR_SUPPORT_DUAL_O) && (mode == eREAD_MODE_DUAL_FAST_READ)) {
			sfc_opcode = 0x0000023b;
			sfc_ctl = 0x00000019;
		}
		else if ((sfc_info->attr & RTK_SFC_ATTR_SUPPORT_DUAL_IO) && (mode == eREAD_MODE_DUAL_FAST_READ)) {
			sfc_opcode = 0x000004bb;
			sfc_ctl = 0x00000019;
		}
		else {
#endif			
			sfc_opcode = 0x0000000b;
			sfc_ctl = 0x00000019;
#if SFC_MULTI_READ
		}
#endif			
		if (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE) {
			sfc_opcode |= 0x00000800;
		}
	}
	else {
		sfc_opcode = 0x00000003;
		sfc_ctl = 0x00000018;
	}

	SFC_SYNC;
	SYS_REG_TRY_LOCK(0);
	REG_WRITE_U32(sfc_opcode, SFC_OPCODE);
#if SFC_USE_DELAY
	_sfc_delay();
#endif
	REG_WRITE_U32(sfc_ctl, SFC_CTL);
	tmp = *(volatile unsigned int*)(FLASH_BASE);
	SYS_REG_TRY_UNLOCK;
	SFC_SYNC;

	return;
}
//---------------------------------------------------------------------------------------------

static int read_rems(rtk_sfc_info_t *sfc_info, unsigned char *byte1, unsigned char*byte2) {
        SFC_SYNC;
	SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
	REG_WRITE_U32(0x00000090, SFC_OPCODE);	/* Read Electronic Manufacturer & Device ID */
#if SFC_USE_DELAY
			_sfc_delay();
#endif

	REG_WRITE_U32(0x0000001a, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 3 */
	SYS_REG_TRY_UNLOCK;
        SFC_SYNC;
 
    	if(is_saturn_cpu()||is_darwin_cpu() || is_macarthur_cpu())
		*byte1 = *((volatile unsigned char*)(FLASH_POLL_ADDR + 1));
	else
		*byte1 = *((volatile unsigned char*)(FLASH_POLL_ADDR + 0));
    	printk("*byte1=0x%x \n",*byte1);
        SFC_SYNC;
	SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
	REG_WRITE_U32(0x00000090, SFC_OPCODE);	/* Read Electronic Manufacturer & Device ID */
#if SFC_USE_DELAY
			_sfc_delay();
#endif

	REG_WRITE_U32(0x0000001a, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 3 */
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	    SFC_SYNC;
	if(is_saturn_cpu()||is_darwin_cpu() || is_macarthur_cpu())
		*byte2 = *((volatile unsigned char*)(FLASH_POLL_ADDR + 0));
	else
		*byte2 = *((volatile unsigned char*)(FLASH_POLL_ADDR + 1));
	printk("*byte2=0x%x \n",*byte2);

	return 0;
}
//---------------------------------------------------------------------------------------------

#if ENTER_4BYTES_MODE
static int en_4bytes_addr_mode(void) {
	volatile unsigned char tmp2;
        SFC_SYNC;
	SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
	REG_WRITE_U32(0x000000b7, SFC_OPCODE);	/* enter 4-bytes address mode */
#if SFC_USE_DELAY
			_sfc_delay();
#endif

	REG_WRITE_U32(0x00000000, SFC_CTL);
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
        SFC_SYNC;	
	tmp2 = *(volatile unsigned char *)FLASH_POLL_ADDR;
	return 0;

}
//---------------------------------------------------------------------------------------------

static int exit_4bytes_addr_mode(void) {
	volatile unsigned char tmp2;
		SFC_SYNC;
	SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
	REG_WRITE_U32(0x000000e9, SFC_OPCODE);	/* exit 4-bytes address mode */
#if SFC_USE_DELAY
			_sfc_delay();
#endif

	REG_WRITE_U32(0x00000000, SFC_CTL);
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
		SFC_SYNC;	
	tmp2 = *(volatile unsigned char *)FLASH_POLL_ADDR;
	return 0;

}
#endif
//---------------------------------------------------------------------------------------------
/*--------------------------------------------------------------------------------
GD serial flash information list
[GD 25Q16B] 16Mbit
	erase size: 32KB / 64KB
	
[GD 25Q64B] 64Mbit
	erase size: 32KB / 64KB 

[GD 25Q128B] 128Mbit
	erase size: 32KB / 64KB


--------------------------------------------------------------------------------*/
static int gd_init(rtk_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
	case 0x40:
		switch(sfc_info->device_id2) {
		case 0x15:
			printk(KERN_NOTICE "RtkSFC MTD: GD 25Q16B detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x200000;
			break;
		case 0x16:
			printk(KERN_NOTICE "RtkSFC MTD: GD 25Q32B detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x400000;
			break;	
		case 0x17:
			printk(KERN_NOTICE "RtkSFC MTD: GD 25Q64B detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x800000;
			break;
		case 0x18:
			printk(KERN_NOTICE "RtkSFC MTD: GD 25Q128B detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x1000000;
			break;
		default:
			printk(KERN_NOTICE "RtkSFC MTD: GD unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			break;
		}
		break;
	default:
		printk(KERN_NOTICE "RtkSFC MTD: GD unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	if(sfc_info->erase_opcode==0xFFFFFFFF)//Set to default.
	{
		SFC_4KB_ERASE;
	}
	return 0;
}

/*--------------------------------------------------------------------------------
SST serial flash information list
[SST 25VF016B] 16Mbit
	erase size: 4KB / 32KB / 64KB
	
[SST 25VF040B] 4Mbit
	erase size: 4KB / 32KB / 64KB

--------------------------------------------------------------------------------*/
static int sst_init(rtk_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
	case 0x25:
		switch(sfc_info->device_id2) {
		case 0x41:
			printk(KERN_NOTICE "RtkSFC MTD: SST 25VF016B detected.\n");
			sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x200000;
			break;
		case 0x8d:
			printk(KERN_NOTICE "RtkSFC MTD: SST 25VF040B detected.\n");
			sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x80000;
			break;
		default:
			printk(KERN_NOTICE "RtkSFC MTD: SST unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			break;
		}
		break;
	default:
		printk(KERN_NOTICE "RtkSFC MTD: SST unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	if(sfc_info->erase_opcode==0xFFFFFFFF)//Set to default.
	{
		SFC_4KB_ERASE;
	}
	return 0;
}

/*--------------------------------------------------------------------------------
SPANSION serial flash information list
[SPANSION S25FL004A ]
	erase size: 64KB
	
[SPANSION S25FL008A ]
	erase size: 64KB

[SPANSION S25FL016A ]
	erase size: 64KB

[SPANSION S25FL032A ]
	erase size: 64KB

[SPANSION S25FL064A ]
	erase size: 64KB



[SPANSION S25FL128P](256K sector)
	erase size: 64KB / 256KB

[SPANSION S25FL129P](256K sector)
	erase size: 64KB / 256KB
--------------------------------------------------------------------------------*/
static int spansion_init(rtk_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
	case 0x02:
		switch(sfc_info->device_id2) {
		case 0x12:
			printk(KERN_NOTICE "RtkSFC MTD: SPANSION S25FL004A detected.\n");
			sfc_info->sec_64k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x80000;
			break;
		case 0x13:
			printk(KERN_NOTICE "RtkSFC MTD: SPANSION S25FL008A detected.\n");
			sfc_info->sec_64k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x100000;
			break;
		case 0x14:
			printk(KERN_NOTICE "RtkSFC MTD: SPANSION S25FL016A detected.\n");
			sfc_info->sec_64k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x200000;
			break;
		case 0x15:
			printk(KERN_NOTICE "RtkSFC MTD: SPANSION S25FL032A detected.\n");
			sfc_info->sec_64k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x400000;
			break;
		case 0x16:
			printk(KERN_NOTICE "RtkSFC MTD: SPANSION S25FL064A detected.\n");
			sfc_info->sec_64k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x800000;
			break;
		default:
			printk(KERN_NOTICE "RtkSFC MTD: SPANSION unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			break;
		}
		break;
	case 0x20:
		switch(sfc_info->device_id2) {
		case 0x18: // S25FL128P/129P has two types of erase size, must identify via extended device id
			{
				unsigned int tmp2 = 0;
				u8 ext_id1;
				u8 ext_id2;
				SFC_SYNC;
				SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
				REG_WRITE_U32(0x0000009f, SFC_OPCODE);	/* JEDEC Read ID */

				#if SFC_USE_DELAY
				_sfc_delay();
				#endif
				
				REG_WRITE_U32(0x00000013, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 3 */
				SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
				SFC_SYNC;
				tmp2 = *(volatile unsigned int *)FLASH_POLL_ADDR;
				printk("Extension ID : 0x%x\n",tmp2);
				if(is_saturn_cpu()||is_darwin_cpu() || is_macarthur_cpu())
				{
					ext_id1 = (tmp2&0xff000000)>>24;
					ext_id2 = (tmp2&0x00ff0000)>>16;
				}
				else
				{
					ext_id1 = RDID_DEVICE_EID_1(tmp2);
					ext_id2 = RDID_DEVICE_EID_2(tmp2);
				}
				if(ext_id1 == 0x03 && ext_id2 == 0x00) {
					printk(KERN_NOTICE "RtkSFC MTD: SPANSION S25FL128P(256K sector) detected.\n");
					SFC_256KB_ERASE;	
					sfc_info->sec_256k_en = sfc_info->sec_64k_en = SUPPORTED;
					sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
					sfc_info->mtd_info->size = 0x1000000;
				}
				else if(ext_id1 == 0x4d && ext_id2 == 0x00) {
					printk(KERN_NOTICE "RtkSFC MTD: SPANSION S25FL129P(256K sector) detected.\n");
					SFC_256KB_ERASE;
					sfc_info->attr |= RTK_SFC_ATTR_SUPPORT_DUAL_IO;
					sfc_info->sec_256k_en = SUPPORTED;
					sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
					sfc_info->mtd_info->size = 0x1000000;
				}
				else if(ext_id1 == 0x4d && ext_id2 == 0x01) {
					printk(KERN_NOTICE "RtkSFC MTD: SPANSION S25FL129P(64K sector) detected.\n");
					SFC_64KB_ERASE;
					sfc_info->attr |= RTK_SFC_ATTR_SUPPORT_DUAL_IO;
					sfc_info->sec_64k_en = SUPPORTED;
					sfc_info->sec_256k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
					sfc_info->mtd_info->size = 0x1000000;
				}	

				else {
					printk(KERN_NOTICE "RtkSFC MTD: SPANSION S25FL128P or others (64K sector) detected.\n");
					SFC_64KB_ERASE;
					sfc_info->sec_64k_en = SUPPORTED;
					sfc_info->sec_256k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
					sfc_info->mtd_info->size = 0x1000000;
				}
			}
			break;
		default:
			printk(KERN_NOTICE "RtkSFC MTD: SPANSION unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			break;
		}
		break;
	default:
		printk(KERN_NOTICE "RtkSFC MTD: SPANSION unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	if(sfc_info->erase_opcode==0xFFFFFFFF)//Set to default.
	{
		SFC_64KB_ERASE;
	}

	return 0;
}

/*--------------------------------------------------------------------------------
MXIC serial flash information list
[MXIC MX25L4005]
	erase size: 4KB / 64KB
	
[MXIC MX25L8005 / MX25L8006E]
	erase size: 4KB / 64KB

[MXIC MX25L1605]
	erase size: 4KB / 64KB	

[MXIC MX25L3205]
	erase size: 4KB / 64KB

[MXIC MX25L6405D]
	erase size: 4KB / 64KB


[MXIC MX25L6445E]
	erase size: 4KB / 32KB / 64KB

[MXIC MX25L12845E]
	erase size: 4KB / 32KB / 64KB

[MXIC MX25L12805E]
	erase size: 4KB / 64KB

[MXIC MX25L25635E]
	erase size: 4KB / 32KB / 64KB

[MXIC MX25L6455E]
	erase size:  4KB / 32KB / 64KB

[MXIC MX25L12855E]
	erase size: 4KB / 32KB / 64KB

--------------------------------------------------------------------------------*/

static int mxic_init(rtk_sfc_info_t *sfc_info) {
	unsigned char manufacturer_id, device_id;

	switch(sfc_info->device_id1) {
	case 0x20:
		switch(sfc_info->device_id2) {
		case 0x13:
			printk(KERN_NOTICE "RtkSFC MTD: MXIC MX25L4005 detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x80000;
			break;
		case 0x14:
			printk(KERN_NOTICE "RtkSFC MTD: MXIC MX25L8005/MX25L8006E detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x100000;
			break;
		case 0x15:
			printk(KERN_NOTICE "RtkSFC MTD: MXIC MX25L1605 detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x200000;
			break;
		case 0x16:
			printk(KERN_NOTICE "RtkSFC MTD: MXIC MX25L3205 detected.\n");
			SFC_4KB_ERASE;
			sfc_info->attr |= RTK_SFC_ATTR_SUPPORT_DUAL_O;
			sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x400000;
			break;
		case 0x17:
			read_rems(sfc_info, &manufacturer_id, &device_id);
			if(manufacturer_id == 0xc2 && device_id == 0x16) {
				printk(KERN_NOTICE "RtkSFC MTD: MXIC MX25L6445E detected....\n");
				SFC_4KB_ERASE;
				//sfc_info->attr |= RTK_SFC_ATTR_SUPPORT_DUAL_IO;				
				sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
				sfc_info->sec_256k_en = NOT_SUPPORTED;
			}
			else {
				printk(KERN_NOTICE "RtkSFC MTD: MXIC MX25L6405D detected.\n");
				SFC_4KB_ERASE;
				sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
				sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			}
			sfc_info->mtd_info->size = 0x800000;
			break;
		case 0x18:
			read_rems(sfc_info, &manufacturer_id, &device_id);
			if(manufacturer_id == 0xc2 && device_id == 0x17) {
				printk(KERN_NOTICE "RtkSFC MTD: MXIC MX25L12845E detected.\n");
				SFC_4KB_ERASE;
				sfc_info->attr |= RTK_SFC_ATTR_SUPPORT_DUAL_IO|RTK_SFC_ATTR_SUPPORT_DUAL_O;
				sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
				sfc_info->sec_256k_en = NOT_SUPPORTED;
			}
			else {
				printk(KERN_NOTICE "RtkSFC MTD: MXIC MX25L12805 detected.\n");
				SFC_4KB_ERASE;
				sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
				sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			}
			sfc_info->mtd_info->size = 0x1000000;
			break;
		case 0x19:
			read_rems(sfc_info, &manufacturer_id, &device_id);
			if(manufacturer_id == 0xc2 && device_id == 0x18) {
				printk(KERN_NOTICE "RtkSFC MTD: MXIC MX25L25635E detected.\n");


				SYS_REG_TRY_LOCK(0);//add by alexchang 0722-200

				REG_WRITE_U32(0x0000002b, SFC_OPCODE);	
			
		        REG_WRITE_U32(0x00000010, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 0 */
		        SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
			SFC_SYNC;
		 		if ((*(volatile unsigned char *)FLASH_POLL_ADDR) 
					& 0x4) 
		 		{
					#if ENTER_4BYTES_MODE
					exit_4bytes_addr_mode();
					#else
					
					#endif
		 		}
				SFC_4KB_ERASE;
				sfc_info->attr |= RTK_SFC_ATTR_SUPPORT_DUAL_IO;
				sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
				sfc_info->sec_256k_en = NOT_SUPPORTED;
				sfc_info->mtd_info->size = 0x2000000;
				
				/* enter 4-byte address mode */
				/* cuz watchdog reset issue , we dont 
				   support yet */
			#if ENTER_4BYTES_MODE
				en_4bytes_addr_mode();
				sfc_info->attr |= RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE;
			#endif

			}
			else {
				printk(KERN_NOTICE "RtkSFC MTD: MXIC unknown mnftr_id=0x%x, dev_id=0x%x .\n", manufacturer_id, device_id) ;
				SFC_4KB_ERASE;
				sfc_info->sec_4k_en = SUPPORTED;
				sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			}
			break;
			
		default:
			printk(KERN_NOTICE "RtkSFC MTD: MXIC unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			SFC_4KB_ERASE;
			sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			break;
		}
		break;
		case 0x26:////add by alexchang 1206-2010
			switch(sfc_info->device_id2) {
				case 0x17:
				printk(KERN_NOTICE "RtkSFC MTD: MXIC MX25L6455E detected.\n");
				SFC_4KB_ERASE;
				sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
				sfc_info->sec_256k_en = NOT_SUPPORTED;
				sfc_info->mtd_info->size = 0x800000;
				break;
		
				case 0x18:
				printk(KERN_NOTICE "RtkSFC MTD: MXIC MX25L12855E detected.\n");
				SFC_4KB_ERASE;
				sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
				sfc_info->sec_256k_en = NOT_SUPPORTED;
				sfc_info->mtd_info->size = 0x1000000;
				break;
				
			default:
				printk(KERN_NOTICE "RtkSFC MTD: MXIC unknown id2=0x%x detected.\n",
								sfc_info->device_id2);
				break;
			}
		break;
	default:
		printk(KERN_NOTICE "RtkSFC MTD: MXIC unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	
	if(sfc_info->erase_opcode==0xFFFFFFFF)//Set to default.
	{
		SFC_64KB_ERASE;
	}

	return 0;

}
/*--------------------------------------------------------------------------------
PMC serial flash information list

--------------------------------------------------------------------------------*/

static int pmc_init(rtk_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
	case 0x9d:
		switch(sfc_info->device_id2) {
		case 0x7c:
			printk(KERN_NOTICE "RtkSFC MTD: PMC Pm25LV010 detected.\n");
			sfc_info->mtd_info->size = 0x20000;
			break;
		case 0x7d:
			printk(KERN_NOTICE "RtkSFC MTD: PMC Pm25LV020 detected.\n");
			sfc_info->mtd_info->size = 0x40000;
			break;
		case 0x7e:
			printk(KERN_NOTICE "RtkSFC MTD: PMC Pm25LV040 detected.\n");
			sfc_info->mtd_info->size = 0x80000;
			break;
		case 0x46:
			printk(KERN_NOTICE "RtkSFC MTD: PMC Pm25LQ032 detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x400000;
			break;		
		default:
			printk(KERN_NOTICE "RtkSFC MTD: PMC unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			break;
		}
		break;
	default:
		printk(KERN_NOTICE "RtkSFC MTD: PMC unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	sfc_info->erase_size	= 0x1000;	//4KB
	sfc_info->erase_opcode	= 0x000000d7;	//for 4KB erase.
	return 0;
}

/*--------------------------------------------------------------------------------
STM serial flash information list
[ST M25P128]
	erase size: 2MB

[ST M25Q128]

[STM N25Q032]
	erase size: 4KB / 64KB

[STM N25Q064]
	erase size: 4KB / 64KB
--------------------------------------------------------------------------------*/

static int stm_init(rtk_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
		case 0x20:
			switch(sfc_info->device_id2) {
				case 0x17:
					printk(KERN_NOTICE "RtkSFC MTD: ST M25P64 detected.\n");
					SFC_64KB_ERASE;
					sfc_info->sec_64k_en = SUPPORTED;
					sfc_info->sec_256k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
					sfc_info->mtd_info->size = 0x1000000;
					break;
				case 0x18:
					printk(KERN_NOTICE "RtkSFC MTD: ST M25P128 detected.\n");
					SFC_256KB_ERASE;
					sfc_info->sec_256k_en = SUPPORTED;
					sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
					sfc_info->mtd_info->size = 0x1000000;
					break;
				default:
					printk(KERN_NOTICE "RtkSFC MTD: ST unknown id2=0x%x detected.\n",	sfc_info->device_id2);
					SFC_64KB_ERASE;
					sfc_info->sec_64k_en = SUPPORTED;
					sfc_info->sec_256k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
					break;
			}
			break;

		case 0xba:
			switch(sfc_info->device_id2) {
				case 0x16:
					printk(KERN_NOTICE "RtkSFC MTD: ST N25Q032 detected.\n");
					SFC_4KB_ERASE;
					sfc_info->sec_4k_en = SUPPORTED;
					sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
					sfc_info->mtd_info->size = 0x400000;
					break;
				case 0x17:
					printk(KERN_NOTICE "RtkSFC MTD: ST N25Q064 detected.\n");
					SFC_4KB_ERASE;
					sfc_info->sec_4k_en = SUPPORTED;
					sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
					sfc_info->mtd_info->size = 0x800000;
					break;
				case 0x18:
					printk(KERN_NOTICE "RtkSFC MTD: ST N25Q128 detected.\n");
					SFC_64KB_ERASE;
					sfc_info->sec_64k_en = SUPPORTED;
					sfc_info->sec_256k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
					sfc_info->mtd_info->size = 0x1000000;
					break;
				default:
					printk(KERN_NOTICE "RtkSFC MTD: ST unknown id2=0x%x detected.\n",	sfc_info->device_id2);
					SFC_64KB_ERASE;
					sfc_info->sec_64k_en = SUPPORTED;
					sfc_info->sec_256k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
					break;
			}
		break;
		default:
			printk(KERN_NOTICE "RtkSFC MTD: ST unknown id1=0x%x detected.\n",	sfc_info->device_id1);
			SFC_64KB_ERASE;
			sfc_info->sec_64k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
			break;
	}
	if(sfc_info->erase_opcode==0xFFFFFFFF)//Set to default.
	{
		SFC_64KB_ERASE;
	}

	return 0;
}


/*--------------------------------------------------------------------------------
EON serial flash information list
[EON EN25B64-100FIP]64Mbits
	erase size: 64KB

[EON EN25F16]
	erase size: 4KB / 64KB
	
[EON EN25Q64]
	erase size: 4KB 


[EON EN25Q128]
	erase size: 4KB / 64KB
--------------------------------------------------------------------------------*/
static int eon_init(rtk_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
	case 0x20:
		switch(sfc_info->device_id2) {
		case 0x17:
			printk(KERN_NOTICE "RtkSFC MTD: EON EN25B64-100FIP detected.\n");
			SFC_64KB_ERASE;
			sfc_info->sec_64k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x800000;
			g_isSegErase = 1;
			break;
		default:
			printk(KERN_NOTICE "RtkSFC MTD: EON unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			SFC_4KB_ERASE;
			sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			break;
		}
		return 0;

	case 0x31:
		switch(sfc_info->device_id2) {
		case 0x15:
			printk(KERN_NOTICE "RtkSFC MTD: EON EN25F16 detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x200000;
			break;
		default:
			printk(KERN_NOTICE "RtkSFC MTD: EON unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			break;
		}
		return 0;
	case 0x30:
		switch(sfc_info->device_id2) {
		case 0x16:
			printk(KERN_NOTICE "RtkSFC MTD: EON EN25Q32 detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x400000;
			break;			
		case 0x17:
			printk(KERN_NOTICE "RtkSFC MTD: EON EN25Q64 detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x800000;
			break;			
		case 0x18:
			printk(KERN_NOTICE "RtkSFC MTD: EON EN25Q128 detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x1000000;
			break;
		default:
			printk(KERN_NOTICE "RtkSFC MTD: EON unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			break;
		}
		return 0;
	case 0x70: 
		switch(sfc_info->device_id2) {
		case 0x18:
			printk(KERN_NOTICE "RtkSFC MTD: EON EON_EN25QH16128A detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x1000000;
			break;
		default:
			printk(KERN_NOTICE "RtkSFC MTD: EON unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			break;
		}
		return 0;
	default:
		printk(KERN_NOTICE "RtkSFC MTD: EON unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	if(sfc_info->erase_opcode==0xFFFFFFFF)//Set to default.
	{
		SFC_64KB_ERASE;
	}

	return 0;
}

/*--------------------------------------------------------------------------------
ATMEL serial flash information list
[ATMEL AT25DF641A]64Mbits
	erase size: 64KB

[ATMEL AT25DF321A]
	erase size: 4KB / 64KB
	

--------------------------------------------------------------------------------*/
static int atmel_init(rtk_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
	case 0x48:
	case 0x86:
		switch(sfc_info->device_id2) {
		case 0x1:
		case 0x0:
			printk(KERN_NOTICE "RtkSFC MTD: AT25DF641A detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x800000;
			break;
		default:
			printk(KERN_NOTICE "RtkSFC MTD: AT25DF641A unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			SFC_4KB_ERASE;
			sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			break;
		}
		return 0;

	case 0x47:
		switch(sfc_info->device_id2) {
		case 0x1:
		case 0x0:			
			printk(KERN_NOTICE "RtkSFC MTD: AT25DF321A detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x400000;
			break;
		default:
			printk(KERN_NOTICE "RtkSFC MTD: AT25DF321A like unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			SFC_4KB_ERASE;
			sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x400000;
			break;
		}
		return 0;
 
	default:
		printk(KERN_NOTICE "RtkSFC MTD: EON unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	if(sfc_info->erase_opcode==0xFFFFFFFF)//Set to default.
	{
		SFC_4KB_ERASE;
	}

	return 0;
}

//---------------------------------------------------------------------------------------------
inline size_t sfc_memcpy(void *dst, void *src, size_t len) {	
  int i;	
  size_t len_align;	
  if ((((unsigned int) dst & 0x3) == 0) && (((unsigned int) src & 0x03) == 0) ) {
  	len_align = len & ~((unsigned int) 0x03);		
	for(i = 0; i < len_align; i += 4) {			
		*((u32*) ((u32)dst+i) ) = *((u32*)((u32)src+i));		
	}		
	for(; i < len; i ++) {			
		*((u8*) ((u32)dst+i) ) = *((u8*)((u32)src+i));		
	}						
	return len;	
  } else if ((((unsigned int) dst & 0x1) == 0) && (((unsigned int) src & 0x01) == 0)) {		
    	len_align = len & ~((unsigned int) 0x01);		
      for(i = 0; i < len_align; i += 2) {			
	  	*((u16*) ((u32)dst+i) ) = *((u16*)((u32)src+i));		
      }
      for(; i < len; i ++) {			
	  	*((u8*) ((u32)dst+i) ) = *((u8*)((u32)src+i));		
	}					
  } else {		
      for(i=0; i < len; i ++) {			
	  	*((u8*) ((u32)dst+i) ) = *((u8*)((u32)src+i));		
	}					
  }	
   return len; 
}
static int _sfc_read_bytes(struct mtd_info *mtd, loff_t from, size_t len,
						size_t *retlen, const u_char *buf) {

	_switch_to_read_mode((rtk_sfc_info_t*)mtd->priv, eREAD_MODE_SINGLE_FAST_READ);

	*retlen = sfc_memcpy(buf, (FLASH_BASE + from), len);	

	return 0;
}
//---------------------------------------------------------------------------------------------

/*--------------------------------------------------------------------------------
WINBOND serial flash information list
[WINBOND 25Q128BVFG]
	erase size: 

[WINBOND W25Q32BV]32 Mbits
	erase size:4KB /32KB /64KB

[SPANSION S25FL064K ] 64Mbits //SPANSION brand, Winbond OEM
	erase size: 4KB / 32KB / 64KB

--------------------------------------------------------------------------------*/
static int winbond_init(rtk_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
	case 0x40:
		switch(sfc_info->device_id2) {
		case 0x14:
			printk(KERN_NOTICE "RtkSFC MTD: WINBOND W25Q80BV detected.\n");
			SFC_4KB_ERASE;
			sfc_info->attr |= RTK_SFC_ATTR_SUPPORT_DUAL_IO|RTK_SFC_ATTR_SUPPORT_DUAL_O;
			sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x100000;
			break;
		case 0x19:
			printk(KERN_NOTICE "RtkSFC MTD: WINBOND S25FL256K detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_4k_en = sfc_info->sec_32k_en = sfc_info->sec_64k_en = SUPPORTED;
			sfc_info->sec_256k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x2000000;
			//louis
			#if 0
			FLASH_BASE = (unsigned char*)0xbdc00000;	
			FLASH_POLL_ADDR = (unsigned char*)0xbec00000;
			#endif
			break;
		case 0x16:
			printk(KERN_NOTICE "RtkSFC MTD: WINBOND W25Q32BV(W25Q32FV) detected.\n");
			SFC_4KB_ERASE;
			sfc_info->attr |= RTK_SFC_ATTR_SUPPORT_DUAL_IO|RTK_SFC_ATTR_SUPPORT_DUAL_O;
			sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x400000;
			break;
		case 0x17:
			printk(KERN_NOTICE "RtkSFC MTD: SPANSION S25FL064K detected.\n");
			SFC_4KB_ERASE;
			sfc_info->attr |= RTK_SFC_ATTR_SUPPORT_DUAL_IO|RTK_SFC_ATTR_SUPPORT_DUAL_O;			
			sfc_info->sec_4k_en = sfc_info->sec_32k_en = sfc_info->sec_64k_en = SUPPORTED;
			sfc_info->sec_256k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x800000;
			break;
		case 0x18:
			printk(KERN_NOTICE "RtkSFC MTD: WINBOND 25Q128BVFG(W25Q128BVFIG) detected.\n");
			SFC_4KB_ERASE;
			sfc_info->attr |= RTK_SFC_ATTR_SUPPORT_DUAL_IO|RTK_SFC_ATTR_SUPPORT_DUAL_O;
			sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x1000000;
			break;
		default:
			printk(KERN_NOTICE "RtkSFC MTD: WINBOND unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			SFC_4KB_ERASE;
			sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;

			break;
		}
		return 0;
	case 0x60:
		switch(sfc_info->device_id2) {
		case 0x16:
			printk(KERN_NOTICE "RtkSFC MTD: WINBOND W25Q32FV(Quad Mode) detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x400000;			
			break;
		default:
			printk(KERN_NOTICE "RtkSFC MTD: WINBOND unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			SFC_4KB_ERASE;
			sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;

			break;
		}
		return 0;
	default:
		printk(KERN_NOTICE "RtkSFC MTD: WINBOND unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	SFC_4KB_ERASE;
	return 0;
}
/*--------------------------------------------------------------------------------
ESMT serial flash information list
[ESMT F25L32PA]32 Mbits
	erase size:4KB / 64KB
[ESMT F25L64QA]64 Mbits
	erase size:4KB / 32KB / 64KB




--------------------------------------------------------------------------------*/
static int esmt_init(rtk_sfc_info_t *sfc_info) {
	switch(sfc_info->device_id1) {
	case 0x20:
		switch(sfc_info->device_id2) {
		case 0x16:
			printk(KERN_NOTICE "RtkSFC MTD: ESMT F25L32PA detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x400000;
			break;
			
		default:
			printk(KERN_NOTICE "RtkSFC MTD: ESMT unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			SFC_4KB_ERASE;
			sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;

			break;
		}
	break;
	case 0x40:
		switch(sfc_info->device_id2) {
		case 0x16:
			printk(KERN_NOTICE "RtkSFC MTD: ESMT F25L32PA detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x400000;
			break;
			
		default:
			printk(KERN_NOTICE "RtkSFC MTD: ESMT unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			SFC_4KB_ERASE;
			sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;

			break;
		}
	break;
	case 0x41:
		switch(sfc_info->device_id2) {
		case 0x16:
			printk(KERN_NOTICE "RtkSFC MTD: ESMT F25L32QA detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x400000;
			break;

		case 0x17:
			printk(KERN_NOTICE "RtkSFC MTD: ESMT F25L64QA detected.\n");
			SFC_4KB_ERASE;
			sfc_info->sec_64k_en = sfc_info->sec_32k_en = sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = NOT_SUPPORTED;
			sfc_info->mtd_info->size = 0x800000;
			break;
			
		default:
			printk(KERN_NOTICE "RtkSFC MTD: ESMT unknown id2=0x%x detected.\n",
							sfc_info->device_id2);
			SFC_4KB_ERASE;
			sfc_info->sec_4k_en = SUPPORTED;
			sfc_info->sec_256k_en = sfc_info->sec_64k_en = sfc_info->sec_32k_en = NOT_SUPPORTED;

			break;
		}
	break;
	default:
		printk(KERN_NOTICE "RtkSFC MTD: ESMT unknown id1=0x%x detected.\n",
							sfc_info->device_id1);
		break;
	}
	if(sfc_info->erase_opcode==0xFFFFFFFF)//Set to default.
	{
		SFC_64KB_ERASE;
	}

	return 0;
}

//---------------------------------------------------------------------------------------------

static int _sfc_read_md(struct mtd_info *mtd, loff_t from, size_t len,
					size_t *retlen, const u_char *buf) {

	volatile u8 *src = (volatile u8*)(SPI_RBUS_PHYS + from);
	
#if !SATURN_RW_MD
	u8 true_data_buf[MD_PP_DATA_SIZE*2];
	u8 *dest = (u8*)buf;
	u8 *data_buf;
	u32 read_count = 0;
#endif
	u32 val;
	int ret;
#ifdef CONFIG_MTD_RTK_MD_READ_COMPARE
	u8 *org_src = (u8*)buf;
	unsigned int org_from = from;
	size_t org_len = len;
retry_mdread:
#endif

#ifdef DEV_DEBUG
	printk(KERN_WARNING "Rtk SFC MTD: %s() $ [%08X => %08X (virt: %08X) : %08X]\n", __func__, (int)src, (int)(virt_to_phys(buf)), buf, (int)len);
#endif

	if(unlikely((len <= RTK_SFC_SMALL_PAGE_WRITE_MASK) || (len & RTK_SFC_SMALL_PAGE_WRITE_MASK)))
		BUG();
#if SATURN_RW_MD//Enable new MD mechanism
	//flush dirty cache lines
	//dma_cache_sync(NULL,(u8 *)buf, len, DMA_FROM_DEVICE);
	//gio_md_read(1);

	_switch_to_read_mode((rtk_sfc_info_t*)mtd->priv, eREAD_MODE_DUAL_FAST_READ);
	
	//setup MD DDR addr and flash addr
	REG_WRITE_U32(((unsigned int)buf), MD_FDMA_DDR_SADDR);
	REG_WRITE_U32(((unsigned int)src), MD_FDMA_FL_SADDR);
	//setup MD direction and move data length
	val = (0x2C000000 | len);		//do swap
	REG_WRITE_U32(val, MD_FDMA_CTRL2); 	//for dma_length bytes.
	//flush dirty cache lines
	dma_cache_sync(NULL,(u8 *)buf, len, DMA_FROM_DEVICE);
	mb();
	SFC_SYNC;
	//go 
	REG_WRITE_U32(0x03, MD_FDMA_CTRL1);
	
	/* wait for MD done its operation */
	while((ret = REG_READ_U32(MD_FDMA_CTRL1)) & 0x1);


	*retlen = len;
//#ifdef CONFIG_MTD_RTK_MD_READ_COMPARE 
#if 0

        gio_md_read(0);
	if (memcmp((void *) (FLASH_BASE + org_from), (void *) org_src, org_len)) {
	    	md_fail(1);
		printk("md read error at buf=%x %x len=%x\n", org_src, (FLASH_BASE + org_from), org_len);	
		printk("\nsrc: ");
		int i;
		for (i=0; i < org_len; i++) {
			printk("%02x ", org_src[i]);
			if (i%8 == 7) printk("\n");
		}
		printk("\ndest: ");
		for (i=0; i < org_len; i++) {
			printk("%02x ",  ((unsigned char *)(FLASH_BASE + org_from))[i]);
			if (i%8 == 7) printk("\n");
		}	
		while(1) {
			if (memcmp((void *) (FLASH_BASE + org_from), (void *) (FLASH_BASE + org_from), org_len)) {
				printk("SPI flash self compare error\n");
			}
			msleep(3000);
		}		
		goto retry_mdread;
	}
#endif		
#else
	// workaround to avoid mantis issue #10822:
	// while writing less than 128 bytes and DDR address is 0x0 - 0x7f, 
	// change DDR address to any address larger than 0x7f for avoidance
	if(len < 128 && ((int)(true_data_buf) & 0x07ffffff) <= 0x7f)
		data_buf = &(true_data_buf[MD_PP_DATA_SIZE]);
	else
		data_buf = true_data_buf;

	while(len > 0) {
		size_t dma_length;
		if(len >= MD_PP_DATA_SIZE)
			dma_length = MD_PP_DATA_SIZE;
		else
			dma_length = len;

		//flush dirty cache lines
		dma_cache_sync(NULL,data_buf, dma_length, DMA_BIDIRECTIONAL);

		_switch_to_read_mode((rtk_sfc_info_t*)mtd->priv, eREAD_MODE_DUAL_FAST_READ);

		//setup MD DDR addr and flash addr
		REG_WRITE_U32(((unsigned int)dest), MD_FDMA_DDR_SADDR);
		REG_WRITE_U32(((unsigned int)src), MD_FDMA_FL_SADDR);
		//setup MD direction and move data length
		val = (0x2C000000 | dma_length);        //do swap
		REG_WRITE_U32(val, MD_FDMA_CTRL2);		//for dma_length bytes.

		mb();
		SFC_SYNC;
		//go 
		REG_WRITE_U32(0x03, MD_FDMA_CTRL1);

		/* wait for MD done its operation */
		while((ret = REG_READ_U32(MD_FDMA_CTRL1)) & 0x1);
#if 0
		/* data moved by MD module is endian inverted 
		 * revert it after read from flash */
		for(i = 0 ; i < dma_length ; i += 4) {
			val = *((u32*)(data_buf + i));
			*((volatile u32*)(dest + i)) = 
	#if 0
			((val >> 24) |
			 ((val >> 8 ) & 0x0000ff00) |
			 ((val << 8 ) & 0x00ff0000) |
			 (val << 24));
	#else
			val;
	#endif

		}		
			
#endif
		src += dma_length;
		dest += dma_length;
		read_count += dma_length;
		len -= dma_length;
	}

	*retlen = read_count;
#endif

	return 0;
}
//---------------------------------------------------------------------------------------------

static int rtk_sfc_read_pages(struct mtd_info *mtd, loff_t from, size_t len,
						size_t *retlen, u_char *buf) {
	rtk_sfc_info_t *sfc_info;
	size_t read_len = 0;
	size_t rlen = 0;
	size_t length_to_read;
	unsigned int buf_addr = 0;
#ifdef CONFIG_MTD_RTK_MD_READ_COMPARE
	u8 *org_src = (u8*)buf;
	unsigned int org_from = from;
	size_t org_len = len;
retry_read:
#endif

#ifdef CONFIG_NOR_SCRAMBLE_DATA_SUPPORT
	u8 *cp_sa = NULL, *cp_da = NULL;
	u8 *cp_org_buff = (u8*)buf;
	unsigned int cp_org_from = from;
	size_t cp_org_len = len, cp_offset = 0;
	unsigned int i;
	unsigned int alloc_da_flag = 0;
	unsigned int decrypt_flag = 0;
#endif

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	add_debug_entry(from, buf, len, READ_ACCESS);
#endif

	if((sfc_info = (rtk_sfc_info_t*)mtd->priv) == NULL)
		return -EINVAL;

	down_write(&rw_sem);

	buf_addr = buf;
	SFC_FLUSH_CACHE(buf_addr,len);

#ifdef CONFIG_NOR_SCRAMBLE_DATA_SUPPORT
	for (i=0;i<mtd_parts_count;i++)
	{
		if ((from >= (sfc_mtd_enc_parts+i)->offset) && (from < (sfc_mtd_enc_parts+i)->offset+(sfc_mtd_enc_parts+i)->size))
		{
			if ((sfc_mtd_enc_parts+i)->isEncrypted == 1)
				decrypt_flag = 1;
			break;
		}
	}

	if (decrypt_flag)
	{
		from = cp_org_from&(~0xf);
		len = (cp_org_from+cp_org_len+15)/16*16-from;
		cp_offset = cp_org_from-from;

		if ((from != cp_org_from) || (len != cp_org_len))
		{
			alloc_da_flag = 1;
			printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_read() before allign [%08X:%08X] after allign [%08X:%08X] offset=0x%X\r\n", (int)cp_org_from, (int)cp_org_len, (int)from, (int)len, cp_offset);
			if ((cp_da = (u8*)kmalloc(len , GFP_KERNEL)) == NULL)
				return -EINVAL;
		}
		else
		{
			cp_da = cp_org_buff;
		}
		if ((cp_sa = (u8*)kmalloc(len , GFP_KERNEL)) == NULL) {
			if (cp_da)
				kfree(cp_da);
			return -EINVAL;
		}
		buf = (u_char *)cp_sa;
	}
#endif
#ifdef	CONFIG_REALTEK_RTICE
        if (rtice_enable > 1) {
                printk("<MTD> %s %d\n", __FUNCTION__, __LINE__);
        }
#endif		
	/* Set SB2 to in-order mode */
	_before_serial_flash_access();

	*retlen = 0;

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(READ_READY);
#endif
	// 1st pass: read data 0-3 bytes 
	if(len > 0 && (from & RTK_SFC_SMALL_PAGE_WRITE_MASK)) {
		if(len > RTK_SFC_SMALL_PAGE_WRITE_MASK)
			length_to_read = (RTK_SFC_SMALL_PAGE_WRITE_MASK + 1) - (from & RTK_SFC_SMALL_PAGE_WRITE_MASK);
		else
			length_to_read = len;

		_sfc_read_bytes(mtd, from, length_to_read, &rlen, buf);

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_read() $ PASS #1 $ [%08X:%08X]\n", (int)from, (int)len);
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_read() $ PASS #1 $ %d bytes read\n", (int)rlen);
#endif

		from += rlen;
		read_len += rlen;
		len -= rlen;
		buf += rlen;
	}

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(READ_STAGE_1);
#endif
	// 2rd pass: using MD to read data which aligned in 4 bytes
	if(len > RTK_SFC_SMALL_PAGE_WRITE_MASK && ((((unsigned int)buf) & RTK_SFC_SMALL_PAGE_WRITE_MASK)==0)  && 
		((from &  RTK_SFC_SMALL_PAGE_WRITE_MASK)==0)) 
	{
		length_to_read = len & (~RTK_SFC_SMALL_PAGE_WRITE_MASK);

		_sfc_read_md(mtd, from, length_to_read, &rlen, buf);

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_read() $ PASS #2 $ [%08X:%08X]\n", (int)from, (int)len);
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_read() $ PASS #2 $ %d bytes read\n", (int)rlen);
#endif

		from += rlen;
		read_len += rlen;
		len -= rlen;
		buf += rlen;
	}

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(READ_STAGE_2);
#endif

	// 3rd pass: read rest data (0-3 bytes)
	if(len > 0) {
		length_to_read = len;

    	dma_cache_sync(NULL,buf, len, DMA_FROM_DEVICE);
		_sfc_read_bytes(mtd, from, length_to_read, &rlen, buf);

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_read() $ PASS #3 $ [%08X:%08X]\n", (int)from, (int)len);
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_read() $ PASS #3 $ %d bytes read\n", (int)rlen);
#endif

		from += rlen;
		read_len += rlen;
		len -= rlen;
		buf += rlen;
	}

	*retlen = read_len;

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(READ_STAGE_3);
#endif
	/* Restore SB2 */
	_after_serial_flash_access();
#if 0
//#ifdef CONFIG_MTD_RTK_MD_READ_COMPARE 
	if (memcmp((void *) (FLASH_BASE + org_from), (void *) org_src, org_len)) {
	    	printk("read error at buf=%x %x len=%x\n", org_src, (FLASH_BASE + org_from), org_len);	
		printk("\nsrc: ");
		int i;
		for (i=0; i < org_len; i++) {
			printk("%02x ", org_src[i]);
			if (i%8 == 7) printk("\n");
		}
		printk("\ndest: ");
		for (i=0; i < org_len; i++) {
			printk("%02x ",  ((unsigned char *)(FLASH_BASE + org_from))[i]);
			if (i%8 == 7) printk("\n");
		}			
		goto retry_read;
	}
#endif	

#ifdef CONFIG_NOR_SCRAMBLE_DATA_SUPPORT
	if (decrypt_flag)
	{
		if (read_len&0xf)
			printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_read() read length %u not alligned to 16\r\n", (int)read_len);

		MCP_AES_ECB_Decryption(gAESKey, cp_sa, cp_da, read_len);
		kfree(cp_sa);
		if(alloc_da_flag) {
			memcpy(cp_org_buff, &cp_da[cp_offset], read_len);
			kfree(cp_da);
		}
		if (read_len >= cp_org_len)
			*retlen = cp_org_len;
		else
			*retlen = read_len;		
	}
#endif
	up_write(&rw_sem);

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(READ_COMPLETED);
#endif
	return 0;
};

//---------------------------------------------------------------------------------------------

static int rtk_sfc_read(struct mtd_info *mtd, loff_t from, size_t len,
						size_t *retlen, u_char *buf) {
	rtk_sfc_info_t *sfc_info;

	printk("<MTD> %s %d\n", __FUNCTION__, __LINE__);
#ifdef CONFIG_REALTEK_RTICE
	if (rtice_enable > 1) {
		printk("<MTD> %s %d\n", __FUNCTION__, __LINE__);
	}
#endif
#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	add_debug_entry(from, buf, len, READ_ACCESS);
#endif

	if((sfc_info = (rtk_sfc_info_t*)mtd->priv) == NULL)
		return -EINVAL;
	if(!buf)
		return -EINVAL;

		
	
	down_write(&rw_sem);

	SFC_FLUSH_CACHE((unsigned long) &buf,len);
#ifdef DEV_DEBUG 
	printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_read(), from = %08X, length = %08X\n", (int)from, (int)len);
#endif

	/* Set SB2 to in-order mode */
	_before_serial_flash_access();

	*retlen = 0;

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(READ_READY);
#endif

	_sfc_read_bytes(mtd, from, len, retlen, buf);

	/* Restore SB2 */
	_after_serial_flash_access();

	up_write(&rw_sem);

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(READ_COMPLETED);
#endif

	return 0;
}
//---------------------------------------------------------------------------------------------

static int _sfc_write_bytes(struct mtd_info *mtd, loff_t to, size_t len,
					size_t *retlen, const u_char *buf) {
	u8 *src = (u8*)buf;
	volatile u8 *dest = (volatile u8*)(FLASH_BASE + to);
	u32 written_count = 0;

	rtk_sfc_info_t *sfc_info = (rtk_sfc_info_t*)mtd->priv;
#ifdef CONFIG_REALTEK_RTICE
	if (rtice_enable > 1) {
		printk("<MTD> %s %d\n", __FUNCTION__, __LINE__);
	}
#endif	

        // write enable
    	REG_WRITE_U32(0x00000006,SFC_OPCODE);
    	REG_WRITE_U32(0x00000000,SFC_CTL);
    	SFC_SYNC;

    	// disable write protect
    	REG_WRITE_U32(0x00000000,SFC_WP);
     
    	// enable write status register
    	REG_WRITE_U32(0x00000050,SFC_OPCODE);
    	REG_WRITE_U32(0x00000000,SFC_CTL);
    	SFC_SYNC;
 
    	// write status register , no memory protection
    	REG_WRITE_U32(0x00000001,SFC_OPCODE);
    	REG_WRITE_U32(0x00000010,SFC_CTL);
    	SFC_SYNC;
 
	REG_WRITE_U32(0x00000106,SFC_EN_WR);
        REG_WRITE_U32(0x00000105,SFC_WAIT_WR);
        REG_WRITE_U32(0x00ffffff,SFC_CE);

        SFC_SYNC;
	SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
	REG_WRITE_U32(0x00000002, SFC_OPCODE);	/* Byte Programming */
#if SFC_USE_DELAY
	_sfc_delay();
#endif

	REG_WRITE_U32(0x00000018, SFC_CTL);	/* dataen = 1, adren = 1, dmycnt = 0 */
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
        SFC_SYNC;

	while(len--) {
		if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu() || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) {
			/* send write enable first */
            		SFC_SYNC;
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000802, SFC_OPCODE);	/* Byte Programming */
#if SFC_USE_DELAY
			_sfc_delay();
#endif

			REG_WRITE_U32(0x00000018, SFC_CTL);	/* dataen = 1, adren = 1, dmycnt = 0 */
			SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	        	SFC_SYNC;
		}
		else 
 		{
			/* send write enable first */
            		SFC_SYNC;
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000006, SFC_OPCODE);	/* Write enable */
#if SFC_USE_DELAY
			_sfc_delay();
#endif
			REG_WRITE_U32(0x00180000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
			SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
		    	SFC_SYNC;
		
            		SFC_SYNC;
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000002, SFC_OPCODE);	/* Byte Programming */
#if SFC_USE_DELAY
			_sfc_delay();
#endif
			REG_WRITE_U32(0x00000018, SFC_CTL);	/* dataen = 1, adren = 1, dmycnt = 0 */
			SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	        	SFC_SYNC;
		}

		*dest++ = *src++;
		mb();
#if !SFC_HW_POLL
		/* using RDSR to make sure the operation is completed. */
		do {
            		SFC_SYNC;
			if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu() || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
			{
				SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
				REG_WRITE_U32(0x00000805, SFC_OPCODE);	/* RDSR */
			}
			else
			{
				SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
				REG_WRITE_U32(0x00000005, SFC_OPCODE);	/* RDSR */
			}
#if SFC_USE_DELAY 
			_sfc_delay();
#endif

			REG_WRITE_U32(0x00000010, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 0 */
			SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
		    	SFC_SYNC;
		} while((*(volatile unsigned char *)FLASH_POLL_ADDR) & 0x1);
#endif
		written_count++;
	}
	/* send write disable then */
    	SFC_SYNC;
	if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000804, SFC_OPCODE);	/* Write disable */
	}
	else
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000004, SFC_OPCODE);	/* Write disable */
	}

	REG_WRITE_U32(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	SFC_SYNC;


	*retlen = written_count;

	return 0;
}

//---------------------------------------------------------------------------------------------


/*
 * _sfc_write_small_pages()
 * using MD to write data from DDR to FLASH which size is within a single page
 *
 * pass-in address must be aligned on 4-bytes, and 
 * length must be multiples of 4-bytes
 */

static int _sfc_write_small_pages(struct mtd_info *mtd, loff_t to, size_t len,
					size_t *retlen, const u_char *buf) {

	u8 *src = (u8*)buf;
	volatile u8 *dest = (volatile u8*)(SPI_RBUS_PHYS + to);
	u32 written_count = 0;
	u8 true_data_buf[MD_PP_DATA_SIZE*2];
	u8 *data_buf;
	u32 val;
	int ret;
	int i;
	rtk_sfc_info_t *sfc_info = (rtk_sfc_info_t*)mtd->priv;

	// workaround to avoid mantis issue #10822:
	// while writing less than 128 bytes and DDR address is 0x0 - 0x7f, 
	// change DDR address to any address larger than 0x7f for avoidance
	if(len < 128 && ((int)(true_data_buf) & 0x07ffffff) <= 0x7f)
		data_buf = &(true_data_buf[MD_PP_DATA_SIZE]);
	else
		data_buf = true_data_buf;


	// support write fewer than 256 bytes and size must be multiples of 4-bytes
	if(unlikely((len >= MD_PP_DATA_SIZE) || (len & RTK_SFC_SMALL_PAGE_WRITE_MASK)))
		BUG();

	if((to & RTK_SFC_SMALL_PAGE_WRITE_MASK)) // only support write onto 4-bytes aligned area
		BUG();

        // write enable
    	REG_WRITE_U32(0x00000006,SFC_OPCODE);
    	REG_WRITE_U32(0x00000000,SFC_CTL);
    	SFC_SYNC;

    	// disable write protect
    	REG_WRITE_U32(0x00000000,SFC_WP);
     
    	// enable write status register
    	REG_WRITE_U32(0x00000050,SFC_OPCODE);
    	REG_WRITE_U32(0x00000000,SFC_CTL);
    	SFC_SYNC;
 
    	// write status register , no memory protection
    	REG_WRITE_U32(0x00000001,SFC_OPCODE);
    	REG_WRITE_U32(0x00000010,SFC_CTL);
    	SFC_SYNC;
 
	REG_WRITE_U32(0x00000106,SFC_EN_WR);
        REG_WRITE_U32(0x00000105,SFC_WAIT_WR);
        REG_WRITE_U32(0x00ffffff,SFC_CE);

        SFC_SYNC;
	SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
	REG_WRITE_U32(0x00000002, SFC_OPCODE);	/* Byte Programming */
#if SFC_USE_DELAY
	_sfc_delay();
#endif

	REG_WRITE_U32(0x00000018, SFC_CTL);	/* dataen = 1, adren = 1, dmycnt = 0 */
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
        SFC_SYNC;

	while(len > 0) {
		for(i = 0; i < len ; i += 4) { /* endian convert */
			val = *((u32*)(src + i));
			*((u32*)(data_buf + i)) = 
#if 0
						((val >> 24) |
						((val >> 8 ) & 0x0000ff00) |
						((val << 8 ) & 0x00ff0000) |
						(val << 24));
#else
						val;
#endif
		}

		
#if 0
		//(write enable)
        SFC_SYNC;
		if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000806, SFC_OPCODE);
		}
		else
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000006, SFC_OPCODE);
		}
#if SFC_USE_DELAY
				_sfc_delay();
#endif

		REG_WRITE_U32(0x00180000, SFC_CTL);
		SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	    SFC_SYNC;
		tmp = *(volatile unsigned char *)FLASH_BASE;
#endif
		//issue write command
        	SFC_SYNC;
		if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE ))
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
 			REG_WRITE_U32(0x00000802, SFC_OPCODE);
		}
		else
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000002, SFC_OPCODE);
		}
#if SFC_USE_DELAY
				_sfc_delay();
#endif

		REG_WRITE_U32(0x00000018, SFC_CTL);
		SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
        	SFC_SYNC;
 		//setup MD DDR addr and flash addr
		REG_WRITE_U32(((unsigned int)data_buf), MD_FDMA_DDR_SADDR);
		REG_WRITE_U32(((unsigned int)dest), MD_FDMA_FL_SADDR);

 		//setup MD direction and move data length
		val = (0x2E000000 | len);               // do swap
		REG_WRITE_U32(val, MD_FDMA_CTRL2);		//for dma_length bytes.
		dma_cache_sync(NULL,data_buf, len, DMA_TO_DEVICE);

		mb();
		SFC_SYNC;

		//go 
		REG_WRITE_U32(0x03, MD_FDMA_CTRL1);

		/* wait for MD done its operation */
		while((ret = REG_READ_U32(MD_FDMA_CTRL1)) & 0x1);
#if !SFC_HW_POLL
		/* wait for flash controller done its operation */
		do {
            		SFC_SYNC;
			if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE ))
			{
				SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
				REG_WRITE_U32(0x00000805, SFC_OPCODE);
			}
			else
			{
				SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
				REG_WRITE_U32(0x00000005, SFC_OPCODE);
			}
#if SFC_USE_DELAY
					_sfc_delay();
#endif

			REG_WRITE_U32(0x00000010, SFC_CTL);
			SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	        SFC_SYNC;
		} while((*(volatile unsigned char *)FLASH_POLL_ADDR) & 0x1);
#endif     	    
		/* shift to next page to writing */
		src += len;
		dest += len;
		written_count += len;
		len -= len;
	} //end of page program

        /* send write disable then */
    	SFC_SYNC;
	if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000804, SFC_OPCODE);	/* Write disable */
	}
	else
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000004, SFC_OPCODE);	/* Write disable */
	}
#if SFC_USE_DELAY
	_sfc_delay();
#endif

	REG_WRITE_U32(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	SFC_SYNC;

	*retlen = written_count;

	return 0;
}
//---------------------------------------------------------------------------------------------

static int _sfc_write_md(struct mtd_info *mtd, loff_t to, size_t len,
					size_t *retlen, const u_char *buf) {

	volatile u8 *dest = (volatile u8*)(SPI_RBUS_PHYS + to);
	u32 val;
	int ret;
	
	rtk_sfc_info_t *sfc_info = (rtk_sfc_info_t*)mtd->priv;

        // write enable
    	REG_WRITE_U32(0x00000006,SFC_OPCODE);
    	REG_WRITE_U32(0x00000000,SFC_CTL);
    	SFC_SYNC;

    	// disable write protect
    	REG_WRITE_U32(0x00000000,SFC_WP);
     
    	// enable write status register
    	REG_WRITE_U32(0x00000050,SFC_OPCODE);
    	REG_WRITE_U32(0x00000000,SFC_CTL);
    	SFC_SYNC;
 
    	// write status register , no memory protection
    	REG_WRITE_U32(0x00000001,SFC_OPCODE);
    	REG_WRITE_U32(0x00000010,SFC_CTL);
    	SFC_SYNC;
 
	REG_WRITE_U32(0x00000106,SFC_EN_WR);
        REG_WRITE_U32(0x00000105,SFC_WAIT_WR);
        REG_WRITE_U32(0x00ffffff,SFC_CE);

        SFC_SYNC;
	SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
	REG_WRITE_U32(0x00000002, SFC_OPCODE);	/* Byte Programming */
#if SFC_USE_DELAY
	_sfc_delay();
#endif

	REG_WRITE_U32(0x00000018, SFC_CTL);	/* dataen = 1, adren = 1, dmycnt = 0 */
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
        SFC_SYNC;

	dma_cache_sync(NULL, &buf, len, DMA_TO_DEVICE);
		//(write enable)
    	SFC_SYNC;

	if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE ))
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010 
			REG_WRITE_U32(0x00000802, SFC_OPCODE);
	}
	else
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000002, SFC_OPCODE);
	}
#if SFC_USE_DELAY
				_sfc_delay();
#endif

	REG_WRITE_U32(0x00000018, SFC_CTL);
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	SFC_SYNC;
	//setup MD DDR addr and flash addr
	REG_WRITE_U32(((unsigned int)buf), MD_FDMA_DDR_SADDR);
	REG_WRITE_U32(((unsigned int)dest), MD_FDMA_FL_SADDR);

   	//setup MD direction and move data length
    	val = (0x2E000000 | len);   // do swap
	REG_WRITE_U32(val, MD_FDMA_CTRL2);		//for dma_length bytes.

	mb();

	//go 
	REG_WRITE_U32(0x03, MD_FDMA_CTRL1);

	/* wait for MD done its operation */
	while((ret = REG_READ_U32(MD_FDMA_CTRL1)) & 0x1);
#if !SFC_HW_POLL
	/* wait for flash controller done its operation */
	do {
        SFC_SYNC;
		if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu() || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE ))
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000805, SFC_OPCODE);
		}
		else
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000005, SFC_OPCODE);
		}
#if SFC_USE_DELAY
		_sfc_delay();
#endif

		REG_WRITE_U32(0x00000010, SFC_CTL);
		SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	    	SFC_SYNC;
		} while((*(volatile unsigned char *)FLASH_POLL_ADDR) & 0x1);
#endif

        /* send write disable then */
        SFC_SYNC;
	if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000804, SFC_OPCODE);	/* Write disable */
	}
	else
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000004, SFC_OPCODE);	/* Write disable */
	}
#if SFC_USE_DELAY
		_sfc_delay();
#endif

	REG_WRITE_U32(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	SFC_SYNC;

	*retlen = len;

	return 0;
}

//---------------------------------------------------------------------------------------------

static int _sfc_write_pages(struct mtd_info *mtd, loff_t to, size_t len,
					size_t *retlen, const u_char *buf) {

	u8 *src = (u8*)buf;
	volatile u8 *dest = (volatile u8*)(SPI_RBUS_PHYS + to);
	u32 written_count = 0;
	u8 data_buf[MD_PP_DATA_SIZE];
	u32 val;
	int ret;
	int i;

	rtk_sfc_info_t *sfc_info = (rtk_sfc_info_t*)mtd->priv;

	if(unlikely((len < MD_PP_DATA_SIZE))) {
		*retlen = 0;
		return 0;
	}

	if(unlikely(((u32)dest&(MD_PP_DATA_SIZE-1)))) {
		BUG();
	}

        // write enable
    	REG_WRITE_U32(0x00000006,SFC_OPCODE);
    	REG_WRITE_U32(0x00000000,SFC_CTL);
    	SFC_SYNC;

    	// disable write protect
    	REG_WRITE_U32(0x00000000,SFC_WP);
     
    	// enable write status register
    	REG_WRITE_U32(0x00000050,SFC_OPCODE);
    	REG_WRITE_U32(0x00000000,SFC_CTL);
    	SFC_SYNC;
 
    	// write status register , no memory protection
    	REG_WRITE_U32(0x00000001,SFC_OPCODE);
    	REG_WRITE_U32(0x00000010,SFC_CTL);
    	SFC_SYNC;
 
	REG_WRITE_U32(0x00000106,SFC_EN_WR);
        REG_WRITE_U32(0x00000105,SFC_WAIT_WR);
        REG_WRITE_U32(0x00ffffff,SFC_CE);

        SFC_SYNC;
	SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
	REG_WRITE_U32(0x00000002, SFC_OPCODE);	/* Byte Programming */
#if SFC_USE_DELAY
	_sfc_delay();
#endif

	REG_WRITE_U32(0x00000018, SFC_CTL);	/* dataen = 1, adren = 1, dmycnt = 0 */
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
        SFC_SYNC;

	while(len >= MD_PP_DATA_SIZE) {
		/* data moved by MD module is endian inverted.
					revert it before moved to flash */
		for(i = 0; i < MD_PP_DATA_SIZE; i += 4) {
			val = *((u32*)(src + i));
			*((u32*)(data_buf + i)) = 
#if 0
						((val >> 24) |
						((val >> 8 ) & 0x0000ff00) |
						((val << 8 ) & 0x00ff0000) |
						(val << 24));
#else
						val;
#endif
		}

		//dma_cache_sync(NULL, data_buf, MD_PP_DATA_SIZE, DMA_TO_DEVICE);
		//(write enable)
        	SFC_SYNC;
#if 0
		if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000806, SFC_OPCODE);
		}
		else
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000006, SFC_OPCODE);
		}
#if SFC_USE_DELAY
				_sfc_delay();
#endif

		REG_WRITE_U32(0x00180000, SFC_CTL);
		SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	    	SFC_SYNC;
		tmp = *(volatile unsigned char *)FLASH_BASE;
		//issue write command
#endif
        	SFC_SYNC;
		if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu() || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE ))
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010 
 			REG_WRITE_U32(0x00000802, SFC_OPCODE);
		}
		else
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000002, SFC_OPCODE);
		}
#if SFC_USE_DELAY
				_sfc_delay();
#endif

		REG_WRITE_U32(0x00000018, SFC_CTL);
		SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	    	SFC_SYNC; 

		//setup MD DDR addr and flash addr
		REG_WRITE_U32(((unsigned int)data_buf), MD_FDMA_DDR_SADDR);
		REG_WRITE_U32(((unsigned int)dest), MD_FDMA_FL_SADDR);

   		//setup MD direction and move data length
    		val = (0x26000000 | MD_PP_DATA_SIZE);   // do swap
		REG_WRITE_U32(val, MD_FDMA_CTRL2);		//for dma_length bytes.
		dma_cache_sync(NULL, data_buf, MD_PP_DATA_SIZE, DMA_TO_DEVICE);

		mb();
		SFC_SYNC;

		//go 
		REG_WRITE_U32(0x03, MD_FDMA_CTRL1);

		/* wait for MD done its operation */
		while((ret = REG_READ_U32(MD_FDMA_CTRL1)) & 0x1);
#if !SFC_HW_POLL
		/* wait for flash controller done its operation */
		do {
            		SFC_SYNC;
			if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu() || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE ))
			{
				SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
				REG_WRITE_U32(0x00000805, SFC_OPCODE);
			}
			else
			{
				SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
				REG_WRITE_U32(0x00000005, SFC_OPCODE);
			}
#if SFC_USE_DELAY
					_sfc_delay();
#endif

			REG_WRITE_U32(0x00000010, SFC_CTL);
			SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
		    	SFC_SYNC;
		} while((*(volatile unsigned char *)FLASH_POLL_ADDR) & 0x1);
#endif
    	    
		/* shift to next page to writing */
		src += MD_PP_DATA_SIZE;
		dest += MD_PP_DATA_SIZE;
		written_count += MD_PP_DATA_SIZE;
		len -= MD_PP_DATA_SIZE;
	}//end of page program
        /* send write disable then */
        SFC_SYNC;
	if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu() || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000804, SFC_OPCODE);	/* Write disable */
	}
	else
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000004, SFC_OPCODE);	/* Write disable */
	}
#if SFC_USE_DELAY
		_sfc_delay();
#endif

	REG_WRITE_U32(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	SFC_SYNC;

	*retlen = written_count;

	return 0;
}

//---------------------------------------------------------------------------------------------

/* rtk_sfc_write_pages() supports MD(move data module) page programming(PP). */
static int rtk_sfc_write_pages(struct mtd_info *mtd, loff_t to, size_t len,
						size_t *retlen, const u_char *buf) {
	rtk_sfc_info_t *sfc_info;
	u8 *src = (u8*)buf;
#ifdef CONFIG_MTD_RTK_MD_WRITE_COMPARE
	u8 *org_src = (u8*)buf;
	unsigned int org_to = to;
	size_t org_len = len;
	u32 i;
	u32 cmp_cnt=0, cmp_mod=0;
	volatile u32 *ptr_flash=NULL, *ptr_src=NULL;
#endif
	size_t written_len = 0;
	size_t wlen = 0;
	size_t length_to_write;
	int ret;

#ifdef CONFIG_NOR_SCRAMBLE_DATA_SUPPORT
	u8 *cp_da = NULL;
	unsigned int encrypt_flag = 0, i;
#endif
	
#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	add_debug_entry(buf, to, len, WRITE_ACCESS);
#endif

#ifdef DEV_DEBUG 
	printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write(), addr = %08X, size = %08X\n", (int)to, (int)len);
#endif

	if((sfc_info = (rtk_sfc_info_t*)mtd->priv) == NULL) { 
		printk("sfc_info is NULL\n");
		return -EINVAL;
	}

	down_write(&rw_sem);

	//SFC_FLUSH_CACHE((unsigned long) buf,len);

#ifdef CONFIG_NOR_SCRAMBLE_DATA_SUPPORT
	for (i=0;i<mtd_parts_count;i++)
	{
		if ((to >= (sfc_mtd_enc_parts+i)->offset) && (to < (sfc_mtd_enc_parts+i)->offset+(sfc_mtd_enc_parts+i)->size))
		{
			if ((sfc_mtd_enc_parts+i)->isEncrypted == 1)
				encrypt_flag = 1;
			break;
		}
	}

	if (encrypt_flag)
	{
		if ((to&0xf) || (len&0xf))
		{
			printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write() not alligned to 16, addr = %08X, size = %08X\r\n", (int)to, (int)len);
			return -EINVAL;
		}

		if ((cp_da = (u8*)kmalloc(len , GFP_KERNEL)) == NULL) {
			return -EINVAL;
		}

		src = cp_da;
		MCP_AES_ECB_Encryption(gAESKey, buf, cp_da, len);
}
#endif
#ifdef CONFIG_REALTEK_RTICE
        if (rtice_enable > 1) {
                printk("<MTD> %s %d\n", __FUNCTION__, __LINE__);
        }
#endif
	/* Set SB2 to in-order mode */
	_before_serial_flash_access();
#ifdef CONFIG_MTD_RTK_MD_WRITE_COMPARE
retry_mdwrite:	
#endif
#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(WRITE_READY);
#endif

	// 1st pass: write data 0-3 bytes 
	// (to make destination address within 4-bytes alignment)
	if(len > 0 && (to & RTK_SFC_SMALL_PAGE_WRITE_MASK)) {
		if(len > RTK_SFC_SMALL_PAGE_WRITE_MASK)
			length_to_write = (RTK_SFC_SMALL_PAGE_WRITE_MASK + 1) - (to & RTK_SFC_SMALL_PAGE_WRITE_MASK);
		else
			length_to_write = len;

		ret = _sfc_write_bytes(mtd, to, length_to_write, &wlen, src);

		if(ret || unlikely(wlen != length_to_write)) {
			*retlen = wlen;

			/* Restore SB2 */
			_after_serial_flash_access();

			up_write(&rw_sem);
			return ret;
		}

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write() $ PASS #1 $ [%08X:%08X]\n", (int)to, (int)len);
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write() $ PASS #1 $ %d bytes written\n", (int)wlen);
#endif

		src += wlen;
		written_len += wlen;
		len -= wlen;
		to += wlen;
	}

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(WRITE_STAGE_1);
#endif
	// 2st pass: write data which aligned in 4 bytes but smaller than a single page (256 bytes)
	// (to make destination address within 256-bytes alignment)
	if(len > RTK_SFC_SMALL_PAGE_WRITE_MASK && 
					(((to + MD_PP_DATA_SIZE - 1) & (~(MD_PP_DATA_SIZE - 1))) - to) > RTK_SFC_SMALL_PAGE_WRITE_MASK) {
		length_to_write = ((to + MD_PP_DATA_SIZE - 1) & (~(MD_PP_DATA_SIZE - 1))) - to;

		if(length_to_write > len)
			length_to_write = len;

		if(length_to_write & RTK_SFC_SMALL_PAGE_WRITE_MASK)
			length_to_write = length_to_write & (~RTK_SFC_SMALL_PAGE_WRITE_MASK);

		ret = _sfc_write_small_pages(mtd, to, length_to_write, &wlen, src);

		if(ret || unlikely(wlen != length_to_write)) {
			*retlen = wlen;

			/* Restore SB2 */
			_after_serial_flash_access();

			up_write(&rw_sem);
//			printk("222W222\n");
			return ret;
		}

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write() $ PASS #2 $ [%08X:%08X]\n", (int)to, (int)len);
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write() $ PASS #2 $ %d bytes written\n", (int)wlen);
#endif

		src += wlen;
		written_len += wlen;
		len -= wlen;
		to += wlen;
	}

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(WRITE_STAGE_2);
#endif

#if 0//SATURN_RW_MD //Use new md mechanism .
	if(len>0)
	{
		if(   ((unsigned int)to<0x400000) && ( (unsigned int)to+(unsigned int)len >=0x400000)  )
		{
			unsigned int offset = len >> 1;
			unsigned int remain = len - offset;
			_sfc_write_md(mtd, to, offset, &wlen, src);
			_sfc_write_md(mtd,to+offset,remain,&wlen,src+offset);
			wlen = len;	
		}
		else
		{
			_sfc_write_md(mtd, to, len, &wlen, src);
		}
		#if 1//def DEV_DEBUG 
			printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write() $ PASS #3 $ [%08X:%08X] (%d)\n", (int)to, (int)len, MD_PP_DATA_SIZE);
			printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write() $ PASS #3 $ %d bytes written\n", (int)wlen);
		#endif
		src += wlen;
		written_len += wlen;
		len -= wlen;
		to += wlen;
	}
#else
	// 3rd pass: write data in page unit (256 bytes)
	if(len >= MD_PP_DATA_SIZE) {
		length_to_write = len / MD_PP_DATA_SIZE * MD_PP_DATA_SIZE;

		ret = _sfc_write_pages(mtd, to, length_to_write, &wlen, src);

		if(ret || unlikely(wlen != length_to_write)) {
			*retlen = wlen;

			/* Restore SB2 */
			_after_serial_flash_access();

			up_write(&rw_sem);
			return ret;
		}

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write() $ PASS #3 $ [%08X:%08X] (%d)\n", (int)to, (int)len, MD_PP_DATA_SIZE);
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write() $ PASS #3 $ %d bytes written\n", (int)wlen);
#endif

		src += wlen;
		written_len += wlen;
		len -= wlen;
		to += wlen;
	}
#endif
#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(WRITE_STAGE_3);
#endif
	// 4th pass: write data which aligned in 4 bytes but smaller than a single page (256 bytes)
	if(len > 0 && (len & (~RTK_SFC_SMALL_PAGE_WRITE_MASK))) {
		length_to_write = len & (~RTK_SFC_SMALL_PAGE_WRITE_MASK);

		ret = _sfc_write_small_pages(mtd, to, length_to_write, &wlen, src);

		if(ret || unlikely(wlen != length_to_write)) {
			*retlen = wlen;

			/* Restore SB2 */
			_after_serial_flash_access();

			up_write(&rw_sem);
			return ret;
		}

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write() $ PASS #4 $ [%08X:%08X]\n", (int)to, (int)len);
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write() $ PASS #4 $ %d bytes written\n", (int)wlen);
#endif

		src += wlen;
		written_len += wlen;
		len -= wlen;
		to += wlen;
	}

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(WRITE_STAGE_4);
#endif
	// 5th pass: write data 0-3 bytes
	if(len > 0) {
		length_to_write = len;

		ret = _sfc_write_bytes(mtd, to, length_to_write, &wlen, src);

		if(ret || unlikely(wlen != length_to_write)) {
			*retlen = wlen;

			/* Restore SB2 */
			_after_serial_flash_access();

			up_write(&rw_sem);
			return ret;
		}

#ifdef DEV_DEBUG 
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write() $ PASS #5 $ [%08X:%08X]\n", (int)to, (int)len);
		printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_write() $ PASS #5 $ %d bytes written\n", (int)wlen);
#endif

		src += wlen;
		written_len += wlen;
		len -= wlen;
		to += wlen;
	}

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(WRITE_STAGE_5);
#endif

	*retlen = written_len;

	/* restore opcode to read */
	_switch_to_read_mode((rtk_sfc_info_t*)mtd->priv, eREAD_MODE_SINGLE_FAST_READ);

	/* Restore SB2 */
	_after_serial_flash_access();
#ifdef CONFIG_MTD_RTK_MD_WRITE_COMPARE 
        //TODO : louis, cache flush issues need to take care, not correct everytime
	ptr_flash = (volatile unsigned int*)(FLASH_BASE + org_to);
	ptr_src = (volatile unsigned int*)(org_src);

	cmp_cnt = org_len / 4;
	cmp_mod = org_len % 4;
	for(i=0;i<cmp_cnt;i++)
	{
		if (ptr_flash[i] != endian_swap(ptr_src[i]))
		{
	    		printk("1 write error at buf=%x %x len=%x\n", org_src, (FLASH_BASE + org_to), org_len);	
			for (i=0; i < org_len; i++) {
				printk("%02x ", org_src[i]);
				if (i%8 == 7) printk("\n");
			}
			printk("\ndest: ");
			for (i=0; i < org_len; i++) {
				printk("%02x ", ((unsigned char *)(FLASH_BASE + org_to))[i]);
				if (i%8 == 7) printk("\n");
			}
			while(1) {
				msleep(3000);
			}				
			goto retry_mdwrite;
		}
	}
	for(i=0;i<cmp_mod;i++)
	{
		if (org_src[cmp_cnt*4+i] != ((u8*)(FLASH_BASE + org_to))[cmp_cnt*4+i])
		{
	    		printk("2 write error at buf=%x %x len=%x\n", org_src, (FLASH_BASE + org_to), org_len);	
			for (i=0; i < org_len; i++) {
				printk("%02x ", org_src[i]);
				if (i%8 == 7) printk("\n");
			}
			printk("\ndest: ");
			for (i=0; i < org_len; i++) {
				printk("%02x ", ((unsigned char *)(FLASH_BASE + org_to))[i]);
				if (i%8 == 7) printk("\n");
			}
			while(1) {
				msleep(3000);
			}				
			goto retry_mdwrite;
		}
	}
#endif	
#ifdef CONFIG_NOR_SCRAMBLE_DATA_SUPPORT
	if (cp_da)
		kfree(cp_da);
#endif 
	up_write(&rw_sem);

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(WRITE_COMPLETED);
#endif

	return 0;
}
//---------------------------------------------------------------------------------------------

static int rtk_sfc_write(struct mtd_info *mtd, loff_t to, size_t len,
					size_t *retlen, const u_char *buf) {
	int retval;

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	add_debug_entry(buf, to, len, WRITE_ACCESS);
#endif

	down_write(&rw_sem);

	SFC_FLUSH_CACHE((unsigned long) buf,len);

#ifdef CONFIG_REALTEK_RTICE
	if (rtice_enable > 1) {
		printk("<MTD> %s %d\n", __FUNCTION__, __LINE__);
	}
#endif 
	/* Set SB2 to in-order mode */
	_before_serial_flash_access();

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(WRITE_READY);
#endif

	retval = _sfc_write_bytes(mtd, to, len, retlen, buf);

	/* restore opcode to read */
	_switch_to_read_mode((rtk_sfc_info_t*)mtd->priv, eREAD_MODE_SINGLE_FAST_READ);


	/* Restore SB2 */
	_after_serial_flash_access();

	up_write(&rw_sem);

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	change_status(WRITE_COMPLETED);
#endif

	return retval;
}
//---------------------------------------------------------------------------------------------
static int rtk_EON_segm_erase(struct mtd_info *mtd, struct erase_info *instr) {
	rtk_sfc_info_t *sfc_info;
	unsigned int size;
	volatile unsigned char *addr;
	unsigned char tmp;

   unsigned int erase_addr;
   unsigned int erase_opcode;
   unsigned int erase_size;


#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	add_debug_entry(instr->addr, 0, instr->len, ERASE_ACCESS);
#endif

	if((sfc_info = (rtk_sfc_info_t*)mtd->priv) == NULL) {
		return -EINVAL;
	}

	if(instr->addr + instr->len > mtd->size)
		return -EINVAL;

	down_write(&rw_sem);

#ifdef DEV_DEBUG 
	printk(KERN_WARNING "Rtk SFC MTD: rtk_sfc_erase(), addr = %08X, size = %08X\n", instr->addr, instr->len);
#endif

#ifdef CONFIG_REALTEK_RTICE
	if (rtice_enable > 1) {
		printk("<MTD> %s %d\n", __FUNCTION__, __LINE__);
	}
#endif

	/* Set SB2 to in-order mode */
	_before_serial_flash_access();

	addr = FLASH_BASE + (instr->addr);
	size = instr->len;

	erase_addr = (unsigned int)instr->addr;
	erase_opcode = sfc_info->erase_opcode;
	erase_size = mtd->erasesize;

	for(size = instr->len ; size > 0 ; size -= erase_size) {

		if((addr-FLASH_BASE)>=0 && (addr-FLASH_BASE)<0x10000)
		{
			erase_opcode = 0x000000d8;
			if((addr-FLASH_BASE)>=0 && (addr-FLASH_BASE)<0x1000)
			{
				erase_size = 0x1000;
			}
			else if((addr-FLASH_BASE)>=0x1000 && (addr-FLASH_BASE)<0x2000)
			{
				erase_size = 0x1000;
			}
			else if((addr-FLASH_BASE)>=0x2000 && (addr-FLASH_BASE)<0x4000)
			{
				erase_size = 0x2000;
			}
			else if((addr-FLASH_BASE)>=0x4000 && (addr-FLASH_BASE)<0x8000)
			{
				erase_size = 0x4000;
			}
			else if((addr-FLASH_BASE)>=0x8000 && (addr-FLASH_BASE)<0x10000)
			{
				erase_size = 0x8000;
			}
			
		}
		else
		{
		
			/* choose erase sector size */
			if (((erase_addr&(0x10000-1)) == 0) && (size >= 0x10000) && (sfc_info->sec_64k_en == SUPPORTED))
			{
				erase_opcode = 0x000000d8;
				erase_size = 0x10000;
			}
			else if (((erase_addr&(0x1000-1)) == 0) && (size >= 0x1000) && (sfc_info->sec_4k_en == SUPPORTED))
			{
				erase_opcode = 0x00000020;
				erase_size = 0x1000;
			}
			else
			{
				erase_opcode = sfc_info->erase_opcode;
				erase_size = mtd->erasesize;
			}
		}

		/* prior to any write instructions, send write enable first */
		//(write enable)
        SFC_SYNC;
		if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000806, SFC_OPCODE);
		}
		else
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000006, SFC_OPCODE);
		}
#if SFC_USE_DELAY
				_sfc_delay();
#endif

		REG_WRITE_U32(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
		SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
        SFC_SYNC;
		tmp = *(volatile unsigned char *)FLASH_POLL_ADDR;
        SFC_SYNC;
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(erase_opcode, SFC_OPCODE);	/* Sector-Erase */
#if SFC_USE_DELAY
				_sfc_delay();
#endif

		REG_WRITE_U32(0x00000008, SFC_CTL);	/* adren = 1 */
		SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
        SFC_SYNC;
		tmp = *addr;
#if 1
		/* using RDSR to make sure the operation is completed. */
		while(1) {
            SFC_SYNC;
			if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
			{
				SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
				REG_WRITE_U32(0x00000805, SFC_OPCODE);
			}
			else
			{
				SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
				REG_WRITE_U32(0x00000005, SFC_OPCODE);	/* Read status register */
			}
#if SFC_USE_DELAY
					_sfc_delay();
#endif

			REG_WRITE_U32(0x00000010, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 0 */
			SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	        SFC_SYNC;

			if((*(volatile unsigned char *)FLASH_POLL_ADDR) & 0x1)
				msleep(100); // erasing 256KBytes might take up to 2 seconds!
			else
				break;
		}
#endif
		addr += erase_size;
		erase_addr += erase_size;

	}

	/* send write disable then */
    SFC_SYNC;
	if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000804, SFC_OPCODE);
	}
	else
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000004, SFC_OPCODE);	/* Write disable */
	}
#if SFC_USE_DELAY
			_sfc_delay();
#endif

	REG_WRITE_U32(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
    SFC_SYNC;
	tmp = *(volatile unsigned char *)FLASH_POLL_ADDR;

	/* restore opcode to read */
	_switch_to_read_mode(sfc_info, eREAD_MODE_SINGLE_FAST_READ);

	/* Restore SB2 */
	_after_serial_flash_access();

	up_write(&rw_sem);

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

#if defined(CONFIG_MTD_RTK_SFC_DEBUG) 
	change_status(ERASE_COMPLETED);
#endif

	return 0;	

}
//---------------------------------------------------------------------------------------------

static int rtk_sfc_erase(struct mtd_info *mtd, struct erase_info *instr) {
	rtk_sfc_info_t *sfc_info;
	unsigned int size;
	volatile unsigned char *addr;
	unsigned char tmp;

   unsigned int erase_addr;
   unsigned int erase_opcode;
   unsigned int erase_size;


#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	add_debug_entry(instr->addr, 0, instr->len, ERASE_ACCESS);
#endif

	if((sfc_info = (rtk_sfc_info_t*)mtd->priv) == NULL) {
		return -EINVAL;
	}

	if(instr->addr + instr->len > mtd->size)
		return -EINVAL;

	down_write(&rw_sem);
#ifdef DEV_DEBUG 
	printk("Rtk SFC MTD: rtk_sfc_erase(), addr = %08X, size = %08X\n", (unsigned int)instr->addr, (unsigned int) instr->len);
#endif
#ifdef CONFIG_REALTEK_RTICE
	if (rtice_enable > 1) {
		printk("<MTD> %s %d\n", __FUNCTION__, __LINE__);
	}
#endif

	/* Set SB2 to in-order mode */
	_before_serial_flash_access();

        //disable auto-prog
        REG_WRITE_U32(0x00000005, SFC_WAIT_WR);
        REG_WRITE_U32(0x00000006, SFC_EN_WR);

	addr = FLASH_BASE + (instr->addr);
	size = instr->len;

	erase_addr = (unsigned int)instr->addr;
	erase_opcode = sfc_info->erase_opcode;
	erase_size = mtd->erasesize;

	for(size = instr->len ; size > 0 ; size -= erase_size) {

		/* choose erase sector size */
		if (((erase_addr&(0x10000-1)) == 0) && (size >= 0x10000) && (sfc_info->sec_64k_en == SUPPORTED))
		{
			erase_opcode = 0x000000d8;
			erase_size = 0x10000;
		}
/*
		else if (((erase_addr&(0x8000-1)) == 0) && (size >= 0x8000) && (sfc_info->sec_32k_en == SUPPORTED))
		{
			erase_opcode = 0x00000052;
			erase_size = 0x8000;
		}
*/
		else if (((erase_addr&(0x1000-1)) == 0) && (size >= 0x1000) && (sfc_info->sec_4k_en == SUPPORTED))
		{
			erase_opcode = 0x00000020;
			erase_size = 0x1000;
		}
		else
		{
			erase_opcode = sfc_info->erase_opcode;
			erase_size = mtd->erasesize;
		}

		/* prior to any write instructions, send write enable first */
		//(write enable)
        	SFC_SYNC;
		if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000806, SFC_OPCODE);
		}
		else
		{
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			REG_WRITE_U32(0x00000006, SFC_OPCODE);
		}
#if SFC_USE_DELAY
		_sfc_delay();
#endif

		REG_WRITE_U32(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
		SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
        	SFC_SYNC;
		tmp = *(volatile unsigned char *)FLASH_POLL_ADDR;
        	SFC_SYNC;
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(erase_opcode, SFC_OPCODE);	/* Sector-Erase */
#if SFC_USE_DELAY
		_sfc_delay();
#endif

		REG_WRITE_U32(0x00000008, SFC_CTL);	/* adren = 1 */
		SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
        	SFC_SYNC;
		tmp = *addr;
#if 1
		/* using RDSR to make sure the operation is completed. */
		while(1) {
            		SFC_SYNC;
			if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
			{
				SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
				REG_WRITE_U32(0x00000805, SFC_OPCODE);
			}
			else
			{
				SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
				REG_WRITE_U32(0x00000005, SFC_OPCODE);	/* Read status register */
			}
#if SFC_USE_DELAY
			_sfc_delay();
#endif

			REG_WRITE_U32(0x00000010, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 0 */
			SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	        	SFC_SYNC;

			if((*(volatile unsigned char *)FLASH_POLL_ADDR) & 0x1)
				msleep(100); // erasing 256KBytes might take up to 2 seconds!
			else
				break;
		}
#endif
		addr += erase_size;
		erase_addr += erase_size;

	}

	/* send write disable then */
    	SFC_SYNC;
	if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && (sfc_info->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000804, SFC_OPCODE);
	}
	else
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000004, SFC_OPCODE);	/* Write disable */
	}
#if SFC_USE_DELAY
	_sfc_delay();
#endif

	REG_WRITE_U32(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
    	SFC_SYNC;
	tmp = *(volatile unsigned char *)FLASH_POLL_ADDR;

        //enable auto-prog
        REG_WRITE_U32(0x00000105, SFC_WAIT_WR);
        REG_WRITE_U32(0x00000106, SFC_EN_WR);

	/* restore opcode to read */
	_switch_to_read_mode(sfc_info, eREAD_MODE_SINGLE_FAST_READ);

	/* Restore SB2 */
	_after_serial_flash_access();
#ifdef DEV_DEBUG 
	printk("Rtk SFC MTD: rtk_sfc_erase(), addr = %08X, size = %08X end\n", (unsigned int) instr->addr, (unsigned int) instr->len);
#endif
	up_write(&rw_sem);

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

#if defined(CONFIG_MTD_RTK_SFC_DEBUG) 
	change_status(ERASE_COMPLETED);
#endif

	return 0;
}
//---------------------------------------------------------------------------------------------

static int rtk_sfc_resumeReg(void)
{
		// initialize hardware (??)
#ifdef CONFIG_REALTEK_RTICE		
		if (rtice_enable > 1) {
			printk("<MTD> %s %d\n", __FUNCTION__, __LINE__);
		}
#endif
		_before_serial_flash_access();
	
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		//REG_WRITE_U32(0x00202020, SFC_CE);
		REG_WRITE_U32(0x00000000, SFC_WP);
		if(is_mars_cpu())
			REG_WRITE_U32(0x01070008, SFC_SCK);
		else if(is_saturn_cpu()||is_darwin_cpu() )
		{
			REG_WRITE_U32(0x0000000b, SFC_SCK);
			#if SET_SPEED
			setAsyFIFOReg();
			REG_WRITE_U32(0x01070005, SFC_SCK);//Set to high speed 50MHz
			REG_WRITE_U32(0x0, SFC_POS_LATCH);
			#endif
		} else if  (is_macarthur_cpu()) {
		
		printk("[%s]Freq. 0x%x\n",__FUNCTION__,REG_READ_U32(SFC_SCK));

		} else {
			//REG_WRITE_U32(0x01070007, SFC_SCK);
                        REG_WRITE_U32(gSFCReg.sfc_ce, SFC_CE);
                        REG_WRITE_U32(gSFCReg.sfc_wp, SFC_WP);
                        REG_WRITE_U32(gSFCReg.sfc_sck, SFC_SCK);
                        REG_WRITE_U32(gSFCReg.sfc_pos_latch, SFC_POS_LATCH);
                        SFC_SYNC;
                }

                if(is_neptune_cpu() || is_mars_cpu() || is_jupiter_cpu())
			REG_WRITE_U32(0x1, SFC_POS_LATCH);

		SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		if (is_jupiter_cpu())
			REG_WRITE_U32(0x00000003, SFC_SCK);  //202.5MHz / 4 ~= 50MHz 
		// restore speed - use 40MHz to access
		else if(is_mars_cpu()) // 168.75MHz / 4 ~= 42MHz
			REG_WRITE_U32(0x01030003, SFC_SCK);
		else if(is_saturn_cpu()||is_darwin_cpu() )
		{
			REG_WRITE_U32(0x0000000b, SFC_SCK);
			#if SET_SPEED
			setAsyFIFOReg();
			REG_WRITE_U32(0x00000005, SFC_SCK);//Set to high speed 50MHz
			REG_WRITE_U32(0x0, SFC_POS_LATCH);
			#endif
		} else if ( is_macarthur_cpu()) {
		} else {// 200MHz / 5 ~= 40MHz
			//REG_WRITE_U32(0x01040004, SFC_SCK);
                }
	
		// use falling edge to latch data because clock rate is changed to high
		if(is_neptune_cpu() || is_mars_cpu() || is_jupiter_cpu() )
			REG_WRITE_U32(0x0, SFC_POS_LATCH);
		SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010

#if SFC_HW_POLL
		// Jupiter has read-status and write-enable auto emission mechanism
		if(is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) {
	
			SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
			// enable RDSR
			REG_WRITE_U32(0x00000105, SFC_WAIT_WR);
	
			// enable WREN
			REG_WRITE_U32(0x00000106, SFC_EN_WR);
			SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
		}
#endif
	
		/* restore SB2 */
		_after_serial_flash_access();
	return 0;
}

//---------------------------------------------------------------------------------------------

static int rtk_sfc_suspend(struct mtd_info *mtd)
{
	/* wait for MD done its operation */
#ifdef CONFIG_REALTEK_RTICE	
	if (rtice_enable > 1) {
		printk("<MTD> %s %d\n", __FUNCTION__, __LINE__);
	}
#endif

        gSFCReg.sfc_ce = REG_READ_U32(SFC_CE);
        gSFCReg.sfc_wp = REG_READ_U32(SFC_WP);
        gSFCReg.sfc_sck = REG_READ_U32(SFC_SCK);
        gSFCReg.sfc_pos_latch = REG_READ_U32(SFC_POS_LATCH);

	while((REG_READ_U32(MD_FDMA_CTRL1)) & 0x1);
	//while((*(volatile unsigned char *)FLASH_POLL_ADDR) & 0x1);
	return 0;

}
//---------------------------------------------------------------------------------------------

static void rtk_sfc_resume(struct mtd_info *mtd)
{
	rtk_sfc_resumeReg();
}
//---------------------------------------------------------------------------------------------

void rtk_sfc_shutdown(struct mtd_info *mtd)
{
#ifdef CONFIG_REALTEK_RTICE	
if (rtice_enable > 1) {
		printk("<MTD> %s %d\n", __FUNCTION__, __LINE__);
}
	#endif
	down_write(&rw_sem);
	while((REG_READ_U32(MD_FDMA_CTRL1)) & 0x1);
	SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
	REG_WRITE_U32(0x00000805, SFC_OPCODE);	/* RDSR */
#if SFC_USE_DELAY 
	_sfc_delay();
#endif

	REG_WRITE_U32(0x00000010, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 0 */
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
        SFC_SYNC;	
	while((*(volatile unsigned char *)FLASH_POLL_ADDR) & 0x1);
	if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && ((&rtk_sfc_info)->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) {
		#if ENTER_4BYTES_MODE
		exit_4bytes_addr_mode();		
		#endif
		printk("rtk_sfc_shutdown: change back to default 3-bytes mode \n");

	}
	if (is_macarthur_cpu() && rtk_sfc_info.mtd_info->size >= 0x2000000) {
		printk("exit 4 byte mode\n");
		_switch_to_read_mode(&rtk_sfc_info, eREAD_MODE_SINGLE_FAST_READ);
		REG_WRITE_U32(0x000000c8, SFC_OPCODE);
       	SFC_SYNC;	
	       SFC_SYNC;		
#if SFC_USE_DELAY
		_sfc_delay();
#endif		  
		REG_WRITE_U32(0x10, SFC_CTL);
       	SFC_SYNC;		
		printk("extened = %x", *((volatile unsigned char *)FLASH_POLL_ADDR));
	      SFC_SYNC;		
		  

		REG_WRITE_U32(0x000000c5, SFC_OPCODE);
       	SFC_SYNC;	
	       SFC_SYNC;		
#if SFC_USE_DELAY
		_sfc_delay();
#endif		  
		REG_WRITE_U32(0x10, SFC_CTL);
       	SFC_SYNC;		
		*((volatile unsigned char *)FLASH_POLL_ADDR) = 0;
	      SFC_SYNC;		
		  
		REG_WRITE_U32(0x000000c8, SFC_OPCODE);
       	SFC_SYNC;	
	       SFC_SYNC;		
#if SFC_USE_DELAY
		_sfc_delay();
#endif		  
		REG_WRITE_U32(0x10, SFC_CTL);
       	SFC_SYNC;		
		printk("extened = %x", *((volatile unsigned char *)FLASH_POLL_ADDR));
	      SFC_SYNC;		
		printk("%x", *((volatile unsigned char *)FLASH_BASE +0x3000));

		REG_WRITE_U32(0x000000c8, SFC_OPCODE);
       	SFC_SYNC;	
	       SFC_SYNC;		
#if SFC_USE_DELAY
		_sfc_delay();
#endif		  
		REG_WRITE_U32(0x10, SFC_CTL);
       	SFC_SYNC;		
		printk("extened = %x", *((volatile unsigned char *)FLASH_POLL_ADDR));
	      SFC_SYNC;		
		  

		REG_WRITE_U32(0x000000c5, SFC_OPCODE);
       	SFC_SYNC;	
	       SFC_SYNC;		
#if SFC_USE_DELAY
		_sfc_delay();
#endif		  
		REG_WRITE_U32(0x10, SFC_CTL);
       	SFC_SYNC;		
		*((volatile unsigned char *)FLASH_POLL_ADDR) = 0;
	      SFC_SYNC;		
		  
		REG_WRITE_U32(0x000000c8, SFC_OPCODE);
       	SFC_SYNC;	
	       SFC_SYNC;		
#if SFC_USE_DELAY
		_sfc_delay();
#endif		  
		REG_WRITE_U32(0x10, SFC_CTL);
       	SFC_SYNC;		
		printk("extened = %x", *((volatile unsigned char *)FLASH_POLL_ADDR));
	      SFC_SYNC;		
		REG_WRITE_U32(0x00000015, SFC_OPCODE);
       	SFC_SYNC;	
	       SFC_SYNC;		
#if SFC_USE_DELAY
		_sfc_delay();
#endif		  
		REG_WRITE_U32(0x10, SFC_CTL);
       	SFC_SYNC;		
		printk("status 3 = %x", *((volatile unsigned char *)FLASH_POLL_ADDR));
	      SFC_SYNC;		
		  
		
		REG_WRITE_U32(0x000000E9, SFC_OPCODE);
       	SFC_SYNC;	
		REG_WRITE_U32(0x00, SFC_CTL);
	       SFC_SYNC;		
#if SFC_USE_DELAY
		_sfc_delay();
#endif		  
		REG_WRITE_U32(0x00, SFC_CTL);
       	SFC_SYNC;		
		REG_WRITE_U32(0x00000000, SFP_OPCODE2);
       	SFC_SYNC;		
		*((volatile unsigned char *)FLASH_POLL_ADDR)  = 0xFF;		
	      SFC_SYNC;		
		  
		REG_WRITE_U32(0x00000015, SFC_OPCODE);
       	SFC_SYNC;	
	       SFC_SYNC;		
#if SFC_USE_DELAY
		_sfc_delay();
#endif		  
		REG_WRITE_U32(0x10, SFC_CTL);
       	SFC_SYNC;		
		printk("status 3 = %x", *((volatile unsigned char *)FLASH_POLL_ADDR));
	      SFC_SYNC;		

		REG_WRITE_U32(0x000000c8, SFC_OPCODE);
       	SFC_SYNC;	
	       SFC_SYNC;		
#if SFC_USE_DELAY
		_sfc_delay();
#endif		  
		REG_WRITE_U32(0x10, SFC_CTL);
       	SFC_SYNC;		
		printk("extened = %x", *((volatile unsigned char *)FLASH_POLL_ADDR));
	      SFC_SYNC;		
		  
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x0000006,  SFC_OPCODE);	/* Write enable */
#if SFC_USE_DELAY
		_sfc_delay();
#endif
		REG_WRITE_U32(0x00000000, SFC_CTL);	/* dataen = 0, adren = 0, dmycnt = 0 */
		SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
	        SFC_SYNC;
		 *(volatile unsigned char *)FLASH_POLL_ADDR = 0xFF;
			
		REG_WRITE_U32(0x000000c5, SFC_OPCODE);
       	SFC_SYNC;	
	       SFC_SYNC;		
#if SFC_USE_DELAY
		_sfc_delay();
#endif		  
		REG_WRITE_U32(0x10, SFC_CTL);
       	SFC_SYNC;		
		*((volatile unsigned char *)FLASH_POLL_ADDR) = 0;
	      SFC_SYNC;		
		  
		REG_WRITE_U32(0x000000c8, SFC_OPCODE);
       	SFC_SYNC;	
	       SFC_SYNC;		
#if SFC_USE_DELAY
		_sfc_delay();
#endif		  
		REG_WRITE_U32(0x10, SFC_CTL);
       	SFC_SYNC;		
		printk("extened = %x", *((volatile unsigned char *)FLASH_POLL_ADDR));
	      SFC_SYNC;		
		  
	}
	_switch_to_read_mode(&rtk_sfc_info, eREAD_MODE_SINGLE_FAST_READ);
	
	return;
}
//---------------------------------------------------------------------------------------------

#if 0
static int rtk_sfc_point(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char **mtdbuf)
{
	return 0;
}

static void rtk_sfc_unpoint(struct mtd_info *mtd, u_char *addr, loff_t from, size_t len)
{
	return;
}
#endif
//---------------------------------------------------------------------------------------------

static int rtk_sfc_probe(struct platform_device *pdev) {
        struct mtd_info *mtd_info;
	rtk_sfc_info_t *sfc_info;
	unsigned int tmp = 0;
	unsigned int reg_muxpad5=0;
	int ret = 0,err=0;

 	mtd_info = (struct mtd_info*)&descriptor;

	if((sfc_info = (rtk_sfc_info_t*)mtd_info->priv) == NULL) {
		return -ENODEV;
	}
#ifdef CONFIG_REALTEK_RTICE	
	if (rtice_enable > 1) {
		printk("<MTD> %s %d\n", __FUNCTION__, __LINE__);
	}
#endif
        SFC_SYNC;
	SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
	reg_muxpad5 = REG_READ_U32(0x18000374)|0x00000001; //set spi pinmux
	REG_WRITE_U32(reg_muxpad5, 0x18000374);
     	REG_WRITE_U32(0x00000001, SFC_POS_LATCH);   	//set serial flash controller latch data at rising edge
	REG_WRITE_U32(0x00090101, SFC_CE);   		//setup control edge
	REG_WRITE_U32(0x0000009f, SFC_OPCODE);		/* JEDEC Read ID */
#if SFC_USE_DELAY
	_sfc_delay();
#endif

	REG_WRITE_U32(0x00000010, SFC_CTL);	/* dataen = 1, adren = 0, dmycnt = 0 */
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
    	SFC_SYNC;

#ifdef DEV_DEBUG
	printk(KERN_INFO "Read id orginal address : 0x%08x\n",FLASH_POLL_ADDR);
#endif
	tmp = endian_swap(*(volatile unsigned int *)FLASH_POLL_ADDR);
	tmp >>= 8;
#ifdef DEV_DEBUG
	printk(KERN_INFO "Read id orginal value : 0x%08x\n",tmp);
#endif
	sfc_info->manufacturer_id = (tmp & 0x00FF0000)>>16;
	sfc_info->device_id2 = RDID_DEVICE_EID_1(tmp);
	sfc_info->device_id1 = RDID_DEVICE_EID_2(tmp);
	printk(KERN_INFO "--RDID Seq: 0x%x | 0x%x | 0x%x\n",sfc_info->manufacturer_id,sfc_info->device_id1,sfc_info->device_id2);
	switch(sfc_info->manufacturer_id) {
	case MANUFACTURER_ID_SST:
		ret = sst_init(sfc_info);
		break;
		
	case MANUFACTURER_ID_SPANSION:
		ret = spansion_init(sfc_info);
		break;
	case MANUFACTURER_ID_MXIC:
		ret = mxic_init(sfc_info);
		break;
	case MANUFACTURER_ID_PMC:
		ret = pmc_init(sfc_info);
		break;
	case MANUFACTURER_ID_STM:
		ret = stm_init(sfc_info);
		break;
	case MANUFACTURER_ID_EON:
		ret = eon_init(sfc_info);
		break;
	case MANUFACTURER_ID_ATMEL:
		ret = atmel_init(sfc_info);
		break;

	case MANUFACTURER_ID_WINBOND:
		ret = winbond_init(sfc_info);
		break;
	case MANUFACTURER_ID_ESMT:
		ret = esmt_init(sfc_info);
		break;
	case MANUFACTURER_ID_GD:
		ret = gd_init(sfc_info);
		break;
	default:
		printk(KERN_WARNING "RtkSFC MTD: Unknown flash type.\n");
		printk(KERN_WARNING "Manufacturer's ID = %02X, Memory Type = %02X, Memory Capacity = %02X\n", tmp & 0xff, (tmp >> 8) & 0xff, (tmp >> 16) & 0xff);
		return -ENODEV;
		break;
	}

	mtd_info->erasesize = sfc_info->erase_size;
	mtd_info->writesize = 0x100;
#ifdef EON_SEGMENT_ERASE
	if(g_isSegErase)
	{
		printk("Set EON segment erase.\n");
		mtd_info->erase = rtk_EON_segm_erase;
	}
#endif

	printk("Supported Erase Size:%s%s%s%s.\n"
		, sfc_info->sec_256k_en?" 256KB":""
		, sfc_info->sec_64k_en?" 64KB":""
		, sfc_info->sec_32k_en?" 32KB":""
		, sfc_info->sec_4k_en?" 4KB":""
	);
#if SFC_MULTI_READ
	printk("Attr:%s%s%s.\n"
		,(sfc_info->attr&RTK_SFC_ATTR_SUPPORT_DUAL_O)?" dualread":""
		,(sfc_info->attr&RTK_SFC_ATTR_SUPPORT_DUAL_IO)?" dualIO":""
		,(sfc_info->attr&RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE)?" 4bytemode":""
	);
#endif

#if defined(CONFIG_MTD_RTK_SFC_READ_MD) || defined(CONFIG_MTD_RTK_SFC_WRITE_MD)
	printk("RtkSFC MTD: Enable RtkSFC MD PP callback function.\n");

	#if defined(CONFIG_MTD_RTK_SFC_READ_MD)
	mtd_info->_read = rtk_sfc_read_pages;
	printk("MD_READ Enable..\n");
	#else
	printk("MD_READ Disable..\n");
	#endif
	#if defined(CONFIG_MTD_RTK_SFC_WRITE_MD)
	mtd_info->_write = rtk_sfc_write_pages;
	printk("MD_WRITE Enable..\n");
	#else
	printk("MD_WRITE Disable..\n");
	#endif
#endif

	if((ret = rtk_sfc_attach(&descriptor)) != 0) {
		printk("[%s]Realtek SFC attach fail\n",__FUNCTION__);
		return ret;
	}
	return 0;
}
//- platform drv --------------------------------------------------------------------------------------------
static struct physmap_flash_data physmap_flash_data = {
        .width          = 1,
};

static struct platform_driver rtk_spirbus_driver = {
        .driver = {
                   .name = "rtk_spirbus",
                   },
        .probe = rtk_spirbus_probe,
        .remove = NULL,
        .suspend    = NULL,//TODO: ,
        .resume     = NULL,//TODO: ,
};

static struct resource physmap_flash_resource[] = {
        [0] = {
#if 0
                .start  = GET_MAPPED_RBUS_ADDR((unsigned int)SB2_REG_BASE),
                .end    = GET_MAPPED_RBUS_ADDR((unsigned int)SB2_REG_BASE+ 0x824),
                .flags = IORESOURCE_MEM
#else
                .start  = ((unsigned int)SPI_RBUS_VIRT),
                .end    = ((unsigned int)SPI_RBUS_VIRT+FLASH_SIZE-1),
                .flags = IORESOURCE_MEM
#endif
        },
        [1] = {
          	.start  = IRQ_SB2,
        	.end    = IRQ_SB2,
        	.flags  = IORESOURCE_IRQ,
    	},
#if 0
        [2] = {
                .start  = ((unsigned int)SPI_RBUS_VIRT),
                .end    = ((unsigned int)SPI_RBUS_VIRT+FLASH_SIZE-1),
                .flags = IORESOURCE_MEM
        }
#endif
};

static u64 rtkSFC_dmamask = DMA_BIT_MASK(32); 
static struct platform_device *rtkSFC_device;
static struct platform_driver rtkSFC_driver = {
    .probe      = rtk_sfc_probe,
    .driver     =
    {
            .name   = "rtkSFC",
            .owner  = THIS_MODULE,
    },
};

//---------------------------------------------------------------------------------------------
static int rtk_sfc_init_1(void)
{
    	int rc = 0;
	int ret=0;
        unsigned long long flashsize;
        struct mtd_info *mtd_info;
        rtk_sfc_info_t *sfc_info;

        mtd_info = (struct mtd_info*)&descriptor;

        if((sfc_info = (rtk_sfc_info_t*)mtd_info->priv) == NULL) {
                return -ENODEV;
        }


	init_rwsem(&rw_sem);

        rc = platform_driver_register(&rtkSFC_driver);

        if (!rc) {
                rtkSFC_device = platform_device_alloc("rtkSFC", 0);
        	rtkSFC_device->num_resources = ARRAY_SIZE(physmap_flash_resource);
        	rtkSFC_device->dev.dma_mask = &rtkSFC_dmamask;
        	rtkSFC_device->dev.coherent_dma_mask = DMA_BIT_MASK(32);
        	rtkSFC_device->resource = physmap_flash_resource;
                if (rtkSFC_device)
        	{
                       	rc = platform_device_add(rtkSFC_device);
        		if ((sfc_info->manufacturer_id == 0xff)&&
        		    (sfc_info->device_id2 == 0xff) &&
        		    (sfc_info->device_id1 == 0xff))
			{
				rc = -1;
			}
                }
                else {
                        rc = -ENOMEM;
                }
                if (rc < 0) {
                        platform_device_put(rtkSFC_device);
                        platform_driver_unregister(&rtkSFC_driver);
                }
        }

    	if (rc < 0){
        	printk(KERN_INFO "Realtek SFC Driver installation fails.\n\n");
        	return -ENODEV;
   	}else{
#ifdef ENABLE_SFC_INT_MODE
        	printk(KERN_INFO "Realtek SFC Driver is running interrupt mode.\n\n");
#endif
        	printk(KERN_INFO "Realtek SFC Driver is successfully installing.\n\n");
    	}

	return rc;
}
	
static int __init rtk_sfc_init(void) {
	unsigned int tmp_int;
        unsigned int flashstart;
        unsigned long long flashsize;

	printk("RtkSFC MTD init...2014/09/03 \n");
	printk("***Add segment erase for EON flash ***\n");
	printk("NOR flash support list ..\n");

#if  defined(CONFIG_REALTEK_USE_HWSEM_AS_SENTINEL )
	printk("(V) HWSEMA support.\n");
#else
	printk("(X) HWSEMA support.\n");
#endif

#if SFC_HW_POLL
	printk("(V) AUTO_HW_POLL support.\n");
#else
	printk("(X) AUTO_HW_POLL support.\n");
#endif

#if ENTER_4BYTES_MODE
	printk("(V) 4Bytes mode support.\n");
#else
	printk("(X) 4Bytes mode support.\n");
#endif

#if	NOR_SUPPORT_MAX_ERASE_SIZE
	printk("(V) Max erase size support.\n");
#else
	printk("(X) Max erase size support.\n");
#endif

#if SFC_USE_DELAY
	printk("(V) Sfc_delay support.\n");
#else
	printk("(X) Sfc_delay support.\n");
#endif

#if SFC_MULTI_READ
	printk("(V) Multi-read support.\n");
#else
	printk("(X) Multi-read support.\n");
#endif

	// init nor flash parameter
	flashsize = FLASH_SIZE; 
        rtk_nor_base_addr = physmap_flash_resource[0].start;   //vir addr
        rtk_nor_size = flashsize;

	// parsing parameters
	FLASH_POLL_ADDR = FLASH_BASE = (unsigned char*)(rtk_nor_base_addr);
	rtk_sfc_size=rtk_nor_size;

	// initialize hardware (??)
	_before_serial_flash_access();

	printk(KERN_INFO "Realtek SFC Driver : reset sfc clk done.\n\n");

	/* prepare mtd_info */
	memset(&descriptor, 0, sizeof(struct mtd_info));
	memset(&rtk_sfc_info, 0, sizeof(rtk_sfc_info_t));

	rtk_sfc_info.mtd_info = &descriptor;
	
	descriptor.priv = &rtk_sfc_info;
	descriptor.name = "RtkSFC";
	//descriptor.writesize = descriptor.size = rtk_sfc_size;	
	descriptor.size = rtk_sfc_size;	
	//descriptor.writesize = 0x1;
	//descriptor.flags = MTD_CLEAR_BITS | MTD_ERASEABLE;
	descriptor.flags = MTD_WRITEABLE;
	//descriptor.flags = MTD_CAP_NORFLASH;
	descriptor._erase = rtk_sfc_erase;
	descriptor._read = rtk_sfc_read;
	descriptor._suspend = rtk_sfc_suspend;
	descriptor._write = rtk_sfc_write;
	descriptor._resume = rtk_sfc_resume;
	//descriptor._shutdown = rtk_sfc_shutdown;
	//descriptor.put_device = rtk_sfc_suspend;
	descriptor.owner = THIS_MODULE;
	//descriptor.type = MTD_NORFLASH; //MTD_NORFLASH for Intel "Sibley" flash 
	descriptor.type = MTD_DATAFLASH;//MTD_DATAFLASH for general serial flash
	descriptor.numeraseregions = 0;	// uni-erasesize
	descriptor.oobsize = 0;

	rtk_sfc_init_1();

	return 0;
}

static int rtk_sfc_attach(struct mtd_info *mtd_info) {
	int nr_parts = 0;
	struct mtd_partition *parts;
	unsigned int tmp;

        printk("Freq. reg value 0x%x\n",REG_READ_U32(SFC_SCK));

	/* WRSR: write status register, no memory protection */
    	SFC_SYNC;
	SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
	REG_WRITE_U32(0x00000001, SFC_OPCODE);
#if SFC_USE_DELAY
	_sfc_delay();
#endif
	REG_WRITE_U32(0x00000010, SFC_CTL);
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
    	SFC_SYNC;
	*((volatile unsigned char*)FLASH_POLL_ADDR)= 0x00;

	/*
	 * Partition selection stuff.
	 */
	

#ifdef CONFIG_MTD_PARTITIONS
	printk("[%s]descriptor size 0x%x\n",__FUNCTION__,(unsigned int)descriptor.size);

	nr_parts = parse_mtd_partitions(&descriptor, part_probes, &parts, 0);
#endif
	if(nr_parts <= 0) {
		printk(KERN_NOTICE "Rtk SFC: using single partition ");
		if(add_mtd_device(&descriptor)) {
			/* restore SB2 */
			_after_serial_flash_access();

			printk(KERN_WARNING "Rtk SFC: (for SST/SPANSION/MXIC/WINBOND SPI-Flash) Failed to register new device\n");
			return -EAGAIN;
		}
	}
	else {
		printk(KERN_NOTICE "Rtk SFC: using dynamic partition ");
		add_mtd_partitions(&descriptor, parts, nr_parts);
	}

#ifdef CONFIG_NOR_SCRAMBLE_DATA_SUPPORT
	printk("Scramble on.\n");
	mtd_parts_count = nr_parts;
	sfc_mtd_enc_parts = (struct sfc_mtd_enc_info*)kmalloc(mtd_parts_count*sizeof(struct sfc_mtd_enc_info), GFP_KERNEL);
	if (sfc_mtd_enc_parts == NULL)
		return -EAGAIN;
	memset(sfc_mtd_enc_parts, 0, mtd_parts_count*sizeof(struct sfc_mtd_enc_info));
	for (i=0;i<mtd_parts_count;i++)
	{
		(sfc_mtd_enc_parts+i)->name = (parts+i)->name;
		(sfc_mtd_enc_parts+i)->part_type = (parts+i)->part_type;
		(sfc_mtd_enc_parts+i)->offset = (parts+i)->offset;
		(sfc_mtd_enc_parts+i)->size = (parts+i)->size;

		if(!strcmp((sfc_mtd_enc_parts+i)->part_type,"aes"))
		{
			(sfc_mtd_enc_parts+i)->isEncrypted = 1;
		}
	}
	memcpy(gAESKey, platform_info.AES_IMG_KEY, sizeof(gAESKey));
#endif

	/* restore opcode to read */
	_switch_to_read_mode(&rtk_sfc_info, eREAD_MODE_SINGLE_FAST_READ);

	/* restore SB2 */
	_after_serial_flash_access();

#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	/* allocating debug information */
	if((dbg_counter = (int *) kmalloc(sizeof(int), GFP_KERNEL)))
		memset(dbg_counter, 0, sizeof(int));

	if((dbg_table = (DESCRIPTOR_TABLE *) kmalloc(sizeof(DESCRIPTOR_TABLE) * DBG_ENTRY_NUM, GFP_KERNEL)))
		memset(dbg_table, 0, sizeof(DESCRIPTOR_TABLE) * DBG_ENTRY_NUM);

	printk("DEBUG COUNTER = %08X  /  DEBUG TABLE = %08X\n", dbg_counter, dbg_table);
#endif

	printk("Rtk SFC: (for SST/SPANSION/MXIC SPI Flash)\n");
	return 0;
}
//---------------------------------------------------------------------------------------------

static void __exit rtk_sfc_exit(void)
{
#ifdef NOR_SCRAMBLE_DATA_SUPPORT
	if(sfc_mtd_enc_parts)
		kfree(sfc_mtd_enc_parts);
#endif
#if defined(CONFIG_MTD_RTK_SFC_DEBUG)
	if(dbg_counter)
		kfree(dbg_counter);
	if(dbg_table)
		kfree(dbg_table);
#endif
#ifdef CONFIG_REALTEK_RTICE
	if (rtice_enable > 1) {
		printk("<MTD> %s %d\n", __FUNCTION__, __LINE__);
	}
#endif

	/* put read opcode into control register */
    	SFC_SYNC;
	if((is_jupiter_cpu()||is_saturn_cpu()||is_darwin_cpu()  || is_macarthur_cpu()) && ((&rtk_sfc_info)->attr & RTK_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE )) 
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000803, SFC_OPCODE);
	}
	else
	{
		SYS_REG_TRY_LOCK(0);//add by alexchang 0722-2010
		REG_WRITE_U32(0x00000003, SFC_OPCODE);
	}
#if SFC_USE_DELAY
	_sfc_delay();
#endif

	REG_WRITE_U32(0x00000018, SFC_CTL);
	SYS_REG_TRY_UNLOCK;//add by alexchang 0722-2010
    	SFC_SYNC;

	del_mtd_device(&descriptor);
}
//---------------------------------------------------------------------------------------------

module_init(rtk_sfc_init);
module_exit(rtk_sfc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chih-pin Wu <wucp@realtek.com.tw>");
MODULE_DESCRIPTION("MTD chip driver for Realtek Rtk Serial Flash Controller");
