/*  Copyright (C) 2007-2014 Realtek Semiconductor Corporation.
 *  hdcp_top.c
 *
 */

#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>

#include <linux/miscdevice.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include "hdmi_reg_1195.h"
#include "hdcp.h"

struct hdcp hdcp;
struct hdcp_sha_in sha_input;
struct hdcp_ksvlist_info ksvlist_info;

static int hdcp_wait_re_entrance;

#ifdef CONFIG_HDCP_REPEATER
/* RX function, for HDCP repeater*/
extern void HdmiRx_disable_bcaps_ready(void);
extern void HdmiRx_save_tx_ksv(struct hdcp_ksvlist_info *tx_ksv);
#endif

/* State machine / workqueue */
//static void hdcp_wq_disable(void);
static void hdcp_wq_start_authentication(void);
static void hdcp_wq_check_r0(void);
static void hdcp_wq_step2_authentication(void);
static void hdcp_wq_authentication_failure(void);
static void hdcp_work_queue(struct work_struct *work);
static struct delayed_work *hdcp_submit_work(int event, int delay);
static void hdcp_cancel_work(struct delayed_work **work);


/* Callbacks */
//static void hdcp_start_frame_cb(void);
//static void hdcp_irq_cb(int hpd_low);

/* Control */
static long hdcp_enable_ctl(void __user *argp);
static long hdcp_disable_ctl(void);
static long hdcp_query_status_ctl(void __user *argp);


/* Driver */
static int __init hdcp_init(void);
static void __exit hdcp_exit(void);


static DECLARE_WAIT_QUEUE_HEAD(hdcp_up_wait_queue);
static DECLARE_WAIT_QUEUE_HEAD(hdcp_down_wait_queue);

void hdcp_set_state(struct miscdevice *pdev, int state);
struct miscdevice mdev;
/*-----------------------------------------------------------------------------
 * Function: hdcp_wq_start_authentication
 *-----------------------------------------------------------------------------
 */
static void hdcp_wq_start_authentication(void)
{
        int status = HDCP_OK;
	
        hdcp.hdcp_state = HDCP_AUTHENTICATION_START;
		hdcp.auth_state = HDCP_STATE_INIT;
		hdcp_set_state(&mdev, hdcp.auth_state);

        if (hdcp.print_messages)
                HDCP_INFO("authentication start\n");
	
        /* Step 1 part 1 (until R0 calc delay) */
        status = hdcp_lib_step1_start();
	
        if (status == -HDCP_AKSV_ERROR) {			
                hdcp_wq_authentication_failure();
        } else if (status == -HDCP_CANCELLED_AUTH) {
                HDCP_ERROR("Authentication step 1 cancelled.");
                return;
        } else if (status != HDCP_OK) {
                hdcp_wq_authentication_failure();			
        } else {
			
                hdcp.hdcp_state = HDCP_WAIT_R0_DELAY;
                hdcp.auth_state = HDCP_STATE_AUTH_1ST_STEP;
                hdcp.pending_wq_event = hdcp_submit_work(HDCP_R0_EXP_EVENT,HDCP_R0_DELAY);			
        }
		
}

/*-----------------------------------------------------------------------------
 * Function: hdcp_wq_check_r0
 *-----------------------------------------------------------------------------
 */
static void hdcp_wq_check_r0(void)
{
        int status = hdcp_lib_step1_r0_check();
	
        if (status == -HDCP_CANCELLED_AUTH) {
                HDCP_ERROR("Authentication step 1/R0 cancelled.");
                return;
        } else if (status < 0)
                hdcp_wq_authentication_failure();
        else {
                hdcp.fail_cnt = 0;
                hdcp.print_messages = 1;
                if (hdcp_lib_check_repeater_bit_in_tx()) {
                        /* Repeater */
                        HDCP_INFO("authentication step 1 "
                                         "successful - Repeater\n");

                        hdcp.hdcp_state = HDCP_WAIT_KSV_LIST;
                        hdcp.auth_state = HDCP_STATE_AUTH_2ND_STEP;

                        /*hdcp.pending_wq_event =
                                hdcp_submit_work(HDCP_KSV_TIMEOUT_EVENT,
                                                 HDCP_KSV_TIMEOUT_DELAY);*/
						hdcp.pending_wq_event =
                                hdcp_submit_work(HDCP_KSV_LIST_RDY_EVENT,500); 
                } else {
                        /* Receiver */
                        HDCP_INFO("authentication step 1 "
                                         "successful - Receiver\n");
#ifdef CONFIG_HDCP_REPEATER
						ksvlist_info.device_count = 0;
						ksvlist_info.bstatus[0] = 0;
						ksvlist_info.bstatus[1] = 0;
						HdmiRx_save_tx_ksv(&ksvlist_info);
#endif
                       // hdcp.av_mute_needed = 1;
                        hdcp.hdcp_state = HDCP_LINK_INTEGRITY_CHECK;
                        hdcp.auth_state = HDCP_STATE_AUTH_3RD_STEP;
						hdcp_set_state(&mdev, hdcp.auth_state);

                        /* Restore retry counter */
                        if (hdcp.en_ctrl->nb_retry == 0)
                                hdcp.retry_cnt = HDCP_INFINITE_REAUTH;
                        else
                                hdcp.retry_cnt = hdcp.en_ctrl->nb_retry;
							
                }
        }	
		
}



