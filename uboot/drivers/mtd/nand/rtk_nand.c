/******************************************************************************
 * $Id: rtk_nand.c 337905 2010-10-18 01:32:27Z alexchang2131 $
 * drivers/mtd/nand/rtk_nand.c
 * Overview: Realtek NAND Flash Controller Driver
 * Copyright (c) 2008 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2010-07-02 AlexChang   create file
 *
 *
 *******************************************************************************/

#include <common.h>
#include <linux/mtd/rtk_nand.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/list.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <mtd/mtd-abi.h>
#include <linux/time.h>
#include <linux/string.h>
#include <asm/arch/rbus/nand_reg.h>
#include <asm/arch/platform_lib/board/gpio.h>

#define BANNER	"Realtek NAND Flash Driver"
#define VERSION	"$Id: rtk_nand.c 337905 2010-10-18 01:32:27Z alexchang2131 $"

#define MODULE_AUTHOR(x)	/* x */
#define MODULE_DESCRIPTION(x)	/* x */

#define OTP_K_S		2048
#define OTP_K_H		2304
#define OTP_K_N         2560


extern unsigned int g_eccbits;
char rtknand_info[128] = {0};
static unsigned int g_BootcodeSize;
static unsigned int g_Factory_param_size;

/* nand driver low-level functions */
static int rtk_read_ecc_page(struct mtd_info *mtd, u16 chipnr, unsigned int page,
			u_char *data, u_char *oob_buf, u16 cp_mode, u16 cp_first, size_t cp_len, int tee_os);
static int rtk_write_ecc_page(struct mtd_info *mtd, u16 chipnr, unsigned int page,
			const u_char *data, const u_char *oob_buf, int isBBT);
static int rtk_erase_block(struct mtd_info *mtd, u16 chipnr, int page);
#ifdef CONFIG_CMD_KEY_BURNING
extern int OTP_Get_Byte(int offset, unsigned char *ptr, unsigned int cnt);
#endif

/* Global Variables */
static int page_size, oob_size, ppb;
static int RBA_PERCENT = 5;

void syncPageRead(void)
{
	CP15DMB;
	REG_WRITE_U32(0x1801a020, 0x0);
	CP15DMB;
}

static void writeNFKey(unsigned int data[4])
{
	REG_WRITE_U32(CP_NF_KEY_0, data[0]);
	REG_WRITE_U32(CP_NF_KEY_1, data[1]);
	REG_WRITE_U32(CP_NF_KEY_2, data[2]);
	REG_WRITE_U32(CP_NF_KEY_3, data[3]);
}

static unsigned int OTP_JUDGE_BIT(unsigned int offset)
{
	unsigned int div_n = 0, rem_n = 0;
	unsigned int align_n = 0, align_rem_n = 0, real_n = 0;

	rem_n = offset%8;
	div_n = offset/8;
	align_n = div_n & ~0x3;
	align_rem_n = div_n & 0x3;
	real_n = REG32(OTP_REG_BASE + align_n);

	return(((real_n >> (align_rem_n*8)) >> rem_n)&1);
}

int WAIT_DONE(unsigned int addr, unsigned int mask, unsigned int value)
{
	int timeout = 0;

	while ((REG_READ_U32(addr) & mask) != value) {
		if (timeout++ > 500) {
			printk("WAIT_DONE timeout\n");
			return -WAIT_TIMEOUT;
		}
		udelay(100);
	}
	return 0;
}

