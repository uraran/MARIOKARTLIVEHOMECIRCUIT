/*
 * nand_base_rtk.c - nand driver
 *
 * Copyright (c) 2017 Realtek Semiconductor Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/parser.h>
#include <linux/semaphore.h>
#include <mtd/mtd-abi.h>
#include <linux/mtd/rtk_nand_reg.h>
#include <linux/mtd/rtk_nand.h>
#include <linux/mtd/rtk_nand_list.h>
#include <asm/page.h>
#include <linux/jiffies.h>
#include <linux/dma-mapping.h>

static DEFINE_MUTEX(rtk_nand_mutex);
#define PLL_SCPU1       0x18000100

#define BACKUP_WRITE	1
#define BACKUP_READ	2
#define BACKUP_ERASE	3

#define MAX_NR_LENGTH	(1024*512)
#define MAX_NW_LENGTH	(1024*512)
#define TIMEOUT_LIMIT	1

static unsigned char *nwBuffer;
static unsigned char *nrBuffer;
dma_addr_t nrPhys_addr;
dma_addr_t oobPhys_addr;
dma_addr_t nwPhys_addr;
static unsigned char *tempBuffer;
dma_addr_t tempPhys_addr;
static int bEnterSuspend;

#define NOTALIGNED(mtd, x) ((x & (mtd->writesize-1)) != 0)
unsigned int g_eccbits;
unsigned int g_boot_flag = 1;
static struct mtd_info *g_rtk_mtd;
static RTK_VER_INFO_T fw_info1;
static RTK_VER_INFO_T fw_info2;

#define check_end(mtd, addr, len)					\
do {									\
	if (mtd->size == 0) {						\
		mutex_unlock(&rtk_nand_mutex);				\
		return -ENODEV;						\
	} else if ((addr + len) > mtd->size) {				\
		printk(						\
			"%s: attempt access past end of device\n",	\
			__func__);					\
		mutex_unlock(&rtk_nand_mutex);				\
		return -EINVAL;						\
	}								\
} while (0)

#define check_block_align(mtd, addr)					\
do {									\
	if (addr & (mtd->erasesize - 1)) {				\
		printk(						\
			"%s: attempt access non-block-aligned data\n",	\
			__func__);					\
		mutex_unlock(&rtk_nand_mutex);				\
		return -EINVAL;						\
	}								\
} while (0)

unsigned int g_mtd_BootcodeSize;


/* RTK Nand Chip ID list */
static int rtd_get_set_nand_info(void);
extern void rtk_nand_reset(void);
extern char g_rtk_nandinfo_line[64];
static device_type_t g_nand_device;

/* NAND low-level MTD interface functions */
static int nand_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
static int nand_read_ecc(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, unsigned char *buf,
				 u_char *oob_buf, struct nand_oobinfo *oobsel, unsigned char *buf_phy);
static int nand_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);
static int nand_write_ecc(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const unsigned char *buf,
				 const u_char *oob_buf, struct nand_oobinfo *oobsel, const unsigned char *buf_phy);
static int nand_erase(struct mtd_info *mtd, struct erase_info *instr);
static void nand_sync(struct mtd_info *mtd);
static int nand_suspend(struct mtd_info *mtd);
static void nand_resume(struct mtd_info *mtd);
static int nand_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops);
static int nand_write_oob(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops);
static int nand_block_isbad(struct mtd_info *mtd, loff_t ofs);

int rtk_update_bbt(struct mtd_info *mtd, __u8 *data_buf, __u8 *oob_buf, BB_t *bbt);


/* Global Variables */
int RBA;
static int oob_size, ppb, isLastPage;
static int page_size;


static unsigned int crc_table[256];
static int crc_table_computed;

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

static unsigned long long rtk_from_to_page(struct mtd_info *mtd, loff_t from)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;

	return (int)(from >> this->page_shift);
}

static unsigned int rtk_find_real_blk(struct mtd_info *mtd, unsigned int blk)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	unsigned int i = 0, real_blk = blk;

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

static void dump_BBT(struct mtd_info *mtd)
{

	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int i;
	int BBs = 0;

	NF_INFO_PRINT("[%s] Nand BBT Content\n", __func__);

	for (i = 0; i < RBA; i++) {
		if (i == 0 && this->bbt[i].BB_die == BB_DIE_INIT && this->bbt[i].bad_block == BB_INIT) {
			NF_INFO_PRINT("Congratulation!! No BBs in this Nand.\n");
			break;
		}
		if (this->bbt[i].bad_block != BB_INIT) {
			NF_INFO_PRINT("[%d] (%d, %u, %d, %u)\n", i, this->bbt[i].BB_die, this->bbt[i].bad_block,
				this->bbt[i].RB_die, this->bbt[i].remap_block);
			BBs++;
		}
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

int rtk_update_bbt_to_flash(struct mtd_info *mtd)
{
	int rc1 = 0;
	int rc2 = 0;
	struct nand_chip *this = mtd->priv;
	unsigned char active_chip = this->active_chip;
	BB_t *bbt = this->bbt;
	u_char *new_buf = NULL;

	new_buf = tempBuffer;
	if (new_buf == NULL) {
		NF_ERR_PRINT("bbt Buffer error......\n");
		return -ENOMEM;
	}

	this->select_chip(mtd, 0);
	this->g_oobbuf[0] = BBT_TAG;

	this->bbt_crc = rtk_nand_bbt_crc_calculate(bbt);

	memcpy(new_buf, &(this->bbt_crc), sizeof(unsigned int));
	memcpy(new_buf + 4, bbt, sizeof(BB_t)*RBA);
	rc1 = this->erase_block(mtd, 0, 64);

	if (rc1 < 0) {
		NF_ERR_PRINT("[%s]error: erase block 1 failure\n", __func__);
	} else {
		rc1 = this->write_ecc_page(mtd, 0, 64, new_buf, this->g_oobbuf, (dma_addr_t *)tempPhys_addr);
		if (rc1 < 0) {
			NF_ERR_PRINT("[%s]update BBT B1 page 64 failure\n", __func__);
		}
	}
	rc2 = this->erase_block(mtd, 0, 128);
	if (rc2 < 0) {
		NF_ERR_PRINT("[%s]error: erase block 2 failure\n", __func__);
	} else {
		rc2 = this->write_ecc_page(mtd, 0, 128, new_buf, this->g_oobbuf, (dma_addr_t *)tempPhys_addr);
		if (rc2 < 0)
			NF_ERR_PRINT("[%s]update BBT B2 failure\n", __func__);

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

	NF_DEBUG_PRINT("check...%d\n", check_data);

	return check_data;
}

static int rtk_backup_block(struct mtd_info *mtd, __u32 src_blk, __u32 remap_blk, __u32 page_offset, __u32 mode)
{
	struct nand_chip *this = mtd->priv;
	unsigned int start_page = src_blk * ppb;
	unsigned int backup_start_page = remap_blk * ppb;
	unsigned int i;
	char *cmp_data = NULL;
	int rc = 0;

	cmp_data = kmalloc(page_size, GFP_KERNEL);
	if (!cmp_data) {
		NF_ERR_PRINT("kmalloc fail\n");
		rc = -2;
		goto rtk_backup_block_end;
	}

	NF_INFO_PRINT("Start to Backup block from %u to %u\n", src_blk, remap_blk);
	rc = this->erase_block(mtd, 0, backup_start_page);
	if (rc < 0) {
		if (rc == -1) {
			NF_ERR_PRINT("[%s]error: erase block %u failure\n",
					__func__, remap_blk);
		} else if (rc == -2) {
			NF_ERR_PRINT("[%s]error: erase page %u align failure\n",
					 __func__, backup_start_page);
		}
		goto rtk_backup_block_end;
	}

	for (i = 0; i < ppb; i++) {
		memset(tempBuffer, 0x0, page_size);

		rc = this->read_ecc_page(mtd, this->active_chip, start_page+i, tempBuffer,
					 this->g_oobbuf, CP_NF_NONE, (dma_addr_t *)tempPhys_addr);
		if (rc == -1) {
			NF_INFO_PRINT("Read original page error.\n");
			rc = -2;
			goto rtk_backup_block_end;
		} else if ((rc == 1) || ((i == page_offset) && (mode == BACKUP_WRITE))) {
			NF_INFO_PRINT("page is blank or skip original write page\n");
			continue;
		}

		rc = this->write_ecc_page(mtd, this->active_chip, backup_start_page+i, tempBuffer,
					 this->g_oobbuf, (dma_addr_t *)tempPhys_addr);
		if (rc < 0) {
			NF_ERR_PRINT("write page(%u) in remap block fail. (%d)\n", i, rc);
			goto rtk_backup_block_end;
		}

		memcpy(cmp_data, tempBuffer, page_size);
		memset(tempBuffer, 0x0, page_size);

		rc = this->read_ecc_page(mtd, this->active_chip, backup_start_page+i, tempBuffer,
					 this->g_oobbuf, CP_NF_NONE, (dma_addr_t *)tempPhys_addr);
		if (rc == -1) {
			NF_ERR_PRINT("read page(%u) in remap block fail. (%d)\n", i, rc);
			goto rtk_backup_block_end;
		}
	}

rtk_backup_block_end:
	kfree(cmp_data);

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
				NF_ERR_PRINT("[%s] erase/write re-map block error.\n", __func__);
				rtk_badblock_handle_update_BBT(mtd, remap_block);
				goto rtk_badblock_handle_redo;
			} else if (rc == -2) {
				NF_ERR_PRINT("[%s] original page read error, memory alloc fail or data verification fail.\n",
						__FUNCTION__);
				return rc;
			}
		}
	}

	rtk_update_BBT(mtd, ogl_block, src_block, remap_block);

	rc = rtk_update_bbt_to_flash(mtd);
	if (rc < 0) {
		NF_ERR_PRINT("[%s] rtk_update_bbt_to_flash() fails.\n", __func__);
		return rc;
	}

	NF_INFO_PRINT("[%s] rtk_update_bbt() successfully.\n", __func__);

	return rc;
}