static void hdcp_wq_auto_check_r0(void)
{
		//HDCP_DEBUG("[%s] %s  %d :%u\n", __FILE__,__FUNCTION__,__LINE__,jiffies_to_msecs(jiffies));
	
        int status = hdcp_lib_r0_check();

        if (status == -HDCP_CANCELLED_AUTH) {
                HDCP_ERROR("Authentication step 3 cancelled.");
                return;
        } else if (status < 0)
		{		
                hdcp_wq_authentication_failure();
		}
        else {
                hdcp.fail_cnt = 0;
                hdcp.print_messages = 1;
               {
                        /* Receiver */
                        HDCP_DEBUG("hdcp_wq_auto_check_r0 "
											 "successful - %s\n",hdcp_lib_check_repeater_bit_in_tx() ? "Repeater" : "Receiver");

                       // hdcp.av_mute_needed = 1;
						hdcp.hdcp_state = HDCP_LINK_INTEGRITY_CHECK;
                        hdcp.auth_state = HDCP_STATE_AUTH_3RD_STEP;

                }
        }

}

irqreturn_t HDCP_interrupt_handler(int irq, void *dev_id)
{		
		if( INTST_get_riupdated( RD_REG_32(hdcp.hdcp_base_addr,INTST) ) )
		{
			hdcp.hdcp_state = HDCP_LINK_INTEGRITY_CHECK;
			hdcp_submit_work(HDCP_START_FRAME_EVENT, 0);					
		}
		WR_REG_32( hdcp.hdcp_base_addr,INTST,0);

		return IRQ_HANDLED;
}

/*-----------------------------------------------------------------------------
 * Function: hdcp_wq_step2_authentication
 *-----------------------------------------------------------------------------
 */
static void hdcp_wq_step2_authentication(void)
{

        int status = HDCP_OK;

		HDCP_DEBUG("[%s] %s  %d :%u\n", __FILE__,__FUNCTION__,__LINE__,jiffies_to_msecs(jiffies));
		
        /* KSV list timeout is running and should be canceled */
        hdcp_cancel_work(&hdcp.pending_wq_event);

        status = hdcp_lib_step2();

        if (status == -HDCP_CANCELLED_AUTH) {
                HDCP_ERROR("Authentication step 2 cancelled.");
                return;
        } else if (status < 0)
                hdcp_wq_authentication_failure();
        else {
                HDCP_INFO("Repeater authentication step 2 successful\n");

               // hdcp.av_mute_needed = 1;
                hdcp.hdcp_state = HDCP_LINK_INTEGRITY_CHECK;
                hdcp.auth_state = HDCP_STATE_AUTH_3RD_STEP;
				hdcp_set_state(&mdev, hdcp.auth_state);
			
                /* Restore retry counter */
                if (hdcp.en_ctrl->nb_retry == 0)
                        hdcp.retry_cnt = HDCP_INFINITE_REAUTH;
                else
                        hdcp.retry_cnt = hdcp.en_ctrl->nb_retry;

        }

}

/*-----------------------------------------------------------------------------
 * Function: hdcp_wq_authentication_failure
 *-----------------------------------------------------------------------------
 */