/**
 *      rtk_nand_reset -
 *
 *      initialize host controller;
 *      dont reset nand flash, it is dangerous
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

static void rtk_read_oob_from_SRAM(struct mtd_info *mtd, __u8 *oobbuf)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	unsigned int reg_oob;
	unsigned char *reg = NULL;
	int i;
	char r_oobbuf[256];
	int oobuse_size;

	REG_WRITE_U32(REG_READ_BY_PP, 0x00);
	REG_WRITE_U32(REG_SRAM_CTL, 0x30 | 0x04);

	memset(r_oobbuf, 0xFF, 256);
	memset(oobbuf, 0xFF, mtd->oobsize);

	reg = REG_NF_BASE_ADDR;

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

	for (i = 0; i < oobuse_size; i++) {
		reg_oob = REG_READ_U32(reg+(i*4));

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

static void rtk_write_oob_to_SRAM(struct mtd_info *mtd, const __u8 *oobbuf)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	unsigned char *reg = NULL;
	int i;
	int oobuse_size;
	unsigned int data;

	REG_WRITE_U32(REG_READ_BY_PP, 0x00);
	REG_WRITE_U32(REG_SRAM_CTL, 0x30 | 0x04);

	reg = REG_NF_BASE_ADDR;

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

	for (i = 0; i < oobuse_size; i++) {

		data = oobbuf[(i*4)] | oobbuf[(i*4)+1] << 8 | oobbuf[(i*4)+2] << 16 | oobbuf[(i*4)+3] << 24;

		REG_WRITE_U32(reg+(i*4), data);
	}

	REG_WRITE_U32(REG_SRAM_CTL, 0x00);
	REG_WRITE_U32(REG_READ_BY_PP, 0x80);

	return;
}

static void rtk_nand_read_id(struct mtd_info *mtd, u_char id[6])
{
	int id_chain;

	REG_WRITE_U32(REG_DATA_TL0, NF_DATA_TL0_length0(0x06));
	REG_WRITE_U32(REG_DATA_TL1, NF_DATA_TL1_access_mode(0x01));

	/* Set PP */
	REG_WRITE_U32(REG_READ_BY_PP, NF_READ_BY_PP_read_by_pp(0x0));

	REG_WRITE_U32(REG_PP_CTL0, NF_PP_CTL0_pp_enable(0x01));
	REG_WRITE_U32(REG_PP_CTL1, NF_PP_CTL1_pp_start_addr(0));

	/* Set command */
	REG_WRITE_U32(REG_ND_CMD, NF_ND_CMD_cmd(CMD_READ_ID));
	REG_WRITE_U32(REG_ND_CTL, NF_ND_CTL_xfer(0x01));
	WAIT_DONE(REG_ND_CTL, 0x80, 0);

	/* Set address */
	REG_WRITE_U32(REG_ND_PA0, 0);
	REG_WRITE_U32(REG_ND_PA1, 0);
	REG_WRITE_U32(REG_ND_PA2, NF_ND_PA2_addr_mode(0x07));

	REG_WRITE_U32(REG_ND_CTL, NF_ND_CTL_xfer(1)|NF_ND_CTL_tran_mode(1));
	WAIT_DONE(REG_ND_CTL, 0x80, 0);
	/* Enable XFER mode */
	REG_WRITE_U32(REG_ND_CTL, NF_ND_CTL_xfer(1)|NF_ND_CTL_tran_mode(4));
	WAIT_DONE(REG_ND_CTL, 0x80, 0);

	/* Reset PP */
	REG_WRITE_U32(REG_PP_CTL0, NF_PP_CTL0_pp_reset(1));

	/* Move data to DRAM from SRAM */
	REG_WRITE_U32(REG_SRAM_CTL, NF_SRAM_CTL_map_sel(1)|NF_SRAM_CTL_access_en(1)|NF_SRAM_CTL_mem_region(0));

	id_chain = REG_READ_U32(REG_ND_PA0);

	id[0] = id_chain & 0xff;
	id[1] = (id_chain >> 8) & 0xff;
	id[2] = (id_chain >> 16) & 0xff;
	id[3] = (id_chain >> 24) & 0xff;

	id_chain = REG_READ_U32(REG_ND_PA1);

	id[4] = id_chain & 0xff;
	id[5] = (id_chain >> 8) & 0xff;

	REG_WRITE_U32(REG_SRAM_CTL, 0x0);
}