static int rtk_block_isbad(struct mtd_info *mtd, u16 chipnr, loff_t ofs)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	unsigned int page, block, page_offset;
	unsigned char block_status_p1;
	unsigned char block_status_p1_org;

	this->active_chip = chipnr = 0;
	page = ofs >> this->page_shift;
	block_status_p1_org = this->g_oobbuf[0];
	page_offset = page & (ppb-1);
	block = page/ppb;

	if (isLastPage) {
		page = block*ppb + (ppb-1);
	}

	if (this->read_ecc_page(mtd, this->active_chip, ppb, this->g_databuf, this->g_oobbuf,
				 CP_NF_NONE, (dma_addr_t *)&oobPhys_addr)) {
		NF_ERR_PRINT("%s: read_oob page=%d failed\n", __func__, page);
		return 1;
	}
	block_status_p1 = this->g_oobbuf[0];

	if (block_status_p1 != 0xff) {
		NF_ERR_PRINT("WARNING: Die %d: block=%d is bad, block_status_p1=0x%x\n",
				 chipnr, block, block_status_p1);
		return -1;
	}

	return 0;
}

static int nand_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
	int rc = 0;
	size_t new_len = 0;
	u_char *new_buf = NULL;
	loff_t new_from = from;
	loff_t new_end;
	struct nand_chip *this = mtd->priv;
	size_t read_ret = 0;
	size_t read_len = 0;
	size_t copy_len = 0;
	size_t copied_len = 0;
	int first_read = 1;
	int buf_shift = 0;
	int i = 0;

	mutex_lock(&rtk_nand_mutex);

	if (len <= 0) {
		*retlen = 0;
		NF_ERR_PRINT("%s:%d read non-positive len=%zu\n", __func__, __LINE__, len);
		mutex_unlock(&rtk_nand_mutex);
		return 0;
	}

	if (NOTALIGNED(mtd, from)) {
		new_from = from & this->page_idx_mask;
	}
	new_end = (from + len + page_size - 1) & this->page_idx_mask;
	new_len = new_end - new_from;

	new_buf = nrBuffer;

	if (new_buf) {
		while (new_len > 0) {
			read_len = (new_len > MAX_NR_LENGTH) ? MAX_NR_LENGTH : new_len;

			rc = nand_read_ecc(mtd, new_from+(i*MAX_NR_LENGTH), read_len, &read_ret, \
						new_buf, NULL, NULL, (unsigned char *)nrPhys_addr);

			if (rc == 0) {
				//memcpy(buf, new_buf+(from-new_from), len);
				copy_len = (new_len > MAX_NR_LENGTH) ? \
					(MAX_NR_LENGTH - (first_read*(from-new_from))) : (len - copied_len);
				memcpy(buf + copied_len, new_buf + (first_read*(from-new_from)), copy_len);
				copied_len = copied_len + copy_len;
				first_read = 0;
				new_len = new_len - read_len;
				i++;
			}
			else {
				*retlen = 0;
				goto nand_read_end;
			}
		}

		*retlen = len;
	}

nand_read_end:
	mutex_unlock(&rtk_nand_mutex);

	return rc;
}

static int nand_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int rc = 0;
	unsigned int page = 0, real_page = 0;
	unsigned int page_offset = 0;
	unsigned int block = 0;
	unsigned int real_block = 0;
	loff_t new_from = from;

	from += ops->ooboffs;

	if (ops->datbuf != NULL && ops->len != 0) {
		ops->retlen = 0;
		nand_read(mtd, from, ops->len, &ops->retlen, ops->datbuf);
	}

	mutex_lock(&rtk_nand_mutex);

	if (ops->oobbuf != NULL && ops->ooblen != 0) {
		ops->oobretlen = 0;

		if (NOTALIGNED(mtd, from)) {
			new_from = from & this->page_idx_mask;
		}

		page = rtk_from_to_page(mtd, new_from);
		page_offset =  page % ppb;
		block = page/ppb;

		real_block = rtk_find_real_blk(mtd, block);

		real_page = (real_block * ppb) + page_offset;

		rc = this->read_ecc_page(mtd, this->active_chip, real_page, this->g_databuf,
				 this->g_oobbuf, CP_NF_NONE, (dma_addr_t *)&nrPhys_addr);

		memcpy(ops->oobbuf, this->g_oobbuf, mtd->oobsize);

		ops->oobretlen = mtd->oobsize;
	}

	mutex_unlock(&rtk_nand_mutex);

	return rc;
}

