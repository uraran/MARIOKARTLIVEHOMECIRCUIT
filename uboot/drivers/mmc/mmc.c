/*
 * Copyright 2008, Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Based vaguely on the Linux code
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <mmc.h>
#include <part.h>
#include <malloc.h>
#include <linux/list.h>
#include <div64.h>
#include "rtkmmc.h"

/* Set block count limit because of 16 bit register limit on some hardware*/
#ifndef CONFIG_SYS_MMC_MAX_BLK_COUNT
#define CONFIG_SYS_MMC_MAX_BLK_COUNT 65535
#endif
//#define MMC_DEBUG
//#define DISABLE_MICRON_AUTO_STANDBY
static struct list_head mmc_devices;
static int cur_dev_num = -1;
static int g_bMicronFlash=0;
unsigned char g_ext_csd[CSD_ARRAY_SIZE];

int __board_mmc_getcd(struct mmc *mmc) {
	return -1;
}

int board_mmc_getcd(struct mmc *mmc)__attribute__((weak,
	alias("__board_mmc_getcd")));

#ifdef CONFIG_MMC_BOUNCE_BUFFER
static int mmc_bounce_need_bounce(struct mmc_data *orig)
{
	ulong addr, len;

	if (orig->flags & MMC_DATA_READ)
		addr = (ulong)orig->dest;
	else
		addr = (ulong)orig->src;

	if (addr % ARCH_DMA_MINALIGN) {
		debug("MMC: Unaligned data destination address %08lx!\n", addr);
		return 1;
	}

	len = (ulong)(orig->blocksize * orig->blocks);
	if (len % ARCH_DMA_MINALIGN) {
		debug("MMC: Unaligned data destination length %08lx!\n", len);
		return 1;
	}

	return 0;
}

static int mmc_bounce_buffer_start(struct mmc_data *backup,
					struct mmc_data *orig)
{
	ulong origlen, len;
	void *buffer;

	if (!orig)
		return 0;

	if (!mmc_bounce_need_bounce(orig))
		return 0;

	memcpy(backup, orig, sizeof(struct mmc_data));

	origlen = orig->blocksize * orig->blocks;
	len = roundup(origlen, ARCH_DMA_MINALIGN);
	buffer = memalign(ARCH_DMA_MINALIGN, len);
	if (!buffer) {
		puts("MMC: Error allocating MMC bounce buffer!\n");
		return 1;
	}

	if (orig->flags & MMC_DATA_READ) {
		orig->dest = buffer;
	} else {
		memcpy(buffer, orig->src, origlen);
		orig->src = buffer;
	}

	return 0;
}

static void mmc_bounce_buffer_stop(struct mmc_data *backup,
					struct mmc_data *orig)
{
	ulong len;

	if (!orig)
		return;

	if (!mmc_bounce_need_bounce(backup))
		return;

	if (backup->flags & MMC_DATA_READ) {
		len = backup->blocksize * backup->blocks;
		memcpy(backup->dest, orig->dest, len);
		free(orig->dest);
		orig->dest = backup->dest;
	} else {
		free((void *)orig->src);
		orig->src = backup->src;
	}

	return;

}
#else
static inline int mmc_bounce_buffer_start(struct mmc_data *backup,
					struct mmc_data *orig) { return 0; }
static inline void mmc_bounce_buffer_stop(struct mmc_data *backup,
					struct mmc_data *orig) { }
#endif

//#define CONFIG_MMC_TRACE
int mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
	struct mmc_data backup;
	int ret;

	memset(&backup, 0, sizeof(backup));

	ret = mmc_bounce_buffer_start(&backup, data);
	if (ret)
		return ret;

#ifdef CONFIG_MMC_TRACE
	int i;
	u8 *ptr;

	printf("CMD_SEND:%d\n", cmd->cmdidx);
	printf("\t\tARG\t\t\t 0x%08X\n", cmd->cmdarg);
	printf("\t\tFLAG\t\t\t %d\n", cmd->flags);
	ret = mmc->send_cmd(mmc, cmd, data);
	switch (cmd->resp_type) {
		case MMC_RSP_NONE:
			printf("\t\tMMC_RSP_NONE\n");
			break;
		case MMC_RSP_R1:
			printf("\t\tMMC_RSP_R1,5,6,7 \t 0x%08X \n",
				cmd->response[0]);
			break;
		case MMC_RSP_R1b:
			printf("\t\tMMC_RSP_R1b\t\t 0x%08X \n",
				cmd->response[0]);
			break;
		case MMC_RSP_R2:
			printf("\t\tMMC_RSP_R2\t\t 0x%08X \n",
				cmd->response[0]);
			printf("\t\t          \t\t 0x%08X \n",
				cmd->response[1]);
			printf("\t\t          \t\t 0x%08X \n",
				cmd->response[2]);
			printf("\t\t          \t\t 0x%08X \n",
				cmd->response[3]);
			printf("\n");
			printf("\t\t\t\t\tDUMPING DATA\n");
			for (i = 0; i < 4; i++) {
				int j;
				printf("\t\t\t\t\t%03d - ", i*4);
				ptr = (u8 *)&cmd->response[i];
				ptr += 3;
				for (j = 0; j < 4; j++)
					printf("%02X ", *ptr--);
				printf("\n");
			}
			break;
		case MMC_RSP_R3:
			printf("\t\tMMC_RSP_R3,4\t\t 0x%08X \n",
				cmd->response[0]);
			break;
		default:
			printf("\t\tERROR MMC rsp not supported\n");
			break;
	}
#else
	ret = mmc->send_cmd(mmc, cmd, data);
#endif
	mmc_bounce_buffer_stop(&backup, data);
	return ret;
}

int mmc_send_status(struct mmc *mmc, int timeout)
{
	struct mmc_cmd cmd;
	int err, retries = 5;
#ifdef CONFIG_MMC_TRACE
	int status;
#endif

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	if (!mmc_host_is_spi(mmc))
		cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	do {
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (!err) {
			if ((cmd.response[0] & MMC_STATUS_RDY_FOR_DATA) &&
			    (cmd.response[0] & MMC_STATUS_CURR_STATE) !=
			     MMC_STATE_PRG)
				break;
			else if (cmd.response[0] & MMC_STATUS_MASK) {
				printf("Status Error: 0x%08X\n",
					cmd.response[0]);
				return COMM_ERR;
			}
		} else if (--retries < 0)
			return err;

		udelay(1000);

	} while (timeout--);

#ifdef CONFIG_MMC_TRACE
	status = (cmd.response[0] & MMC_STATUS_CURR_STATE) >> 9;
	printf("CURR STATE:%d\n", status);
#endif
	if (!timeout) {
		printf("Timeout waiting card ready\n");
		return TIMEOUT;
	}

	return 0;
}

int mmc_set_blocklen(struct mmc *mmc, int len)
{
	struct mmc_cmd cmd;

	cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = len;
	cmd.flags = 0;

	return mmc_send_cmd(mmc, &cmd, NULL);
}

struct mmc *find_mmc_device(int dev_num)
{
	struct mmc *m;
	struct list_head *entry;

	list_for_each(entry, &mmc_devices) {
		m = list_entry(entry, struct mmc, link);

		if (m->block_dev.dev == dev_num)
			return m;
	}

	printf("MMC Device %d not found\n", dev_num);

	return NULL;
}

static ulong mmc_erase_t(struct mmc *mmc, ulong start, lbaint_t blkcnt)
{
	struct mmc_cmd cmd;
	ulong end;
	int err, start_cmd, end_cmd;

	if (mmc->high_capacity)
		end = start + blkcnt - 1;
	else {
		end = (start + blkcnt - 1) * mmc->write_bl_len;
		start *= mmc->write_bl_len;
	}

	if (IS_SD(mmc)) {
		start_cmd = SD_CMD_ERASE_WR_BLK_START;
		end_cmd = SD_CMD_ERASE_WR_BLK_END;
	} else {
		start_cmd = MMC_CMD_ERASE_GROUP_START;
		end_cmd = MMC_CMD_ERASE_GROUP_END;
	}

	cmd.cmdidx = start_cmd;
	cmd.cmdarg = start;
	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		goto err_out;

	cmd.cmdidx = end_cmd;
	cmd.cmdarg = end;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		goto err_out;

	cmd.cmdidx = MMC_CMD_ERASE;
	cmd.cmdarg = SECURE_ERASE;
	cmd.resp_type = MMC_RSP_R1b;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		goto err_out;

	return 0;

err_out:
	puts("mmc erase failed\n");
	return err;
}