static int rtk_erase_block(struct mtd_info *mtd, u16 chipnr, int page)
{
	struct nand_chip *this = NULL;
	unsigned int chip_section = 0;
	unsigned int section = 0;

	page_size = mtd->oobblock;
	oob_size = mtd->oobsize;
	ppb = mtd->erasesize/mtd->oobblock;
	this = (struct nand_chip *) mtd->priv;

	if (page & (ppb-1)) {
		printk("%s: page %d is not block alignment !!\n", __func__, page);
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
		printk("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		return -WAIT_TIMEOUT;
	}

	REG_WRITE_U32(REG_POLL_FSTS, NF_POLL_FSTS_bit_sel(6)|NF_POLL_FSTS_trig_poll(1));

	if (WAIT_DONE(REG_POLL_FSTS, 0x01, 0x0)) {
		printk("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		return -WAIT_TIMEOUT;
	}

	if (WAIT_DONE(REG_ND_CTL, 0x40, 0x40)) {
		printk("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		return -WAIT_TIMEOUT;
	}

	if (REG_READ_U32(REG_ND_DAT) & 0x01) {
		printk("[%s] erasure is not completed at block %d\n", __func__, page/ppb);
		return -1;
	} else {
		printk("[%s] erase %d OK\n", __func__, page/ppb);
	}

	return 0;
}

static int rtk_read_ecc_page (struct mtd_info *mtd, u16 chipnr, unsigned int page, u_char *data_buf, u_char *oob_buf, u16 cp_mode, u16 cp_first, size_t cp_len, int tee_os)
{
	struct nand_chip *this = NULL;
	int rc = 0;
	int dram_sa, dma_len, spare_dram_sa;
	int blank_all_one = 0;
	int page_len;
	unsigned int blank_check = 0;
	unsigned int chip_section = 0;
	unsigned int section = 0;
	unsigned int index = 0;
	uint8_t	auto_trigger_mode = 2;
	uint8_t	addr_mode = 1;
	uint8_t	bChkAllOne = 0;
	uint8_t read_retry_cnt = 0;
	uint8_t max_read_retry_cnt = 0;
	unsigned int eccNum = 0;
	unsigned int blank_confirm = 0;


	this = (struct nand_chip *) mtd->priv;
	page_size = mtd->oobblock;
	oob_size = mtd->oobsize;
	ppb = mtd->erasesize/mtd->oobblock;

	volatile unsigned int data;
	unsigned char ks[16];
	unsigned char kh[16];
	unsigned char dest[16];
	unsigned int *intPTR;

	if (((uint32_t)data_buf&0x7) != 0) {
		printk("[%s]data_buf must 8 byte alignmemt!!\n", __func__);
		BUG();
	}

blank_confirm_read:
	while (1) {

		REG_WRITE_U32(REG_DATA_TL0, NF_DATA_TL0_length0(0));
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

		if (cp_mode == CP_NF_AES_ECB_128 || (cp_mode == CP_NF_AES_CBC_128 && cp_first == 1)) {

#ifdef CONFIG_NAND_ON_THE_FLY_TEST_KEY
			if (cp_mode == CP_NF_AES_CBC_128) {

				REG_WRITE_U32(CP_NF_INI_KEY_0, 0x0);
				REG_WRITE_U32(CP_NF_INI_KEY_1, 0x0);
				REG_WRITE_U32(CP_NF_INI_KEY_2, 0x0);
				REG_WRITE_U32(CP_NF_INI_KEY_3, 0x0);
			}

			REG_WRITE_U32(CP_NF_KEY_0, 0xad0d8175);
			REG_WRITE_U32(CP_NF_KEY_1, 0xa0d732c0);
			REG_WRITE_U32(CP_NF_KEY_2, 0xe56ef350);
			REG_WRITE_U32(CP_NF_KEY_3, 0xc53ce48b);
			/* set CP register. */

			if (cp_mode == CP_NF_AES_ECB_128) {
				/* sel=0, cw_entry=0, bcm=1, aes_mode=0. Its ECB mode. */
				REG_WRITE_U32(CP_NF_SET, 0x200);
			} else {

				/* sel=0, cw_entry=0, bcm=0, aes_mode=0. Its CBC mode. */
				REG_WRITE_U32(CP_NF_SET, 0x0);
			}
#else
			/* set CP register. */

			if (cp_mode == CP_NF_AES_ECB_128) {

				if (!OTP_JUDGE_BIT(OTP_BIT_SECUREBOOT)) {
					/* key from kn */
					REG_WRITE_U32(CP_NF_SET, 0x202);
				} else if (OTP_JUDGE_BIT(OTP_BIT_SECUREBOOT)) {

#ifdef CONFIG_CMD_KEY_BURNING
					OTP_Get_Byte(OTP_K_S, ks, 16);
					OTP_Get_Byte(OTP_K_H, kh, 16);
#endif
					AES_ECB_encrypt(ks, 16, dest, kh);

					intPTR = dest;
					intPTR[0] = swap_endian(intPTR[0]);
					intPTR[1] = swap_endian(intPTR[1]);
					intPTR[2] = swap_endian(intPTR[2]);
					intPTR[3] = swap_endian(intPTR[3]);

					writeNFKey(dest);

					memset(ks, 0, 16);
					memset(kh, 0, 16);
					memset(dest, 0, 16);

					/* sel=0, cw_entry=0, bcm=1, aes_mode=0. Its ECB mode. */
					REG_WRITE_U32(CP_NF_SET, 0x200);
				}
			} else {

				if (!OTP_JUDGE_BIT(OTP_BIT_SECUREBOOT)) {
					/* key from kn */
					REG_WRITE_U32(CP_NF_SET, 0x002);
				} else if (OTP_JUDGE_BIT(OTP_BIT_SECUREBOOT)) {

					if (tee_os)
						REG_WRITE_U32(CP_NF_SET, 0x0000 | 0x2002);
					else
						REG_WRITE_U32(CP_NF_SET, 0x0000 | 0x4002);

				}
			}
#endif

			if (cp_mode == CP_NF_AES_CBC_128)
				/* integer multiple of dma_len. */
				REG_WRITE_U32(REG_CP_LEN, (cp_len / 0x200) << 9);
			else
				/* integer multiple of dma_len. */
				REG_WRITE_U32(REG_CP_LEN, (page_size / 0x200) << 9);

			syncPageRead();
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

		dram_sa = ((uint32_t)data_buf >> 3);
		REG_WRITE_U32(REG_DMA_CTL1, NF_DMA_CTL1_dram_sa(dram_sa));
		dma_len = page_size >> 9;
		REG_WRITE_U32(REG_DMA_CTL2, NF_DMA_CTL2_dma_len(dma_len));

		if (cp_mode == CP_NF_AES_CBC_128) {

			if (cp_first == 1)
				REG_WRITE_U32(REG_DMA_CTL3, NF_DMA_CTL3_cp_enable(1)|NF_DMA_CTL3_cp_first(1)|NF_DMA_CTL3_ddr_wr(1)|NF_DMA_CTL3_dma_xfer(1));
			else
				REG_WRITE_U32(REG_DMA_CTL3, NF_DMA_CTL3_cp_enable(1)|NF_DMA_CTL3_cp_first(0)|NF_DMA_CTL3_ddr_wr(1)|NF_DMA_CTL3_dma_xfer(1));
		} else if (cp_mode == CP_NF_AES_ECB_128) {

			REG_WRITE_U32(REG_DMA_CTL3, NF_DMA_CTL3_cp_enable(1)|NF_DMA_CTL3_cp_first(1)|NF_DMA_CTL3_ddr_wr(1)|NF_DMA_CTL3_dma_xfer(1));
		} else {

			REG_WRITE_U32(REG_DMA_CTL3, NF_DMA_CTL3_ddr_wr(1)|NF_DMA_CTL3_dma_xfer(1));
		}

		/* flush cache. */
		RTK_FLUSH_CACHE(data_buf, page_size);
		RTK_FLUSH_CACHE(oob_buf, oob_size);


		/* Enable Auto mode */
		REG_WRITE_U32(REG_AUTO_TRIG, NF_AUTO_TRIG_auto_trig(1)|NF_AUTO_TRIG_spec_auto_case(0) | NF_AUTO_TRIG_auto_case(auto_trigger_mode));

		if (WAIT_DONE(REG_AUTO_TRIG, 0x80, 0)) {
			printk("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
			return -WAIT_TIMEOUT;
		}

		if (WAIT_DONE(REG_DMA_CTL3, 0x01, 0)) {
			printk("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
			return -WAIT_TIMEOUT;

		}

		if (oob_buf)	{
			syncPageRead();
			rtk_read_oob_from_SRAM(mtd, oob_buf);
		}


		if (blank_confirm) {
			blank_confirm = 0;

			if (REG_READ_U32(REG_ND_ECC) & 0x8) {
				printk("[DBG]ecc error... page=0x%x, REG_BLANK_CHK reg: 0x%x\n", page, REG_READ_U32(REG_BLANK_CHK));
				REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));
				return -1;
			} else {
				printk("Correctable data all one\n");
				if (oob_buf)
					memset(oob_buf, 0xFF, mtd->oobsize);

				REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));
				return 1;
			}
		} else {

			blank_check = REG_READ_U32(REG_BLANK_CHK);
			if (blank_check & 0x2) {
				printk("data all one\n");
				REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));
				return 1;
			} else if (REG_READ_U32(REG_ND_ECC) & 0x8) {
				blank_confirm = 1;
				printk("[DBG]ecc error... goto blank_confirm_read   page=0x%x, REG_BLANK_CHK reg: 0x%x\n", page, REG_READ_U32(REG_BLANK_CHK));
				REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));
				goto blank_confirm_read;
			} else {
				if (REG_READ_U32(REG_ND_ECC) & 0x04) {
					eccNum = REG_READ_U32(REG_MAX_ECC_NUM) & 0xff;
					printk("[NAND_DBG][%s]:%d, ecc_num:%d, page:%u\n", __func__, __LINE__, eccNum, page);
					if (eccNum > (g_eccbits - 2)) {
						printk("[NAND_DBG][%s]:%d, ecc_num:%d, page:%u\n", __func__, __LINE__, eccNum, page);
						REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));
						return -2;
					}
				}

				REG_WRITE_U32(REG_BLANK_CHK, NF_BLANK_CHK_blank_ena(1)|NF_BLANK_CHK_read_ecc_xnor_ena(0));
				return 0;
			}

		} /* if(blank_confirm) */
	} /* while (1) */

	return rc;
}