static int nand_read_ecc(struct mtd_info *mtd, loff_t from, size_t len,
			size_t *retlen, unsigned char *buf, unsigned char *oob_buf,
			 struct nand_oobinfo *oobsel, unsigned char *buf_phy)
{
	struct nand_chip *this = mtd->priv;
	__u32 page, realpage;
	__u64 page_ppb;
	int data_len, oob_len;
	int rc, retry_count;
	__u32 old_page, page_offset, block, real_block;
	int chipnr, chipnr_remap;

	if ((from + len) > mtd->size) {
		NF_ERR_PRINT("nand_read_ecc: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	if (NOTALIGNED(mtd, from) || NOTALIGNED(mtd, len)) {
		NF_ERR_PRINT("nand_read_ecc: Attempt to read not page aligned data\n");
		*retlen = 0;
		return -EINVAL;
	}

	realpage = (int)(from >> this->page_shift);
	old_page = page = realpage & this->pagemask;
	page_offset = page & (ppb-1);
	page_ppb = page;
	do_div(page_ppb, ppb);
	block = (unsigned int)page_ppb;

	this->active_chip = chipnr = chipnr_remap = 0;

	this->select_chip(mtd, chipnr);

	if (retlen)
		*retlen = 0;

	data_len = oob_len = 0;
	retry_count = 0;

	while (data_len < len) {
read_after_backup:
		real_block = rtk_find_real_blk(mtd, block);
		page = real_block*ppb + page_offset;

		rc = this->read_ecc_page(mtd, this->active_chip, page, &buf[data_len], NULL, CP_NF_NONE,
					 (dma_addr_t *)&buf_phy[data_len]);

		if (rc < 0) {
			if (rc == -1) {
				NF_ERR_PRINT("rtk_read_ecc_page: Un-correctable HW ECC Error at page=%u block:[%u] real_block:[%u]\n",
					page, block, real_block);

				if (rtk_badblock_handle(mtd, page, block, 0, BACKUP_READ) == 0) {
					NF_ERR_PRINT("Finish backup bad block. read after backup.......\n");
					NF_ERR_PRINT("Update BBT OK\n");
				} else {
					NF_ERR_PRINT("Update BBT FAIL\n");
				}
				return -1;
			} else if (rc == -2) {
				NF_ERR_PRINT("read_ecc_page: rc:%d read ppb:%u, page:%u failed\n", rc, ppb, page);
				NF_ERR_PRINT("Prepare to to backup\n");
				if (rtk_badblock_handle(mtd, page, block, 1, BACKUP_READ) == 0) {
					NF_ERR_PRINT("Finish backup bad block. read after backup.......\n");
					goto read_after_backup;
				} else {
					NF_ERR_PRINT("backup bad block fail.\n");
				}
			} else if (rc == -WAIT_TIMEOUT) {

				if (retry_count < TIMEOUT_LIMIT) {
					NF_ERR_PRINT("WAIT_LOOP timeout, read after nand reset\n");
					rtk_nand_reset();
					retry_count++;

					goto read_after_backup;

				} else {
					panic("WAIT_LOOP timeout over expected times \n");
				}

			} else {
				NF_ERR_PRINT("%s: read_ecc_page:  unknow failed\n", __func__);
			}
		}

		data_len += page_size;

		if (oob_buf)
			oob_len += oob_size;

		old_page++;
		page_offset = old_page & (ppb-1);
		block = old_page/ppb;
	}

	if (retlen) {
		if (data_len == len) {
			*retlen = data_len;
		} else {
			NF_ERR_PRINT("[%s] error: data_len %d != len %zu\n", __func__, data_len, len);
			return -1;
		}
	}

	return 0;
}

static int nand_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
	int rc = 0;
	u_char *new_buf = NULL;
	int write_ret = 0;
	int write_len = 0;
	int i = 0;

	mutex_lock(&rtk_nand_mutex);

	if (bEnterSuspend) {
		NF_ERR_PRINT("[%s] - prevent cmd execute while in suspend stage\n", __func__);
		mutex_unlock(&rtk_nand_mutex);
		return 0;
	}

	new_buf = nwBuffer;

	*retlen = 0;

	if (new_buf) {
		while (len > 0) {
			write_len = (len > MAX_NW_LENGTH) ? MAX_NW_LENGTH : len;

			memcpy(new_buf, buf + (i*MAX_NW_LENGTH), write_len);

			rc = nand_write_ecc(mtd, to + (i*MAX_NW_LENGTH), write_len, &write_ret, \
						new_buf, NULL, NULL, (unsigned char *)nwPhys_addr);

			if (rc == 0) {
				*retlen = *retlen + write_ret;
				len = len - write_len;
				i++;
			} else {
				*retlen = 0;
				goto nand_write_end;
			}
		}
	}

nand_write_end:
	mutex_unlock(&rtk_nand_mutex);

	return rc;
}

static int nand_write_ext(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
	struct nand_chip *this = mtd->priv;
	int rc = 0;
	u_char *new_buf = NULL;

	if (bEnterSuspend) {
		NF_ERR_PRINT("[%s] - prevent cmd execute while in suspend stage\n", __func__);
		return 0;
	}

	new_buf = nwBuffer;

	if (new_buf) {
		if (ops->datbuf != NULL && ops->len != 0) {
			memcpy(new_buf, ops->datbuf, ops->len);
		}

		if (ops->oobbuf != NULL && ops->ooblen != 0) {
			memcpy(this->g_oobbuf, ops->oobbuf, ops->ooblen);
		}

		if (ops->datbuf != NULL && ops->oobbuf != NULL) {
			rc = nand_write_ecc(mtd, to, ops->len, &ops->retlen, new_buf, this->g_oobbuf,
						 NULL, (unsigned char *)nwPhys_addr);
		} else if (ops->datbuf != NULL && ops->oobbuf == NULL) {
			rc = nand_write_ecc(mtd, to, ops->len, &ops->retlen, new_buf, NULL, NULL,
						 (unsigned char *)nwPhys_addr);
		} else if (ops->datbuf == NULL && ops->oobbuf != NULL) {
			rc = nand_write_ecc(mtd, to, ops->len, &ops->retlen, NULL, this->g_oobbuf,
						 NULL, (unsigned char *)nwPhys_addr);
		}

		if (rc == 0) {
			ops->oobretlen = ops->ooblen;
		} else {
			NF_ERR_PRINT("nand_write_ext() FAIL\n");
			ops->oobretlen = 0;
		}
	}

	return rc;
}

static int nand_write_oob(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
	struct nand_chip *this = mtd->priv;
	int rc = 0;
	u_char *new_buf = NULL;

	mutex_lock(&rtk_nand_mutex);

	rc = nand_write_ext(mtd, to, ops);

	mutex_unlock(&rtk_nand_mutex);

	return rc;
}

