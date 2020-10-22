#ifndef __SE_EXPORT_H__
#define __SE_EXPORT_H__

/*
 * ioctl definitions
 */

/* Use 'k' as magic number */
#define SE_IOC_MAGIC	'k'

/* Please use a different 8-bit nunber in your code */
#define SE_IOCRESET		_IO(SE_IOC_MAGIC, 0)

/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": switch G and S atomically
 * H means "sHift": switch T and Q atomically
 */
#define SE_IOCSQUANTUM	_IOW (SE_IOC_MAGIC, 1,  int)
#define SE_IOCSQSET		_IOW (SE_IOC_MAGIC, 2,  int)
#define SE_IOCTQUANTUM	_IO  (SE_IOC_MAGIC, 3)
#define SE_IOCTQSET		_IO  (SE_IOC_MAGIC, 4)
#define SE_IOCGQUANTUM	_IOR (SE_IOC_MAGIC, 5,  int)
#define SE_IOCGQSET		_IOR (SE_IOC_MAGIC, 6,  int)
#define SE_IOCQQUANTUM	_IO  (SE_IOC_MAGIC, 7)
#define SE_IOCQQSET		_IO  (SE_IOC_MAGIC, 8)
#define SE_IOCXQUANTUM	_IOWR(SE_IOC_MAGIC, 9,  int)
#define SE_IOCXQSET		_IOWR(SE_IOC_MAGIC, 10, int)
#define SE_IOCHQUANTUM	_IO  (SE_IOC_MAGIC, 11)
#define SE_IOCHQSET		_IO  (SE_IOC_MAGIC, 12)

/*
 * The other entities only have "Tell" and "Query", because they're
 * not printed in the book, and there's no need to have all six.
 * (The previous stuff was only there to show different ways to do it.
 */
#define SE_P_IOCTSIZE						_IO  (SE_IOC_MAGIC, 13)
#define SE_P_IOCQSIZE						_IO  (SE_IOC_MAGIC, 14)
/* ... more to come */

#define SE_IOC_DG_LOCK						_IOW (SE_IOC_MAGIC, 15, int)

#define SE_IOC_DG_UNLOCK					_IO  (SE_IOC_MAGIC, 16)

#define SE_IOC_READ_HW_CMD_COUNTER			_IOR (SE_IOC_MAGIC, 17, int)

#define SE_IOC_READ_SW_CMD_COUNTER			_IOR (SE_IOC_MAGIC, 18, int)

#define SE_IOC_SET_DCU_INFO					_IOW (SE_IOC_MAGIC, 19, int)

#define SE_IOC_READ_HW_CMD_COUNTER_LOW_WORD	_IOR (SE_IOC_MAGIC, 20, int)

#define SE_IOC_SET_VSYNC_QUEUE				_IOR (SE_IOC_MAGIC, 21, vsync_queue_param_t)

#define SE_IOC_READ_INST_COUNT				_IOR (SE_IOC_MAGIC, 22, int)

#define SE_IOC_WRITE_ISSUE_CMD				_IOR (SE_IOC_MAGIC, 23, int)

#define SE_IOC_WRITE_QUEUE_CMD				_IOR (SE_IOC_MAGIC, 24, int)

#define SE_IOC_VIDEO_RPC_YUYV_TO_RGB		_IO  (SE_IOC_MAGIC, 25)

#define SE_IOC_WAIT_FOR_COMPLETE			_IO  (SE_IOC_MAGIC, 26)

#define SE_IOC_MAXNR 27

#endif /*__SE_EXPORT_H__*/