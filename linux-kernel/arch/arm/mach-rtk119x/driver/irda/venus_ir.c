//#include <linux/config.h>

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* DBG_PRINT() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/kfifo.h>
#include <linux/interrupt.h>

//TODO : remove delay if the interrupt mux is fix victor.hsu
#include <linux/delay.h>


#if defined(CONFIG_REALTEK_DARWIN) || defined(CONFIG_REALTEK_MACARTHUR)
#include <linux/timer.h>
#endif /* CONFIG_REALTEK_DARWIN */
#include <linux/jiffies.h>
#include <linux/device.h>


//#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <asm/io.h>
#include <linux/sched.h>
//#include <linux/devfs_fs_kernel.h> /* for devfs */
#include <linux/time.h>

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/interrupt.h>

#include <asm/bitops.h>		/* bit operations */
//#include <platform.h>
#include "venus_ir.h"

#ifdef CONFIG_SUPPORT_INPUT_DEVICE
#include "ir_input.h"
#endif
#include "ir_rp.h"
#include "ir_tx.h"

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/sysfs.h>

#include <mach/rtk_ipc_shm.h>
/* -----------------------------------------------------
// 2014/12/30 define for replace IRRX-key map table 
// Usage:
// 1. put config file to box
// 2. init.rc run :  echo "/xx/xx/ir-keymap.conf" > /sys/devcies/18007000.irda/ir_replace_table
//
// === ir-keymap.conf === start ====
irrx-protocol=1
cust-code=0x7F80
scancode-msk=0x00FF0000
custcode-msk=0x0000FFFF
reg-ir-dpir=50
keymap-size=29
keymap-tbl=<
0x18=116
0x5A=353
0x58=154
0x1A=377
0x14=102
0x56=357
0x54=358
0x19=232
0x57=158
0x55=207
0x17=128
0x15=372
0x4F=168
0x4D=103
0x16=208
0x0C=105
0x4C=352
0x0E=106
0x08=412
0x48=108
0x09=407
0x4B=114
0x49=115
0x0B=532
0x0A=531
0xEEEE=0
0xEEEE=1
0xFFFF=272
0xFFFF=113
>
// === keymap table format === end ====
//-----------------------------------------*/

//
#define IR_STR_IRRX_PROTOCOL "irrx-protocol"
#define IR_STR_CUST_CODE	"cust-code"
#define IR_STR_SCANCODE_MSK "scancode-msk"
#define IR_STR_CUSTCODE_MSK "custcode-msk"
#define IR_STR_KEYMAP_SIZE 	"keymap-size"
#define IR_STR_KEYMAP_TBL 	"keymap-tbl"
#define IR_STR_RET_IR_DPIR 	"reg-ir-dpir"

#define REPEAT_MODE 0xFF
#define REPEAT_MODE_FIFO 1024
//#define REPEAT_MODE_SELFTEST
#define FIFO_DEPTH	16		/* driver queue depth */

#if defined(CONFIG_REALTEK_DARWIN) || defined(CONFIG_REALTEK_MACARTHUR)
//#define DEV_DEBUG
#endif /* CONFIG_REALTEK_DARWIN */

//#define DEV_DEBUG

extern long int Input_Keybit_Length;

#define DELAY_FIRST_REPEAT

/* Module Variables */
/*
 * Our parameters which can be set at load time.
 */

#define REPEAT_MAX_INTERVAL		200

#ifdef CONFIG_QUICK_HIBERNATION
extern int in_suspend;
#endif

enum {
	NEC = 1,
	RC5 = 2,
	SHARP = 3,
	SONY = 4,
	C03 = 5,
	RC6 = 6,
	RAW_NEC = 7,
	RCA = 8,
	PANASONIC = 9,
	KONKA=10, //wangzhh add ,this value must same with the AP layer 20120927
	RAW_RC6 = 11,
	RC6_MODE_6 = 12, //add for RC6 mode 6 
	TOSHIBA = 13, // add for Toshiba, is similar to NEC
};

enum {
	NEC_TX = 1,
	RC5_TX = 2,
	SHARP_TX = 3,
	SONY_TX = 4,
	C03_TX = 5,
	RC6_TX = 6,
	RCA_TX = 7,
	PANASONIC_TX = 8,
	KONKA_TX=9, //wangzhh add ,this value must same with the AP layer 20120927
};

enum {
	SINGLE_WORD_IF = 0,	// send IRRP only
	DOUBLE_WORD_IF = 1,	// send IRRP with IRSR together
};

static volatile unsigned int *reg_isr;
static volatile unsigned int *reg_soft_rst;
static volatile unsigned int *reg_ir_psr;
static volatile unsigned int *reg_ir_per;
static volatile unsigned int *reg_ir_sf;
static volatile unsigned int *reg_ir_dpir;
static volatile unsigned int *reg_ir_cr;
static volatile unsigned int *reg_ir_rp;
static volatile unsigned int *reg_ir_sr;
static volatile unsigned int *reg_ir_raw_ctrl;
static volatile unsigned int *reg_ir_raw_ff;
static volatile unsigned int *reg_ir_raw_sample_time;
static volatile unsigned int *reg_ir_raw_wl;
static volatile unsigned int *reg_ir_raw_deb;
static volatile unsigned int *reg_ir_ctrl_rc6;
static volatile unsigned int *reg_ir_rp2;
static volatile unsigned int *reg_irtx_cfg;
static volatile unsigned int *reg_irtx_tim;
static volatile unsigned int *reg_ritx_pwm_setting;
static volatile unsigned int *reg_irtx_int_en;
static volatile unsigned int *reg_irtx_int_st;
static volatile unsigned int *reg_irtx_fifo_st;
static volatile unsigned int *reg_irtx_fifo;
static volatile unsigned int *reg_irrcmm_timing;
static volatile unsigned int *reg_ir_cr1;
static volatile unsigned int *reg_irrcmm_apkb;
static volatile unsigned int *reg_irrxrclfifo;
/* static volatile unsigned int *reg_ir_raw_sf; */

static struct Venus_IRTx_KeycodeTable IRTx_KeycodeTable;
static int IRTx_KeycodeTableEnable=0;
static bool isKeypadTimerCreate=false;
static spinlock_t ir_splock;

static void venus_irtx_do_tasklet(unsigned long unused);
DECLARE_TASKLET(irtx_tasklet, venus_irtx_do_tasklet, 0);
static struct timer_list ir_wakeup_timer;

//#if defined(CONFIG_REALTEK_DARWIN) || defined(CONFIG_REALTEK_MACARTHUR)
static struct timer_list venus_ir_timer;
//#endif /* CONFIG_REALTEK_DARWIN */
static uint32_t lastSrValue = 0;

MODULE_AUTHOR("Chih-pin Wu, Realtek Semiconductor");
MODULE_LICENSE("GPL");

struct user_key_data{
	struct venus_key_table key_table;
	unsigned int max_size;
};

static struct venus_key rtk_mk5_tv_keys[] = {
	{0x18, KEY_POWER},			//POWER
	{0x5A, KEY_SELECT},			//SOURCE
	{0x58, KEY_CYCLEWINDOWS},	//PIP
	{0x1A, KEY_TV},				//TV SYSTEM
	{0x14, KEY_HOME},			//HOME
	{0x56, KEY_OPTION},			//OPTION MENU
	{0x54, KEY_INFO},				//MEDIA_INFO
	{0x19, KEY_REPLY},			//REPEAT
	{0x57, KEY_BACK},			//RETURN
	{0x55, KEY_PLAY},				//PLAY/PAUSE
	{0x17, KEY_STOP},			//STOP
	{0x15, KEY_ZOOM},			//ZOOM_IN
	{0x4F, KEY_REWIND},			//FR
	{0x4D, KEY_UP},				//UP
	{0x16, KEY_FASTFORWARD},	//FF
	{0x0C, KEY_LEFT},				//LEFT
	{0x4C, KEY_OK},				//OK
	{0x0E, KEY_RIGHT},			//RIGHT
	{0x08, KEY_PREVIOUS},		//PREVIOUS
	{0x48, KEY_DOWN},			//DOWN
	{0x09, KEY_NEXT},			//NEXT
	{0x4B, KEY_VOLUMEDOWN},		//VOL-
	{0x49, KEY_VOLUMEUP},		//VOL+
	{0x0B, KEY_TOUCHPAD_OFF},	//CURSOR
	{0x0A, KEY_TOUCHPAD_ON},	// GESTURE
	{0xEEEE,	REL_X},				//REL_X			//mouse X-axi	0xEEEE : REL event type
	{0xEEEE,	REL_Y},				//REL_Y			//mouse Y-axi	0xEEEE : REL event type
	{0xFFFF,	BTN_LEFT},			//BTN_LEFT		//mouse left key	0xFFFF : KEY event type

};


/*
static struct venus_key rtk_mk5_tv_keys[] = {
	{0x12, KEY_POWER},
	{0x11, KEY_EJECTCD},	// CMD_EJECT
	{0x10, KEY_1},
	{0x5B, KEY_2},
	{0x59, KEY_3},
	{0x1B, KEY_TV},	// CMD_SOURCE
	{0x18, KEY_4},
	{0x5A, KEY_5},
	{0x58, KEY_6},
	{0x1A, KEY_EPG},
	{0x14, KEY_7},
	{0x56, KEY_8},
	{0x54, KEY_9},
	{0x19, KEY_RECORD},	// CMD_FORCE_RECORD
	{0x57, KEY_SEARCH},	//widget
	{0x55, KEY_0},
	{0x17, KEY_ZOOM},
	//{0x15, KEY_FORCE_TIMESHIFT},	//CMD_FORCE_TIMESHIFT
	{0x4F, KEY_MENU},	// CMD_GUIDE
	{0x4D, KEY_UP},
	{0x16, KEY_BACK},	// CMD_RETURN
	{0x0C, KEY_LEFT},
	{0x4C, BTN_LEFT},
	{0x0E, KEY_RIGHT},
	{0x08, KEY_MENU},
	{0x48, KEY_DOWN},
	//{0x09, KEY_DISPLAY},	// CMD_DISPLAY
	{0x4B, KEY_REWIND},	// CMD_FRWD
	{0x49, KEY_PLAY},
	{0x0B, KEY_STOP},
	{0x0A, KEY_FASTFORWARD},	// CMD_FFWD
	{0x47, KEY_PREVIOUS},	// CMD_PREV
	{0x45, KEY_VOLUMEDOWN},		// CMD_VOL_DOWN
	{0x07, KEY_VOLUMEUP},	// CMD_VOL_UP
	{0x06, KEY_NEXT},
	{0x04, KEY_MUTE},
	//{0x46, KEY_PSCAN},	// CMD_PSCAN
	{0x44, KEY_SETUP},
	{0x05, KEY_SEARCH},
	{0x00, KEY_RED},	// CMD_OPTION_RED
	{0x42, KEY_GREEN},	// CMD_OPTION_GREEN
	{0x40, KEY_YELLOW},	// CMD_OPTION_YELLOW
	{0x01, KEY_BLUE},	// CMD_OPTION_BLUE
	{0x43, KEY_AUDIO},
	{0x41, KEY_SUBTITLE},	// CMD_STITLE
	//{0x03, KEY_REPEAT},	// CMD_REPEAT
	{0x02, KEY_SLOW}	// CMD_SFWD
};
*/

static struct  venus_key_table rtk_mk5_tv_key_table = {
	.keys = rtk_mk5_tv_keys,
	.size = ARRAY_SIZE(rtk_mk5_tv_keys),
	.cust_code = RTK_MK5_CUSTOMER_CODE,
	.scancode_msk = 0x00FF0000,
	.custcode_msk = 0xFFFF,
};
#if 0
#if defined(CONFIG_YK_54LU)
static struct venus_key yk_54lu_keys[] = {
	//{0x17e8, KEY_PICTURE_MODE},	// CMD_PICTURE_MODE
	{0x0af5, KEY_DELETE},
	{0x0cf3, KEY_POWER},
	//{0x18e7, KEY_SOUND_MODE},	// CMD_SOUND_MODE
	//{0x4fb0, KEY_DISPLAY_RATIO},	// CMD_DISPLAY_RATIO
	//{0x0bf4, KEY_INTRO},	// CMD_INTRO
	//{0x1ce3, KEY_VII},	// CMD_VII
	//{0x1ae5, KEY_FREEZE},	// CMD_FREEZE
	//{0x16e9, KEY_DISPLAY},	// CMD_DISPLAY
	{0x11ee, KEY_MENU},	// CMD_MENU
	{0x4db2, KEY_HOME},	//CMD_HOME
	//{0x0ff0, KEY_SOURCE},	//CMD_SOURCE
	{0x5ba4, KEY_BACK},	// CMD_RETURN
	//{0x51ae, KEY_SHORTCUT},	//CMD_SHORTCUT
	//{0x12ed, KEY_CHANNEL_INC},	// CMD_CHANNEL_INC
	//{0x13ec, KEY_CHANNEL_DEC},	// CMD_CHANNEL_DEC
	{0x0df2, KEY_MUTE},
	{0x14eb, KEY_VOLUMEUP},
	{0x15ea, KEY_VOLUMEDOWN},
	{0x4ab5, KEY_STOP},
	{0x43bc, KEY_DOWN},
	{0x44bb, KEY_LEFT},
	{0x45ba, KEY_RIGHT},
	{0x46b9, KEY_SELECT},
	{0x01fe, KEY_1},
	{0x02fd, KEY_2},
	{0x03fc, KEY_3},
	{0x04fb, KEY_4},
	{0x05fa, KEY_5},
	{0x06f9, KEY_6},
	{0x07f8, KEY_7},
	{0x08f7, KEY_8},
	{0x09f6, KEY_9},
	{0x00ff, KEY_0},
	{0x42bd, KEY_UP},
	{0x49b6, KEY_PLAY},
	{0x40bf, KEY_REWIND},	// CMD_FRWD
	{0x41be, KEY_FASTFORWARD},	//CMD_FFWD
	{0x47b8, KEY_PREVIOUS},	// CMD_PREV
	{0x48b7, KEY_NEXT},
	//{0x57a8, KEY_SKIP},	// CMD_CMSKIP
	//{0x59a6, KEY_INSTANT_REPLAY},	//CMD_INSTANT_REPLAY
	{0x58a7, KEY_SLOW},	// CMD_SFWD
	{0x5aa5, KEY_ZOOM},
	{0x53ac, KEY_HOME},	// CMD_GUIDE
	{0x7986, KEY_COFFEE},	// three display share key
};

static struct  venus_key_table yk_54lu_key_table = {
	.keys = yk_54lu_keys,
	.size = ARRAY_SIZE(yk_54lu_keys),
};
#endif

#if defined(CONFIG_RTK_MK5_2)
static struct venus_key rtk_mk5_2_tv_keys[] = {
	{0x5f, KEY_POWER},
	{0x5b, KEY_MUTE},
	{0x53, KEY_1},
	{0x50, KEY_2},
	{0x12, KEY_3},
	//{0x40, KEY_CHANNEL_INC},	// CMD_CHANNEL_INC
	{0x4f, KEY_4},
	{0x4c, KEY_5},
	{0x0e, KEY_6},
	//{0x5d, KEY_CHANNEL_DEC},	// CMD_CHANNEL_DEC
	{0x4b, KEY_7},
	{0x48, KEY_8},
	{0x0a, KEY_9},
	{0x03, KEY_VOLUMEUP},	// CMD_VOL_UP
	{0x47, KEY_DELETE},
	{0x44, KEY_0},
	{0x06, KEY_ENTER},	// CMD_ENTER
	{0x1f, KEY_VOLUMEDOWN},		// CMD_VOL_DOWN
	//{0x57, KEY_SOURCE},	//CMD_SOURCE
	//{0x08, KEY_TOOLS},	// CMD_TOOLS
	{0x59, KEY_UP},
	{0x51, KEY_DOWN},
	{0x56, KEY_LEFT},
	{0x14, KEY_RIGHT},
	{0x55, KEY_ENTER},
	//{0x46, KEY_PIP},	// CMD_PIP
	//{0x45, KEY_POP},	// CMD_POP
	{0x42, KEY_HOME},	// CMD_GUIDE
	//{0x41, KEY_MP},	// CMD_MP
	//{0x52, KEY_JUMP},	// CMD_JUMP
	{0x10, KEY_BACK},	// CMD_RETURN
	{0x4e, KEY_RED},	// CMD_OPTION_RED
	{0x4d, KEY_GREEN},	// CMD_OPTION_GREEN
	{0x0c, KEY_YELLOW},	// CMD_OPTION_YELLOW
	{0x0d, KEY_BLUE},	// CMD_OPTION_BLUE
	//{0x01, KEY_FAVCH},	// CMD_FAVCH
	{0x4a, KEY_EPG},	// CMD_EPG
	//{0x49, KEY_PROGINFO},	// CMD_PROGINFO
	{0x09, KEY_MENU},	// CMD_MENU
	{0x43, KEY_SLEEP},
	//{0xe41b, KEY_DISPLAY},	// CMD_DISPLAY
	//{0x1a, KEY_WIDE},	// CMD_WIDE
	//{0x54, KEY_CAPTION},	// CMD_CAPTION
	{0x17, KEY_VIDEO},
	{0x16, KEY_AUDIO},
	//{0x05, KEY_ROI},	// CMD_ROI
	//{0x04, KEY_FREEZE},	// CMD_FREEZE
	//{0x00, KEY_CARD_READER},	// CMD_CARD_READER
};