static int nand_write_ecc(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen,
			const u_char *buf, const u_char *oob_buf, struct nand_oobinfo *oobsel, const u_char *buf_phy)
{
	struct nand_chip *this = mtd->priv;
	__u32 page, realpage;
	__u64 page_ppb;
	int data_len, oob_len;
	int rc = 0, retry_count;
	unsigned int old_page, page_offset, block, real_block;
	int chipnr, chipnr_remap;

	if (bEnterSuspend) {
		NF_ERR_PRINT("[%s] - prevent cmd execute while in suspend stage\n", __func__);
		return 0;
	}

	if ((to + len) > mtd->size) {
		NF_ERR_PRINT("nand_write_ecc: Attempt write beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	if (NOTALIGNED(mtd, to) || NOTALIGNED(mtd, len)) {
		NF_ERR_PRINT("nand_write_ecc: Attempt to write not page aligned data mtd size: %x\n", mtd->writesize-1);
		*retlen = 0;
		return -EINVAL;
	}

	realpage = (unsigned int)(to >> this->page_shift);
	old_page = page = realpage & this->pagemask;
	page_offset = page & (ppb-1);
	page_ppb = page;
	do_div(page_ppb, ppb);
	block = (unsigned int)page_ppb;

	NF_DEBUG_PRINT("[NAND_DBG][%s] READY write to block 0x%x, page 0x%x", __func__, block, page);

	this->active_chip = chipnr = chipnr_remap = 0;

	this->select_chip(mtd, chipnr);

	if (retlen)
		*retlen = 0;

	data_len = oob_len = 0;
	retry_count = 0;

	while (data_len < len) {
write_after_backup:
		real_block = rtk_find_real_blk(mtd, block);

		page = real_block*ppb + page_offset;

		NF_DEBUG_PRINT("Confirm to write blk:[%u] real_block:[%d], page:[%u]\n",
				(__u32)page/ppb, real_block, (__u32)page%ppb);

		if (buf && oob_buf) {
			rc = this->write_ecc_page(mtd, this->active_chip, page, &buf[data_len],
						 &oob_buf[oob_len], (dma_addr_t *)&buf_phy[data_len]);
		} else if (buf && !oob_buf) {
			rc = this->write_ecc_page(mtd, this->active_chip, page, &buf[data_len], NULL,
						 (dma_addr_t *)&buf_phy[data_len]);
		} else if (!buf && oob_buf) {
			rc = this->write_oob(mtd, this->active_chip, page, oob_size, &oob_buf[oob_len]);
		} else {
			return -EINVAL;
		}


		if (rc < 0) {
			if (rc == -WAIT_TIMEOUT) {
				if (retry_count < TIMEOUT_LIMIT) {
					NF_ERR_PRINT("WAIT_LOOP timeout, write after nand reset\n");
					rtk_nand_reset();
					retry_count++;

					goto write_after_backup;

				} else {
					panic("WAIT_LOOP timeout over expected times \n");
				}

			} else {
				NF_ERR_PRINT("%s: write_ecc_page:  write blk:%u, page_offset:[%u]\n",
						 __func__, (__u32)page/ppb, page_offset);
				if (rtk_badblock_handle(mtd, page, block, 1, BACKUP_WRITE) == 0) {
					NF_ERR_PRINT("Finish backup bad block, re-write ...\n");
					goto write_after_backup;
				} else {
					NF_ERR_PRINT("backup bad block fail.\n");
				}
			}
		}

		data_len += page_size;
		oob_len += oob_size;
		old_page++;
		page_offset = old_page & (ppb-1);
		block = old_page/ppb;
	}

	if (retlen) {

		if (data_len == len) {
			*retlen = data_len;
		} else {
			NF_ERR_PRINT("[%s] error: data_len %d != len %zu\n", __func__, data_len, len);
			return -1;
		}
	}

	return 0;
}

static int nand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	return nand_erase_nand(mtd, instr, 0);
}

int nand_erase_nand(struct mtd_info *mtd, struct erase_info *instr, int allowbbt)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	u_int32_t addr = instr->addr;
	u_int32_t len = instr->len;
	int page, chipnr;
	int old_page, block, real_block;
	u_int32_t elen = 0;
	int rc = 0, retry_count;
	int realpage, chipnr_remap;

	mutex_lock(&rtk_nand_mutex);

	if (bEnterSuspend) {
		NF_ERR_PRINT("[%s] - prevent cmd execute while in suspend stage\n", __func__);
		mutex_unlock(&rtk_nand_mutex);
		return 0;
	}

	check_end(mtd, addr, len);
	check_block_align(mtd, addr);

	instr->fail_addr = 0xffffffff;

	realpage =  addr >> this->page_shift;
	old_page = page = realpage & this->pagemask;
	block = page/ppb;

	this->active_chip = chipnr = chipnr_remap = 0;
	this->select_chip(mtd, chipnr);

	instr->state = MTD_ERASING;
	retry_count = 0;

erase_after_bb_handle:
	while (elen < len) {
		real_block = rtk_find_real_blk(mtd, block);

		page = real_block*ppb;
		NF_DEBUG_PRINT("confirm to Erase blk[%u][%d] this->active_chip:[%d]\n",
				 page/ppb, real_block, this->active_chip);
		rc = this->erase_block(mtd, this->active_chip, page);

		if (rc == -WAIT_TIMEOUT) {
			if (retry_count < TIMEOUT_LIMIT) {
				NF_ERR_PRINT("WAIT_LOOP timeout, erase after nand reset\n");
				rtk_nand_reset();
				retry_count++;

				goto erase_after_bb_handle;
			} else {
				panic("WAIT_LOOP timeout over expected times \n");
			}
		} else if (rc == -1) {
			NF_ERR_PRINT("%s: block erase failed at page address=%u\n", __func__, page);
			if (rtk_badblock_handle(mtd, page, block, 0, BACKUP_ERASE) == 0) {
				goto erase_after_bb_handle;
			} else {
				instr->fail_addr = (page << this->page_shift);
			}
		} else if (rc == -2) {
			NF_ERR_PRINT("%s: block erase failed at page address=0x%08x not block alignment\n",
					__func__, page);
			goto err;
		}

		if (chipnr != chipnr_remap)
			this->select_chip(mtd, chipnr);

		elen += mtd->erasesize;

		old_page += ppb;

		if (elen < len && !(old_page & this->pagemask)) {
			old_page &= this->pagemask;
			chipnr++;
			this->active_chip = chipnr;
			this->select_chip(mtd, chipnr);
		}

		block = old_page/ppb;
	}
	instr->state = MTD_ERASE_DONE;

	if (!rc)
		mtd_erase_callback(instr);

	mutex_unlock(&rtk_nand_mutex);
err:
	return rc;
}


static void nand_sync(struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;

	this->state = FL_READY;
}


static int nand_suspend(struct mtd_info *mtd)
{
	bEnterSuspend = 1;

	return 0;
}


