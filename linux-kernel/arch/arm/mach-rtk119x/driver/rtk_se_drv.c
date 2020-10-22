#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>

#include <linux/byteorder/generic.h>

#include <asm/pgtable.h>
#include <asm/uaccess.h>

#include <rbus/reg.h>
#include "rtk_se_drv.h"
#include <mach/rtk_ipc_shm.h>

#define SWAP32(x)	cpu_to_be32(x)

//#define VSYNC_SHARE_MEM_ADDR	phys_to_virt(0x00010000)

struct semaphore sem_checkfinish;

typedef enum {
	SeClearWriteData	= 0,
	SeWriteData			= BIT(0),
	SeGo				= BIT(1),
	SeEndianSwap		= BIT(2),
} SE_CTRL_REG;

void vsync_rpc_set_flag(uint32_t flag)
{
	struct RTK119X_ipc_shm __iomem *ipc = (void __iomem *)IPC_SHM_VIRT;

	dbg_info("vsync_rpc_addr:%x", (u32)&(ipc->vo_vsync_flag));
	writel(flag, &(ipc->vo_vsync_flag));//audio RPC flag
}

unsigned int vsync_rpc_get_flag(void)
{
	struct RTK119X_ipc_shm __iomem *ipc = (void __iomem *)IPC_SHM_VIRT;

	dbg_info("vsync_rpc_addr:%x", (u32)&(ipc->vo_vsync_flag));
	return readl(&(ipc->vo_vsync_flag));//audio RPC flag
}

inline void InitSeReg(void)
{
	volatile SEREG_INFO  *SeRegInfo = (volatile SEREG_INFO  *)se_device->base;

	//Stop Streaming Engine
	SeRegInfo->SeCtrl[0].Value			= (SeGo | SeEndianSwap | SeClearWriteData);
	SeRegInfo->SeCtrl[0].Value			= (SeEndianSwap | SeWriteData);
	SeRegInfo->SeCmdBase[0].Value		= (uint32_t) se_drv_data->CmdBase;
	SeRegInfo->SeCmdLimit[0].Value		= (uint32_t) se_drv_data->CmdLimit;
	SeRegInfo->SeCmdReadPtr[0].Value	= (uint32_t) se_drv_data->CmdBase;
	SeRegInfo->SeCmdWritePtr[0].Value	= (uint32_t) se_drv_data->CmdBase;
	SeRegInfo->SeInstCntL[0].Value		= 0;
    SeRegInfo->SeInstCntH[0].Value		= 0;

	se_drv_data->CmdWritePtr			= se_drv_data->CmdBase;
	se_drv_data->u64InstCnt				= 0;
	se_drv_data->u64IssuedInstCnt		= 0;
	//dbg_info("instcnt=%d, u64cnt=%lld, u64IssuedInstCnt=%lld", SeRegInfo->SeInstCntL[0].Value & 0xFFFF,
	//							se_drv_data->u64InstCnt, se_drv_data->u64IssuedInstCnt);
}

inline void UpdateInstCount(struct se_dev *dev)
{
	volatile SEREG_INFO  *SeRegInfo = (volatile SEREG_INFO  *)se_device->base;
	uint64_t u64InstCnt;

	dbg_info("wrptr = 0x%08x, rdptr = 0x08x", SeRegInfo->SeCmdWritePtr[0].Value);

	u64InstCnt = (uint64_t)SeRegInfo->SeInstCntL[0].Value;
	u64InstCnt += (uint64_t)((uint64_t)SeRegInfo->SeInstCntH[0].Value<<32);
	dev->u64InstCnt = u64InstCnt;
}

