#ifndef _RPCDRIVER_H
#define _RPCDRIVER_H

//#define MY_COPY

#define KERNEL2_6

#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif

//#define REG_SB2_CPU_INT           0xb801a104
//#define REG_SB2_CPU_INT_EN        0xb801a108
#define RPC_SB2_INT         0x0
#define RPC_SB2_INT_EN      0x4


#define RPC_INT_WRITE_1 1

#define RPC_INT_AS  (1 << 3)
#define RPC_INT_SA  (1 << 1)

#define __read_32bit_caller_register()      \
({ int __res;                               \
    __asm__ __volatile__(                   \
        "mov\t%0, lr\n\t"                   \
        : "=r" (__res));                    \
    __res;                                  \
})

#define CONFIG_REALTEK_RPC_PROGRAM_REGISTER
#define RPC_SUPPORT_MULTI_CALLER_SEND_TID_PID

//#define CONFIG_REALTEK_AVCPU

#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/current.h>

#define R_PROGRAM       98
#define AUDIO_AGENT     202
#define VIDEO_AGENT     300

#define KERNELID    98
#define REPLYID     99
#define RPC_AUDIO   0x0
#define RPC_VIDEO   0x1
#define RPC_VIDEO2  0x2 //add by Angus
#define RPC_KCPU    0x3
#define RPC_OK      0
#define RPC_FAIL    -1

#ifndef RPC_ID
#define RPC_ID 0x5566
#endif

#ifndef RPC_MAJOR
#define RPC_MAJOR 240       /* dynamic major by default */
#endif

#ifndef RPC_NR_DEVS
//modify by Angus, 2 for S-A, 2 for A-S, 2 for S-V1, 2 for V1-S, 2 for S-V2, 2 for V2-S
typedef enum
{
    RPC_POLL_DEV_SA_ID0,    //poll audio write
    RPC_POLL_DEV_AS_ID1,    //poll audio read
    RPC_POLL_DEV_SV1_ID2,   //poll video write
    RPC_POLL_DEV_V1S_ID3,   //poll video read
//  RPC_POLL_DEV_SV2_ID4,
//  RPC_POLL_DEV_V2S_ID5,
    RPC_POLL_DEV_TOTAL
}NUM_POLL_DEV;

typedef enum
{
    RPC_INTR_DEV_SA_ID0,    //intr audio write
    RPC_INTR_DEV_AS_ID1,    //intr audio read
    RPC_INTR_DEV_SV1_ID2,   //intr video write
    RPC_INTR_DEV_V1S_ID3,   //intr video read
//  RPC_INTR_DEV_SV2_ID4,
//  RPC_INTR_DEV_V2S_ID5,
    RPC_INTR_DEV_TOTAL
}NUM_INTR_DEV;
#define RPC_NR_DEVS     (RPC_POLL_DEV_TOTAL + RPC_INTR_DEV_TOTAL)       /* total ring buffer number */

#endif

#ifndef RPC_NR_PAIR
#define RPC_NR_PAIR 2       /* for interrupt and polling mode */
#endif

#ifndef RPC_NR_KERN_DEVS
//modify by Angus, S-A,  A-S,  S-V1, V1-S,  S-V2,  V2-S

typedef enum
{
    RPC_KERN_DEV_SA_ID0,    //kern audio write
    RPC_KERN_DEV_AS_ID1,    //kern audio read
    RPC_KERN_DEV_SV1_ID2,   //kern video write
    RPC_KERN_DEV_V1S_ID3,   //kern video read
//  RPC_KERN_DEV_SV2_ID4,
//  RPC_KERN_DEV_V2S_ID5,
    RPC_NR_KERN_DEVS    /* total ring buffer number */
}NUM_KERN_DEV;

//#define RPC_NR_KERN_DEVS 6    /* total ring buffer number */
#endif

#define AVCPU_NOCACHE       0xa0000000
#define AVCPU_SCPU_OFFSET   (AVCPU_NOCACHE + RPC_RINGBUF_PHYS - (unsigned long)RPC_RINGBUF_VIRT)
#define AVCPU2SCPU(x)       (x - AVCPU_SCPU_OFFSET)
#define SCPU2AVCPU(x)       (x + AVCPU_SCPU_OFFSET)
#define RPCREAD32(addr)     AVCPU2SCPU(addr)
#define RPCWRITE32(addr, val)   do{ AVCPU2SCPU(addr) = val; }while(0)

