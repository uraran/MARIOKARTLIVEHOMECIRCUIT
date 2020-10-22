#include <common.h>
#include <exports.h>
#include <asm/arch/se_storage.h>
#include <asm/arch/system.h>
#include <fw_info.h>
#include <tar.h>
#include <nand.h>
#include <rtkspi.h>

#define ENABLE_SE_STORAGE_CONSOLE_MODE
//#define ENABLE_SE_STORAGE_DEBUG

#define SE_PRINTF(fmt, args...)	printf(fmt, ##args)

#ifdef	ENABLE_SE_STORAGE_DEBUG
#define SE_DEBUG(fmt, args...)		printf(fmt, ##args)
#else
#define SE_DEBUG(fmt, args...)
#endif

#ifndef CONFIG_SE_STORAGE_START
#define CONFIG_SE_STORAGE_START	0x2200000	/* For eMMC */
#endif
#ifndef CONFIG_SE_STORAGE_SIZE
#define CONFIG_SE_STORAGE_SIZE		0x400000	/* For eMMC */
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int storage_dirty = 0;
static uchar *storage_buffer = NULL;
static uchar *storage_buffer2 = NULL;
static int storage_tarsize = 0;

extern BOOT_FLASH_T boot_flash_type;
extern unsigned int tar_read_print_info;
extern unsigned char tar_read_print_prefix[32];

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int se_storage_check_sanity_from_eMMC(uchar *buffer, int length)
{
#ifdef CONFIG_SYS_RTK_EMMC_FLASH

	uint se_storage_header_bytes = 512;
	uint blk_start, blk_cnt = 0;
	int n = 0;
	int ret = 0;
	posix_header *p_start = NULL;
	posix_header *p_end = NULL;
	char se_storage_header[] = SE_STORAGE_HEADER_FILE_NAME;
	unsigned int pp_tarsize;
	int i = 0;

 	if (buffer == NULL) {
		SE_PRINTF("[SE_S][ERR] se_storage buffer is NULL\n");
		return -1;
 	}

	/* reset se_storage data in ddr */
	memset(buffer, 0, length);

	p_start = (posix_header *)malloc(se_storage_header_bytes);

	{
		/* Read se_storage header */
		blk_start = CONFIG_SE_STORAGE_START / EMMC_BLOCK_SIZE;
		blk_cnt = se_storage_header_bytes / EMMC_BLOCK_SIZE;
		n = rtk_eMMC_read(blk_start, se_storage_header_bytes, (unsigned int *)p_start);
		if (n != blk_cnt)
		{
			/* Read error */
			SE_PRINTF("[SE_S][ERR] Get se_storage header from eMMC failed\n");
			ret = -1;
		}
		else
		{
			/* Read success */
			SE_DEBUG("[SE_S] Get se_storage header from eMMC\n");
		}

#ifdef ENABLE_SE_STORAGE_DEBUG
		SE_DEBUG("[SE_S] ******** CHECK SE_STORAGE HEADER********\n");

		if (!ret) {
			SE_DEBUG("[SE_S] -- PASS: read emmc\n");
		}
		else {
			SE_DEBUG("[SE_S] -- FAIL: read emmc\n");
			continue;
		}

		if (!strcmp(p_start, se_storage_header)) {
			SE_DEBUG("[SE_S] -- PASS: se_storage header\n");
		}
		else {
			SE_DEBUG("[SE_S] -- FAIL: se_storage header\n");
			continue;
		}

		if (tar_check_header(p_start) == 0) {
			SE_DEBUG("[SE_S] -- PASS: tar header\n");
		}
		else {
			SE_DEBUG("[SE_S] -- FAIL: tar header\n");
			continue;
		}

		if (p_start->rtk_tarsize >= 256) {
			SE_DEBUG("[SE_S] -- PASS: tar size >= 256\n");
		}
		else {
			SE_DEBUG("[SE_S] -- FAIL: tar size < 256\n");
			continue;
		}

		if (p_start->rtk_tarsize < (CONFIG_SE_STORAGE_SIZE - sizeof(posix_header))) {
			SE_DEBUG("[SE_S] -- PASS: tar size 0x%08x\n", p_start->rtk_tarsize);
		}
		else {
			SE_DEBUG("[SE_S] -- FAIL: tar size 0x%08x > 0x%08x\n", p_start->rtk_tarsize, CONFIG_SE_STORAGE_SIZE - sizeof(posix_header));
			continue;
		}

		SE_DEBUG("[SE_S] ******** CHECK SE_STORAGE HEADER [%d] ALL PASS\n", i);
		SE_DEBUG("[SE_S] Need to check the end header\n");
#endif
        pp_tarsize = p_start->rtk_tarsize;
	}

	free(p_start);

	blk_start = CONFIG_SE_STORAGE_START / EMMC_BLOCK_SIZE;
	n = rtk_eMMC_read(blk_start, pp_tarsize, (unsigned int *)buffer);

	return pp_tarsize;

#else /* !CONFIG_SYS_RTK_EMMC_FLASH */

	SE_DEBUG("[SE_S][WARN] CONFIG_SYS_RTK_EMMC_FLASH is undefined\n");
	return 0;

#endif
}

