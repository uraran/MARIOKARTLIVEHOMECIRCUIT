//#include <linux/config.h>
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
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/device.h>
//for linux 2.6.34
#include <linux/platform_device.h>
#include <linux/freezer.h>
#include <linux/device.h>


//#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <asm/io.h>
#include <linux/sched.h>
//#include <linux/devfs_fs_kernel.h> /* for devfs */

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/interrupt.h>

#include <asm/bitops.h>		/* bit operations */
#include <asm/delay.h>
#include <platform.h>

#include "rtk_vfd.h"

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>



#define VIC_DEBUG
#ifdef VIC_DEBUG
#define RTK_debug  printk 
#endif


//for linux 2.6.34
#define __kfifo_put kfifo_in
#define __kfifo_get kfifo_out
#define __kfifo_len kfifo_len
#define PF_FREEZE PF_FREEZING

/* Module Variables */
/*
 * Our parameters which can be set at load time.
 */
#define ioread32 readl
#define iowrite32 writel

#define FIFO_DEPTH	256		/* driver queue depth */
//#define DEV_DEBUG

static DECLARE_WAIT_QUEUE_HEAD(venus_vfd_keypad_read_wait);
static DECLARE_WAIT_QUEUE_HEAD(venus_vfd_write_done_wait);
//TODO  vfd_keypad_fifo
static struct kfifo vfd_keypad_fifo;

/* Major Number + Minor Number */
static dev_t dev_venus_vfd = 0;

//TODO
static struct cdev *venus_vfd_vfdo_cdev 	= NULL;
static struct cdev *venus_vfd_wrctl_cdev 	= NULL;
static struct cdev *venus_vfd_keypad_cdev 	= NULL;

static volatile unsigned int *reg_umsk_isr;
static volatile unsigned int *reg_isr;
static volatile unsigned int *reg_umsk_isr_kpadah;
static volatile unsigned int *reg_umsk_isr_kpadal;
static volatile unsigned int *reg_umsk_isr_kpaddah;
static volatile unsigned int *reg_umsk_isr_kpaddal;
static volatile unsigned int *reg_umsk_isr_sw;
static volatile unsigned int *reg_vfd_ctl;
static volatile unsigned int *reg_vfd_wrctl;
static volatile unsigned int *reg_vfdo;
static volatile unsigned int *reg_vfd_ardctl;
static volatile unsigned int *reg_vfd_kpadlie;
static volatile unsigned int *reg_vfd_kpadhie;
static volatile unsigned int *reg_vfd_swie;
static volatile unsigned int *reg_vfd_ardkpadl;
static volatile unsigned int *reg_vfd_ardkpadh;
static volatile unsigned int *reg_vfd_ardsw;

//spinlock_t venus_vfd_lock 	= SPIN_LOCK_UNLOCKED;
//TODO ????
static struct class *vfd_vfdo_class;
static struct class *vfd_wrctl_class;
static struct class *vfd_keypad_class;

MODULE_AUTHOR("Chih-pin Wu, Realtek Semiconductor");
MODULE_LICENSE("GPL");

struct venus_vfd_dev *venus_vfd_devices;	/* allocated in scull_init_module */

/* Module Functions */
/* *** ALL INITIALIZATION HERE *** */

// cyhuang (2011/11/19) +++	
//                          Add for new device PM driver.
#ifdef CONFIG_PM
static int venus_vfd_pm_suspend_noirq(struct device *dev) {
//	uint32_t regValue;

	/* [KEYPAD] disable whole key matrix = key1-key4, seg1-seg12 interrupt */
	iowrite32(0x00000000, reg_vfd_kpadlie);
	iowrite32(0x00000000, reg_vfd_kpadhie);

	/* [KEYPAD] Clear reg_umsk_isr_kpadah/reg_umsk_isr_kpadal */
	iowrite32(0xfffffff0, reg_umsk_isr_kpadah);
	iowrite32(0xfffffff0, reg_umsk_isr_kpadal);

	return 0;
}