#ifndef RPC_RING_SIZE
#define RPC_RING_SIZE 512   /* size of ring buffer */
#endif

#ifndef RPC_POLL_DEV_ADDR
#define RPC_POLL_DEV_ADDR (AVCPU_NOCACHE + 0x01ffe000)
#endif

#ifndef RPC_INTR_DEV_ADDR
#define RPC_INTR_DEV_ADDR (RPC_POLL_DEV_ADDR+RPC_RING_SIZE)
#endif

#ifndef RPC_KERN_DEV_ADDR
#define RPC_KERN_DEV_ADDR (AVCPU_NOCACHE + 0x01fff200)
#endif

#ifndef RPC_RECORD_SIZE
#define RPC_RECORD_SIZE 64  /* size of ring buffer record */
#endif

#ifndef RPC_POLL_RECORD_ADDR
#define RPC_POLL_RECORD_ADDR (AVCPU_NOCACHE + 0x01fff000)
#endif

#ifndef RPC_INTR_RECORD_ADDR
#define RPC_INTR_RECORD_ADDR (RPC_POLL_RECORD_ADDR+RPC_RECORD_SIZE*(RPC_NR_DEVS/RPC_NR_PAIR))
#endif

#ifndef RPC_KERN_RECORD_ADDR
#define RPC_KERN_RECORD_ADDR (AVCPU_NOCACHE + 0x01fffa00)
#endif

extern struct file_operations rpc_poll_fops;   /* for poll mode */
extern struct file_operations rpc_intr_fops;   /* for intr mode */
extern struct file_operations rpc_ctrl_fops;   /* for ctrl mode */

extern struct file_operations *rpc_fop_array[];

typedef struct RPC_STRUCT {
    unsigned long programID;      // program ID defined in IDL file
    unsigned long versionID;      // version ID defined in IDL file
    unsigned long procedureID;    // function ID defined in IDL file

    unsigned long taskID;         // the caller's task ID, assign 0 if NONBLOCK_MODE
#ifdef RPC_SUPPORT_MULTI_CALLER_SEND_TID_PID
    unsigned long sysTID;
#endif
    unsigned long sysPID;         // the callee's task ID
    unsigned long parameterSize;  // packet's body size
    unsigned long mycontext;      // return address of reply value
} RPC_STRUCT;

typedef struct RPC_SYNC_Struct {
    wait_queue_head_t   waitQueue;  /* for blocking read */
//    struct semaphore    readSem;    /* mutual exclusion semaphore */
//    struct semaphore    writeSem;   /* mutual exclusion semaphore */
    struct rw_semaphore readSem;
    struct rw_semaphore writeSem;
} RPC_SYNC_Struct;

/*
// read : nonblocking
// write: nonblocking
*/
typedef struct RPC_POLL_Dev {
    char            *ringBuf;   /* buffer for interrupt mode */
    char            *ringStart; /* pointer to start of ring buffer */
    char            *ringEnd;   /* pointer to end   of ring buffer */
    char            *ringIn;    /* pointer to where next data will be inserted  in   the ring buffer */
    char            *ringOut;   /* pointer to where next data will be extracted from the ring buffer */

    RPC_SYNC_Struct     *ptrSync;
    unsigned long       dummy[7];
} RPC_POLL_Dev;

/*
// read : nonblocking blocking
// write: nonblocking
*/
typedef struct RPC_INTR_Dev {
    char            *ringBuf;   /* buffer for interrupt mode */
    char            *ringStart; /* pointer to start of ring buffer */
    char            *ringEnd;   /* pointer to end   of ring buffer */
    char            *ringIn;    /* pointer to where next data will be inserted  in   the ring buffer */
    char            *ringOut;   /* pointer to where next data will be extracted from the ring buffer */

    RPC_SYNC_Struct     *ptrSync;
    unsigned long       dummy[7];
} RPC_INTR_Dev;

/*
// read : nonblocking blocking
// write: nonblocking
*/
typedef struct RPC_KERN_Dev {
    char            *ringBuf;   /* buffer for interrupt mode */
    char            *ringStart; /* pointer to start of ring buffer */
    char            *ringEnd;   /* pointer to end   of ring buffer */
    char            *ringIn;    /* pointer to where next data will be inserted  in   the ring buffer */
    char            *ringOut;   /* pointer to where next data will be extracted from the ring buffer */

    RPC_SYNC_Struct     *ptrSync;
    unsigned long       dummy[7];
} RPC_KERN_Dev;