static unsigned long
mmc_berase(int dev_num, unsigned long start, lbaint_t blkcnt)
{
	int err = 0;
	struct mmc *mmc = find_mmc_device(dev_num);
	lbaint_t blk = 0, blk_r = 0;
	int timeout = 1000;

	if (!mmc)
		return -1;

	if ((start % mmc->erase_grp_size) || (blkcnt % mmc->erase_grp_size))
		printf("\n\nCaution! Your devices Erase group is 0x%x\n"
			"The erase range would be change to 0x%lx~0x%lx\n\n",
		       mmc->erase_grp_size, start & ~(mmc->erase_grp_size - 1),
		       ((start + blkcnt + mmc->erase_grp_size)
		       & ~(mmc->erase_grp_size - 1)) - 1);

	while (blk < blkcnt) {
		blk_r = ((blkcnt - blk) > mmc->erase_grp_size) ?
			mmc->erase_grp_size : (blkcnt - blk);
		err = mmc_erase_t(mmc, start + blk, blk_r);
		if (err)
			break;

		blk += blk_r;

		/* Waiting for the ready status */
		if (mmc_send_status(mmc, timeout))
			return 0;
	}

	return blk;
}

static ulong
mmc_write_blocks(struct mmc *mmc, ulong start, lbaint_t blkcnt, const void*src)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int timeout = 1000;
	int err=0, ret=0;
	ALLOC_CACHE_ALIGN_BUFFER(char, ext_csd, CSD_ARRAY_SIZE);

	if ((start + blkcnt) > mmc->block_dev.lba) {
		printf("MMC: block number 0x%lx exceeds max(0x%lx)\n",
			start + blkcnt, mmc->block_dev.lba);
		return 0;
	}

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_WRITE_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_WRITE_SINGLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * mmc->write_bl_len;

	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;

	data.src = src;
	data.blocks = blkcnt;
	data.blocksize = mmc->write_bl_len;
	data.flags = MMC_DATA_WRITE;

	if ((ret = mmc_send_cmd(mmc, &cmd, &data))>0) {
		printf("%s : mmc write failed: 0x%08x\n", __func__,ret);
		return 0;
	}
	else
	{
	    if(cmd.cmdidx != 0x10)
            {
		printf("[LY] %s : write blk fail, change to sdr mode\n",__func__);
		err = error_handling(cmd.cmdidx);
		if (err)
			return err;
	    }
	}

	/* SPI multiblock writes terminate using a special
	 * token, not a STOP_TRANSMISSION request.
	 */
	#ifndef CONFIG_SYS_RTK_EMMC_FLASH
	if (!mmc_host_is_spi(mmc) && blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.flags = 0;
		if (mmc_send_cmd(mmc, &cmd, NULL)) {
			printf("mmc fail to send stop cmd\n");
			return 0;
		}
	}
    #endif

	/* Waiting for the ready status */
	if (mmc_send_status(mmc, timeout))
		return 0;

	return blkcnt;
}

static ulong
mmc_bwrite(int dev_num, ulong start, lbaint_t blkcnt, const void*src)
{
	lbaint_t cur, blocks_todo = blkcnt;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
		return 0;

	if (mmc_set_blocklen(mmc, mmc->write_bl_len))
		return 0;

	do {
		cur = (blocks_todo > mmc->b_max) ?  mmc->b_max : blocks_todo;
		if(mmc_write_blocks(mmc, start, cur, src) != cur)
			return 0;
		blocks_todo -= cur;
		start += cur;
		src += cur * mmc->write_bl_len;
	} while (blocks_todo > 0);

	return blkcnt;
}

int mmc_read_blocks(struct mmc *mmc, void *dst, ulong start, lbaint_t blkcnt)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int err=0;
	ALLOC_CACHE_ALIGN_BUFFER(char, ext_csd, CSD_ARRAY_SIZE);

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * mmc->read_bl_len;

	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;

	data.dest = dst;
	data.blocks = blkcnt;
	data.blocksize = mmc->read_bl_len;
	data.flags = MMC_DATA_READ;

	if (mmc_send_cmd(mmc, &cmd, &data)>0)
		return 0;
	else
	{
	   if (cmd.cmdidx != 0x10)
	   {
		printf("[LY] %s : read blk fail, change to sdr mode\n",__func__);
		err = error_handling(cmd.cmdidx);
		if (err)
			return err;
           }
	}

    #ifndef CONFIG_SYS_RTK_EMMC_FLASH
	if (blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.flags = 0;
		if (mmc_send_cmd(mmc, &cmd, NULL)) {
			printf("mmc fail to send stop cmd\n");
			return 0;
		}
	}
    #endif

	return blkcnt;
}

static ulong mmc_bread(int dev_num, ulong start, lbaint_t blkcnt, void *dst)
{
	lbaint_t cur, blocks_todo = blkcnt;

	if (blkcnt == 0)
		return 0;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
		return 0;

	if ((start + blkcnt) > mmc->block_dev.lba) {
		printf("MMC: block number 0x%lx exceeds max(0x%lx)\n",
			start + blkcnt, mmc->block_dev.lba);
		return 0;
	}

	if (mmc_set_blocklen(mmc, mmc->read_bl_len))
		return 0;

	do {
		cur = (blocks_todo > mmc->b_max) ?  mmc->b_max : blocks_todo;
		if(mmc_read_blocks(mmc, dst, start, cur) != cur)
			return 0;
		blocks_todo -= cur;
		start += cur;
		dst += cur * mmc->read_bl_len;
	} while (blocks_todo > 0);

	return blkcnt;
}

int mmc_go_idle(struct mmc* mmc)
{
	struct mmc_cmd cmd;
	int err;

	udelay(1000);

	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_NONE;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	udelay(2000);

	return 0;
}

int
sd_send_op_cond(struct mmc *mmc)
{
	int timeout = 1000;
	int err;
	struct mmc_cmd cmd;

	do {
		cmd.cmdidx = MMC_CMD_APP_CMD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;

		/*
		 * Most cards do not answer if some reserved bits
		 * in the ocr are set. However, Some controller
		 * can set bit 7 (reserved for low voltages), but
		 * how to manage low voltages SD card is not yet
		 * specified.
		 */
		cmd.cmdarg = mmc_host_is_spi(mmc) ? 0 :
			(mmc->voltages & 0xff8000);

		if (mmc->version == SD_VERSION_2)
			cmd.cmdarg |= OCR_HCS;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		udelay(1000);
	} while ((!(cmd.response[0] & OCR_BUSY)) && timeout--);

	if (timeout <= 0)
		return UNUSABLE_ERR;

	if (mmc->version != SD_VERSION_2)
		mmc->version = SD_VERSION_1_0;

	if (mmc_host_is_spi(mmc)) { /* read OCR for spi */
		cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;
	}

	mmc->ocr = cmd.response[0];

	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 0;

	return 0;
}

int mmc_send_op_cond(struct mmc *mmc)
{
	int timeout = 10000;
	struct mmc_cmd cmd;
	int err;


	/* Some cards seem to need this */
	mmc_go_idle(mmc);

#if 0
 	/* Asking to the card its capabilities */
 	cmd.cmdidx = MMC_CMD_SEND_OP_COND;
 	cmd.resp_type = MMC_RSP_R3;
 	cmd.cmdarg = 0;
 	cmd.flags = 0;

 	err = mmc_send_cmd(mmc, &cmd, NULL);

 	if (err)
 		return err;
#endif
 	udelay(1000);

	do {
		cmd.cmdidx = MMC_CMD_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = (mmc_host_is_spi(mmc) ? 0 :
				(mmc->voltages &
				(cmd.response[0] & OCR_VOLTAGE_MASK)) |
				(cmd.response[0] & OCR_ACCESS_MODE));

		if (mmc->host_caps & MMC_MODE_HC)
			cmd.cmdarg |= OCR_HCS;

		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		udelay(1000);
		flush_cache((unsigned long)cmd.response[0], 8);
		#ifdef MMC_DEBUG
		printf("[LY] retry cmd1, cnt=%d\n",timeout);
		#endif
	} while (!(cmd.response[0] & OCR_BUSY) && timeout--);

	if (timeout <= 0)
		return UNUSABLE_ERR;

	if (mmc_host_is_spi(mmc)) { /* read OCR for spi */
		cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;
	}

	mmc->version = MMC_VERSION_UNKNOWN;
	mmc->ocr = cmd.response[0];

	//force byte mode in our platform
	#if 0
	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	#else
	mmc->high_capacity = 0;
	#endif
	mmc->rca = 0;

	return 0;
}