static void hdcp_wq_authentication_failure(void)
{
        if (hdcp.hdmi_state == HDMI_STOPPED) {
                hdcp.auth_state = HDCP_STATE_AUTH_FAILURE;
				hdcp_set_state(&mdev, hdcp.auth_state);
                return;
        }
		//HDCP_DEBUG("[%s] %s  %d :%u\n", __FILE__,__FUNCTION__,__LINE__,jiffies_to_msecs(jiffies));
        hdcp_lib_auto_ri_check(false);
      
        if (hdcp.av_mute_needed)
                hdcp_lib_set_av_mute(AV_MUTE_SET);
        hdcp_lib_set_encryption(HDCP_ENC_OFF);

        hdcp_cancel_work(&hdcp.pending_wq_event);

        if (hdcp.retry_cnt && (hdcp.hdmi_state != HDMI_STOPPED)) {
		
                if (hdcp.retry_cnt < HDCP_INFINITE_REAUTH) {
                        hdcp.retry_cnt--;
                        HDCP_ERROR("authentication failed - "
                                         "retrying, attempts=%d\n",
                                                        hdcp.retry_cnt);
                } else {
                        hdcp.fail_cnt++;
                        if (hdcp.print_messages) {
                                if (hdcp.fail_cnt < HDCP_MAX_FAIL_MESSAGES)
                                        HDCP_ERROR("authentication "
                                               "failed - retrying\n");
                                else {
                                        hdcp.print_messages = 0;
                                        HDCP_ERROR(" authentication "
                                               "failed %d consecutive times\n",
                                               hdcp.fail_cnt);
                                        HDCP_ERROR(" will keep "
                                               "trying but silencing logs "
                                               "until hotplug or success\n");
                                }
                        }
                }

                hdcp.hdcp_state = HDCP_AUTHENTICATION_START;
                hdcp.auth_state = HDCP_STATE_AUTH_FAIL_RESTARTING;

                hdcp.pending_wq_event = hdcp_submit_work(HDCP_AUTH_REATT_EVENT,
                                                         HDCP_REAUTH_DELAY);
        } else {
                HDCP_ERROR("authentication failed -HDCP disabled\n");
                hdcp.hdcp_state = HDCP_ENABLE_PENDING;
                hdcp.auth_state = HDCP_STATE_AUTH_FAILURE;
				hdcp_set_state(&mdev, hdcp.auth_state); 
        }
#ifdef CONFIG_HDCP_REPEATER
		HdmiRx_disable_bcaps_ready();
#endif
		//hdcp_set_state(&mdev, hdcp.auth_state);
}

/*-----------------------------------------------------------------------------
 * Function: hdcp_work_queue
 *-----------------------------------------------------------------------------
 */ 
 
