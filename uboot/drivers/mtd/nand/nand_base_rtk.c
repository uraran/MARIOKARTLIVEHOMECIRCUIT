/******************************************************************************
 * $Id: nand_base_rtk.c 317223 2010-06-01 07:38:49Z alexchang2131 $
 * drivers/mtd/nand/nand_base_rtk.c
 * Overview: Realtek MTD NAND Driver
 * Copyright (c) 2008 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2008-06-10 Ken-Yu   create file
 *    #001 2008-09-10 Ken-Yu   add BBT and BB management
 *    #002 2008-09-28 Ken-Yu   change r/w from single to multiple pages
 *    #003 2008-10-09 Ken-Yu   support single nand with multiple dies
 *    #004 2008-10-23 Ken-Yu   support multiple nands
 *
 *******************************************************************************/
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <mtd/mtd-abi.h>
#include <linux/mtd/rtk_nand.h>
#include <asm/arch/rbus/nand_reg.h>
#include <linux/bitops.h>
#include <rtk_nand_list.h>

#define BACKUP_WRITE    1
#define BACKUP_READ     2
#define BACKUP_ERASE    3

#define CRCSIZE	4
#define PLL_SCPU1	0x18000100

#define BBT1PAGE	64
#define BBT2PAGE	128
#define TIMEOUT_LIMIT   1

#define MODULE_AUTHOR(x)	/* x */
#define MODULE_DESCRIPTION(x)	/* x */

#define NOTALIGNED(mtd, x) ((x & (mtd->oobblock-1)) != 0)

#define check_end(mtd, addr, len)					\
do {									\
	if (mtd->size == 0)						\
		return -ENODEV;						\
	else								\
	if ((addr + len) > mtd->size) {					\
		printk(						\
			"%s: attempt access past end of device\n",	\
			__func__);					\
		return -EINVAL;						\
	}								\
} while (0)

#define check_page_align(mtd, addr)					\
do {									\
	if (addr & (mtd->oobblock - 1)) {				\
		printk(						\
			"%s: attempt access non-page-aligned data\n",	\
			__func__);					\
		printk(						\
			"%s: mtd->oobblock = 0x%x\n",			\
			__func__, mtd->oobblock);			\
		return -EINVAL;						\
	}								\
} while (0)

#define check_block_align(mtd, addr)					\
do {									\
	if (addr & (mtd->erasesize - 1)) {				\
		printk(						\
			"%s: attempt access non-block-aligned data\n",	\
			__func__);					\
		return -EINVAL;						\
	}								\
} while (0)

#define check_len_align(mtd, len)					\
do {									\
	if (len & (512 - 1)) {					\
		printk(						\
		      "%s: attempt access non-512bytes-aligned mem\n",	\
			__func__);					\
		return -EINVAL;						\
	}								\
} while (0)


typedef struct rtk_ver_info {
	unsigned char active_fw;
	unsigned char boot_major_version;
	unsigned char boot_minor_version;
	unsigned char system_major_version;
	unsigned char system_minor_version;
	unsigned int hash;
} RTK_VER_INFO_T;


unsigned int badblock_temp[128];
extern char rtknand_info[128];
char rtk_bootfw[8];
struct mtd_info *g_rtk_mtd;
static int ppb_shift;
unsigned int g_eccbits;
int RBA;
static int oob_size, ppb, isLastPage;
static int page_size;
static unsigned int crc_table[256];
static int crc_table_computed;

extern void rtk_nand_reset(void);

/* NAND low-level MTD interface functions */
static int nand_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
static int nand_read_ecc(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf, u_char *oob_buf, struct nand_oobinfo *oobsel);
static int nand_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);
static int nand_write_ecc(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf, const u_char *oob_buf, struct nand_oobinfo *oobsel);
static int nand_erase(struct mtd_info *mtd, struct erase_info *instr);
static int nand_erase_nand(struct mtd_info *mtd, struct erase_info *instr, int allowbbt);
static void nand_sync(struct mtd_info *mtd);
static int nand_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops);
static int nand_write_oob(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops);
static unsigned int rtk_find_real_blk(struct mtd_info *mtd, unsigned int blk);
static int nand_block_isbad(struct mtd_info *mtd, loff_t ofs);
static int rtk_update_bbt(struct mtd_info *mtd, __u8 *data_buf, __u8 *oob_buf, BB_t *bbt);
static int rtk_write_bbt(struct mtd_info *mtd, int page);

static unsigned int Reflect(unsigned int ref, char ch)
{
	unsigned int value = 0;
	int i;
	/* Swap bit 0 for bit 7 */
	/* bit 1 for bit 6, etc. */
	for (i = 1; i < (ch + 1); i++) {
		if (ref & 1)
			value |= 1 << (ch - i);
		ref >>= 1;
	}
	return value;
}

static void make_crc_table(void)
{
	unsigned int polynomial = 0x04C11DB7;
	int i, j;

	for (i = 0; i <= 0xFF; i++) {
		crc_table[i] = Reflect(i, 8) << 24;
		for (j = 0; j < 8; j++)
			crc_table[i] = (crc_table[i] << 1) ^ (crc_table[i] & (1 << 31) ? polynomial : 0);
		crc_table[i] = Reflect(crc_table[i],  32);
	}

	crc_table_computed = 1;
}

static unsigned int rtk_crc32(unsigned char *p, int len, unsigned int *crc)
{
	int cnt = len;
	unsigned int value;

	if (!crc_table_computed)
		make_crc_table();

	value = 0xFFFFFFFF;
	while (cnt--) {
		value = (value >> 8) ^ crc_table[(value & 0xFF) ^ *p++];
	}

	*crc = value ^ 0xFFFFFFFF;

	return 0;
}


static void dump_BBT(struct mtd_info *mtd)
{

	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int i;
	int BBs = 0;

	printk("[%s] Nand BBT Content\n", __func__);
	for (i = 0; i < RBA; i++) {
		if ((this->bbt[i].BB_die == BB_DIE_INIT) && (this->bbt[i].bad_block == BB_INIT)) {
			printk("[%d] (BB_DIE_INIT, BB_INIT, %u, %u)\n", i, this->bbt[i].RB_die, this->bbt[i].remap_block);
		} else if (this->bbt[i].bad_block == BAD_RESERVED) {
			printk("[%d] (%u, 0x%x, %u, %u)\n", i, this->bbt[i].BB_die, this->bbt[i].bad_block,
				this->bbt[i].RB_die, this->bbt[i].remap_block);
		} else {
			printk("[%d] (%u, %u, %u, %u)\n", i, this->bbt[i].BB_die, this->bbt[i].bad_block,
				this->bbt[i].RB_die, this->bbt[i].remap_block);
		}


		if ((this->bbt[i].bad_block != BB_INIT))
			BBs++;

	}

	this->BBs = BBs;
}

static unsigned int rtk_page_to_block(__u32 page)
{
	return page/ppb;
}

static unsigned int rtk_page_offset_in_block(__u32 page)
{
	return page%ppb;
}

int rtk_update_bbt_to_flash(struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int rc1 = 0;
	int rc2 = 0;

	rc1 = rtk_write_bbt(mtd, BBT1PAGE);
	if (rc1 < 0) {
		printk("%s: write BBT1 Fail.\n", __func__);
	}

	rc2 = rtk_write_bbt(mtd, BBT2PAGE);
	if (rc2 < 0) {
		printk("%s: write BBT2 Fail.\n", __func__);
	}

	if ((rc1 < 0) && (rc2 < 0))
		return -1;
	else
		return 0;
}

static int rtk_find_available_remap_block(struct mtd_info *mtd, unsigned int *remap_blk)
{
	struct nand_chip *this = mtd->priv;
	unsigned int i;

	for (i = 0; i < RBA; i++) {
		if (this->bbt[i].bad_block == BB_INIT) {
			*remap_blk = this->bbt[i].remap_block;
			return 0;
		}
	}

	printk("[%s] RBA do not have free remap block\n", __func__);

	return -1;
}

