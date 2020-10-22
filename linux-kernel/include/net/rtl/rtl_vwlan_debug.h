#ifndef	_RTL_BDBG_H_
#define	_RTL_BDBG_H_

#define stringiz(arg) #arg

enum dbg_level {
	LV_ERR,
	LV_INFO,
	LV_RELEASE,
	LV_DEBUG,
	LV_TRACE,
	LV_END,
};

static const char lv_str[LV_END][32] = {
	stringiz(LV_ERR),
	stringiz(LV_INFO),
	stringiz(LV_RELEASE),
	stringiz(LV_DEBUG),
	stringiz(LV_TRACE),
};

/* set debug print level */
#if 0//disable debug
#define BDBG_LEVEL LV_DEBUG
#endif


//-------------------------------
#define LV_0414_SHROTCUT LV_TRACE
#define LV_0414_SHROTCUT_INDEV LV_TRACE
#define LV_0408_FRAG LV_TRACE
#define LV_0331_INDEVT LV_TRACE
#define LV_0328_RPCHDR LV_TRACE
#define LV_0328_DHCP  LV_TRACE
#define LV_0325_CFG_IOCTL LV_TRACE
#define LV_0324_CHK_BASE_DEV LV_TRACE
#define LV_0324_MIFACE LV_TRACE
#define LV_0321_RXHANDLER LV_TRACE
#define LV_0314_PROC LV_TRACE
#define LV_0314_CFG LV_TRACE
#define LV_0314_RX_DEV LV_TRACE
//-------------------------------

#if defined(BDBG_LEVEL)
#define BDBG_SHOW_FUNC_LINE 1
#define BDBG_SHOW_LEVEL_STR 1

  #if defined(BDBG_SHOW_FUNC_LINE)
    #if defined(BDBG_SHOW_LEVEL_STR)
	#define bdbg(level,fmt,args...) \
		do { \
		if (BDBG_LEVEL>=level) \
			printk("<%s> [%s:%d] "fmt, \
				lv_str[level], __FUNCTION__, __LINE__, \
				##args); \
		} while(0)
	#else
	#define bdbg(level,fmt,args...) \
		do { \
			if (BDBG_LEVEL>=level) \
				printk("[%s:%d] "fmt, \
				__FUNCTION__, __LINE__, ##args); \
		} while(0)
	#endif
  #else
	#define bdbg(level,fmt,args...) \
		do { \
			if (BDBG_LEVEL>=level) \
				printk(""fmt,##args); \
		} while(0)
  #endif
#else
	#define bdbg(level,fmt,args...)	do {} while(0)
#endif	/* #if defined(BDBG_LEVEL) */

#define bdbg_err(fmt,args...) \
	do { \
			printk(""fmt,##args); \
	} while(0)

#if defined(VWLAN_FOR_USERS)
#undef bdbg
#undef bdbg_err

#if defined(BDBG_LEVEL)
#define BDBG_SHOW_FUNC_LINE 1
#define BDBG_SHOW_LEVEL_STR 1

  #if defined(BDBG_SHOW_FUNC_LINE)
    #if defined(BDBG_SHOW_LEVEL_STR)
	#define bdbg(level,fmt,args...) \
		do { \
		if (BDBG_LEVEL>=level) \
			printf("<%s> [%s:%d] "fmt, \
				lv_str[level], __FUNCTION__, __LINE__, \
				##args); \
		} while(0)
	#else
	#define bdbg(level,fmt,args...) \
		do { \
			if (BDBG_LEVEL>=level) \
				printf("[%s:%d] "fmt, \
				__FUNCTION__, __LINE__, ##args); \
		} while(0)
	#endif
  #else
	#define bdbg(level,fmt,args...) \
		do { \
			if (BDBG_LEVEL>=level) \
				printf(""fmt,##args); \
		} while(0)
  #endif
#else
	#define bdbg(level,fmt,args...)	do {} while(0)
#endif	/* #if defined(BDBG_LEVEL) */

#define bdbg_err(fmt,args...) \
	do { \
			printf(""fmt,##args); \
	} while(0)

#endif /* #if defined(VWLAN_FOR_USERS) */

#endif /* _RTL_BDBG_H_ */