typedef struct RPC_COMMON_Dev {
    char                    *ringBuf;       /* buffer for interrupt mode */
    char                    *ringStart;     /* pointer to start of ring buffer */
    char                    *ringEnd;       /* pointer to end   of ring buffer */
    char                    *ringIn;        /* pointer to where next data will be inserted  in   the ring buffer */
    char                    *ringOut;       /* pointer to where next data will be extracted from the ring buffer */
} RPC_COMMON_Dev;

typedef struct RPC_DEV_EXTRA {
    RPC_COMMON_Dev          *dev;
    void                    *currProc;      /* current process which handles rpc command */
    char                    *name;          /* the rpc device name */
    struct list_head        tasks;          /* tasks that open this rpc device */
    struct tasklet_struct   tasklet;        /* schedule a tasklet for bottom half */
    char                    *nextRpc;       /* points to next Rpc */
    spinlock_t              lock;           /* lock to protect data synchronization */
    struct device           *sdev;          /* struct device */
} RPC_DEV_EXTRA;

//maintain a list of threads in the same process
typedef struct RPC_THREAD
{
    pid_t                   pid;
    struct list_head        list;
} RPC_THREAD;

//maintain a list of processes that use the same RPC device
typedef struct RPC_PROCESS
{
    pid_t                   pid;
    wait_queue_head_t       waitQueue;
    RPC_COMMON_Dev          *dev;
    RPC_DEV_EXTRA           *extra;
#ifdef CONFIG_REALTEK_RPC_PROGRAM_REGISTER
    struct list_head        handlers;
#endif
    struct list_head        threads;        /* pids that share the same file descriptor */
    struct list_head        list;
} RPC_PROCESS;

#ifdef CONFIG_REALTEK_RPC_PROGRAM_REGISTER
//maintain a list of handlers
typedef struct RPC_HANDLER
{
    unsigned long           programID;
    struct list_head        list;
} RPC_HANDLER;
#endif

typedef struct RPC_DBG_FLAG
{
    int op;      //0:get, 1:set
    unsigned int flagValue;
    unsigned int flagAddr;
} RPC_DBG_FLAG;

extern int pid_max;
extern RPC_DEV_EXTRA rpc_intr_extra[RPC_NR_DEVS/RPC_NR_PAIR];
extern RPC_DEV_EXTRA rpc_poll_extra[RPC_NR_DEVS/RPC_NR_PAIR];
void rpc_dispatch(unsigned long data);

#ifdef MY_COPY
//FIXME: accepts SCPU addr
static inline int my_memcpy(int *des, int *src, int size)
{
    char *csrc, *cdes;
    int i;

    pr_debug("%s dest:%p src:%p size:%d\n", __func__, des, src, size);
    if (((int)src & 0x3) || ((int)des & 0x3))
        pr_warn("my_memcpy: unaligned happen...\n");

    while (size >= 4) {
        *des++ = *src++;
        size -= 4;
    }

    csrc = (char *)src;
    cdes = (char *)des;
    for (i = 0; i < size; i++)
        cdes[i] = csrc[i];

    return 0;
}
#endif

//FIXME: must use the same type of addr (AVCPU addr or SCPU addr)
static inline int __get_ring_data_size(char *start, char *end, char *in, char *out)
{
    int ringsize = end - start;
    int datasize = (ringsize + in - out) % ringsize;
    pr_debug("%s ringsize:%d datasize:%d start:%p end:%p in:%p out:%p\n", __func__, ringsize, datasize, start, end, in, out);
    return datasize;
}

static inline int get_ring_data_size(RPC_COMMON_Dev *dev)
{
    return __get_ring_data_size(dev->ringStart, dev->ringEnd, dev->ringIn, dev->ringOut);
}

