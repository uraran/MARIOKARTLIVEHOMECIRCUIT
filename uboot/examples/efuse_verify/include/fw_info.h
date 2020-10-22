/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2013 by CK <cklai@realtek.com>
 *
 */

#ifndef _ASM_ARCH_FW_INFO_H_
#define _ASM_ARCH_FW_INFO_H_

//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
#define BYTE(d,n)		(((d) >> ((n) << 3)) & 0xFF)
#define SWAPEND32(d)	((BYTE(d,0) << 24) | (BYTE(d,1) << 16) | (BYTE(d,2) << 8) | (BYTE(d,3) << 0))

#define FW_DESC_TABLE_V1_T_VERSION_1		0x1
#define FW_DESC_TABLE_V1_T_VERSION_11		0x11
#define FW_DESC_TABLE_V1_T_VERSION_21		0x21

#define FW_DESC_TABLE_V2_T_VERSION_2		0x2
#define FW_DESC_TABLE_V2_T_VERSION_12		0x12
#define FW_DESC_TABLE_V2_T_VERSION_22		0x22

#define FW_DESC_BASE_VERSION(v)			((v) & 0xf)
#define FW_DESC_EXT_VERSION(v)			((v) >> 4)

#define FIRMWARE_DESCRIPTION_TABLE_SIGNATURE	"VERONA__" /* 8 bytes signature. */

#define EMMC_EXTERN_PARAM_BLOCK_NO	0x7c
#define EMMC_EXTERN_PARAM_SIZE		0x800
#define EMMC_HW_SETTING_BLOCK_NO	0x104
#define EMMC_BLOCK_SIZE				0x200

#define BOOT_AV_INFO_MAGICNO BOOT_AV_INFO_MAGICNO_RTK
#define BOOT_AV_INFO_MAGICNO_RTK	0x2452544B	// $RTK
#define BOOT_AV_INFO_MAGICNO_STD3	0x53544433	// STD3 <= support dynamic decode buffer

#define FIRMWARE_DESCRIPTION_TABLE_ADDR	(0x06400000)

#define dmb()		__asm__ __volatile__ ("dmb" : : : "memory")

#define __iormb()	dmb()

#define __arch_getb(a)			(*(volatile unsigned char *)(a))
#define __arch_getw(a)			(*(volatile unsigned short *)(a))
#define __arch_getl(a)			(*(volatile unsigned int *)(a))

#define readb(c)	({ u8  __v = __arch_getb(c); __iormb(); __v; })
#define readw(c)	({ u16 __v = __arch_getw(c); __iormb(); __v; })
#define readl(c)	({ u32 __v = __arch_getl(c); __iormb(); __v; })

//-----------------------------------------------------------------------------------------------
typedef enum {
	FW_IDX_LINUX_KERNEL = 0,
	FW_IDX_AUDIO,
	FW_IDX_VIDEO,
	FW_IDX_SQUASH,
	FW_IDX_JFFS2,
	FW_IDX_BOOTCODE,
} fw_desc_index_t;

typedef enum {
	PART_TYPE_RESERVED = 0,
	PART_TYPE_FW,
	PART_TYPE_FS,
} part_type_code_t;

typedef enum {
   FW_TYPE_RESERVED = 0,
   FW_TYPE_BOOTCODE,
   FW_TYPE_KERNEL,
   FW_TYPE_RESCUE_DT,
   FW_TYPE_KERNEL_DT,
   FW_TYPE_RESCUE_ROOTFS,	//5
   FW_TYPE_KERNEL_ROOTFS,
   FW_TYPE_AUDIO,
   FW_TYPE_AUDIO_FILE,
   FW_TYPE_VIDEO_FILE,
   FW_TYPE_EXT4,		//10
   FW_TYPE_UBIFS,
   FW_TYPE_SQUASH,
   FW_TYPE_EXT3,
   FW_TYPE_ODD,
   FW_TYPE_YAFFS2,		//15
   FW_TYPE_ISO,
   FW_TYPE_SWAP,
   FW_TYPE_NTFS,
   FW_TYPE_JFFS2,
   FW_TYPE_IMAGE_FILE,		//20
   FW_TYPE_IMAGE_FILE1,
   FW_TYPE_IMAGE_FILE2,
   FW_TYPE_AUDIO_FILE1,
   FW_TYPE_AUDIO_FILE2,
   FW_TYPE_VIDEO_FILE1,		//25
   FW_TYPE_VIDEO_FILE2,
   FW_TYPE_VIDEO,
   FW_TYPE_VIDEO2,
   FW_TYPE_ECPU,
   FW_TYPE_TEE,			//30
   FW_TYPE_CONFIG,
   FW_TYPE_UBOOT,
   FW_TYPE_INITRD_ROOTFS,
   FW_TYPE_UNKNOWN
} fw_type_code_t;