static void rtk_update_BBT(struct mtd_info *mtd, __u32 ogl_blk, __u32 src_blk, __u32 map_blk)
{
	struct nand_chip *this = mtd->priv;

	if (ogl_blk != src_blk) {
		this->bbt[(this->block_num-1) - src_blk].bad_block = 0x4444;
		this->bbt[(this->block_num-1) - src_blk].BB_die = 0;
	}

	this->bbt[(this->block_num-1) - map_blk].bad_block = ogl_blk;
	this->bbt[(this->block_num-1) - map_blk].BB_die = 0;

	return;
}

static void rtk_badblock_handle_update_BBT(struct mtd_info *mtd, __u32 blk)
{
	struct nand_chip *this = mtd->priv;

	this->bbt[(this->block_num-1) - blk].bad_block = 0x4444;
	this->bbt[(this->block_num-1) - blk].BB_die = 0;

	return;
}

static int cmp_mem(unsigned char *data, unsigned char *data_enc, int len)
{
	unsigned int i;
	int check_data = 0;

	for (i = 0; i < len; i++) {
		if (data[i] != data_enc[i]) {
			check_data = 1;
			break;
		}
	}

	return check_data;
}

static int rtk_backup_block(struct mtd_info *mtd, __u32 src_blk, __u32 remap_blk, __u32 page_offset, __u32 mode)
{
	struct nand_chip *this = mtd->priv;
	unsigned int start_page = src_blk * ppb;
	unsigned int backup_start_page = remap_blk * ppb;
	unsigned int i;
	u_char *buffer = NULL;
	int rc = 0;

	buffer = kmalloc(page_size, GFP_KERNEL);
	if (!buffer) {
		printk("kmalloc fail\n");
		rc = -2;
		goto rtk_backup_block_end;
	}

	printk("Start to Backup block from %u to %u\n", src_blk, remap_blk);
	rc = this->erase_block(mtd, this->active_chip, remap_blk*ppb);
	if (rc < 0) {
		if (rc == -1) {
			printf("erase remap block %u fail.\n", remap_blk);

		} else if (rc == -2) {
			printf("erase remap page %u align fail.\n", remap_blk*ppb);

		}
		goto rtk_backup_block_end;
	}

	for (i = 0; i < ppb; i++) {
		memset(buffer, 0x00, page_size);

		rc = this->read_ecc_page(mtd, this->active_chip, start_page+i, buffer, NULL, CP_NF_NONE, 0, 0, 0);
		if (rc == -1) {
			printf("read source page error\n");
			rc = -2;
			goto rtk_backup_block_end;
		} else if ((rc == 1) || ((i == page_offset) && (mode == BACKUP_WRITE))) {
			continue;
		}
		rc = this->write_ecc_page(mtd, 0, backup_start_page+i, buffer, NULL, 0);
		if (rc < 0) {
			printf("write page(%u) in remap block fail. (%d)\n", i, rc);
			goto rtk_backup_block_end;
		}

		memset(buffer, 0x00, page_size);
		rc = this->read_ecc_page(mtd, this->active_chip, backup_start_page+i, buffer, NULL, CP_NF_NONE, 0, 0, 0);
		if (rc == -1) {
			printf("read page(%u) in remap block fail. (%d)\n", i, rc);
			goto rtk_backup_block_end;
		}
	}

rtk_backup_block_end:
	if (buffer)
		kfree(buffer);

	/*rc = -1, erase/write error */
	/*rc = -2, memory/page-align error */
	return rc;
}

static int rtk_badblock_handle(struct mtd_info *mtd, __u32 page, __u32 ogl_block, uint32_t backup, uint32_t mode)
{
	__u32 src_block = rtk_page_to_block(page);
	__u32 remap_block = 0;
	__u32 page_offset = rtk_page_offset_in_block(page);
	int rc = 0;

rtk_badblock_handle_redo:
	rc = rtk_find_available_remap_block(mtd, &remap_block);
	if (rc < 0) {
		printk("[%s] No available re-map block.\n", __func__);
		return rc;
	}

	if (backup) {
		rc = rtk_backup_block(mtd, src_block, remap_block, page_offset, mode);
		if (rc < 0) {
			if (rc == -1) {
				printk("[%s] erase/write re-map block error.\n", __func__);
				rtk_badblock_handle_update_BBT(mtd, remap_block);
				goto rtk_badblock_handle_redo;
			} else if (rc == -2) {
				printk("[%s] memory alloc fail or data verification fail.\n", __func__);
				return rc;
			}
		}
	}

	rtk_update_BBT(mtd, ogl_block, src_block, remap_block);

	rc = rtk_update_bbt_to_flash(mtd);
	if (rc < 0) {
		printk("[%s] rtk_update_bbt_to_flash() fails.\n", __func__);
		return rc;
	}
	printk("[%s] rtk_update_bbt() successfully.\n", __func__);

	return rc;
}

static unsigned long long rtk_from_to_page(struct mtd_info *mtd, loff_t from)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;

	return (int)(from >> this->page_shift);
}

static unsigned int rtk_find_real_blk(struct mtd_info *mtd, unsigned int blk)
{
    struct nand_chip *this = (struct nand_chip *) mtd->priv;
    unsigned int i;
    unsigned int real_blk = blk;

    for (i = 0; i < RBA; i++) {
	if (this->bbt[i].bad_block != BB_INIT) {
	    if (blk == this->bbt[i].bad_block) {
		real_blk = this->bbt[i].remap_block;
		break;
	    }
	}
    }

    return real_blk;
}

static int check_BBT(struct mtd_info *mtd, unsigned int blk)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;

	printk("[%s]..\n", __func__);
	int i;

	for (i = 0; i < RBA; i++) {
		if (this->bbt[i].bad_block == blk) {
			printk("blk 0x%x exist\n", blk);
			return -1;
		}
	}

	return 0;
}

static int nand_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	return 0;
}

static int nand_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
	int rc = 0;

	rc = nand_read_ecc(mtd, from, len, retlen, buf, NULL, NULL);

	return rc;
}

static int nand_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	int rc = 0;

	mtd->oobinfo.useecc = ops->mode;
	rc = nand_read_ecc(mtd, from, ops->len, &ops->retlen, ops->datbuf, ops->oobbuf, NULL);

	return rc;
}

static int nand_read_ecc(struct mtd_info *mtd, loff_t from, size_t len,
			size_t *retlen, u_char *buf, u_char *oob_buf, struct nand_oobinfo *oobsel)
{
	struct nand_chip *this = mtd->priv;
	unsigned int  page, real_page, page_offset;
	int data_len, oob_len;
	int rc, retry_count;
	unsigned int block, real_block;
	int chipnr;

	if ((from + len) > mtd->size) {
		printk("nand_read_ecc: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	if (NOTALIGNED(mtd, from) || NOTALIGNED(mtd, len)) {
		printk(KERN_NOTICE "nand_read_ecc: Attempt to read not page aligned data\n");
		return -EINVAL;
	}

	this->active_chip = chipnr = 0;
	page = ((int)(from >> this->page_shift)) & (this->pagemask);
	page_offset = page & (ppb-1);
	block = page/ppb;

	this->select_chip(mtd, chipnr);

	if (retlen)
		*retlen = 0;

	data_len = oob_len = 0;
	retry_count = 0;

nand_read_ecc_after_bb_handle:
	while (data_len < len) {
		real_block = rtk_find_real_blk(mtd, block);
		real_page = real_block*ppb + page_offset;

		rc = this->read_ecc_page (mtd, this->active_chip, real_page, &buf[data_len], &oob_buf[oob_len], CP_NF_NONE, 0, 0, 0);

		if (rc < 0) {
			if (rc == -1) {
				printk("%s: read_ecc_page: Un-correctable HW ECC\n", __func__);
				if (rtk_badblock_handle(mtd, real_page, block, 0, BACKUP_READ) == 0) {
					printk("Update BBT OK.\n");
				} else {
					printk("Update BBT fail.\n");
				}
				return -1;
			} else if (rc == -2) {
				printk("read_ecc_page: rc:%d read ppb:%u, page:%x failed\n", rc, ppb, page);
				printk("Prepare to do backup\n");
				if (rtk_badblock_handle(mtd, real_page, block, 1, BACKUP_READ) == 0) {
					printk("Finish backup bad block. read after backup.......\n");
					goto nand_read_ecc_after_bb_handle;
				} else {
					printk("backup bad block fail.\n");
					return -1;
				}
			} else if (rc == -WAIT_TIMEOUT) {

					if (retry_count < TIMEOUT_LIMIT) {
						printk("WAIT_LOOP timeout, read after nand reset\n");
						rtk_nand_reset();
						retry_count++;

						goto nand_read_ecc_after_bb_handle;

					} else {
						panic("WAIT_LOOP timeout over expected times \n");
					}

			} else {
				printk("%s: read_ecc_page:  semphore failed\n", __func__);
				return -1;
			}
		}

		data_len += page_size;

		if (oob_buf)
			oob_len += oob_size;

		page++;
		page_offset = page & (ppb-1);
		block = page/ppb;
	} /* while (data_len < len) */

	if (retlen) {
		if (data_len == len) {
			*retlen = data_len;
		} else {
			printk("[%s] error: data_len %d != len %d\n", __func__, data_len, len);
			return -1;
		}
	}

	return 0;
}


static int nand_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
	int rc = 0;

	rc = nand_write_ecc(mtd, to, len, retlen, buf, NULL, NULL);

	return rc;
}