static void nand_resume(struct mtd_info *mtd)
{
	bEnterSuspend = 0;
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


static int scan_last_die_BB(struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	__u32 start_page = 0, start_block = 0;
	uint32_t addr;
	int block_num = this->block_num;
	int block_size = 1 << this->phys_erase_shift;
	int i, j;
	int numchips = this->numchips;
	uint32_t chip_size = this->chipsize;
	int rc = 0;
	int bb = 0;
	__u8 *block_status = NULL;

	NF_ERR_PRINT("scan_last_die_BB()\n");

	start_page = 0x00000000;

	this->active_chip = numchips-1;
	this->select_chip(mtd, numchips-1);

	block_status = kmalloc(block_num, GFP_KERNEL);
	if (!block_status) {
		NF_ERR_PRINT("%s: Error, no enough memory for block_status\n", __func__);
		rc = -ENOMEM;
		goto EXIT;
	}
	memset((__u32 *)block_status, 0, block_num);

	start_block = (start_page >> this->page_shift) >> (this->phys_erase_shift - this->page_shift);
	for (addr = start_page; addr < chip_size; addr += block_size) {
		if (rtk_block_isbad(mtd, numchips-1, addr)) {
			bb = addr >> this->phys_erase_shift;
			block_status[bb] = 0xff;
		}
	}

	for (i = 0 ; i < RBA; i++) {
		this->bbt[i].bad_block = BB_INIT;
		this->bbt[i].BB_die = BB_DIE_INIT;
		this->bbt[i].remap_block = (block_num-1) - i;
		this->bbt[i].RB_die = BB_DIE_INIT;
	}

	j = 0;
	for (i = start_block; i < block_num; i++) {
		if (block_status[i] == 0xff) {
			this->bbt[j].BB_die      = numchips-1;
			this->bbt[j].bad_block   = i;
			this->bbt[j].RB_die      = numchips-1;
			j++;
		}
	}

	RTK_FLUSH_CACHE((unsigned long) this->bbt, sizeof(BB_t)*RBA);

EXIT:
	kfree(block_status);

	return 0;
}

static int rtk_create_bbt(struct mtd_info *mtd, int page)
{
	int rc = 0;

	if (page == 64) {
		NF_INFO_PRINT("[%s] nand driver creates B1 !!\n", __func__);
	} else if (page == 128) {
		NF_INFO_PRINT("[%s] nand driver creates B2 !!\n", __func__);
	} else {
		NF_ERR_PRINT("[%s] abort creating BB on page %x !!\n", __func__, page);
		return -1;
	}

	if (scan_last_die_BB(mtd)) {
		NF_ERR_PRINT("[%s] scan_last_die_BB() error !!\n", __func__);
		return -1;
	}

	dump_BBT(mtd);

	if (rtk_update_bbt_to_flash(mtd)) {
		NF_ERR_PRINT("[%s] rtk_update_bbt_to_flash() fails.\n", __func__);
		return -1;
	}
	NF_INFO_PRINT("[%s] rtk_update_bbt() successfully.\n", __func__);

	return rc;
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


	if (crc_value == hash_value) {
		*value = hash_value;
		return 0;
	} else {
		*value = 0;
		return -1;
	}
}

static int rtk_nand_read_bbt(struct mtd_info *mtd, int bbt_page, u8 *temp_BBT, unsigned char *buf_phy)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int rc = 0;
	__u8 isbbt = 0;
	u_char *new_buf = nrBuffer;

	rc = this->read_ecc_page(mtd, this->active_chip, bbt_page, new_buf, this->g_oobbuf, CP_NF_NONE, (dma_addr_t *)&buf_phy[0]);
	if (rc == 0 || rc == 1 || rc == -2) {
		isbbt = this->g_oobbuf[0];

		if (isbbt == BBT_TAG) {
			memcpy(temp_BBT, new_buf, page_size);
			return 0;

		} else {
			NF_ERR_PRINT("rtk_nand_read_bbt Get bbt TAG FAIL.\n");
			return -1;
		}
	} else {
		NF_ERR_PRINT("rtk_nand_read_bbt read block[%d][%d] FAIL.\n", bbt_page = 64 ? 1 : 2, bbt_page);
		return -2;
	}
}

static int rtk_write_bbt(struct mtd_info *mtd, int page)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int rc = 0;
	u8 *temp_BBT = 0;
	int i;
	unsigned int crc_value = 0;

	if (page == 64) {
		printk("[%s] write block1 !!\n", __func__);
	} else if (page == 128) {
		printk("[%s] write block2 !!\n", __func__);
	}

	temp_BBT = nwBuffer;
	if (!temp_BBT) {
		printk("%s: Error, no memory for temp_BBT\n", __func__);
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
	if (this->write_ecc_page(g_rtk_mtd, 0, page, temp_BBT, this->g_oobbuf, (dma_addr_t *)nwPhys_addr)) {
		printk("[%s] write BBT B%d page %d failure!!\n", __func__, page == 0?0:1, page);
		rc =  -1;
	}

EXIT:
	return rc;
}

static int rtk_nand_bbt_sync(struct mtd_info *mtd, int block)
{
	return rtk_write_bbt(mtd, block*ppb);
}

static void rtk_nand_disable_cpu(void)
{
	REG_WRITE_U32(PLL_SCPU1, 0x0);
}