static int venus_vfd_pm_resume_noirq(struct device *dev) {
	/* [KEYPAD] enable whole key matrix = key1-key4, seg1-seg12 interrupt */
	iowrite32(0xffffffff, reg_vfd_kpadlie);
	iowrite32(0x0000ffff, reg_vfd_kpadhie);

	return 0;
}

static const struct dev_pm_ops venus_vfd_pm_ops = {
    .suspend_noirq    = venus_vfd_pm_suspend_noirq,
    .resume_noirq     = venus_vfd_pm_resume_noirq,
#ifdef CONFIG_HIBERNATION
    .freeze_noirq     = venus_vfd_pm_suspend_noirq,
    .thaw_noirq       = venus_vfd_pm_resume_noirq,
    .poweroff_noirq   = venus_vfd_pm_suspend_noirq,
    .restore_noirq    = venus_vfd_pm_resume_noirq,
#endif
};

#endif // CONFIG_PM
// cyhuang (2011/11/19) +++	 

static struct platform_device *venus_vfd_devs;

static struct of_device_id rtk119x_vfd_ids[] = {
	{ .compatible = "Realtek,rtk119x-vfd" },
	{ /* Sentinel */ },
};

static struct platform_driver venus_vfd_platform_driver = {
	.driver = {
		.name       = "VenusVFD",
		.bus        = &platform_bus_type,
// cyhuang (2011/11/19) +++	        
#ifdef CONFIG_PM
	        .pm         = &venus_vfd_pm_ops,
#endif
// cyhuang (2011/11/19) +++	 
		.of_match_table = rtk119x_vfd_ids,
	},
};

//typedef irqreturn_t (*irq_handler_t)(int, void *);
//static irqreturn_t IR_interrupt_handler(int irq, void *dev_id) {
//static irqreturn_t VFD_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs) {
static irqreturn_t VFD_interrupt_handler(int irq, void *dev_id) {	
	int kfifoAdded = 0;
	int firstBitNr;
	char keyNr;
	uint32_t regValue;
	uint32_t statusRegValue;

	regValue = ioread32(reg_isr);

	/* check if the interrupt belongs to us */
	if(regValue & 0x0007c000) {
		if(regValue & 0x00040000) {	/* SW dis-assert [DONT CARE] */
			iowrite32(0x00040000, reg_isr); /* clear interrupt flag */
		}
		
		if(regValue & 0x00020000) { /* SW assert */
			statusRegValue = ioread32(reg_umsk_isr_sw) & 0xf0;
			while((firstBitNr = ffs(statusRegValue)) != 0) {
#ifdef DEV_DEBUG
			printk(KERN_WARNING "Venus VFD: SwitchNr[%d].\n", firstBitNr-1-4);
#endif
				kfifoAdded++;
				keyNr = (char)firstBitNr - 1 - 4 + VENUS_VFD_SWITCH_BASE;
				__kfifo_put(&vfd_keypad_fifo, &keyNr, sizeof(char));
				clear_bit(firstBitNr-1, (unsigned long *)&statusRegValue);
			}
			iowrite32(0x00000ff0, reg_umsk_isr_sw);
			iowrite32(0x00020000, reg_isr); /* clear interrupt flag */
		}
		
		if(regValue & 0x00010000) { /* Keypad dis-assert [DONT CARE] */
			iowrite32(0x00fffff0, reg_umsk_isr_kpaddah);
			iowrite32(0xfffffff0, reg_umsk_isr_kpaddal);
			iowrite32(0x00010000, reg_isr); /* clear interrupt flag */
		}
		
		if(regValue & 0x00008000) { /* Keypad assert */
			/* scan for lower 28 bits */
			statusRegValue = ioread32(reg_umsk_isr_kpadal) & 0xfffffff0;
			while((firstBitNr = ffs(statusRegValue)) != 0) {
#ifdef DEV_DEBUG
printk(KERN_WARNING "Venus VFD: KeyNr[%d].\n", firstBitNr-1-4);
#endif
				kfifoAdded++;
				keyNr = (char)firstBitNr - 1 - 4;
				__kfifo_put(&vfd_keypad_fifo, &keyNr, sizeof(char));
				clear_bit(firstBitNr-1, (unsigned long *)&statusRegValue);
			}
			iowrite32(0xfffffff0, reg_umsk_isr_kpadal);

			/* scan for higher 20 bits */
			statusRegValue = ioread32(reg_umsk_isr_kpadah) & 0x00fffff0;
			while((firstBitNr = ffs(statusRegValue)) != 0) {
#ifdef DEV_DEBUG
printk(KERN_WARNING "Venus VFD: KeyNr[%d].\n", firstBitNr-1-4+28);
#endif
				kfifoAdded++;
				keyNr = (char)firstBitNr - 1 - 4 + 28;
				__kfifo_put(&vfd_keypad_fifo, &keyNr, sizeof(char));
				clear_bit(firstBitNr-1, (unsigned long *)&statusRegValue);
			}
			iowrite32(0x00fffff0, reg_umsk_isr_kpadah);
			iowrite32(0x00008000, reg_isr); /* clear interrupt flag */
		}
		
		if(regValue & 0x00004000) { /* write_done [DONT CARE] */
			iowrite32(0x00004000, reg_isr); /* clear interrupt flag */
		}

		if(kfifoAdded != 0)
			wake_up_interruptible(&venus_vfd_keypad_read_wait);

		return IRQ_HANDLED;
	}
	else {
		return IRQ_NONE;
	}
}

