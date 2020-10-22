/*
 * rtk_nand.c - nand driver
 *
 * Copyright (c) 2017 Realtek Semiconductor Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/pm.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/mtd/rtk_nand_reg.h>
#include <linux/mtd/rtk_nand.h>
#include <linux/bitops.h>
#include <mtd/mtd-abi.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/delay.h>
#include "../mtdcore.h"

#define BANNER			"Realtek NAND Flash Driver"
#define VERSION			"$Id: rtk_nand.c 2018-10-29 01:32:27Z aaron.lin $"
#define MTDSIZE			(sizeof(struct mtd_info) + sizeof(struct nand_chip))

#define RTKNAND_PROC_DIR_NAME           "rtknand"
#define RTKNAND_PROC_INFO_DIR_NAME      "rtknand/info"
#define RTKNAND_PROC_TEST_DIR_NAME	"rtknand/test"

extern struct proc_dir_entry proc_root;
static struct proc_dir_entry *rtknand_proc_dir;
static int g_id_chain;
extern unsigned int g_eccbits;
extern dma_addr_t oobPhys_addr;

char g_rtk_nandinfo_line[128];


const char *ptypes[] = {"cmdlinepart", NULL};

/* nand driver low-level functions */
static int rtk_read_ecc_page(struct mtd_info *mtd, u16 chipnr, unsigned int page,
			u_char *data, u_char *oob_buf, u16 cp_mode, dma_addr_t *data_phy);
static int rtk_write_oob(struct mtd_info *mtd, u16 chipnr, int page, int len, const u_char *buf);
static int rtk_write_ecc_page(struct mtd_info *mtd, u16 chipnr, unsigned int page, const u_char *data, const u_char *oob_buf, const dma_addr_t *data_phy);
static int rtk_erase_block(struct mtd_info *mtd, u16 chipnr, int page);
void rtk_nand_reset(void);
/* Global Variables */
struct mtd_info *rtk_mtd;

static int page_size, oob_size, ppb;
static int RBA_PERCENT = 5;

static struct rw_semaphore rw_sem;

struct rtk_nand_host {
	struct mtd_info		mtd;
	struct nand_chip	nand;
	struct mtd_partition	*parts;
	struct device		*dev;
	void    *spare0;
	void    *main_area0;
	void    *main_area1;
	void __iomem    *base;
	void __iomem    *regs;
	int	    status_request;
	struct clk  *clk;
	int	clk_act;
	int	irq;
	wait_queue_head_t	irq_waitq;
	uint8_t		*data_buf;
	unsigned int    buf_start;
	int		spare_len;
};

void syncPageRead(void)
{
	dmb(sy);
	dmb(sy);
}

int WAIT_DONE(void *addr, u64 mask, unsigned int value)
{
	int timeout = 0;

	while ((REG_READ_U32(addr) & mask) != value) {
		if (timeout++ > 500) {
			printk(KERN_ERR "WAIT_DONE timeout\n");
			return -WAIT_TIMEOUT;
		}
		usleep_range(50, 100);
	}

	return 0;
}

/**
 *	rtk_nand_reset -
 *
 *	initialize host controller;
 *	dont reset nand flash, it is dangerous
 *
 */
void rtk_nand_reset(void)
{
	/* controller init */
	REG_WRITE_U32(REG_PD, 0x1E);
	REG_WRITE_U32(REG_TIME_PARA3, 0x2);
	REG_WRITE_U32(REG_TIME_PARA2, 0x5);
	REG_WRITE_U32(REG_TIME_PARA1, 0x2);

	REG_WRITE_U32(REG_MULTI_CHNL_MODE, 0x0);
	REG_WRITE_U32(REG_READ_BY_PP, 0x0);


	REG_WRITE_U32(REG_NF_LOW_PWR, REG_READ_U32(REG_NF_LOW_PWR)&~0x10);
	REG_WRITE_U32(REG_SPR_DDR_CTL, NF_SPR_DDR_CTL_spare_ddr_ena(1) | NF_SPR_DDR_CTL_per_2k_spr_ena(1) | NF_SPR_DDR_CTL_spare_dram_sa(0));

}
EXPORT_SYMBOL(rtk_nand_reset);

static int rtk_nand_suspend(struct device *dev)
{
	struct clk *clk = clk_get(dev, NULL);

	clk_disable_unprepare(clk);

	return 0;
}

static int rtk_nand_resume(struct device *dev)
{
	struct clk *clk = clk_get(dev, NULL);
	int rc;

	clk_prepare_enable(clk);

	/* controller init. */
	REG_WRITE_U32(REG_PD, 0x1E);
	REG_WRITE_U32(REG_TIME_PARA3, 0x2);
	REG_WRITE_U32(REG_TIME_PARA2, 0x5);
	REG_WRITE_U32(REG_TIME_PARA1, 0x2);

	REG_WRITE_U32(REG_MULTI_CHNL_MODE, 0x0);
	REG_WRITE_U32(REG_READ_BY_PP, 0x0);

	/* reset nand. */
	REG_WRITE_U32(REG_ND_CMD, 0xff);
	REG_WRITE_U32(REG_ND_CTL, 0x80);

	rc = WAIT_DONE(REG_ND_CTL, 0x80, 0);
	rc = WAIT_DONE(REG_ND_CTL, 0x40, 0x40);

	REG_WRITE_U32(REG_NF_LOW_PWR, REG_READ_U32(REG_NF_LOW_PWR)&~0x10);

	REG_WRITE_U32(REG_SPR_DDR_CTL, NF_SPR_DDR_CTL_spare_ddr_ena(1) | NF_SPR_DDR_CTL_per_2k_spr_ena(1) | NF_SPR_DDR_CTL_spare_dram_sa(0));

	return 0;
}