//get ring data in buf and return next data pointer
//FIXME: out is AVCPU addr
static inline char *get_ring_data(const char *func, RPC_COMMON_Dev *dev, char *out, char *buf, int datasize)
{
    int size = __get_ring_data_size(dev->ringStart, dev->ringEnd, dev->ringIn, out);
    int tail = dev->ringEnd - out;

    if(size < datasize){
        pr_warn("%s: Size not enough %d < %d\n", func, size, datasize);
        return NULL;
    }

    if(tail >= datasize){
#ifdef MY_COPY
        my_memcpy((int *)buf, (int *)AVCPU2SCPU(out), datasize);
#else
        memcpy(buf, AVCPU2SCPU(out), datasize);
#endif
        out += datasize;
    }else{
#ifdef MY_COPY
        my_memcpy((int *)buf, (int *)AVCPU2SCPU(out), tail);
        my_memcpy((int *)(buf + tail), (int *)AVCPU2SCPU(dev->ringStart), datasize - tail);
#else
        memcpy(buf, AVCPU2SCPU(out), tail);
        memcpy(buf + tail, AVCPU2SCPU(dev->ringStart), datasize - tail);
#endif
        out = dev->ringStart + datasize - tail;
    }

    return out;
}

static inline void dump_ring_buffer(const char *func, RPC_DEV_EXTRA *extra)
{
    int i;
    RPC_COMMON_Dev *dev = extra->dev;
    char *bufaddr = AVCPU2SCPU(dev->ringStart);
    pr_err("==*== %s Start:%p Out:%p next:%p In:%p End:%p size:%d %s ==*==\n", func, AVCPU2SCPU(dev->ringStart), AVCPU2SCPU(dev->ringOut), AVCPU2SCPU(extra->nextRpc), AVCPU2SCPU(dev->ringIn), AVCPU2SCPU(dev->ringEnd), get_ring_data_size(dev), in_atomic() ? "atomic" : "");
    for(i = 0; i < RPC_RING_SIZE; i+=16){
        unsigned long *addr = (unsigned long *)(bufaddr + i);
        pr_err("%p: %08x %08x %08x %08x\n", addr, ntohl(*(addr + 0)), ntohl(*(addr + 1)), ntohl(*(addr + 2)), ntohl(*(addr + 3)));
    }
}

static inline void convert_rpc_struct(const char *func, RPC_STRUCT *rpc)
{
    rpc->programID = ntohl(rpc->programID);
    rpc->versionID = ntohl(rpc->versionID);
    rpc->procedureID = ntohl(rpc->procedureID);
    rpc->taskID = ntohl(rpc->taskID);
#ifdef RPC_SUPPORT_MULTI_CALLER_SEND_TID_PID
    rpc->sysTID = ntohl(rpc->sysTID);
#endif
    rpc->sysPID = ntohl(rpc->sysPID);
    rpc->parameterSize = ntohl(rpc->parameterSize);
    rpc->mycontext = ntohl(rpc->mycontext);
}

static inline void show_rpc_struct(const char *func, RPC_STRUCT *rpc)
{
#ifdef RPC_SUPPORT_MULTI_CALLER_SEND_TID_PID
    pr_err("%s: program:%lu version:%lu procedure:%lu taskID:%lu sysTID:%lu sysPID:%lu size:%lu context:%lx %s\n",
        func, rpc->programID, rpc->versionID, rpc->procedureID, rpc->taskID, rpc->sysTID, rpc->sysPID, rpc->parameterSize,
        rpc->mycontext, in_atomic() ? "atomic" : "");
#else
    pr_err("%s: program:%lu version:%lu procedure:%lu taskID:%lu sysPID:%lu size:%lu context:%lx %s\n",
        func, rpc->programID, rpc->versionID, rpc->procedureID, rpc->taskID, rpc->sysPID, rpc->parameterSize,
        rpc->mycontext, in_atomic() ? "atomic" : "");
#endif
}

static inline void peek_rpc_struct(const char *func, RPC_COMMON_Dev *dev)
{
    RPC_STRUCT rpc;
    unsigned long pid;
    unsigned long arg;
    char *out = dev->ringOut;

    out = get_ring_data(func, dev, out, (char *)&rpc, sizeof(RPC_STRUCT));
    if(out == NULL)
        return;
    convert_rpc_struct(func, &rpc);
    show_rpc_struct(func, &rpc);

    if(rpc.programID == AUDIO_AGENT && rpc.versionID == 0){
        //Parse more information here
    }else if(rpc.programID == VIDEO_AGENT && rpc.versionID == 0){
        //Parse more information here
    }else if(rpc.programID == R_PROGRAM && rpc.versionID == 0){
        out = get_ring_data(func, dev, out, (char *)&arg, sizeof(unsigned long));
        if(out == NULL)
            return;
        arg = ntohl(arg);
        if(rpc.procedureID == 1)
            pr_info("%s: alloc %lu bytes\n", func, arg);
        else
            pr_info("%s: free addr %lx\n", func, arg);
    }else if(rpc.programID == REPLYID && rpc.versionID == REPLYID){
        out = get_ring_data(func, dev, out, (char *)&pid, sizeof(unsigned long));
        if(out == NULL)
            return;
        pid = ntohl(pid);
        pr_info("%s: reply to taskid:%lu\n", func, pid);
    }
}