int mmc_send_ext_csd(struct mmc *mmc, char *ext_csd)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int err;

	/* Get the Card Status Register */
	cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	data.dest = ext_csd;
	data.blocks = 1;
	data.blocksize = 512;
	data.flags = MMC_DATA_READ;

	err = mmc_send_cmd(mmc, &cmd, &data);

    #ifdef MMC_DEBUG
	flush_cache((unsigned long)ext_csd, CSD_ARRAY_SIZE);
    	mmc_show_ext_csd(ext_csd);
    #endif

	return err;
}

int mmc_switch(struct mmc *mmc, u8 set, u8 index, u8 value, u8 bHS)
{
	struct mmc_cmd cmd;
	int timeout = 1000;
	int ret;

	if ((index == EXT_CSD_BUS_WIDTH)&&(bHS))
	{
		#ifdef MMC_DEBUG
		printf("change card bus width at hs : %d ->",value);
		#endif
		value = (value == BUS_WIDTH_8) ?
                                EXT_CSD_DDR_BUS_WIDTH_8 : EXT_CSD_DDR_BUS_WIDTH_4;
		#ifdef MMC_DEBUG
		printf("%d\n",value);
		#endif
	}

	cmd.cmdidx = MMC_CMD_SWITCH;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
				 (index << 16) |
				 (value << 8)|set;
	cmd.flags = 0;

	ret = mmc_send_cmd(mmc, &cmd, NULL);

	/* Waiting for the ready status */
	if (!ret)
		ret = mmc_send_status(mmc, timeout);

	return ret;

}

int mmc_get_card_caps(struct mmc *mmc, char *crd_ext_csd)
{
	char cardtype;
	int err=0;

	cardtype = crd_ext_csd[EXT_CSD_CARD_TYPE] & 0xff;

	#ifdef MMC_DEBUG
	printf("[LY] cardtype=%02x\n",cardtype);
	#endif

	if (cardtype == 0)
	{
		mmc->card_caps = 0;
		printf("cardtype is empty, set sdr/ddr as default\n");
		mmc->card_caps |= MMC_MODE_HS;
		mmc->card_caps |= MMC_MODE_HS_52MHz;
		mmc->card_caps |= MMC_MODE_HSDDR_52MHz;
		if (!((((mmc->cid[0] >> 24)&0xff)== MANU_ID_MICRON1) || (((mmc->cid[0] >> 24)&0xff)== MANU_ID_MICRON2)))
			mmc->card_caps |= MMC_MODE_HS200;
	}
	else
	{
		/* High Speed is set, there are two types: 52MHz and 26MHz */
		mmc->card_caps = 0;
		if (cardtype & MMC_HS_26MHZ)
			mmc->card_caps |= MMC_MODE_HS;
		if (cardtype & MMC_HS_52MHZ)
			mmc->card_caps |= MMC_MODE_HS_52MHz;
		if (cardtype & MMC_HS_DDR_1_8V_52MHZ)
			mmc->card_caps |= MMC_MODE_HSDDR_52MHz;
		if (cardtype & MMC_HS_200_1_8V_52MHZ)
			mmc->card_caps |= MMC_MODE_HS200;
	}
	printf("[LY] cardtype=%02x, mmc->card_caps=%02x\n",cardtype,mmc->card_caps);
	return 0;
}

int mmc_getset_pad(struct mmc *mmc, unsigned int flag)
{
	mmc_set_ios(mmc,flag);
	return 0;
}

int mmc_change_ldo(struct mmc *mmc, unsigned int freq)
{
	mmc->ldo = freq;
	mmc_set_ios(mmc,MMC_IOS_LDO);
	return 0;
}

int mmc_change_freq(struct mmc *mmc,unsigned int mode,unsigned int chg_type)
{
	ALLOC_CACHE_ALIGN_BUFFER(char, ext_csd,  CSD_ARRAY_SIZE*2);
	char cardtype;
	int err=0;
	extern unsigned int gCurrentBootMode;

	if (mmc_host_is_spi(mmc))
		return -1;

	/* Only version 4 supports high-speed */
	if (mmc->version < MMC_VERSION_4)
		return -1;

	switch(mode)
	{
		case MODE_SD20:
			gCurrentBootMode = MODE_SD20;
			if (chg_type & CHANGE_FREQ_CARD)
				err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, (mode>>2)+1, 0); //sdr20 is 1
			if (chg_type & CHANGE_FREQ_HOST)
				mmc_set_mode_select(mmc,MMC_MODE_HS_52MHz);
			sync();
			break;
		case MODE_DDR:
			gCurrentBootMode = MODE_DDR;
			if (chg_type & CHANGE_FREQ_CARD)
				err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, mode>>2, 0);
			if (chg_type & CHANGE_FREQ_HOST)
				mmc_set_mode_select(mmc,MMC_MODE_HSDDR_52MHz);
			sync();
			break;
		case MODE_SD30:
			gCurrentBootMode = MODE_SD30;
			if (chg_type & CHANGE_FREQ_CARD)
				err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, mode>>2, 0);
			if (chg_type & CHANGE_FREQ_HOST)
				mmc_set_mode_select(mmc,MMC_MODE_HS200);
			sync();
			break;
	}

	if (err)
		return err;
	return 0;
}

int mmc_switch_part(int dev_num, unsigned int part_num)
{
	struct mmc *mmc = find_mmc_device(dev_num);

	if (!mmc)
		return -1;

	return mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CONF,
			  (mmc->part_config & ~PART_ACCESS_MASK)
			  | (part_num & PART_ACCESS_MASK),0);
}

int mmc_getcd(struct mmc *mmc)
{
	int cd;

	cd = board_mmc_getcd(mmc);

	if ((cd < 0) && mmc->getcd)
		cd = mmc->getcd(mmc);

	return cd;
}

int sd_switch(struct mmc *mmc, int mode, int group, u8 value, u8 *resp)
{
	struct mmc_cmd cmd;
	struct mmc_data data;

	/* Switch the frequency */
	cmd.cmdidx = SD_CMD_SWITCH_FUNC;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = (mode << 31) | 0xffffff;
	cmd.cmdarg &= ~(0xf << (group * 4));
	cmd.cmdarg |= value << (group * 4);
	cmd.flags = 0;

	data.dest = (char *)resp;
	data.blocksize = 64;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	return mmc_send_cmd(mmc, &cmd, &data);
}


int sd_change_freq(struct mmc *mmc)
{
	int err;
	struct mmc_cmd cmd;
	ALLOC_CACHE_ALIGN_BUFFER(uint, scr, 2);
	ALLOC_CACHE_ALIGN_BUFFER(uint, switch_status, 16);
	struct mmc_data data;
	int timeout;

	mmc->card_caps = 0;

	if (mmc_host_is_spi(mmc))
		return 0;

	/* Read the SCR to find out if this card supports higher speeds */
	cmd.cmdidx = MMC_CMD_APP_CMD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	cmd.cmdidx = SD_CMD_APP_SEND_SCR;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	timeout = 3;

retry_scr:
	data.dest = (char *)scr;
	data.blocksize = 8;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	err = mmc_send_cmd(mmc, &cmd, &data);

	if (err) {
		if (timeout--)
			goto retry_scr;

		return err;
	}

	mmc->scr[0] = __be32_to_cpu(scr[0]);
	mmc->scr[1] = __be32_to_cpu(scr[1]);

	switch ((mmc->scr[0] >> 24) & 0xf) {
		case 0:
			mmc->version = SD_VERSION_1_0;
			break;
		case 1:
			mmc->version = SD_VERSION_1_10;
			break;
		case 2:
			mmc->version = SD_VERSION_2;
			break;
		default:
			mmc->version = SD_VERSION_1_0;
			break;
	}

	if (mmc->scr[0] & SD_DATA_4BIT)
		mmc->card_caps |= MMC_MODE_4BIT;

	/* Version 1.0 doesn't support switching */
	if (mmc->version == SD_VERSION_1_0)
		return 0;

	timeout = 4;
	while (timeout--) {
		err = sd_switch(mmc, SD_SWITCH_CHECK, 0, 1,
				(u8 *)switch_status);

		if (err)
			return err;

		/* The high-speed function is busy.  Try again */
		if (!(__be32_to_cpu(switch_status[7]) & SD_HIGHSPEED_BUSY))
			break;
	}

	/* If high-speed isn't supported, we return */
	if (!(__be32_to_cpu(switch_status[3]) & SD_HIGHSPEED_SUPPORTED))
		return 0;

	/*
	 * If the host doesn't support SD_HIGHSPEED, do not switch card to
	 * HIGHSPEED mode even if the card support SD_HIGHSPPED.
	 * This can avoid furthur problem when the card runs in different
	 * mode between the host.
	 */
	if (!((mmc->host_caps & MMC_MODE_HS_52MHz) &&
		(mmc->host_caps & MMC_MODE_HS)))
		return 0;

	err = sd_switch(mmc, SD_SWITCH_SWITCH, 0, 1, (u8 *)switch_status);

	if (err)
		return err;

	if ((__be32_to_cpu(switch_status[4]) & 0x0f000000) == 0x01000000)
		mmc->card_caps |= MMC_MODE_HS;

	return 0;
}