static struct  venus_key_table rtk_mk5_2_tv_key_table = {
	.keys = rtk_mk5_2_tv_keys,
	.size = ARRAY_SIZE(rtk_mk5_2_tv_keys),
};
#endif
#endif

static bool bUseUserTable = false;

/*
static struct user_key_data user_key = {
	.key_table = {
		.keys = NULL,
		.size = 0,
	},
	.max_size = 0,
};
*/
static struct user_key_data rtk_mk5_user_keys = {
	.key_table = {
		.keys = NULL,
		.size = 0,
	},
	.max_size = 0,
};

//#if defined(CONFIG_YK_54LU)
static struct user_key_data yk_54lu_user_keys = {
	.key_table = {
		.keys = NULL,
		.size = 0,
	},
	.max_size = 0,
};
//#endif

//#if defined(CONFIG_RTK_MK5_2)
static struct user_key_data rtk_mk5_2_user_keys = {
	.key_table = {
		.keys = NULL,
		.size = 0,
	},
	.max_size = 0,
};
//#endif

static struct user_key_data libra_ms_user_keys = {
	.key_table = {
		.keys = NULL,
		.size = 0,
	},
	.max_size = 0,
};

static struct user_key_data jaecs_t118_user_keys = {
	.key_table = {
		.keys = NULL,
		.size = 0,
	},
	.max_size = 0,
};

static struct user_key_data rtk_mk3_user_keys = {
	.key_table = {
		.keys = NULL,
		.size = 0,
	},
	.max_size = 0,
};

static struct user_key_data yk_70lb_user_keys = {
	.key_table = {
		.keys = NULL,
		.size = 0,
	},
	.max_size = 0,
};

static struct user_key_data rtk_mk4_user_keys = {
	.key_table = {
		.keys = NULL,
		.size = 0,
	},
	.max_size = 0,
};

static struct user_key_data netg_ms_user_keys = {
	.key_table = {
		.keys = NULL,
		.size = 0,
	},
	.max_size = 0,
};

static struct venus_key_table* p_rtk_key_table = &rtk_mk5_tv_key_table;
struct device_node *rtk119x_irda_node = NULL;
static int ir_protocol = NEC;
static int irtx_protocol = NEC_TX;
static u32	dpir_val = 60;
int rpmode = 0;
int rpmode_fifosize = 0;
unsigned char *rpmode_txfifo;
unsigned char *rpmode_rxfifo;
int start_tx = 0;
static int sample_rate = 0;
static unsigned int lastRecvMs;
static unsigned int debounce = 300;
static unsigned int driver_mode = SINGLE_WORD_IF;
static unsigned int rtk119x_ir_irq;
static bool RPRepeatIsHandling = false;
//bool f_GestureEnable = true;
bool f_CursorEnable = false;
static void __iomem *iso_sys_reg_base;
static void __iomem *iso_irda_reg_base;
#ifdef DELAY_FIRST_REPEAT
static unsigned int PressRecvMs;
static unsigned int firstRepeatInterval = 300; // default disable first repeat delay
static unsigned int firstRepeatTriggered = false;
#endif
module_param(ir_protocol, int, S_IRUGO);
module_param(irtx_protocol, int, S_IRUGO);

static void _venus_ir_get_user_data(uint32_t, struct user_key_data **, u32 *);
static void _venus_ir_clean_user_keys(struct user_key_data *);
void dump_keymap_tbl(void);
int Venus_IR_Init(int mode, int tx_mode);

static unsigned int irda_dev_count =1;	/* for multiple support 2015.04.22*/

void	venus_ir_set_keybit(unsigned long *addr)
{
	int i;
	for (i = 0; i <p_rtk_key_table->size; i++) {
		if(p_rtk_key_table->keys[i].scancode != 0xEEEE)		//not EV_REL type
			set_bit(p_rtk_key_table->keys[i].keycode, addr);
	}

#if 0
	for (i = 0; i < rtk_mk5_tv_key_table.size; i++) {
		set_bit(rtk_mk5_tv_key_table.keys[i].keycode, addr);
	}
	set_bit(BTN_LEFT, addr);


#if defined(CONFIG_YK_54LU)
	for (i = 0; i < yk_54lu_key_table.size; i++) {
		set_bit(yk_54lu_key_table.keys[i].keycode, addr);
	}
#endif

#if defined(CONFIG_RTK_MK5_2)
	for (i = 0; i < rtk_mk5_2_tv_key_table.size; i++) {
		set_bit(rtk_mk5_2_tv_key_table.keys[i].keycode, addr);
	}
#endif
#endif
}

void	venus_ir_set_relbit(unsigned long *addr)
{

	int i;
	for (i = 0; i <p_rtk_key_table->size; i++) {
		if(p_rtk_key_table->keys[i].scancode == 0xEEEE)		//is EV_REL type
			set_bit(p_rtk_key_table->keys[i].keycode, addr);
	}
//	set_bit(REL_X, addr);
//	set_bit(REL_Y, addr);
}

static void venus_ir_clean_user_keys(void)
{
	_venus_ir_clean_user_keys(&rtk_mk5_user_keys);
//#if defined(CONFIG_YK_54LU)
	_venus_ir_clean_user_keys(&yk_54lu_user_keys);
//#endif
//#if defined(CONFIG_RTK_MK5_2)
	_venus_ir_clean_user_keys(&rtk_mk5_2_user_keys);
//#endif
	_venus_ir_clean_user_keys(&libra_ms_user_keys);
	_venus_ir_clean_user_keys(&jaecs_t118_user_keys);
	_venus_ir_clean_user_keys(&rtk_mk3_user_keys);
	_venus_ir_clean_user_keys(&yk_70lb_user_keys);
	_venus_ir_clean_user_keys(&rtk_mk4_user_keys);
	_venus_ir_clean_user_keys(&netg_ms_user_keys);
}

int venus_ir_IrMessage2Keycode(uint32_t message, u32* p_keycode)
{
	u32 scancode;
	int i,k=0,count=0;
	int custcode_off=0x0;
	RTK_debug(KERN_ALERT "[%s]%s  %d : begin ----bUseUserTable=%d, message=0x%x \n",__FILE__, __FUNCTION__,__LINE__, bUseUserTable, message);

	if(bUseUserTable)
	{
		struct user_key_data *pUserData;
		_venus_ir_get_user_data(message, &pUserData, &scancode);
		if(pUserData == NULL) {
			return -1;
		}
		RTK_debug(KERN_ALERT "[%s]%s  %d :scancode=0x%x \n",__FILE__, __FUNCTION__,__LINE__, scancode);

		if(pUserData->key_table.size > 0 && pUserData->key_table.keys) {

			RTK_debug(KERN_ALERT "[%s]%s  %d search in user mappint table\n",__FILE__, __FUNCTION__,__LINE__);
			RTK_debug(KERN_ALERT "[%s]%s  %d table size=%d",__FILE__, __FUNCTION__,__LINE__,pUserData->key_table.size);
			for(i=0; i<pUserData->key_table.size; i++) {

				RTK_debug(KERN_ALERT "[%s]%s  %d   i=%d, scancode=%d, keycode=%d \n",
					__FILE__, __FUNCTION__,__LINE__,i, pUserData->key_table.keys[i].scancode, pUserData->key_table.keys[i].keycode);

				if(scancode == pUserData->key_table.keys[i].scancode)	{
					// mapping found, report key and return
					*p_keycode = pUserData->key_table.keys[i].keycode;
					RTK_debug(KERN_ALERT "[%s] %s  %d  report key 0x%x %d\n",__FILE__,__FUNCTION__,__LINE__, scancode, *p_keycode);
					return 0;
				}
			}
		}
		// no mapping found, just return
		return -1;
	}
	
	for(count=0;count<irda_dev_count;count++)	/* for multiple support 2015.04.22*/
	{
		while(k<32)
		{
			if((0x1<<k)&(p_rtk_key_table[count].custcode_msk))
			{
				custcode_off = k;
				break;
			}
			k++;
		}
		
		RTK_debug(KERN_ALERT "[%s]  %s  %d    custcode_off=0x%08x \n",__FILE__,__FUNCTION__,__LINE__, custcode_off);
	
		// use default mapping tables
		if (((message & p_rtk_key_table[count].custcode_msk)>>custcode_off) == p_rtk_key_table[count].cust_code) {
			int scancode_off=0x0, j=0x0;
			while(j<32)
			{
				if((0x1<<j)&(p_rtk_key_table[count].scancode_msk))
				{
					scancode_off = j;
					break;
				}
				j++;
			}
			if(j>=32) {
				pr_warn("]%s Parsing DTB fail, scancode_msk = %d may be wrong number.\n", __func__, p_rtk_key_table[count].custcode_msk);
			}
	
			RTK_debug(KERN_ALERT "[%s]  %s  %d    scancode_off=0x%08x \n",__FILE__,__FUNCTION__,__LINE__, scancode_off);
	
			scancode = (message & p_rtk_key_table[count].scancode_msk) >> scancode_off;
			for (i = 0; i < p_rtk_key_table[count].size; i++) {
				if (scancode == p_rtk_key_table[count].keys[i].scancode) {
					*p_keycode = p_rtk_key_table[count].keys[i].keycode;
					RTK_debug(KERN_ALERT "[%s]  %s  %d    report key scancode=0x%x keycode=%d\n",__FILE__,__FUNCTION__,__LINE__, scancode, *p_keycode);
					return 0;
				}
			}
		}
	}


#if 0
	if ((message & 0x0000ffff) == RTK_MK5_CUSTOMER_CODE) {
		scancode = (message & 0x00ff0000) >> 16;
		for (i = 0; i < rtk_mk5_tv_key_table.size; i++) {
			if (scancode == rtk_mk5_tv_key_table.keys[i].scancode) {
				*p_keycode = rtk_mk5_tv_key_table.keys[i].keycode;
				RTK_debug(KERN_ALERT "[%s]  %s  %d    report key 0x%x %d\n",__FILE__,__FUNCTION__,__LINE__, scancode, *p_keycode);
				return 0;
			}
		}
	}

#if defined(CONFIG_YK_54LU)
	if ((message & 0x0000ffff) == YK_54LU_CUSTOMER_CODE) {
		scancode = (message & 0xffff0000) >> 16;
		for (i = 0; i < yk_54lu_key_table.size; i++) {
			if (scancode == yk_54lu_key_table.keys[i].scancode) {
				*p_keycode = yk_54lu_key_table.keys[i].keycode;
				RTK_debug(KERN_ALERT "[%s] %s  %d    report key 0x%x %d\n",__FILE__,__FUNCTION__,__LINE__, scancode, *p_keycode);
				return 0;
			}
		}
	}
#endif

#if defined(CONFIG_RTK_MK5_2)
	if ((message & 0x0000ffff) == RTK_MK5_2_CUSTOMER_CODE) {
		scancode = (message & 0x00ff0000) >> 16;
		for (i = 0; i < rtk_mk5_2_tv_key_table.size; i++) {
			if (scancode == rtk_mk5_2_tv_key_table.keys[i].scancode) {
				*p_keycode = rtk_mk5_2_tv_key_table.keys[i].keycode;
				RTK_debug(KERN_ALERT "[%s] %s  %d    report key 0x%x %d\n",__FILE__,__FUNCTION__,__LINE__, scancode, *p_keycode);
				return 0;
			}
		}
	}
#endif
#endif

	return -1;

}


static void _venus_ir_get_user_data(uint32_t message, struct user_key_data **ppUsrData, u32 *pCode)
{
	if ((message & 0x0000ffff) == RTK_MK5_CUSTOMER_CODE) {

		RTK_debug(KERN_ALERT "match RTK_MK5_CUSTOMER_CODE");

		*ppUsrData = &rtk_mk5_user_keys;
		*pCode = (message & 0x00ff0000)>>16;
	}
	else
//#if defined(CONFIG_YK_54LU)
	if ((message & 0x0000ffff) == YK_54LU_CUSTOMER_CODE) {
		*ppUsrData = &yk_54lu_user_keys;
		*pCode = (message & 0xffff0000)>>16;
	}
	else
//#endif
//#if defined(CONFIG_RTK_MK5_2)
	if ((message & 0x0000ffff) == RTK_MK5_2_CUSTOMER_CODE) {
		*ppUsrData = &rtk_mk5_2_user_keys;
		*pCode = (message & 0x00ff0000)>>16;
	}
	else
//#endif
	if ((message & 0x0000ffff) == LIBRA_MS_CUSTOMER_CODE) {
		*ppUsrData = &libra_ms_user_keys;
		*pCode = (message & 0x00ff0000)>>16;
	}
	else
	if ((message & 0x0000ffff) == JAECS_T118_CUSTOMER_CODE) {
		*ppUsrData = &jaecs_t118_user_keys;
		*pCode = (message & 0x00ff0000)>>16;
	}
	else
	if ((message & 0x0000ffff) == RTK_MK3_CUSTOMER_CODE) {
		*ppUsrData = &rtk_mk3_user_keys;
		*pCode = (message & 0x00ff0000)>>16;
	}
	else
	if ((message & 0x0000ffff) == YK_70LB_CUSTOMER_CODE) {
		*ppUsrData = &yk_70lb_user_keys;
		*pCode = (message & 0xffff0000)>>16;
	}
	else
	if ((message & 0x0000ffff) == RTK_MK4_CUSTOMER_CODE) {
		*ppUsrData = &rtk_mk4_user_keys;
		*pCode = (message & 0x00ff0000)>>16;
	}
	else
	if ((message & 0x0000ffff) == NETG_MS_CUSTOMER_CODE) {
		*ppUsrData = &netg_ms_user_keys;
		*pCode = (message & 0x00007c0)>>6;
	}
	else
	{
		*ppUsrData = NULL;
		*pCode = 0;
	}
}

static void _venus_ir_clean_user_keys(struct user_key_data *pUserKeys)
{
	if(pUserKeys->key_table.keys && pUserKeys->max_size>0) {
		kfree(pUserKeys->key_table.keys);
		pUserKeys->key_table.keys = NULL;
	}
	pUserKeys->key_table.size = 0;
	pUserKeys->max_size = 0;
}

static bool _venus_ir_checkkeycode(struct user_key_data *pUserKeys, unsigned int keycode)
{
	int i;
	for (i = 0; i < pUserKeys->key_table.size; i++) {
		if (pUserKeys->key_table.keys[i].keycode == keycode) {
			return true;
		}
	}
	return false;
}

void venus_ir_enable_irtx(void)
{
	uint32_t regValue;
	regValue =	ioread32(reg_irtx_cfg);
	iowrite32((regValue | 0x1), reg_irtx_cfg); //irtx stop
	regValue =	ioread32(reg_irtx_cfg);
	RTK_debug(KERN_ALERT "venus_ir_disable_irtx : set reg_irtx_cfg=0x%08X (disable Tx by ~0x1)\n" , regValue);
}

void venus_ir_disable_irtx(void)
{
	uint32_t regValue;
	regValue =	ioread32(reg_irtx_cfg);
	iowrite32((regValue & ~0x1), reg_irtx_cfg); //irtx stop
	regValue =	ioread32(reg_irtx_cfg);
	RTK_debug(KERN_ALERT "venus_ir_disable_irtx : set reg_irtx_cfg=0x%08X (disable Tx by ~0x1)\n" , regValue);
}


void venus_ir_enable_irrp(void)
{
	uint32_t regValue;	
	iowrite32(0x5df, reg_ir_cr);	//IRRC enable
	regValue =	ioread32(reg_ir_cr);
	RTK_debug(KERN_ALERT "venus_ir_enable_irrp : set IRRC enable! set reg_ir_cr=0x%08X, \n" , regValue);
}