static int nand_write_oob(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
	int rc = 0;

	rc =  nand_write_ecc(mtd, to, ops->len, &ops->retlen, ops->datbuf, ops->oobbuf, NULL);

	return rc;
}

static int nand_write_ecc(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen,
			const u_char *buf, const u_char *oob_buf, struct nand_oobinfo *oobsel)
{
	struct nand_chip *this = mtd->priv;
	unsigned int page, real_page, page_offset;
	int data_len, oob_len;
	int rc, retry_count;
	unsigned int block, real_block;
	int chipnr;

	if ((to + len) > mtd->size) {
		printk("nand_write_ecc: Attempt write beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	if (NOTALIGNED(mtd, to) || NOTALIGNED(mtd, len)) {
		printk(KERN_NOTICE "nand_write_ecc: Attempt to write not page aligned data\n");
		return -EINVAL;
	}

	this->active_chip = chipnr = 0;
	page = ((unsigned int)(to >> this->page_shift)) & this->pagemask;
	page_offset = page & (ppb-1);
	block = page / ppb;

	this->select_chip(mtd, chipnr);

	if (retlen)
		*retlen = 0;

	data_len = oob_len = 0;
	retry_count = 0;

nand_write_ecc_after_bb_handle:
	while (data_len < len) {
		real_block = rtk_find_real_blk(mtd, block);
		real_page = real_block*ppb + page_offset;

		if (oob_buf)
			rc = this->write_ecc_page(mtd, this->active_chip, real_page, &buf[data_len], &oob_buf[oob_len], 0);
		else
			rc = this->write_ecc_page(mtd, this->active_chip, real_page, &buf[data_len], NULL, 0);

		if (rc < 0) {
			if (rc == -WAIT_TIMEOUT) {
				if (retry_count < TIMEOUT_LIMIT) {
					printk("WAIT_LOOP timeout, write after nand reset\n");
					rtk_nand_reset();
					retry_count++;

					goto nand_write_ecc_after_bb_handle;

				} else {
					panic("WAIT_LOOP timeout over expected times \n");
				}
			} else if (rc == -1) {
				if (rtk_badblock_handle(mtd, real_page, block, 1, BACKUP_WRITE) == 0) {
					printk("Finish backup bad block. read after backup.......\n");
					goto nand_write_ecc_after_bb_handle;
				} else {
					printk("backup bad block fail.\n");
					return -1;
				}
			}
		}

		data_len += page_size;
		oob_len += oob_size;

		page++;
		page_offset = page & (ppb-1);
		block = page/ppb;
	} /* while ( data_len < len) */

	if (retlen) {
		if (data_len == len) {
			*retlen = data_len;
		} else {
			printk("[%s] error: data_len %d != len %d\n", __func__, data_len, len);
			return -1;
		}
	}

	return 0;
}

static int nand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	return nand_erase_nand(mtd, instr, 0);
}

static int nand_erase_nand(struct mtd_info *mtd, struct erase_info *instr, int allowbbt)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	u_int64_t addr = instr->addr;
	u_int64_t len = instr->len;
	int page, chipnr;
	int block;
	u_int64_t elen = 0;
	int rc = 0, retry_count;
	unsigned int real_block;
	unsigned int real_page;

	check_end(mtd, addr, len);
	check_block_align(mtd, addr);

	instr->fail_addr = 0xffffffff;

	this->active_chip = chipnr = 0;
	page = addr >> this->page_shift;
	block = page/ppb;

	this->select_chip(mtd, chipnr);

	instr->state = MTD_ERASING;
	retry_count = 0;

nand_erase_nand_after_bb_handle:
	while (elen < len) {
		real_block = rtk_find_real_blk(mtd, block);
		real_page = real_block*ppb;

		printk("Erase blk %u\n", page/ppb);
		rc = this->erase_block(mtd, this->active_chip, real_page);

		if (rc == -WAIT_TIMEOUT) {
			if (retry_count < TIMEOUT_LIMIT) {
				printk("WAIT_LOOP timeout, erase after nand reset\n");
				rtk_nand_reset();
				retry_count++;

				goto nand_erase_nand_after_bb_handle;
			} else {
				panic("WAIT_LOOP timeout over expected times \n");
			}
		} else if (rc == -1) {
			printk("%s: block erase failed at page address=0x%08x\n", __func__, addr);
			instr->fail_addr = (page << this->page_shift);

			if (rtk_badblock_handle(mtd, real_page, block, 0, BACKUP_ERASE) == 0) {
				goto nand_erase_nand_after_bb_handle;
			}
		} else if (rc == -2) {
			printk("%s: block erase failed at page address=0x%08x not block alignment\n", __func__, addr);
			goto err;
		}

		elen += mtd->erasesize;
		page += ppb;
		block = page/ppb;
	}

	instr->state = MTD_ERASE_DONE;
err:
	return rc;
}

static void nand_sync(struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;

	this->state = RTK_FL_READY;

	return;
}

static void nand_select_chip(struct mtd_info *mtd, int chip)
{
	switch (chip) {
	case -1:
		REG_WRITE_U32(REG_PD, 0xff);
		break;
	case 0:
	case 1:
	case 2:
	case 3:
		REG_WRITE_U32(REG_PD, ~(1 << chip));
		break;
	default:
		REG_WRITE_U32(REG_PD, ~(1 << 0));
	}
}

static int rtk_block_bad(struct mtd_info *mtd, unsigned int block)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;

	return this->erase_block(mtd, 0, block * ppb);

}

static int comfirm_remap_block(unsigned int block)
{
	int i;

	for (i = 0; i < 128; i++) {
		if (badblock_temp[i] == 0x9999)
			break;

		if (block == badblock_temp[i]) {
			return -1;
		}
	}

	return 0;
}


static int scan_last_die_BB(struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int block_num = this->block_num;
	int i, j;
	int rc = 0;

	this->active_chip = 0;
	this->select_chip(mtd, 0);

	for (i = 0; i < 128; i++) {
		badblock_temp[i] = 0x9999;
	}

	j = 0;
	for (i = 32; i < block_num; i++) {
		if (rtk_block_bad(mtd, i) != 0) {
			printk("Erase block %d fail.\n", i);
			badblock_temp[j] = i;
			j++;
		}
	}

	for (i = 0 ; i < RBA; i++) {
		this->bbt[i].bad_block = BB_INIT;
		this->bbt[i].BB_die = BB_DIE_INIT;
		this->bbt[i].RB_die = 0;

		if (comfirm_remap_block((block_num - 1) - i) != 0) {
			this->bbt[i].bad_block = 0x4444;
		}

		this->bbt[i].remap_block = (block_num-1) - i;
	}

	j = 0;
	for (i = 0; i < 128; i++) {
		if (badblock_temp[i] != 0x9999) {
			if (this->bbt[j].bad_block != 0x4444) {
				/* this->bbt[i].BB_die = 0; */
				/* this->bbt[i].RB_die = 0; */
				this->bbt[j].bad_block = badblock_temp[i];
			} else {
				i--;
			}
			j++;
		} else {
			break;
		}
	}

	RTK_FLUSH_CACHE((unsigned long) this->bbt, sizeof(BB_t)*RBA);

	return 0;
}