/* frequency bases */
/* divided by 10 to be nice to platforms without floating point */
static const int fbase[] = {
	10000,
	100000,
	1000000,
	10000000,
};

/* Multiplier values for TRAN_SPEED.  Multiplied by 10 to be nice
 * to platforms without floating point.
 */
static const int multipliers[] = {
	0,	/* reserved */
	10,
	12,
	13,
	15,
	20,
	25,
	30,
	35,
	40,
	45,
	50,
	55,
	60,
	70,
	80,
};

void mmc_set_ios(struct mmc *mmc, unsigned int caps)
{
	mmc->set_ios(mmc,caps);
}

void mmc_set_clock(struct mmc *mmc, uint clock)
{
	if (clock > mmc->f_max)
		clock = mmc->f_max;

	if (clock < mmc->f_min)
		clock = mmc->f_min;

	mmc->clock = clock;

	mmc_set_ios(mmc,MMC_IOS_CLK);
}

void mmc_set_bus_width(struct mmc *mmc, uint width)
{
	mmc->bus_width = width;

	mmc_set_ios(mmc,MMC_IOS_BUSWIDTH);
}

void mmc_set_mode_select(struct mmc *mmc, uint mode)
{
	mmc->mode_sel= mode;

	mmc_set_ios(mmc,MMC_IOS_CLK);
}

int mmc_startup(struct mmc *mmc)
{
	int err, width, hs_width=0, bSetHS=0;
	uint mult=0, freq=0;
	volatile u64 cmult=0, csize=0, capacity=0;
	struct mmc_cmd cmd;
	volatile uint vsn=0;
	volatile unsigned int * resp = NULL;
	static volatile int cmd_retry=0,cmd_retry1=0;
	volatile uint cid_val=0;

	struct mmc_data data;

	ALLOC_CACHE_ALIGN_BUFFER(char, outBlk, CSD_ARRAY_SIZE);
	ALLOC_CACHE_ALIGN_BUFFER(char, rcvBlk, CSD_ARRAY_SIZE);
	ALLOC_CACHE_ALIGN_BUFFER(char, outTwoBlk, CSD_ARRAY_SIZE*2);
	ALLOC_CACHE_ALIGN_BUFFER(char, ext_csd, CSD_ARRAY_SIZE);
	ALLOC_CACHE_ALIGN_BUFFER(char, test_csd, CSD_ARRAY_SIZE);
	int timeout = 1000;

#ifdef CONFIG_MMC_SPI_CRC_ON
	if (mmc_host_is_spi(mmc)) { /* enable CRC check for spi */
		cmd.cmdidx = MMC_CMD_SPI_CRC_ON_OFF;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 1;
		cmd.flags = 0;
		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;
	}
#endif

	//set initial speed
	mmc_change_ldo(mmc, 0x57); //50Mhz
	mmccr_set_speed(2);        //no wrapper divider

CMD_RETRY:
	/* Put the Card in Identify Mode */
	cmd.cmdidx = mmc_host_is_spi(mmc) ? MMC_CMD_SEND_CID :
		MMC_CMD_ALL_SEND_CID; /* cmd not supported in spi */
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
	{
		if (cmd_retry1++ > MAX_CMD_RETRY_COUNT)
			return err;
		cmd_retry=0;
		while (cmd_retry++ < MAX_CMD_RETRY_COUNT)
		{
               		err = mmc_send_op_cond(mmc);
                	if (err) {
                        	printf("Card did not respond to voltage select!\n");
				continue;
                	}
			else
				break;
		}
		if (cmd_retry >= MAX_CMD_RETRY_COUNT)
			return err;
		else
		{
			goto CMD_RETRY;
		}
	}

	memcpy(mmc->cid, cmd.response, 16);
    	flush_cache((unsigned long)mmc->cid, sizeof(unsigned int)*4);

	cid_val = (mmc->cid[0]>>24)&0xff;
#ifdef MMC_DEBUG
	printf("[LY] cid[0]=0x%02x\n",mmc->cid[0]>>24);
#endif
#ifdef DISABLE_MICRON_AUTO_STANDBY
	if ((((mmc->cid[0] >> 24)&0xff)== MANU_ID_MICRON1) || (((mmc->cid[0] >> 24)&0xff)== MANU_ID_MICRON2))
	{
		g_bMicronFlash = 1;
		//try sdr first
		mmc_change_ldo(mmc, 0x57); //50Mhz
		mmccr_set_speed(2);        //no wrapper divider

		cmd_retry=0;cmd_retry1=0;
		memset(outBlk, 0x00, 512);
		memset(outTwoBlk, 0x00, 1024);
		memset(rcvBlk, 0x00, 512);
		outBlk[0] = 0x84;
		outTwoBlk[0] = 0x08;

		MPRINTF("[LY] disable procedure start -->\n");
		//step 1
		/* Reset the Card */
		err = mmc_go_idle(mmc);
		MPRINTF("[LY] <--- cmd0 sent(%d)--->\n",err);
		if (err)
			return err;

		//step 2.1
		cmd.cmdidx = MMC_CMD_MICRON_63;
		cmd.resp_type = MMC_RSP_NONE;
		cmd.cmdarg = 0x50485349;
		cmd.flags = 0;
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err)
			return err;
		MPRINTF("[LY] <---cmd63 sent--->\n");
		//step 2.2
		cmd.cmdidx = MMC_CMD_MICRON_63;
		cmd.resp_type = MMC_RSP_NONE;
		cmd.cmdarg = 0x50485353;
		cmd.flags = 0;
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err)
			return err;
		MPRINTF("[LY] <---cmd63 sent--->\n");

		//step 3
		/* Reset the Card */
		sync();
		err = mmc_go_idle(mmc);
		MPRINTF("[LY] <---cmd0 sent--->\n");

		//step 4
		cmd.cmdidx = MMC_CMD_MICRON_63;
		cmd.resp_type = MMC_RSP_NONE;
		cmd.cmdarg = 0x50485343;
		cmd.flags = 0;
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err)
			return err;
		MPRINTF("[LY] <--- cmd63 sent--->\n");

		//step 5
		//cmd 0/1
		sync();
		err = mmc_send_op_cond(mmc);
		if (err) {
			printf("Card did not respond to voltage select!\n");
			return UNUSABLE_ERR;
		}

        	/* Put the Card in Identify Mode */
		//cmd 2
        	cmd.cmdidx = mmc_host_is_spi(mmc) ? MMC_CMD_SEND_CID :
                	MMC_CMD_ALL_SEND_CID; /* cmd not supported in spi */
        	cmd.resp_type = MMC_RSP_R2;
        	cmd.cmdarg = 0;
        	cmd.flags = 0;
        	err = mmc_send_cmd(mmc, &cmd, NULL);
        	if (err)
                	return err;
		MPRINTF("[LY] <---cmd 0/1/2 sent--->\n");

		//step 6
		cmd.cmdidx = MMC_CMD_MICRON_62;
		cmd.resp_type = MMC_RSP_NONE;
		cmd.cmdarg = 0x5048534D;
		cmd.flags = 0;
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err)
			return err;
		MPRINTF("[LY] <---cmd62 sent--->\n");
		sync();

		//step 7
		cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
		cmd.cmdarg = mmc->rca << 16;
		cmd.resp_type = MMC_RSP_R6;
		cmd.flags = 0;
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err)
			return err;
		MPRINTF("[LY] <--- cmd3 sent --->\n");
		sync();

		//step 8
		cmd.cmdidx = MMC_CMD_SELECT_CARD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = mmc->rca << 16;
		cmd.flags = 0;
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err)
			return err;
		MPRINTF("[LY] <--- cmd7 sent--->\n");
		sync();

		mmc_set_ios(mmc,MMC_IOS_NONE_DIV);
		udelay(1000);
		sync();
