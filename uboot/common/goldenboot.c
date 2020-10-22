/*************************************************
 *
 * Realtek GoldenBoot 2 stage bootcode
 *
 * GoldenBoot is designed to load 2nd stage bootcode
 * at same ADDR as current bootcode(1st bootcode).
 * All Jumper-code text/data is placed in different
 * address according to u-boot.lds in "golden_section"
 * depends on each board type. For example :
 * rtd1195 -> <TOP>/board/realtek/qa/u-boot.lds
 *
 ************************************************/

#include <goldenboot.h>
#include <environment.h>

#define GOLDENBOOT_MAGIC1	0x676f6c64 /* 'gold' */
#define GOLDENBOOT_MAGIC2	0x626f6f74 /* 'boot' */

/*****************************************
 *
 * Func : copy_2nd_bootloader_and_run
 * 	This function will copy next stage bootcode and jump.
 * - dst : ADDR where next stage bootcode is loaded and executed.
 * - src : source ADDR where next stage bootcode is loaded.
 * - size : size of next stage bootcode to be copied.
 *
 ****************************************/
int copy_2nd_bootloader_and_run(char *dst, const char *src, unsigned size)
{
	unsigned i = 0;
	const char *ptr_s = src;
	char *ptr_d = dst;
	void (*second_bootloader_entry)(void);

	printf("copy_2nd_bootloader_and_run : src:0x%08x, dst:0x%08x, size:0x%08x\n", src, dst, size);

	second_bootloader_entry = (void (*)(void))dst;
	printf("Jumping to 2nd bootloader...\n");
	cleanup_before_bootcode();

	/* copy the 2nd bootloader to target addr */
	if(src != dst) {
		for(i = 0 ; i < size ; i++) {
			*ptr_d++ = *ptr_s++;
		}
	}

	second_bootloader_entry();
}

/*****************************************
 *
 * Func : start_2nd_bootcode_from_golden
 * 	Called by outer function/command for jumping to
 * 	2nd stage boot code. The GOLDEN REGION should not
 * 	be overwritten when doing bootcode copy.
 * Note : Be sure that BootCode firmware is already loaded
 * 	and with env:bootcode2nd_loadaddr set.
 *
 ****************************************/
int start_2nd_bootcode_from_golden(void)
{
	uint bc_2nd_addr = 0, bc_2nd_tmp_addr = 0;
	unsigned cp_size = 0;

	if(!is_golden_boot()) {
		printf("shouldn't call 2nd bootcode EXCEPT GOLDEN MODE!!\n");
		return -1;
	}

	bc_2nd_addr = getenv_ulong("bootcode2nd_loadaddr", 16, 0);
	if (!bc_2nd_addr) {
		printf("2nd bootcode load address not found, SKIP!!\n");
		return -1;
	}

	bc_2nd_tmp_addr = getenv_ulong("bootcode2ndtmp_loadaddr", 16, 0);
	if (!bc_2nd_tmp_addr) {
		printf("2nd bootcode temp address not found, SKIP!!\n");
		return -1;
	}

	set_golden_magic();
#ifdef CONFIG_RTD1195	// FIXME, cp_size will encounter strange error. Use constant first.
	// SYNC the setting with setting under "u-boot.lds".
	cp_size = (0x000e0000 - (unsigned)CONFIG_SYS_TEXT_BASE);
#else
	// Copy size of 2nd bootloader, don't over-write the golden section.
	cp_size = ((unsigned)(&__golden_sec_start) - (unsigned)CONFIG_SYS_TEXT_BASE);
#endif

	copy_2nd_bootloader_and_run((char*)bc_2nd_addr, (char*)bc_2nd_tmp_addr, cp_size);

	return -1; // should never return
}

/*****************************************
 *
 * Func : is_golden_boot
 * 	check GOLDEN REGION MAGIC to identify if
 * 	current boot stage had already pass GOLDEN.
 *
 ****************************************/
int is_golden_boot(void)
{
	if((__golden_magic_1 == GOLDENBOOT_MAGIC1) && (__golden_magic_2 == GOLDENBOOT_MAGIC2))
		return 0;
	else
		return 1;
}

/*****************************************
 *
 * Func : set_golden_magic
 * 	set up GOLDEN REGION MAGIC so next stage
 * 	bootcode could identify whether it is under
 * 	GOLDEN(1st) stage or not.
 *
 ****************************************/
int set_golden_magic(void)
{
	__golden_magic_1 = GOLDENBOOT_MAGIC1;
	__golden_magic_2 = GOLDENBOOT_MAGIC2;

	return 0;
}
