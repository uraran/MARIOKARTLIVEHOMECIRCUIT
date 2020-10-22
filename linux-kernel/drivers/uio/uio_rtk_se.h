#ifndef __SE_DRIVER_H__
#define __SE_DRIVER_H__

#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>

//#include "se_export.h"
//#include "debug.h"

#define SE_NUM_ENGINES			2 //3
#define SE_COMMAND_ENTRIES		4096
#define SE_CMDBUF_SIZE			(SE_COMMAND_ENTRIES * sizeof(uint32_t))

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

typedef struct se_engine {
	volatile uint8_t  *CmdBuf;          //kernel virtual start address of command queue
	volatile uint32_t CmdBase;          //physical start address of command queue
	volatile uint32_t CmdLimit;         //physical end address of command queue
	volatile uint32_t CmdWritePtr;      //physical address of write pointer
	volatile uint32_t BufSize;          //total size of command queue
} se_engine_t;

typedef struct se_hw {
	struct se_engine engine[SE_NUM_ENGINES];
} se_hw_t;

typedef struct se_resource {
	void __iomem		*base;
	int					irq;
} se_resource_t;

#endif // __SE_DRIVER_H__