void WriteCmd(struct se_dev *dev, uint8_t *pbyCommandBuffer, int32_t lCommandLength)
{
	uint32_t dwDataCounter = 0;
	uint32_t wrptr;
	volatile SEREG_INFO  *SeRegInfo = (volatile SEREG_INFO  *)se_device->base;

	wrptr = dev->CmdWritePtr - dev->CmdBase;

	while(1) {
		uint32_t rdptr = (uint32_t) SeRegInfo->SeCmdReadPtr[0].Value - dev->CmdBase;
		if(rdptr <= wrptr) {
			rdptr += dev->size;
		}

		if((wrptr + lCommandLength) < rdptr) {
			break;
		}
	}


	//dbg_info("[%x-%d]\n", &dev->CmdBuf[wrptr], lCommandLength);
//printk("[%x-%d]\n", &dev->CmdBuf[wrptr], lCommandLength);
	//Start writing command words to the ring buffer.
	for(dwDataCounter = 0; dwDataCounter < (uint32_t) lCommandLength; dwDataCounter += sizeof(uint32_t)) {
		//dbg_info("(%8x-%8x)\n", (uint32_t)pWptr, *(uint32_t *)(pbyCommandBuffer + dwDataCounter));
//printk("(W:%8x - %8x)\n", (uint32_t)wrptr, (*(unsigned int *)(pbyCommandBuffer + dwDataCounter)) );

		// FIXME: uncached address conversion error : barry
		//*(uint32_t *)((dev->CmdBase + wrptr) | 0xA0000000) = *(uint32_t *)(pbyCommandBuffer + dwDataCounter);
		*(uint32_t *)((dev->CmdBase + wrptr) ) = *(uint32_t *)(pbyCommandBuffer + dwDataCounter);
		wrptr += sizeof(uint32_t);
		if(wrptr >= dev->size) {
			wrptr = 0;
		}
	}

	// sync the write buffer
	//__asm__ __volatile__ (".set push");
	//__asm__ __volatile__ (".set mips32");
	//__asm__ __volatile__ ("sync;");
	//__asm__ __volatile__ (".set pop");

	//convert to physical address
	dev->CmdWritePtr = (uint32_t)wrptr + dev->CmdBase;
	SeRegInfo->SeCmdWritePtr[0].Value = (uint32_t) dev->CmdWritePtr;
	SeRegInfo->SeCtrl[0].Value = (SeGo | SeEndianSwap | SeWriteData);
}

static irqreturn_t se_irq_handler(int irq, void* dev_id)
{
	struct se_dev *dev = dev_id;
	volatile SEREG_INFO *SeRegInfo = (volatile SEREG_INFO*)se_device->base;

	if(SeRegInfo->SeInte[0].Fields.com_empty && SeRegInfo->SeInts[0].Fields.com_empty) {
		dbg_info("se com empty interrupt");
		//SEREG_INTS ints;
		//SEREG_INTE inte;

		//inte.Value = 0;
		//inte.Fields.write_data = 0;
		//inte.Fields.com_empty = 1;
		SeRegInfo->SeInte[0].Value = 0x8;//inte.Value;  //disable com_empty interrupt

		//ints.Value = 0;
		//ints.Fields.write_data = 0;
		//ints.Fields.com_empty = 1;
		SeRegInfo->SeInts[0].Value = 0x8;//ints.Value;  //clear com_empty interrupt status

		//while(SeRegInfo->SeCtrl[0].Fields.go == 1 && SeRegInfo->SeIdle[0].Fields.idle == 0)
		if((SeRegInfo->SeCtrl[0].Value & 0x2) && ((SeRegInfo->SeIdle[0].Value & 0x1) == 0)) {
			dbg_info("se is not idle");
		}
		up(&sem_checkfinish);
		return IRQ_HANDLED;
	}

//	if((*(volatile uint32_t *)VSYNC_SHARE_MEM_ADDR) & SWAP32(0x2)) {
	if(vsync_rpc_get_flag() & SWAP32(0x2)) {


		int dirty = 0;
		vsync_queue_node_t *p_node = dev->p_vsync_queue_head;

		dbg_info("se_irq_handler");

//		*(volatile uint32_t *)VSYNC_SHARE_MEM_ADDR = SWAP32(1);
		vsync_rpc_set_flag(SWAP32(1));

		if(dev->ignore_interrupt) {
			return IRQ_HANDLED;
		}

		while(p_node) {
			vsync_queue_t *p_vsync_queue = (vsync_queue_t *)p_node->p_vsync_queue;

			if(p_vsync_queue) {
				uint8_t *u8Buf = (uint8_t *)&p_vsync_queue->u8Buf;
				uint32_t u32RdPtr = p_vsync_queue->u32RdPtr;
				uint32_t u32WrPtr = p_vsync_queue->u32WrPtr;

				while(u32RdPtr != u32WrPtr) {
					uint32_t len = *(uint32_t *)&u8Buf[u32RdPtr];
					dirty = 1;

					dbg_info("2: r=%d, w=%d, size=%d",
								   u32RdPtr, u32WrPtr, *(uint32_t *)&u8Buf[u32RdPtr]);

					if(len == 0) {
						u32RdPtr += 4;
						break;
					}
					else {
						len -= 4;
						u32RdPtr += 4;
						WriteCmd(dev, &u8Buf[u32RdPtr], len);
						u32RdPtr += len;
					}
				}
				p_vsync_queue->u32RdPtr = u32RdPtr;
			}
			p_node = p_node->p_next;
		}
		if(!dirty) {
			vsync_queue_node_t *p_node = dev->p_vsync_queue_head;
			while(p_node) {
				vsync_queue_node_t *p_node_tmp = p_node;
				if(p_node->p_vsync_queue) {
					kfree((void *)p_node->p_vsync_queue);
				}
				p_node = p_node->p_next;
				kfree((void *)p_node_tmp);
			}
			dev->p_vsync_queue_head = dev->p_vsync_queue_tail = NULL;
			//*(volatile uint32_t *)VSYNC_SHARE_MEM_ADDR = SWAP32(0);
			vsync_rpc_set_flag(SWAP32(0));
		}
		return IRQ_HANDLED;
	}

	return IRQ_HANDLED;
}