static void hdcp_work_queue(struct work_struct *work)
{
        struct hdcp_delayed_work *hdcp_w = container_of(work, struct hdcp_delayed_work, work.work);
        int event = hdcp_w->event;
	
        mutex_lock(&hdcp.lock);

        HDCP_DEBUG("hdcp_work_queue() - START - %u hdmi=%d hdcp=%d auth=%d evt= %x %d",
                jiffies_to_msecs(jiffies),
                hdcp.hdmi_state,
                hdcp.hdcp_state,
                hdcp.auth_state,
                (event & 0xFF00) >> 8,
                event & 0xFF);

        /* Clear pending_wq_event
         * In case a delayed work is scheduled from the state machine
         * "pending_wq_event" is used to memorize pointer on the event to be
         * able to cancel any pending work in case HDCP is disabled
         */
        if (event & HDCP_WORKQUEUE_SRC)
		{		
                hdcp.pending_wq_event = 0;
		}

        /* First handle HDMI state */
        if (event == HDCP_START_FRAME_EVENT) 
		{							
                hdcp.pending_start = 0;
                hdcp.hdmi_state = HDMI_STARTED;
        }
		
		if (event == HDCP_DISABLE_CTL) 
		{
			hdcp.retry_cnt = 0;
			hdcp_wq_authentication_failure();
		}
	
        /**********************/
        /* HDCP state machine */
        /**********************/
        switch (hdcp.hdcp_state) {

        /* State */
        /*********/
        case HDCP_DISABLED:
                /* HDCP enable control or re-authentication event */
                if (event == HDCP_ENABLE_CTL) {
										
                        if (hdcp.en_ctrl->nb_retry == 0)
                                hdcp.retry_cnt = HDCP_INFINITE_REAUTH;
                        else
                                hdcp.retry_cnt = hdcp.en_ctrl->nb_retry;								

                        if (hdcp.hdmi_state == HDMI_STARTED)
								hdcp_wq_start_authentication();
                        else
                                hdcp.hdcp_state = HDCP_ENABLE_PENDING;
												
                }

                break;

        /* State */
        /*********/
        case HDCP_ENABLE_PENDING:
                /* HDMI start frame event */
                if (event == HDCP_START_FRAME_EVENT)
                        hdcp_wq_start_authentication();		

                break;

        /* State */
        /*********/
        case HDCP_AUTHENTICATION_START:
                /* Re-authentication */
                if (event == HDCP_AUTH_REATT_EVENT)
                        hdcp_wq_start_authentication();

                break;

        /* State */
        /*********/
        case HDCP_WAIT_R0_DELAY:
                /* R0 timer elapsed */
                if (event == HDCP_R0_EXP_EVENT)
                       hdcp_wq_check_r0();
                break;

        /* State */
        /*********/
        case HDCP_WAIT_KSV_LIST:
                /* Ri failure */
                if (event == HDCP_RI_FAIL_EVENT) {
                        
						HDCP_ERROR("Ri check failure\n");
                        hdcp_wq_authentication_failure();
                }
                /* KSV list ready event */
                else if (event == HDCP_KSV_LIST_RDY_EVENT)
                        hdcp_wq_step2_authentication();
                /* Timeout */
                else if (event == HDCP_KSV_TIMEOUT_EVENT) {
                        HDCP_ERROR("BCAPS polling timeout\n");
                        hdcp_wq_authentication_failure();
                }
                break;

        /* State */
        /*********/
        case HDCP_LINK_INTEGRITY_CHECK:							
				hdcp_wq_auto_check_r0();
                /* Ri failure */
                if (event == HDCP_RI_FAIL_EVENT) {
                        HDCP_ERROR("Ri check failure\n");
                        hdcp_wq_authentication_failure();
                }
                break;

        default:
                HDCP_ERROR("error - unknow HDCP state\n");
                break;
        }
        kfree(hdcp_w);
        hdcp_w = 0;
        if (event == HDCP_START_FRAME_EVENT)
                hdcp.pending_start = 0;
        if (event == HDCP_KSV_LIST_RDY_EVENT ||
            event == HDCP_R0_EXP_EVENT) {
                hdcp.pending_wq_event = 0;
        }

        HDCP_DEBUG("hdcp_work_queue() - END - %u hdmi=%d hdcp=%d auth=%d evt=%x %d ",
                jiffies_to_msecs(jiffies),
                hdcp.hdmi_state,
                hdcp.hdcp_state,
                hdcp.auth_state,
                (event & 0xFF00) >> 8,
                event & 0xFF);
				
        mutex_unlock(&hdcp.lock);		
}

/*-----------------------------------------------------------------------------
 * Function: hdcp_submit_work
 *-----------------------------------------------------------------------------
 */
static struct delayed_work *hdcp_submit_work(int event, int delay)
{

        struct hdcp_delayed_work *work;

        work = kmalloc(sizeof(struct hdcp_delayed_work), GFP_ATOMIC);

        if (work) {
                INIT_DELAYED_WORK(&work->work, hdcp_work_queue);
                work->event = event;
                queue_delayed_work(hdcp.workqueue,
                                   &work->work,
                                   msecs_to_jiffies(delay));
        } else {
                HDCP_ERROR("Cannot allocate memory to "
                                    "create work\n");
                return 0;
        }

        return &work->work;
}

/*-----------------------------------------------------------------------------
 * Function: hdcp_cancel_work
 *-----------------------------------------------------------------------------
 */
static void hdcp_cancel_work(struct delayed_work **work)
{
        int ret = 0;

        if (*work) {
                ret = cancel_delayed_work(*work);
                if (ret != 1) {
                        ret = cancel_work_sync(&((*work)->work));
                        HDCP_ERROR("Canceling work failed - "
                                         "cancel_work_sync done %d\n", ret);
                }
                kfree(*work);
                *work = 0;
        }
}

/******************************************************************************
 * HDCP control from ioctl
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Function: hdcp_enable_ctl
 *-----------------------------------------------------------------------------
 */