static void Venus_VFD_Init(void) {

	RTK_debug("[%s] %s %d \n",__FILE__,__FUNCTION__,__LINE__);
/* Initialize Venus */
	/* VFD Auto Read Control Register
	   6 bytes Keypad, Switch HIGH active, Keypad HIGH Active, 10ms-period Auto Read */
	iowrite32(0x00000630, reg_vfd_ardctl);

	/* [KEYPAD] enable whole key matrix = key1-key4, seg1-seg12 interrupt */
	iowrite32(0xffffffff, reg_vfd_kpadlie);
	iowrite32(0x0000ffff, reg_vfd_kpadhie);

	/* [SWITCH] by default disable switch0-switch3 interrupt */
	iowrite32(0x00000000, reg_vfd_swie);

	/* Enable VFD Controller (enable kpad, enable switch, 1.037us Duty Cycle, Enable VFD) */
	iowrite32(0x0000000d, reg_vfd_ctl);

	/* [KEYPAD] Clear Interrupt Flags */
	iowrite32(0x00fffff0, reg_umsk_isr_kpaddah);
	iowrite32(0xfffffff0, reg_umsk_isr_kpaddal);
	iowrite32(0x00fffff0, reg_umsk_isr_kpadah);
	iowrite32(0xfffffff0, reg_umsk_isr_kpadal);
	return;
}

ssize_t venus_vfd_vfdo_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
	unsigned char *kBuf;

	if(count != 4) 	/* we only support 32bit access to match register width */
		return -EFAULT;

	kBuf = kmalloc(count, GFP_KERNEL);
	if(!kBuf) 
		return -ENOMEM;

	if(copy_from_user(kBuf, buf, count)) {
		kfree(kBuf);
		return -EFAULT;
	}

	iowrite32(*(uint32_t *)kBuf, reg_vfdo);
#ifdef DEV_DEBUG
	printk(KERN_WARNING "Venus VFD: writing %08X to reg_vfdo(%08X)\n", *(uint32_t *)kBuf, reg_vfdo);
#endif
	kfree(kBuf);
	return count;
}

ssize_t venus_vfd_wrctl_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	uint32_t regValue;

	if(count != 4)
		return -EFAULT;

	regValue = ioread32(reg_vfd_wrctl);
#ifdef DEV_DEBUG
        printk(KERN_WARNING "Venus VFD: reading %08X from reg_vfd_wrctl(%08X)\n", regValue, reg_vfd_wrctl);
#endif

	if(copy_to_user(buf, &regValue, count))
		return -EFAULT;

	return count;
}