#if 0
static inline void update_currProc(RPC_DEV_EXTRA *extra, RPC_PROCESS *proc)
{
    extra->currProc = proc;
}

//FIXME: accept AVCPU addr
static inline void update_nextRpc(RPC_DEV_EXTRA *extra, char *nextRpc)
{
    extra->nextRpc = nextRpc;
}

static inline int rpc_done(RPC_DEV_EXTRA *extra)
{
    RPC_COMMON_Dev *dev = extra->dev;
    return dev->ringOut == extra->nextRpc;
}

static inline int ring_empty(RPC_COMMON_Dev *dev)
{
    return dev->ringOut == dev->ringIn;
}

static inline int need_dispatch(RPC_DEV_EXTRA *extra)
{
    //size is not zero and previous rpc is done
    return extra->currProc == NULL && !ring_empty(extra->dev) && rpc_done(extra);
}
#else
#define update_currProc(extra, proc)    do{ extra->currProc = proc; }while(0)
#define update_nextRpc(extra, next)     do{ extra->nextRpc = next; }while(0)
#define rpc_done(extra)                 (extra->dev->ringOut == extra->nextRpc)
#define ring_empty(dev)                 (dev->ringOut == dev->ringIn)
#define need_dispatch(extra)            (extra->currProc == NULL && !ring_empty(extra->dev) && rpc_done(extra))
#endif

static inline RPC_PROCESS *pick_one_proc(RPC_DEV_EXTRA *extra)
{
    RPC_PROCESS *proc = NULL;
    if(!list_empty(&extra->tasks)){
        proc = list_first_entry(&extra->tasks, RPC_PROCESS, list);
        pr_debug("%s:%d Pick process:%d\n", extra->name, __LINE__, proc->pid);
    }else{
        pr_debug("%s:%d No available process\n", extra->name, __LINE__);
    }
    return proc;
}

#ifdef CONFIG_REALTEK_RPC_PROGRAM_REGISTER
static inline RPC_PROCESS *pick_supported_proc(RPC_DEV_EXTRA *extra, unsigned long programID)
{
    RPC_PROCESS *proc;
    RPC_HANDLER *handler;

    list_for_each_entry(proc, &extra->tasks, list){
        list_for_each_entry(handler, &proc->handlers, list){
            if(handler->programID == programID){
                pr_debug("%s:%d pid:%d supports programID:%lu\n", __func__, __LINE__, proc->pid, programID);
                return proc;
            }
        }
    }
    pr_debug("%s:%d can't find any process supports programID:%lu\n", __func__, __LINE__, programID);
    return NULL;
}
#endif

static inline RPC_PROCESS *lookup_by_taskID(RPC_DEV_EXTRA *extra, unsigned long taskID)
{
    pid_t pid;
    RPC_PROCESS *proc = NULL;
    struct task_struct *task;

    //sanity check
    if(unlikely(taskID >= pid_max)){
        pr_err("%s:%d invalid taskID:%lu >= pid_max:%d\n", extra->name, __LINE__, taskID, pid_max);
        return NULL;
    }

    task = find_task_by_pid_ns(taskID, &init_pid_ns);
    if(task == NULL){
        __WARN_printf("%s:%d can't find_task_by_pid_ns! taskID:%lu, thread may be dead\n", extra->name, __LINE__, taskID);
        //BUG();
        pid = taskID; //taskID may be dead, use it directly, risky!! the same taskID may exist in more than one thread list
    }else{
        pid = task->tgid;
        pr_debug("%s:%d find_task_by_pid_ns taskID:%lu => pid:%d tgid:%d\n", extra->name, __LINE__, taskID, task->pid, task->tgid);
    }

    list_for_each_entry(proc, &extra->tasks, list){
        pr_debug("%s:%d proc->pid:%d target-pid:%d\n", extra->name, __LINE__, proc->pid, pid);
        if(proc->pid == pid)
        {
            pr_debug("%s:%d found process:%d\n", extra->name, __LINE__, pid);
            return proc;
        }
        else
        {
            RPC_THREAD *thread;
            list_for_each_entry(thread, &proc->threads, list){
                if(thread->pid == pid){
                    pr_debug("%s:%d found thread:%d in process:%d\n", extra->name, __LINE__,
                        pid, proc->pid);
                    return proc;
                }
            }
        }
    }
    pr_err("%s:%d taskID:%lu never shows in thread list\n", extra->name, __LINE__, taskID);
    return NULL;
}