void se_drv_init(void)
{
	uint32_t dwPhysicalAddress = 0;

	sema_init(&se_drv_data->sem, 1);
	sema_init(&sem_checkfinish, 1);
	down(&sem_checkfinish);

	se_drv_data->size = (SE_COMMAND_ENTRIES * sizeof(uint32_t));

	//se_drv_data[0].CmdBuf = kmalloc(se_drv_data[0].size, GFP_KERNEL);

	dwPhysicalAddress = (uint32_t)__pa(se_drv_data->CmdBuf);

	dbg_info("CMD Buf Addr: 0x%x, Physical Address = %x", (uint32_t)se_drv_data->CmdBuf, dwPhysicalAddress);

	//dev->v_to_p_offset = (int32_t) ((uint32_t)dev->CmdBuf - (uint32_t)dwPhysicalAddress);
	//dev->wrptr = 0;
	se_drv_data->CmdBase = (uint32_t) dwPhysicalAddress;
	se_drv_data->CmdLimit = (uint32_t) (dwPhysicalAddress + se_drv_data->size);
	se_drv_data->CmdWritePtr = se_drv_data->CmdBase;

	InitSeReg();

	if (request_irq(se_device->irq, se_irq_handler, IRQF_SHARED, "se", (void*)se_drv_data)) {
		dbg_err("se: cannot register IRQ %d", se_device->irq);
	}
}

void se_drv_uninit(void)
{
	//*(volatile uint32_t*)VSYNC_SHARE_MEM_ADDR = SWAP32(0);
	vsync_rpc_set_flag(SWAP32(0));

	free_irq(se_device->irq, (void*)se_drv_data);
}

int se_open(struct inode *inode, struct file *file)
{
//	struct se_dev *dev = PDE_DATA(inode);
	struct se_dev *dev = se_drv_data;
	int queue = iminor(inode);		/* we use minor number to indicate queue */

	dbg_info("%s %s: queue = %d",__FILE__, __func__, queue);

	file->private_data = dev;

	/* now trim to 0 the length of the device if open write-only */
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	if (dev->initialized) {
		dev->initialized++;
		up(&dev->sem);
		return 0;
	}

	dev->initialized++;
	dev->queue = queue;

	//*(volatile uint32_t*)VSYNC_SHARE_MEM_ADDR = 0;
	vsync_rpc_set_flag(0);

	up(&dev->sem);

	return 0;
}