static int rtk_nand_scan_bbt(struct mtd_info *mtd, unsigned char *buf_phy)
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
	u_char *new_buf = nrBuffer;
	size_t retlen;

	RTK_FLUSH_CACHE((unsigned long) this->bbt, sizeof(BB_t)*RBA);

	temp_BBT1 = kmalloc(page_size, GFP_KERNEL);
	if (!temp_BBT1) {
		printk(KERN_ERR "%s: Error, no enough memory for temp_BBT1\n", __func__);
		return -ENOMEM;
	}
	memset(temp_BBT1, 0xff, page_size);

	temp_BBT2 = kmalloc(page_size, GFP_KERNEL);
	if (!temp_BBT2) {
		printk(KERN_ERR "%s: Error, no enough memory for temp_BBT2\n", __func__);
		kfree(temp_BBT1);
		return -ENOMEM;
	}
	memset(temp_BBT2, 0xff, page_size);
	rc = rtk_nand_read_bbt(mtd, 64, temp_BBT1, buf_phy);
	if (rc < 0) {
		NF_INFO_PRINT("Read bbt1 FAIL\n");
		if (rtk_nand_read_bbt(mtd, 128, temp_BBT2, buf_phy) < 0) {
			NF_ERR_PRINT("Read bbt2 FAIL, system suspend.\n");
			rtk_nand_disable_cpu();
		} else {
			if (rtk_nand_bbt_crc_check(temp_BBT2, &crc2, &count2) < 0) {
				NF_ERR_PRINT("[%s] bbt2 crc check fail , system suspend !!\n", __func__);
				rtk_nand_disable_cpu();
			} else {
				NF_INFO_PRINT("[%s] bbt2 crc check OK , load bbt from BBT2.\n", __func__);
				memcpy(this->bbt, temp_BBT2 + 4, sizeof(BB_t)*RBA);
				this->bbt_crc = crc2;
				if (rc == -1)
					sync_need = 1;
			}
		}
	} else {
		NF_INFO_PRINT("Read bbt1 OK\n");

		if (rtk_nand_bbt_crc_check(temp_BBT1, &crc1, &count1) < 0) {
			NF_INFO_PRINT("[%s] bbt1 crc check fail\n", __func__);
			rc = rtk_nand_read_bbt(mtd, 128, temp_BBT2, buf_phy);
			if (rc < 0) {
				NF_ERR_PRINT("[%s] read bbt2 fail. system suspend\n", __func__);
				rtk_nand_disable_cpu();
			} else {
				NF_INFO_PRINT("Read bbt2 OK\n");
				if (rtk_nand_bbt_crc_check(temp_BBT2, &crc2, &count2) < 0) {
					NF_ERR_PRINT("[%s] bbt2 crc check fail. system suspend\n", __func__);
					rtk_nand_disable_cpu();
				} else {
					NF_INFO_PRINT("[%s] bbt2 crc check OK , load bbt from BBT2.\n", __func__);
					memcpy(this->bbt, temp_BBT2 + 4, sizeof(BB_t)*RBA);
					this->bbt_crc = crc2;
					sync_need = 1;
				}
			}
		} else {
			NF_INFO_PRINT("[%s] bbt1 crc check ok\n", __func__);
			rc = rtk_nand_read_bbt(mtd, 128, temp_BBT2, buf_phy);
			if (rc < 0) {
				NF_ERR_PRINT("[%s] read bbt2 fail. load bbt from BBT1\n", __func__);
				memcpy(this->bbt, temp_BBT1 + 4, sizeof(BB_t)*RBA);
				this->bbt_crc = crc1;
				if (rc == -1)
					sync_need = 2;
			} else {
				NF_INFO_PRINT("Read bbt2 OK\n");
				if (rtk_nand_bbt_crc_check(temp_BBT2, &crc2, &count2) < 0) {
					NF_INFO_PRINT("[%s] bbt2 crc check fail. load bbt from BBT1\n", __func__);
					memcpy(this->bbt, temp_BBT1 + 4, sizeof(BB_t)*RBA);
					this->bbt_crc = crc1;
					sync_need = 2;
				} else {
					NF_INFO_PRINT("[%s] bbt2 crc check OK , compare count of bbt.\n", __func__);

					if (count1 == count2) {
						NF_INFO_PRINT("[%s] count1 == count2, load bbt from BBT1\n", __func__);
						memcpy(this->bbt, temp_BBT1 + 4, sizeof(BB_t)*RBA);
						this->bbt_crc = crc1;
					} else {
						if (count1 > count2) {
							NF_INFO_PRINT("[%s] count1 > count2, load bbt from BBT1\n", __func__);
							memcpy(this->bbt, temp_BBT1 + 4, sizeof(BB_t)*RBA);
							this->bbt_crc = crc1;
							sync_need = 2;
						} else {
							NF_INFO_PRINT("[%s] count1 < count2, load bbt from BBT2\n", __func__);
							memcpy(this->bbt, temp_BBT2 + 4, sizeof(BB_t)*RBA);
							this->bbt_crc = crc2;
							sync_need = 1;
						}
					}
				}
			}
		}
	}

	if (sync_need) {
		NF_INFO_PRINT("[%s] sync bbt1 and bbt2.\n", __func__);
		rtk_nand_bbt_sync(mtd, sync_need);
	}

	dump_BBT(mtd);

	kfree(temp_BBT1);

	kfree(temp_BBT2);


	memset(&fw_info1, 0x0, sizeof(RTK_VER_INFO_T));
	memset(&fw_info2, 0x0, sizeof(RTK_VER_INFO_T));

	rc = nand_read_ecc(mtd, ppb*page_size*4, page_size, &retlen, new_buf, NULL, NULL, (unsigned char *)nrPhys_addr);
	if (rc < 0) {
		NF_ERR_PRINT("Read fw1 version fail.\n");
	} else {
		memcpy(&fw_info1, new_buf, sizeof(RTK_VER_INFO_T));
	}

	rc = nand_read_ecc(mtd, ppb*page_size*5, page_size, &retlen, new_buf, NULL, NULL, (unsigned char *)nrPhys_addr);
	if (rc < 0) {
		NF_ERR_PRINT("Read fw2 version fail.\n");
	} else {
		memcpy(&fw_info2, new_buf, sizeof(RTK_VER_INFO_T));
	}

	return rc;
}

static int nand_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	return 0;
}

void rtk_get_upgrade_info(RTK_VER_INFO_T *info, int fw)
{
	if (fw == 1)
		memcpy(info, &fw_info1, sizeof(RTK_VER_INFO_T));
	else if (fw == 2)
		memcpy(info, &fw_info2, sizeof(RTK_VER_INFO_T));

	return;
}

int rtk_set_upgrade_info(RTK_VER_INFO_T *info, int fw)
{
	struct nand_chip *this = (struct nand_chip *)g_rtk_mtd->priv;
	char *new_buf;
	unsigned int page_num = 0;
	int rc = 0;
	unsigned int ogl_block;

	mutex_lock(&rtk_nand_mutex);

	new_buf = nwBuffer;
rtk_set_upgrade_info_redo:
	memset(new_buf, 0x0, page_size);
	memcpy(new_buf, info, sizeof(RTK_VER_INFO_T));

	if (fw == 1)
		ogl_block = 4;
	else if (fw == 2)
		ogl_block = 5;

	page_num = rtk_find_real_blk(g_rtk_mtd, ogl_block) * ppb;

	rc = this->erase_block(g_rtk_mtd, 0, page_num);
	if (rc == -1) {
		NF_ERR_PRINT("[%s]error: erase block %u failure\n", __func__, page_num);
		rc = rtk_badblock_handle(g_rtk_mtd, page_num, ogl_block, 0, BACKUP_ERASE);
		if (rc < 0) {
			NF_ERR_PRINT("[%s]error: erase block %u failure\n", __func__, page_num/ppb);
			goto rtk_set_upgrade_info_end;
		} else {
			goto rtk_set_upgrade_info_redo;
		}
	} else if (rc == -2) {
		NF_ERR_PRINT("[%s]error: erase page %u align failure\n", __func__, page_num);
		goto rtk_set_upgrade_info_end;
	}


	if (this->write_ecc_page(g_rtk_mtd, 0, page_num, new_buf, this->g_oobbuf, (dma_addr_t *)nwPhys_addr)) {
		NF_ERR_PRINT("[%s]update  fw info block %u failure\n", __func__, page_num);
		rc = rtk_badblock_handle(g_rtk_mtd, page_num, ogl_block, 0, BACKUP_WRITE);
		if (rc < 0) {
			NF_ERR_PRINT("[%s]error: write page %u failure\n", __func__, page_num);
			goto rtk_set_upgrade_info_end;
		} else {
			goto rtk_set_upgrade_info_redo;
		}
	} else {
		if (fw == 1)
			memcpy(&fw_info1, info, sizeof(RTK_VER_INFO_T));
		else if (fw == 2)
			memcpy(&fw_info2, info, sizeof(RTK_VER_INFO_T));
	}

rtk_set_upgrade_info_end:
	mutex_unlock(&rtk_nand_mutex);

	return rc;
}

static void rtd_set_nand_info(char *item)
{
	int ret = 0;
	char *s = NULL;
	unsigned int temp = 0;
	s = strstr(item, "ds:");
	if (s != NULL) {
		ret = kstrtouint(s+3, 10, &g_nand_device.size);
		g_nand_device.chipsize = g_nand_device.size;
		return;
	}
	s = strstr(item, "ps:");
	if (s != NULL) {
		ret = kstrtouint(s+3, 10, &g_nand_device.PageSize);
		return;
	}
	s = strstr(item, "bs:");
	if (s != NULL) {
		ret = kstrtouint(s+3, 10, &g_nand_device.BlockSize);
		return;
	}
	s = strstr(item, "t1:");
	if (s != NULL) {
		ret = kstrtouint(s+3, 10, &temp);
		g_nand_device.T1 = (unsigned char)temp;
		return;
	}
	s = strstr(item, "t2:");
	if (s != NULL) {
		ret = kstrtouint(s+3, 10, &temp);
		g_nand_device.T2 = (unsigned char)temp;
		return;
	}
	s = strstr(item, "t3:");
	if (s != NULL) {
		ret = kstrtouint(s+3, 10, &temp);
		g_nand_device.T3 = (unsigned char)temp;
		return;
	}
	s = strstr(item, "eb:");
	if (s != NULL) {
		ret = kstrtouint(s+3, 10, &temp);
		g_nand_device.eccSelect = (unsigned short)temp;
		return;
	}
	s = strstr(item, "bf:");
	if (s != NULL) {
		ret = kstrtouint(s+3, 10, &temp);
		g_boot_flag = (unsigned short)temp;
		return;
	}
}