ssize_t venus_vfd_wrctl_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
	unsigned char *kBuf;

	if(count != 4) 	/* we only support 32bit access to match register width */
		return -EFAULT;

	kBuf = kmalloc(count, GFP_KERNEL);
	if(!kBuf) 
		return -ENOMEM;

	if(copy_from_user(kBuf, buf, count)) {
		kfree(kBuf);
		return -EFAULT;
	}

	iowrite32(*(uint32_t *)kBuf, reg_vfd_wrctl);
#ifdef DEV_DEBUG
	printk(KERN_WARNING "Venus VFD: writing %08X to reg_vfd_wrctl(%08X)\n", *(uint32_t *)kBuf, reg_vfd_wrctl);
#endif

	kfree(kBuf);
	return count;
}

unsigned int venus_vfd_wrctl_poll(struct file *filp, poll_table *wait) {
    return POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM;
}


//int venus_vfd_wrctl_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
long venus_vfd_wrctl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	int err = 0;
	int retval = 0;

	if (_IOC_TYPE(cmd) != VENUS_VFD_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > VENUS_VFD_IOC_MAXNR) return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) 
		return -EFAULT;

	if (!capable (CAP_SYS_ADMIN))
		return -EPERM;

	switch(cmd) {
		case VENUS_VFD_IOC_DISABLE_AUTOREAD:
			iowrite32((ioread32(reg_vfd_ctl) & 0xfffffff3), reg_vfd_ctl);
			break;
		case VENUS_VFD_IOC_ENABLE_AUTOREAD:
			iowrite32((ioread32(reg_vfd_ctl) | 0x0000000c), reg_vfd_ctl);
			break;
		default:
			retval = -ENOIOCTLCMD;
	}

	return retval;
}


int venus_vfd_keypad_open(struct inode *inode, struct file *filp) {
	/* clear interrupt flag */
	iowrite32(0x00fffff0, reg_umsk_isr_kpaddah);
	iowrite32(0xfffffff0, reg_umsk_isr_kpaddal);
	iowrite32(0x00fffff0, reg_umsk_isr_kpadah);
	iowrite32(0xfffffff0, reg_umsk_isr_kpadal);

	/* reinitialize kfifo */
	kfifo_reset(&vfd_keypad_fifo);

	return 0;	/* success */
}

ssize_t venus_vfd_keypad_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	char byteBuf;
	int i, readCount = count;
	unsigned int cnt;

restart:
	if(wait_event_interruptible(venus_vfd_keypad_read_wait, __kfifo_len(&vfd_keypad_fifo) > 0) != 0) {
#if 1
		if (try_to_freeze())
			goto restart;
#else
		if(unlikely(current->flags & PF_FREEZE)) {
			//refrigerator(PF_FREEZE);
			refrigerator();
			goto restart;
		}
		else 
			return -ERESTARTSYS;
#endif
	}

	if(__kfifo_len(&vfd_keypad_fifo) < count)
		readCount = __kfifo_len(&vfd_keypad_fifo);

	for(i = 0 ; i < readCount ; i++) {
		cnt = __kfifo_get(&vfd_keypad_fifo, &byteBuf, 1);
		copy_to_user(buf, &byteBuf, 1);
		buf++;
	}

	return readCount;
}

unsigned int venus_vfd_keypad_poll(struct file *filp, poll_table *wait) {
	poll_wait(filp, &venus_vfd_keypad_read_wait, wait);

	if(__kfifo_len(&vfd_keypad_fifo) > 0)
		return POLLIN | POLLRDNORM;
	else
		return 0;
}


struct file_operations venus_vfd_vfdo_fops = {
	.owner =    THIS_MODULE,
	.write =    venus_vfd_vfdo_write,
};

struct file_operations venus_vfd_wrctl_fops = {
	.owner =    THIS_MODULE,
	.read  =    venus_vfd_wrctl_read,
	.write =    venus_vfd_wrctl_write,
	.poll  =    venus_vfd_wrctl_poll,
	.unlocked_ioctl =    venus_vfd_wrctl_ioctl,
};

struct file_operations venus_vfd_keypad_fops = {
	.owner =    THIS_MODULE,
	.open  =    venus_vfd_keypad_open,
	.read  =    venus_vfd_keypad_read,
	.poll  =    venus_vfd_keypad_poll,
};

