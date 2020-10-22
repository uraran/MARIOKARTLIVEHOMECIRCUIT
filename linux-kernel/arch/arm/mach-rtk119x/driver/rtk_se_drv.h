#ifndef __SE_DRIVER_H__
#define __SE_DRIVER_H__

#include <linux/ioctl.h>
#include <linux/cdev.h>

#include "se_export.h"
#include "debug.h"

#define SE_COMMAND_ENTRIES		4096

#define MAX_QUEUE_ENTRIES		32
#define BUF_OFFSET				12
#define BUF_SIZE				(4096*32 - BUF_OFFSET)

typedef struct vsync_queue {
	uint32_t u32WrPtr;
	uint32_t u32RdPtr;
	uint32_t u32Released;
	uint8_t  u8Buf[BUF_SIZE];
} vsync_queue_t;

typedef struct vsync_queue_node {
	struct vsync_queue_node *p_next;
	vsync_queue_t           *p_vsync_queue;
} vsync_queue_node_t;


#define S_OK		0x10000000

#define RPCCMD_YUYV_TO_RGB 0x100

enum YUV_FMT {
	FMT_YUYV = 0,
	FMT_YUV420 = 1,
	FMT_YUV422P = 2,
};
typedef enum YUV_FMT YUV_FMT;

typedef enum IMG_TARGET_FORMAT {
	IMG_YUV,                  /* pTargetLuma is buffer id (32bits) , LSB(16bits) is Y_BufID, MSB(16bits) is C_BufID */
	IMG_ARGB8888,
	IMG_RGB565,
	IMG_RGB332,               /* don't support */
	IMG_HANDLE,               /* don't support */
	IMG_RGBA8888,
	IMG_RGBA8888_LE,          /* little endin for RGBA8888 */
	IMG_ARGB8888_LE,          /* little endin for ARGB8888 */
	IMG_RGB565_LE             /* little endin for RGB565 */
} IMG_TARGET_FORMAT;

typedef struct VIDEO_RPC_YUYV_TO_RGB {
	long yuyv_addr;
	long rgb_addr;
	long width;
	long height;
	long imgPitch;
	enum YUV_FMT yuv_fmt;
	enum IMG_TARGET_FORMAT rgb_fmt;
} VIDEO_RPC_YUYV_TO_RGB;

struct se_dev {
	volatile uint32_t initialized;
	volatile uint32_t CmdBase;
	volatile uint32_t CmdLimit;
	volatile uint32_t CmdWritePtr;
//	volatile uint32_t size;
	volatile uint64_t u64InstCnt;
	volatile uint64_t u64IssuedInstCnt;
	volatile uint64_t u64QueueInstCnt;
	volatile uint32_t size;
	volatile uint32_t ignore_interrupt;
	volatile uint32_t queue;
//	volatile uint64_t u64QueueInstCnt;
	volatile uint32_t reserved[3];	//pad to 16
	volatile uint8_t  CmdBuf[SE_COMMAND_ENTRIES * sizeof(uint32_t)];
	vsync_queue_node_t *p_vsync_queue_head;
	vsync_queue_node_t *p_vsync_queue_tail;
	struct semaphore   sem;
	struct cdev        cdev;
};

struct se_device {
	struct miscdevice	dev;
	void __iomem		*base;
	int					irq;
};


int se_open(struct inode *inode, struct file *file);
int se_release(struct inode *inode, struct file *file);
long se_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
ssize_t se_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos);
int se_mmap(struct file *file, struct vm_area_struct *vma);


void se_drv_init(void);
void se_drv_uninit(void);

// se drvier global
extern struct se_dev		*se_drv_data;
extern struct se_device		*se_device;


#endif // __SE_DRIVER_H__