static unsigned int rtk_nand_bbt_crc_calculate(u8 *temp_BBT)
{
	BB_t *bb = temp_BBT;
	char hash_temp[64] = {0};
	unsigned int hash_value = 0;
	unsigned int hash_value_temp = 0;
	int i;

	memset(hash_temp, 0, sizeof(hash_temp));

	for (i = 0; i < RBA; i++) {
		if ((bb[i].BB_die != BB_DIE_INIT) && (bb[i].bad_block != BB_INIT)) {
			hash_value_temp = hash_value_temp + bb[i].BB_die + bb[i].bad_block + bb[i].RB_die + bb[i].remap_block;
		}
	}

	memset(hash_temp, 0, sizeof(hash_temp));
	sprintf(hash_temp, "%u", hash_value_temp);
	printk("hash_temp:[%s][%d]\n", hash_temp, strlen(hash_temp));
	rtk_crc32(hash_temp, strlen(hash_temp), &hash_value);

	printk("rtk_nand_bbt_crc_calculate  hash_value:[%u]\n", hash_value);

	return hash_value;
}

static int rtk_write_bbt(struct mtd_info *mtd, int page)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int rc = 0;
	u8 *temp_BBT = 0;
	int i;
	unsigned int crc_value = 0;

	if (page == BBT1PAGE) {
		printk("[%s] write block1 !!\n", __func__);
	} else if (page == BBT2PAGE) {
		printk("[%s] write block2 !!\n", __func__);
	}

	temp_BBT = kmalloc(page_size, GFP_KERNEL);
	if (!temp_BBT) {
		printk("%s: Error, no enough memory for temp_BBT\n", __func__);
		return -ENOMEM;
	}

	memset(temp_BBT, 0xff, page_size);
	this->select_chip(mtd, 0);
	if (this->erase_block(mtd, 0, page)) {
		printk("[%s]erase block %d failure !!\n", __func__, page/ppb);
		rc =  -1;
		goto EXIT;
	}
	this->g_oobbuf[0] = BBT_TAG;

	if (CRCSIZE == 4) {
		crc_value = rtk_nand_bbt_crc_calculate(this->bbt);
		memcpy(temp_BBT, &crc_value, sizeof(unsigned int));
	}

	memcpy(temp_BBT + CRCSIZE, this->bbt, sizeof(BB_t)*RBA);
	if (this->write_ecc_page(mtd, 0, page, temp_BBT, this->g_oobbuf, 1)) {
		printk("[%s] write BBT B%d page %d failure!!\n", __func__, page == 0?0:1, page);
		rc =  -1;
		goto EXIT;
	}

EXIT:
	if (temp_BBT)
		kfree(temp_BBT);

	return rc;
}

static int rtk_create_bbt(struct mtd_info *mtd, int page)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;

	int rc = 0;
	u8 *temp_BBT = 0;
	int i;
	unsigned int crc_value = 0;

	if (page == ppb)
		printk("[%s] nand driver creates B1 !!\n", __func__);
	else if (page == (ppb<<1))
		printk("[%s] nand driver creates B2 !!\n", __func__);
	else {
		printk("[%s] abort creating BB on page %x !!\n", __func__, page);
		return -1;
	}

	if (scan_last_die_BB(mtd)) {
		printk("[%s] scan_last_die_BB() error !!\n", __func__);
		return -1;
	}

	if (this->numchips > 1) {
			printk("[%s] scan_last_die() error !!\n", __func__);
			return -1;
	}

	temp_BBT = kmalloc(page_size, GFP_KERNEL);
	if (!temp_BBT) {
		printk("%s: Error, no enough memory for temp_BBT\n", __func__);
		return -ENOMEM;
	}
	memset(temp_BBT, 0xff, page_size);
	this->select_chip(mtd, 0);
	if (this->erase_block(mtd, 0, page)) {
		printk("[%s]erase block %d failure !!\n", __func__, page/ppb);
		rc =  -1;
		goto EXIT;
	}
	this->g_oobbuf[0] = BBT_TAG;

	crc_value = rtk_nand_bbt_crc_calculate(this->bbt);
	memcpy(temp_BBT, &crc_value, sizeof(unsigned int));

	memcpy(temp_BBT + 4, this->bbt, sizeof(BB_t)*RBA);
	if (this->write_ecc_page(mtd, 0, page, temp_BBT, this->g_oobbuf, 1)) {
		printk("[%s] write BBT B%d page %d failure!!\n", __func__, page == 0?0:1, page);
		rc =  -1;
		goto EXIT;
	}

EXIT:
	if (temp_BBT)
		kfree(temp_BBT);

	return rc;
}


static int rtk_update_bbt(struct mtd_info *mtd, __u8 *data_buf, __u8 *oob_buf, BB_t *bbt)
{
	int rc = 0;
	struct nand_chip *this = mtd->priv;
	unsigned char active_chip = this->active_chip;
	u8 *temp_BBT = 0;

	oob_buf[0] = BBT_TAG;

	this->select_chip(mtd, 0);

	if (sizeof(BB_t)*RBA <= page_size) {
		memcpy(data_buf, bbt, sizeof(BB_t)*RBA);
	} else {
		temp_BBT = kmalloc(2*page_size, GFP_KERNEL);
		if (!(temp_BBT)) {
			printk("%s: Error, no enough memory for temp_BBT\n", __func__);
			return -ENOMEM;
		}
		memset(temp_BBT, 0xff, 2*page_size);
		memcpy(temp_BBT, bbt, sizeof(BB_t)*RBA);
		memcpy(data_buf, temp_BBT, page_size);
	}

	if (this->erase_block(mtd, 0, ppb)) {
		printk("[%s]error: erase block 1 failure\n", __func__);
	}

	if (this->write_ecc_page(mtd, 0, ppb, data_buf, oob_buf, 1)) {
		printk("[%s]update BBT B1 page 0 failure\n", __func__);
	} else {
		if (sizeof(BB_t)*RBA > page_size) {
			memset(data_buf, 0xff, page_size);
			memcpy(data_buf, temp_BBT+page_size, sizeof(BB_t)*RBA - page_size);
			if (this->write_ecc_page(mtd, 0, ppb+1, data_buf, oob_buf, 1)) {
				printk("[%s]update BBT B1 page 1 failure\n", __func__);
			}
			memcpy(data_buf, temp_BBT, page_size);//add by alexchang 0906-2010
		}
	}

	if (this->erase_block(mtd, 0, ppb<<1)) {
		printk("[%s]error: erase block 1 failure\n", __func__);
		return -1;
	}

	if (this->write_ecc_page(mtd, 0, ppb<<1, data_buf, oob_buf, 1)) {
		printk("[%s]update BBT B2 failure\n", __func__);
		return -1;
	} else {
		if (sizeof(BB_t)*RBA > page_size) {
			memset(data_buf, 0xff, page_size);
			memcpy(data_buf, temp_BBT+page_size, sizeof(BB_t)*RBA - page_size);
			if (this->write_ecc_page(mtd, 0, 2*ppb+1, data_buf, oob_buf, 1)) {
				printk("[%s]error: erase block 2 failure\n", __func__);
				return -1;
			}
		}
	}

	this->select_chip(mtd, active_chip);
	if (temp_BBT)
		kfree(temp_BBT);

	return rc;
}