#if 1
  #if 1
		err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1, 0);
        	if (err)
		{
			printf("[LY] card : switch to hs fail\n");
        		return err;
		}
  #endif
		mmc_getset_pad(mmc, MMC_IOS_GET_PAD_DRV);
		udelay(1000);
		sync();
        	err = mmc_Select_SDR50_Push_Sample();
        	if (err)
		{
			printf("[LY] host : switch to hs fail\n");
                	return err;
		}
		mmc_getset_pad(mmc, MMC_IOS_RESTORE_PAD_DRV);
		udelay(1000);
		sync();
#endif
		//step 9 (send blk)
		//speed up, disable ip divider
		cmd.cmdidx = MMC_CMD_MICRON_60;
		cmd.cmdarg = 0x0;   //??
		cmd.resp_type = MMC_RSP_R1;
		cmd.flags = 0;

		data.src = outBlk;
		data.blocks = 1;
		data.blocksize = 0x200;
		data.flags = MMC_DATA_WRITE;
		MPRINTF("[LY] write blk size = %08x\n",data.blocksize);

		if (mmc_send_cmd(mmc, &cmd, &data)>0) {
			printf("mmc write failed,cmd60 fail, send stop\n");
#if 0
			cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
			cmd.cmdarg = 0;
			cmd.resp_type = MMC_RSP_R1b;
			cmd.flags = 0;
			if (mmc_send_cmd(mmc, &cmd, NULL)) {
				printf("mmc fail to send stop cmd\n");
				goto REINIT;
			}
#endif
			goto REINIT;
		}
		MPRINTF("[LY] <--- cmd60 sent --->\n");
		sync();

		//step 10 (recv blk)
		cmd.cmdidx = MMC_CMD_MICRON_61;
		cmd.cmdarg = 0x0;   //??
		cmd.resp_type = MMC_RSP_R1;
		cmd.flags = 0;

		data.src = rcvBlk;
		data.blocks = 1;
		data.blocksize = 0x200;
		data.flags = MMC_DATA_READ;
		MPRINTF("[LY] read blk size = %08x\n",data.blocksize);

		if (mmc_send_cmd(mmc, &cmd, &data)>0) {
			printf("mmc read failed,cmd61 fail\n");
			cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
			cmd.cmdarg = 0;
			cmd.resp_type = MMC_RSP_R1b;
			cmd.flags = 0;
			if (mmc_send_cmd(mmc, &cmd, NULL)) {
				printf("mmc fail to send stop cmd\n");
				goto REINIT;
			}
			goto REINIT;
		}
		MPRINTF("[LY] <--- cmd61 sent --->\n");
		sync();

		//step 11
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.flags = 0;
		if (mmc_send_cmd(mmc, &cmd, NULL)) {
			printf("mmc fail to send stop cmd\n");
			goto REINIT;
		}
		MPRINTF("[LY] <--- cmd12 sent --->\n");
		sync();

		/* Waiting for the ready status */
		if (mmc_send_status(mmc, 1000))
		{
			printf("[LY] send status fail\n");
			goto REINIT;
		}
		MPRINTF("[LY] <--- cmd13 sent --->\n");
		sync();

		//step 12 (send 2 blk)
		memcpy(outTwoBlk+512, outBlk, 512);
		outTwoBlk[512+58] = 0xff;
		sync();
		flush_cache(outTwoBlk, CSD_ARRAY_SIZE*2);
		cmd.cmdidx = MMC_CMD_MICRON_60;
		cmd.cmdarg = 0x0;   //??
		cmd.resp_type = MMC_RSP_R1;
		cmd.flags = 0;

		data.src = outTwoBlk;
		data.blocks = 2;
		data.blocksize = 0x200*2;
		data.flags = MMC_DATA_WRITE;
		MPRINTF("[LY] write 2 blk size = %08x\n",data.blocksize);

		if (mmc_send_cmd(mmc, &cmd, &data)>0) {
			printf("mmc write failed\n");
			goto REINIT;
		}
		MPRINTF("[LY] <--- cmd60 sent--->\n");
		MPRINTF("[LY] disable procedure done ------->\n");
		sync();
		printf("Standby : disable micron standby mode\n");

REINIT:
		//back to original startup path
		//cmd 0/1
		mmc_set_ios(mmc,MMC_IOS_INIT_DIV);
		udelay(1000);
		sync();
		err = mmc_send_op_cond(mmc);
		if (err) {
			printf("Card did not respond to voltage select!\n");
			return UNUSABLE_ERR;
		}

        	/* Put the Card in Identify Mode */

        	cmd.cmdidx = mmc_host_is_spi(mmc) ? MMC_CMD_SEND_CID :
                	MMC_CMD_ALL_SEND_CID; /* cmd not supported in spi */
        	cmd.resp_type = MMC_RSP_R2;
        	cmd.cmdarg = 0;
        	cmd.flags = 0;
        	err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err) 
			return err;
	}