static int se_storage_get_from_eMMC(uchar *buffer)
{
	int ret = 0;

	ret = se_storage_check_sanity_from_eMMC(buffer, CONFIG_SE_STORAGE_SIZE);

	return ret;
}

static int se_storage_save_to_eMMC(uchar* buffer, int len)
{
#ifdef CONFIG_SYS_RTK_EMMC_FLASH

	posix_header *pp_start;
	uint blk_start, blk_cnt, n, blk_cnt_erase;
	uint total_len;

 	if (buffer == NULL) {
		return -1;
 	}

	/* fill the first header as RTK header */
	pp_start = (posix_header *)buffer;
	pp_start->rtk_signature[0] = 'R';
	pp_start->rtk_signature[1] = 'T';
	pp_start->rtk_signature[2] = 'K';
	pp_start->rtk_signature[3] = 0;
	pp_start->rtk_tarsize = (unsigned int)len;
	pp_start->rtk_seqnum = 0;
	tar_fill_checksum((char *)pp_start);

	/* the last block will be reserved to save serial no , and magic no */
	/* can't exceed 2M size - (1 block size)  */
	if (len > (CONFIG_SE_STORAGE_SIZE - EMMC_BLOCK_SIZE) ) {
		SE_DEBUG("[SE_S][ERR] se_storage_save_to_eMMC: too big\n");
		return 1;
	}

	total_len = len + sizeof(posix_header);

	blk_start = CONFIG_SE_STORAGE_START / EMMC_BLOCK_SIZE;
	blk_cnt = ALIGN(total_len , EMMC_BLOCK_SIZE) / EMMC_BLOCK_SIZE;
    blk_cnt_erase = ALIGN(CONFIG_SE_STORAGE_SIZE , EMMC_BLOCK_SIZE) / EMMC_BLOCK_SIZE;
    
    memset(storage_buffer2, 0, CONFIG_SE_STORAGE_SIZE);
    rtk_eMMC_write(blk_start, CONFIG_SE_STORAGE_SIZE, storage_buffer2);
	n = rtk_eMMC_write(blk_start, total_len, (uint *)buffer);
	if (n != blk_cnt)
	{
		/* Write error */
		SE_PRINTF("[SE_S][ERR] Save to eMMC FAILED\n");
		return 1;
	}
	else
	{
		/* Write success */
		SE_PRINTF("[SE_S] Save to eMMC (blk#:0x%x, buf:0x%08x, len:0x%x)\n",
			blk_start, (uint)buffer, total_len);
	}

#endif

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

int se_storage_dump_info(void)
{

	SE_PRINTF("****** dump se_storage info ******\n");
	SE_PRINTF("boot_flash_type    = ");
	switch(boot_flash_type){
		case BOOT_NOR_SERIAL:
			SE_PRINTF("NOR\n");
			break;

		case BOOT_NAND:
			SE_PRINTF("NAND\n");
			break;

		case BOOT_EMMC:
			SE_PRINTF("MMC\n");
			break;

		default:
			SE_PRINTF("UNKNOWN\n");
			break;
	}

	SE_PRINTF("se_storage_buffer     = 0x%08x\n", (uint)storage_buffer);
	SE_PRINTF("se_storage_buffer2    = 0x%08x\n", (uint)storage_buffer2);
	SE_PRINTF("se_storage_tarsize    = 0x%08x\n", storage_tarsize);

	return 0;
}

int se_storage_dump_list(void)
{
	char *dst_addr;
	uint dst_length;

	tar_read_print_info = 1;

	memset(tar_read_print_prefix, 0, sizeof(tar_read_print_prefix));

	strcpy((char *)tar_read_print_prefix, SE_STORAGE_HEADER_FILE_NAME);

	tar_read("DUMMY_NAME_FOR_DUMP", (char *)SE_STORAGE_DATA_ADDR, CONFIG_SE_STORAGE_SIZE, &dst_addr, &dst_length);

	tar_read_print_info = 0;
    
	return 0;
}

int se_storage_reset(void)
{
	int i = 0;
	posix_header *p_start = NULL;

 	if (storage_buffer == NULL) {
		SE_PRINTF("[SE_S] se_storage_reset: storage_buffer is NULL\n");
		return -1;
 	}

	/* reset tar size */
	storage_tarsize = sizeof(posix_header);

	memset(storage_buffer, 0, CONFIG_SE_STORAGE_SIZE);

	/* build RTK header */
	p_start = (posix_header *)storage_buffer;
	tar_build_header((char *)p_start, SE_STORAGE_HEADER_FILE_NAME, 0, '5');
	p_start->rtk_signature[0] = 'R';
	p_start->rtk_signature[1] = 'T';
	p_start->rtk_signature[2] = 'K';
	p_start->rtk_signature[3] = 0;
	p_start->rtk_tarsize = sizeof(posix_header);
	p_start->rtk_seqnum = 0;
	tar_fill_checksum((char *)p_start);
	tar_check_header((char *)p_start);

	// save to storage data
	storage_dirty = 1;

	se_storage_save();
	return 0;
}

int se_storage_delete(char *path)
{
 	if (storage_buffer == NULL) {
		SE_PRINTF("[SE_S] se_storage_delete: storage_buffer is NULL\n");
		return -1;
 	}

	memset(storage_buffer2, 0, CONFIG_SE_STORAGE_SIZE);
	storage_tarsize = tar_add_or_delete((char *)storage_buffer, path, 0, 0, 
                                       (char *)storage_buffer2, CONFIG_SE_STORAGE_SIZE, 0);
	storage_dirty = 1;

	return 0;
}


/*
 * Return Value:
 *    0: success
 *    1: tar read failed
 *   -1: se_storage buffer error
 */
int se_storage_read(char *path, char**buffer, int *length)
{
 	if (storage_buffer == NULL) {
		SE_PRINTF("[SE_S] se_storage_read: storage_buffer is NULL\n");
		return -1;
 	}

	return tar_read(path, (char *)storage_buffer, CONFIG_SE_STORAGE_SIZE, buffer, (uint *)length) == 0;
}

int se_storage_write(char *path, char *buffer, int length)
{
	int length_512 = ((length + 511)/512) * 512;

 	if (storage_buffer == NULL) {
		SE_PRINTF("[SE_S] se_storage_write: storage_buffer is NULL\n");
		return -1;
 	}

	if (length_512 + storage_tarsize + 512 > CONFIG_SE_STORAGE_SIZE) {
		SE_PRINTF("[SE_S] se_storage_write: too big\n");
		return -1;
	}

	memset(storage_buffer2, 0 , CONFIG_SE_STORAGE_SIZE);

	if ((storage_tarsize = tar_add_or_delete((char *)storage_buffer, path, buffer, 
                                   length, (char *)storage_buffer2, CONFIG_SE_STORAGE_SIZE, 1)) == 0){
		SE_PRINTF("[SE_S] se_storage_write: tar_add_or_delete fail\n");
		return -1;
	}
	SE_DEBUG("se_storage_tarsize = 0x%x\n", storage_tarsize);

	storage_dirty = 1;

	return 0;
}

int se_storage_save(void)
{
	int ret = 0;

	SE_PRINTF("[SE_S] se_storage_save: ");

 	if (storage_buffer == NULL) {
		SE_PRINTF("FAILED (storage_buffer is NULL)\n");
		return -1;
 	}

	if (storage_dirty && storage_tarsize > 0) {

		switch(boot_flash_type){
			case BOOT_NOR_SERIAL:
				SE_PRINTF("NOR\n");
				break;

			case BOOT_NAND:
				SE_PRINTF("NAND\n");
				break;

			case BOOT_EMMC:
				SE_PRINTF("MMC\n");
				ret = se_storage_save_to_eMMC(storage_buffer, storage_tarsize);
				break;

			default:
				SE_PRINTF("UNKNOWN\n");
				ret = -1;
				break;
		}
	}
	else {
		SE_PRINTF("no change\n");
		SE_DEBUG("[SE_S] storage_dirty is %d\n", storage_dirty);
	}

	storage_dirty = 0;

	return 0;
}

int se_storage_init(void)
{
	int ret = 0;

	storage_buffer = (uchar *)SE_STORAGE_DATA_ADDR;
    storage_buffer2 = (uchar *)SE_STORAGE_DATA_ADDR + CONFIG_SE_STORAGE_SIZE;
	switch(boot_flash_type){
		case BOOT_NOR_SERIAL:
			SE_PRINTF("NOR\n");
			break;

		case BOOT_NAND:
			SE_PRINTF("NAND\n");
			break;

		case BOOT_EMMC:
			SE_PRINTF("MMC\n");
			SE_DEBUG("[SE_S] storage_buffer  = 0x%x\n", storage_buffer);
            SE_DEBUG("[SE_S] storage_buffer2  = 0x%x\n", storage_buffer2);
			storage_tarsize = se_storage_get_from_eMMC(storage_buffer);
			SE_DEBUG("[SE_S] storage_tarsize = 0x%x\n", storage_tarsize);
			break;

		default:
			SE_PRINTF("UNKNOWN\n");
			ret = -1;
			break;
	}

	return 0;
}

#ifdef ENABLE_SE_STORAGE_CONSOLE_MODE
int do_se_storage(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *buffer;
	unsigned long dst_buffer;//for rtice tool se_storage read
	int length;
	int rc;

	if (argc < 2) {
		return CMD_RET_USAGE;
	}
	else {
		if (strcmp(argv[1], "save") == 0) {
			se_storage_save();
		}
		else if (strcmp(argv[1], "reset") == 0) {
			se_storage_reset();
		}
		else if (strcmp(argv[1], "info") == 0) {
			se_storage_dump_info();
		}
		else if (strcmp(argv[1], "list") == 0) {
			se_storage_dump_list();
		}
		else if (strcmp(argv[1], "delete") == 0) {
			se_storage_delete(argv[2]);
		}
		else if (strcmp(argv[1], "read") == 0) {
			rc = se_storage_read(argv[2], &buffer, &length);

			if (rc)
				SE_PRINTF("%s: file not found\n", argv[2]);
			else
				SE_PRINTF("%s: %d bytes at 0x%08x\n", argv[2], length, (uint)buffer);

			if (argc == 4) {
				dst_buffer = simple_strtoul(argv[3], NULL, 16);
				memcpy((void *)dst_buffer, (void *)buffer, length);
				SE_PRINTF("src[0x%08x] dst[0x%08x]\n", (uint)buffer, (uint)dst_buffer);
			}
		}
		else if (strcmp(argv[1], "write") == 0) {

			buffer = (char *)simple_strtoul(argv[3], NULL, 16);
			length = (int)simple_strtoul(argv[4], NULL, 16);

			se_storage_write(argv[2], buffer, length);
		}
		else {
			return CMD_RET_USAGE;
		}
	}

	return 0;
}


U_BOOT_CMD(
	se_storage,	5,	1,	do_se_storage,
	"SE_STORAGE sub system",
	"read path\n"
	"se_storage write path addr length\n"
	"se_storage delete path\n"
	"se_storage save\n"
	"se_storage reset\n"
	"se_storage info\n"
	"se_storage list"
);
#endif