typedef enum {
	RTK_PLAT_ERR_OK = 0,
	RTK_PLAT_ERR_PARSE_FW_DESC,
	RTK_PLAT_ERR_READ_FW_IMG,
	RTK_PLAT_ERR_READ_KERNEL_IMG,
	RTK_PLAT_ERR_READ_RESCUE_IMG,
	RTK_PLAT_ERR_BOOT,
} rtk_plat_err_t;

typedef enum {
   FS_TYPE_JFFS2 = 0,
   FS_TYPE_YAFFS2,
   FS_TYPE_SQUASH,
   FS_TYPE_RAWFILE,
   FS_TYPE_EXT4,
   FS_TYPE_UBIFS,
   FS_TYPE_NONE,
   FS_TYPE_UNKOWN
} rtk_part_fw_type_t;

//-----------------------------------------------------------------------------------------------

typedef struct {
	unsigned char	signature[8];
	unsigned int	checksum;
	unsigned char	version;
	unsigned char	reserved[7];
	unsigned int	paddings;
	unsigned int	part_list_len;
	unsigned int	fw_list_len;
} __attribute__((packed)) fw_desc_table_v1_t;

typedef struct {
   unsigned char type;
#ifdef LITTLE_ENDIAN
   unsigned char reserved:7,
   ro:1;
#else 
   unsigned char ro:1,
   reserved:7;
#endif
   u64  length;
   unsigned char   fw_count;
   unsigned char   fw_type;
#ifdef CONFIG_SYS_RTK_EMMC_FLASH
   unsigned char   emmc_partIdx;
   unsigned char   reserved_1[3];
#else
   unsigned char   reserved_1[4];
#endif
   unsigned char   mount_point[32];
} __attribute__((packed)) part_desc_entry_v1_t;

// for fw_desc_table_v1_t.version = FW_DESC_TABLE_V1_T_VERSION_1
typedef struct {
	unsigned char	type;
#ifdef LITTLE_ENDIAN
	unsigned char	reserved:6,
					lzma:1,
					ro:1;
#else
	unsigned char	ro:1,
					lzma:1,
					reserved:6;
#endif
	unsigned int	version;
	unsigned int	target_addr;
	unsigned int	offset;
	unsigned int	length;
	unsigned int	paddings;
	unsigned int	checksum;
	unsigned char	sha_hash[32];
	unsigned char	reserved_1[6];
} __attribute__((packed)) fw_desc_entry_v1_t;

typedef struct {
	unsigned int	type;
	unsigned int	target_addr;
	unsigned int	offset;
	unsigned int	length;
	unsigned char	sha_hash[32];
} __attribute__((packed)) fw_desc_entry_verify;

typedef struct {
	unsigned int	bootcode_offset[4];
	unsigned int	bootcode_length;
	unsigned char	bootcode_sha_hash[32];
	unsigned int	hwsetting_offset[4];
	unsigned int	hwsetting_length;
	unsigned char	hwsetting_sha_hash[32];
	unsigned int	data_partition_offset;
	unsigned int	data_partition_length;
	unsigned char	data_partition_sha_hash[32];
} __attribute__((packed)) bootcode_hwsetting_desc_entry_verify;

// for fw_desc_table_v1_t.version = FW_DESC_TABLE_V1_T_VERSION_11
typedef struct {
	fw_desc_entry_v1_t v1;
	unsigned int	act_size;
	unsigned char	hash[32];
	unsigned char	part_num;
	unsigned char	reserved[27];
} __attribute__((packed)) fw_desc_entry_v11_t;


// for fw_desc_table_v1_t.version = FW_DESC_TABLE_V1_T_VERSION_21
typedef struct {
	fw_desc_entry_v1_t v1;
	unsigned int	act_size;
	unsigned char	part_num;
	unsigned char	RSA_sign[256];
	unsigned char	reserved[27];
} __attribute__((packed)) fw_desc_entry_v21_t;

// Version 2, offset->u64 / checksum->sha256
typedef struct {
	unsigned char	type;
#ifdef LITTLE_ENDIAN
	unsigned char	reserved:6,
					lzma:1,
					ro:1;
#else
	unsigned char	ro:1,
					lzma:1,
					reserved:6;
#endif
	unsigned int	version;
	unsigned int	target_addr;
	unsigned long	offset;
	unsigned int	length;
	unsigned int	paddings;
	unsigned char	sha_hash[32];
	unsigned char	reserved_1[6];
} __attribute__((packed)) fw_desc_entry_v2_t;