//char *(*devnode)(struct device *dev, umode_t *mode);
static char *vfdo_devnode(struct device *dev, umode_t *mode)
{
	return NULL;
}

static char *wrctl_devnode(struct device *dev, umode_t *mode)
{
	return NULL;
}

static char *keypad_devnode(struct device *dev, umode_t *mode)
{
	return NULL;
}

int venus_vfd_init_module(void) {
	int result, i;
	void __iomem *vfd_base=NULL;
	void __iomem *iso_base=NULL;
	struct device_node *np;



	RTK_debug("[%s] %s %d \n",__FILE__,__FUNCTION__,__LINE__);
	np = of_find_compatible_node(NULL, NULL, "Realtek,rtk119x-vfd");
	if (np) {
		iso_base = of_iomap(np, 0);
		vfd_base = of_iomap(np, 1);
	}

	if ((!iso_base)||(!vfd_base))
		return -EFAULT;
	
	/* register address assignment by IC */
/*
	if(is_saturn_cpu())
	{
*/
		reg_umsk_isr         = (volatile unsigned int *) (iso_base + SATURN_ISO_UMSK_ISR_REGOFF);
		reg_isr              = (volatile unsigned int *) (iso_base + SATURN_ISO_ISR_REGOFF);
		reg_umsk_isr_kpadah  = (volatile unsigned int *) (iso_base + SATURN_ISO_UMSK_ISR_KPADAH_REGOFF);
		reg_umsk_isr_kpadal  = (volatile unsigned int *) (iso_base + SATURN_ISO_UMSK_ISR_KPADAL_REGOFF);
		reg_umsk_isr_kpaddah = (volatile unsigned int *) (iso_base + SATURN_ISO_UMSK_ISR_KPADDAH_REGOFF);
		reg_umsk_isr_kpaddal = (volatile unsigned int *) (iso_base + SATURN_ISO_UMSK_ISR_KPADDAL_REGOFF);
		reg_umsk_isr_sw      = (volatile unsigned int *) (iso_base + SATURN_ISO_UMSK_ISR_SW_REGOFF);

		reg_vfd_ctl          = (volatile unsigned int *) (vfd_base + SATURN_ISO_VFD_CTL_REGOFF);
		reg_vfd_wrctl        = (volatile unsigned int *) (vfd_base + SATURN_ISO_VFD_WRCTL_REGOFF);
		reg_vfdo             = (volatile unsigned int *) (vfd_base + SATURN_ISO_VFDO_REGOFF);
		reg_vfd_ardctl       = (volatile unsigned int *) (vfd_base + SATURN_ISO_VFD_ARDCTL_REGOFF);
		reg_vfd_kpadlie      = (volatile unsigned int *) (vfd_base + SATURN_ISO_VFD_KPADLIE_REGOFF);
		reg_vfd_kpadhie      = (volatile unsigned int *) (vfd_base + SATURN_ISO_VFD_KPADHIE_REGOFF);
		reg_vfd_swie         = (volatile unsigned int *) (vfd_base + SATURN_ISO_VFD_SWIE_REGOFF);
		reg_vfd_ardkpadl     = (volatile unsigned int *) (vfd_base + SATURN_ISO_VFD_ARDKPADL_REGOFF);
		reg_vfd_ardkpadh     = (volatile unsigned int *) (vfd_base + SATURN_ISO_VFD_ARDKPADH_REGOFF);
		reg_vfd_ardsw        = (volatile unsigned int *) (vfd_base + SATURN_ISO_VFD_ARDSW_REGOFF);
#if 0
	}
	else
	if(is_jupiter_cpu())
	{
		reg_umsk_isr         = ISO_UMSK_ISR;
		reg_isr              = ISO_ISR;
		reg_umsk_isr_kpadah  = ISO_UMSK_ISR_KPADAH;
		reg_umsk_isr_kpadal  = ISO_UMSK_ISR_KPADAL;
		reg_umsk_isr_kpaddah = ISO_UMSK_ISR_KPADDAH;
		reg_umsk_isr_kpaddal = ISO_UMSK_ISR_KPADDAL;
		reg_umsk_isr_sw      = ISO_UMSK_ISR_SW;
		reg_vfd_ctl          = ISO_VFD_CTL;
		reg_vfd_wrctl        = ISO_VFD_WRCTL;
		reg_vfdo             = ISO_VFDO;
		reg_vfd_ardctl       = ISO_VFD_ARDCTL;
		reg_vfd_kpadlie      = ISO_VFD_KPADLIE;
		reg_vfd_kpadhie      = ISO_VFD_KPADHIE;
		reg_vfd_swie         = ISO_VFD_SWIE;
		reg_vfd_ardkpadl     = ISO_VFD_ARDKPADL;
		reg_vfd_ardkpadh     = ISO_VFD_ARDKPADH;
		reg_vfd_ardsw        = ISO_VFD_ARDSW;
	}
	else
	{
		reg_umsk_isr         = MIS_UMSK_ISR;
		reg_isr              = MIS_ISR;
		reg_umsk_isr_kpadah  = MIS_UMSK_ISR_KPADAH;
		reg_umsk_isr_kpadal  = MIS_UMSK_ISR_KPADAL;
		reg_umsk_isr_kpaddah = MIS_UMSK_ISR_KPADDAH;
		reg_umsk_isr_kpaddal = MIS_UMSK_ISR_KPADDAL;
		reg_umsk_isr_sw      = MIS_UMSK_ISR_SW;
		reg_vfd_ctl          = MIS_VFD_CTL;
		reg_vfd_wrctl        = MIS_VFD_WRCTL;
		reg_vfdo             = MIS_VFDO;
		reg_vfd_ardctl       = MIS_VFD_ARDCTL;
		reg_vfd_kpadlie      = MIS_VFD_KPADLIE;
		reg_vfd_kpadhie      = MIS_VFD_KPADHIE;
		reg_vfd_swie         = MIS_VFD_SWIE;
		reg_vfd_ardkpadl     = MIS_VFD_ARDKPADL;
		reg_vfd_ardkpadh     = MIS_VFD_ARDKPADH;
		reg_vfd_ardsw        = MIS_VFD_ARDSW;
	}
