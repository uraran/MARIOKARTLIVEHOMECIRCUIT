#ifndef _RTK_IPC_SHM_H_
#define _RTK_IPC_SHM_H_

struct RTK119X_ipc_shm {
/*0C4*/	uint32_t		sys_assign_serial;
/*0C8*/	uint32_t		pov_boot_vd_std_ptr;
/*0CC*/	uint32_t		pov_boot_av_info;
/*0D0*/	uint32_t		audio_rpc_flag;
/*0D4*/	uint32_t		suspend_mask;
/*0D8*/	uint32_t		suspend_flag;
/*0DC*/ uint32_t		vo_vsync_flag;
/*0E0*/	uint32_t		audio_fw_entry_pt;
/*0E4*/	uint32_t		power_saving_ptr;
/*0E8*/	unsigned char	printk_buffer[24];
/*100*/	uint32_t		ir_extended_tbl_pt;
/*104*/	uint32_t		vo_int_sync;
/*108*/	uint32_t		bt_wakeup_flag;//Bit31~24:magic key(0xEA), Bit23:high-active(1) low-active(0), Bit22~0:mask
/*10C*/	uint32_t		ir_scancode_mask;
/*110*/	uint32_t		ir_wakeup_scancode;
/*114*/	uint32_t	    suspend_wakeup_flag;                /* [31-24] magic key(0xEA) [5] cec [4] timer [3] alarm [2] gpio [1] ir [0] lan , (0) disable (1) enable */
/*118*/	uint32_t	    acpu_resume_state;                  /* [31-24] magic key(0xEA) [23-16] enum { NONE = 0, UNKNOW, IR, GPIO, LAN, ALARM, TIMER, CEC}  [15-0] ex GPIO Number */
/*11C*/	uint32_t        gpio_wakeup_enable;                 /* [31-24] magic key(0xEA) [20-0] mapping to the number of iso gpio 0~20 */
/*120*/	uint32_t        gpio_wakeup_activity;               /* [31-24] magic key(0xEA) [20-0] mapping to the number of iso gpio 0~20 , (0) low activity (1) high activity */
/*124*/	uint32_t        gpio_output_change_enable;          /* [31-24] magic key(0xEA) [20-0] mapping to the number of iso gpio 0~20 */
/*128*/	uint32_t        gpio_output_change_activity;        /* [31-24] magic key(0xEA) [20-0] mapping to the number of iso gpio 0~20 , (0) low activity (1) high activity AT SUSPEND TIME */
/*12C*/ uint32_t        audio_reciprocal_timer_sec;         /* [31-24] magic key(0xEA) [23-0] audio reciprocal timer (sec) */
/*130*/	uint32_t	u_boot_version_magic;
/*134*/	uint32_t	u_boot_version_info;
/*138*/ uint32_t	suspend_watchdog;		/* [31-24] magic key(0xEA) [23] state (0) disable (1) enable [22] acpu response state [15-0] watch timeout (sec) */
/*13C*/ unsigned char	reserved[36];			/* reserve 0x13C ~ 0x160 */
};
#endif // _RTK_IPC_SHM_H_
