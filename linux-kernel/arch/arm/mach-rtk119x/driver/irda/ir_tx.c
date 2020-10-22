#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/freezer.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include "venus_ir.h"
#include "ir_tx.h"

//for linux 2.6.34
#define __kfifo_put kfifo_in
#define __kfifo_get kfifo_out
#define __kfifo_len kfifo_len
//#define PF_FREEZE PF_FREEZING
#define FIFO_DEPTH	16		/* driver queue depth */
//static DECLARE_WAIT_QUEUE_HEAD(venus_ir_read_wait);
static struct kfifo venus_irtx_fifo;
extern int rpmode;
extern int rpmode_fifosize;
extern unsigned char *rpmode_txfifo;
static dev_t dev_venus_irtx = 0;
static struct cdev *venus_irtx_cdev 	= NULL;
//spinlock_t venus_ir_lock 	= SPIN_LOCK_UNLOCKED;
static struct class *irtx_class;

void	Venus_irtx_fifoReset(void)
{
	kfifo_reset(&venus_irtx_fifo);
}

void	Venus_irtx_fifoGet(u32* p_received, u32 size)
{	
	unsigned int cnt;
	RTK_debug(KERN_WARNING "Venus_irtx_fifoGet: size=%d.\n", size);
	cnt = __kfifo_get(&venus_irtx_fifo, p_received, size);
}

void	Venus_irtx_fifoPut( u32 * p_regValue, u32 size)
{
	RTK_debug(KERN_WARNING "Venus_irtx_fifoPut: size=%d.\n", size);
	__kfifo_put(&venus_irtx_fifo, p_regValue, size);
}

unsigned int Venus_irtx_fifoAvail()
{
	return kfifo_avail(&venus_irtx_fifo);
}

unsigned int Venus_irtx_fifoGetLen(void)
{
	return __kfifo_len(&venus_irtx_fifo);
}

static struct platform_device *venus_ir_tx_devs;

/*
// cyhuang (2011/11/19) +++
//                          Add for new device PM driver.
#ifdef CONFIG_PM
static int venus_ir_rp_pm_suspend(struct device *dev) {
	return venus_ir_pm_suspend(dev); 
}

static int venus_ir_rp_pm_resume(struct device *dev) {
	return venus_ir_pm_resume(dev);
}

static const struct dev_pm_ops venus_ir_rp_pm_ops = {
    .suspend    = venus_ir_rp_pm_suspend,
    .resume     = venus_ir_rp_pm_resume,
#ifdef CONFIG_HIBERNATION
    .freeze     = venus_ir_rp_pm_suspend,
    .thaw       = venus_ir_rp_pm_resume,
    .poweroff   = venus_ir_rp_pm_suspend,
    .restore    = venus_ir_rp_pm_resume,
#endif
};
#endif
// cyhuang (2011/11/19) +++
*/
static struct platform_driver venus_ir_tx_platform_driver = {
    .driver = {
	.name       = "VenusIRTX",
	.bus        = &platform_bus_type,
/*
// cyhuang (2011/11/19) +++
#ifdef CONFIG_PM
	.pm         = &venus_ir_tx_pm_ops,
#endif
// cyhuang (2011/11/19) +++
*/
    },
} ;

int venus_ir_tx_open(struct inode *inode, struct file *filp) {
	/* reinitialize kfifo */
	RTK_debug(KERN_WARNING "venus_ir_tx_open -- begin\n ");
	kfifo_reset(&venus_irtx_fifo);

	return 0;	/* success */
}
ssize_t venus_ir_tx_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
	char keycode[256]={0};
	u32 inputKeyCode=0;
	u32 w_cnt=0;	
	int write_Count = count;
	int i;

	if(rpmode) {
		write_Count = Venus_irtx_fifoAvail();
		if(count < write_Count)
			write_Count = count;

		if(copy_from_user(rpmode_txfifo, buf, write_Count)) {
			RTK_debug(KERN_ALERT "venus_ir_write : copy from user error!\n");
			return -EFAULT;
		}

		venus_irtx_raw_send(write_Count);
		
		return write_Count;
	} else {
	RTK_debug(KERN_WARNING "venus_ir_tx_write : count = %d, buf=%s \n ",count, buf);
	if(count >= 256 || count < 1)
	{
		RTK_debug(KERN_ERR "venus_ir_tx_write : Error! count = %d \n ",count);		
		return -EFAULT;
	}
	w_cnt=(count-1);
	if (copy_from_user(keycode, buf,w_cnt*sizeof(char))){
		RTK_debug(KERN_ERR "venus_ir_tx_write : copy from user error! %d ");		
		return -EFAULT;
	}
 
	venus_ir_disable_irrp();
	sscanf(keycode,"%d", &inputKeyCode);
	RTK_debug(KERN_DEBUG "venus_ir_tx_write inputKeyCode=%d \n ",inputKeyCode);		
	
	if( venus_irtx_send(inputKeyCode)!= 0 ) {
		RTK_debug(KERN_ERR "venus_ir_tx_write venus_irtx_send fail! \n ");
		venus_ir_enable_irrp();
		return -EFAULT;
	}

	RTK_debug(KERN_DEBUG "venus_ir_tx_write : finish w_cnt = %d, ret=%d \n ",w_cnt, count);		
	}

	return count;
}