static void rtk_read_oob_from_SRAM(struct mtd_info *mtd, __u8 *oobbuf)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	unsigned int reg_oob;
	void __iomem *reg = NULL;
	int i;
	char r_oobbuf[256];
	int oobuse_size;

	REG_WRITE_U32(REG_READ_BY_PP, 0x00);
	REG_WRITE_U32(REG_SRAM_CTL, 0x30 | 0x04);

	memset(r_oobbuf, 0xFF, 256);
	memset(oobbuf, 0xFF, mtd->oobsize);


	switch (this->ecc_select) {
	case 0x0:
		oobuse_size = 6 + 10;
		break;
	case 0x1:
		oobuse_size = 6 + 20;
		break;
	default:
		oobuse_size = 6 + 10;
		break;
	}

	for (i = 0; i < (mtd->oobsize/4); i++) {
		reg_oob = REG_READ_U32(REG_NF_BASE_ADDR+(i*4));

		r_oobbuf[i*4] = reg_oob & 0xff;
		r_oobbuf[(i*4)+1] = (reg_oob >> 8) & 0xff;
		r_oobbuf[(i*4)+2] = (reg_oob >> 16) & 0xff;
		r_oobbuf[(i*4)+3] = (reg_oob >> 24) & 0xff;
	}

	for (i = 0; i < 4; i++) {
		memcpy(oobbuf+(i*oobuse_size), r_oobbuf+(i*(mtd->oobsize/4)), oobuse_size);
	}

	REG_WRITE_U32(REG_SRAM_CTL, 0x00);
	REG_WRITE_U32(REG_READ_BY_PP, 0x80);

	return;
}

static void rtk_nand_read_id(struct mtd_info *mtd, u_char id[6])
{
	int id_chain;
	int rc;

	REG_WRITE_U32(REG_DATA_TL0, NF_DATA_TL0_length0(0x06));
	REG_WRITE_U32(REG_DATA_TL1, NF_DATA_TL1_access_mode(0x01));

	/* Set PP */
	REG_WRITE_U32(REG_READ_BY_PP, NF_READ_BY_PP_read_by_pp(0x0));

	REG_WRITE_U32(REG_PP_CTL0, NF_PP_CTL0_pp_enable(0x01));
	REG_WRITE_U32(REG_PP_CTL1, NF_PP_CTL1_pp_start_addr(0));

	/* Set command */
	REG_WRITE_U32(REG_ND_CMD, NF_ND_CMD_cmd(CMD_READ_ID));
	REG_WRITE_U32(REG_ND_CTL, NF_ND_CTL_xfer(0x01));
	rc = WAIT_DONE(REG_ND_CTL, 0x80, 0);

	/* Set address */
	REG_WRITE_U32(REG_ND_PA0, 0);
	REG_WRITE_U32(REG_ND_PA1, 0);
	REG_WRITE_U32(REG_ND_PA2, NF_ND_PA2_addr_mode(0x07));

	REG_WRITE_U32(REG_ND_CTL, NF_ND_CTL_xfer(1)|NF_ND_CTL_tran_mode(1));
	rc = WAIT_DONE(REG_ND_CTL, 0x80, 0);
	/* Enable XFER mode */
	REG_WRITE_U32(REG_ND_CTL, NF_ND_CTL_xfer(1)|NF_ND_CTL_tran_mode(4));
	rc = WAIT_DONE(REG_ND_CTL, 0x80, 0);

	/* Reset PP */
	REG_WRITE_U32(REG_PP_CTL0, NF_PP_CTL0_pp_reset(1));

	/* Move data to DRAM from SRAM */
	REG_WRITE_U32(REG_SRAM_CTL, NF_SRAM_CTL_map_sel(1)|NF_SRAM_CTL_access_en(1)|NF_SRAM_CTL_mem_region(0));

	id_chain = REG_READ_U32(REG_ND_PA0);
	g_id_chain = id_chain;
	NF_DEBUG_PRINT("id_chain 1 = 0x%x\n", id_chain);
	id[0] = id_chain & 0xff;
	id[1] = (id_chain >> 8) & 0xff;
	id[2] = (id_chain >> 16) & 0xff;
	id[3] = (id_chain >> 24) & 0xff;

	id_chain = REG_READ_U32(REG_ND_PA1);
	NF_DEBUG_PRINT("id_chain 2 = 0x%x\n", id_chain);
	id[4] = id_chain & 0xff;
	id[5] = (id_chain >> 8) & 0xff;

	REG_WRITE_U32(REG_SRAM_CTL, 0x0);
}

/**
 * rtk_erase_block -
 *
 * Rerurn 0 on success
 *       -1 on erase error
 *       -2 on align error
 */
