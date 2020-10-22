/*
 *  arch/arm/mach-rtk119x/include/mach/memory.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#define RTK_FLAG_NONCACHED      (1U << 0)
#define RTK_FLAG_SCPUACC        (1U << 1)
#define RTK_FLAG_ACPUACC        (1U << 2)
#define RTK_FLAG_HWIPACC        (1U << 3)
#define RTK_FLAG_DEAULT         (RTK_FLAG_NONCACHED | RTK_FLAG_SCPUACC | RTK_FLAG_ACPUACC | RTK_FLAG_HWIPACC)

#define PLAT_PHYS_OFFSET		(0x00000000)
#define PLAT_MEM_SIZE			(2*256*1024*1024)

#define SYS_BOOTCODE_MEMBASE	(PLAT_PHYS_OFFSET)
#define SYS_BOOTCODE_MEMSIZE	(0x0000C000)

#define PLAT_AUDIO_BASE_PHYS	(0x01b00000)
#define PLAT_AUDIO_SIZE			(0x00064000)//(0x00400000)

#define RPC_RINGBUF_PHYS		(0x01ffe000)
#define RPC_RINGBUF_VIRT		(0xFC7F8000+0x00004000)
#define RPC_RINGBUF_SIZE		(0x00004000)

#define RBUS_BASE_PHYS			(0x18000000)
#define RBUS_BASE_VIRT			(0xFE000000)
#define RBUS_BASE_SIZE			(0x00070000)

#define PLAT_NOR_BASE_PHYS		(0x18100000)
#define PLAT_NOR_SIZE			(0x01000000)

#define PLAT_SECURE_BASE_PHYS	(0x10000000)
#define PLAT_SECURE_SIZE		(0x00100000)

#ifdef CONFIG_ARM_NORMAL_WORLD_OS
#define PLAT_SECUREOS_BASE_PHYS	        (0x07D00000)
#define PLAT_SECUREOS_SIZE		(0x00300000)
#endif
#if 0
#define VE_BASE_PHYS			(0x20000000)
#define VE_BASE_VIRT			(VE_BASE_PHYS)
#define VE_BASE_SIZE			(0x10000000)
#else
#define VE_BASE_SIZE			0 //(1024*1024*16)
#define VE_BASE_PHYS			(0x18000000 - (VE_BASE_SIZE))
#define VE_BASE_VIRT			(VE_BASE_PHYS)
#endif

#define PLAT_FRAME_BUFFER_SIZE_576P         (0x004BF000) // 4MB
#define PLAT_FRAME_BUFFER_SIZE_800x600      (0x0057F000) // 5MB
#define PLAT_FRAME_BUFFER_SIZE_720P         (0x00C00000) // 12MB
#define PLAT_FRAME_BUFFER_SIZE_1080P        (0x01800000) // 24MB

#define PLAT_FRAME_BUFFER_SIZE	PLAT_FRAME_BUFFER_SIZE_1080P

#define VE_ION_SIZE			    (1024*1024*169)
#define ION_TILER_HEAP_SIZE		(0)
#define ION_AUDIO_HEAP_SIZE		(0)   //(1024*1024*1)
#define ION_MEDIA_HEAP_SIZE		(1024*1024*39 + VE_ION_SIZE + PLAT_FRAME_BUFFER_SIZE)

/* 26MB */
#define ION_MEDIA_HEAP_PHYS4    PLAT_NOR_BASE_PHYS
#define ION_MEDIA_HEAP_SIZE4    (PLAT_NOR_SIZE + 0x00A00000)
#define ION_MEDIA_HEAP_FLAG4    (RTK_FLAG_NONCACHED | RTK_FLAG_HWIPACC)

/* 26MB */
#define ION_MEDIA_HEAP_PHYS3    (0x1E600000)
#define ION_MEDIA_HEAP_SIZE3    (0x01A00000)
#define ION_MEDIA_HEAP_FLAG3    (RTK_FLAG_NONCACHED | RTK_FLAG_HWIPACC)

