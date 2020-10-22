/*
 * Register Address for Saturn
 */
#include <linux/kernel.h>	/* DBG_PRINT() */
#include <linux/types.h>	/* size_t */

//#define DEV_DEBUG
#ifdef DEV_DEBUG
#define RTK_debug(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define RTK_debug(fmt, ...)
#endif


#define SATURN_ISO_ISR_OFF						(0x00)
#define SATURN_ISO_IR_PSR_OFF					(0x00)
#define SATURN_ISO_IR_PER_OFF					(0x04)
#define SATURN_ISO_IR_SF_OFF					(0x08)
#define SATURN_ISO_IR_DPIR_OFF				(0x0c)
#define SATURN_ISO_IR_CR_OFF					(0x10)
#define SATURN_ISO_IR_RP_OFF					(0x14)
#define SATURN_ISO_IR_SR_OFF					(0x18)
#define SATURN_ISO_IR_RAW_CTRL_OFF			(0x1c)
#define SATURN_ISO_IR_RAW_FF_OFF				(0x20)
#define SATURN_ISO_IR_RAW_SAMPLE_TIME_OFF	(0x24)
#define SATURN_ISO_IR_RAW_WL_OFF				(0x28)
#define SATURN_ISO_IR_RAW_DEB_OFF			(0x2c)
#define SATURN_ISO_IR_PSR_UP_OFF				(0x30)
#define SATURN_ISO_IR_PER_UP_OFF				(0x34)
#define SATURN_ISO_IR_CTRL_RC6_OFF			(0x38)
#define SATURN_ISO_IR_RP2_OFF					(0x3C)
#define SATURN_ISO_IRTX_CFG_OFF				(0x40)
#define SATURN_ISO_IRTX_TIM_OFF				(0x44)
#define SATURN_ISO_IRTX_PWM_SETTING_OFF		(0x48)
#define SATURN_ISO_IRTX_INT_EN_OFF			(0x4c)
#define SATURN_ISO_IRTX_INT_ST_OFF			(0x50)
#define SATURN_ISO_IRTX_FIFO_ST_OFF			(0x54)
#define SATURN_ISO_IRTX_FIFO_OFF				(0x58)
#define SATURN_ISO_IRRCMM_TIMING_OFF			(0x60)
#define SATURN_ISO_IR_CR1_OFF					(0x64)
#define SATURN_ISO_IRRCMM_APKB_OFF			(0x68)
#define SATURN_ISO_IRRXRCLFIFO_OFF			(0x6C)

#define LIBRA_MS_CUSTOMER_CODE 		0x08F7
#define JAECS_T118_CUSTOMER_CODE		0xFC03
#define RTK_MK3_CUSTOMER_CODE		0xB649
#define YK_70LB_CUSTOMER_CODE			0x0E
#define RTK_MK4_CUSTOMER_CODE		0x6B86
#define RTK_MK5_CUSTOMER_CODE		0x7F80
#define NETG_MS_CUSTOMER_CODE		0x18
#define YK_54LU_CUSTOMER_CODE		0xf1f1
#define RTK_MK5_2_CUSTOMER_CODE		0xfb04
#define RTK_RC6_MODE_6_CUSTOMER_CODE	0x800F // add for Philip RC6 mode 6

struct venus_key {
	u32 scancode;
	u32 keycode;
};

struct venus_key_table {
	struct venus_key *keys;
	int size;
	unsigned int cust_code;
	unsigned int scancode_msk;
	unsigned int custcode_msk;
};

struct Venus_IRTx_KeycodeTable {
	int irtx_protocol;
	u32 irtx_keycode_list_len;
	u32 irtx_keycode_list[2048];
};

struct RTK119X_ir_wake_up_key {
uint32_t		protocol;
uint32_t		ir_scancode_mask;
uint32_t		ir_wakeup_scancode;
uint32_t		ir_cus_mask;
uint32_t		ir_cus_code;
};

struct RTK119X_ipc_shm_ir {
uint32_t		RTK119X_ipc_shm_ir_magic;
uint32_t		dev_count;
struct RTK119X_ir_wake_up_key key_tbl[5];	
};

#ifdef CONFIG_PM	
int venus_ir_pm_suspend(struct device *dev);
int venus_ir_pm_resume(struct device *dev) ;
#endif
long venus_ir_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
void venus_ir_disable_irtx(void);
void venus_ir_enable_irrp(void);
void venus_ir_disable_irrp(void);
void	venus_ir_set_keybit(unsigned long *addr);
void	venus_ir_set_relbit(unsigned long *addr);
int venus_ir_setkeycode(unsigned int scancode, unsigned int new_keycode, unsigned int *p_old_keycode, unsigned long * keybit_addr);
int venus_ir_getkeycode(u32 scancode, u32* p_keycode);
int venus_irtx_send(u32 keycode);
void venus_irtx_raw_send(int write_Count);