static int rtk_erase_block(struct mtd_info *mtd, u16 chipnr, int page)
{
	struct nand_chip *this = NULL;
	unsigned int chip_section = 0;
	unsigned int section = 0;

	page_size = mtd->writesize;
	oob_size = mtd->oobsize;
	ppb = mtd->erasesize/mtd->writesize;
	this = (struct nand_chip *) mtd->priv;

	down_write(&rw_sem);

	if (page & (ppb-1)) {
		NF_ERR_PRINT("%s: page %d is not block alignment !!\n", __func__, page);
		up_write(&rw_sem);
		return -2;
	}

	REG_WRITE_U32(REG_MULTI_CHNL_MODE, NF_MULTI_CHNL_MODE_no_wait_busy(1)|NF_MULTI_CHNL_MODE_edo(1));

	REG_WRITE_U32(REG_ND_CMD, NF_ND_CMD_cmd(CMD_BLK_ERASE_C1));
	REG_WRITE_U32(REG_CMD2, NF_CMD2_cmd2(CMD_BLK_ERASE_C2));
	REG_WRITE_U32(REG_CMD3, NF_CMD3_cmd3(CMD_BLK_ERASE_C3));

	REG_WRITE_U32(REG_ND_PA0, NF_ND_PA0_page_addr0(page));
	REG_WRITE_U32(REG_ND_PA1, NF_ND_PA1_page_addr1(page>>8));
	REG_WRITE_U32(REG_ND_PA2, NF_ND_PA2_addr_mode(0x04)|NF_ND_PA2_page_addr2(page>>16));
	REG_WRITE_U32(REG_ND_PA3, NF_ND_PA3_page_addr3((page >> 21)&0x7));

	REG_WRITE_U32(REG_AUTO_TRIG, NF_AUTO_TRIG_auto_trig(1)|NF_AUTO_TRIG_spec_auto_case(1)|NF_AUTO_TRIG_auto_case(2));
	if (WAIT_DONE(REG_AUTO_TRIG, 0x80, 0)) {
		NF_ERR_PRINT("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		up_write(&rw_sem);
		return -WAIT_TIMEOUT;
	}

	REG_WRITE_U32(REG_POLL_FSTS, NF_POLL_FSTS_bit_sel(6)|NF_POLL_FSTS_trig_poll(1));
	if (WAIT_DONE(REG_POLL_FSTS, 0x01, 0x0)) {
		NF_ERR_PRINT("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		up_write(&rw_sem);
		return -WAIT_TIMEOUT;
	}
	if (WAIT_DONE(REG_ND_CTL, 0x40, 0x40)) {
		NF_ERR_PRINT("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		up_write(&rw_sem);
		return -WAIT_TIMEOUT;
	}

	if (REG_READ_U32(REG_ND_DAT) & 0x01) {
		up_write(&rw_sem);
		NF_ERR_PRINT("[%s] erase is not completed at block %d\n", __func__, page/ppb);
		return -1;
	}

	up_write(&rw_sem);

	return 0;
}

static int rtk_read_ecc_page(struct mtd_info *mtd, u16 chipnr, unsigned int page, u_char *data_buf, u_char *oob_buf, u16 cp_mode, dma_addr_t *data_phy)
{
	struct nand_chip *this = NULL;
	int dram_sa, dma_len;
	int page_len;
	uint8_t	auto_trigger_mode = 2;
	uint8_t	addr_mode = 1;
	uint8_t	bChkAllOne = 0;
	volatile unsigned int data;
	unsigned int eccNum = 0;
	unsigned int blank_check = 0;
	unsigned int blank_confirm = 0;

	this = (struct nand_chip *) mtd->priv;
	page_size = mtd->writesize;
	oob_size = mtd->oobsize;
	ppb = mtd->erasesize/mtd->writesize;

	syncPageRead();

	down_write(&rw_sem);

	if (((unsigned int)data_buf&0x7) != 0) {
		NF_DEBUG_PRINT("[%s]data_buf must 8 byte alignmemt!!\n", __func__);
		BUG();
	}

blank_confirm_read:
	while (1) {

		if (this->ecc_select >= 0x18) {
			if (bChkAllOne) {
				/* enable randomizer */
				REG_WRITE_U32(REG_RMZ_CTRL, 0);
				REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(1));
			} else {
				/* enable randomizer */
				REG_WRITE_U32(REG_RMZ_CTRL, 1);
				REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));
			}

			page_len = page_size >> 10;
			REG_WRITE_U32(REG_DATA_TL0, NF_DATA_TL0_length0(1024));
			REG_WRITE_U32(REG_DATA_TL1, NF_DATA_TL1_access_mode(1)|NF_DATA_TL1_length1(4));
		} else {
			/* set random read */
			REG_WRITE_U32(REG_RND_EN, 1);
			REG_WRITE_U32(REG_RND_CMD1, 0x5);
			REG_WRITE_U32(REG_RND_CMD2, 0xe0);
			/* data start address MSB (always 0) */
			REG_WRITE_U32(REG_RND_DATA_STR_COL_H, 0);
			/* spare start address MSB */
			REG_WRITE_U32(REG_RND_SPR_STR_COL_H, page_size >> 8);
			/* spare start address LSB */
			REG_WRITE_U32(REG_RND_SPR_STR_COL_L, page_size & 0xff);

			page_len = page_size >> 9;
			REG_WRITE_U32(REG_DATA_TL0, NF_DATA_TL0_length0(512));
			REG_WRITE_U32(REG_DATA_TL1, NF_DATA_TL1_access_mode(1)|NF_DATA_TL1_length1(2));
		}
		REG_WRITE_U32(REG_PAGE_LEN, NF_PAGE_LEN_page_len(page_len));
		/* Set PP */
		REG_WRITE_U32(REG_READ_BY_PP, NF_READ_BY_PP_read_by_pp(1));
		REG_WRITE_U32(REG_PP_CTL1, NF_PP_CTL1_pp_start_addr(0));
		REG_WRITE_U32(REG_PP_CTL0, 0);

		/* enable blank check */
		if (blank_confirm)
			REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(1));
		else
			REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));


		if (cp_mode == CP_NF_AES_ECB_128 || cp_mode == CP_NF_AES_CBC_128) {
			REG_WRITE_U32(CP_NF_INI_KEY_0, 0x0);
			REG_WRITE_U32(CP_NF_INI_KEY_1, 0x0);
			REG_WRITE_U32(CP_NF_INI_KEY_2, 0x0);
			REG_WRITE_U32(CP_NF_INI_KEY_3, 0x0);

			REG_WRITE_U32(CP_NF_KEY_0, 0x8746bca3);
			REG_WRITE_U32(CP_NF_KEY_1, 0xcdef89ab);
			REG_WRITE_U32(CP_NF_KEY_2, 0x54369923);
			REG_WRITE_U32(CP_NF_KEY_3, 0xcdefbcab);

			/* set CP register. */
			if (cp_mode == CP_NF_AES_ECB_128)
				/* sel=0, cw_entry=0, bcm=1, aes_mode=0. Its ECB mode. */
				REG_WRITE_U32(CP_NF_SET, 0x200);
			else
				/* sel=0, cw_entry=0, bcm=0, aes_mode=0. Its CBC mode. */
				REG_WRITE_U32(CP_NF_SET, 0x0);

			/* integer multiple of dma_len. */
			REG_WRITE_U32(REG_CP_LEN, (page_size / 0x200) << 9);

		}

		/* Set command */
		REG_WRITE_U32(REG_ND_CMD, NF_ND_CMD_cmd(CMD_PG_READ_C1));
		REG_WRITE_U32(REG_CMD2, NF_CMD2_cmd2(CMD_PG_READ_C2));
		REG_WRITE_U32(REG_CMD3, NF_CMD3_cmd3(CMD_PG_READ_C3));

		/* Set address */
		REG_WRITE_U32(REG_ND_PA0, NF_ND_PA0_page_addr0(0xff&page));

		REG_WRITE_U32(REG_ND_PA1, NF_ND_PA1_page_addr1(0xff&(page>>8)));
		REG_WRITE_U32(REG_ND_PA2, NF_ND_PA2_addr_mode(addr_mode)|NF_ND_PA2_page_addr2(0x1f&(page>>16)));
		REG_WRITE_U32(REG_ND_PA3, NF_ND_PA3_page_addr3(0x7&(page>>21)));

		REG_WRITE_U32(REG_ND_CA0, 0);
		REG_WRITE_U32(REG_ND_CA1, 0);

		/* Set ECC */
		REG_WRITE_U32(REG_MULTI_CHNL_MODE, NF_MULTI_CHNL_MODE_edo(1));
		REG_WRITE_U32(REG_ECC_STOP, NF_ECC_STOP_ecc_n_stop(0x01));

		REG_WRITE_U32(REG_ECC_SEL, this->ecc_select);

		dram_sa = ((unsigned int)data_phy >> 3);
		REG_WRITE_U32(REG_DMA_CTL1, NF_DMA_CTL1_dram_sa(dram_sa));
		dma_len = page_size >> 9;
		REG_WRITE_U32(REG_DMA_CTL2, NF_DMA_CTL2_dma_len(dma_len));

		if (cp_mode == CP_NF_AES_ECB_128 || cp_mode == CP_NF_AES_CBC_128) {
			REG_WRITE_U32(REG_DMA_CTL3, NF_DMA_CTL3_cp_enable(1)|NF_DMA_CTL3_cp_first(1)|NF_DMA_CTL3_ddr_wr(1)|NF_DMA_CTL3_dma_xfer(1));
		} else {
			REG_WRITE_U32(REG_DMA_CTL3, NF_DMA_CTL3_ddr_wr(1)|NF_DMA_CTL3_dma_xfer(1));
		}

		smp_wmb();

		syncPageRead();

		/* Enable Auto mode */
		REG_WRITE_U32(REG_AUTO_TRIG, NF_AUTO_TRIG_auto_trig(1)|NF_AUTO_TRIG_spec_auto_case(0) | NF_AUTO_TRIG_auto_case(auto_trigger_mode));

		if (WAIT_DONE(REG_AUTO_TRIG, 0x80, 0)) {
			NF_ERR_PRINT("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
			up_write(&rw_sem);
			return -WAIT_TIMEOUT;
		}

		if (WAIT_DONE(REG_DMA_CTL3, 0x01, 0)) {
			NF_ERR_PRINT("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
			up_write(&rw_sem);
			return -WAIT_TIMEOUT;
		}


		if (oob_buf) {
			rtk_read_oob_from_SRAM(mtd, oob_buf);
		}

		if (blank_confirm) {
			blank_confirm = 0;

			if (REG_READ_U32(REG_ND_ECC) & 0x8) {
				printk("[DBG]ecc error... page=0x%x, REG_BLANK_CHK reg: 0x%x\n", page, REG_READ_U32(REG_BLANK_CHK));
				REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));
				up_write(&rw_sem);
				return -1;
			} else {
				if (oob_buf)
					memset(oob_buf, 0xFF, mtd->oobsize);

				REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));
				up_write(&rw_sem);
				return 1;
			}
		} else {

			/* return OK if all data bit is 1 (page is not written yet) */
			blank_check = REG_READ_U32(REG_BLANK_CHK);
			if (blank_check & 0x2) {
				REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));
				up_write(&rw_sem);
				return 1;
			} else if (REG_READ_U32(REG_ND_ECC) & 0x8) {
				blank_confirm = 1;
				NF_ERR_PRINT("[DBG]ecc error... goto blank_confirm_read   page=0x%x, REG_BLANK_CHK reg: 0x%x\n", page, REG_READ_U32(REG_BLANK_CHK));
				REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));
				goto blank_confirm_read;
			} else {
				if (REG_READ_U32(REG_ND_ECC) & 0x04) {
					eccNum = REG_READ_U32(REG_MAX_ECC_NUM) & 0xff;
					NF_DEBUG_PRINT("[NAND_DBG][%s]:%d, ecc_num:%d, page:%u\n", __func__, __LINE__, eccNum, page);
					if (eccNum > (g_eccbits - 2)) {
						NF_ERR_PRINT("[NAND_DBG][%s]:%d, ecc_num:%d, page:%u\n", __func__, __LINE__, eccNum, page);
						REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));
						up_write(&rw_sem);
						return -2;
					}
				}

				REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));
				up_write(&rw_sem);
				return 0;
			}

		}
	}
}

