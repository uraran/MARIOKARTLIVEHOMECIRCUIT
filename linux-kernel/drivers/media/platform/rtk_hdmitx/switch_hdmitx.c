#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/switch.h>
#include <linux/device.h>

#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>

#include "hdmitx.h"
#include "hdmitx_dev.h"
#include "hdmitx_api.h"

#define HDMI_SWITCH_NAME "hdmi"

#ifdef CONFIG_HDCP_REPEATER
extern void HdmiRx_hotplug_pulses(void);
#endif

static struct hdmitx_switch_data s_data;
static struct switch_dev *sdev = NULL;

#if 0 //for auto detecting rx sense.
#include <linux/sched.h>   
#include <linux/kthread.h> 
#include <linux/err.h> 	
#include "hdmitx_reg.h"

static struct task_struct *rxsense_task; 
int rtk_hdmitx_init_polling_rx_sense(void);
static int rxsense_thread(void *arg);
#endif

#if 0
static ssize_t hdmitx_show(struct device *device, struct device_attribute *attr, char *buffer)
{
	
	HDMI_DEBUG("hdmi_show_interface");
	return sprintf(buffer, "%d", s_data.state);
	
}

static ssize_t hdmitx_store(struct device *device, struct device_attribute *attr, const char *buffer, ssize_t count)
{
        int state;
		
        state = simple_strtol(buffer, NULL, 10);
        s_data.state = (state) ? 1 : 0;
        schedule_work(&s_data.work);
		HDMI_DEBUG("hdmi_store_interface");
        return count;
}

static DEVICE_ATTR(hdmitx, 0777, hdmitx_show, hdmitx_store);
#endif
static ssize_t hdmitx_switch_print_state(struct switch_dev *sdev, char *buffer)
{
	HDMI_DEBUG("hdmitx_switch_print_state");
	return sprintf(buffer, "%d", s_data.state);
}

static void hdmitx_switch_work_func(struct work_struct *work) 
{
	int state =0,sink_changed=0;		
	hdmitx_device_t *pdev = container_of(sdev, hdmitx_device_t,sdev);

	state = gpio_get_value(s_data.pin);	
	s_data.state = state;

	//for auto detecting rx sense.	
	//if(state == HDMI_5V_HPD)			
	//	s_data.state |= HDMI_5V_HPD;
	//else
	//	s_data.state &= ~HDMI_5V_HPD;

	HDMI_INFO("%s:start", state?"plugged in":"pulled out");
	
	if(state == 1)
		sink_changed = hdmitx_get_sink_capability((asoc_hdmi_t*)hdmitx_get_drvdata(pdev));
	else
    {       
		hdmitx_turn_off_tmds(HDMI_MODE_HDMI);     
		hdmitx_reset_sink_capability((asoc_hdmi_t*)hdmitx_get_drvdata(pdev));
    } 

	if(sink_changed)	
	{		
		state = gpio_get_value(s_data.pin);
		s_data.state = state;
	
         	//for auto detecting rx sense.	
		//if(state == HDMI_5V_HPD)		
		//	s_data.state |= HDMI_5V_HPD;
		//else
		//	s_data.state &= ~HDMI_5V_HPD;			
		
		hdmitx_dump_error_code();		
	}
	
	HDMI_INFO("%s:done", state?"plugged in":"pulled out");
	
	switch_set_state(sdev, state);

#ifdef CONFIG_HDCP_REPEATER
	if(!state)
		HdmiRx_hotplug_pulses();
#endif

}

static irqreturn_t hdmitx_switch_isr(int irq, void *data)
{    		
	schedule_work(&s_data.work);	
	HDMI_DEBUG("hdmitx_switch_isr");
	
	return IRQ_HANDLED;
}