int se_release(struct inode *inode, struct file *file)
{
//	struct se_dev *dev = PDE_DATA(inode);
	struct se_dev *dev = se_drv_data;
	int queue = dev->queue;
	volatile SEREG_INFO *SeRegInfo = (volatile SEREG_INFO*)se_device->base;
	dbg_info("%s %s",__FILE__, __func__);

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	dev->initialized--;
	if (dev->initialized > 0) {
		up(&dev->sem);
		return 0;
	}

	// stop SE
	SeRegInfo->SeCtrl[queue].Value = (SeGo | SeEndianSwap | SeClearWriteData);

	up(&dev->sem);

	return 0;
}

ssize_t _se_write(struct file *file, const char __user *buf, size_t count)
{
//	struct se_dev *dev = PDE_DATA(file_inode(file));
	struct se_dev *dev = se_drv_data;
	ssize_t ret = -ENOMEM;

	if (count <= 256) {
		uint8_t data[256];
		if (copy_from_user(data, buf, count)) {
			ret = -EFAULT;
			goto out;
		}
		WriteCmd(dev, (uint8_t*)data, count);
	}
	else {
		uint8_t *data = kzalloc(count, GFP_KERNEL);
		if (copy_from_user(data, buf, count)) {
			kfree(data);
			ret = -EFAULT;
			goto out;
		}
		WriteCmd(dev, (uint8_t*)data, count);
		kfree(data);
	}
	ret = count;
out:
	return ret;
}

ssize_t se_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos)
{
//	struct se_dev *dev = PDE_DATA(file_inode(file));
	struct se_dev *dev = se_drv_data;
	ssize_t size;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	size = _se_write(file, buf, count);
	if (f_pos) *f_pos += size;
	up(&dev->sem);

	return size;
}

