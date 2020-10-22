/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
#include <asm/arch/system.h>
#include <asm/arch/extern_param.h>

#ifdef CONFIG_GOLDENBOOT_SUPPORT
#include <goldenboot.h>
#endif

int rtk_boot2ndbootcode(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	do {
		
#ifdef CONFIG_GOLDENBOOT_SUPPORT
		unsigned fw_load_result = 0;
		if(!getenv_ulong("bootcode2nd_loadaddr", 16, 0))
			fw_load_result = rtk_plat_prepare_fw_image_from_eMMC();
		if(!fw_load_result)
			start_2nd_bootcode_from_golden();
#else
		printf("2nd stage bootcode NOT supported!!\n");
#endif
	} while(0);

	printf("Could not perform 2nd stage bootcode!!!\n");

	return 0;
}

U_BOOT_CMD(
	b2ndbc, 4, 0,	rtk_boot2ndbootcode,
	"load 2nd stage bootcode",
	"b2ndbc\n"
);