void venus_ir_disable_irrp(void)
{
	uint32_t regValue;
	regValue =	ioread32(reg_ir_cr);
	iowrite32((regValue & ~0x100), reg_ir_cr);	//irrx stop
	regValue =	ioread32(reg_ir_cr);
	RTK_debug(KERN_ALERT "venus_ir_disable_irrp: IRRC unit disable! set reg_ir_cr=0x%08X\n", regValue);
}

int venus_ir_setkeycode(unsigned int scancode, unsigned int new_keycode, unsigned int *p_old_keycode, unsigned long * keybit_addr)
{
	int i;
	int OldKeyCode = KEY_CNT;
	struct user_key_data *pUsrData;
	u32 ir_scancode;
	RTK_debug(KERN_ALERT "[venus_ir_setkeycode]\n");

	_venus_ir_get_user_data(scancode, &pUsrData, &ir_scancode);
	if(pUsrData == NULL) {
		printk(KERN_ALERT "user keys not found for %d", scancode);
		return 0;
	}
	bUseUserTable = true;

	RTK_debug(KERN_ALERT "[venus_ir_setkeycode]size=%d, max=%d", pUsrData->key_table.size, pUsrData->max_size);

	for(i=0; i<pUsrData->key_table.size; i++) {
		if(ir_scancode == pUsrData->key_table.keys[i].scancode) {

			RTK_debug(KERN_ALERT "[venus_ir_setkeycode]found keycode, scancode=%d, keycode=%d", ir_scancode, pUsrData->key_table.keys[i].keycode);

			//mapping found, set to new keycode
			OldKeyCode = pUsrData->key_table.keys[i].keycode;
			pUsrData->key_table.keys[i].keycode = new_keycode;
			break;
		}
	}

 		if(OldKeyCode != KEY_CNT) {
 			__clear_bit(OldKeyCode, keybit_addr);
 			__set_bit(new_keycode, keybit_addr);

 			if(
 			   _venus_ir_checkkeycode(&rtk_mk5_user_keys, OldKeyCode) ||
 	//#if defined(CONFIG_YK_54LU)
 			   _venus_ir_checkkeycode(&yk_54lu_user_keys, OldKeyCode) ||
 	//#endif
 	//#if defined(CONFIG_RTK_MK5_2)
 			   _venus_ir_checkkeycode(&rtk_mk5_2_user_keys , OldKeyCode) ||
 	//#endif
 			   _venus_ir_checkkeycode(&libra_ms_user_keys, OldKeyCode) ||
 			   _venus_ir_checkkeycode(&jaecs_t118_user_keys, OldKeyCode) ||
 			   _venus_ir_checkkeycode(&rtk_mk3_user_keys, OldKeyCode) ||
 			   _venus_ir_checkkeycode(&yk_70lb_user_keys, OldKeyCode) ||
 			   _venus_ir_checkkeycode(&rtk_mk4_user_keys, OldKeyCode) ||
 			   _venus_ir_checkkeycode(&netg_ms_user_keys, OldKeyCode) ||
 			   false) {
 				__set_bit(OldKeyCode, keybit_addr);
 			}
			*p_old_keycode = OldKeyCode;
 			return 0;
 		}

	RTK_debug(KERN_ALERT "[setkeycode]keycode not found");

	if(pUsrData->key_table.size == pUsrData->max_size) {
		//realloc for key table
		struct venus_key *pNew;
		RTK_debug(KERN_ALERT "[setkeycode]try to realloc memory for mappint table, new size =%d", pUsrData->max_size+10);

		pNew = (struct venus_key *)krealloc(pUsrData->key_table.keys,
			                                (pUsrData->max_size+10) * sizeof(struct venus_key),
			                                GFP_ATOMIC);
		if(!pNew)
			return -ENOMEM;

		pUsrData->key_table.keys = pNew;
		pUsrData->max_size+=10;

		if(pUsrData->key_table.size == 0) {
			// first time set, disable default key map, clear keybit table
			memset(keybit_addr, 0, Input_Keybit_Length);
		}

	}


	RTK_debug(KERN_ALERT "[setkeycode]add mapping, scancode=%d, new_keycode=%d", ir_scancode, new_keycode);

	pUsrData->key_table.keys[pUsrData->key_table.size].scancode = ir_scancode;
	pUsrData->key_table.keys[pUsrData->key_table.size].keycode = new_keycode;
	pUsrData->key_table.size++;
	__set_bit(new_keycode, keybit_addr);

	*p_old_keycode = new_keycode;

	return 0;
}