static int rtk_nand_bbt_sync(struct mtd_info *mtd, int block)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;

	return rtk_write_bbt(mtd, block*ppb);
}

static int rtk_nand_bbt_crc_check(u8 *temp_BBT, unsigned int *value, unsigned int *count)
{
	unsigned int crc_value = 0;
	BB_t *bb = (BB_t *)(temp_BBT+4);
	char hash_temp[64] = {0};
	unsigned int hash_value = 0;
	unsigned int hash_value_temp = 0;
	int i;

	*count = 0;

	memcpy(&crc_value, temp_BBT, sizeof(unsigned int));

	for (i = 0; i < RBA; i++) {
		if ((bb[i].BB_die != BB_DIE_INIT) && (bb[i].bad_block != BB_INIT)) {

			hash_value_temp = hash_value_temp + bb[i].BB_die + bb[i].bad_block + bb[i].RB_die + bb[i].remap_block;

			*count = *count + 1;
		}
	}

	memset(hash_temp, 0, sizeof(hash_temp));
	sprintf(hash_temp, "%u", hash_value_temp);
	rtk_crc32(hash_temp, strlen(hash_temp), &hash_value);

	printk("rtk_nand_bbt_crc_check  crc_value:[%u]  hash_value:[%u] count:[%u]\n", crc_value, hash_value, *count);

	if (crc_value == hash_value) {
		*value = hash_value;
		return 0;
	} else {
		*value = 0;
		return -1;
	}
}

static int rtk_nand_read_bbt(struct mtd_info *mtd, int bbt_page, u8 *temp_BBT)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int rc = 0;
	__u8 isbbt = 0;

	rc = this->read_ecc_page(mtd, 0, bbt_page, this->g_databuf, this->g_oobbuf, CP_NF_NONE, 0, 0, 0);
	if (rc == 0 || rc == 1 || rc == -2) {
		isbbt = this->g_oobbuf[0];

		if (isbbt == BBT_TAG) {
			memcpy(temp_BBT, this->g_databuf, page_size);
			return 0;

		} else {
			printk("rtk_nand_read_bbt Get bbt TAG FAIL.\n");
			return -1;
		}
	} else {
		printk("rtk_nand_read_bbt read block[%d][%d] FAIL.\n", bbt_page = 64 ? 1 : 2, bbt_page);
		return -2;
	}
}

static int rtk_nand_disable_cpu(void)
{
	REG_WRITE_U32(PLL_SCPU1, 0x0);
}

static int rtk_nand_scan_bbt(struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int rc = 0;
	__u8 isbbt_b1, isbbt_b2;
	u8 *temp_BBT1 = NULL;
	u8 *temp_BBT2 = NULL;
	unsigned int crc1 = 0;
	unsigned int crc2 = 0;
	unsigned int count1 = 0;
	unsigned int count2 = 0;
	int i = 0;
	int sync_need = 0;

	RTK_FLUSH_CACHE((unsigned long) this->bbt, sizeof(BB_t)*RBA);

	temp_BBT1 = kmalloc(page_size, GFP_KERNEL);
	if (!temp_BBT1) {
		printk("%s: Error, no enough memory for temp_BBT1\n", __func__);
		return -ENOMEM;
	}
	memset(temp_BBT1, 0xff, page_size);

	temp_BBT2 = kmalloc(page_size, GFP_KERNEL);
	if (!temp_BBT2) {
		printk("%s: Error, no enough memory for temp_BBT2\n", __func__);
		kfree(temp_BBT1);
		return -ENOMEM;
	}
	memset(temp_BBT2, 0xff, page_size);
	rc = rtk_nand_read_bbt(mtd, 64, temp_BBT1);
	if (rc < 0) {
		printk("Read bbt1 FAIL\n");
		if (rtk_nand_read_bbt(mtd, 128, temp_BBT2) < 0) {
			printk("Read bbt2 FAIL, system suspend.\n");
			rtk_nand_disable_cpu();
		} else {
			if (rtk_nand_bbt_crc_check(temp_BBT2, &crc2, &count2) < 0) {
				printk("[%s] bbt2 crc check fail , system suspend !!\n", __func__);
				rtk_nand_disable_cpu();
			} else {
				printk("[%s] bbt2 crc check OK , load bbt from BBT2.\n", __func__);
				memcpy(this->bbt, temp_BBT2 + CRCSIZE, sizeof(BB_t)*RBA);
				this->bbt_crc = crc2;

				/* -1 means can read block but get bbt fail.
				 * -2 means can't read block completely.
				 */
				if (rc == -1)
					sync_need = 1;
			}
		}
	} else {
		printk("Read bbt1 OK\n");
		if (rtk_nand_bbt_crc_check(temp_BBT1, &crc1, &count1) < 0) {
			printk("[%s] bbt1 crc check fail\n", __func__);
			rc = rtk_nand_read_bbt(mtd, 128, temp_BBT2);
			if (rc < 0) {
				printk("[%s] read bbt2 fail. system suspend\n", __func__);
				rtk_nand_disable_cpu();
			} else {
				printk("Read bbt2 OK\n");
				if (rtk_nand_bbt_crc_check(temp_BBT2, &crc2, &count2) < 0) {
					printk("[%s] bbt2 crc check fail. system suspend\n", __func__);
					rtk_nand_disable_cpu();
				} else {
					printk("[%s] bbt2 crc check OK , load bbt from BBT2.\n", __func__);
					memcpy(this->bbt, temp_BBT2 + CRCSIZE, sizeof(BB_t)*RBA);
					this->bbt_crc = crc2;
					sync_need = 1;
				}
			}
		} else {
			printk("[%s] bbt1 crc check ok\n", __func__);
			rc = rtk_nand_read_bbt(mtd, 128, temp_BBT2);
			if (rc < 0) {
				printk("[%s] read bbt2 fail. load bbt from BBT1\n", __func__);
				memcpy(this->bbt, temp_BBT1 + CRCSIZE, sizeof(BB_t)*RBA);
				this->bbt_crc = crc1;
				if (rc == -1)
					sync_need = 2;
			} else {
				printk("Read bbt2 OK\n");
				if (rtk_nand_bbt_crc_check(temp_BBT2, &crc2, &count2) < 0) {
					printk("[%s] bbt2 crc check fail. load bbt from BBT1\n", __func__);
					memcpy(this->bbt, temp_BBT1 + CRCSIZE, sizeof(BB_t)*RBA);
					this->bbt_crc = crc1;
					sync_need = 2;
				} else {
					printk("[%s] bbt2 crc check OK , compare count of bbt. count1:[%u] count2:[%u]\n",
						__func__, count1, count2);

					if (count1 == count2) {
						printk("[%s] count1 == count2, load bbt from BBT1\n", __func__);
						memcpy(this->bbt, temp_BBT1 + CRCSIZE, sizeof(BB_t)*RBA);
						this->bbt_crc = crc1;
					} else {
						if (count1 > count2) {
							printk("[%s] count1 > count2, load bbt from BBT1\n", __func__);
							memcpy(this->bbt, temp_BBT1 + CRCSIZE, sizeof(BB_t)*RBA);
							this->bbt_crc = crc1;
							sync_need = 2;
						} else {
							printk("[%s] count1 < count2, load bbt from BBT2\n", __func__);
							memcpy(this->bbt, temp_BBT2 + CRCSIZE, sizeof(BB_t)*RBA);
							this->bbt_crc = crc2;
							sync_need = 1;
						}
					}
				}
			}
		} /* if (rtk_nand_bbt_crc_check(temp_BBT1, &crc1, &count1) < 0) */
	} /* if ((rc = rtk_nand_read_bbt(mtd, 64, temp_BBT1)) < 0) */

	if (sync_need) {
		printk("[%s] sync bbt1 and bbt2.\n", __func__);
		rtk_nand_bbt_sync(mtd, sync_need);
	}

	//dump_BBT(mtd);
	if (temp_BBT1)
		kfree(temp_BBT1);
	if (temp_BBT2)
		kfree(temp_BBT2);

	return 0;
}