static long hdcp_enable_ctl(void __user *argp)
{
        //HDCP_DEBUG("[%s] %s  %d :%u\n", __FILE__,__FUNCTION__,__LINE__,jiffies_to_msecs(jiffies));
	
		hdcp.hdcp_enabled=1;
		
        if (hdcp.en_ctrl == 0) {			
                hdcp.en_ctrl = kmalloc(sizeof(struct hdcp_enable_control),GFP_KERNEL);

                if (hdcp.en_ctrl == 0) {
                        HDCP_ERROR("Cannot allocate memory for HDCP"
											" enable control struct\n");
                        return -EFAULT;
                }
        }
		
        if (copy_from_user(hdcp.en_ctrl, argp,sizeof(struct hdcp_enable_control))) 
		{
                HDCP_ERROR("Error copying from user space "
                                    "- enable ioctl\n");
                return -EFAULT;
        }
			
        /* Post event to workqueue */
        if (hdcp_submit_work(HDCP_ENABLE_CTL, 0) == 0)
                return -EFAULT;
		
        return 0;
}

/*-----------------------------------------------------------------------------
 * Function: hdcp_disable_ctl
 *-----------------------------------------------------------------------------
 */
static long hdcp_disable_ctl(void)
{
        HDCP_DEBUG("[%s] %s  %d :%u\n", __FILE__,__FUNCTION__,__LINE__,jiffies_to_msecs(jiffies));
        
		hdcp.hdcp_enabled=0;
		
		hdcp_cancel_work(&hdcp.pending_start);
        hdcp_cancel_work(&hdcp.pending_wq_event);

		hdcp_lib_auto_ri_check(false);    
        hdcp_lib_set_encryption(HDCP_ENC_OFF);
				
		/* Variable init */
        //hdcp.en_ctrl  = 0;
        hdcp.hdcp_state = HDCP_DISABLED;
        hdcp.pending_start = 0;
        hdcp.pending_wq_event = 0;
        hdcp.retry_cnt = 0;
        hdcp.auth_state = HDCP_STATE_DISABLED;
        //ddc.pending_disable = 0;
        hdcp.hdcp_up_event = 0;
        hdcp.hdcp_down_event = 0;
        hdcp_wait_re_entrance = 0;
        hdcp.hpd_low = 0;  
		hdcp.hdmi_state = HDMI_STARTED;

        return 0;
}

/*-----------------------------------------------------------------------------
 * Function: hdcp_query_status_ctl
 *-----------------------------------------------------------------------------
 */
static long hdcp_query_status_ctl(void __user *argp)
{
        uint32_t *status = (uint32_t *)argp;

        HDCP_DEBUG("[%s] %s  %d :%u\n", __FILE__,__FUNCTION__,__LINE__,jiffies_to_msecs(jiffies));

        *status = hdcp.auth_state;

        return 0;
}


static long hdcp_query_sink_hdcp_capable_ctl(void __user *argp)
{
        uint32_t *hdcp_capable = (uint32_t *)argp;

        HDCP_DEBUG("[%s] %s  %d :%u\n", __FILE__,__FUNCTION__,__LINE__,jiffies_to_msecs(jiffies));

		if( hdcp_lib_query_sink_hdcp_capable())
        {
            hdcp_set_state(&mdev, hdcp.auth_state);
			*hdcp_capable = HDCP_INCAPABLE;
            HDCP_INFO("[%s] HDCP_INCAPABLE, auth_state(%d)\n",__func__,hdcp.auth_state);
        }
		else
			*hdcp_capable = HDCP_CAPABLE;

        return 0;
}

static long hdcp_get_downstream_KSVlist_ctl(void __user *argp)
{	
        HDCP_DEBUG("[%s] %s  %d :%u\n", __FILE__,__FUNCTION__,__LINE__,jiffies_to_msecs(jiffies));
				
		if (copy_to_user(argp, &ksvlist_info ,sizeof(struct hdcp_ksvlist_info))) {
			HDCP_DEBUG("[%s]ksvlist info failed to copy to user !\n", __func__);
			return -EFAULT;
		}	
		
        return 0;		
}

static long hdcp_set_22_cipher_ctl(void __user *argp)
{	
        HDCP_DEBUG("[%s] %s  %d :%u\n", __FILE__,__FUNCTION__,__LINE__,jiffies_to_msecs(jiffies));
		
		hdcp_lib_set_22_cipher((HDCP_22_CIPHER_INFO *)argp);
		        
		return 0;        
}