int venus_ir_getkeycode(u32 scancode, u32* p_keycode)
{
	struct user_key_data *pUsrData;
	int i;
	u32 ir_scancode;
	RTK_debug(KERN_ALERT "[venus_ir_getkeycode]\n");

	if(bUseUserTable == false)
		return -1;

	_venus_ir_get_user_data(scancode, &pUsrData, &ir_scancode);
	if(pUsrData == NULL)
		return -1;

	for(i=0; i<pUsrData->key_table.size; i++) {
		if(ir_scancode == pUsrData->key_table.keys[i].scancode) {
			//mapping found
			*p_keycode = pUsrData->key_table.keys[i].keycode;
			RTK_debug(KERN_ALERT "[venus_ir_getkeycode]scancode=%d, keycode=%d", ir_scancode, pUsrData->key_table.keys[i].keycode);
			return 0;
		}
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define IRTX_CUSTOMER_CODE RTK_MK5_CUSTOMER_CODE
//#define IRTX_PROTOCOL RTK_MK5_CUSTOMER_CODE

static int _venus_irtx_nec_send(u32 scancode, u32 customer_code)
{
	volatile u8 AGC_burs_reg[3], NecAddr_reg[6], NecCmd_reg[6], NecCmdEnd_reg;
	u16 NecAddr, NecCmd;
	unsigned int i;
	volatile int reg_bit_offset;
	volatile u32 reg_val;

	RTK_debug(KERN_ALERT "{IRTX]_venus_irtx_nec_send : --- begin --- tx reg_bit_offset of NecAddr_reg is negtive \n");

	NecAddr = (u16)(customer_code & 0x0000ffff);
	NecCmd = (u16)(((~(scancode&0x000000ff))<<8)  |  (scancode&0x000000ff));
	RTK_debug(KERN_ALERT "{IRTX] NecAddr = %X  NecCmd = %X \n",NecAddr,NecCmd);
	memset ((void*)AGC_burs_reg, 0, sizeof(AGC_burs_reg));
	memset ((void*)NecAddr_reg, 0, sizeof(NecAddr_reg));
	memset ((void*)NecCmd_reg, 0, sizeof(NecCmd_reg));

	AGC_burs_reg[2] = 0xff;
	AGC_burs_reg[1] = 0xff;
	AGC_burs_reg[0] = 0x00;
	NecCmdEnd_reg= 0x80;

	reg_bit_offset = (8*sizeof(NecAddr_reg))-1;
	for (i=0; i<(8*sizeof(NecAddr)); i++)
	{
		if (reg_bit_offset<0)
		{
			RTK_debug(KERN_ALERT "{IRTX]_venus_irtx_nec_send: ERROR : tx reg_bit_offset of NecAddr_reg is negtive \n");
			return -1;
		}else if(NecAddr & (0x1<<i)){
			RTK_debug(KERN_DEBUG "{IRTX]_venus_irtx_nec_send: (1) reg_bit_offset = %x, NecAddr_reg=%x \n" ,reg_bit_offset, (unsigned int)NecAddr_reg);
			__set_bit(reg_bit_offset, (unsigned long *)NecAddr_reg);
			reg_bit_offset= reg_bit_offset-4;
		}else{
			RTK_debug(KERN_DEBUG "{IRTX]_venus_irtx_nec_send: (2) reg_bit_offset = %x, NecAddr_reg=%x \n" ,reg_bit_offset, (unsigned int)NecAddr_reg);
			__set_bit(reg_bit_offset, (unsigned long *)NecAddr_reg);
			reg_bit_offset= reg_bit_offset-2;
		}
	}


	reg_bit_offset = (8*sizeof(NecCmd_reg))-1;
	for (i=0; i<(8*sizeof(NecCmd)); i++)
	{
		if (reg_bit_offset<0)
		{
			RTK_debug(KERN_ALERT "{IRTX]_venus_irtx_nec_send: ERROR : tx reg_bit_offset of NecAddr_reg is negtive \n");
			return -1;
		}
		if(NecCmd & (0x1<<i)){
			RTK_debug(KERN_DEBUG "{IRTX]_venus_irtx_nec_send: (3) reg_bit_offset = %x, NecCmd_reg=%x \n" ,reg_bit_offset, (unsigned int)NecCmd_reg);
			__set_bit(reg_bit_offset, (unsigned long *)NecCmd_reg);
			reg_bit_offset= reg_bit_offset-4;
		}else{
			RTK_debug(KERN_DEBUG "{IRTX]_venus_irtx_nec_send: (4) reg_bit_offset = %x, NecCmd_reg=%x \n" ,reg_bit_offset, (unsigned int)NecCmd_reg);
			__set_bit(reg_bit_offset, (unsigned long *)NecCmd_reg);
			reg_bit_offset= reg_bit_offset-2;
		}
	}

	reg_val = ((AGC_burs_reg[2]<<24) | (AGC_burs_reg[1]<<16) |(AGC_burs_reg[0]<<8) | NecAddr_reg[5] );
	RTK_debug(KERN_DEBUG "{IRTX]_venus_irtx_nec_send:reg_irtx_fifo ---- (5) reg_value = %x\n", reg_val);
	iowrite32( reg_val,  reg_irtx_fifo);

	reg_val = ((NecAddr_reg[4]<<24) | (NecAddr_reg[3]<<16) |(NecAddr_reg[2]<<8) | NecAddr_reg[1] );
	RTK_debug(KERN_DEBUG "{IRTX]_venus_irtx_nec_send:reg_irtx_fifo ---- (6) reg_value = %x\n", reg_val);
	iowrite32( reg_val,  reg_irtx_fifo);

	reg_val = ((NecAddr_reg[0]<<24) | (NecCmd_reg[5]<<16) |(NecCmd_reg[4]<<8) | NecCmd_reg[3] );
	RTK_debug(KERN_DEBUG "{IRTX]_venus_irtx_nec_send:reg_irtx_fifo ---- (7) reg_value = %x\n", reg_val);
	iowrite32( reg_val,  reg_irtx_fifo);

	reg_val = ((NecCmd_reg[2]<<24) | (NecCmd_reg[1]<<16) |(NecCmd_reg[0]<<8) | NecCmdEnd_reg );
	RTK_debug(KERN_DEBUG "{IRTX]_venus_irtx_nec_send:reg_irtx_fifo ---- (8)  reg_value = %x\n", reg_val);
	iowrite32( reg_val,  reg_irtx_fifo);

	RTK_debug(KERN_ALERT "{IRTX]_venus_irtx_nec_send: end !\n");
	return 0;

}

static int _venus_irtx_send_start(u32 scancode, u32 customer_code)
{
	u32 regValue;

	RTK_debug(KERN_ALERT " %s  %s  %d --- begin : irtx_protocol=0x%x, scancode=0x%x\n" ,__FILE__ ,__FUNCTION__, __LINE__,irtx_protocol,scancode);

	if (irtx_protocol== NEC_TX) {
		if ( -1 == _venus_irtx_nec_send(scancode, customer_code))
		{
			RTK_debug(KERN_ALERT " %s  %s  %d \n" ,__FILE__ ,__FUNCTION__, __LINE__);
			return -1;
		}
	}
	else
	{
		RTK_debug(KERN_ALERT " %s  %s  %d \n" ,__FILE__ ,__FUNCTION__, __LINE__);
		return -1;
	}
	regValue = ioread32(reg_irtx_cfg);  //get irtx_cfg
	
	RTK_debug(KERN_DEBUG " %s  %s  %d ---start tx transmittion! begin ! (get reg_irtx_cfg=0x%08X. set to 0x80000711) !\n" ,__FILE__ ,__FUNCTION__, __LINE__,regValue);
	iowrite32(0x80000711, reg_irtx_cfg);  //start Tx
	
	regValue = ioread32(reg_irtx_cfg);  //get irtx_cfg
	RTK_debug(KERN_DEBUG " %s  %s  %d ---start tx transmittion! end (re-get reg_irtx_cfg=0x%08X) !\n" ,__FILE__ ,__FUNCTION__, __LINE__,regValue);

#if 0	//iso interrupt is fix
	/*TODO : fix iso interrupt by mux */
#if 0
	while(timeout_cnt)
	{
		if (ioread32(reg_irtx_int_st) | 0x4)
			break;
		timeout_cnt --;
	}
#else
	mdelay(1000);
#endif
	iowrite32(0x80000710, reg_irtx_cfg);
	if (!timeout_cnt)
	{
		RTK_debug(KERN_ALERT " %s  %s  %d timeout_cnt = %x\n" ,__FILE__ ,__FUNCTION__, __LINE__, timeout_cnt);
		return -1;
	}
	RTK_debug(KERN_ALERT " %s  %s  %d timeout_cnt = %x\n" ,__FILE__ ,__FUNCTION__, __LINE__,timeout_cnt);
#endif

	return 0;
}


static void _venus_irtx_get_user_data(struct user_key_data **ppUsrData)
{
	if (IRTX_CUSTOMER_CODE == RTK_MK5_CUSTOMER_CODE) {
		RTK_debug(KERN_ALERT "match RTK_MK5_CUSTOMER_CODE");
		*ppUsrData = &rtk_mk5_user_keys;
	}
	else
//#if defined(CONFIG_YK_54LU)
	if (IRTX_CUSTOMER_CODE == YK_54LU_CUSTOMER_CODE) {
		*ppUsrData = &yk_54lu_user_keys;
	}
	else
//#endif
//#if defined(CONFIG_RTK_MK5_2)
	if (IRTX_CUSTOMER_CODE == RTK_MK5_2_CUSTOMER_CODE) {
		*ppUsrData = &rtk_mk5_2_user_keys;
	}
	else
//#endif
	if (IRTX_CUSTOMER_CODE == LIBRA_MS_CUSTOMER_CODE) {
		*ppUsrData = &libra_ms_user_keys;
	}
	else
	if (IRTX_CUSTOMER_CODE == JAECS_T118_CUSTOMER_CODE) {
		*ppUsrData = &jaecs_t118_user_keys;
	}
	else
	if (IRTX_CUSTOMER_CODE == RTK_MK3_CUSTOMER_CODE) {
		*ppUsrData = &rtk_mk3_user_keys;
	}
	else
	if (IRTX_CUSTOMER_CODE == YK_70LB_CUSTOMER_CODE) {
		*ppUsrData = &yk_70lb_user_keys;
	}
	else
	if (IRTX_CUSTOMER_CODE == RTK_MK4_CUSTOMER_CODE) {
		*ppUsrData = &rtk_mk4_user_keys;
	}
	else
	if ((IRTX_CUSTOMER_CODE & 0x0000ffff) == NETG_MS_CUSTOMER_CODE) {
		*ppUsrData = &netg_ms_user_keys;
	}
	else
	{
		*ppUsrData = NULL;
	}
}


ssize_t irda_show_ir_replace_table(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t irda_store_ir_replace_table(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count);

static DEVICE_ATTR(ir_replace_table, S_IRUGO | S_IWUSR, irda_show_ir_replace_table, irda_store_ir_replace_table);

static struct attribute *irda_dev_attrs[] = {
	&dev_attr_ir_replace_table.attr,
	NULL,
};

static struct attribute_group irda_attr_group = {
	.attrs		= irda_dev_attrs,
};


ssize_t irda_show_ir_replace_table(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);

	RTK_debug(KERN_ALERT "--- debug : irda_show_ir_replace_table called : table size=%d---- \n", p_rtk_key_table->size);

	if( p_rtk_key_table==NULL)
		return sprintf(buf, "IR_ERR: key table is not exist!\n");

	return sprintf(buf, "table size=%d\n", p_rtk_key_table->size);
}

ssize_t irda_store_ir_replace_table(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	char filename[256]={0};
	char buffer[1024]={0};
	struct file *filep;
	char *token, *variable_token, *cur = buffer;
	u32 idx=0;
	u32 keymap_size=p_rtk_key_table->size;
	struct venus_key_table* new_p_rtk_key_table;

	if(buf ==NULL || count < 1) {
		pr_err("IR_ERR : irda_store_ir_replace_table	====  buffer is null! \n");
		return count;
	}
	sscanf(buf, "%s", filename);
	pr_info("irda_store_ir_replace_table: get filename=[%s] \n", filename);

	filep=filp_open(filename, O_RDONLY, 0);
	if(IS_ERR(filep)){
		pr_info("irda_store_ir_replace_table: open filename=[%s] fail!! \n", filename);
		return count;
	}

	if (kernel_read(filep, 0, buffer, 1024) < 0) {
		pr_err("irda_store_ir_replace_table: read file error?\n");
		goto irda_error1;
	}
#ifdef DEV_DEBUG
	dump_keymap_tbl();
#endif
	// NOTE : parsing file 
	new_p_rtk_key_table = (struct venus_key_table*)kzalloc(sizeof(struct venus_key_table), GFP_KERNEL);

	while (token = strsep(&cur, "\n")) {
		//pr_info("token => [%s] \n", token);
		variable_token = strsep(&token, "=");
		//pr_info("[%s] [%s]\n", variable_token, token);
		if( strcmp(variable_token, IR_STR_IRRX_PROTOCOL) == 0 ) {
			sscanf(token,"%d", &ir_protocol);
			pr_info(" :%s=[%d] \n", IR_STR_IRRX_PROTOCOL, ir_protocol);
		}
		else if ( strcmp(variable_token, IR_STR_CUST_CODE) == 0 ) {
			sscanf(token,"0x%X",  &(new_p_rtk_key_table->cust_code));
			pr_info(" :%s=[0x%X] \n", IR_STR_CUST_CODE, new_p_rtk_key_table->cust_code);
		}
		else if ( strcmp(variable_token, IR_STR_SCANCODE_MSK) == 0 ) {
			sscanf(token,"0x%08X",  &(new_p_rtk_key_table->scancode_msk));
			pr_info(" :%s=[0x%08X] \n", IR_STR_SCANCODE_MSK, new_p_rtk_key_table->scancode_msk);
		}
		else if ( strcmp(variable_token, IR_STR_CUSTCODE_MSK) == 0 ) {
			sscanf(token,"0x%08X",  &(new_p_rtk_key_table->custcode_msk));
			pr_info(" :%s=[0x%08X] \n", IR_STR_CUSTCODE_MSK, new_p_rtk_key_table->custcode_msk);
		}
		else if ( strcmp(variable_token, IR_STR_RET_IR_DPIR) == 0 ) {
			sscanf(token,"%d", &dpir_val);
			pr_info(" :%s=[%d] \n", IR_STR_RET_IR_DPIR, dpir_val);
		}
		else if ( strcmp(variable_token, IR_STR_KEYMAP_SIZE) == 0 ) {
			sscanf(token,"%d", &keymap_size);
			pr_info(" :%s=[%d] \n", IR_STR_KEYMAP_SIZE, keymap_size);
		}
 		else if ( strcmp(variable_token, IR_STR_KEYMAP_TBL) == 0 ) {
			if( keymap_size<1 ) {
				pr_err(" : [ERR] IR_STR_KEYMAP_TBL , keymap_size=%d is not legal!! return! \n", keymap_size);
				kfree(new_p_rtk_key_table);
				goto irda_error1;
			}
			pr_info(" : IR_STR_KEYMAP_TBL --- begin : keymap_size=%d -----\n",keymap_size);
			new_p_rtk_key_table->keys = (struct venus_key*)kzalloc(sizeof(struct venus_key) * (keymap_size+1), GFP_KERNEL);
			new_p_rtk_key_table->size = keymap_size;

			idx=0;
			while ((token = strsep(&cur, "\n")) && (idx < keymap_size)) {  
				pr_info("token => [%s] \n", token);
				variable_token = strsep(&token, "=");

				if( strlen(variable_token) > 1 ) {
					sscanf(variable_token,"0x%X", 	&(new_p_rtk_key_table->keys[idx].scancode));
					sscanf(token,		"%d",  		&(new_p_rtk_key_table->keys[idx].keycode));
					pr_info("idx=%d : [0x%X] = [%d] \n", idx, new_p_rtk_key_table->keys[idx].scancode, new_p_rtk_key_table->keys[idx].keycode);

					idx++;
				}
			}
			pr_info(" : IR_STR_KEYMAP_TBL --- end -----\n\n");
			break;
		}
	}  
	if( p_rtk_key_table ) {
		kfree(p_rtk_key_table);
		p_rtk_key_table = new_p_rtk_key_table;
		pr_info("irda_store_ir_replace_table : free orginal keymap here, new keyMap size=%d \n", p_rtk_key_table->size);
	}
#ifdef DEV_DEBUG
	dump_keymap_tbl();
#endif
	pr_info("irda_store_ir_replace_table :	call to Venus_IR_Init(%d,%d) \n",ir_protocol,irtx_protocol);
	/* Hardware Registers Initialization */
	Venus_IR_Init(ir_protocol, irtx_protocol);

irda_error1:
	filp_close(filep,0);

	pr_info("--- debug : irda_store_ir_replace_table  ====  done \n");
	return count;
}

int venus_irtx_set_wake_up_keys(u32 keycode, u32* p_wake_up_keys)
{
	u32 i,j;	
	struct RTK119X_ipc_shm_ir *p_table =(struct RTK119X_ipc_shm_ir*)p_wake_up_keys;
		
	if((!p_wake_up_keys))
	{
		RTK_debug(KERN_ERR "[ERR] venus_ir : p_wake_up_keys pointer is NULL\n");
		return -1;
	}	
	
	p_table->dev_count = htonl(irda_dev_count);
				
	for(j=0;j<irda_dev_count;j++){		
	
		p_table-> key_tbl[j].protocol = htonl(ir_protocol);
		p_table-> key_tbl[j].ir_scancode_mask = htonl(p_rtk_key_table[j].scancode_msk);
		p_table-> key_tbl[j].ir_cus_mask = htonl(p_rtk_key_table[j].custcode_msk);
		p_table-> key_tbl[j].ir_cus_code = htonl(p_rtk_key_table[j].cust_code);
				
		for (i = 0; i < p_rtk_key_table[j].size; i++) {
			if (keycode == p_rtk_key_table[j].keys[i].keycode) {
				p_table->key_tbl[j].ir_wakeup_scancode = htonl(p_rtk_key_table[j].keys[i].scancode);				
				break;			
			}					
		}				
	}
	p_table-> RTK119X_ipc_shm_ir_magic = htonl(0x49525641); //IRVA(IR Version A)
	
	return 0;
}


int venus_irtx_getscancode(u32 keycode, u32* p_scancode, u32* p_scancode_msk)
{
	u32 i;

	if((!p_scancode)||(!p_scancode_msk))
	{
		RTK_debug(KERN_ERR "[ERR] venus_ir : the p_scancode/p_scancode_msk pointer is NULL\n");
		return -1;
	}	
	RTK_debug(KERN_DEBUG "[venus_irtx_getscancode] begin \n");

#if 0
	if(bUseUserTable == true)
	{
		struct user_key_data *pUsrData;
	
		_venus_irtx_get_user_data(&pUsrData);

		if(pUsrData == NULL)
			return -1;

		for(i=0; i<pUsrData->key_table.size; i++) {
			if(keycode == pUsrData->key_table.keys[i].keycode) {
				//mapping found
				*p_scancode = pUsrData->key_table.keys[i].scancode;
				RTK_debug(KERN_ALERT "[getscancode]keycode=%d, scancode=%d", keycode, pUsrData->key_table.keys[i].scancode);
				return 0;
			}
		}
		return -1;
	}

	if (IRTX_CUSTOMER_CODE == RTK_MK5_CUSTOMER_CODE) {
		RTK_debug(KERN_ALERT " %s  %s  %d rtk_mk5_tv_key_table.size = %d \n" ,__FILE__ ,__FUNCTION__, __LINE__,rtk_mk5_tv_key_table.size);
		for (i = 0; i < rtk_mk5_tv_key_table.size; i++) {
			RTK_debug(KERN_ALERT "[getscancode]keycode=%d, rtk_mk5_tv_key_table.keys[i].keycode=%d\n", keycode, rtk_mk5_tv_key_table.keys[i].keycode);

			if (keycode == rtk_mk5_tv_key_table.keys[i].keycode) {
				*p_scancode = rtk_mk5_tv_key_table.keys[i].scancode;
				RTK_debug(KERN_ALERT "[getscancode]keycode=%d, scancode=%d\n", keycode, rtk_mk5_tv_key_table.keys[i].scancode);
				return 0;
			}
		}
		return -1;
	}
#else
	*p_scancode_msk = p_rtk_key_table->scancode_msk;

	for (i = 0; i < p_rtk_key_table->size; i++) {
		if (keycode == p_rtk_key_table->keys[i].keycode) {
			*p_scancode = p_rtk_key_table->keys[i].scancode;
			RTK_debug(KERN_DEBUG "[venus_irtx_getscancode] found! keycode=%d scancode=0x%x !\n",keycode,*p_scancode);
			return 0;
		}
	}
#endif
	RTK_debug(KERN_ERR "[venus_irtx_getscancode] keycode=%d fail!", keycode);
	return -1;
}

void venus_irtx_raw_send(int write_Count)
{
	unsigned int reg;
	int i;

	reg = ioread32(reg_irtx_cfg);
	if(!(reg & 0x1)) {
		for(i=0; i<(write_Count/4); i++)
			iowrite32(*((int *)(rpmode_txfifo)+i), reg_irtx_fifo);
		iowrite32(0x80000511, reg_irtx_cfg);
		if(write_Count > FIFO_DEPTH*4)
			Venus_irtx_fifoPut(rpmode_txfifo + FIFO_DEPTH*4, write_Count - FIFO_DEPTH*4);
	} else {
		Venus_irtx_fifoPut(rpmode_txfifo, write_Count);
	}
}

int venus_irtx_send(u32 keycode)
{
	u32 scancode;
	u32 scancode_msk;
	RTK_debug(KERN_ALERT "[venus_irtx_send] keycode=%d  \n" , keycode);

	if (-1 == venus_irtx_getscancode(keycode, &scancode, &scancode_msk))
	{
		RTK_debug(KERN_ALERT " %s  %s  %d  venus_irtx_getscancode fail \n" ,__FILE__ ,__FUNCTION__, __LINE__);
		return -1;
	}

	RTK_debug(KERN_ALERT "[venus_irtx_send] scancode=0x%x, CUSTOMER_CODE = 0x%x \n" , scancode,IRTX_CUSTOMER_CODE);

	if (-1 ==  _venus_irtx_send_start(scancode, IRTX_CUSTOMER_CODE))
	{
		RTK_debug(KERN_ERR " %s  %s  %d _venus_irtx_send_start fail\n" ,__FILE__ ,__FUNCTION__, __LINE__);
		return -1;
	}
	RTK_debug(KERN_ALERT " %s  %s  %d _venus_irtx_send_start OK\n" ,__FILE__ ,__FUNCTION__, __LINE__);
	return 0;
}

static void venus_irtx_do_tasklet(unsigned long unused)
{
	u32 keycode;
	if (Venus_irtx_fifoGetLen()) {
		return;
	}
	Venus_irtx_fifoGet(&keycode, sizeof(u32));
	RTK_debug(KERN_ALERT "Venus IRTX tasklet: keycode = [0x%08X]...\n", keycode);
	if (-1 == venus_irtx_send(keycode))
		RTK_debug(KERN_ALERT " %s  %s  %d venus_irtx_send fail\n" ,__FILE__ ,__FUNCTION__, __LINE__);
}

static void rtk_ir_wakeup_timer(unsigned long data)
{
    unsigned long lock_flags;

	printk(KERN_EMERG "Venus ir wakeup end\n");
    //send power key to wakeup android   victor20140617
    spin_lock_irqsave(&ir_splock, lock_flags);
    venus_ir_input_report_key(KEY_STOP,&f_CursorEnable);
    venus_ir_input_report_end(&f_CursorEnable);
    spin_unlock_irqrestore(&ir_splock, lock_flags);
}

int venus_irtx_Check_keycode(u32 keycode)
{
	RTK_debug(KERN_DEBUG " %s  %s  %d \n" ,__FILE__ ,__FUNCTION__, __LINE__);
#if 0
	int i;
//	if (!IRTx_KeycodeTableEnable)
//		return -1;

//	for (i=0; i<IRTx_KeycodeTable.irtx_keycode_list_len; i++)
	{
//		if (keycode == IRTx_KeycodeTable.irtx_keycode_list[i])
		{
			Venus_irtx_fifoPut(&keycode, sizeof(unsigned char));
			tasklet_schedule(&irtx_tasklet);
			return 0;
		}
	}
	return -1;
#else
	venus_irtx_send(keycode);
	return 0;
#endif
}


struct venus_ir_dev *venus_ir_devices;	/* allocated in scull_init_module */

/* Module Functions */

static int examine_ir_avail(uint32_t *regValue, uint32_t *irrp2Value, int *dataRepeat) {
	u32 srValue = ioread32(reg_ir_sr);

	*irrp2Value = 0;
	RTK_debug(KERN_ALERT "[%s]%s %d   reg_ir_sr = [0x%08X]\n",__FILE__,__FUNCTION__,__LINE__, srValue);


	if(srValue & 0x1) { // 0x1 is check for "Data Valid"
		if(srValue & 0x2)// 0x2 is check for "Repeat flag for NEC only"
			*dataRepeat = 1;
		else
			*dataRepeat = 0;

		iowrite32(0x00000003, reg_ir_sr); /* clear IRDVF */
		*regValue = ioread32(reg_ir_rp); /* read IRRP */
		if(ir_protocol == PANASONIC)
		{
			*irrp2Value = ioread32(reg_ir_rp2); /* read IRRP2 */
		}

		RTK_debug(KERN_DEBUG "[%s]%s %d   reg_ir_rp = [0x%08X]\n",__FILE__,__FUNCTION__,__LINE__,*regValue);
		if(ir_protocol == PANASONIC)
		{
			RTK_debug(KERN_DEBUG "[%s]%s %d   reg_ir_rp2 = [0x%08X]\n",__FILE__,__FUNCTION__,__LINE__, *irrp2Value);
		}

		if(ir_protocol == RC6)
		{
			if((lastSrValue&0xffff) == (*regValue&0xffff)) {
				*dataRepeat = 1;
				debounce = 0;
			}
			else
				*dataRepeat = 0;
			lastSrValue = *regValue;
		}
		else if(ir_protocol == RCA)
		{
			if(lastSrValue == *regValue) {
				*dataRepeat = 1;
				debounce = 0; /* for first repeat interval*/
			}
			else
				*dataRepeat = 0;
			lastSrValue = *regValue;
		}
		return 0;
	}
	else
		return -ERESTARTSYS;
}

static int repeat_key_handle(uint32_t regValue, uint32_t irrp2Value, int dataRepeat) {
		int received = 0;
		u32 keycode;
		RTK_debug(KERN_ALERT "repeat_key_handle: -- begin -- regValue=0x%x, dataRepeat=%d RPRepeatIsHandling=%d\n",regValue, dataRepeat,RPRepeatIsHandling);

		if((RPRepeatIsHandling== true) && ((jiffies_to_msecs(jiffies)- lastRecvMs) < debounce)) {
				RTK_debug(KERN_DEBUG "repeat_key_handle: Venus IR: %dms, ignored..\n", (jiffies_to_msecs(jiffies) - lastRecvMs));

				lastRecvMs = jiffies_to_msecs(jiffies);
				received = 1;
		}
/*
#ifdef DELAY_FIRST_REPEAT
		else if(dataRepeat == 1 && firstRepeatInterval > 0 && firstRepeatTriggered == false && (jiffies_to_msecs(jiffies)- PressRecvMs) < firstRepeatInterval) {

				RTK_debug(KERN_DEBUG "Venus IR: %dms, ignored for first repeat interval..\n", (jiffies_to_msecs(jiffies) - PressRecvMs));

				lastRecvMs = jiffies_to_msecs(jiffies);
		}
#endif
		else if(dataRepeat == 1 && (jiffies_to_msecs(jiffies)- lastRecvMs) < REPEAT_MAX_INTERVAL) {

				RTK_debug(KERN_DEBUG "Venus IR: Repeat Symbol after %dms, ignored..\n", (jiffies_to_msecs(jiffies) - lastRecvMs));

		}
*/		else {

			RTK_debug(KERN_DEBUG "[%s]%s  %d Venus IR: Non-repeated frame [%dms]\n",__FILE__,__FUNCTION__, __LINE__, (jiffies_to_msecs(jiffies) - lastRecvMs));
			RTK_debug(KERN_DEBUG "[%s]%s  %d IRRP = [0x%08X].\n",__FILE__,__FUNCTION__,__LINE__, regValue);
			RTK_debug(KERN_DEBUG "[%s]%s  %d f_CursorEnable = %d \n",__FILE__,__FUNCTION__,__LINE__, f_CursorEnable);

			lastRecvMs = jiffies_to_msecs(jiffies);
			if(ir_protocol == RC6) {
				regValue = ~(regValue)&0x1fffff;  // 21 bits
				RTK_debug(KERN_DEBUG "[%s]%s  %d IRRP = [%08X].\n",__FILE__,__FUNCTION__,__LINE__, regValue);
			}
			else if(ir_protocol == RC5)
				regValue = (regValue&0xfff);

			if(ir_protocol == PANASONIC && irrp2Value != 0x20020000) {
				RTK_debug(KERN_DEBUG "Venus IR: Maker code = [%08X].\n", irrp2Value);
			}
			else {
				Venus_IR_rp_fifoPut( (unsigned char *)&regValue, sizeof(uint32_t));
				if (-1 != venus_ir_IrMessage2Keycode(regValue, &keycode))
				{
#ifdef CONFIG_SUPPORT_INPUT_DEVICE
					venus_ir_input_report_key(keycode,&f_CursorEnable);
                    isKeypadTimerCreate = true;
#endif

#if 0//def  CONFIG_REALTEK_IRTX
					if (-1 == venus_irtx_Check_keycode(keycode))
						printk("[IRTX] ERROR : venus_irtx_Check_keycode fail \n");
#endif
				}

				if(driver_mode == DOUBLE_WORD_IF)
						Venus_IR_rp_fifoPut( (unsigned char *)&dataRepeat, sizeof(int32_t));

#ifdef DELAY_FIRST_REPEAT
				if(dataRepeat == 1)
						firstRepeatTriggered = true;
				else {
						PressRecvMs = lastRecvMs;
						firstRepeatTriggered = false;
				}
#endif

				if ( f_CursorEnable && ((keycode == KEY_RIGHT) ||(keycode == KEY_LEFT)
					||(keycode == KEY_UP) ||(keycode == KEY_DOWN)) )
				{
					RPRepeatIsHandling =false;	//always report input
				} else {
					RPRepeatIsHandling =true;	//report input in the first of repeat dates
				}
				received = 1;				//need report sync
			}
		}
	return received;
}

static int get_bit_cnt(int *bit_num, int *sample_cnt, unsigned long *sample, int polar) {
	int bit_cnt = 0;

	if(polar != 0 && polar != 1)
		return -1;

	while(1) {
		if((*bit_num) == 0) {
			if(ioread32(reg_ir_raw_wl) > 0) {
				(*sample_cnt)++;
				(*sample) = ioread32(reg_ir_raw_ff);
				(*bit_num) = 32;
				continue;
			}
			else
				break;
		}
		else {
			if(test_bit((*bit_num)-1, sample)) {
				if(polar == 1) {
					(*bit_num)--;
					bit_cnt++;
				}
				else
					break;
			}
			else {
				if(polar == 0) {
					(*bit_num)--;
					bit_cnt++;
				}
				else
					break;
			}
		}
	}

	return bit_cnt;
}

static inline int get_next_raw_bit(int *bit_num, int *sample_cnt, unsigned long *sample) {
	if((*bit_num) == 1) {
		if(ioread32(reg_ir_raw_sample_time) > (*sample_cnt)) {
			(*sample_cnt)++;
			(*sample) = ioread32(reg_ir_raw_ff);
			(*bit_num) = 32;

			return test_bit((*bit_num), sample);
		}
		else
			return -1;
	}
	else {
		(*bit_num)--;
		return test_bit((*bit_num), sample);
	}
}

// 5000 Hz NEC Protocol Decoder
static int raw_nec_decoder(int *dataRepeat) {
	int i;
	int raw_bit_anchor = 32;
	int raw_bit_sample_cnt = 1;
	unsigned long raw_bit_sample;
	int symbol = 0;
	int length_low, length_high;
	unsigned long stopCnt;

	static unsigned long lastSymbol = 0;


	uint32_t srValue = ioread32(reg_ir_sr);
	RTK_debug(KERN_DEBUG "[%s] Venus IR: reg_ir_sr = [%08X]\n", __func__, srValue);


	if(srValue & 0xc) {
		iowrite32((srValue & (~0xf)), reg_ir_sr); /* clear raw_fifo_val and raw_fifo_ov */
	}else{
		return symbol;
	}

	raw_bit_sample = ioread32(reg_ir_raw_ff);

	// [decode] PREMBLE (High for 8ms / Low for 4ms)
	length_high = get_bit_cnt(&raw_bit_anchor, &raw_bit_sample_cnt, &raw_bit_sample, 0);
	length_low = get_bit_cnt(&raw_bit_anchor, &raw_bit_sample_cnt, &raw_bit_sample, 1);

	RTK_debug(KERN_DEBUG "Venus BT: 1 for %d and 0 for %d is detected.\n", length_high, length_low);

	if(length_high >= 40 && length_low <= 12) {	//repeat : 9ms burst and 2.25ms space
		symbol = lastSymbol;
		goto get_symbol;
	}
	else if(length_high < 40 || length_low < 20)	//not AGC burst : 9ms burst and 4.5ms space
		goto sleep;
	else

	for(i = 0 ; i < 32 ; i++) {			//one transmit is 32 bit (8 addr, 8 invert addr, 8 cmd, 8 invert cmd)
		int length_high, length_low;

		length_high = get_bit_cnt(&raw_bit_anchor, &raw_bit_sample_cnt, &raw_bit_sample, 0);
		length_low = get_bit_cnt(&raw_bit_anchor, &raw_bit_sample_cnt, &raw_bit_sample, 1);


		RTK_debug(KERN_DEBUG "Venus IR: 1 for %d and 0 for %d is detected [%d].\n", length_high, length_low,i);


		if(length_high >= 2) {
			if(length_low > 10) { 		// Repeat : 560us burst and space until 110ms
				*dataRepeat = 1;
				symbol = lastSymbol;
				break;
			}
			else if(length_low >= 7)	// 1.68 ms space to logical "1"
				symbol |= (0x1 << i);
			else if(length_low >= 2)	// 560us space to logical "0"
				symbol &= (~(0x1 << i));
			else
				goto sleep;

		}
		else
			goto sleep;
	}

get_symbol:

	RTK_debug(KERN_DEBUG "Venus IR: [%d = %08X] is detected.\n", symbol, symbol);

	if(lastSymbol == symbol) {
		*dataRepeat = 1;
	}
	else {
		*dataRepeat = 0;
		lastSymbol = symbol;
	}

sleep:
	stopCnt = ioread32(reg_ir_raw_sample_time) + 0xc8;

	// prepare to stop sampling ..
	iowrite32(0x03000048 | (stopCnt << 8),  reg_ir_raw_ctrl); // stop sampling for at least 150 (= 30ms), fifo_thred = 8

	return symbol;
}


// 10000 Hz RC6 Protocol Decoder
static uint32_t raw_rc6_decoder(int *dataRepeat) {
	int i,j;
	int raw_bit_anchor = 32;
	int raw_bit_sample_cnt = 1;
	unsigned long raw_bit_sample;
	int symbol = 0x8000;
	int length_low, length_high;
	int length_get = 0;
	unsigned long stopCnt;
	static unsigned long lastSymbol = 0;
	unsigned char check_rc6_tail = 0;

	uint32_t srValue = ioread32(reg_ir_sr);
	RTK_debug(KERN_DEBUG "[%s] Venus IR: reg_ir_sr = [%08X]\n", __func__, srValue);


	if(srValue & 0xc) {
		iowrite32((srValue & (~0xf)), reg_ir_sr); /* clear raw_fifo_val and raw_fifo_ov */
	}else{
		return symbol;
	}


	raw_bit_sample = ioread32(reg_ir_raw_ff);

	// [decode] PREMBLE (High for 2.6ms / Low for 0.8ms)
	// 1 bit == 0.1ms
	length_high = get_bit_cnt(&raw_bit_anchor, &raw_bit_sample_cnt, &raw_bit_sample, 0);
	length_low = get_bit_cnt(&raw_bit_anchor, &raw_bit_sample_cnt, &raw_bit_sample, 1);

	/* check RC6 LS */
	if (!(length_high >= 26 && length_high <= 29 && length_low >= 8 && length_low <= 10)){	//leader pulse 2.666ms + 889us space
			goto sleep;
	}
	else{
			int bit_value = 0;
			int delay_ct = 0;
			int length_warn = 0;
			length_high = 0;
			length_low = 0;
			symbol = 0;

			for (i = 0; i < 21; i++) { /* get symbol */	//SB+MB2+MB1+MB0+TR+A7+A6+A5+A4+A3+A2+A1+A0+C7+C6+C5+C4+C3+C2+C1+C0
					delay_ct = 0;
					while (ioread32(reg_ir_raw_wl) < 4 && delay_ct <= 12) {  /* Check HW water level(32 word) out of range */
							delay_ct ++;
					}
					if (ioread32(reg_ir_raw_wl) >= 26) { /* Check HW water level(32 word) out of range */
							goto sleep;
					}
					if (length_high == 0 && length_low == 0) {  /* PreGet symbol : notice */
							if (length_get == 1)  { /* get high then low */
									length_low = get_bit_cnt(&raw_bit_anchor, &raw_bit_sample_cnt, &raw_bit_sample, 1);
									length_high = get_bit_cnt(&raw_bit_anchor, &raw_bit_sample_cnt, &raw_bit_sample, 0);
									bit_value = 0;
							}
							else { /* get low then high */
									length_high = get_bit_cnt(&raw_bit_anchor, &raw_bit_sample_cnt, &raw_bit_sample, 0);
									length_low = get_bit_cnt(&raw_bit_anchor, &raw_bit_sample_cnt, &raw_bit_sample, 1);
									bit_value = 1;
							}
					}
					else {
							if (length_high == 0) {  /* just get low (two high is successive then only get low) */
									length_high = get_bit_cnt(&raw_bit_anchor, &raw_bit_sample_cnt, &raw_bit_sample, 0);
									length_get = 1;
									bit_value = 0;
							}
							else if(length_low == 0) {  /* just get high (two low is successive then only get high) */
									length_low = get_bit_cnt(&raw_bit_anchor, &raw_bit_sample_cnt, &raw_bit_sample, 1);
									length_get = 0;
									bit_value = 1;
							}
					}
					if(length_high <= 3 || length_low <= 3) {
							length_warn++;
					}
/*					if(length_high <= 2 || length_low <= 2) { // Check if  length_high or length_low too low(check if sig bit is valid)
							goto sleep;
					}*/
					if (i==0) {  /* bypass start bit */
							//symbol |= (0x1 << i);
					}
					else {  /* assign symbol bit */
							symbol |= (bit_value << (19-(i-1)));
					}

					if (i ==4) { /* handle TR */
							if (length_high >= 11 && length_high <= 16)	{ 	/* Check if length_high continuous (valid range : 7~16 (100us unit)) */
									length_high -= 8;
							}
							else if (length_high >= 17)  /* Check if length_low continuous (valid range: 7~16 (100us unit)) */
									goto get_symbol;
							else
									length_high = 0;

							if (length_low >= 11 && length_low <= 16) {
									length_low -= 8;
							}
							else if (length_low >= 17)
									goto get_symbol;
							else
									length_low = 0;
					}
					else {
							if (length_high >= 7 && length_high <= 16) { /* Check if length_high continuous(valid range : 7~16 (100us unit)) */
									if (length_high >= 12 && length_low >= 7 )  /* convert length_high is continuous */
											length_high -= 8;
									else  /* Normal length_high is not continuous */
											length_high -= 4;
									/*bit_value=1;*/
							}
							else if (length_high >= 17) /*Check if length_low continuous(valid range: 7~16 (100us unit)) */
									goto get_symbol;
							else
									length_high = 0;

							if (length_low >= 7 && length_low <= 16) {
									if(length_low >= 12 && length_high >= 7)
											length_low -= 8;
									else
											length_low -= 4;
									/*bit_value = 0;*/
							}
							else if (length_low >= 17)
									goto get_symbol;
							else
									length_low = 0;
					}
			}
			if(length_warn >= 4) {
					goto sleep;
			}
	}

get_symbol:
	check_rc6_tail = 1;
	for (j = 0; j < 2; j++) {
		raw_bit_sample = ioread32(reg_ir_raw_ff);

	//	RTK_debug("raw_bit_sample-=%x\n", raw_bit_sample);

		if(raw_bit_sample != 0xFFFFFFFF && raw_bit_sample != 0x0) {
			symbol = 0x8000;

	//		RTK_debug("rc6-\n");

			goto sleep;
		}
	}

	if(i < 20)  // symbol bits >=20
		goto sleep;

	if (symbol == lastSymbol) {
		*dataRepeat = 1;
	}
	else {
		*dataRepeat = 0;
		lastSymbol = symbol;
    }


	RTK_debug(KERN_DEBUG "====>  symbol=%x dataRepeat=%d\n", symbol, *dataRepeat);

	if (((symbol & 0x0000ffff) == 0x000c) && (*dataRepeat == 1)) // power key don't response repeat
		goto sleep;

sleep:
	if(check_rc6_tail == 0){
		for (j = 0; j < 2; j++) {
			raw_bit_sample = ioread32(reg_ir_raw_ff);

//			RTK_debug("raw_bit_sample+=%x\n", raw_bit_sample);

			if(raw_bit_sample != 0xFFFFFFFF && raw_bit_sample != 0x0){
				symbol = 0x8000;

//				RTK_debug("rc6+\n");

				goto sleep1;
			}
		}
}

sleep1:
	stopCnt = ioread32(reg_ir_raw_sample_time) + 0xc8;
	// prepare to stop sampling ..
	iowrite32(0x03000048 | (stopCnt << 8), reg_ir_raw_ctrl); // stop sampling for at least 150 (= 30ms), fifo_thred = 8

	return symbol;
}


static void keypad_timer_expired(unsigned long data)
{
    unsigned long lock_flags;
    spin_lock_irqsave(&ir_splock, lock_flags);
	if (isKeypadTimerCreate)
	{
		isKeypadTimerCreate = false;
		venus_ir_input_report_end(&f_CursorEnable);
	}
    spin_unlock_irqrestore(&ir_splock, lock_flags);
}

static int get_raw_data(void)
{
	unsigned int srValue;
	int regVal = -1;
	int len;
	int i;
	unsigned int data[FIFO_DEPTH*4];

	srValue = ioread32(reg_ir_sr);
	if(srValue & 0xc)
		iowrite32((srValue & (~0xf)), reg_ir_sr);
	else
		return regVal;

#ifdef REPEAT_MODE_SELFTEST
	len = Venus_irtx_fifoAvail();
	if(len < 512 && start_tx==0) {
		srValue = ioread32(reg_irtx_cfg);
		if(!(srValue & 0x1)) {
			Venus_irtx_fifoGet(data, FIFO_DEPTH*4);
			for(i=0; i<FIFO_DEPTH; i++)
				iowrite32( data[i],  reg_irtx_fifo);

			iowrite32(0x80000511, reg_irtx_cfg);
		}
		start_tx=1;
	}

	len = ioread32(reg_ir_raw_wl);
	for(i=0; i<len; i++)
		data[i] = ~ioread32(reg_ir_raw_ff);

	Venus_irtx_fifoPut(data, len*4);
#else
	len = ioread32(reg_ir_raw_wl);
	for(i=0; i<len; i++)
		data[i] = ~ioread32(reg_ir_raw_ff);

	Venus_IR_rp_fifoPut(data, len*4);
#endif
	return 0;
}

static irqreturn_t IR_interrupt_handler(int irq, void *dev_id) {
	int dataRepeat = 0;
	uint32_t regValue;
	uint32_t irrp2Value = 0;
	int received = 0;
	int txlen, i;
	RTK_debug(KERN_DEBUG "\n\n\n\n\n");
	RTK_debug(KERN_DEBUG "[%s] %s  %d -- begin \n\n", __FILE__,__FUNCTION__,__LINE__);

	regValue = ioread32(reg_isr);
	RTK_debug(KERN_DEBUG "[%s] %s  %d  reg_isr = 0x%x, ir_protocol=%d \n", __FILE__,__FUNCTION__,__LINE__,regValue,ir_protocol);

	{
		if(ir_protocol == RAW_NEC) {
			debounce = 0;
			regValue = raw_nec_decoder(&dataRepeat);
			if(regValue == 0) {
				//return IRQ_HANDLED;
				goto CONTINUE_IRTX;
			}
			if (dataRepeat == 0)
			{
				if (isKeypadTimerCreate)
				{
					isKeypadTimerCreate = false;
					venus_ir_input_report_end(&f_CursorEnable);
				}
				received = repeat_key_handle(regValue, irrp2Value, dataRepeat);
			}
		}
		else if(ir_protocol == RAW_RC6) {
			debounce = 0;
			regValue = raw_rc6_decoder(&dataRepeat);
			if(regValue == 0x8000) {
				//return IRQ_HANDLED;
				goto CONTINUE_IRTX;
			}
			if (dataRepeat == 0)
			{
				if (isKeypadTimerCreate)
				{
					isKeypadTimerCreate = false;
					venus_ir_input_report_end(&f_CursorEnable);
				}
				received = repeat_key_handle(regValue, irrp2Value, dataRepeat);
			}
		}
		else if(ir_protocol == REPEAT_MODE) {
			get_raw_data();
		}
		else {
				RPRepeatIsHandling =false;
				while(examine_ir_avail(&regValue, &irrp2Value, &dataRepeat) == 0) {
					RTK_debug(KERN_DEBUG "[%s] %s  %d dataRepeat = %d ,isKeypadTimerCreate=%d \n", __FILE__,__FUNCTION__,__LINE__,dataRepeat,isKeypadTimerCreate);
					if (dataRepeat == 0)
					{
						if (isKeypadTimerCreate)
						{
							isKeypadTimerCreate = false;
							venus_ir_input_report_end(&f_CursorEnable);
						}
						received = repeat_key_handle(regValue, irrp2Value, dataRepeat);
					}
                    else if(isKeypadTimerCreate == false)
                    {
                        received = repeat_key_handle(regValue, irrp2Value, 0);
                    }
					RTK_debug(KERN_DEBUG "[%s] %s  %d  received = %d\n", __FILE__,__FUNCTION__,__LINE__,received);
				}
		}

		if(received == 1) {
			RTK_debug(KERN_DEBUG "[%s] %s  %d  \n", __FILE__,__FUNCTION__,__LINE__);
			Venus_IR_rp_WakeupQueue();
#ifdef CONFIG_SUPPORT_INPUT_DEVICE
			//venus_ir_input_report_end(&f_CursorEnable);
#endif
		}

		mod_timer(&venus_ir_timer, jiffies + msecs_to_jiffies(230));
	}
CONTINUE_IRTX:
	RTK_debug(KERN_DEBUG "[%s] %s  %d --  Tx processing ----- \n", __FILE__,__FUNCTION__,__LINE__);
	regValue = ioread32(reg_irtx_int_st);
	RTK_debug(KERN_DEBUG "[%s] %s  %d reg_irtx_int_st= 0x%08X \n", __FILE__,__FUNCTION__,__LINE__,regValue);

	if(rpmode) {
		if(regValue & 0x2) {
			unsigned int data[FIFO_DEPTH];
			txlen = Venus_irtx_fifoGetLen() / sizeof(unsigned int);
			if(txlen > (FIFO_DEPTH-1))
				txlen = FIFO_DEPTH - 1;
			if(txlen==0) {
				printk(KERN_ERR "[IRTX] no data in fifo\n");
				iowrite32(0xf04, reg_irtx_int_en);
			} else {
				Venus_irtx_fifoGet(data, txlen * sizeof(unsigned int));
				for(i=0; i<txlen; i++)
					iowrite32(data[i], reg_irtx_fifo);
			}
		}
		if (regValue & 0x4) {
			printk(KERN_ERR "[IRTX] stop tx 0x%x\n", regValue);
			venus_ir_disable_irtx();

			iowrite32(0x80000000, reg_irtx_fifo_st);
			iowrite32(0x00000000, reg_irtx_fifo_st);

			iowrite32(0xf06, reg_irtx_int_en);
		}
	} else {
	if(	regValue & 0x4) // IRTX_INT_ST: FIN_FLAG = 1
	{
		uint32_t regTmpValue=0; 
		RTK_debug(KERN_DEBUG "[%s] %s  %d :  IRTX_INT_ST: FIN_FLAG = 1 \n", __FILE__,__FUNCTION__,__LINE__);
#if 0
		regValue = 	ioread32(reg_ir_cr);
		iowrite32((regValue | 0x100), reg_ir_cr);	//IRRC disable

		regValue = 	ioread32(reg_irtx_cfg);
		iowrite32((regValue & ~0x1), reg_irtx_cfg);	//irtx stop
#else
#endif
		venus_ir_disable_irtx();

		iowrite32(0x80000000, reg_irtx_fifo_st);	//irtx fifo reset
		iowrite32(0x00000000, reg_irtx_fifo_st);	//irtx fifo normal

		regTmpValue = ioread32(reg_irtx_fifo);	//irtx fifo normal
		
		RTK_debug(KERN_DEBUG "Venus IRTX: after reset reg_irtx_fifo_st, get reg_irtx_fifo=0x%08X\n", regTmpValue);
		
		venus_ir_enable_irrp();
	}
	}
	regValue = ioread32(reg_isr);
	
	RTK_debug(KERN_ALERT "[%s] %s  %d -- end (ISO_ISR=0x%08X) \n\n", __FILE__,__FUNCTION__,__LINE__,regValue);
	return IRQ_HANDLED;
}

/* *** ALL INITIALIZATION HERE *** */
int Venus_IR_Init(int mode, int tx_mode) {
	int retval = 0;
	u32	reg_dpir = 0;
	RTK_debug(KERN_ALERT "Venus_IR_Init ---- begin ----\n");
#if 0 // by toto, move to probe
	if (!rtk119x_irda_node)
		panic("No irda of node found");

	if (of_property_read_u32(rtk119x_irda_node, "Realtek,reg-ir-dpir", &dpir_val)) {
		printk(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default reg-ir-dpir = %d .\n", __func__, dpir_val);
	}
#endif
	/* Initialize Venus IR Registers*/
	lastRecvMs = jiffies_to_msecs(jiffies);

	/* using HWSD parameters */
	switch(mode) {
		case NEC:
			RTK_debug(KERN_ALERT "Venus_IR_Init for NEC mode\n");
			iowrite32(0x80000000, reg_ir_cr);
			iowrite32(0x5a13133b, reg_ir_psr);
			iowrite32(0x0001162d, reg_ir_per);
//#if defined(CONFIG_REALTEK_DARWIN) || defined(CONFIG_REALTEK_MACARTHUR)
//			iowrite32(0xff00021b, reg_ir_sf);	// 50KHz
//#else
//			iowrite32(0x19c7F, reg_ir_sf);	// test   victor.hsu 20140107
//			iowrite32(0x00000293, reg_ir_sf);	// 50KHz for FPGA 33M   victor.hsu 20140107
			iowrite32(0x0000021b, reg_ir_sf);	// 50KHz
//#endif /* CONFIG_REALTEK_DARWIN */
//			iowrite32(0x1f4, reg_ir_dpir); // 10ms
			reg_dpir = (dpir_val*1000 *27)/(ioread32(reg_ir_sf)+1);
			printk(KERN_ALERT "[%s] get reg-ir-dpir = %uld us from dtb, regval=0x%ulx \n",__FILE__, dpir_val, reg_dpir);
			iowrite32(reg_dpir, reg_ir_dpir); //move the dpir setting in device tree

			iowrite32(0x5df, reg_ir_cr);
			break;
		case RC5:
			iowrite32(0x80000000, reg_ir_cr);
			iowrite32(0x00242424, reg_ir_psr);
			iowrite32(0x00010000, reg_ir_per);
			iowrite32(0x21b, reg_ir_sf);
//			iowrite32(0x3ff, reg_ir_dpir);
			reg_dpir = (dpir_val*1000 *27)/(ioread32(reg_ir_sf)+1);
			RTK_debug(KERN_ALERT "[%s] get reg-ir-dpir = %uld us from dtb, regval=0x%ulx \n",__FILE__, dpir_val, reg_dpir);
			iowrite32(reg_dpir, reg_ir_dpir); //move the dpir setting in device tree
			iowrite32(0x70c, reg_ir_cr);
			break;
		case SHARP:
			iowrite32(0x80000000, reg_ir_cr);
			iowrite32(0x00051b54, reg_ir_psr); //modulation length = 0.1ms
			iowrite32(0x00010000, reg_ir_per);
			iowrite32(0x21b, reg_ir_sf);
//			iowrite32(0xbe, reg_ir_dpir); // 3.8ms
			reg_dpir = (dpir_val*1000 *27)/(ioread32(reg_ir_sf)+1);
			RTK_debug(KERN_ALERT "[%s] get reg-ir-dpir = %uld us from dtb, regval=0x%ulx \n",__FILE__, dpir_val, reg_dpir);
			iowrite32(reg_dpir, reg_ir_dpir); //move the dpir setting in device tree
			iowrite32(0x58e, reg_ir_cr);
			break;
		case SONY:
			iowrite32(0x80000000, reg_ir_cr);
			iowrite32(0x18181830, reg_ir_psr);
			iowrite32(0x00010006, reg_ir_per);
			iowrite32(0x21b, reg_ir_sf);
//			iowrite32(0x1f4, reg_ir_dpir);
			reg_dpir = (dpir_val*1000 *27)/(ioread32(reg_ir_sf)+1);
			RTK_debug(KERN_ALERT "[%s] get reg-ir-dpir = %uld us from dtb, regval=0x%ulx \n",__FILE__, dpir_val, reg_dpir);
			iowrite32(reg_dpir, reg_ir_dpir); //move the dpir setting in device tree
			iowrite32(0xdd3, reg_ir_cr);
			break;
		case C03:
			iowrite32(0x80000000, reg_ir_cr);
			iowrite32(0x25151034, reg_ir_psr);
			iowrite32(0x00000025, reg_ir_per);
#if defined(CONFIG_REALTEK_DARWIN) || defined(CONFIG_REALTEK_MACARTHUR)
			iowrite32(0x200, DARWIN_ISO_IR_CTRL_RC6);
			iowrite32(0xff00021b, reg_ir_sf);
#else
			iowrite32(0x0000021b, reg_ir_sf);
#endif /* CONFIG_REALTEK_DARWIN */
//			iowrite32(0x3ff, reg_ir_dpir);
			reg_dpir = (dpir_val*1000 *27)/(ioread32(reg_ir_sf)+1);
			RTK_debug(KERN_ALERT "[%s] get reg-ir-dpir = %uld us from dtb, regval=0x%ulx \n",__FILE__, dpir_val, reg_dpir);
			iowrite32(reg_dpir, reg_ir_dpir); //move the dpir setting in device tree
			iowrite32(0x5ff, reg_ir_cr);
			break;
		case RC6:
			iowrite32(0x80000000, reg_ir_cr);
			iowrite32(0x1c0e0e0e, reg_ir_psr);
			iowrite32(0x00030008, reg_ir_per);
			iowrite32(0x21b, reg_ir_sf);
//			iowrite32(0x3ff, reg_ir_dpir);
			reg_dpir = (dpir_val*1000 *27)/(ioread32(reg_ir_sf)+1);
			RTK_debug(KERN_ALERT "[%s] get reg-ir-dpir = %uld us from dtb, regval=0x%ulx \n",__FILE__, dpir_val, reg_dpir);
			iowrite32(reg_dpir, reg_ir_dpir); //move the dpir setting in device tree
			iowrite32(0x715, reg_ir_cr); // valid bit length is 21
#if defined(CONFIG_REALTEK_DARWIN) || defined(CONFIG_REALTEK_MACARTHUR)
			iowrite32(0x123, DARWIN_ISO_IR_CTRL_RC6);
#else
//			if(is_saturn_cpu()||is_nike_cpu())
				iowrite32(0x123,  reg_ir_ctrl_rc6); // RC6_EN & TB = 0x23 = 35*20 = 700us
//			else
//			if(is_jupiter_cpu())
//				iowrite32(0x123,  ISO_IR_CTRL_RC6); // RC6_EN & TB = 0x23 = 35*20 = 700us
//			else
//				iowrite32(0x82a3, MIS_DUMMY); // RC6_EN & TB = 0x23 = 35*20 = 700us
#endif /* CONFIG_REALTEK_DARWIN */
			break;

		case RAW_NEC:
			RTK_debug(KERN_ALERT "Venus_IR_Init for RAW_NEC mode\n");
			iowrite32(0x80000000, reg_ir_cr);
#if defined(CONFIG_REALTEK_DARWIN) || defined(CONFIG_REALTEK_MACARTHUR)
	    		iowrite32(0x1517, reg_ir_raw_sf);       // use 5000 Hz sampling rate
#else
	    		iowrite32(0x1517, reg_ir_sf);       // use 5000 Hz sampling rate
#endif
	    		iowrite32(0x0a8b, reg_ir_raw_deb);  // 2x over sampling for debounce	//27M /2700 = 10KHz
	    		iowrite32(0x03138848,  reg_ir_raw_ctrl);
	    		iowrite32(0x7300, reg_ir_cr); // enable raw mode
			break;
		case KONKA:
			iowrite32(0x80000000, reg_ir_cr);
			iowrite32(0x1e143c64, reg_ir_psr); //80% spec suggest value
			iowrite32(0x0003001a, reg_ir_per);// 80% spec suggest value
			iowrite32(0x21b, reg_ir_sf); // 50KHZ
//			iowrite32(0x30c, reg_ir_dpir);	//80% spec suggest value  0x190
			reg_dpir = (dpir_val*1000 *27)/(ioread32(reg_ir_sf)+1);
			RTK_debug(KERN_ALERT "[%s] get reg-ir-dpir = %uld us from dtb, regval=0x%ulx \n",__FILE__, dpir_val, reg_dpir);
			iowrite32(reg_dpir, reg_ir_dpir); //move the dpir setting in device tree
			iowrite32(0x7cf, reg_ir_cr);  //  bit6 need set 1,  16 bits

			break;
		case RCA://TCL REMOTE Control
			iowrite32(0x80000000, reg_ir_cr);
			iowrite32(0x2f082a5a, reg_ir_psr);
			iowrite32(0x0001002a, reg_ir_per);
			iowrite32(0xff00021b, reg_ir_sf);
//			iowrite32(0x190, reg_ir_dpir); // 8ms
			reg_dpir = (dpir_val*1000 *27)/(ioread32(reg_ir_sf)+1);
			RTK_debug(KERN_ALERT "[%s] get reg-ir-dpir = %uld us from dtb, regval=0x%ulx \n",__FILE__, dpir_val, reg_dpir);
			iowrite32(reg_dpir, reg_ir_dpir); //move the dpir setting in device tree
			iowrite32(0x7d7, reg_ir_cr);
			break;
		case PANASONIC:
			iowrite32(0x80000000, reg_ir_cr);
			iowrite32(0x24120d30, reg_ir_psr); // modulation length = 360us
			iowrite32(0x00010010, reg_ir_per);
			iowrite32(0x6767021b, reg_ir_sf);  // idle time and command time 70ms
//			iowrite32(0xda9, reg_ir_dpir);   // 70ms
			reg_dpir = (dpir_val*1000 *27)/(ioread32(reg_ir_sf)+1);
			RTK_debug(KERN_ALERT "[%s] get reg-ir-dpir = %uld us from dtb, regval=0x%ulx \n",__FILE__, dpir_val, reg_dpir);
			iowrite32(reg_dpir, reg_ir_dpir); //move the dpir setting in device tree
			iowrite32(0x00af05c0, reg_ir_cr);
			break;
		case RAW_RC6:
			iowrite32(0x80000000, reg_ir_cr);
#if defined(CONFIG_REALTEK_DARWIN) || defined(CONFIG_REALTEK_MACARTHUR)
			iowrite32(0xa8b, reg_ir_raw_sf); 		// use 10000 Hz sampling rate
#else
			iowrite32(0xa8b, reg_ir_sf); 		// use 10000 Hz sampling rate
#endif
			iowrite32(0x545, reg_ir_raw_deb);	// 2x over sampling for debounce
			iowrite32(0x03138848,  reg_ir_raw_ctrl);
			iowrite32(0x7300, reg_ir_cr); // enable raw mode
			RTK_debug(KERN_ALERT "[%s] %s %d \n", __FILE__,__FUNCTION__,__LINE__);
			break;

		case RC6_MODE_6:
			RTK_debug(KERN_ALERT "[%s] get irrp_protocol = RC6_MODE_6 \n",__FILE__);
			iowrite32(0x80000000, reg_ir_cr);
			iowrite32(0x00000123, reg_ir_ctrl_rc6);
			iowrite32(0x00111111, reg_ir_psr);
			iowrite32(0x00000000, reg_ir_per);
			iowrite32(0x0000021b, reg_ir_sf);
			reg_dpir = (dpir_val*1000 *27)/(ioread32(reg_ir_sf)+1);
			RTK_debug(KERN_ALERT "[%s] get reg-ir-dpir = %uld us from dtb, regval=0x%ulx \n",__FILE__, dpir_val, reg_dpir);
			iowrite32(reg_dpir, reg_ir_dpir); //move the dpir setting in device tree
			iowrite32(0x00a50720, reg_ir_cr);
			break;

		case TOSHIBA:
			RTK_debug(KERN_ALERT "Venus_IR_Init for TOSHIBA mode\n");
			iowrite32(0x80000000, reg_ir_cr);
			iowrite32(0x2d161643, reg_ir_psr);
			iowrite32(0x0001102d, reg_ir_per);
			iowrite32(0x0000021b, reg_ir_sf); // 50KHz
			reg_dpir = (dpir_val*1000 *27)/(ioread32(reg_ir_sf)+1);
			printk(KERN_ALERT "[%s] get reg-ir-dpir = %uld us from dtb, regval=0x%ulx \n",__FILE__, dpir_val, reg_dpir);
			iowrite32(reg_dpir, reg_ir_dpir); //move the dpir setting in device tree

			iowrite32(0x010007df, reg_ir_cr);
			break;
		case REPEAT_MODE:
			iowrite32(0x80000000, reg_ir_cr);
			iowrite32((sample_rate*27 - 1), reg_ir_sf);       // use 5000 Hz sampling rate
			iowrite32(0x1a, reg_ir_raw_deb);  // 2x over sampling for debounce	//27M /2700 = 10KHz
			iowrite32(0x03001050,  reg_ir_raw_ctrl);
			iowrite32(0x7300, reg_ir_cr); // enable raw mode
			break;
	}
#if defined(CONFIG_REALTEK_DARWIN) || defined(CONFIG_REALTEK_MACARTHUR)
		iowrite32(0x21, DARWIN_SPU_INT_EN);
#endif

	switch(tx_mode) {
		case NEC_TX:
			RTK_debug(KERN_ALERT "Venus_IR_Init for NEC_TX mode\n");
			//Inactive Ouput Level :   1: High level
			//but acturally the implement is low level by add an modify on pcb.
			iowrite32(0x00000400, reg_irtx_cfg);	//Inactive Ouput Level set 1 to default low

			// PWM=38kHz, carrier freq, duty factor = 1/3
			// PWM clock source divisor = 33MHz/2^(1+1)=8.25MHz =>0x1
			// PWM clock duty = 217/3=72 =>0x47
			// PWM output clock divisor = 8.25MHz/217=38.018kHz =>0xD8
			iowrite32(0x147d8, reg_ritx_pwm_setting);

			// measured unit-high width=560us => freq=1.786kHz
			// IRTX output frequncy = XTAL 33MHz / (IRTX_FD+1)=1.786kHz
			iowrite32(0x482c, reg_irtx_tim);

			// IRTX Enable
			// Inactive Ouput High Level (acturally the implement is low level)
			// IRTX Output Modulation disable
			// IRTX Endian Select: LSB first
			// FIFO Output Inverse Bypass
			// IRTX Output Inverse
			// IRTX STOP
			iowrite32(0x80000710, reg_irtx_cfg);

			// Data Threshold=16, Data finish Interrupt Enable, Data Empty Interrupt Disable, Data Request Interrupt Disable
			iowrite32(0xf04, reg_irtx_int_en);

			//tx fifo reset
			iowrite32(0x80000000, reg_irtx_fifo_st);	//irtx fifo reset
			iowrite32(0x00000000, reg_irtx_fifo_st);	//irtx fifo normal
			break;
		case REPEAT_MODE:
			iowrite32(0x00000400, reg_irtx_cfg);	//Inactive Ouput Level set 1 to default low
			iowrite32(0x13AB0, reg_ritx_pwm_setting);
			iowrite32((sample_rate*27 - 1), reg_irtx_tim);
			iowrite32(0x80000510, reg_irtx_cfg);
			iowrite32(0xf06, reg_irtx_int_en);

			//tx fifo reset
			iowrite32(0x80000000, reg_irtx_fifo_st);	//irtx fifo reset
			iowrite32(0x00000000, reg_irtx_fifo_st);	//irtx fifo normal

			break;
	}

	return retval;
}

// cyhuang (2011/11/19) +++
//                          Add for new device PM driver.
#ifdef CONFIG_PM
int venus_ir_pm_suspend(struct device *dev) {
	uint32_t regValue;
	
	struct RTK119X_ipc_shm __iomem *ipc = (void __iomem *)IPC_SHM_VIRT;
	struct RTK119X_ipc_shm_ir __iomem *ir_ipc = (void __iomem *)(IPC_SHM_VIRT +sizeof(struct RTK119X_ipc_shm));
	uint32_t phy_ir_ipc = RPC_COMM_PHYS + (0xC4) + sizeof(struct RTK119X_ipc_shm);

	if(irda_dev_count>1){
		venus_irtx_set_wake_up_keys(116, (u32*)ir_ipc);
		writel(__cpu_to_be32(phy_ir_ipc), &(ipc->ir_extended_tbl_pt));	
	}
	
#ifdef CONFIG_QUICK_HIBERNATION
	if (!in_suspend) {
		// flush IRRP,
		while(ioread32(reg_ir_sr) & 0x1) {
			iowrite32(0x00000003, reg_ir_sr); /* clear IRDVF */
			regValue = ioread32(reg_ir_rp); /* read IRRP */
		}
	}
#else
	// flush IRRP
	while(ioread32(reg_ir_sr) & 0x1) {
		iowrite32(0x00000003, reg_ir_sr); /* clear IRDVF */
		regValue = ioread32(reg_ir_rp); /* read IRRP */
	}
#endif

	// disable interrupt
	regValue = ioread32(reg_ir_cr);
	regValue = regValue & ~(0x400);
	iowrite32(regValue, reg_ir_cr);

	return 0;
}

int venus_ir_pm_resume(struct device *dev) {
	uint32_t regValue;

#ifdef CONFIG_QUICK_HIBERNATION
	if (!in_suspend) {
		/* reinitialize kfifo */
		Venus_IR_rp_fifoReset();

		// flush IRRP
		while(ioread32(reg_ir_sr) & 0x1) {
			iowrite32(0x00000003, reg_ir_sr); /* clear IRDVF */
			regValue = ioread32(reg_ir_rp); /* read IRRP */
		}
	}
#else
	/* reinitialize kfifo */
	Venus_IR_rp_fifoReset();

	// flush IRRP
	while(ioread32(reg_ir_sr) & 0x1) {
		iowrite32(0x00000003, reg_ir_sr); /* clear IRDVF */
		regValue = ioread32(reg_ir_rp); /* read IRRP */
	}
#endif

#ifdef CONFIG_HIBERNATION
	// reinitialize ir register
	Venus_IR_Init(ir_protocol, irtx_protocol);
#else
	Venus_IR_Init(ir_protocol, irtx_protocol);
	// enable interrupt
	regValue = ioread32(reg_ir_cr);
	regValue = regValue | 0x400;
	iowrite32(regValue, reg_ir_cr);
#endif
	mod_timer(&ir_wakeup_timer, (jiffies + (msecs_to_jiffies(1000))) );
	printk(KERN_EMERG"Venus IR resume end\n");
	return 0;
}
#endif

long venus_ir_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	int retval = 0;
	unsigned int reg;
	RTK_debug(KERN_ALERT "venus_ir_ioctl -- begin: cmd = %d \n ", cmd);	

	switch(cmd) {
		case VENUS_IR_IOC_SET_IRPSR:
			iowrite32(arg, reg_ir_psr);
			break;
		case VENUS_IR_IOC_SET_IRPER:
			iowrite32(arg, reg_ir_per);
			break;
		case VENUS_IR_IOC_SET_IRSF:
			iowrite32(arg, reg_ir_sf);
			break;
		case VENUS_IR_IOC_SET_IRCR:
			iowrite32(arg, reg_ir_cr);
			break;
		case VENUS_IR_IOC_SET_IRIOTCDP:
			iowrite32(arg, reg_ir_dpir);
			break;
		case VENUS_IR_IOC_SET_PROTOCOL:
			ir_protocol = (int)arg;
		case VENUS_IR_IOC_FLUSH_IRRP:
			if((retval = Venus_IR_Init(ir_protocol, irtx_protocol)) == 0)
				Venus_IR_rp_fifoReset();
			break;
		case VENUS_IR_IOC_SET_DEBOUNCE:
			debounce = (unsigned int)arg;
			break;
		case VENUS_IR_IOC_SET_FIRST_REPEAT_DELAY:
#ifdef DELAY_FIRST_REPEAT
			firstRepeatInterval = (unsigned int)arg;
#else
			retval = -EPERM;
#endif
			break;
		case VENUS_IR_IOC_SET_DRIVER_MODE:
			if(((unsigned int)arg) >= 2)
				retval = -EFAULT;
			else {
				Venus_IR_rp_fifoReset();
				driver_mode = (unsigned int)arg;
			}
			break;

		case VENUS_IRTX_IOC_SET_TX_TABLE:
			if (copy_from_user(&IRTx_KeycodeTable, ((void __user *)arg), sizeof(struct Venus_IRTx_KeycodeTable)))
				retval =  -EFAULT;
			IRTx_KeycodeTableEnable = 1 ;
			break;
		case VENUS_IRRX_RAW_STOP:
			disable_irq(rtk119x_ir_irq);
			reg = ioread32(reg_soft_rst);
			reg = reg & ~0x00000002;
			iowrite32(reg, reg_soft_rst);
			Venus_IR_rp_fifoReset();
			start_tx = 0;
			reg = ioread32(reg_soft_rst);
			reg = reg | 0x00000002;
			iowrite32(reg, reg_soft_rst);
			Venus_IR_Init(ir_protocol, irtx_protocol);
			enable_irq(rtk119x_ir_irq);
			break;
		default:
			retval = -ENOIOCTLCMD;
	}

	return retval;
}

#if 0 //def CONFIG_REALTEK_DARWIN
static void IR_polling_wrapper(unsigned long data)
{
	IR_interrupt_handler(0, NULL, NULL);
	venus_ir_timer.expires  = jiffies + HZ/10;
	init_timer(&venus_ir_timer);
	add_timer(&venus_ir_timer);
}
#endif /* CONFIG_REALTEK_DARWIN */


void venus_ir_reg_init(void) {
	if (!iso_irda_reg_base || !iso_sys_reg_base )
		panic("[%s] %s  %d venus_ir_reg_init fail \n", __FILE__,__FUNCTION__,__LINE__);
	reg_isr                			= (volatile unsigned int *)( (unsigned int)iso_sys_reg_base + SATURN_ISO_ISR_OFF);
	reg_soft_rst = (volatile unsigned int *)( (unsigned int)iso_sys_reg_base + 0x88);
	reg_ir_psr             		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_PSR_OFF);
	reg_ir_per             		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_PER_OFF);
	reg_ir_sf              		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_SF_OFF);
	reg_ir_dpir            		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_DPIR_OFF);
	reg_ir_cr              		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_CR_OFF);
	reg_ir_rp              		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_RP_OFF);
	reg_ir_sr              		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_SR_OFF);
	reg_ir_raw_ctrl        		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_RAW_CTRL_OFF);
	reg_ir_raw_ff         		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_RAW_FF_OFF);
	reg_ir_raw_sample_time = (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_RAW_SAMPLE_TIME_OFF);
	reg_ir_raw_wl          		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_RAW_WL_OFF);
	reg_ir_raw_deb         	= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_RAW_DEB_OFF);
	reg_ir_ctrl_rc6        		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_CTRL_RC6_OFF);
	reg_irtx_cfg				= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IRTX_CFG_OFF);
	reg_irtx_tim				= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IRTX_TIM_OFF);
	reg_ritx_pwm_setting	= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IRTX_PWM_SETTING_OFF);
	reg_irtx_int_en			= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IRTX_INT_EN_OFF);
	reg_irtx_int_st			= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IRTX_INT_ST_OFF);
	reg_irtx_fifo_st		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IRTX_FIFO_ST_OFF);
	reg_irtx_fifo			= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IRTX_FIFO_OFF);
	reg_irrcmm_timing		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IRRCMM_TIMING_OFF);
	reg_ir_cr1				= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IR_CR1_OFF);
	reg_irrcmm_apkb		= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IRRCMM_APKB_OFF);
	reg_irrxrclfifo			= (volatile unsigned int *)( (unsigned int)iso_irda_reg_base + SATURN_ISO_IRRXRCLFIFO_OFF);

}