/* 127MB */
#if 0//def CONFIG_ARM_NORMAL_WORLD_OS
#define ION_MEDIA_HEAP_PHYS2    (PLAT_SECUREOS_BASE_PHYS + PLAT_SECUREOS_SIZE)
#define ION_MEDIA_HEAP_SIZE2    (0x18000000 - ION_MEDIA_HEAP_PHYS2)
#define ION_MEDIA_HEAP_FLAG2    (RTK_FLAG_DEAULT)
#else
#define ION_MEDIA_HEAP_PHYS2    (PLAT_SECURE_BASE_PHYS + PLAT_SECURE_SIZE)
#define ION_MEDIA_HEAP_SIZE2    (0x18000000 - ION_MEDIA_HEAP_PHYS2)
#define ION_MEDIA_HEAP_FLAG2    (RTK_FLAG_DEAULT)
#endif
/* 29MB */
#define ION_MEDIA_HEAP_SIZE1    (ION_MEDIA_HEAP_SIZE   - ION_MEDIA_HEAP_SIZE2 - ION_MEDIA_HEAP_SIZE3 - ION_MEDIA_HEAP_SIZE4)
#ifdef CONFIG_ARM_NORMAL_WORLD_OS
#define ION_MEDIA_HEAP_PHYS1    (PLAT_SECUREOS_BASE_PHYS - ION_MEDIA_HEAP_SIZE1)
#else
#define ION_MEDIA_HEAP_PHYS1    (PLAT_SECURE_BASE_PHYS - ION_MEDIA_HEAP_SIZE1)
#endif
#define ION_MEDIA_HEAP_FLAG1    (RTK_FLAG_DEAULT)

#define ION_TILER_HEAP_PHYS     (ION_MEDIA_HEAP_PHYS1  - ION_TILER_HEAP_SIZE)
#define ION_TILER_HEAP_FLAG     (RTK_FLAG_DEAULT)

#define ION_AUDIO_HEAP_PHYS     (ION_TILER_HEAP_PHYS   - ION_AUDIO_HEAP_SIZE)
#define ION_AUDIO_HEAP_FLAG     (RTK_FLAG_DEAULT)


#define RPC_COMM_PHYS			(0x0000B000)
#define RPC_COMM_VIRT			(RBUS_BASE_VIRT+RBUS_BASE_SIZE)
#define RPC_COMM_SIZE			(0x00001000)

#define SPI_RBUS_PHYS			(0x18100000)
#define SPI_RBUS_VIRT			(0xfb000000)
#define SPI_RBUS_SIZE			(0x01000000)

/**************** For 512MB layout ****************/

#define UHD_VE_ION_SIZE             (1024*1024*69)
#define UHD_ION_MEDIA_HEAP_SIZE     (1024*1024*39 + UHD_VE_ION_SIZE + PLAT_FRAME_BUFFER_SIZE)

#define UHD_ION_MEDIA_HEAP_PHYS4    ION_MEDIA_HEAP_PHYS4
#define UHD_ION_MEDIA_HEAP_SIZE4    ION_MEDIA_HEAP_SIZE4
#define UHD_ION_MEDIA_HEAP_FLAG4    ION_MEDIA_HEAP_FLAG4

#define UHD_ION_MEDIA_HEAP_PHYS3    ION_MEDIA_HEAP_PHYS3
#define UHD_ION_MEDIA_HEAP_SIZE3    ION_MEDIA_HEAP_SIZE3
#define UHD_ION_MEDIA_HEAP_FLAG3    ION_MEDIA_HEAP_FLAG3

#define UHD_ION_MEDIA_HEAP_PHYS2    (PLAT_SECURE_BASE_PHYS + PLAT_SECURE_SIZE)
#define UHD_ION_MEDIA_HEAP_SIZE2    (UHD_ION_MEDIA_HEAP_SIZE - UHD_ION_MEDIA_HEAP_SIZE3 - UHD_ION_MEDIA_HEAP_SIZE4)
#define UHD_ION_MEDIA_HEAP_FLAG2    ION_MEDIA_HEAP_FLAG2

/**************** For 256MB layout ****************/

#define ULM_VE_ION_SIZE             (1024*1024*40)
#define ULM_ION_MEDIA_HEAP_SIZE     (1024*1024*38 + ULM_VE_ION_SIZE + PLAT_FRAME_BUFFER_SIZE)

#define ULM_ION_MEDIA_HEAP_PHYS1    (ION_AUDIO_HEAP_PHYS - ULM_ION_MEDIA_HEAP_SIZE)
#define ULM_ION_MEDIA_HEAP_SIZE1    (ULM_ION_MEDIA_HEAP_SIZE)
#define ULM_ION_MEDIA_HEAP_FLAG1    (RTK_FLAG_DEAULT)

/**************** For 256MB Encode only layout ****************/

#define ENC_ION_MEDIA_HEAP_SIZE     (1024*1024*42)

#define ENC_ION_MEDIA_HEAP_PHYS1    0x5000000
#define ENC_ION_MEDIA_HEAP_SIZE1    (ENC_ION_MEDIA_HEAP_SIZE)
#define ENC_ION_MEDIA_HEAP_FLAG1    (RTK_FLAG_DEAULT)

/************************FOR AUDIO_RPC layout******************************/
#define ION_AUDIO_HEAP_PHYS1    0x7a00000
#define ION_AUDIO_HEAP_SIZE1    0x10000
#define ION_AUDIO_HEAP_FLAG1    (RTK_FLAG_DEAULT)


/**************************************************/
#endif