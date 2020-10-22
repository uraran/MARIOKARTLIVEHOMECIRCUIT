#include <common.h>
#include <asm/arch/system.h>
#include <asm/arch/fw_info.h>
#include <watchdog.h>
#include <asm/arch/extern_param.h>
#include <customized.h>
#include <crc.h>

#define BIT(x) (1 << (x))
#define SETBITS(x,y) ((x) |= (y))
#define CLEARBITS(x,y) ((x) &= (~(y)))
#define SETBIT(x,y) SETBITS((x), (BIT((y))))
#define CLEARBIT(x,y) CLEARBITS((x), (BIT((y))))

#define SWAPEND16(d)    ((BYTE(d,0) << 8) | (BYTE(d,1) << 0))

extern uint eMMC_fw_desc_table_start;
extern uchar boot_logo_enable;
extern uint custom_logo_src_width;
extern uint custom_logo_src_height;
extern uint custom_logo_dst_width;
extern uint custom_logo_dst_height;

#ifdef CONFIG_CUSTOMIZED_LOGO_TYPE1

#define MAX_NAME_LEN (16)
#define MAX_VER_LEN (48)
#define MAX_MAGIC_LEN (4)
#define HDR_MAGIC (0x55667788)
#define MD5_LEN (8) /* store 16 bytes md5sum into two 8 bytes parts */

typedef struct
{
	unsigned int magic;		/* 0x55667788 (for bootloader to find the kernel partition) */
	char name[MAX_NAME_LEN];	/* image name */
	char ver[MAX_VER_LEN];		/* image version */
	unsigned int len;		/* length of the data */
	unsigned char type;		/* 1: kernel image, 2: fs image, 3: common bineray file*/
					/* 4: common data file, 5: android ota.zip, 6: multiple combination */
	unsigned char flags;		/* whether write header to storage device, 0: no, 1: yes */
	unsigned char version;		/* header version */
	unsigned char algo_type;	/* encryption method, 0: not encrypted, 1: md5 */
	unsigned char md5sum1[MD5_LEN];	/* md5 sum part1 -- header and data */
	unsigned short offset;		/* offset of the data in the image */
	unsigned short crc;		/* 16-bit crc check sum -- data only */
	unsigned char md5sum2[MD5_LEN];	/* md5 sum part2 */
	unsigned short LOGO_SRC_WIDTH;	/* CUSTOM_LOGO_SRC_WIDTH: 1920 */
	unsigned short LOGO_SRC_HEIGHT;	/* CUSTOM_LOGO_SRC_HEIGHT: 1080*/
	unsigned short LOGO_DST_WIDTH;	/* CUSTOM_LOGO_DST_WIDTH: 1920 */
	unsigned short LOGO_DST_HEIGHT;	/* CUSTOM_LOGO_DST_HEIGHT: 1080 */
}logo_header;

#ifdef DUMMP_LOGO_HEADER
void dump_logo_header(logo_header header)
{
	int i = 0;
	printf("***** LOGO Image Header DUMP *****\n");
	printf("HEADER SIZE : %u\n", sizeof(logo_header));
	printf("magic : 0x%8x\n", SWAPEND32(header.magic));
	printf("name : %s\n", header.name);
	printf("ver : %s\n", header.ver);
	printf("len : 0x%08x\n", SWAPEND32(header.len));
	printf("type : %u\n", (unsigned int)header.type);
	printf("flags : %u\n", (unsigned int)header.flags);
	printf("version : %u\n", (unsigned int)header.version);
	printf("algo_type : %u\n", (unsigned int)header.algo_type);

	printf("md5sum1 : ");
	for(i = 0 ; i < MD5_LEN ; i++)
		printf("%02x",header.md5sum1[i]);
	printf("\n");

	printf("offset : 0x%08x\n", (unsigned int)SWAPEND16(header.offset));
	printf("crc : 0x%08x\n", (unsigned int)SWAPEND16(header.crc));

	printf("md5sum2 : ");
	for(i = 0 ; i < MD5_LEN ; i++)
		printf("%02x",header.md5sum2[i]);
	printf("\n");

	printf("LOGO_SRC_WIDTH : %u\n", (unsigned int)SWAPEND16(header.LOGO_SRC_WIDTH));
	printf("LOGO_SRC_HEIGHT : %u\n", (unsigned int)SWAPEND16(header.LOGO_SRC_HEIGHT));
	printf("LOGO_DST_WIDTH : %u\n", (unsigned int)SWAPEND16(header.LOGO_DST_WIDTH));
	printf("LOGO_DST_HEIGHT : %u\n", (unsigned int)SWAPEND16(header.LOGO_DST_HEIGHT));

	printf("*********************************\n");
}
#endif

