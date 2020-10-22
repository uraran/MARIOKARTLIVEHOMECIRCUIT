#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/freezer.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include "venus_ir.h"
#include "ir_rp.h"

//for linux 2.6.34
#define __kfifo_put kfifo_in
#define __kfifo_get kfifo_out
#define __kfifo_len kfifo_len
#define PF_FREEZE PF_FREEZING
#define FIFO_DEPTH	16		/* driver queue depth */
static DECLARE_WAIT_QUEUE_HEAD(venus_ir_read_wait);
static struct kfifo venus_ir_fifo;
static dev_t dev_venus_ir = 0;
static struct cdev *venus_ir_cdev 	= NULL;
//spinlock_t venus_ir_lock 	= SPIN_LOCK_UNLOCKED;
static struct class *irrp_class;
extern int rpmode;
extern int rpmode_fifosize;
extern unsigned char *rpmode_rxfifo;

void	Venus_IR_rp_fifoReset(void)
{
	kfifo_reset(&venus_ir_fifo);
}

void	Venus_IR_rp_fifoGet(int* p_received, unsigned int size)
{	
	unsigned int cnt;
	cnt = __kfifo_get(&venus_ir_fifo, p_received, size);
}

void	Venus_IR_rp_fifoPut( unsigned char * p_regValue, unsigned int size)
{
	__kfifo_put(&venus_ir_fifo, p_regValue, size);
}

unsigned int Venus_IR_rp_fifoGetLen(void)
{
	return __kfifo_len(&venus_ir_fifo);
}

void Venus_IR_rp_WakeupQueue(void)
{
	wake_up_interruptible(&venus_ir_read_wait);
}


static struct platform_device *venus_ir_rp_devs;

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

static struct platform_driver venus_ir_rp_platform_driver = {
    .driver = {
	.name       = "VenusIR",
	.bus        = &platform_bus_type,
// cyhuang (2011/11/19) +++
#ifdef CONFIG_PM
	.pm         = &venus_ir_rp_pm_ops,
#endif
// cyhuang (2011/11/19) +++
    },
} ;

int venus_ir_rp_open(struct inode *inode, struct file *filp) {
	/* reinitialize kfifo */
	kfifo_reset(&venus_ir_fifo);

	return 0;	/* success */
}

ssize_t venus_ir_rp_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	int uintBuf;
	int i, readCount = count;
	int len;

	RTK_debug("%s %s %d code, value",__FILE__,__FUNCTION__,__LINE__);

restart:
	if(wait_event_interruptible(venus_ir_read_wait, __kfifo_len(&venus_ir_fifo) > 0) != 0) {
#if 1
		if (try_to_freeze())
			goto restart;
#else
		if(unlikely(current->flags & PF_FROZEN)) {
//			refrigerator(PF_FREEZE);
			refrigerator();

			goto restart;
		}
		else
			return -ERESTARTSYS;
#endif
	}

	if(__kfifo_len(&venus_ir_fifo) < count)
		readCount = __kfifo_len(&venus_ir_fifo);

	if(rpmode) {
		len = __kfifo_get(&venus_ir_fifo, rpmode_rxfifo, readCount);
		readCount = len;
		if(copy_to_user(buf, rpmode_rxfifo, len)) {
			RTK_debug(KERN_ALERT "copy fail\n");
			return -EFAULT;
		}
	} else {
	for(i = 0 ; i < readCount ; i += len) {
		len = __kfifo_get(&venus_ir_fifo, &uintBuf, sizeof(uint32_t));
		if (copy_to_user(buf, &uintBuf, len))
			return -EFAULT;
	}
	}
	return readCount;
}

unsigned int venus_ir_rp_poll(struct file *filp, poll_table *wait) {
	poll_wait(filp, &venus_ir_read_wait, wait);

	if(__kfifo_len(&venus_ir_fifo) > 0)
		return POLLIN | POLLRDNORM;
	else
		return 0;
}

long venus_ir_rp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	int err = 0;

	if (_IOC_TYPE(cmd) != VENUS_IR_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > VENUS_IR_IOC_MAXNR) return -ENOTTY;

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

struct file_operations venus_ir_rp_fops = {
	.owner		=    THIS_MODULE,
	.open		=    venus_ir_rp_open,
	.read		=    venus_ir_rp_read,
	.poll		=    venus_ir_rp_poll,
	.unlocked_ioctl	=    venus_ir_rp_ioctl,
};

static char *irrp_devnode(struct device *dev, umode_t *mode)
{
	return NULL;
}

