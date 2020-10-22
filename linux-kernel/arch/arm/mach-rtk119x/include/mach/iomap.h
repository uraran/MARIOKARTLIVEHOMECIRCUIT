/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2012 by Chuck Chen <ycchen@realtek.com>
 *
 * IO mappings for Magellan
 */

#include <asm/memory.h>

/*
 * ----------------------------------------------------------------------------
 * Magellan specific IO mapping
 * ----------------------------------------------------------------------------
 */

#define RBUS_BASE_PHYS          0x18000000
#define RBUS_BASE_VIRT          0xfe000000
#define RBUS_BASE_SIZE          0x70000

#define RPC_BASE_PHYS           0x01600000
#define RPC_BASE_VIRT           __va(RPC_BASE_PHYS)
#define RPC_BASE_SIZE           0xa00000

#define FLASH_BASE_PHYS         0x18100000
#define FLASH_BASE_SIZE         0x2000000