static unsigned long  checksum(char *buf, int nbytes, unsigned long sum)
{
	int i;

	/* Checksum all the pairs of bytes first... */
	for (i = 0; i < (nbytes & ~1); i += 2) {

		sum += (unsigned short)SWAPEND16(*(unsigned short *)(buf + i));

		/* Add carry. */
		if (sum > (unsigned long)(0xFFFF))
			sum -= (unsigned long)(0xFFFF);
	}

	/* If there's a single byte left over, checksum it, too.   Network
	      byte order is big-endian, so the remaining byte is the high byte. */
	if (i < nbytes) {
		sum += ((unsigned long)(buf [i] << 8)) & ((unsigned long)(0xFF)<<8);
		/* Add carry. */
		if (sum > (unsigned long)(0xFFFF))
			sum -= (unsigned long)(0xFFFF);
	}

	return sum;
}

static unsigned long wrapsum(unsigned long sum)
{
    sum = ~sum & (unsigned long)(0xFFFF);
    return sum;
}


static int image_check_and_set(void* fw_entry, void* image_addr, boot_av_info_t* boot_av)
{
	fw_desc_entry_v1_t *this_entry = (fw_desc_entry_v1_t *)fw_entry;
	logo_header *img_header = (logo_header*)image_addr;
	unsigned logo_addr = (unsigned)image_addr + SWAPEND16(img_header->offset);
	unsigned short crc_check = 0;
	unsigned long sum = 0;

#ifdef DUMMP_LOGO_HEADER
	dump_logo_header(*img_header);
#endif
	if((SWAPEND32(img_header->len) + sizeof(logo_header)) > this_entry->length) {
		printf("Header length exceed IMAGE length!!\n");
		return -1;
	}

	if((logo_addr + SWAPEND32(img_header->len)) > (image_addr + this_entry->length)) {
		printf("Logo address final addr exceed IMAGE length!!\n");
		return -1;
	}

	sum = checksum ((char*)logo_addr, SWAPEND32(img_header->len), sum);
	sum = wrapsum(sum);
	crc_check = SWAPEND16((unsigned short)sum);

	if(crc_check != img_header->crc) {
		printf("CRC check fail, crc_check:0x%x, img_header->crc:0x%x\n", crc_check, img_header->crc);
		return -1;
	}

	// All check pass, start to fill boot_av data
	boot_av->dwMagicNumber = SWAPEND32(BOOT_AV_INFO_MAGICNO);
	boot_av-> logo_enable = boot_logo_enable;
	boot_av-> logo_addr = CPU_TO_BE32(logo_addr);
	boot_av-> src_width = (img_header->LOGO_SRC_WIDTH << 16);
	boot_av-> src_height = (img_header->LOGO_SRC_HEIGHT << 16);
	boot_av-> dst_width = (img_header->LOGO_DST_WIDTH << 16);
	boot_av-> dst_height = (img_header->LOGO_DST_HEIGHT << 16);

        return 0;
}