#endif
 
	/*
	 * For MMC cards, set the Relative Address.
	 * For SD cards, get the Relatvie Address.
	 * This also puts the cards into Standby State
	 */
	if (!mmc_host_is_spi(mmc)) { /* cmd not supported in spi */
		cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
		cmd.cmdarg = mmc->rca << 16;
		cmd.resp_type = MMC_RSP_R6;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		if (IS_SD(mmc))
			mmc->rca = (cmd.response[0] >> 16) & 0xffff;
	}


	/* Get the Card-Specific Data */
	cmd.cmdidx = MMC_CMD_SEND_CSD;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	/* Waiting for the ready status */
	mmc_send_status(mmc, timeout);

	if (err)
		return err;

    mmc->csd[0] = cmd.response[0];
    mmc->csd[1] = cmd.response[1];
    mmc->csd[2] = cmd.response[2];
    mmc->csd[3] = cmd.response[3];

    #ifdef MMC_DEBUG
    mmc_show_csd(mmc);
    mmc_decode_cid(mmc);
    #endif

    flush_cache((unsigned long)cmd.response, sizeof(unsigned int)*4);
    flush_cache((unsigned long)mmc->csd, sizeof(unsigned int)*4);

	printf("mmc->version=0x%08x\n", mmc->version);
	if (mmc->version == MMC_VERSION_UNKNOWN) {
		int version = (cmd.response[0] >> 26) & 0xf;

		printf("version=0x%08x\n", version);
		switch (version) {
			case 0:
				mmc->version = MMC_VERSION_1_2;
				break;
			case 1:
				mmc->version = MMC_VERSION_1_4;
				break;
			case 2:
				mmc->version = MMC_VERSION_2_2;
				break;
			case 3:
				mmc->version = MMC_VERSION_3;
				break;
			case 4:
				mmc->version = MMC_VERSION_4;
				break;
			default:
				mmc->version = MMC_VERSION_1_2;
				break;
		}
	}

	/* divide frequency by 10, since the mults are 10x bigger */
    freq = fbase[(cmd.response[0] & 0x7)];
    mult = multipliers[((cmd.response[0] >> 3) & 0xf)];

    mmc->tran_speed = (u64)(freq * mult);
    #ifdef MMC_DEBUG
    printf("0 [LY] freq=0x%08x, mult=0x%08x,mmc->tran_speed=%lld\n",freq,mult,mmc->tran_speed);
    #endif
    mmc->read_bl_len = 1 << ((cmd.response[1] >> 16) & 0xf);

	if (IS_SD(mmc))
		mmc->write_bl_len = mmc->read_bl_len;
	else
		mmc->write_bl_len = 1 << ((cmd.response[3] >> 22) & 0xf);

	if (mmc->high_capacity) {
		csize = (mmc->csd[1] & 0x3f) << 16
			| (mmc->csd[2] & 0xffff0000) >> 16;
		cmult = 8;
    		#ifdef MMC_DEBUG
		printf("1 [LY] csize=0x%lld, cmult=0x%lld\n",csize,cmult);
		#endif
	} else {
		csize = (mmc->csd[1] & 0x3ff) << 2
			| (mmc->csd[2] & 0xc0000000) >> 30;
		cmult = (mmc->csd[2] & 0x00038000) >> 15;
    		#ifdef MMC_DEBUG
		printf("1.1 [LY] csize=%lld, cmult=%lld\n",csize,cmult);
		#endif
	}

    	#ifdef MMC_DEBUG
	printf("2 [LY] read_bl_len=0x%08x, cmd.rsp[1]=0x%08x\n",mmc->read_bl_len,(cmd.response[1]>>16)&0xf);
	#endif
	mmc->capacity = (u64)((csize + 1) << (cmult + 2));
	mmc->capacity = ((u64)(mmc->capacity)) * ((u64)(mmc->read_bl_len));
    	#ifdef MMC_DEBUG
	printf("3 [LY] mmc->cap=%lld\n",mmc->capacity);
	#endif
	if (mmc->read_bl_len > 512)
		mmc->read_bl_len = 512;

	if (mmc->write_bl_len > 512)
		mmc->write_bl_len = 512;

	/* Select the card, and put it into Transfer Mode */
	if (!mmc_host_is_spi(mmc)) { /* cmd not supported in spi */
		cmd.cmdidx = MMC_CMD_SELECT_CARD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = mmc->rca << 16;
		cmd.flags = 0;
		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;
	}

	//8b first
	err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_BUS_WIDTH, BUS_WIDTH_8, 0);
	if (err){
		printf("8b switch fail ! \n");
		return -1;
	}
	else{
		printf("8b switch ok ! \n");
	}
	mmc_set_bus_width(mmc, 8);
	//err = mmc_change_freq(mmc, MODE_DDR, CHANGE_FREQ_CARD);
	err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1, 0);
    if (err)
	{
		printf("[LY] switch to hs fail\n");
        	return err;
	}
 	err = mmc_Select_SDR50_Push_Sample();
        if (err)
	{
		printf("[LY] sdr50 k fail\n");
        	return err;
	}
	/*
	 * For SD, its erase group is always one sector
	 */
	mmc->erase_grp_size = 1;
	mmc->part_config = MMCPART_NOAVAILABLE;
	if (!IS_SD(mmc) && (mmc->version >= MMC_VERSION_4)) {
		/* check ext_csd version and capacity */
		flush_cache((unsigned long)ext_csd, CSD_ARRAY_SIZE);
		err = mmc_send_ext_csd(mmc, ext_csd);
		flush_cache((unsigned long)ext_csd, CSD_ARRAY_SIZE);
		if (err)
			return -1;
		else
		{
			flush_cache((unsigned long)g_ext_csd, CSD_ARRAY_SIZE);
			memcpy(g_ext_csd,ext_csd, CSD_ARRAY_SIZE);
			flush_cache((unsigned long)g_ext_csd, CSD_ARRAY_SIZE);
		}

#ifdef MMC_DEBUG
	printf("[LY] ext_csd[EXT_CSD_REV] = 0x%08x, mmc->version=%08x\n", ext_csd[EXT_CSD_REV], mmc->version);
#endif
        if (!err & (ext_csd[EXT_CSD_REV] >= 2)) {
			/*
			 * According to the JEDEC Standard, the value of
			 * ext_csd's capacity is valid if the value is more
			 * than 2GB
			 */
			capacity = ext_csd[EXT_CSD_SEC_CNT] << 0
					| ext_csd[EXT_CSD_SEC_CNT + 1] << 8
					| ext_csd[EXT_CSD_SEC_CNT + 2] << 16
					| ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
			capacity *= 512;
			if ((capacity >> 20) > 2 * 1024)
				mmc->capacity = capacity;
#ifdef MMC_DEBUG
			printf("5 [LY] mmc->cap=%lld, cap=%lld\n",mmc->capacity,capacity);
#endif
		}

		/*
		 * Check whether GROUP_DEF is set, if yes, read out
		 * group size from ext_csd directly, or calculate
		 * the group size from the csd value.
		 */
		if (ext_csd[EXT_CSD_ERASE_GROUP_DEF])
			mmc->erase_grp_size =
			      ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 512 * 1024;
		else {
			int erase_gsz, erase_gmul;
			erase_gsz = (mmc->csd[2] & 0x00007c00) >> 10;
			erase_gmul = (mmc->csd[2] & 0x000003e0) >> 5;
			mmc->erase_grp_size = (erase_gsz + 1)
				* (erase_gmul + 1);
		}

		/* store the partition info of emmc */
		if (ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & PART_SUPPORT)
			mmc->part_config = ext_csd[EXT_CSD_PART_CONF];
	}
	else
		printf("[LY] mmc->version < 4\n");
	
	if (IS_SD(mmc))
		err = sd_change_freq(mmc);
	else
		err = mmc_get_card_caps(mmc,ext_csd);

	if(err > 0)
		return err;

	/* Restrict card's capabilities by what the host can do */
	mmc->card_caps &= mmc->host_caps;

	if (IS_SD(mmc)) {
		if (mmc->card_caps & MMC_MODE_4BIT) {
			cmd.cmdidx = MMC_CMD_APP_CMD;
			cmd.resp_type = MMC_RSP_R1;
			cmd.cmdarg = mmc->rca << 16;
			cmd.flags = 0;

			err = mmc_send_cmd(mmc, &cmd, NULL);
			if (err)
				return err;

			cmd.cmdidx = SD_CMD_APP_SET_BUS_WIDTH;
			cmd.resp_type = MMC_RSP_R1;
			cmd.cmdarg = 2;
			cmd.flags = 0;
			err = mmc_send_cmd(mmc, &cmd, NULL);
			if (err)
				return err;

			mmc_set_bus_width(mmc, 4);
		}

		if (mmc->card_caps & MMC_MODE_HS)
			mmc->tran_speed = 50000000;
		else
			mmc->tran_speed = 25000000;
	}
 
	mmc_set_clock(mmc, mmc->tran_speed);
	mmc->boot_caps |= MMC_MODE_HS | MMC_MODE_HS_52MHz;

#ifdef MMC_DEBUG
	printf("[LY] bootcaps = %08x\n", mmc->boot_caps);
	printf("[LY] hostcaps= %08x\n", mmc->host_caps);
	printf("[LY] cardcaps = %08x\n", mmc->card_caps);
	printf("[LY] freq = %08x, clk diver = %08x\n", REG32(PLL_EMMC3),REG32(CR_CKGEN_CTL));
#endif	

	//get original pad value
	//mmc_getset_pad(mmc, MMC_IOS_GET_PAD_DRV);
//for qa b/d, we don't support HS200/DDR50
#if 0
	//if (g_bMicronFlash == 0)
	if (1)
	{
	  err = mmc_select_hs200(mmc,ext_csd);
	  printf("[LY] hs200 : %d\n", err);
	  if (err)
	  {
		if ((err != -4)||(err != -5)||(err != -6))
		{
		   err = mmc_select_ddr50(mmc,ext_csd);
		   printf("[LY] ddr50 : %d\n", err);
		}
		if (err)
		{
			err = mmc_select_sdr50(mmc,ext_csd);
			printf("[LY] sdr50 : %d\n", err);
		}
	  }
	}
	else
	{
	  err = mmc_select_ddr50(mmc,ext_csd);
	  printf("[LY] ddr50 : %d\n", err);
	  if (err)
	    {
		err = mmc_select_sdr50(mmc,ext_csd);
		printf("[LY] sdr50 : %d\n", err);
	    }
	}
#else
	printf("[LY] TBD : to support DDR50/HS200\n");
	err = mmc_select_sdr50(mmc,ext_csd);
	printf("[LY] sdr50 : %d\n", err);