int register_hdmitx_switchdev(hdmitx_device_t * device)
{
	int ret;
	static struct workqueue_struct *workq;
			
	HDMI_DEBUG("register_hdmitx_switch");	
	
	sdev = kzalloc(sizeof(struct switch_dev), GFP_KERNEL);
	if (!sdev)
		return -ENOMEM;
		
	sdev = &device->sdev;		
 
	INIT_WORK(&s_data.work, hdmitx_switch_work_func);
	
	sdev->name = HDMI_SWITCH_NAME;
	sdev->print_state = hdmitx_switch_print_state;

	ret = switch_dev_register(sdev);
	if (ret < 0) {
		HDMI_ERROR("err_register_switch");
		goto err_register_switch;
	}
		
	s_data.pin  = device-> num_gpio;
	s_data.state = gpio_get_value(device->num_gpio);
	gpio_direction_input(device->num_gpio); 
	
	s_data.irq = device-> irq_num;
		
	irq_set_irq_type(s_data.irq, IRQ_TYPE_EDGE_BOTH);
	if(request_irq(s_data.irq, hdmitx_switch_isr,IRQF_SHARED,"switch_hdmitx",&device->dev)) {
		HDMI_ERROR("cannot register IRQ %d", s_data.irq);
	}
	
	gpio_set_debounce(s_data.pin,30*1000); //30ms

	/*ret = device_create_file(sdev->dev, &dev_attr_hdmitx);

	if (ret < 0) {
		goto err_create_sysfs_file;
	}*/	
		
	workq = create_singlethread_workqueue("switch-hdmitx");
    if (!workq){
		HDMI_ERROR("switch init work failed");
		//return -ENOMEM;		
	}
        
	queue_work(workq, &s_data.work);
 
	destroy_workqueue(workq);
			
	goto end;
		
//err_create_sysfs_file:
//	switch_dev_unregister(sdev);
err_register_switch:	
	HDMI_ERROR("register_hdmitx_switch failed");
end:	
	return ret;
}
EXPORT_SYMBOL(register_hdmitx_switchdev);


void deregister_hdmitx_switchdev(hdmitx_device_t* device)
{
    return switch_dev_unregister(&device-> sdev);
}
EXPORT_SYMBOL(deregister_hdmitx_switchdev);


int show_hpd_status(bool real_time)
{		
	if(real_time)
		return gpio_get_value(s_data.pin);	
	else
		return s_data.state;
}
EXPORT_SYMBOL(show_hpd_status);


int rtk_hdmitx_switch_suspend(void)
{	
	int state=0;

    if(!sdev)
        return 0;

	HDMI_INFO("Enter %s",__FUNCTION__);

	hdmitx_device_t *pdev = container_of(sdev, hdmitx_device_t,sdev);
	
	s_data.state = state;		
	hdmitx_reset_sink_capability((asoc_hdmi_t*)hdmitx_get_drvdata(pdev));	
	switch_set_state(sdev, state);	

	free_irq(s_data.irq, &pdev->dev);
	HDMI_DEBUG("%s free irq=%x ",__FUNCTION__,s_data.irq);

	HDMI_INFO("Exit %s",__FUNCTION__);

	return 0;
}
EXPORT_SYMBOL(rtk_hdmitx_switch_suspend);

int rtk_hdmitx_switch_resume(void)
{	
    if(!sdev)
        return 0;

	HDMI_INFO("Enter %s",__FUNCTION__);

	hdmitx_device_t *pdev = container_of(sdev, hdmitx_device_t,sdev);

	gpio_set_debounce(s_data.pin,30*1000); //30ms

	irq_set_irq_type(s_data.irq, IRQ_TYPE_EDGE_BOTH);
	
	if(request_irq(s_data.irq, hdmitx_switch_isr,IRQF_SHARED,"switch_hdmitx",&pdev->dev)) {
		HDMI_ERROR("cannot register IRQ %d", s_data.irq);
	}

	schedule_work(&s_data.work);

	HDMI_INFO("Exit %s",__FUNCTION__);

	return 0;
}
EXPORT_SYMBOL(rtk_hdmitx_switch_resume);

#if 0  //for auto detecting rx sense.
static int rxsense_thread(void *arg)
{	
	unsigned int timeout;
	void __iomem * mhl_cbus_reg;
	mhl_cbus_reg = ioremap(MHL_CBUS_reg, 0x4);
  
    for(;;) {
        if (kthread_should_stop()) break;
        
        do {		
			
			if(field_get(rd_reg(mhl_cbus_reg),7,0) == 0xff)			
				s_data.state |= HDMI_Rx_Sense;
			else
				s_data.state &= ~HDMI_Rx_Sense;
			
			switch_set_state(sdev, s_data.state);
			
            set_current_state(TASK_INTERRUPTIBLE);
            timeout = schedule_timeout(1* HZ);		
        } while(timeout);
    }
 
    return 0;	
}

int rtk_hdmitx_init_polling_rx_sense(void)
{	
	int err;  
	
	rxsense_task = kzalloc(sizeof(struct task_struct), GFP_KERNEL);
	if (!rxsense_task)
		return -ENOMEM;
		  
    rxsense_task = kthread_create(rxsense_thread, NULL, "rxsense_task");  
  
    if(IS_ERR(rxsense_task))
	{  
		HDMI_ERROR("Unable to start rxsense thread.");  
		err = PTR_ERR(rxsense_task);  
		rxsense_task = NULL;  
		return err;    
    }  
  
    wake_up_process(rxsense_task);  
  
    return 0;  

}
#endif