static int rtk_write_oob(struct mtd_info *mtd, u16 chipnr, int page, int len, const u_char *oob_buf)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int rc = 0;
	uint8_t auto_trigger_mode = 1;
	uint8_t addr_mode = 1;
	unsigned int page_len, dram_sa, dma_len, spare_dram_sa;
	unsigned char oob_1stB;
	unsigned int chip_section = 0;
	unsigned int section = 0;
	unsigned int index = 0;

	memset(this->g_databuf, 0xff, page_size);

	page_size = mtd->writesize;
	oob_size = mtd->oobsize;
	ppb = mtd->erasesize/mtd->writesize;

	down_write(&rw_sem);

	if (len > oob_size || !oob_buf) {
		NF_ERR_PRINT("[%s] error: len>oob_size OR oob_buf is NULL\n", __func__);
		up_write(&rw_sem);
		return -1;
	}

	REG_WRITE_U32(REG_SRAM_CTL, 0x00);
	REG_WRITE_U32(REG_DATA_TL0, NF_DATA_TL0_length0(0));
	if (this->ecc_select >= 0x18) {
		page_len = page_size >> 10;
		REG_WRITE_U32(REG_DATA_TL1, NF_DATA_TL1_length1(4));
	} else {
		page_len = page_size >> 9;
		REG_WRITE_U32(REG_DATA_TL1, NF_DATA_TL1_length1(2));
	}

	REG_WRITE_U32(REG_READ_BY_PP, NF_READ_BY_PP_read_by_pp(0));
	REG_WRITE_U32(REG_PP_CTL1, NF_PP_CTL1_pp_start_addr(0));
	REG_WRITE_U32(REG_PP_CTL0, 0);

	REG_WRITE_U32(REG_PAGE_LEN, NF_PAGE_LEN_page_len(page_len));

	REG_WRITE_U32(REG_ND_CMD, NF_ND_CMD_cmd(CMD_PG_WRITE_C1));
	REG_WRITE_U32(REG_CMD2, NF_CMD2_cmd2(CMD_PG_WRITE_C2));
	REG_WRITE_U32(REG_CMD3, NF_CMD3_cmd3(CMD_PG_WRITE_C3));

	REG_WRITE_U32(REG_ND_PA0, NF_ND_PA0_page_addr0(page));
	REG_WRITE_U32(REG_ND_PA1, NF_ND_PA1_page_addr1(page>>8));
	REG_WRITE_U32(REG_ND_PA2, NF_ND_PA2_addr_mode(addr_mode)|NF_ND_PA2_page_addr2(page>>16));
	REG_WRITE_U32(REG_ND_PA3, NF_ND_PA3_page_addr3((page>>21)&0x7));
	REG_WRITE_U32(REG_MULTI_CHNL_MODE, NF_MULTI_CHNL_MODE_edo(1));

	REG_WRITE_U32(REG_ECC_SEL, this->ecc_select);

	dram_sa = ((uintptr_t)this->g_databuf >> 3);
	REG_WRITE_U32(REG_DMA_CTL1, NF_DMA_CTL1_dram_sa(dram_sa));
	dma_len = page_size >> 9;
	REG_WRITE_U32(REG_DMA_CTL2, NF_DMA_CTL2_dma_len(dma_len));
	REG_WRITE_U32(REG_DMA_CTL3, NF_DMA_CTL3_ddr_wr(1)|NF_DMA_CTL3_dma_xfer(1));

	spare_dram_sa = ((uintptr_t)oob_buf >> 3);
	REG_WRITE_U32(REG_SPR_DDR_CTL, NF_SPR_DDR_CTL_spare_ddr_ena(1)|NF_SPR_DDR_CTL_per_2k_spr_ena(1)|NF_SPR_DDR_CTL_spare_dram_sa(spare_dram_sa));

	REG_WRITE_U32(REG_AUTO_TRIG, NF_AUTO_TRIG_auto_trig(1)|NF_AUTO_TRIG_spec_auto_case(0) | NF_AUTO_TRIG_auto_case(auto_trigger_mode));

	WAIT_DONE(REG_AUTO_TRIG, 0x80, 0);
	WAIT_DONE(REG_DMA_CTL3, 0x01, 0);
	REG_WRITE_U32(REG_POLL_FSTS, NF_POLL_FSTS_bit_sel(6)|NF_POLL_FSTS_trig_poll(1));
	WAIT_DONE(REG_POLL_FSTS, 0x01, 0x0);
	WAIT_DONE(REG_ND_CTL, 0x40, 0x40);

	if (REG_READ_U32(REG_ND_DAT) & 0x01) {
		NF_ERR_PRINT("[%s] write oob is not completed at page %d\n", __func__, page);
		up_write(&rw_sem);
		return -1;
	}

	printk(KERN_ERR "[%s] write oob OK at page %d\n", __func__, page);

	up_write(&rw_sem);

	return rc;
}