#endif
	/* fill in device description */
	mmc->block_dev.lun = 0;
	mmc->block_dev.type = 0;
	mmc->block_dev.blksz = mmc->read_bl_len;
	mmc->block_dev.lba = lldiv(mmc->capacity, mmc->read_bl_len);
	sprintf(mmc->block_dev.vendor, "Man %06x Snr %08x", mmc->cid[0] >> 8,
			(mmc->cid[2] << 8) | (mmc->cid[3] >> 24));
	sprintf(mmc->block_dev.product, "%c%c%c%c%c", mmc->cid[0] & 0xff,
			(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
			(mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff);
	sprintf(mmc->block_dev.revision, "%d.%d", mmc->cid[2] >> 28,
			(mmc->cid[2] >> 24) & 0xf);
	init_part(&mmc->block_dev);

	return 0;
}

int mmc_select_sdr50(struct mmc *mmc,char* ext_csd)
{
	extern unsigned int gCurrentBootMode;
	volatile int width=0;
	volatile int err=0, ret=0;
	ALLOC_CACHE_ALIGN_BUFFER(char, test_csd, CSD_ARRAY_SIZE*2);

		gCurrentBootMode = MODE_SD20;
		//try sdr first
		mmc_change_ldo(mmc, 0x57); //50Mhz
		mmccr_set_speed(0);        //no wrapper divider
		
		//restore original pad value
		//mmc_getset_pad(mmc, MMC_IOS_RESTORE_PAD_DRV);

		err = mmc_change_freq(mmc, MODE_SD20, CHANGE_FREQ_HOST);
		width = BUS_WIDTH_8;
		//let card in better quality channel
		err = mmc_change_freq(mmc, MODE_DDR, CHANGE_FREQ_CARD);
		if (err)
		{
			printf("switch MODE_DDR fail !\n");
			return -7;
		}
		sync();
		for (; width >= 0; width--) {
			printf("[LY] SDR bus width=%d\n", width);
			sync();
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH, width, 0);
			sync();
			if (err)
			{
				if (width == BUS_WIDTH_1)
					return -4;
				else
					continue;
			}

			if (!width)
				mmc_set_bus_width(mmc, 1);
			else
				mmc_set_bus_width(mmc, 4 * width);

			sync();
			err = mmc_change_freq(mmc, MODE_DDR, CHANGE_FREQ_CARD);
			sync();
			if (err)
			{
				if (width == BUS_WIDTH_1)
					return -5;
				else
					continue;
			}
			sync();
			err = mmc_change_freq(mmc, MODE_SD20, CHANGE_FREQ_HOST);
			sync();
			if (err)
			{
				if (width == BUS_WIDTH_1)
					return -6;
				else
					continue;
			}

			//get the correct sample/push point at SDR mode
			err = mmc_Select_SDR50_Push_Sample();
			if (err)
			{
				if (width == BUS_WIDTH_1)
				{
					printf("[LY] sdr50 tuning fail(%d)\n",width);
					return err;
				}
				else
					continue;
			}

			flush_cache((unsigned long)test_csd, CSD_ARRAY_SIZE);
			err = mmc_send_ext_csd(mmc, test_csd);
			#ifdef MMC_DEBUG
    			mmc_show_ext_csd(test_csd);
			#endif

			if (!err && ext_csd[EXT_CSD_PARTITIONING_SUPPORT] \
				    == test_csd[EXT_CSD_PARTITIONING_SUPPORT]
				 && ext_csd[EXT_CSD_ERASE_GROUP_DEF] \
				    == test_csd[EXT_CSD_ERASE_GROUP_DEF] \
				 && ext_csd[EXT_CSD_REV] \
				    == test_csd[EXT_CSD_REV]
				 && ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] \
				    == test_csd[EXT_CSD_HC_ERASE_GRP_SIZE]
				 && memcmp(&ext_csd[EXT_CSD_SEC_CNT], \
					&test_csd[EXT_CSD_SEC_CNT], 4) == 0) {

				mmc->card_caps |= width;
				mmc->boot_caps |= (width << MMC_MODE_WIDTH_BITS_SHIFT);
				printf("[LY] mmc->boot_caps = %02x\n",mmc->boot_caps);
				ret=0;
				break;
			}
			else {
				if (!width) {
					printf("MMC: set bus width 1 fail\n");
				}
				else {
					printf("MMC: set bus width %d fail, try other mode\n", (4 * width));
				}
				ret=-2;
			}
		}

		if (mmc->card_caps & MMC_MODE_HS) {
			if (mmc->card_caps & MMC_MODE_HS_52MHz)
				mmc->tran_speed = 52000000;
			else
				mmc->tran_speed = 26000000;
		}

	return ret;
}

int mmc_select_ddr50(struct mmc *mmc,char* ext_csd)
{
	volatile int width=0;
	volatile int err=0, ret=0;

	ALLOC_CACHE_ALIGN_BUFFER(char, test_csd, CSD_ARRAY_SIZE*2);

	// DDR50
	if (!IS_SD(mmc)&&((mmc->host_caps & MMC_MODE_HSDDR_52MHz) == (mmc->card_caps & MMC_MODE_HSDDR_52MHz)))
	{
		//restore original pad value
		//mmc_getset_pad(mmc, MMC_IOS_RESTORE_PAD_DRV);

		//let card in better quality channel
		err = mmc_change_freq(mmc, MODE_DDR, CHANGE_FREQ_CARD);
		if (err)
		{
			return -7;
		}
		udelay(1000);
		sync();
		//try ddr50, 8/4 bits
		printf("[LY] speed up emmc at DDR50 \n");
		width = BUS_WIDTH_8;
		for (; width > 0; width--) {
			printf("[LY] DDR bus width=%d\n", width);
#if 0
			sync();
			err = mmc_change_freq(mmc, MODE_DDR, CHANGE_FREQ_CARD);
			if (err)
			{
				if (width == BUS_WIDTH_4)
					return -4;
				continue;
			}
#endif
			sync();
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH, width, 1);
			udelay(1000);
			sync();
			if (err)
			{
				if (width == BUS_WIDTH_4)
					return -6;
				continue;
			}

			sync();
			mmc_set_bus_width(mmc, 4 * width);
			sync();
			err = mmc_change_freq(mmc, MODE_DDR, CHANGE_FREQ_HOST);
			udelay(1000);
			sync();
			if (err)
			{
				if (width == BUS_WIDTH_4)
					return -5;
				continue;
			}
			
			//get the correct sample/push point at DDR mode
			err = mmc_Tuning_DDR50();
			if (err)
			{
				if (width == BUS_WIDTH_4)
				{
					printf("[LY] ddr50 tuning fail(%d)\n",width);
					return err;
				}
			}
			else
			{
		
				flush_cache((unsigned long)test_csd, CSD_ARRAY_SIZE);

				err = mmc_send_ext_csd(mmc, test_csd);
				flush_cache((unsigned long)test_csd, CSD_ARRAY_SIZE);

				if (!err && ext_csd[EXT_CSD_PARTITIONING_SUPPORT] \
				    == test_csd[EXT_CSD_PARTITIONING_SUPPORT]
				 && ext_csd[EXT_CSD_ERASE_GROUP_DEF] \
				    == test_csd[EXT_CSD_ERASE_GROUP_DEF] \
				 && ext_csd[EXT_CSD_REV] \
				    == test_csd[EXT_CSD_REV]
				 && ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] \
				    == test_csd[EXT_CSD_HC_ERASE_GRP_SIZE]
				 && memcmp(&ext_csd[EXT_CSD_SEC_CNT], \
					&test_csd[EXT_CSD_SEC_CNT], 4) == 0) {

					mmc->card_caps |= width;
					mmc->boot_caps |= (width << MMC_MODE_WIDTH_BITS_SHIFT);
					mmc->boot_caps |= MMC_MODE_HSDDR_52MHz;
					printf("[LY] mmc->boot_caps = %02x\n",mmc->boot_caps);
					ret = 0;
					break;
				}
				else {
					if (!width) {
						printf("MMC: set ddr50 bus width 1 fail\n");
					}
					else {
						printf("MMC: set ddr50 bus width %d fail, try other mode\n", (4 * width));
					}
					ret = -2;
				}
			}
		}
	}
	else
		return -1;

	//ddr50 at 50Mhz
	mmc->tran_speed = 52000000;
	return ret;
}