long venus_ir_tx_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	int err = 0;
	RTK_debug(KERN_ALERT "venus_ir_tx_ioctl -- begin : cmd = %d \n ", cmd);		

	if (_IOC_TYPE(cmd) != VENUS_IRTX_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > VENUS_IRTX_IOC_MAXNR) return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err)
		return -EFAULT;

	if (!capable (CAP_SYS_ADMIN))
		return -EPERM;

	return venus_ir_ioctl(filp, cmd, arg);

}

struct file_operations venus_ir_tx_fops = {
	.owner		=    THIS_MODULE,
	.open		=    venus_ir_tx_open,
	.write		=    venus_ir_tx_write,
	.unlocked_ioctl	=    venus_ir_tx_ioctl,
};

static char *irtx_devnode(struct device *dev, umode_t *mode)
{
	return NULL;
}

int venus_ir_tx_init(void) {
	int result = -1;
	int size;
	/* MKDEV */
	dev_venus_irtx = MKDEV(VENUS_IRTX_MAJOR, VENUS_IRTX_MINOR_RP);

	/* Request Device Number */
	result = register_chrdev_region(dev_venus_irtx, VENUS_IRTX_DEVICE_NUM, "irtx");
	if(result < 0) {
		RTK_debug(KERN_ERR "Venus IRTX: can't register device number.\n");
		goto fail_alloc_dev;
	}
	if(rpmode)
		size = rpmode_fifosize;
	else
		size = FIFO_DEPTH*sizeof(uint32_t);
	/* Initialize kfifo */
	if(kfifo_alloc(&venus_irtx_fifo,size, GFP_KERNEL)) {
		result = -ENOMEM;
		goto fail_alloc_kfifo;
	}

	venus_ir_tx_devs = platform_device_register_simple("VenusIRTX", -1, NULL, 0);
	if(platform_driver_register(&venus_ir_tx_platform_driver) != 0)
		goto fail_device_register;

	/* Char Device Registration */
	/* Expose Register MIS_VFDO write interface */
	venus_irtx_cdev = cdev_alloc();
	if(!venus_irtx_cdev){
		RTK_debug(KERN_ERR "Venus IR: can't allocate character device for irtx\n");
		result = -ENOMEM;
		goto fail_cdev_alloc;
	}
	venus_irtx_cdev->ops = &venus_ir_tx_fops;
	if(cdev_add(venus_irtx_cdev, MKDEV(VENUS_IRTX_MAJOR, VENUS_IRTX_MINOR_RP), 1)) {
		RTK_debug(KERN_ERR "Venus IRTX: can't add character device for irtx\n");
		result = -ENOMEM;
		goto fail_cdev_alloc;
	}
	/* use devfs to create device file! */

	irtx_class = class_create(THIS_MODULE, "irtx");
	if (IS_ERR(irtx_class))
	{
		RTK_debug(KERN_ERR "Venus IRTX: can't class_create irtx\n");
		result = PTR_ERR(irtx_class);
		goto fail_class_create;
	}
	
	irtx_class->devnode = irtx_devnode;
	device_create(irtx_class, NULL, MKDEV(VENUS_IRTX_MAJOR, VENUS_IRTX_MINOR_RP), NULL, "venus_irtx");

	return 0;	/* succeed ! */


fail_class_create:
	cdev_del(venus_irtx_cdev);
fail_cdev_alloc:
	platform_driver_unregister(&venus_ir_tx_platform_driver);
fail_device_register:
	kfifo_free(&venus_irtx_fifo);
	platform_device_unregister(venus_ir_tx_devs);

fail_alloc_kfifo:
	unregister_chrdev_region(dev_venus_irtx, VENUS_IRTX_DEVICE_NUM);
fail_alloc_dev:
	return result;
}

void venus_ir_tx_cleanup(void) {
	RTK_debug(KERN_ALERT "venus_ir_tx_cleanup: begin\n");

	/* remove device file by devfs */
    device_destroy(irtx_class, MKDEV(VENUS_IRTX_MAJOR,VENUS_IRTX_MINOR_RP));
    class_destroy(irtx_class);


	/* Release Character Device Driver */
	cdev_del(venus_irtx_cdev);

	/* Free Kernel FIFO */
	kfifo_free(&venus_irtx_fifo);

	//driver_unregister(&venus_ir_driver);
	platform_driver_unregister(&venus_ir_tx_platform_driver);

	/* device driver removal */
	platform_device_unregister(venus_ir_tx_devs);

	/* Return Device Numbers */
	unregister_chrdev_region(dev_venus_irtx, VENUS_IRTX_DEVICE_NUM);
}