#endif

	/* MKDEV */
	dev_venus_vfd = MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_VFDO);
	RTK_debug("[%s] %s %d \n",__FILE__,__FUNCTION__,__LINE__);

	/* Request Major Number */
	result = register_chrdev_region(dev_venus_vfd, VENUS_VFD_DEVICE_NUM, "vfdo");
	if(result < 0) {
		printk(KERN_WARNING "Venus VFD: can't register device number.\n");
		goto fail_alloc_dev_vfd;
	}

	/* Initialize kfifo */
	//vfd_keypad_fifo = kfifo_alloc(FIFO_DEPTH, GFP_KERNEL, &venus_vfd_lock);
	//if(IS_ERR(vfd_keypad_fifo)) {
	if(kfifo_alloc(&vfd_keypad_fifo, FIFO_DEPTH*sizeof(uint32_t), GFP_KERNEL)) {
		result = -ENOMEM;
		goto fail_alloc_kfifo;
	}
	RTK_debug("[%s] %s %d \n",__FILE__,__FUNCTION__,__LINE__);

	venus_vfd_devs = platform_device_register_simple("VenusVFD", -1, NULL, 0);
	//if(driver_register(&venus_vfd_driver) != 0)
	if(platform_driver_register(&venus_vfd_platform_driver) != 0)
		goto fail_device_register;
#if 0
	/* Request IRQ */
	if(request_irq(VENUS_VFD_IRQ, 
						VFD_interrupt_handler, 
//#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 4, 5))
#if 1
						IRQF_SHARED,
#else
						//SA_INTERRUPT|SA_SAMPLE_RANDOM|SA_SHIRQ, 
						IRQF_SAMPLE_RANDOM|IRQF_SHARED,
#endif
						"Venus_VFD", 
						VFD_interrupt_handler)) {
		printk(KERN_ERR "VFD: cannot register IRQ %d\n", VENUS_VFD_IRQ);
		result = -EIO;
		goto fail_alloc_irq;
	}