static long hdcp_control_22_cipher_ctl(void __user *argp)
{	
        int flag ;
		
		HDCP_DEBUG("[%s] %s  %d :%u\n", __FILE__,__FUNCTION__,__LINE__,jiffies_to_msecs(jiffies));
						
		if (copy_from_user(&flag, argp, sizeof(flag))) {
			HDCP_DEBUG("[%s]failed to copy from user !\n", __func__);
			return -EFAULT;
		}
				
		hdcp_lib_control_22_cipher(flag);
		        
		return 0;        
}

int hdcp_open(struct inode *inode, struct file *filp)
{	
	
	if (nonseekable_open(inode, filp))
		return -ENODEV;
		
	return 0; 	
}
/*-----------------------------------------------------------------------------
 * Function: hdcp_ioctl
 *-----------------------------------------------------------------------------
 */
long hdcp_ioctl(struct file *fd, unsigned int cmd, unsigned long arg)
{
        void __user *argp = (void __user *)arg;
		
		HDCP_DEBUG("[%s] %s  %d :%u\n", __FILE__,__FUNCTION__,__LINE__,jiffies_to_msecs(jiffies));
	
        switch (cmd) {
        case HDCP_ENABLE:
                return hdcp_enable_ctl(argp);
				
        case HDCP_DISABLE:
                return hdcp_disable_ctl();

        case HDCP_QUERY_STATUS:
                return hdcp_query_status_ctl(argp);
				
		case HDCP_QUERY_SINK_HDCP_CAPABLE:		
				return hdcp_query_sink_hdcp_capable_ctl(argp);
				
		case HDCP_GET_DOWNSTREAM_KSVLIST:		
				return hdcp_get_downstream_KSVlist_ctl(argp);

		case HDCP_SET_22_CIPHER:		
				return hdcp_set_22_cipher_ctl(argp);

		case HDCP_CONTROL_22_CIPHER:		
				return hdcp_control_22_cipher_ctl(argp);			

        default:
                return -ENOTTY;
        } /* End switch */
}


/******************************************************************************
 * HDCP driver init/exit
 *****************************************************************************/


static struct file_operations hdcp_fops = {
        .owner = THIS_MODULE,
		.open	    	= hdcp_open,
        .unlocked_ioctl = hdcp_ioctl,
};




static const struct of_device_id rtk_hdcp_dt_ids[] = {
     { .compatible = "Realtek,rtk119x-hdcp", },
     {},
};


static ssize_t hdcp_state_show(struct device *device, struct device_attribute *attr, char *buffer)
{	
	HDCP_DEBUG("%s\n",__FUNCTION__);
	return sprintf(buffer, "%d", hdcp.auth_state);	
}

static ssize_t hdcp_name_show(struct device *device, struct device_attribute *attr, char *buffer)
{	
	HDCP_DEBUG("%s\n",__FUNCTION__);	
	return sprintf(buffer, "%s\n", "hdcp");	
}

void hdcp_set_state(struct miscdevice *pdev, int state)
{
	char name_buf[120];
	char state_buf[120];
	char *prop_buf;
	char *envp[3];
	int env_offset = 0;
	int length;

		prop_buf = (char *)get_zeroed_page(GFP_KERNEL);
		if (prop_buf) {
			length = hdcp_name_show(pdev->this_device, NULL, prop_buf);
			if (length > 0) {
				if (prop_buf[length - 1] == '\n')
					prop_buf[length - 1] = 0;
				snprintf(name_buf, sizeof(name_buf),"HDCP1x_NAME=%s", prop_buf);
				envp[env_offset++] = name_buf;				
			}
			length = hdcp_state_show(pdev->this_device, NULL, prop_buf);
			if (length > 0) {
				if (prop_buf[length - 1] == '\n')
					prop_buf[length - 1] = 0;
				snprintf(state_buf, sizeof(state_buf),"HDCP1x_STATE=%s", prop_buf);
				envp[env_offset++] = state_buf;
			}		
			envp[env_offset] = NULL;		
			kobject_uevent_env(&pdev->this_device->kobj, KOBJ_CHANGE, envp);
			free_page((unsigned long)prop_buf);
		} else {
			printk(KERN_ERR "out of memory in %s\n",__FUNCTION__);
			kobject_uevent(&pdev->this_device->kobj, KOBJ_CHANGE);
		}
	
}

static DEVICE_ATTR(state, S_IRUGO, hdcp_state_show, NULL);
static DEVICE_ATTR(name, S_IRUGO, hdcp_name_show, NULL);
/*-----------------------------------------------------------------------------
 * Function: hdcp_init
 *-----------------------------------------------------------------------------
 */
