#ifndef __KMD_H__
#define __KMD_H__

#include <linux/spinlock.h>
#include <linux/list.h>

typedef u64     MD_CMD_HANDLE;
typedef u64     MD_COPY_HANDLE;


typedef struct
{
    unsigned char       flags;
    MD_CMD_HANDLE       request_id;
    struct list_head    list;
    dma_addr_t          dst;
    size_t              dst_len;
    dma_addr_t          src;
    size_t              src_len;
}MD_COMPLETION;


struct kmd_dev
{    
    void*           CmdBuf;
    void*           CmdBase;
    void*           CmdLimit;
    int             wrptr;
    int             v_to_p_offset;
    int             size;
    uint64_t u64InstCnt;
    uint64_t u64IssuedInstCnt;

    struct rw_semaphore sem;

    struct list_head task_list;
    struct list_head free_task_list;
};


#if 0
#define DBG_PRINT(s, args...) printk(s, ## args)
#define MD_COMPLETION_DBG(fmt, args...) printk("[KMD][CMP] " fmt, ## args)
#else
#define DBG_PRINT(s, args...)
#define MD_COMPLETION_DBG(fmt, args...)
#endif

//#define endian_swap_32(a) (((a)>>24) | (((a)>>8) & 0xFF00) | (((a)<<8) & 0xFF0000) | (((a)<<24) & 0xFF000000))
#define endian_swap_32(a) (a)


///////////////////////////////////////////////////////////////////////
// Low Level API
///////////////////////////////////////////////////////////////////////
MD_CMD_HANDLE kmd_write_command(const char *buf, size_t count);
bool kmd_checkfinish(MD_CMD_HANDLE handle);


///////////////////////////////////////////////////////////////////////
// Copy Request Management APIs
///////////////////////////////////////////////////////////////////////
int kmd_copy_sync(MD_COPY_HANDLE handle);
void kmd_check_copy_tasks(void);


///////////////////////////////////////////////////////////////////////
// Asynchronous APIs
///////////////////////////////////////////////////////////////////////
MD_COPY_HANDLE kmd_copy_start(void* dst, void* src, int len, int dir);
MD_CMD_HANDLE  kmd_memset_start(void* lpDst, u8 data, int len);


///////////////////////////////////////////////////////////////////////
// Synchronous APIs
///////////////////////////////////////////////////////////////////////
int kmd_memcpy(void* lpDst, void* lpSrc, int len, bool forward);
int kmd_memset(void* lpDst, u8 data, int len);


#endif  // __KMD_H__