// for fw_desc_table_v1_t.version = FW_DESC_TABLE_V1_T_VERSION_11
typedef struct {
	fw_desc_entry_v2_t v2;
	unsigned int	act_size;
	unsigned char	hash[32];
	unsigned char	part_num;
	unsigned char	reserved[27];
} __attribute__((packed)) fw_desc_entry_v12_t;


// for fw_desc_table_v1_t.version = FW_DESC_TABLE_V1_T_VERSION_21
typedef struct {
	fw_desc_entry_v2_t v2;
	unsigned int	act_size;
	unsigned char	part_num;
	unsigned char	RSA_sign[256];
	unsigned char	reserved[27];
} __attribute__((packed)) fw_desc_entry_v22_t;

 /* copy from darwin boot audio/video data info */
typedef struct {
	unsigned int	dwMagicNumber;			//identificatin String "$RTK" or "STD3"

	unsigned int	dwVideoStreamAddress;	//The Location of Video ES Stream
	unsigned int	dwVideoStreamLength;	//Video ES Stream Length

	unsigned int	dwAudioStreamAddress;	//The Location of Audio ES Stream
	unsigned int	dwAudioStreamLength;	//Audio ES Stream Length

	unsigned char	bVideoDone;
	unsigned char	bAudioDone;

	unsigned char	bHDMImode;				//monitor device mode (DVI/HDMI)

	char	dwAudioStreamVolume;	//Audio ES Stream Volume -60 ~ 40

	char	dwAudioStreamRepeat;	//0 : no repeat ; 1 :repeat

	unsigned char	bPowerOnImage;			// Alternative of power on image or video
	unsigned char	bRotate;				// enable v &h flip
	unsigned int	dwVideoStreamType;		// Video Stream Type

	unsigned int	audio_buffer_addr;		// Audio decode buffer
	unsigned int	audio_buffer_size;
	unsigned int	video_buffer_addr;		// Video decode buffer
	unsigned int	video_buffer_size;
	unsigned int	last_image_addr;		// Buffer used to keep last image
	unsigned int	last_image_size;
	
	unsigned char	logo_enable;
	unsigned int	logo_addr;
	unsigned int   src_width;              //logo_src_width
    unsigned int   src_height;             //logo_src_height
    unsigned int   dst_width;              //logo_dst_width
    unsigned int   dst_height;             //logo_dst_height
	
} boot_av_info_t;

/* Boot flash type (saved extern param, logo, rescue) */
typedef enum {
	BOOT_NOR_SERIAL,
	BOOT_NAND,
	BOOT_EMMC
} BOOT_FLASH_T;

typedef enum {
	FLASH_TYPE_NAND = 0,
	FLASH_TYPE_SPI,
	FLASH_TYPE_EMMC,
	FLASH_TYPE_UNKNOWN
} FLASH_TYPE_T;

#define FLASH_TYPE_NAND_STRING		"nand"
#define FLASH_TYPE_SPI_STRING		"spi"
#define FLASH_TYPE_EMMC_STRING		"emmc"
#define FLASH_TYPE_UNKNOWN_STRING	"none"

typedef enum {
	NONE_SECURE_BOOT = 0,
	RTK_SECURE_BOOT,
} SECUTE_BOOT_T;

extern BOOT_FLASH_T boot_flash_type;

//-----------------------------------------------------------------------------------------------

/* Add for fixing Rescue Kernel stop at 8051 standby */
typedef enum {
        BOOT_CONSOLE_MODE = 0,
        BOOT_RESCUE_MODE,
        BOOT_NORMAL_MODE,
        BOOT_MANUAL_MODE,
        BOOT_QC_VERIFY_MODE,
        BOOT_FLASH_WRITER_MODE,
        BOOT_MODE_NUM,  
        BOOT_ANDROID_MODE,      
		BOOT_KERNEL_ONLY_MODE	
} BOOT_MODE;

//-----------------------------------------------------------------------------------------------
typedef struct{
	unsigned char bIsEncrypted;			/* must fill this value before we call get layout */
	unsigned char bIsCompressed;		/* must fill this value before we call get layout */

	unsigned int flash_to_ram_addr;		/* will get this result at least */
	unsigned int encrpyted_addr;		/* option result */
	unsigned int decrypted_addr;		/* option result */
	unsigned int compressed_addr;		/* option result */
	unsigned int decompressed_addr;		/* option result */
	unsigned int image_target_addr;		/* must fill this value before we call get layout */
} MEM_LAYOUT_WHEN_READ_IMAGE_T;

#endif	// _ASM_ARCH_FW_INFO_H_