int mmc_select_hs200(struct mmc *mmc,char* ext_csd)
{
	volatile int width=0;
	volatile int err=0,ret=0;
	ALLOC_CACHE_ALIGN_BUFFER(char, test_csd, CSD_ARRAY_SIZE*2);


	// HS-200
	if (!IS_SD(mmc)&&((mmc->host_caps & MMC_MODE_HS200) == (mmc->card_caps & MMC_MODE_HS200)))
	{
		//set max pad value
		//mmc_getset_pad(mmc, MMC_IOS_SET_PAD_DRV);

		//try hs-200, 8/4 bits
		printf("[LY] speed up emmc at HS-200 \n");
		width = BUS_WIDTH_8;
		//let card in better quality channel
		err = mmc_change_freq(mmc, MODE_DDR, CHANGE_FREQ_CARD);
		if (err)
		{
			return -7;
		}
		sync();
		for (; width > 0; width--) {
			printf("[LY] HS-200 bus width=%d\n", width);
			sync();
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH, width, 0);
			sync();
			if (err)
			{
				//get the correct sample/push point at SDR mode
				printf("[LY] hs200 card bus switch retry result = %d\n", err);
				if (width == BUS_WIDTH_4)
					return -6;
				continue;
			}
			sync();
			mmc_set_bus_width(mmc, 4 * width);
			sync();

			err = mmc_change_freq(mmc, MODE_SD30, CHANGE_FREQ_CARD);
			if (err)
			{
				printf("[LY] hs200 card bus switch retry result = %d\n", err);
				if (width == BUS_WIDTH_4)
					return -5;
				continue;
			}
			sync();
			err = mmc_change_freq(mmc, MODE_SD30, CHANGE_FREQ_HOST);
			if (err)
			{
				//get the correct sample/push point at SDR mode
				err = mmc_Select_SDR50_Push_Sample();
				printf("[LY] hs200 speed switch retry result = %d\n", err);
				if (width == BUS_WIDTH_4)
					return -4;
				continue;
			}
			
			//get the correct sample/push point at DDR mode
			err = mmc_Tuning_HS200();
			if (err)
			{
				if (width == BUS_WIDTH_4)
				{
					printf("[LY] hs200 tuning fail(%d)\n",width);
					return err;
				}
			}
			else
			{

			flush_cache((unsigned long)test_csd, CSD_ARRAY_SIZE);
			err = mmc_send_ext_csd(mmc, test_csd);
			flush_cache((unsigned long)test_csd, CSD_ARRAY_SIZE);

			if (!err && ext_csd[EXT_CSD_PARTITIONING_SUPPORT] \
				    == test_csd[EXT_CSD_PARTITIONING_SUPPORT]
				 && ext_csd[EXT_CSD_ERASE_GROUP_DEF] \
				    == test_csd[EXT_CSD_ERASE_GROUP_DEF] \
				 && ext_csd[EXT_CSD_REV] \
				    == test_csd[EXT_CSD_REV]
				 && ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] \
				    == test_csd[EXT_CSD_HC_ERASE_GRP_SIZE]
				 && memcmp(&ext_csd[EXT_CSD_SEC_CNT], \
					&test_csd[EXT_CSD_SEC_CNT], 4) == 0) {

				mmc->card_caps |= width;
				mmc->boot_caps |= (width << MMC_MODE_WIDTH_BITS_SHIFT);
				mmc->boot_caps |= MMC_MODE_HS200;
				printf("[LY] mmc->boot_caps = %02x\n",mmc->boot_caps);
				ret = 0;
				break;
			}
			else {
				if (!width) {
					printf("MMC: set hs200 bus width 1 fail\n");
				}
				else {
					printf("MMC: set hs200 bus width %d fail, try other mode\n", (4 * width));
				}
				ret = -2;
			}
			}
		}

	}
	else
		return -1;

	//hs200 at 100Mhz
	mmc->tran_speed = 100000000;
	return ret;
}

int mmc_send_if_cond(struct mmc *mmc)
{
        struct mmc_cmd cmd;
        int err;

	cmd.cmdidx = SD_CMD_SEND_IF_COND;
	/* We set the bit if the host supports voltages between 2.7 and 3.6 V */
	cmd.cmdarg = ((mmc->voltages & 0xff8000) != 0) << 8 | 0xaa;
	cmd.resp_type = MMC_RSP_R7;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	if ((cmd.response[0] & 0xff) != 0xaa)
		return UNUSABLE_ERR;
	else
		mmc->version = SD_VERSION_2;

	return 0;
}

int mmc_register(struct mmc *mmc)
{
	/* Setup the universal parts of the block interface just once */
	mmc->block_dev.if_type = IF_TYPE_MMC;
	mmc->block_dev.dev = cur_dev_num++;
	mmc->block_dev.removable = 1;
	mmc->block_dev.block_read = mmc_bread;
	mmc->block_dev.block_write = mmc_bwrite;
	mmc->block_dev.block_erase = mmc_berase;
	if (!mmc->b_max)
		mmc->b_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;

	INIT_LIST_HEAD (&mmc->link);

	list_add_tail (&mmc->link, &mmc_devices);

	return 0;
}

#ifdef CONFIG_PARTITIONS
block_dev_desc_t *mmc_get_dev(int dev)
{
	struct mmc *mmc = find_mmc_device(dev);
	if (!mmc)
		return NULL;

	mmc_init(mmc);
	return &mmc->block_dev;
}
#endif

int mmc_init(struct mmc *mmc)
{
	int err;
	static int cmd_retry=0;

	if (mmc_getcd(mmc) == 0) {
		mmc->has_init = 0;
		printf("MMC: no card present\n");
		return NO_CARD_ERR;
	}

	if (mmc->has_init)
		return 0;

	err = mmc->init(mmc);

	if (err)
		return err;

	mmc_set_bus_width(mmc, 1);
	mmc_set_clock(mmc, 1);
	mmc_set_mode_select(mmc, 0);

#if defined(CONFIG_RTD299X) || defined(CONFIG_RTD1195) // just decrease the initialized time
	mmc->part_num = 0;
	err = TIMEOUT;
#else
	/* run standard uboot emmc driver flow */
	printf("MMC: Test for SD version\n");

	/* Reset the Card */
	err = mmc_go_idle(mmc);

	if (err)
		return err;

	/* The internal partition reset to user partition(0) at every CMD0*/
	mmc->part_num = 0;

	/* Test for SD version 2 */
	err = mmc_send_if_cond(mmc);

	/* Now try to get the SD card's operating condition */
	err = sd_send_op_cond(mmc);
#endif
	/* If the command timed out, we check for an MMC card */
	if (err == TIMEOUT) {
CMD0_RETRY:
		err = mmc_send_op_cond(mmc);

		if (err) {
			printf("Card did not respond to voltage select!\n");
			if (cmd_retry++ < MAX_CMD_RETRY_COUNT)
				goto CMD0_RETRY;
			else
				return UNUSABLE_ERR;
		}
	}

	err = mmc_startup(mmc);
	printf("exit from mmc_startup(),err=%d\n",err);
	if (err)
		mmc->has_init = 0;
	else
		mmc->has_init = 1;
#if defined(CONFIG_RTD299X) || defined(CONFIG_RTD1195)
	mmc_data_sync(mmc);
	// sync data struct between standard mmc driver and rtk emmc diiver
	//extern e_device_type emmc_card;
	//emmc_card.rca = mmc->rca;
	//emmc_card.sector_addressing = mmc->high_capacity ? 1 : 0;
#endif
	return err;
}

/*
 * CPU and board-specific MMC initializations.  Aliased function
 * signals caller to move on
 */
static int __def_mmc_init(bd_t *bis)
{
	return -1;
}

int cpu_mmc_init(bd_t *bis) __attribute__((weak, alias("__def_mmc_init")));
int board_mmc_init(bd_t *bis) __attribute__((weak, alias("__def_mmc_init")));

void print_mmc_devices(char separator)
{
	struct mmc *m;
	struct list_head *entry;

	list_for_each(entry, &mmc_devices) {
		m = list_entry(entry, struct mmc, link);

		printf("%s: %d", m->name, m->block_dev.dev);

		if (entry->next != &mmc_devices)
			printf("%c ", separator);
	}

	printf("\n");
}

int get_mmc_num(void)
{
	return cur_dev_num;
}

int mmc_initialize(bd_t *bis)
{
	INIT_LIST_HEAD (&mmc_devices);
	cur_dev_num = 0;

	if (board_mmc_init(bis) < 0)
		cpu_mmc_init(bis);

	print_mmc_devices(',');

	return 0;
}