int rtk_nand_scan(struct mtd_info *mtd, int maxchips)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	unsigned char id[6];
	unsigned int device_size = 0;
	unsigned int i;
	unsigned int nand_type_id;
	int rtk_lookup_table_flag = 0;
	unsigned char maker_code;
	unsigned char device_code;
	unsigned char B5th;
	unsigned char B6th;
	unsigned int block_size;
	unsigned int num_chips = 1;
	uint64_t chip_size = 0;
	unsigned int num_chips_probe = 1;
	uint64_t result = 0;
	uint64_t div_res = 0;
	unsigned int list_num = 0;

	g_rtk_mtd = mtd;

	if (!this->select_chip)
		this->select_chip = nand_select_chip;

	if (!this->scan_bbt)
		this->scan_bbt = rtk_nand_scan_bbt;

	this->active_chip = 0;
	this->select_chip(mtd, 0);

	mtd->name = "rtk_nand";

	while (1) {
		/* read 3 times for timing issue. */
		this->read_id(mtd, id);
		this->read_id(mtd, id);
		this->read_id(mtd, id);

		this->maker_code = maker_code = id[0];
		this->device_code = device_code = id[1];
		nand_type_id = id[3]<<24 | id[2]<<16 | device_code<<8 | maker_code;
		B5th = id[4];
		B6th = id[5];

		printk("[Phoenix] READ ID:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", id[0], id[1], id[2], id[3], id[4], id[5]);

		i = 1;

		if (i > 1) {
			num_chips_probe = i;
			printk(KERN_INFO "NAND Flash Controller detects %d dies\n", num_chips_probe);
		}

		list_num = sizeof(n_device)/sizeof(n_device_type);

		for (i = 0; i < list_num; i++) {
			if (n_device[i].id == nand_type_id &&
				((n_device[i].id5 == 0xff)?1:(n_device[i].id5 == B5th)) &&
				((n_device[i].id6 == 0xff)?1:(n_device[i].id6 == B6th))) {

				if (nand_type_id != HY27UT084G2M) {
					REG_WRITE_U32(REG_MULTI_CHNL_MODE, 0x20);
				}

				printk("One %s chip on board with index(%d)\n", n_device[i].string, i);

				device_size = n_device[i].size;
				chip_size = n_device[i].size;
				page_size = n_device[i].PageSize;
				block_size = n_device[i].BlockSize;

				if (n_device[i].ecc_bit == 0x0)
					oob_size = 64;
				else
					oob_size = 128;

				num_chips = 1;
				isLastPage = 0;
				rtk_lookup_table_flag = 1;

				REG_WRITE_U32(REG_TIME_PARA1, n_device[i].t1);
				REG_WRITE_U32(REG_TIME_PARA2, n_device[i].t2);
				REG_WRITE_U32(REG_TIME_PARA3, n_device[i].t3);

				printk("index(%d) nand part=%s, id=0x%x, device_size=0x%x, page_size=0x%x, block_size=0x%x, oob_size=0x%x, "
				       "num_chips=0x%x, isLastPage=0x%x, ecc_sel=0x%x\n",
					i, n_device[i].string, n_device[i].id, device_size,
					page_size, block_size, oob_size, num_chips,
					isLastPage, n_device[i].ecc_bit);

				break;
			}
		}

		if (!rtk_lookup_table_flag) {
			printk("Warning: Lookup Table do not have this nand flash !!\n");
			printk ("%s: Manufacture ID=0x%02x, Chip ID=0x%02x, 3thID=0x%02x, 4thID=0x%02x, 5thID=0x%02x, 6thID=0x%02x\n",
				mtd->name, id[0], id[1], id[2], id[3], id[4], id[5]);
			return -1;
		}

		/* prepare nand info for linux kernel */
		sprintf(rtknand_info, "nandinfo=id:%x,ds:%u,ps:%d,bs:%d,t1:%d,t2:%d,t3:%d,eb:%d",
				 n_device[i].id, n_device[i].size, n_device[i].PageSize,
				 n_device[i].BlockSize, n_device[i].t1, n_device[i].t2,
				 n_device[i].t3, n_device[i].ecc_bit);

		this->page_shift = generic_ffs(page_size)-1;
		this->phys_erase_shift = generic_ffs(block_size)-1;
		this->oob_shift = generic_ffs(oob_size)-1;
		ppb = this->ppb = block_size >> this->page_shift;
		ppb_shift = this->phys_erase_shift - this->page_shift;

		if (chip_size) {
			this->block_num = chip_size >> this->phys_erase_shift;
			this->page_num = chip_size >> this->page_shift;
			this->chipsize = chip_size;
			this->device_size = device_size;
			this->chip_shift =  generic_ffs(this->chipsize)-1;
		}


		this->pagemask = (this->chipsize >> this->page_shift) - 1;
		mtd->oobsize = this->oob_size = oob_size;
		mtd->writesize = mtd->oobblock = page_size;
		mtd->erasesize = block_size;
		mtd->writesize_shift = this->page_shift;
		mtd->erasesize_shift = this->phys_erase_shift;
		this->ecc_select = n_device[i].ecc_bit;

		break;

	}

	switch (this->ecc_select) {
	case 0x0:
		g_eccbits = 6;
		break;
	case 0x1:
		g_eccbits = 12;
		break;
	default:
		g_eccbits = 6;
		break;
	}

	this->select_chip(mtd, 0);

	if (num_chips != num_chips_probe)
		this->numchips = num_chips_probe;
	else
		this->numchips = num_chips;

	div_res = mtd->size = this->numchips * this->chipsize;

	mtd->u32RBApercentage = this->RBA_PERCENT;
	RBA = ((device_size / block_size) / 100) * this->RBA_PERCENT;

	printk("RBA:[%u]\n", RBA);

	this->bbt = kmalloc(sizeof(BB_t)*RBA, GFP_KERNEL);
	if (!this->bbt) {
		printk("%s: Error, no enough memory for BBT\n", __func__);
		return -1;
	}
	memset(this->bbt, 0,  sizeof(BB_t)*RBA);

	this->g_databuf = kmalloc(page_size, GFP_KERNEL);
	if (!this->g_databuf) {
		printk("%s: Error, no enough memory for g_databuf\n", __func__);
		return -ENOMEM;
	}
	memset(this->g_databuf, 0xff, page_size);

	this->g_oobbuf = kmalloc(oob_size, GFP_KERNEL);
	if (!this->g_oobbuf) {
		printk("%s: Error, no enough memory for g_oobbuf\n", __func__);
		return -ENOMEM;
	}
	memset(this->g_oobbuf, 0xff, oob_size);

	mtd->type		= MTD_NANDFLASH;
	mtd->flags		= MTD_CAP_NANDFLASH;
	mtd->ecctype		= MTD_ECC_RTK_HW;
	mtd->erase		= nand_erase;
	mtd->point		= NULL;
	mtd->unpoint		= NULL;
	mtd->read		= nand_read;
	mtd->write		= nand_write;
	mtd->read_ecc		= nand_read_ecc;
	mtd->write_ecc		= nand_write_ecc;
	mtd->read_oob		= nand_read_oob;
	mtd->write_oob		= nand_write_oob;
	mtd->sync		= nand_sync;
	mtd->lock		= NULL;
	mtd->unlock		= NULL;
	mtd->block_isbad	= nand_block_isbad;
	mtd->block_markbad	= NULL;

	mtd->reload_bbt = rtk_nand_scan_bbt;

	return this->scan_bbt(mtd);
}