static int __rtk_plat_load_customize_logo_emmc(void* fw_entry, int fw_count, uchar version)
{
	fw_desc_entry_v1_t *this_entry;
	int i;
	uint unit_len;
	uint fw_checksum = 0;
	boot_av_info_t *boot_av;
	uint block_no;
	MEM_LAYOUT_WHEN_READ_IMAGE_T mem_layout;
	uint imageSize = 0;
	unsigned int secure_mode;
	int target_logo_fw = 0;

	unsigned logo_bitmask = 0; // bitmask for available logos

	secure_mode = rtk_get_secure_boot_type();
        /* find fw_entry structure according to version */
	switch (version) {
		case FW_DESC_TABLE_V1_T_VERSION_1:
			unit_len = sizeof(fw_desc_entry_v1_t);
			break;

		case FW_DESC_TABLE_V1_T_VERSION_11:
			unit_len = sizeof(fw_desc_entry_v11_t);
			break;

		case FW_DESC_TABLE_V1_T_VERSION_21:
			unit_len = sizeof(fw_desc_entry_v21_t);
			break;

		default:
			return -1;
	}

	boot_av = (boot_av_info_t *) MIPS_BOOT_AV_INFO_ADDR; // boot_av value should already been zero out by caller.
	// First step, scan through the fw table and find out how many LOGO image is available.
	for (i = 0; i < fw_count; i++) {

		this_entry = (fw_desc_entry_v1_t *)(fw_entry + unit_len * i);

		if (this_entry->target_addr) {

			switch(this_entry->type) {
				case FW_TYPE_IMAGE_FILE:
				case FW_TYPE_IMAGE_FILE1:
					SETBIT(logo_bitmask, this_entry->type);
				default:
					continue;
			}
		}
	}

	while(logo_bitmask) {
		// We found out all the Logo Image available, now Pick the best choice.
		target_logo_fw = fls(logo_bitmask) - 1; // decrease 1 since fls count from 1 ~ 32
		CLEARBIT(logo_bitmask, target_logo_fw);

		if(!target_logo_fw) {
			printf("No available IMAGE FW\n");
			return -1;
		}

		// Now go back to the FW entry that should be load
		for (i = 0; i < fw_count; i++) {
			this_entry = (fw_desc_entry_v1_t *)(fw_entry + unit_len * i);

			if(this_entry->type == target_logo_fw)
				break;
		}

		if(target_logo_fw == FW_TYPE_IMAGE_FILE)
			printf("IMAGE FILE:\n");
		else if(target_logo_fw == FW_TYPE_IMAGE_FILE1)
			printf("IMAGE FILE 1:\n");
#ifdef EMMC_BLOCK_LOG
		printf("\t FW Image to 0x%08x ~ 0x%08x, size=0x%08x, 0x%x blocks\n",
			this_entry->target_addr,
			this_entry->target_addr + this_entry->length,
			this_entry->length,
			(this_entry->length%EMMC_BLOCK_SIZE)?(this_entry->length/EMMC_BLOCK_SIZE)+1:(this_entry->length/EMMC_BLOCK_SIZE));
		printf("\t FW Image fr 0x%08x, blk 0x%x\n",
			eMMC_fw_desc_table_start + this_entry->offset,
			(eMMC_fw_desc_table_start + this_entry->offset)/EMMC_BLOCK_SIZE);
#else
		printf("\t FW Image to 0x%08x, size=0x%08x (0x%08x)\n", this_entry->target_addr, this_entry->length, this_entry->target_addr + this_entry->length);
		printf("\t FW Image fr 0x%08x \n", eMMC_fw_desc_table_start + this_entry->offset);
#endif
		WATCHDOG_KICK();

		/* get memory layout before copy other image */
		mem_layout.bIsEncrypted = 0;
		mem_layout.bIsCompressed = 0;
		mem_layout.image_target_addr = this_entry->target_addr & (~MIPS_KSEG_MSK);

		get_mem_layout_when_read_image(&mem_layout);

		imageSize = this_entry->length;

		// 512B aligned
		if (imageSize&(EMMC_BLOCK_SIZE-1)) {
			imageSize &= ~(EMMC_BLOCK_SIZE-1);
			imageSize += EMMC_BLOCK_SIZE;
		}

		block_no = (eMMC_fw_desc_table_start + this_entry->offset) / EMMC_BLOCK_SIZE ;

		if (!rtk_eMMC_read(block_no, imageSize, (uint *)mem_layout.flash_to_ram_addr)) {
			printf("[ERR] Read fw error (type:%d)!\n", this_entry->type);
			return -1;
		}

		/* Bypass FW table checksum since customer won't update image through OTA */

		if(!image_check_and_set(this_entry, (void*)mem_layout.flash_to_ram_addr, boot_av))
			break;
		else
			printf("IMAGE FW check FAIL!\n");
	}

	return target_logo_fw;
}

#endif //#CONFIG_CUSTOMIZED_LOGO_TYPE1

int rtk_plat_load_customize_logo_emmc(void* fw_entry, int fw_count,uchar version)
{
#ifdef CONFIG_CUSTOMIZED_LOGO_TYPE1
	return __rtk_plat_load_customize_logo_emmc(fw_entry, fw_count, version);
#else
	printf("No Customized LOGO available!!\n");
	return -1;
#endif
}