static int rtk_write_ecc_page(struct mtd_info *mtd, u16 chipnr, unsigned int page, const u_char *data_buf, const  u_char *oob_buf, const dma_addr_t *data_phy)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int rc = 0;
	uint8_t	auto_trigger_mode = 1;
	uint8_t	addr_mode = 1;
	unsigned int page_len, dram_sa, dma_len, spare_dram_sa;
	unsigned int chip_section = 0;
	unsigned int section = 0;
	unsigned int index = 0;

	syncPageRead();

	down_write(&rw_sem);

	if (((uintptr_t)data_buf&0x7) != 0) {
		NF_ERR_PRINT("[%s]data_buf must 8 byte alignmemt!!\n", __func__);
		BUG();
	}

	if (((uintptr_t)oob_buf&0x7) != 0) {
		NF_ERR_PRINT("[%s]oob_buf must 8 byte alignmemt!!\n", __func__);
		BUG();
	}


	REG_WRITE_U32(REG_DATA_TL0, NF_DATA_TL0_length0(0));
	if (this->ecc_select >= 0x18) {
		/* enable randomizer */
		REG_WRITE_U32(REG_RMZ_CTRL, 1);
		REG_WRITE_U32(REG_RMZ_SEED_H, ((page+1) & 0xff00)>>8);
		REG_WRITE_U32(REG_RMZ_SEED_L, (page+1) & 0xff);
		page_len = page_size >> 10;
		REG_WRITE_U32(REG_DATA_TL1, NF_DATA_TL1_length1(4));
	} else {
		/* set random write */
		REG_WRITE_U32(REG_RND_EN, 1);
		REG_WRITE_U32(REG_RND_CMD1, 0x85);
		/* data start address MSB (always 0) */
		REG_WRITE_U32(REG_RND_DATA_STR_COL_H, 0);
		/* spare start address MSB */
		REG_WRITE_U32(REG_RND_SPR_STR_COL_H, page_size >> 8);
		/* spare start address LSB */
		REG_WRITE_U32(REG_RND_SPR_STR_COL_L, page_size & 0xff);
		page_len = page_size >> 9;
		REG_WRITE_U32(REG_DATA_TL1, NF_DATA_TL1_length1(2));
	}

	REG_WRITE_U32(REG_PAGE_LEN, NF_PAGE_LEN_page_len(page_len));

	/* Set PP */
	REG_WRITE_U32(REG_READ_BY_PP, NF_READ_BY_PP_read_by_pp(0));
	REG_WRITE_U32(REG_PP_CTL1, NF_PP_CTL1_pp_start_addr(0));
	REG_WRITE_U32(REG_PP_CTL0, 0);

	/* Set command */
	REG_WRITE_U32(REG_ND_CMD, NF_ND_CMD_cmd(CMD_PG_WRITE_C1));
	REG_WRITE_U32(REG_CMD2, NF_CMD2_cmd2(CMD_PG_WRITE_C2));
	REG_WRITE_U32(REG_CMD3, NF_CMD3_cmd3(CMD_PG_WRITE_C3));

	/* Set address */
	REG_WRITE_U32(REG_ND_PA0, NF_ND_PA0_page_addr0(page));
	REG_WRITE_U32(REG_ND_PA1, NF_ND_PA1_page_addr1(page>>8));
	REG_WRITE_U32(REG_ND_PA2, NF_ND_PA2_addr_mode(addr_mode)|NF_ND_PA2_page_addr2(page>>16));
	REG_WRITE_U32(REG_ND_PA3, NF_ND_PA3_page_addr3((page>>21)&0x7));
	REG_WRITE_U32(REG_ND_CA0, 0);
	REG_WRITE_U32(REG_ND_CA1, 0);

	/* Set ECC */
	REG_WRITE_U32(REG_MULTI_CHNL_MODE, NF_MULTI_CHNL_MODE_edo(1));
	REG_WRITE_U32(REG_ECC_STOP, NF_ECC_STOP_ecc_n_stop(0x01));

	REG_WRITE_U32(REG_ECC_SEL, this->ecc_select);

	dram_sa = ((uintptr_t)data_phy >> 3);
	REG_WRITE_U32(REG_DMA_CTL1, NF_DMA_CTL1_dram_sa(dram_sa));
	dma_len = page_size >> 9;
	REG_WRITE_U32(REG_DMA_CTL2, NF_DMA_CTL2_dma_len(dma_len));

	spare_dram_sa = ((uintptr_t)oobPhys_addr >> 3);
	REG_WRITE_U32(REG_SPR_DDR_CTL, 0x60000000|NF_SPR_DDR_CTL_spare_dram_sa(spare_dram_sa));
	REG_WRITE_U32(REG_DMA_CTL3, NF_DMA_CTL3_ddr_wr(0)|NF_DMA_CTL3_dma_xfer(1));

	syncPageRead();

	REG_WRITE_U32(REG_AUTO_TRIG, NF_AUTO_TRIG_auto_trig(1)|NF_AUTO_TRIG_spec_auto_case(0) | NF_AUTO_TRIG_auto_case(auto_trigger_mode));
	smp_wmb();

	if (WAIT_DONE(REG_DMA_CTL3, 0x01, 0)) {
		NF_ERR_PRINT("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		up_write(&rw_sem);
		return -WAIT_TIMEOUT;
	}
	if (WAIT_DONE(REG_AUTO_TRIG, 0x80, 0)) {
		NF_ERR_PRINT("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		up_write(&rw_sem);
		return -WAIT_TIMEOUT;
	}
	REG_WRITE_U32(REG_POLL_FSTS, NF_POLL_FSTS_bit_sel(6)|NF_POLL_FSTS_trig_poll(1));
	if (WAIT_DONE(REG_POLL_FSTS, 0x01, 0x0)) {
		NF_ERR_PRINT("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		up_write(&rw_sem);
		return -WAIT_TIMEOUT;
	}
	if (WAIT_DONE(REG_ND_CTL, 0x40, 0x40)) {
		NF_ERR_PRINT("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		up_write(&rw_sem);
		return -WAIT_TIMEOUT;
	}

	if (REG_READ_U32(REG_ND_DAT) & 0x01) {
		up_write(&rw_sem);
		NF_ERR_PRINT("[%s] write is not completed at page %d\n", __func__, page);
		return -1;
	}

	chip_section = (chipnr * this->page_num) >> 5;
	section = page >> 5;
	index = page & (32-1);

	up_write(&rw_sem);
	return rc;
}


static int rtk_nand_profile(void)
{
	int maxchips = 4;
	int pnum = 0;
	char *ptype;
	struct mtd_partition *mtd_parts;
	struct nand_chip *this = (struct nand_chip *)rtk_mtd->priv;
	int rc = 0;

	memset(&mtd_parts, 0, sizeof(mtd_parts));

	this->RBA_PERCENT = RBA_PERCENT;

	if (rtk_nand_scan(rtk_mtd, maxchips) < 0 || rtk_mtd->size == 0) {
		NF_ERR_PRINT("%s: Error, cannot do nand_scan(on-board)\n", __func__);
		return -ENODEV;
	}

	if (!(rtk_mtd->writesize&(0x200-1))) {
		REG_WRITE_U32(REG_PAGE_LEN, rtk_mtd->writesize >> 9);
	} else {
		NF_ERR_PRINT("Error: pagesize is not 512B Multiple");
		return -1;
	}

#ifdef CONFIG_MTD_CMDLINE_PARTS
	ptype = (char *)ptypes[0];
	pnum = parse_mtd_partitions(rtk_mtd, ptypes, &mtd_parts, 0);
#endif

	if (pnum <= 0) {
		printk(KERN_ERR "RTK: using the whole nand as a partitoin\n");
		if (add_mtd_device(rtk_mtd)) {
			printk(KERN_WARNING "RTK: Failed to register new nand device\n");
			return -EAGAIN;
		}
	} else {
		printk(KERN_ERR "RTK: using dynamic nand partition\n");
		if (add_mtd_partitions(rtk_mtd, mtd_parts, pnum)) {
			printk("%s: Error, cannot add %s device\n", __func__, rtk_mtd->name);
			rtk_mtd->size = 0;
			return -EAGAIN;
		}
	}

	return 0;
}

static void display_version(void)
{
	const __u8 *revision;
	const __u8 *date;
	char *running = (__u8 *)VERSION;

	strsep(&running, " ");
	strsep(&running, " ");
	revision = strsep(&running, " ");
	date = strsep(&running, " ");
	printk(BANNER " Rev:%s (%s)\n", revision, date);
}

#define CTL_ENABLE  1
#define CTL_DISABLE 0

static int rtk_nand_clk_reset_ctrl(struct device *dev, int enable)
{
	struct reset_control *reset = reset_control_get(dev, NULL);
	struct clk *clk = clk_get(dev, NULL);

	if (enable == CTL_ENABLE) {
		reset_control_deassert(reset);
		clk_prepare_enable(clk);
	} else {
		clk_disable_unprepare(clk);
	}

	clk_put(clk);
	reset_control_put(reset);

	return 0;
}

static int rtk_read_proc_nandinfo(struct seq_file *m, void *v)
{
	struct nand_chip *this = (struct nand_chip *) rtk_mtd->priv;
	int i;

	seq_printf(m, "nand_size:%u\n", (uint32_t)this->device_size);
	seq_printf(m, "chip_size:%u\n", (uint32_t)this->chipsize);
	seq_printf(m, "block_size:%u\n", rtk_mtd->erasesize);
	seq_printf(m, "page_size:%u\n", rtk_mtd->writesize);
	seq_printf(m, "oob_size:%u\n", rtk_mtd->oobsize);
	seq_printf(m, "ppb:%u\n", rtk_mtd->erasesize/rtk_mtd->writesize);
	seq_printf(m, "RBA:%u\n", this->RBA);
	seq_printf(m, "BBs:%u\n", this->BBs);
	seq_printf(m, "ecc bits:%u\n", g_eccbits);
	seq_puts(m, "\n");
	seq_puts(m, "Bad block table:\n");

	for (i = 0; i < this->RBA; i++) {
		if ((this->bbt[i].BB_die == BB_DIE_INIT) && (this->bbt[i].bad_block == BB_INIT)) {
			seq_printf(m, "[%d] (BB_DIE_INIT, BB_INIT, %d, %u(0x%x))\n",
					i,
					this->bbt[i].RB_die,
					this->bbt[i].remap_block, this->bbt[i].remap_block * 131072);
		} else if (this->bbt[i].bad_block == BAD_RESERVED) {
			seq_printf(m, "[%d] (%d, 0x%x, %u, %u(0x%x))\n",
					i,
					this->bbt[i].BB_die,
					this->bbt[i].bad_block,
					this->bbt[i].RB_die,
					this->bbt[i].remap_block, this->bbt[i].remap_block * 131072);
		} else {
			seq_printf(m, "[%d] (%d, %u(0x%x), %d, %u(0x%x))\n",
					i,
					this->bbt[i].BB_die,
					this->bbt[i].bad_block, this->bbt[i].bad_block * 131072,
					this->bbt[i].RB_die,
					this->bbt[i].remap_block, this->bbt[i].remap_block * 131072);
		}
	}
	seq_puts(m, "\n");

	return 0;
}

static int rtk_nand_proc_open(struct inode *inode, struct  file *file)
{
	return single_open(file, rtk_read_proc_nandinfo, NULL);
}

/*******************************************************************************************************/
/* ioctl start */
/*******************************************************************************************************/

#define IOC_MAGIC	'k'
#define GETRUNFLAG	_IOR(IOC_MAGIC, 1, int)
#define GETFWINFO1	_IOR(IOC_MAGIC, 2, RTK_VER_INFO_T)
#define GETFWINFO2	_IOR(IOC_MAGIC, 3, RTK_VER_INFO_T)
#define SETFWINFO1	_IOW(IOC_MAGIC, 1, RTK_VER_INFO_T)
#define SETFWINFO2	_IOW(IOC_MAGIC, 2, RTK_VER_INFO_T)
#define IOC_MAXNR       5

extern unsigned int g_boot_flag;
extern void rtk_get_upgrade_info(void *info, int fw);
extern int rtk_set_upgrade_info(void *info, int fw);
static int rtk_nand_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
	int tmp, err = 0, ret = 0;
	RTK_VER_INFO_T info;

	if (_IOC_TYPE(cmd) != IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > IOC_MAXNR)
		return -ENOTTY;
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)args, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & (_IOC_WRITE))
		err = !access_ok(VERIFY_READ, (void __user *)args, _IOC_SIZE(cmd));


	if (err)
	       return -EFAULT;

	switch (cmd) {
	case GETRUNFLAG:
		if (copy_to_user((int __user *)args, &g_boot_flag, sizeof(int)))
			return -EFAULT;
		break;

	case GETFWINFO1:
		memset(&info, 0x0, sizeof(RTK_VER_INFO_T));
		rtk_get_upgrade_info(&info, 1);
		if (copy_to_user((int __user *)args, &info, sizeof(RTK_VER_INFO_T)))
			return -EFAULT;
		break;

	case GETFWINFO2:
		memset(&info, 0x0, sizeof(RTK_VER_INFO_T));
		rtk_get_upgrade_info(&info, 2);
		if (copy_to_user((int __user *)args, &info, sizeof(RTK_VER_INFO_T)))
			return -EFAULT;
		break;

	case SETFWINFO1:
		memset(&info, 0x0, sizeof(RTK_VER_INFO_T));
		if (copy_from_user(&info, (int __user *)args, sizeof(RTK_VER_INFO_T)))
			return -EFAULT;
		ret = rtk_set_upgrade_info(&info, 1);
		break;

	case SETFWINFO2:
		memset(&info, 0x0, sizeof(RTK_VER_INFO_T));
		if (copy_from_user(&info, (int __user *)args, sizeof(RTK_VER_INFO_T)))
			return -EFAULT;
		ret = rtk_set_upgrade_info(&info, 2);
		break;
	default:
		return -ENOTTY;
	}

	return ret;
}
/*******************************************************************************************************/
/* ioctl end */
/*******************************************************************************************************/