/* on the fly */
int nand_read_ecc_on_the_fly(struct mtd_info *mtd, loff_t from, size_t len,
			size_t *retlen, u_char *buf, u_char *oob_buf, struct nand_oobinfo *oobsel, u16 cp_mode, int tee_os)
{
	struct nand_chip *this = mtd->priv;
	int data_len, oob_len;
	int rc;
	unsigned int page, page_offset, block, real_block, real_page;
	int chipnr;
	int i;

	if ((from + len) > mtd->size) {
		printk("nand_read_ecc: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	if (NOTALIGNED(mtd, from) || NOTALIGNED(mtd, len)) {
		printk(KERN_NOTICE "nand_read_ecc: Attempt to read not page aligned data\n");
		return -EINVAL;
	}

	this->active_chip = chipnr = 0;
	page = ((int)(from >> this->page_shift)) & this->pagemask;
	page_offset = page & (ppb-1);
	block = page/ppb;

	this->select_chip(mtd, chipnr);

	if (retlen)
		*retlen = 0;

	data_len = oob_len = 0;

	while (data_len < len) {
		real_block = rtk_find_real_blk(mtd, block);
		real_page = real_block*ppb + page_offset;

		if (data_len == 0)
			rc = this->read_ecc_page(mtd, this->active_chip, real_page, &buf[data_len],
					 &oob_buf[oob_len], cp_mode, 1, len, tee_os);
		else
			rc = this->read_ecc_page(mtd, this->active_chip, real_page, &buf[data_len],
					 &oob_buf[oob_len], cp_mode, 0, 0, tee_os);

		if (rc < 0) {
			if (rc == -1) {
				printk("%s: read_ecc_page: Un-correctable HW ECC\n", __func__);
				printk("rtk_read_ecc_page: Un-correctable HW ECC Error at page=%d\n", page);
				if (rtk_badblock_handle(mtd, real_page, block, 0, BACKUP_READ) == 0) {
					printk("Update BBT OK\n");
				} else {
					printk("Update BBT Fail\n");
				}
				return -1;
			} else if (rc == -2) {
				if (rtk_badblock_handle(mtd, real_page, block, 1, BACKUP_READ) == 0) {
					printk("Finish backup bad block\n");
				} else {
					printk("backup bad block fail.\n");
				}
			} else if (rc == -WAIT_TIMEOUT) {
				panic("%s: WAIT_LOOP timeout \n", __func__);
			}
		}

		data_len += page_size;

		if (oob_buf)
			oob_len += oob_size;

		page++;
		page_offset = page & (ppb-1);
		block = page/ppb;
	} /* while (data_len < len) */

	if (retlen) {
		if (data_len == len) {
			*retlen = data_len;
		} else {
			printk("[%s] error: data_len %d != len %d\n", __func__, data_len, len);
			return -1;
		}
	}

	return 0;
}

int nand_read_on_the_fly(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf, u16 cp_mode, int tee_os)
{
	int rc = 0;
	unsigned int rd_len;
	unsigned int tmp_len;

	tmp_len = rd_len = len;

	rc = nand_read_ecc_on_the_fly(mtd, from, len, retlen, buf, NULL, NULL, cp_mode, tee_os);

	if (*retlen == len)
		*retlen = tmp_len;

	return rc;
}

void rtk_rebuild_bbt(void)
{
	printk("rtk_rebuild_bbt() for test .....\n");
	rtk_create_bbt(g_rtk_mtd, 64);
	rtk_create_bbt(g_rtk_mtd, 128);

	dump_BBT(g_rtk_mtd);
}

void rtk_dump_bbt(void)
{
	printk("rtk_dump_bbt() .....\n");

	dump_BBT(g_rtk_mtd);
}

#ifdef RTK_NAND_VERIFY
void rtk_print_buf(char *data, unsigned int len)
{
	int i;

	printk("-----------------------------------------------------------------------------------\n");
	for (i = 0; i < len; i++) {
		printk("0x%x ", data[i]);
		if (i%16 == 15)
			printk("\n");
	}
	printk("-----------------------------------------------------------------------------------\n");
}

void rtk_nand_dump_page(unsigned int block, unsigned int page)
{
	struct nand_chip *this = (struct nand_chip *)g_rtk_mtd->priv;
	int rc = 0;
	char page_data[2048];
	int i;

	memset(page_data, 0xff, page_size);
	memset(this->g_databuf, 0xff, page_size);
	memset(this->g_oobbuf, 0xff, oob_size);

	rc = this->read_ecc_page(g_rtk_mtd, 0, (block*ppb)+page, this->g_databuf, this->g_oobbuf, CP_NF_NONE, 0, 0, 0);

	printk("dump page [%u] rc:[%d]\n", page, rc);

	memcpy(page_data, this->g_databuf, 2048);

	printk("-----------------------------------------------------------------------------------\n");
	for (i = 0; i < 2048; i++) {
		printk("0x%x ", page_data[i]);
		if (i%16 == 15)
			printk("\n");
	}
	printk("-----------------------------------------------------------------------------------\n");
	for (i = 0; i < oob_size; i++) {
		printk("0x%x ", this->g_oobbuf[i]);
		if (i%16 == 15)
			printk("\n");
	}
	printk("-----------------------------------------------------------------------------------\n");
}

void rtk_nand_erase_block(unsigned int block)
{
	struct nand_chip *this = (struct nand_chip *)g_rtk_mtd->priv;
	int rc = 0;

	rc = this->erase_block(g_rtk_mtd, 0, ppb*block);

	printk("erase block [%u] rc:[%d]\n", block, rc);
}

void rtk_nand_write_block_zero(unsigned int block)
{
	struct nand_chip *this = (struct nand_chip *)g_rtk_mtd->priv;
	int rc = 0;
	int i;

	memset(this->g_databuf, 0x0, 2048);
	for (i = 0; i < 20; i++) {
		this->g_databuf[i] = 0xFF;
	}

	for (i = 0; i < 64; i++) {
		rc = this->write_ecc_page(g_rtk_mtd, 0, (ppb*block)+i, this->g_databuf, this->g_oobbuf, 1);
		printk("[%u] write block [%u] page:[%d] rc:[%d]\n", i, block, (ppb*block)+i, rc);
	}

}

void rtk_nand_make_bad_block(unsigned int block)
{
	struct nand_chip *this = (struct nand_chip *)g_rtk_mtd->priv;
	int rc = 0, rc2 = 0;
	unsigned int i = 0;
	unsigned int j = 0;

	while (rc == 0) {
		rc = this->erase_block(g_rtk_mtd, 0, ppb*block);
		j++;
		printk("[%u] erase block [%u] rc:[%d]\n", j, block, rc);

		for (i = 0; i < 64; i++) {
			memset(this->g_databuf, 0x0, 2048);
			rc2 = this->write_ecc_page(g_rtk_mtd, 0, (ppb*block)+i, this->g_databuf, this->g_oobbuf, 1);
			if (rc2 != 0) {
				printk("[%u] write page [%u] rc:[%d]\n", i, (ppb*block)+i, rc);
			}
		}
	}
}

void rtk_nand_fake_bad_block(unsigned int block)
{
	struct nand_chip *this = (struct nand_chip *)g_rtk_mtd->priv;
	int rc;

	memset(this->g_databuf, 0xff, 2048);
	memset(this->g_oobbuf, 0xff, oob_size);

	rc = this->read_ecc_page(g_rtk_mtd, 0, (ppb*block), this->g_databuf, this->g_oobbuf, CP_NF_NONE, 0, 0, 0);
	rtk_print_buf(this->g_oobbuf, oob_size);

	this->g_oobbuf[0] = 0xaa;
	this->g_oobbuf[1] = 0xbb;
	this->g_oobbuf[2] = 0xcc;
	this->g_oobbuf[3] = 0xdd;
	this->g_oobbuf[4] = 0xee;
	this->g_oobbuf[5] = 0x11;
	this->g_oobbuf[6] = 0x22;
	this->g_oobbuf[7] = 0x33;
	this->g_oobbuf[8] = 0x44;
	this->g_oobbuf[9] = 0x55;
	this->g_oobbuf[10] = 0x66;

	rc = this->write_ecc_page_raw(g_rtk_mtd, 0, (ppb*block), this->g_databuf, this->g_oobbuf, 1);
	rtk_nand_dump_page(ppb*block, 0);
}

void rtk_nand_change_bit(char *data)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (*data & (0x1 << i)) {
			*data ^= (0x1 << i);
			break;
		}
	}
}

