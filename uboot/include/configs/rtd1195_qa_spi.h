/*
 * Configuration settings for the Realtek 1195 fpga board.
 *
 * Won't include this file.
 * Just type "make <board_name>_config" and will be included in source tree.
 */

#ifndef __CONFIG_RTK_RTD1195_QA_H
#define __CONFIG_RTK_RTD1195_QA_H

/*
 * Include the common settings of RTD1195 platform.
 */
#include <configs/rtd1195_common.h>

#ifdef CONFIG_ENV_SIZE
	#undef CONFIG_ENV_SIZE
	#define CONFIG_ENV_SIZE (4<<10)
	#undef CONFIG_SYS_MALLOC_LEN
	#define CONFIG_SYS_MALLOC_LEN	(CONFIG_ENV_SIZE + (256 << 10))
#endif

#ifdef CONFIG_INSTALL_GPIO_NUM
	#undef CONFIG_INSTALL_GPIO_NUM
#endif

/*
 * The followings were RTD1195 demo board specific configuration settings.
 */

/* Board config name */
#define CONFIG_BOARD_FPGA_RTD1195_EMMC
//#define CONFIG_BOARD_QA_RTD1195
//#define CONFIG_BOARD_QA_RTD1195_WIFI_STORAGE


/* Flash writer setting:
 *   The corresponding setting will be located at
 *   uboot/examples/flash_writer_u/$(CONFIG_FLASH_WRITER_SETTING).inc
 */
//#define CONFIG_FLASH_WRITER_SETTING


/* Flash type is SPI or NAND or eMMC*/
#define CONFIG_SYS_RTK_SPI_FLASH
//#define CONFIG_SYS_RTK_NAND_FLASH
//#define CONFIG_SYS_RTK_EMMC_FLASH

//#define CONFIG_NAND_ON_THE_FLY_TEST_KEY

#if defined(CONFIG_SYS_RTK_EMMC_FLASH)
    #define CONFIG_FLASH_WRITER_SETTING                 "1195_force_emmc_nS_nE"
    #define CONFIG_CHIP_ID                              "rtd1195"
    #define CONFIG_CUSTOMER_ID                          "demo"
    #define CONFIG_CHIP_TYPE                            "0000"
    /* Factory start address and size in eMMC */
    #define CONFIG_FACTORY_START                        0x220000    /* For eMMC */
    #define CONFIG_FACTORY_SIZE                         0x400000    /* For eMMC */
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
    #undef  CONFIG_ENV_IS_NOWHERE
    #ifdef CONFIG_SYS_FACTORY
        #define CONFIG_ENV_IS_IN_FACTORY
    #else
        #define CONFIG_ENV_IS_IN_MMC                    /* if enable CONFIG_SYS_FACTORY, env will be saved in factory data */
    #endif

    #define CONFIG_SYS_FACTORY_READ_ONLY

    #ifdef CONFIG_RESCUE_FROM_USB
        #undef CONFIG_RESCUE_FROM_USB_VMLINUX
        #undef CONFIG_RESCUE_FROM_USB_DTB
        #undef CONFIG_RESCUE_FROM_USB_ROOTFS
        #define CONFIG_RESCUE_FROM_USB_VMLINUX          "emmc.uImage"
        #define CONFIG_RESCUE_FROM_USB_DTB              "rescue.emmc.dtb"
        #define CONFIG_RESCUE_FROM_USB_ROOTFS           "rescue.root.emmc.cpio.gz_pad.img"
    #endif /* CONFIG_RESCUE_FROM_USB */

#elif defined(CONFIG_SYS_RTK_NAND_FLASH)
    #define CONFIG_FLASH_WRITER_SETTING                 "1195_force_nand"
    #define CONFIG_CHIP_ID                              "rtd1195"
    #define CONFIG_CUSTOMER_ID                          "demo"
    #define CONFIG_CHIP_TYPE                            "0000"
    #define CONFIG_FACTORY_START_BLK                    0x60        /* For NAND */
    #define CONFIG_FACTORY_SIZE                         0x800000    /*For MLC; SLC:0x400000 */
    #define CONFIG_MTD_DEVICE
    #define CONFIG_MTD_PARTITIONS
    #define CONFIG_CMD_NAND
    #define CONFIG_SYS_MAX_NAND_DEVICE                  1
    #define CONFIG_SYS_NAND_BASE                        0x00000000
    #undef  CONFIG_ENV_IS_NOWHERE
    #define CONFIG_ENV_IS_IN_FACTORY

