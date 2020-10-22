#ifndef __PCB_H__
#define __PCB_H__

#include <platform_lib/board/pcb_mgr.h>


#if defined(CONFIG_BOARD_FPGA_RTD1195_EMMC)
	#include "pcb/pcb_rtd1095_fpga.h"	
#else
	#include "pcb/pcb_rtd1095_default.h"
	//#error "pcbMgr no support this board."
#endif

#endif	// __PCB_H__