void rtk_nand_fake_ecc_block(unsigned int block, unsigned int bits)
{
	struct nand_chip *this = (struct nand_chip *)g_rtk_mtd->priv;
	int rc;
	int i;

	memset(this->g_databuf, 0xff, 2048);
	memset(this->g_oobbuf, 0xff, oob_size);

	rc = this->read_ecc_page(g_rtk_mtd, 0, (ppb*block), this->g_databuf, this->g_oobbuf, CP_NF_NONE, 0, 0, 0);

	for (i = 0; i < bits; i++)
		rtk_nand_change_bit(&this->g_databuf[i]);

	rc = this->write_ecc_page_raw(g_rtk_mtd, 0, (ppb*block), this->g_databuf, this->g_oobbuf, 1);
	rtk_nand_dump_page(ppb*block, 0);
}

void rtk_nand_bad_bbt_test(unsigned int block)
{
	int rc;
	unsigned int remap_block;

	rc = rtk_find_available_remap_block(g_rtk_mtd, &remap_block);
        if (rc < 0) {
                printk("[%s] No available re-map block.\n", __func__);
                return rc;
        }

	rtk_update_BBT(g_rtk_mtd, block, block, remap_block);

	rc = rtk_write_bbt(g_rtk_mtd, BBT1PAGE);
	if (rc < 0) {
		printk("%s: write BBT1 Fail.\n", __func__);
	}
}
#endif


void rtk_get_upgrade_info(void)
{
	struct nand_chip *this = (struct nand_chip *)g_rtk_mtd->priv;
	RTK_VER_INFO_T info1;
	RTK_VER_INFO_T info2;

	memset(&info1, 0x0, sizeof(RTK_VER_INFO_T));
	memset(&info2, 0x0, sizeof(RTK_VER_INFO_T));

	this->read_ecc_page(g_rtk_mtd, 0, (ppb*4), this->g_databuf, this->g_oobbuf, CP_NF_NONE, 0, 0, 0);
	memcpy(&info1, this->g_databuf, sizeof(RTK_VER_INFO_T));

	this->read_ecc_page(g_rtk_mtd, 0, (ppb*5), this->g_databuf, this->g_oobbuf, CP_NF_NONE, 0, 0, 0);
	memcpy(&info2, this->g_databuf, sizeof(RTK_VER_INFO_T));

	printk("BLOCK4:\n");
	printk("active_fw		:[%d]\n", info1.active_fw);
	printk("boot_major_version	:[%d]\n", info1.boot_major_version);
	printk("boot_minor_version	:[%d]\n", info1.boot_minor_version);
	printk("system_major_version	:[%d]\n", info1.system_major_version);
	printk("system_minor_version	:[%d]\n", info1.system_minor_version);
	printk("hash			:[%u]\n", info1.hash);

	printk("\nBLOCK5:\n");
	printk("active_fw		:[%d]\n", info2.active_fw);
	printk("boot_major_version	:[%d]\n", info2.boot_major_version);
	printk("boot_minor_version	:[%d]\n", info2.boot_minor_version);
	printk("system_major_version	:[%d]\n", info2.system_major_version);
	printk("system_minor_version	:[%d]\n", info2.system_minor_version);
	printk("hash			:[%u]\n", info2.hash);
}

static unsigned int rtk_hash(char *str, int len)
{
	unsigned int hash = 3798;
	int c;
	int i;

	for (i = 0; i < len; i++) {
		c = *str++;
		hash = ((hash << 5) + hash) + c;
	}

	return hash;
}

int bootfw;

int rtk_get_active_fw(void)
{
	struct nand_chip *this = (struct nand_chip *)g_rtk_mtd->priv;
	RTK_VER_INFO_T info1;
	RTK_VER_INFO_T info2;
	char hash_temp[64] = {0};
	unsigned int hash_value = 0;
	int retlen;
	int rc;

	memset(&info1, 0x0, sizeof(RTK_VER_INFO_T));
	memset(&info2, 0x0, sizeof(RTK_VER_INFO_T));

	rc = nand_read_ecc(g_rtk_mtd, (ppb*4)*page_size, page_size, &retlen, this->g_databuf, NULL, NULL);
	memcpy(&info1, this->g_databuf, sizeof(RTK_VER_INFO_T));

	rc = nand_read_ecc(g_rtk_mtd, (ppb*5)*page_size, page_size, &retlen, this->g_databuf, NULL, NULL);
	memcpy(&info2, this->g_databuf, sizeof(RTK_VER_INFO_T));

	memset(hash_temp, 0, sizeof(hash_temp));
	sprintf(hash_temp, "%d%d%d%d%d", info1.active_fw, info1.boot_major_version, info1.boot_minor_version,
					 info1.system_major_version, info1.system_minor_version);
	printk("hash_temp:[%s][%d]\n", hash_temp, strlen(hash_temp));
	rtk_crc32(hash_temp, strlen(hash_temp), &hash_value);
	printk("info1.hash:[%u] hash_value:[%u]\n", info1.hash, hash_value);
	if (info1.hash == hash_value) {
		printk("BLOCK4 is OK, check BLOCK5\n");

		memset(hash_temp, 0, sizeof(hash_temp));
		hash_value = 0;
		sprintf(hash_temp, "%d%d%d%d%d", info2.active_fw, info2.boot_major_version,
					 info2.boot_minor_version, info2.system_major_version,
					 info2.system_minor_version);
		printk("hash_temp:[%s][%d]\n", hash_temp, strlen(hash_temp));
		rtk_crc32(hash_temp, strlen(hash_temp), &hash_value);
		printk("info2.hash:[%u] hash_value:[%u]\n", info2.hash, hash_value);
		if (info2.hash == hash_value) {
			printk("BLOCK5 is OK\n");

			if (info1.system_major_version > info2.system_major_version) {
				bootfw = info1.active_fw;
			} else if (info1.system_major_version < info2.system_major_version) {
				bootfw = info2.active_fw;
			} else if (info1.system_major_version == info2.system_major_version) {
				if (info1.system_minor_version > info2.system_minor_version) {
					bootfw = info1.active_fw;
				} else if (info1.system_minor_version < info2.system_minor_version) {
					bootfw = info2.active_fw;
				} else if (info1.system_minor_version == info2.system_minor_version) {
					bootfw = info1.active_fw;
				}
			}

		} else {
			printk("BLOCK5 is NG.\n");
			bootfw = info1.active_fw;
		}

	} else {
		printk("BLOCK4 is NG, check BLOCK5\n");

		memset(hash_temp, 0, sizeof(hash_temp));
		hash_value = 0;
		sprintf(hash_temp, "%d%d%d%d%d", info2.active_fw, info2.boot_major_version,
					 info2.boot_minor_version, info2.system_major_version,
					 info2.system_minor_version);
		printk("hash_temp:[%s][%d]\n", hash_temp, strlen(hash_temp));
		rtk_crc32(hash_temp, strlen(hash_temp), &hash_value);
		printk("info2.hash:[%u] hash_value:[%u]\n", info2.hash, hash_value);
		if (info2.hash == hash_value) {
			printk("BLOCK5 is OK\n");
			bootfw = info2.active_fw;
		} else {
			printk("BLOCK5 is NG.\n");
			bootfw = -1;
		}
	} /* if(info1.hash == hash_value) */

	memset(rtk_bootfw, 0x0, sizeof(rtk_bootfw));
	sprintf(rtk_bootfw, ",bf:%d", bootfw);
	strcat(rtknand_info, rtk_bootfw);
	printk("rtknand_info:[%s]\n", rtknand_info);

	return bootfw;
}

void rtk_erase_fwblock(void)
{
	struct nand_chip *this = (struct nand_chip *)g_rtk_mtd->priv;
	struct erase_info instr;

	if (bootfw == 1)
		instr.addr = ppb*page_size*4;
	else if (bootfw == 2)
		instr.addr = ppb*page_size*5;

	instr.len = page_size;

	nand_erase(g_rtk_mtd, &instr);

	return;
}