static const struct file_operations info_proc_fops = {
	.owner = THIS_MODULE,
	.open = rtk_nand_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.unlocked_ioctl = rtk_nand_ioctl,
};

static void rtknand_proc_init(void)
{
	rtknand_proc_dir = proc_mkdir(RTKNAND_PROC_DIR_NAME, &proc_root);
	if (rtknand_proc_dir) {
		proc_create_data("info", 0000, rtknand_proc_dir, &info_proc_fops, NULL);
	}
}

static int __init rtk_nand_probe(void)
{
	int rc = 0;
	struct nand_chip *this = NULL;

	init_rwsem(&rw_sem);

	REG_WRITE_U32(REG_CLOCK_ENABLE1, REG_READ_U32(REG_CLOCK_ENABLE1) & (~0x00800000));
	REG_WRITE_U32(REG_NF_CKSEL, 0x04);
	REG_WRITE_U32(REG_CLOCK_ENABLE1, REG_READ_U32(REG_CLOCK_ENABLE1) | (0x00800000));

	REG_WRITE_U32(REG_NF_LOW_PWR, REG_READ_U32(REG_NF_LOW_PWR)&~0x10);
	REG_WRITE_U32(REG_NF_SWC_LOW_PWR, REG_READ_U32(REG_NF_SWC_LOW_PWR)&~0x10);
	NF_DEBUG_PRINT("NF_LOW_PWR:0x%08x, NF_SWC_LOW_PWR:0x%08x\n", REG_READ_U32(REG_NF_LOW_PWR), REG_READ_U32(REG_NF_LOW_PWR));

	/* controller init. */
	REG_WRITE_U32(REG_PD, 0x1E);
	REG_WRITE_U32(REG_TIME_PARA3, 0x2);
	REG_WRITE_U32(REG_TIME_PARA2, 0x5);
	REG_WRITE_U32(REG_TIME_PARA1, 0x2);

	REG_WRITE_U32(REG_MULTI_CHNL_MODE, 0x0);
	REG_WRITE_U32(REG_READ_BY_PP, 0x0);

	/* reset nand. */
	REG_WRITE_U32(REG_ND_CMD, 0xff);
	REG_WRITE_U32(REG_ND_CTL, 0x80);
	rc = WAIT_DONE(REG_ND_CTL, 0x80, 0);
	rc = WAIT_DONE(REG_ND_CTL, 0x40, 0x40);
	NF_DEBUG_PRINT("nand ready.\n");

	if (REG_READ_U32(REG_CLOCK_ENABLE1) & 0x00800000) {
		display_version();
	} else{
		NF_ERR_PRINT("Nand Flash Clock is NOT Open. Installing fails.\n");
	}

	/* set spare2ddr func. 4=>0.5k spe2ddr_ena at A000F000 */
	REG_WRITE_U32(REG_SPR_DDR_CTL, NF_SPR_DDR_CTL_spare_ddr_ena(1) | NF_SPR_DDR_CTL_per_2k_spr_ena(1) | NF_SPR_DDR_CTL_spare_dram_sa(0));
	rtk_mtd = kmalloc(MTDSIZE, GFP_KERNEL);
	if (!rtk_mtd) {
		NF_ERR_PRINT("%s: Error, no enough memory for rtk_mtd\n", __func__);
		rc = -ENOMEM;
		goto EXIT;
	}
	memset((char *)rtk_mtd, 0, MTDSIZE);
	rtk_mtd->priv = this = (struct nand_chip *)(rtk_mtd+1);

	this->read_id		= rtk_nand_read_id;
	this->read_ecc_page	= rtk_read_ecc_page;
	this->read_oob		= NULL;
	this->write_ecc_page	= rtk_write_ecc_page;
	this->write_oob		= rtk_write_oob;
	this->erase_block	= rtk_erase_block;
	this->sync		= NULL;

	if (rtk_nand_profile() < 0) {
		rc = -1;
		goto EXIT;
	}

	page_size = rtk_mtd->writesize;
	oob_size = rtk_mtd->oobsize;
	ppb = (rtk_mtd->erasesize)/(rtk_mtd->writesize);

	rtknand_proc_init();
EXIT:
	if (rc < 0) {
		if (rtk_mtd) {
			kfree(rtk_mtd);
		}
	} else {
		NF_INIT_PRINT("Realtek Nand Flash Driver is successfully installing.\n");
	}

	return rc;
}

