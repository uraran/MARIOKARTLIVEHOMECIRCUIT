#ifndef __POWER_H__
#define __POWER_H__

#include <config.h>


#if defined(CONFIG_BOARD_DEMO_RTD1095)
	#include "power/power_rtd1095_demo.h"
#else
	#error "power-saving does not support this board."
#endif



#endif	// __POWER_H__