int venus_ir_rp_init(void) {
	int result = -1;
	int size;

	RTK_debug("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);

	/* MKDEV */
	dev_venus_ir = MKDEV(VENUS_IR_MAJOR, VENUS_IR_MINOR_RP);

	/* Request Device Number */
	result = register_chrdev_region(dev_venus_ir, VENUS_IR_DEVICE_NUM, "irrp");
	if(result < 0) {
		printk(KERN_WARNING "Venus IR: can't register device number.\n");
		goto fail_alloc_dev;
	}
	RTK_debug("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);

	/* Initialize kfifo */
//	venus_ir_fifo = kfifo_alloc(FIFO_DEPTH*sizeof(uint32_t), GFP_KERNEL, &venus_ir_lock);
//	if(IS_ERR(venus_ir_fifo)) {
	if(rpmode)
		size = rpmode_fifosize;
	else
		size = FIFO_DEPTH*sizeof(uint32_t);

	if(kfifo_alloc(&venus_ir_fifo,size, GFP_KERNEL)) {
		result = -ENOMEM;
		goto fail_alloc_kfifo;
	}

	venus_ir_rp_devs = platform_device_register_simple("VenusIR", -1, NULL, 0);
//	if(driver_register(&venus_ir_driver) != 0)
	if(platform_driver_register(&venus_ir_rp_platform_driver) != 0)
		goto fail_device_register;

	RTK_debug("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);

	/* Char Device Registration */
	/* Expose Register MIS_VFDO write interface */
	venus_ir_cdev = cdev_alloc();
	if(!venus_ir_cdev){
		printk(KERN_ERR "Venus IR: can't allocate character device for irrp\n");
		result = -ENOMEM;
		goto fail_cdev_alloc;
	}
	venus_ir_cdev->ops = &venus_ir_rp_fops;
	if(cdev_add(venus_ir_cdev, MKDEV(VENUS_IR_MAJOR, VENUS_IR_MINOR_RP), 1)) {
		printk(KERN_ERR "Venus IR: can't add character device for irrp\n");
		result = -ENOMEM;
		goto fail_cdev_alloc;
	}
	/* use devfs to create device file! */
//	devfs_mk_cdev(MKDEV(VENUS_IR_MAJOR, VENUS_IR_MINOR_RP), S_IFCHR|S_IRUSR|S_IWUSR, VENUS_IR_DEVICE_FILE);

	RTK_debug("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);

	irrp_class = class_create(THIS_MODULE, "irrp");
	if (IS_ERR(irrp_class))
	{
		printk(KERN_ERR "Venus IR: can't class_create irrp\n");
		result = PTR_ERR(irrp_class);
		goto fail_class_create;
	}
	RTK_debug("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);
	
	irrp_class->devnode = irrp_devnode;
	device_create(irrp_class, NULL, MKDEV(VENUS_IR_MAJOR, VENUS_IR_MINOR_RP), NULL, "venus_irrp");

	RTK_debug("[%s] %s  %d \n", __FILE__,__FUNCTION__,__LINE__);

	return 0;	/* succeed ! */


fail_class_create:
	cdev_del(venus_ir_cdev);
fail_cdev_alloc:
	platform_driver_unregister(&venus_ir_rp_platform_driver);
fail_device_register:
	kfifo_free(&venus_ir_fifo);
	//platform_driver_unregister(&venus_ir_rp_platform_driver);
	platform_device_unregister(venus_ir_rp_devs);
//	driver_unregister(&venus_ir_driver);
fail_alloc_kfifo:
	unregister_chrdev_region(dev_venus_ir, VENUS_IR_DEVICE_NUM);
fail_alloc_dev:
	return result;
}

void venus_ir_rp_cleanup(void) {

	/* remove device file by devfs */
//	devfs_remove(VENUS_IR_DEVICE_FILE);
    device_destroy(irrp_class, MKDEV(VENUS_IR_MAJOR,VENUS_IR_MINOR_RP));
    class_destroy(irrp_class);


	/* Release Character Device Driver */
	cdev_del(venus_ir_cdev);

	/* Free Kernel FIFO */
	kfifo_free(&venus_ir_fifo);

	//driver_unregister(&venus_ir_driver);
	platform_driver_unregister(&venus_ir_rp_platform_driver);

	/* device driver removal */
	platform_device_unregister(venus_ir_rp_devs);

	/* Return Device Numbers */
	unregister_chrdev_region(dev_venus_ir, VENUS_IR_DEVICE_NUM);
}