static int rtd_get_set_nand_info(void)
{
	const char * const delim = ",";
	char *sepstr = g_rtk_nandinfo_line;
	char *substr = NULL;

	if (strlen(g_rtk_nandinfo_line) == 0) {
		NF_INIT_PRINT("No nand info got from lk!!!!\n");
		return -1;
	}

	memset(&g_nand_device, 0x0, sizeof(device_type_t));

	NF_INIT_PRINT("g_rtk_nandinfo_line:[%s]\n", sepstr);

	substr = strsep(&sepstr, delim);

	do {
		rtd_set_nand_info(substr);

		substr = strsep(&sepstr, delim);
	} while (substr);

	g_nand_device.num_chips = 1;
	g_nand_device.isLastPage = 0;

	if (g_nand_device.eccSelect == 0x1)
		g_nand_device.OobSize = 128;
	else
		g_nand_device.OobSize = 64;

	NF_INFO_PRINT("total flash size:[%u]\n", g_nand_device.size);
	NF_INFO_PRINT("chip size:[%u]\n", g_nand_device.chipsize);
	NF_INFO_PRINT("page size:[%d]\n", g_nand_device.PageSize);
	NF_INFO_PRINT("block size:[%d]\n", g_nand_device.BlockSize);
	NF_INFO_PRINT("oob size:[%d]\n", g_nand_device.OobSize);
	NF_INFO_PRINT("t1:[%d]\n", g_nand_device.T1);
	NF_INFO_PRINT("t2:[%d]\n", g_nand_device.T2);
	NF_INFO_PRINT("t3:[%d]\n", g_nand_device.T3);
	NF_INFO_PRINT("ecc select:[%d]\n", g_nand_device.eccSelect);

	return 1;
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
	uint32_t chip_size = 0;
	unsigned int num_chips_probe = 1;
	uint64_t result = 0;
	uint64_t div_res = 0;
	unsigned int flag_size, mempage_order;

	g_rtk_mtd = mtd;

	if (!this->select_chip)
		this->select_chip = nand_select_chip;
	if (!this->scan_bbt)
		this->scan_bbt = rtk_nand_scan_bbt;

	this->active_chip = 0;
	this->select_chip(mtd, 0);

	NF_INFO_PRINT("nand_base_rtk version:0815-2018\n");

	mtd->name = "rtk_nand";

	if (rtd_get_set_nand_info() > 0) {
		NF_INFO_PRINT("Get nand info from bootcode successfully.\n");

		REG_WRITE_U32(REG_TIME_PARA1, g_nand_device.T1);
		REG_WRITE_U32(REG_TIME_PARA2, g_nand_device.T2);
		REG_WRITE_U32(REG_TIME_PARA3, g_nand_device.T3);

		device_size = g_nand_device.size;
		chip_size   = g_nand_device.chipsize;
		page_size   = g_nand_device.PageSize;
		block_size  = g_nand_device.BlockSize;
		oob_size    = g_nand_device.OobSize;
		num_chips   = g_nand_device.num_chips;
		isLastPage  = g_nand_device.isLastPage;

		REG_WRITE_U32(REG_TIME_PARA1, g_nand_device.T1);
		REG_WRITE_U32(REG_TIME_PARA2, g_nand_device.T2);
		REG_WRITE_U32(REG_TIME_PARA3, g_nand_device.T3);

		this->page_shift = __ffs(page_size);
		this->page_idx_mask = ~((1L << this->page_shift) - 1);
		this->phys_erase_shift = __ffs(block_size);
		this->oob_shift = __ffs(oob_size);
		ppb = this->ppb = block_size >> this->page_shift;

		if (chip_size) {
			this->block_num = chip_size >> this->phys_erase_shift;
			this->page_num = chip_size >> this->page_shift;
			this->chipsize = chip_size;
			this->device_size = device_size;
			this->chip_shift =  __ffs(this->chipsize);
		}

		this->pagemask = (this->chipsize >> this->page_shift) - 1;

		mtd->oobsize = this->oob_size = oob_size;

		mtd->writesize = page_size;
		mtd->erasesize = block_size;
		mtd->writebufsize = page_size;

		mtd->erasesize_shift = this->phys_erase_shift;
		mtd->writesize_shift = this->page_shift;

		this->ecc_select = g_nand_device.eccSelect;
	} else {
		while (1) {
			this->read_id(mtd, id);

			this->maker_code = maker_code = id[0];
			this->device_code = device_code = id[1];
			nand_type_id = maker_code<<24 | device_code<<16 | id[2]<<8 | id[3];
			B5th = id[4];
			B6th = id[5];

			NF_INFO_PRINT("READ ID:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", id[0], id[1], id[2], id[3], id[4], id[5]);

			NF_INFO_PRINT("NAND Flash Controller detects %d dies\n", num_chips_probe);

			for (i = 0; nand_device[i].name; i++) {

				if (nand_device[i].id == nand_type_id &&
						((nand_device[i].CycleID5th == 0xff)?1:(nand_device[i].CycleID5th == B5th)) &&
						((nand_device[i].CycleID6th == 0xff)?1:(nand_device[i].CycleID6th == B6th))) {
					REG_WRITE_U32(REG_TIME_PARA1, nand_device[i].T1);
					REG_WRITE_U32(REG_TIME_PARA2, nand_device[i].T2);
					REG_WRITE_U32(REG_TIME_PARA3, nand_device[i].T3);
					if (nand_type_id != HY27UT084G2M) {
						REG_WRITE_U32(REG_MULTI_CHNL_MODE, 0x20);
					}
					if (nand_device[i].size == num_chips_probe * nand_device[i].chipsize) {
						if (num_chips_probe == nand_device[i].num_chips) {
							NF_INFO_PRINT("One %s chip has %d die(s) on board\n",
									nand_device[i].name, nand_device[i].num_chips);
							device_size = nand_device[i].size;
							chip_size = nand_device[i].chipsize;
							page_size = nand_device[i].PageSize;
							block_size = nand_device[i].BlockSize;
							oob_size = nand_device[i].OobSize;
							num_chips = nand_device[i].num_chips;
							isLastPage = nand_device[i].isLastPage;
							rtk_lookup_table_flag = 1;
							REG_WRITE_U32(REG_TIME_PARA1, nand_device[i].T1);
							REG_WRITE_U32(REG_TIME_PARA2, nand_device[i].T2);
							REG_WRITE_U32(REG_TIME_PARA3, nand_device[i].T3);
							NF_INFO_PRINT("index(%d) nand part=%s, id=0x%x, device_size=%ui, page_size=0x%x, block_size=0x%x, oob_size=0x%x, "
									"num_chips=0x%x, isLastPage=0x%x, ecc_sel=0x%x\n",
									i, nand_device[i].name, nand_device[i].id,
									nand_device[i].size, page_size,	block_size,
									oob_size, num_chips, isLastPage,
									nand_device[i].eccSelect);
							break;
						}
					} else {
						if (!strcmp(nand_device[i].name, "HY27UF084G2M")) {
							continue;
						} else {
							NF_INFO_PRINT("We have %d the same %s chips on board\n",
									num_chips_probe/nand_device[i].num_chips,
									 nand_device[i].name);
							device_size = nand_device[i].size;
							chip_size = nand_device[i].chipsize;
							page_size = nand_device[i].PageSize;
							block_size = nand_device[i].BlockSize;
							oob_size = nand_device[i].OobSize;
							num_chips = nand_device[i].num_chips;
							isLastPage = nand_device[i].isLastPage;
							rtk_lookup_table_flag = 1;
							NF_INFO_PRINT("nand part=%s, id=%x, device_size=%u, chip_size=%u, num_chips=%d, isLastPage=%d, eccBits=%d\n",
									nand_device[i].name, nand_device[i].id,
									nand_device[i].size, nand_device[i].chipsize,
									nand_device[i].num_chips, nand_device[i].isLastPage,
									 nand_device[i].eccSelect);
							break;
						}
					}
				}
			}

			if (!rtk_lookup_table_flag) {
				NF_ERR_PRINT("Warning: Lookup Table do not have this nand flash !!\n");
				NF_ERR_PRINT("%s: Manufacture ID=0x%02x, Chip ID=0x%02x, 3thID=0x%02x, 4thID=0x%02x, 5thID=0x%02x, 6thID=0x%02x\n",
						mtd->name, id[0], id[1], id[2], id[3], id[4], id[5]);
				return -1;
			}

			this->page_shift = __ffs(page_size);
			this->page_idx_mask = ~((1L << this->page_shift) - 1);
			this->phys_erase_shift = __ffs(block_size);
			this->oob_shift = __ffs(oob_size);
			ppb = this->ppb = block_size >> this->page_shift;

			if (chip_size) {
				this->block_num = chip_size >> this->phys_erase_shift;
				this->page_num = chip_size >> this->page_shift;
				this->chipsize = chip_size;
				this->device_size = device_size;
				this->chip_shift =  __ffs(this->chipsize);
			}

			this->pagemask = (this->chipsize >> this->page_shift) - 1;

			mtd->oobsize = this->oob_size = oob_size;

			mtd->writesize = page_size;
			mtd->erasesize = block_size;
			mtd->writebufsize = page_size;

			mtd->erasesize_shift = this->phys_erase_shift;
			mtd->writesize_shift = this->page_shift;

			this->ecc_select = nand_device[i].eccSelect;

			break;
		}
	}

	switch (this->ecc_select) {
	case 0x0:
		g_eccbits = 6;
		break;
	case 0x1:
		g_eccbits = 12;
		break;
	case 0xe:
		g_eccbits = 16;
		break;
	case 0xa:
		g_eccbits = 24;
		break;
	case 0x2:
		g_eccbits = 40;
		break;
	case 0x4:
		g_eccbits = 43;
		break;
	case 0x6:
		g_eccbits = 65;
		break;
	case 0x8:
		g_eccbits = 72;
		break;
	default:
		g_eccbits = 6;
		break;
	}

	if (g_eccbits >= 0x18)
		mtd->ecc_strength = 1024;
	else
		mtd->ecc_strength = 512;

	this->select_chip(mtd, 0);

	if (num_chips != num_chips_probe)
		this->numchips = num_chips_probe;
	else
		this->numchips = num_chips;
	div_res = mtd->size = this->numchips * this->chipsize;
	do_div(div_res, block_size);
	result = div_res;
	result *= this->RBA_PERCENT;
	do_div(result, 100);
	RBA = this->RBA = result;

	RBA = this->RBA = (((unsigned int)mtd->size/block_size) / 100) * this->RBA_PERCENT;
	printk("RBA:[%u]\n", RBA);

	mtd->dev.coherent_dma_mask = 0xffffffff;

	this->bbt = kmalloc(sizeof(BB_t)*RBA, GFP_KERNEL);

	if (!this->bbt) {
		NF_ERR_PRINT("%s: Error, no enough memory for BBT\n", __func__);
		return -1;
	}
	memset(this->bbt, 0,  sizeof(BB_t)*RBA);

	nrBuffer = (unsigned char *)dma_alloc_coherent(&mtd->dev, MAX_NR_LENGTH, (dma_addr_t *)
						(&nrPhys_addr), GFP_DMA | GFP_KERNEL);
	if (!nrBuffer) {
		NF_ERR_PRINT("%s: Error, no enough memory for nrBuffer\n", __func__);
		return -ENOMEM;
	}
	memset(nrBuffer, 0xff, MAX_NR_LENGTH);


	nwBuffer = (unsigned char *)dma_alloc_coherent(&mtd->dev, MAX_NW_LENGTH, (dma_addr_t *)
						 (&nwPhys_addr), GFP_DMA | GFP_KERNEL);
	if (!nwBuffer) {
		NF_ERR_PRINT("%s: Error, no enough memory for nwBuffer\n", __func__);
		return -ENOMEM;
	}
	memset(nwBuffer, 0xff, MAX_NW_LENGTH);

	this->g_oobbuf = (unsigned char  *)dma_alloc_coherent(&mtd->dev, oob_size*256,
						 (dma_addr_t *) (&oobPhys_addr), GFP_KERNEL | GFP_DMA);
	if (!this->g_oobbuf) {
		NF_ERR_PRINT("%s: Error, no enough memory for g_oobbuf\n", __func__);
		return -ENOMEM;
	}
	memset(this->g_oobbuf, 0xff, oob_size);

	tempBuffer = (unsigned char  *)dma_alloc_coherent(&mtd->dev, 2048*4,
						 (dma_addr_t *) (&tempPhys_addr), GFP_KERNEL | GFP_DMA);
	if (!tempBuffer) {
		NF_ERR_PRINT("%s: Error, no enough memory for tempBuffer\n", __func__);
		return -ENOMEM;
	}
	memset(tempBuffer, 0xff, 2048*4);

	flag_size =  (this->numchips * this->page_num) >> 3;
	mempage_order = get_order(flag_size);

	mtd->type		= MTD_NANDFLASH;
	mtd->flags		= MTD_CAP_NANDFLASH;
	mtd->_erase		= nand_erase;
	mtd->_point		= NULL;
	mtd->_unpoint		= NULL;
	mtd->_read		= nand_read;
	mtd->_write		= nand_write;
	mtd->_read_oob		= nand_read_oob;
	mtd->_write_oob		= nand_write_oob;
	mtd->_sync		= nand_sync;
	mtd->_lock		= NULL;
	mtd->_unlock		= NULL;
	mtd->_suspend		= nand_suspend;
	mtd->_resume		= nand_resume;
	mtd->owner		= THIS_MODULE;
	mtd->_block_isbad	= nand_block_isbad;
	mtd->_block_markbad	= NULL;
	mtd->_panic_write	= NULL;
	mtd->owner = THIS_MODULE;

	bEnterSuspend = 0;
	crc_table_computed = 0;
	return this->scan_bbt(mtd, (unsigned char *)nrPhys_addr);
}
