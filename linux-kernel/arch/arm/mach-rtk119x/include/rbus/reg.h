#ifndef _REG_H_INCLUDED_
#define _REG_H_INCLUDED_

#include <mach/memory.h>

#define SYSTEM_GIC_BASE_PHYS	(0xff010000)
#define SYSTEM_GIC_BASE_VIRT	IOMEM(0xff010000)
#define SYSTEM_GIC_CPU_BASE		IOMEM(0xff012000)
#define SYSTEM_GIC_DIST_BASE	IOMEM(0xff011000)

#include "reg_misc.h"
#include "reg_iso.h"
#include "reg_mcp.h"
#include "reg_se.h"
#include "reg_md.h"
#include "reg_sb2.h"

#endif //_REG_H_INCLUDED_