#ifdef CONFIG_QUICK_HIBERNATION
int receive_ir_key(void)
{
	uint32_t regValue;
	uint32_t irrp2Value;
	int dataRepeat;
	int received = 0;

	RTK_debug(KERN_DEBUG "Venus IR: receive ir key...\n");

	if (ir_protocol == RAW_NEC) {
		regValue = raw_nec_decoder(&dataRepeat);
		return regValue;
	}
	else if(ir_protocol == RAW_RC6) {
		regValue = raw_rc6_decoder(&dataRepeat);
		return regValue;
	}

	if (Venus_IR_rp_fifoGetLen()) {
		Venus_IR_rp_fifoGet(&received, sizeof(uint32_t));
		RTK_debug(KERN_DEBUG "Venus IR: received key1 = [%08X]...\n", received);

		return received;
	}

	while (examine_ir_avail(&regValue, &irrp2Value, &dataRepeat) == 0) {

		RTK_debug(KERN_DEBUG "Venus IR: IRRP = [%08X]...\n", regValue);

		if (ir_protocol == RC6)
			regValue = ~(regValue) & 0x1fffff;
		else
			if (ir_protocol == RC5)
				regValue = (regValue & 0xfff);

		if (ir_protocol == PANASONIC && irrp2Value != 0x20020000) {

			RTK_debug(KERN_DEBUG "Venus IR: maker code = [%08X]...\n", irrp2Value);

		} else {
			received = regValue;

			RTK_debug(KERN_DEBUG "Venus IR: received key2 = [%08X]...\n", received);

			break;
		}
	}

	return received;
}
#endif