#elif defined(CONFIG_SYS_RTK_SPI_FLASH)
    #define CONFIG_FLASH_WRITER_SETTING                 "1195_force_spi_nS_nE"
    //#define CONFIG_FLASH_WRITER_SETTING               "1195_force_romcode_on_spi_nS_nE"
    #define CONFIG_CHIP_ID                              "rtd1195"
    #define CONFIG_CUSTOMER_ID                          "demo"
    #define CONFIG_CHIP_TYPE                            "0000"

    /* SPI */
    #define CONFIG_RTKSPI
    #define CONFIG_CMD_RTKSPI
    //#define CONFIG_DTB_IN_SPI_NOR
    
    #define CONFIG_BOOTCODE2_BASE                       0x00080000
#ifdef CONFIG_DTB_IN_SPI_NOR
    #define CONFIG_BOOTCODE2_MAX_SIZE					0x00050000			// up to 320KB
    #define CONFIG_DTS_BASE                             0x000D0000
    #define CONFIG_DTS_SIZE                             0x00010000
#else
    #define CONFIG_BOOTCODE2_MAX_SIZE					0x00060000			// up to 384KB
    #define CONFIG_DTS_BASE                             0xFFFFFFFF
    #define CONFIG_DTS_SIZE                             0x00000000
#endif

    #define CONFIG_FACTORY_BASE                         0x000E0000

#if 1
    #define CONFIG_FACTORY_SIZE                         0x00020000			// 64*2 KB
#else
    #define CONFIG_FACTORY_SIZE                         0x00080000			// 256*2 KB
#endif

#if 0
    #define CONFIG_FACTORY_RO_BASE                      0x00100000
    #define CONFIG_FACTORY_RO_SIZE                      0x00080000
#endif
    /* ENV */
    #undef  CONFIG_ENV_IS_NOWHERE
    #define CONFIG_ENV_IS_IN_FACTORY
    #define CONFIG_SYS_FACTORY_READ_ONLY
	
	/*related to Linux boot process */
	#define CONFIG_RESCUE_FROM_USB
	#define CONFIG_BOOT_FROM_USB
	#ifdef  CONFIG_BOOT_FROM_USB
	#define CONFIG_RESCUE_FROM_USB_VMLINUX			"spi.uImage"
	#define CONFIG_RESCUE_FROM_USB_DTB				"rescue.spi.dtb"
	#define CONFIG_NORMAL_FROM_USB_DTB				"android.spi.dtb"
	#define CONFIG_RESCUE_FROM_USB_ROOTFS			"rescue.root.spi.cpio.gz_pad.img"
	#define CONFIG_NORMAL_FROM_USB_ROOTFS			"android.root.spi.cpio.gz_pad.img"
	#define CONFIG_RESCUE_FROM_USB_AUDIO_CORE		"bluecore.audio"
    #endif /* CONFIG_BOOT_FROM_USB */
		
#else
    "rtd1195_qa_spi.h : FIX ME"
#endif

/* Boot Revision */
#define CONFIG_COMPANY_ID       "0000"
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


#define ARM_ROMCODE_SIZE            (44*1024)
#define MIPS_RESETROM_SIZE              (0x1000UL)
#define CONFIG_NR_DRAM_BANKS        1
#define CONFIG_SYS_SDRAM_BASE       0
#define CONFIG_SYS_RAM_DCU1_SIZE     0x40000000

#undef  V_NS16550_CLK
#define V_NS16550_CLK           27000000

#endif

#define CONFIG_CMD_MD5SUM

#endif /* __CONFIG_RTK_RTD1195_QA_H */