static int __init hdcp_init(void)
{
        		
		struct device_node __maybe_unused *hdcp_node;
		int ret;
		
		//HDCP_DEBUG("[%s] %s  %d :%u\n", __FILE__,__FUNCTION__,__LINE__,jiffies_to_msecs(jiffies));
	
		hdcp_node = of_find_matching_node(NULL, rtk_hdcp_dt_ids);
		if (!hdcp_node)
			printk(KERN_ALERT "no matching hdcp node\n");
		
				
        hdcp.hdcp_base_addr = of_iomap(hdcp_node , 0);
		//HDCP_DEBUG("hdcp.hdcp_base_addr=0x%x\n", (unsigned int)hdcp.hdcp_base_addr);

        if (!hdcp.hdcp_base_addr) {
                printk(KERN_ERR "HDCP: HDCP iomap error\n");
                goto err_map_hdcp;
				//return -EFAULT;
        }
		
		hdcp.hdcp_irq_num = irq_of_parse_and_map(hdcp_node, 0);		
		//HDCP_DEBUG("hdcp.hdcp_irq_num=%d\n", hdcp.hdcp_irq_num);
		if (!hdcp.hdcp_irq_num) 
			printk(KERN_ALERT "map hdcp irq num failed\n");
	

        mutex_init(&hdcp.lock);

        mdev.minor = MISC_DYNAMIC_MINOR;
        mdev.name = "hdcp";
        mdev.mode = 0666;
        mdev.fops = &hdcp_fops;

        if (misc_register(&mdev)) {
                printk(KERN_ERR "HDCP: Could not add character driver\n");
                goto err_register;
        }
		
        mutex_lock(&hdcp.lock);

        /* Variable init */
        hdcp.en_ctrl  = 0;
        hdcp.hdcp_state = HDCP_DISABLED;
        hdcp.pending_start = 0;
        hdcp.pending_wq_event = 0;
        hdcp.retry_cnt = 0;
        hdcp.auth_state = HDCP_STATE_DISABLED;	
        //ddc.pending_disable = 0;
        hdcp.hdcp_up_event = 0;
        hdcp.hdcp_down_event = 0;
        hdcp_wait_re_entrance = 0;
        hdcp.hpd_low = 0;
		hdcp.hdcp_enabled=-1;
		
		ret = device_create_file(mdev.this_device, &dev_attr_state);

		if (ret < 0) {
			goto err_add_driver;
		}
		
		ret = device_create_file(mdev.this_device, &dev_attr_name);

		if (ret < 0) {
			goto err_add_driver;
		}

        spin_lock_init(&hdcp.spinlock);

        hdcp.workqueue = create_singlethread_workqueue("hdcp");
        if (hdcp.workqueue == NULL)
                goto err_add_driver;


        hdcp.hdmi_state = HDMI_STARTED;
		
		memset(&ksvlist_info,0x0,sizeof(struct hdcp_ksvlist_info));
        mutex_unlock(&hdcp.lock);
				
        return 0;

err_add_driver:
        misc_deregister(&mdev);

err_register:
        mutex_destroy(&hdcp.lock);
        iounmap(hdcp.hdcp_base_addr);

err_map_hdcp:
        return -EFAULT;
}

/*-----------------------------------------------------------------------------
 * Function: hdcp_exit
 *-----------------------------------------------------------------------------
 */
static void __exit hdcp_exit(void)
{
       //HDCP_DEBUG("[%s] %s  %d :%u\n", __FILE__,__FUNCTION__,__LINE__,jiffies_to_msecs(jiffies));

        mutex_lock(&hdcp.lock);

        kfree(hdcp.en_ctrl);

        /* Un-register HDCP callbacks to HDMI library */
        //omapdss_hdmi_register_hdcp_callbacks(0, 0, 0);
   
        misc_deregister(&mdev);

        /* Unmap HDCP */
        iounmap(hdcp.hdcp_base_addr);

        destroy_workqueue(hdcp.workqueue);

        mutex_unlock(&hdcp.lock);

        mutex_destroy(&hdcp.lock);
}

/*-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */
module_init(hdcp_init);
module_exit(hdcp_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Realtek HDCP kernel module");
MODULE_AUTHOR("Wilma Wu");