static int rtk_write_ecc_page(struct mtd_info *mtd, u16 chipnr, unsigned int page,
			const u_char *data_buf, const  u_char *oob_buf, int isBBT)
{
	unsigned int ppb = mtd->erasesize/mtd->oobblock;
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int rc = 0;
	uint8_t	auto_trigger_mode = 1;
	uint8_t	addr_mode = 1;

	unsigned int page_len, dram_sa, dma_len, spare_dram_sa;
	unsigned char oob_1stB;

	unsigned char nf_oob_buf[oob_size];
	unsigned int chip_section = 0;
	unsigned int section = 0;
	unsigned int index = 0;


	if (((uint32_t)data_buf&0x7) != 0) {
		printk("[%s]data_buf must 8 byte alignmemt!!\n", __func__);
		BUG();
	}

	if (((uint32_t)oob_buf&0x7) != 0) {
		printk("[%s]oob_buf must 8 byte alignmemt!!\n", __func__);
		BUG();
	}


	REG_WRITE_U32(REG_DATA_TL0, NF_DATA_TL0_length0(0));
	if (this->ecc_select >= 0x18) {
		/* enable randomizer */
		REG_WRITE_U32(REG_RMZ_CTRL, 1);

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

	/* flush cache. */
	RTK_FLUSH_CACHE(data_buf, page_size);
	RTK_FLUSH_CACHE(this->g_oobbuf, oob_size);

	dram_sa = ((uint32_t)data_buf >> 3);
	REG_WRITE_U32(REG_DMA_CTL1, NF_DMA_CTL1_dram_sa(dram_sa));
	dma_len = page_size >> 9;
	REG_WRITE_U32(REG_DMA_CTL2, NF_DMA_CTL2_dma_len(dma_len));

	spare_dram_sa = ((uint32_t)this->g_oobbuf >> 3);
	REG_WRITE_U32(REG_SPR_DDR_CTL, NF_SPR_DDR_CTL_spare_ddr_ena(1)|NF_SPR_DDR_CTL_per_2k_spr_ena(1)|NF_SPR_DDR_CTL_spare_dram_sa(spare_dram_sa));

	REG_WRITE_U32(REG_DMA_CTL3, NF_DMA_CTL3_ddr_wr(0)|NF_DMA_CTL3_dma_xfer(1));
	REG_WRITE_U32(REG_AUTO_TRIG, NF_AUTO_TRIG_auto_trig(1)|NF_AUTO_TRIG_spec_auto_case(0) | NF_AUTO_TRIG_auto_case(auto_trigger_mode));


	if (WAIT_DONE(REG_DMA_CTL3, 0x01, 0)) {
		printk("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		return -WAIT_TIMEOUT;
	}
	if (WAIT_DONE(REG_AUTO_TRIG, 0x80, 0)) {
		printk("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		return -WAIT_TIMEOUT;
	}

	REG_WRITE_U32(REG_POLL_FSTS, NF_POLL_FSTS_bit_sel(6)|NF_POLL_FSTS_trig_poll(1));
	if (WAIT_DONE(REG_POLL_FSTS, 0x01, 0x0)) {
		printk("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		return -WAIT_TIMEOUT;
	}

	if (WAIT_DONE(REG_ND_CTL, 0x40, 0x40)) {
		printk("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		return -WAIT_TIMEOUT;
	}

	if (REG_READ_U32(REG_ND_DAT) & 0x01) {
		printk("[%s] write is not completed at page %d\n", __func__, page);
		return -1;
	}

	return 0;
}

#ifdef RTK_NAND_VERIFY
static int rtk_write_ecc_page_raw_mode(struct mtd_info *mtd, u16 chipnr, unsigned int page,
			const u_char *data_buf, const  u_char *oob_buf, int isBBT)
{
	unsigned int ppb = mtd->erasesize/mtd->oobblock;
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int rc = 0;
	uint8_t	auto_trigger_mode = 1;
	uint8_t	addr_mode = 1;

	unsigned int page_len, dram_sa, dma_len, spare_dram_sa;
	unsigned char oob_1stB;

	unsigned char nf_oob_buf[oob_size];
	unsigned int chip_section = 0;
	unsigned int section = 0;
	unsigned int index = 0;

	if (((uint32_t)data_buf&0x7) != 0) {
		printk("[%s]data_buf must 8 byte alignmemt!!\n", __func__);
		BUG();
	}

	if (((uint32_t)oob_buf&0x7) != 0) {
		printk("[%s]oob_buf must 8 byte alignmemt!!\n", __func__);
		BUG();
	}

	if (oob_buf) {
		rtk_write_oob_to_SRAM(mtd, oob_buf);
	}

	REG_WRITE_U32(REG_DATA_TL0, NF_DATA_TL0_length0(0));
	if (this->ecc_select >= 0x18) {
		/* enable randomizer */
		REG_WRITE_U32(REG_RMZ_CTRL, 1);

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
	REG_WRITE_U32(REG_MULTI_CHNL_MODE, NF_MULTI_CHNL_MODE_edo(1) | NF_MULTI_CHNL_MODE_ecc_pass(1) | NF_MULTI_CHNL_MODE_ecc_no_check(0));
	REG_WRITE_U32(REG_ECC_STOP, NF_ECC_STOP_ecc_n_stop(0x01));

	REG_WRITE_U32(REG_ECC_SEL, this->ecc_select);

	/* flush cache. */
	RTK_FLUSH_CACHE(data_buf, page_size);

	dram_sa = ((uint32_t)data_buf >> 3);
	REG_WRITE_U32(REG_DMA_CTL1, NF_DMA_CTL1_dram_sa(dram_sa));
	dma_len = page_size >> 9;
	REG_WRITE_U32(REG_DMA_CTL2, NF_DMA_CTL2_dma_len(dma_len));

	REG_WRITE_U32(REG_SPR_DDR_CTL, 0x0);

	REG_WRITE_U32(REG_DMA_CTL3, NF_DMA_CTL3_ddr_wr(0)|NF_DMA_CTL3_dma_xfer(1));
	REG_WRITE_U32(REG_AUTO_TRIG, NF_AUTO_TRIG_auto_trig(1)|NF_AUTO_TRIG_spec_auto_case(0) | NF_AUTO_TRIG_auto_case(auto_trigger_mode));


	if (WAIT_DONE(REG_DMA_CTL3, 0x01, 0)) {
		printk("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		return -WAIT_TIMEOUT;
	}
	if (WAIT_DONE(REG_AUTO_TRIG, 0x80, 0)) {
		printk("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		return -WAIT_TIMEOUT;
	}

	REG_WRITE_U32(REG_POLL_FSTS, NF_POLL_FSTS_bit_sel(6)|NF_POLL_FSTS_trig_poll(1));
	if (WAIT_DONE(REG_POLL_FSTS, 0x01, 0x0)) {
		printk("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		return -WAIT_TIMEOUT;
	}
	if (WAIT_DONE(REG_ND_CTL, 0x40, 0x40)) {
		printk("[%s] WAIT_DONE timeout. (%d)\n", __func__, __LINE__);
		return -WAIT_TIMEOUT;
	}

	if (REG_READ_U32(REG_ND_DAT) & 0x01) {
		printk("[%s] write is not completed at page %d\n", __func__, page);
		return -1;
	}

	return 0;
}
#endif

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

int board_nand_init(struct nand_chip *nand)
{
	display_version();

	REG_WRITE_U32(REG_CLOCK_ENABLE1, REG_READ_U32(REG_CLOCK_ENABLE1) & (~0x00800000));
	REG_WRITE_U32(REG_NF_CKSEL, 0x04);
	REG_WRITE_U32(REG_CLOCK_ENABLE1, REG_READ_U32(REG_CLOCK_ENABLE1) | (0x00800000));

	/* set pinmux to nand. */
	REG_WRITE_U32(SYS_muxpad0, 0x55555554);

	/* controller init. */
	REG_WRITE_U32(REG_PD, 0x1E);
	REG_WRITE_U32(REG_TIME_PARA3, 0x1);
	REG_WRITE_U32(REG_TIME_PARA2, 0x1);
	REG_WRITE_U32(REG_TIME_PARA1, 0x1);

	REG_WRITE_U32(REG_MULTI_CHNL_MODE, 0x20);
	REG_WRITE_U32(REG_READ_BY_PP, 0x0);

	/* reset nand. */
	REG_WRITE_U32(REG_ND_CMD, 0xff);
	REG_WRITE_U32(REG_ND_CTL, 0x80);

	WAIT_DONE(REG_ND_CTL, 0x80, 0);
	WAIT_DONE(REG_ND_CTL, 0x40, 0x40);

	nand->read_id		= rtk_nand_read_id;
	nand->read_ecc_page	= rtk_read_ecc_page;
	nand->write_ecc_page	= rtk_write_ecc_page;
#ifdef RTK_NAND_VERIFY
	nand->write_ecc_page_raw	= rtk_write_ecc_page_raw_mode;
#endif
	nand->erase_block	= rtk_erase_block;

	nand->RBA_PERCENT	= RBA_PERCENT;
	printk("[Default]RBA percentage: %d'%'\n", nand->RBA_PERCENT);

	return 0;
}

MODULE_AUTHOR("AlexChang <alexchang2131@realtek.com>");
MODULE_DESCRIPTION("Realtek NAND Flash Controller Driver");
