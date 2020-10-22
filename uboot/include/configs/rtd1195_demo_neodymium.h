/*
 * Configuration settings for the Realtek 1195 demo board.
 *
 * Won't include this file.
 * Just type "make <board_name>_config" and will be included in source tree.
 */

#ifndef __CONFIG_RTK_RTD1195_DEMO_NEODYMIUM_H
#define __CONFIG_RTK_RTD1195_DEMO_NEODYMIUM_H

/*
 * Include the common settings of RTD1195 platform.
 */
#include <configs/rtd1195_common.h>

/*
 * The followings were RTD1195 demo board specific configuration settings.
 */

/* Board config name */
#define CONFIG_BOARD_DEMO_RTD1195_NEODYMIUM

     
/* Flash type is eMMC*/
#define CONFIG_SYS_RTK_EMMC_FLASH

#if defined(CONFIG_SYS_RTK_EMMC_FLASH)
	/* Flash writer setting:
	 *   The corresponding setting will be located at
	 *   uboot/examples/flash_writer_u/$(CONFIG_FLASH_WRITER_SETTING).inc */
	#define CONFIG_FLASH_WRITER_SETTING             "1195_mobile"
	#define CONFIG_CHIP_ID            				"rtd1195"
	#define CONFIG_CUSTOMER_ID            			"demo" //"rom2","secure","NAS"
	#define CONFIG_CHIP_TYPE            			"0001" // mobile
	#define CONFIG_FLASH_TYPE            		    "RTK_FLASH_EMMC"

	/* Factory start address and size in eMMC */
	#define CONFIG_FACTORY_START	0x220000	/* For eMMC */
	#define CONFIG_FACTORY_SIZE		0x400000	/* For eMMC */

	//#define EMMC_1_8V				/* emmc supports 1.8V */

	#ifndef CONFIG_SYS_PANEL_PARAMETER
		#define CONFIG_SYS_PANEL_PARAMETER
	#endif
	
	/* MMC */
	#define CONFIG_MMC
	#ifndef CONFIG_PARTITIONS
		#define CONFIG_PARTITIONS
	#endif
	#define CONFIG_DOS_PARTITION
	#define CONFIG_GENERIC_MMC   
	#define CONFIG_RTK_MMC
	#define CONFIG_CMD_MMC
	#define USE_SIMPLIFY_READ_WRITE
	
	/* ENV */
	#undef	CONFIG_ENV_IS_NOWHERE
	#ifdef CONFIG_SYS_FACTORY    
		#define CONFIG_ENV_IS_IN_FACTORY
	#else
		#define CONFIG_ENV_IS_IN_MMC	/* if enable CONFIG_SYS_FACTORY, env will be saved in factory data */
	#endif
	
	//#define CONFIG_SYS_FACTORY_READ_ONLY

#endif

/* Boot Revision */
#define CONFIG_COMPANY_ID 		"0000"
#define CONFIG_BOARD_ID         "0705"
#define CONFIG_VERSION          "0000"

/*
 * SDRAM Memory Map
 * Even though we use two CS all the memory
 * is mapped to one contiguous block
 */
#if 1 	
	// undefine existed configs to prevent compile warning
	#undef CONFIG_NR_DRAM_BANKS
	#undef CONFIG_SYS_SDRAM_BASE
	#undef CONFIG_SYS_RAM_DCU1_SIZE
	
	
	#define ARM_ROMCODE_SIZE			(44*1024)			//page aligned
	#define MIPS_RESETROM_SIZE          (0x1000UL)
	#define CONFIG_NR_DRAM_BANKS		1
	#define CONFIG_SYS_SDRAM_BASE		0
	#define CONFIG_SYS_RAM_DCU1_SIZE	0x40000000	
#endif

/* Bootcode Feature: Rescue linux read from USB */
#define CONFIG_RESCUE_FROM_USB
#ifdef CONFIG_RESCUE_FROM_USB
	#define CONFIG_RESCUE_FROM_USB_VMLINUX			"emmc.uImage"
	#define CONFIG_RESCUE_FROM_USB_DTB				"rescue.emmc.dtb"
	#define CONFIG_RESCUE_FROM_USB_ROOTFS			"rescue.root.emmc.cpio.gz_pad.img"
	#define CONFIG_RESCUE_FROM_USB_AUDIO_CORE		"bluecore.audio"
#endif /* CONFIG_RESCUE_FROM_USB */


#undef CONFIG_CMD_NET
/* Eth Net */
#undef CONFIG_CMD_PING
#undef CONFIG_CMD_TFTPPUT
#undef CONFIG_RTL8168
#undef CONFIG_TFTP_BLOCKSIZE		400

/* Network setting */
#undef CONFIG_ETHADDR				00:10:20:30:40:50
#undef CONFIG_IPADDR				192.168.100.1
#undef CONFIG_GATEWAYIP				192.168.100.254
#undef CONFIG_SERVERIP				192.168.100.2
#undef CONFIG_NETMASK				255.255.255.0

#endif /* __CONFIG_RTK_RTD1195_DEMO_NEODYMIUM_H */