static inline int update_thread_list(RPC_PROCESS *proc)
{
    RPC_THREAD *thread;

    //current thread is main thread or the thread that open this device
    if(current->pid == proc->pid)
        return 0;

    list_for_each_entry(thread, &proc->threads, list){
        //current is already in thread list
        if(thread->pid == current->pid)
            return 0;
    }

    //not found, so add new thread to list
    thread = kmalloc(sizeof(RPC_THREAD), GFP_KERNEL);
    if(thread == NULL){
        pr_err("%s: failed to allocate RPC_THREAD\n", __func__);
        return -ENOMEM;
    }
    pr_debug("%s:%d add thread:%s,%d,%d to thread list of process:%d\n", __func__, __LINE__,
        current->comm, current->pid, current->tgid, proc->pid);
    thread->pid = current->pid;
    list_add(&thread->list, &proc->threads);

    return 1;
}

/*
 * Prototypes for shared functions
 */
#ifdef MY_COPY
int my_copy_user(int *des, int *src, int size);
#endif

int     rpc_poll_init(void);
int     rpc_poll_pause(void);
int     rpc_poll_suspend(void);
int     rpc_poll_resume(void);
void    rpc_poll_cleanup(void);
int     rpc_intr_init(void);
int     rpc_intr_pause(void);
int     rpc_intr_suspend(void);
int     rpc_intr_resume(void);
void    rpc_intr_cleanup(void);

int     rpc_kern_init(void);
int     rpc_kern_pause(void);
int     rpc_kern_suspend(void);
int     rpc_kern_resume(void);
ssize_t rpc_kern_read(int opt, char *buf, size_t count);
ssize_t rpc_kern_write(int opt, const char *buf, size_t count);

#define RPC_DVR_MALLOC      0x1
#define RPC_DVR_FREE        0x2
#define RPC_SIO_INIT        0x3

typedef unsigned long (*FUNC_PTR)(unsigned long, unsigned long);
int register_kernel_rpc(unsigned long command, FUNC_PTR ptr);
int send_rpc_command(int opt, unsigned long command, unsigned long param1, unsigned long param2, unsigned long *retvalue);

extern RPC_POLL_Dev *rpc_poll_devices;
extern RPC_INTR_Dev *rpc_intr_devices;
extern RPC_KERN_Dev *rpc_kern_devices;
extern void __iomem *rpc_int_base;
extern void rpc_set_flag(uint32_t);
#ifdef CONFIG_SND_REALTEK
int RPC_DESTROY_AUDIO_FLOW(int pid);
#endif

/* Use 'k' as magic number */
#define RPC_IOC_MAGIC  'k'

/*
 * S means "Set"        : through a ptr,
 * T means "Tell"       : directly with the argument value
 * G means "Get"        : reply by setting through a pointer
 * Q means "Query"      : response is on the return value
 * X means "eXchange"   : G and S atomically
 * H means "sHift"      : T and Q atomically
 */
#define RPC_IOCTTIMEOUT _IO(RPC_IOC_MAGIC,  0)
#define RPC_IOCQTIMEOUT _IO(RPC_IOC_MAGIC,  1)
#define RPC_IOCTRESET _IO(RPC_IOC_MAGIC,  2)
#ifdef CONFIG_REALTEK_RPC_PROGRAM_REGISTER
#define RPC_IOCTHANDLER _IO(RPC_IOC_MAGIC, 3)
#endif

#define RPC_DBGREG_GET 0
#define RPC_DBGREG_SET 1
#define RPC_IOCTRGETDBGREG_A _IOWR(RPC_IOC_MAGIC,  0x10, RPC_DBG_FLAG)
#endif