static __exit rtk_nand_remove(void)
{
	struct nand_chip *this = NULL;

	if (rtk_mtd) {
		this = (struct nand_chip *)rtk_mtd->priv;
		if (this->g_databuf)
			kfree(this->g_databuf);
		if (this->g_oobbuf)
			kfree(this->g_oobbuf);
		kfree(rtk_mtd);

	}

	return 0;
}

static struct of_device_id rtk129x_nand_ids[] = {
	{ .compatible = "Realtek,rtk1395-nand" },
	{ .compatible = "Realtek,rtk1295-nand" },
	{ .compatible = "Realtek,rtk1195-nand" },
	{ /* Sentinel */ },
};

static const struct dev_pm_ops rtk_nand_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(rtk_nand_suspend, rtk_nand_resume)
};

static struct platform_driver rtk_nand_driver = {
	.probe      = rtk_nand_probe,
	.remove     = rtk_nand_remove,
	.driver     = {
		.name = "rtk_nand_driver",
		.owner = THIS_MODULE,
		.of_match_table = rtk129x_nand_ids,
		.pm     = &rtk_nand_dev_pm_ops,
	},
};

module_init(rtk_nand_probe);
module_exit(rtk_nand_remove);

static int __init rtk_nand_info_setup(char *line)
{
	strlcpy(g_rtk_nandinfo_line, line, sizeof(g_rtk_nandinfo_line));

	return 1;
}
__setup("nandinfo=", rtk_nand_info_setup);

MODULE_AUTHOR("AlexChang <alexchang2131@realtek.com>");
MODULE_DESCRIPTION("Realtek NAND Flash Controller Driver");