#endif

for(i=0; i<4; i++)
{

	int vfd_irq;

	vfd_irq = irq_of_parse_and_map(np, i);	
	if (!vfd_irq) {		
		printk(KERN_ALERT "Venus IR: fail to parse of irq.\n");		
		return -ENXIO;	
	}	

	RTK_debug("[%s] %s %d   vfd_irq = %d \n",__FILE__,__FUNCTION__,__LINE__,vfd_irq);

	/* Request IRQ */
	if(request_irq(vfd_irq, 
						VFD_interrupt_handler, 
//#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 4, 5))
#if 1
						IRQF_SHARED,
#else
						//SA_INTERRUPT|SA_SAMPLE_RANDOM|SA_SHIRQ, 
						IRQF_SAMPLE_RANDOM|IRQF_SHARED,
#endif
						"Venus_VFD", 
						VFD_interrupt_handler)) {
		printk(KERN_ERR "VFD: cannot register IRQ %d\n", VENUS_VFD_IRQ);
		result = -EIO;
		goto fail_alloc_irq;
	}

}











	RTK_debug("[%s] %s %d \n",__FILE__,__FUNCTION__,__LINE__);

	/* Char Device Registration */
	/* Expose Register MIS_VFDO write interface */
	venus_vfd_vfdo_cdev = cdev_alloc();
	venus_vfd_vfdo_cdev->ops = &venus_vfd_vfdo_fops;
	if(cdev_add(venus_vfd_vfdo_cdev, MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_VFDO), 1)) {
		printk(KERN_ERR "Venus VFD: can't add character device for vfd_vfdo\n");
		result = -ENOMEM;
		goto fail_cdev_alloc_vfdo;
	}

	RTK_debug("[%s] %s %d \n",__FILE__,__FUNCTION__,__LINE__);
	/* Expose Register MIS_VFD_WRCTL read/write interface */
	venus_vfd_wrctl_cdev = cdev_alloc();
	venus_vfd_wrctl_cdev->ops = &venus_vfd_wrctl_fops;
	if(cdev_add(venus_vfd_wrctl_cdev, MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_WRCTL), 1)) {
		printk(KERN_ERR "Venus VFD: can't add character device for vfd_wrctl\n");
		result = -ENOMEM;
		goto fail_cdev_alloc_wrctl;
	}
	RTK_debug("[%s] %s %d \n",__FILE__,__FUNCTION__,__LINE__);

	/* Keystroke interface on VFD Controller */
	venus_vfd_keypad_cdev = cdev_alloc();
	venus_vfd_keypad_cdev->ops = &venus_vfd_keypad_fops;
	if(cdev_add(venus_vfd_keypad_cdev, MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_KEYPAD), 1)) {
		printk(KERN_ERR "Venus VFD: can't add character device for vfd_wrctl\n");
		result = -ENOMEM;
		goto fail_cdev_alloc_keypad;
	}

	/* use devfs to create device file */
	//devfs_mk_cdev(MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_VFDO), S_IFCHR|S_IRUSR|S_IWUSR, VENUS_VFD_VFDO_DEVICE);
	//devfs_mk_cdev(MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_WRCTL), S_IFCHR|S_IRUSR|S_IWUSR, VENUS_VFD_WRCTL);
	//devfs_mk_cdev(MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_KEYPAD), S_IFCHR|S_IRUSR|S_IWUSR, VENUS_VFD_KEYPAD);
	vfd_vfdo_class   = class_create(THIS_MODULE, "vfd_vfdo_class");;
	if( IS_ERR(vfd_vfdo_class))
	{
		printk(KERN_ERR "Venus VFD: can't class_create vfd_vfdo");
		result = PTR_ERR(vfd_vfdo_class);
		//TODO  goto:
	}
	RTK_debug("[%s] %s %d \n",__FILE__,__FUNCTION__,__LINE__);
	
	vfd_vfdo_class->devnode = vfdo_devnode;
	device_create(vfd_vfdo_class, NULL, MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_VFDO), NULL, VENUS_VFD_VFDO_DEVICE);
	vfd_wrctl_class  = class_create(THIS_MODULE, "vfd_wrctl_class");;
	if( IS_ERR(vfd_wrctl_class))
	{
		printk(KERN_ERR "Venus VFD: can't class_create vfd_wrctl");
		result = PTR_ERR(vfd_wrctl_class);
		//TODO  goto:
	}
	vfd_wrctl_class->devnode = wrctl_devnode;
	device_create(vfd_wrctl_class, NULL, MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_WRCTL), NULL, VENUS_VFD_WRCTL);
	vfd_keypad_class = class_create(THIS_MODULE, "vfd_keypad_class");;
	if( IS_ERR(vfd_keypad_class))
	{
		printk(KERN_ERR "Venus VFD: can't class_create vfd_keypad");
		result = PTR_ERR(vfd_keypad_class);
		//TODO  goto:
	}
	RTK_debug("[%s] %s %d \n",__FILE__,__FUNCTION__,__LINE__);
	
	vfd_keypad_class->devnode = keypad_devnode;
	device_create(vfd_keypad_class, NULL, MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_KEYPAD), NULL, VENUS_VFD_KEYPAD);

	/* Hardware Registers Initialization */
	Venus_VFD_Init();
	RTK_debug("[%s] %s %d \n",__FILE__,__FUNCTION__,__LINE__);

	
	return 0;	/* succeed ! */