#ifdef DEV_DEBUG
void dump_keymap_tbl(void)
{
	int i;
	pr_info("=========================================\n");
	pr_info("=========================================\n");
	pr_info("ir keymap table : \n");
	for(i=0;i<p_rtk_key_table->size;i++)
	{
		pr_info("scancode=[0x%x] keycode=[%d]\n",p_rtk_key_table->keys[i].scancode, p_rtk_key_table->keys[i].keycode);
	}
	pr_info("=========================================\n");
	pr_info("=========================================\n");
}
#endif

static struct of_device_id rtk119x_irda_ids[] = {
	{ .compatible = "Realtek,rtk119x-irda" },
	{ /* Sentinel */ },
};
MODULE_DEVICE_TABLE(of, rtk119x_irda_ids);

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#if 0
/*gpio test start*/
static irqreturn_t gpio_test_interrupt_handler(int irq, void *dev_id)
{
		RTK_debug(KERN_DEBUG "Venus IRTX: Interrupt Handler Triggered.\n");
		return IRQ_HANDLED;
}
/*gpio test end*/
#endif


int rtk119x_ir_multiple_probe(unsigned int multiple_support)
{
	struct device_node *irda_node;
	const char* irda_dev[]={"irda0","irda1","irda2","irda3","irda4"};
	int i,j;
	
	int result;
   	const u32 *prop;
    int size;
    int err;
	unsigned int cust_code, scancode_msk, custcode_msk;
	struct venus_key_table *tmp_rtk_key_table;

	
	if(!multiple_support)
		return -EINVAL;
		
	if (of_property_read_u32(rtk119x_irda_node, "Realtek,irda_dev_count", &irda_dev_count)) {
		RTK_debug(KERN_ALERT "[WARNING]%s Parsing irda_dev_count fail\n");
		return -EINVAL;
	}
	else{		
		tmp_rtk_key_table = (struct venus_key_table*)kzalloc(irda_dev_count*sizeof(struct venus_key_table), GFP_KERNEL);		
		RTK_debug(KERN_ALERT "support %d irda devices\n", irda_dev_count);
	}	
								
	for(j=0; j<irda_dev_count;j++)
	{		
		irda_node = of_get_child_by_name(rtk119x_irda_node,irda_dev[j]);
		if (!irda_node) {
			 RTK_debug(KERN_ALERT "could not find %s sub-node\n",irda_dev[j]);
			return -EINVAL;
		}
		else
			RTK_debug(KERN_ALERT "find %s sub-node\n",irda_dev[j]);
			
		if (of_property_read_u32(irda_node, "Realtek,irrx-protocol", &ir_protocol)) {
			RTK_debug(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default irrx-protocol = %d .\n", __func__, ir_protocol);
			RTK_debug(KERN_ALERT "NOTE : NEC = 1, RC5 = 2, SHARP = 3, SONY = 4, C03 = 5, RC6 = 6, RAW_NEC = 7, RCA = 8, PANASONIC = 9, KONKA=10, RAW_RC6 = 11, RC6_MODE_6 = 12, TOSHIBA = 13\n");
		}

		if (of_property_read_u32(irda_node, "Realtek,irtx-protocol", &irtx_protocol)) {
			RTK_debug(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default irtx-protocol = %d .\n", __func__, irtx_protocol);
			RTK_debug(KERN_ALERT "NOTE : NEC_TX = 1, RC5_TX = 2, SHARP_TX = 3, SONY_TX = 4,	C03_TX = 5, RC6_TX = 6, RCA_TX = 7, PANASONIC_TX = 8, KONKA_TX=9\n");
		}
#define KEYMAP_TABLE_COLUMN 2		
		prop = of_get_property(irda_node, "Realtek,keymap-tbl", &size);
    	err = size % (sizeof(u32)*KEYMAP_TABLE_COLUMN);

		if (of_property_read_u32(irda_node, "Realtek,cust-code", &cust_code))
		{
			RTK_debug(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default cust_code\n", __func__);
			err++;
		}
		if(of_property_read_u32(irda_node, "Realtek,scancode-msk", &scancode_msk))
		{
			RTK_debug(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default scancode_msk\n", __func__);
			err++;
		}
		if(of_property_read_u32(irda_node, "Realtek,custcode-msk", &custcode_msk))
		{
			RTK_debug(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default custcode_msk\n", __func__);
			err++;
		}
			
		if ((prop) && (!err))
		{
			int counter = size / (sizeof(u32)*KEYMAP_TABLE_COLUMN);
			//Todo : could use devm_kzalloc to replace kzalloc (by toto)			
			tmp_rtk_key_table[j].keys = (struct venus_key*)kzalloc(sizeof(struct venus_key) * (counter+1), GFP_KERNEL);
			tmp_rtk_key_table[j].size = counter;
			tmp_rtk_key_table[j].cust_code = cust_code;
			tmp_rtk_key_table[j].scancode_msk = scancode_msk;
			tmp_rtk_key_table[j].custcode_msk = custcode_msk;
			RTK_debug(KERN_ALERT "[%s] %s  %d counter=%d cust_code=0x%x\n", __FILE__,__FUNCTION__,__LINE__,counter,cust_code); 	
			
			RTK_debug(KERN_ALERT "[%s] %s  %d scancode_msk=0x%x custcode_msk=0x%x\n", __FILE__,__FUNCTION__,__LINE__,scancode_msk,custcode_msk); 	
	
			for (i=0;i<counter;i+=1)
			{
				tmp_rtk_key_table[j].keys[i].scancode= of_read_number(prop, 1 + (i*KEYMAP_TABLE_COLUMN));
				tmp_rtk_key_table[j].keys[i].keycode = of_read_number(prop, 2 + (i*KEYMAP_TABLE_COLUMN));				
				RTK_debug(KERN_ALERT "[%s] %s  %d idx=%d : scancode=0x%x, keycode=%d\n", __FILE__,__FUNCTION__,__LINE__,i,tmp_rtk_key_table[j].keys[i].scancode,tmp_rtk_key_table[j].keys[i].keycode );				
			}
		} 
		else 
		{
			RTK_debug(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default keymap table rtk_mk5_tv_key_table.\n", __func__);
		}
		
		if (of_property_read_u32(irda_node, "Realtek,reg-ir-dpir", &dpir_val)) {
			printk(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default reg-ir-dpir = %d .\n", __func__, dpir_val);
		}	
					
	}

	p_rtk_key_table = tmp_rtk_key_table;
					
	return 0;	
}

int rtk119x_ir_probe(struct platform_device * pdev)
{
	int result;
	struct pinctrl *pinctrl;
   	const u32 *prop;
    int size;
    int err;
    int i;
	unsigned int cust_code, scancode_msk, custcode_msk;
	unsigned int multiple_support=0;

    spin_lock_init(&ir_splock);

	setup_timer(&ir_wakeup_timer, rtk_ir_wakeup_timer, 0);

	rtk119x_irda_node = pdev->dev.of_node;
	if (!rtk119x_irda_node)
		panic("No irda of node found");


	pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR(pinctrl)) {
		result = PTR_ERR(pinctrl);
		RTK_debug(KERN_ALERT "[%s] %s  %d  no pinctrl found %d  \n", __FILE__,__FUNCTION__,__LINE__,result);
	}

	result = sysfs_create_group(&pdev->dev.kobj, &irda_attr_group);
	if (result < 0) {
		dev_err(&pdev->dev, "rtk119x_ir_probe: sysfs_create_group() failed: %d\n", result);
		return result;
	}	
#if 0
{
/*gpio test start*/
	int res;
	unsigned int pin_num;
	pin_num = of_get_gpio_flags(rtk119x_irda_node, 0, NULL);


	if (gpio_is_valid(pin_num)) {
		res = gpio_request(pin_num, "nand_rdy");
		if (res < 0) {
			printk("can't request rdy gpio %d\n",
				pin_num);
		}

		res = gpio_direction_input(pin_num);
		if (res < 0) {
			printk("can't request input direction rdy gpio %d\n",
				pin_num);
		}
	}
/*
	if (request_irq(gpio_to_irq(pin_num),
			at91_vbus_irq, 0, driver_name, udc)) {
		DBG("request vbus irq %d failed\n",
		    udc->board.vbus_pin);
		retval = -EBUSY;
		goto fail3;
	}
*/
/*
	if(request_irq(gpio_to_irq(pin_num),
			gpio_test_interrupt_handler,
			IRQF_SHARED,
			"gpio_test_Venus_IR",
			IR_interrupt_handler)) {
		printk("gpio test IR: cannot register IRQ %d\n", gpio_to_irq(pin_num));
	}
*/
/*gpio test end*/
}
#endif

	if (rtk119x_irda_node) {
		iso_sys_reg_base = of_iomap(rtk119x_irda_node, 0);
		if (!iso_sys_reg_base)
			panic("Can't map iso_sys_reg_base for %s", rtk119x_irda_node->name);

		iso_irda_reg_base = of_iomap(rtk119x_irda_node, 1);
		if (!iso_irda_reg_base)
			panic("Can't map iso_irda_reg_base for %s", rtk119x_irda_node->name);

		RTK_debug(KERN_ALERT "[%s] %s  %d iso_sys_reg_base = %x \n", __FILE__,__FUNCTION__,__LINE__,(u32)iso_sys_reg_base);
		RTK_debug(KERN_ALERT "[%s] %s  %d iso_irda_reg_base = %x \n", __FILE__,__FUNCTION__,__LINE__,(u32)iso_irda_reg_base);
	}

	/* for multiple support 2015.04.22*/
	if (of_property_read_u32(rtk119x_irda_node, "Realtek,multiple_support", &multiple_support)) {
		RTK_debug(KERN_ALERT "[WARNING]%s Parsing DTB fail, default support one irda device\n", __func__);		
	}
	
	if(!rtk119x_ir_multiple_probe(multiple_support))
		goto INIT;
	

	if (of_property_read_u32(rtk119x_irda_node, "Realtek,irrx-protocol", &ir_protocol)) {
		RTK_debug(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default irrx-protocol = %d .\n", __func__, ir_protocol);
		RTK_debug(KERN_ALERT "NOTE : NEC = 1, RC5 = 2, SHARP = 3, SONY = 4, C03 = 5, RC6 = 6, RAW_NEC = 7, RCA = 8, PANASONIC = 9, KONKA=10, RAW_RC6 = 11, RC6_MODE_6 = 12, TOSHIBA = 13\n");
	}

	if (of_property_read_u32(rtk119x_irda_node, "Realtek,irtx-protocol", &irtx_protocol)) {
		RTK_debug(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default irtx-protocol = %d .\n", __func__, irtx_protocol);
		RTK_debug(KERN_ALERT "NOTE : NEC_TX = 1, RC5_TX = 2, SHARP_TX = 3, SONY_TX = 4,	C03_TX = 5, RC6_TX = 6, RCA_TX = 7, PANASONIC_TX = 8, KONKA_TX=9\n");
	}

#define KEYMAP_TABLE_COLUMN 2
	prop = of_get_property(pdev->dev.of_node, "Realtek,keymap-tbl", &size);
    	err = size % (sizeof(u32)*KEYMAP_TABLE_COLUMN);

	if (of_property_read_u32(rtk119x_irda_node, "Realtek,cust-code", &cust_code))
	{
		RTK_debug(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default cust_code\n", __func__);
		err++;
	}
	if(of_property_read_u32(rtk119x_irda_node, "Realtek,scancode-msk", &scancode_msk))
	{
		RTK_debug(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default scancode_msk\n", __func__);
		err++;
	}
	if(of_property_read_u32(rtk119x_irda_node, "Realtek,custcode-msk", &custcode_msk))
	{
		RTK_debug(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default custcode_msk\n", __func__);
		err++;
	}

	if ((prop) && (!err))
	{
		int counter = size / (sizeof(u32)*KEYMAP_TABLE_COLUMN);
		//Todo : could use devm_kzalloc to replace kzalloc (by toto)
		p_rtk_key_table = (struct venus_key_table*)kzalloc(sizeof(struct venus_key_table), GFP_KERNEL);
		p_rtk_key_table->keys = (struct venus_key*)kzalloc(sizeof(struct venus_key) * (counter+1), GFP_KERNEL);
		p_rtk_key_table->size = counter;
		p_rtk_key_table->cust_code = cust_code;
		p_rtk_key_table->scancode_msk = scancode_msk;
		p_rtk_key_table->custcode_msk = custcode_msk;
		RTK_debug(KERN_ALERT "[%s] %s  %d counter=%d cust_code=0x%x\n", __FILE__,__FUNCTION__,__LINE__,counter,cust_code); 	
		
		RTK_debug(KERN_ALERT "[%s] %s  %d scancode_msk=0x%x custcode_msk=0x%x\n", __FILE__,__FUNCTION__,__LINE__,scancode_msk,custcode_msk); 	

		for (i=0;i<counter;i+=1)
		{
			p_rtk_key_table->keys[i].scancode= of_read_number(prop, 1 + (i*KEYMAP_TABLE_COLUMN));
			p_rtk_key_table->keys[i].keycode = of_read_number(prop, 2 + (i*KEYMAP_TABLE_COLUMN));
			RTK_debug(KERN_ALERT "[%s] %s  %d idx=%d : scancode=0x%x, keycode=%d\n", __FILE__,__FUNCTION__,__LINE__,i,p_rtk_key_table->keys[i].scancode,p_rtk_key_table->keys[i].keycode );	
		}
	} else {
		RTK_debug(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default keymap table rtk_mk5_tv_key_table.\n", __func__);
	}
	
	if (of_property_read_u32(rtk119x_irda_node, "Realtek,reg-ir-dpir", &dpir_val)) {
		printk(KERN_ALERT "[WARNING]%s Parsing DTB fail, Use default reg-ir-dpir = %d .\n", __func__, dpir_val);
	}

INIT:
	of_property_read_u32(rtk119x_irda_node, "Realtek,ir-repeat-mode", &rpmode);
	if(rpmode) {
		if(of_property_read_u32(rtk119x_irda_node, "Realtek,ir-repeat-fifosize", rpmode_fifosize))
			rpmode_fifosize = 1024;
		if(of_property_read_u32(rtk119x_irda_node, "Realtek,ir-repeat-samplerate", sample_rate))
			sample_rate = 200;

		ir_protocol = REPEAT_MODE;
		irtx_protocol = REPEAT_MODE;
		rpmode_rxfifo = devm_kzalloc(&pdev->dev, rpmode_fifosize, GFP_KERNEL);
		rpmode_txfifo = devm_kzalloc(&pdev->dev, rpmode_fifosize, GFP_KERNEL);
	}
	
#ifdef DEV_DEBUG
	dump_keymap_tbl();
#endif

	venus_ir_reg_init();

#ifdef CONFIG_SUPPORT_INPUT_DEVICE
	if (venus_ir_input_init() != 0) {
		RTK_debug(KERN_ALERT "Venus IR: fail to register as an input device.\n");
	}
#endif

	if (venus_ir_rp_init() != 0) {
		RTK_debug(KERN_ALERT "Venus IR: fail to register as an input device.\n");
	}

	if (venus_ir_tx_init() != 0) {
		RTK_debug(KERN_ALERT "Venus IR: fail to register as an input device.\n");
	}

	/* Request IRQ */
	rtk119x_ir_irq = irq_of_parse_and_map(rtk119x_irda_node, 0);
	if (!rtk119x_ir_irq) {
		RTK_debug(KERN_ERR "Venus IR: fail to parse of irq.\n");
		return -ENXIO;
	}
	RTK_debug(KERN_ALERT "[%s] %s  %d irq number = %d \n", __FILE__,__FUNCTION__,__LINE__,rtk119x_ir_irq);

	if(request_irq(rtk119x_ir_irq,
			IR_interrupt_handler,
			IRQF_SHARED,
			"Venus_IR",
			IR_interrupt_handler)) {
		RTK_debug(KERN_ERR "IR: cannot register IRQ %d\n", rtk119x_ir_irq);
		result = -EIO;
		goto fail_alloc_irq;
	}

	/* Hardware Registers Initialization */
	Venus_IR_Init(ir_protocol, irtx_protocol);


	init_timer(&venus_ir_timer);
	venus_ir_timer.function = keypad_timer_expired;
	venus_ir_timer.data 	= (unsigned long)0;
	venus_ir_timer.expires	= jiffies + msecs_to_jiffies(70);
	add_timer(&venus_ir_timer);

#if 0
//#ifdef CONFIG_REALTEK_DARWIN
	/* this is hack for DARWIN */
printk(KERN_DEBUG "================================\n");
printk(KERN_DEBUG "================================\n");
printk(KERN_DEBUG "================================\n");
printk(KERN_DEBUG "======= Venus IR Timer =========\n");
//	init_timer(&venus_ir_timer);
//	venus_ir_timer.function = IR_polling_wrapper;
//	venus_ir_timer.data     = (unsigned long)0;
//	venus_ir_timer.expires  = jiffies + HZ/10;
//	add_timer(&venus_ir_timer);
printk(KERN_DEBUG "======= Venus IR Timer done=====\n");
printk(KERN_DEBUG "================================\n");
printk(KERN_DEBUG "================================\n");
printk(KERN_DEBUG "================================\n");
#endif /* CONFIG_REALTEK_DARWIN */
	return 0;	/* succeed ! */

fail_alloc_irq:
	return result;
}



//void rtk119x_ir_cleanup_module(void) {
int rtk119x_ir_cleanup_module(struct platform_device * pdev)
{
	del_timer_sync(&ir_wakeup_timer);

	/* Reset Hardware Registers */
	/* Venus: Disable IRUE */
	iowrite32(0x80000000, reg_ir_cr);

	/* Free IRQ Handler */
	free_irq(rtk119x_ir_irq, IR_interrupt_handler);

	venus_ir_rp_cleanup();
	venus_ir_tx_cleanup();
#ifdef CONFIG_SUPPORT_INPUT_DEVICE
	venus_ir_clean_user_keys();
	venus_ir_input_cleanup();
#endif

	return 0;
}

/* Register Macros */

static struct platform_driver rtk119x_ir_driver = {
	.driver		= {
		.name	= "rtk119x-ir",
		.owner	= THIS_MODULE,
		.of_match_table = rtk119x_irda_ids,
	},
	.probe		= rtk119x_ir_probe,
	.remove		= rtk119x_ir_cleanup_module,
};

//static int  rtk119x_ir_init(void)
//{
//	return platform_driver_register(&rtk119x_ir_driver);
//}


//module_init(rtk119x_ir_init);
//module_init(venus_ir_init_module);
//module_exit(venus_ir_cleanup_module);
module_platform_driver(rtk119x_ir_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("REALTEK Corporation");
MODULE_ALIAS("platform:irda");