long se_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
//	struct se_dev *dev = PDE_DATA(file_inode(file));
	struct se_dev *dev = se_drv_data;
	int retval = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	switch (cmd) {
		case SE_IOC_WAIT_FOR_COMPLETE:
			{
				volatile SEREG_INFO *SeRegInfo = (volatile SEREG_INFO*)se_device->base;
				SEREG_INTS ints;
				ints.Value = 0;
				ints.Fields.write_data = 0;
				ints.Fields.com_empty = 1;
				//SeRegInfo->SeInts[0].Value = 0x9;
				SeRegInfo->SeInts[0].Value = ints.Value;   //clear interrupt
				dbg_info("ioctl wait for complete, %08x, %08x", SeRegInfo->SeCtrl[0].Value,
																SeRegInfo->SeIdle[0].Value);

				//if(SeRegInfo->SeCtrl[0].Fields.go && !SeRegInfo->SeIdle[0].Fields.idle)
				if((SeRegInfo->SeCtrl[0].Value & 0x2) && ((SeRegInfo->SeIdle[0].Value & 0x1) == 0)) {
					SEREG_INTE inte;
					inte.Value = 0;
					inte.Fields.write_data = 1;
					inte.Fields.com_empty = 1;
					SeRegInfo->SeInte[0].Value = inte.Value;  //enable interrupt
					dbg_info("wait for complete before down");
					down(&sem_checkfinish);
					dbg_info("wait for complete after down");
				}
				break;
			}
		case SE_IOC_READ_INST_COUNT:
			{
				dbg_info("%s:%d, se ioctl code=%d!!!!!!!!!!!!!!!", __FILE__, __LINE__, cmd);
				UpdateInstCount(dev);
				if (copy_to_user((void __user *)arg, (void *)&dev->u64InstCnt, sizeof(dev->u64InstCnt))) {
					retval = -EFAULT;
					goto out;
				}
				//dbg_info("se ioctl code=SE_IOC_READ_INST_COUNT:\n");
				break;
			}
		case SE_IOC_READ_SW_CMD_COUNTER:
			{
				dbg_info("%s:%d, se ioctl code=%d, counter = %lld!!!!!!!!!!!!!!!\n", __FILE__, __LINE__, cmd, dev->u64IssuedInstCnt);
				if (copy_to_user((void __user *)arg, (void *)&dev->u64IssuedInstCnt, sizeof(dev->u64IssuedInstCnt))) {
					retval = -EFAULT;
					goto out;
				}
				//dbg_info("se ioctl code=SE_IOC_READ_INST_COUNT:\n");
				break;
			}
		case SE_IOC_WRITE_ISSUE_CMD:
			{
				struct {
					uint32_t instCnt;
					uint32_t length;
					void *   cmdBuf;
				} header;
				dbg_info("%s:%d, se ioctl code=%d!!!!!!!!!!!!!!!", __FILE__, __LINE__, cmd);
				dev->ignore_interrupt = 1;
				if (copy_from_user((void *)&header, (const void __user *)arg, sizeof(header))) {
					retval = -EFAULT;
					dbg_info("se ioctl code=SE_IOC_WRITE_ISSUE_CMD failed!!!!!!!!!!!!!!!");
					goto out;
				}

				dbg_info("se ioctl code=SE_IOC_WRITE_ISSUE_CMD!!!!! count = %d, size = %d, 0x%08x", header.instCnt, header.length, (uint32_t)header.cmdBuf);
				_se_write(file, (const void __user *)header.cmdBuf, header.length);
				dev->u64IssuedInstCnt += header.instCnt;
				//printk("<%lld>\n", dev->u64IssuedInstCnt);
				dev->ignore_interrupt = 0;
				dbg_info("se ioctl code=SE_IOC_WRITE_ISSUE_CMD");
				break;
			}
		case SE_IOC_WRITE_QUEUE_CMD:
			{
				int ii;
				struct {
					uint32_t instCnt;;
					uint32_t count;
				} header;
				uint32_t count;
				vsync_queue_t *p_vsync_queue[MAX_QUEUE_ENTRIES] = {NULL,};

				dbg_info("%s:%d:%s\n", __FILE__, __LINE__, __func__);
				dbg_info("se ioctl code=SE_IOC_WRITE_QUEUE_CMD\n");

				if (copy_from_user((void *)&header, (const void __user *)arg, sizeof(header))) {
					retval = -EFAULT;
					dbg_info("%d, se ioctl code=SE_IOC_WRITE_QUEUE_CMD failed!!!!!!!!!!!!!!!1\n", __LINE__);
					goto out;
				}
				count = header.count;
				if(count > MAX_QUEUE_ENTRIES) {
					retval = -EFAULT;
					dbg_info("%d, se ioctl code=SE_IOC_WRITE_QUEUE_CMD failed!!!!!!!!!!!!!!!1\n", __LINE__);
					goto out;
				}
				if (copy_from_user((void *)p_vsync_queue, (const void __user *)((uint32_t)arg + sizeof(header)), count*sizeof(vsync_queue_t *))) {
					retval = -EFAULT;
					dbg_info("%d, se ioctl code=SE_IOC_WRITE_QUEUE_CMD failed!!!!!!!!!!!!!!!1\n", __LINE__);
					goto out;
				}
				dbg_info("%d, count = %d, instCount = %d\n", __LINE__, count, header.instCnt);
				dev->ignore_interrupt = 1;
				for(ii=0; ii<count; ii++) {
					uint32_t size;
					uint8_t *buf;
					vsync_queue_node_t *p_node;
					if (copy_from_user((void *)&size, (const void __user *)p_vsync_queue[ii], sizeof(uint32_t))) {
						retval = -EFAULT;
						dbg_info("%d, se ioctl code=SE_IOC_WRITE_QUEUE_CMD failed!!!!!!!!!!!!!!!1\n", __LINE__);
						dev->ignore_interrupt = 0;
						goto out;
					}

					size += offsetof(vsync_queue_t, u8Buf);
					buf = kmalloc(size, GFP_KERNEL);
					if(buf == NULL) {
						retval = -EFAULT;
						dbg_info("%d, se ioctl code=SE_IOC_WRITE_QUEUE_CMD failed!!!!!!!!!!!!!!!1\n", __LINE__);
						dev->ignore_interrupt = 0;
						goto out;
					}

					if(copy_from_user((void *)buf, (const void __user *)p_vsync_queue[ii], size)) {
						retval = -EFAULT;
						dbg_info("%d, se ioctl code=SE_IOC_WRITE_QUEUE_CMD failed!!!!!!!!!!!!!!!1\n", __LINE__);
						dev->ignore_interrupt = 0;
						kfree(buf);
						goto out;
					}
					p_node = (vsync_queue_node_t *)kmalloc(sizeof(vsync_queue_node_t), GFP_KERNEL);
					p_node->p_next = NULL;
					p_node->p_vsync_queue = (vsync_queue_t *)buf;;
					if(dev->p_vsync_queue_head == NULL) {
						dev->p_vsync_queue_head = dev->p_vsync_queue_tail = p_node;
					}
					else {
						dev->p_vsync_queue_tail->p_next = p_node;
						dev->p_vsync_queue_tail = p_node;
					}
				}
				dbg_info("@@@@@@@@@@@ se.c,(%p, %lld)\n", &dev->u64IssuedInstCnt, dev->u64IssuedInstCnt);
				//dev->u64QueueInstCnt += header.instCnt;
				dev->u64IssuedInstCnt += header.instCnt;
				dbg_info("@@@@@@@@@@@ se.c,(%lld)\n", dev->u64IssuedInstCnt);
				dev->ignore_interrupt = 0;
				//*(volatile uint32_t *)VSYNC_SHARE_MEM_ADDR |= SWAP32(1);
				vsync_rpc_set_flag(SWAP32(1));

				break;
			}
		case SE_IOC_VIDEO_RPC_YUYV_TO_RGB:
			{
				VIDEO_RPC_YUYV_TO_RGB *structYuyvToRbg = 0;

				if(structYuyvToRbg == 0) {
					struct page *page = alloc_page(GFP_KERNEL);
					structYuyvToRbg = (VIDEO_RPC_YUYV_TO_RGB *)page_address(page);
				}

				if (copy_from_user((void *)structYuyvToRbg, (const void __user *)arg, sizeof(VIDEO_RPC_YUYV_TO_RGB))) {
					retval = -EFAULT;
					dbg_info("se ioctl code=SE_IOC_VIDEO_RPC_YUYV_TO_RGB failed!!!!!!!!!!!!!!!1\n");
					goto out;
				}

				// FIXME: issue RPC to AVCPU for format conversion
				dbg_err("DONT implemtation RPC yet");
#if 0
				{
					unsigned long ret;
					#define HTOL(a) (a = htonl(a))
					HTOL(structYuyvToRbg->yuyv_addr);
					HTOL(structYuyvToRbg->rgb_addr);
					HTOL(structYuyvToRbg->width);
					HTOL(structYuyvToRbg->height);
					HTOL(structYuyvToRbg->imgPitch);
					HTOL(structYuyvToRbg->yuv_fmt);
					HTOL(structYuyvToRbg->rgb_fmt);

					if (send_rpc_command(RPC_VIDEO, RPCCMD_YUYV_TO_RGB, UNCAC_BASE|(unsigned long)structYuyvToRbg, 0, &ret)) {
						goto out;
					}
					if (ret != S_OK) {
						printk("YUYV_TO_RBG failed\n");
						goto out;
					}
				}
#endif
			}
			break;
		default:
			dbg_warn("se ioctl code not supported");
			retval = -ENOTTY;
			goto out;
	}

out:
	up(&dev->sem);
	return retval;
}

int se_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long offset = vma->vm_pgoff;
//	struct se_dev *dev = PDE_DATA(file_inode(file));
	struct se_dev *dev = se_drv_data;
	int ret = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	if (offset > (~0UL >> PAGE_SHIFT)) {
		dbg_err("%s %d offset = %ld", __func__, __LINE__, offset);
		ret = -EINVAL;
		goto out;
	}

	offset = offset << PAGE_SHIFT;

	if (offset == 0) {
		offset = (unsigned long)__pa(dev);
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	}
	else if (offset == 0x1800c000) {
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	}
	else {
		dbg_err("%s %d offset = %ld", __func__, __LINE__, offset);
		ret = -EINVAL;
		goto out;
	}

	if (io_remap_pfn_range(vma, vma->vm_start, offset >> PAGE_SHIFT, size, vma->vm_page_prot)) {
		ret = -EAGAIN;
		goto out;
	}

out:
	up(&dev->sem);
	return ret;
}