fail_cdev_alloc_keypad:
	cdev_del(venus_vfd_wrctl_cdev);
fail_cdev_alloc_wrctl:
	cdev_del(venus_vfd_vfdo_cdev);
fail_cdev_alloc_vfdo:
	free_irq(VENUS_VFD_IRQ, VFD_interrupt_handler);
fail_alloc_irq:
	kfifo_free(&vfd_keypad_fifo);
	platform_driver_unregister(&venus_vfd_platform_driver); 
fail_device_register:
	platform_device_unregister(venus_vfd_devs);
	//driver_unregister(&venus_vfd_driver);
fail_alloc_kfifo:
	unregister_chrdev_region(dev_venus_vfd, VENUS_VFD_DEVICE_NUM);
fail_alloc_dev_vfd:
	return result;
}

void venus_vfd_cleanup_module(void) {
	/* Reset Hardware Registers */
	/* [KEYPAD] enable key1, seg1-seg8 interrupt, and disable other keys */
	iowrite32(0x0, reg_vfd_kpadlie);
	iowrite32(0x0, reg_vfd_kpadhie);

	/* Venus: Disable VFD Controller */
	iowrite32(0x0, reg_vfd_ctl);

	/* delete device file by devfs */
	//devfs_remove(VENUS_VFD_VFDO_DEVICE);
	//devfs_remove(VENUS_VFD_WRCTL);
	//devfs_remove(VENUS_VFD_KEYPAD);
	device_destroy(vfd_vfdo_class, MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_VFDO));
	class_destroy(vfd_vfdo_class);
	device_destroy(vfd_wrctl_class, MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_WRCTL));
	class_destroy(vfd_wrctl_class);
	device_destroy(vfd_keypad_class, MKDEV(VENUS_VFD_MAJOR, VENUS_VFD_MINOR_KEYPAD));
	class_destroy(vfd_keypad_class);
	
	/* Release Character Device Driver */
	cdev_del(venus_vfd_keypad_cdev);
	cdev_del(venus_vfd_wrctl_cdev);
	cdev_del(venus_vfd_vfdo_cdev);

	/* Free IRQ Handler */
	free_irq(VENUS_VFD_IRQ, VFD_interrupt_handler);

	/* Free Kernel FIFO */
	kfifo_free(&vfd_keypad_fifo);

	/* device driver removal */
	platform_device_unregister(venus_vfd_devs);
	//driver_unregister(&venus_vfd_driver);
	platform_driver_unregister(&venus_vfd_platform_driver); 

	/* Return Major Numbers */
	unregister_chrdev_region(dev_venus_vfd, VENUS_VFD_DEVICE_NUM);
}

/* Register Macros */

module_init(venus_vfd_init_module);
module_exit(venus_vfd_cleanup_module);
