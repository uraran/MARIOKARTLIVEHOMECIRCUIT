#include "hdmirx_rpc.h"
#include <linux/printk.h>
#include <linux/kernel.h>
#include <asm/io.h>

static unsigned long long hdmirx_reverseLongInteger(unsigned long long value)
{
  unsigned long long ret;
  unsigned char *des, *src;
  src = (unsigned char *)&value;
  des = (unsigned char *)&ret;
  des[0] = src[7];
  des[1] = src[6];
  des[2] = src[5];
  des[3] = src[4];
  des[4] = src[3];
  des[5] = src[2];
  des[6] = src[1];
  des[7] = src[0];
  return ret;
}

void    hdmirx_pli_IPCWriteULONGLONG(BYTE* des, unsigned long long data)
{
    volatile unsigned long long A; // prevent gcc -O3 optimization to create non-atomic access
    if (((int)des & 0x3) != 0)
        pr_err("error in pli_IPCWriteULONG()...\n");
    A = hdmirx_reverseLongInteger(data);
    *(unsigned long long*)des = A;
}

unsigned long hdmirx_reverseInteger(unsigned long value)
{
    unsigned long b0 = value & 0x000000ff;
    unsigned long b1 = (value & 0x0000ff00) >> 8;
    unsigned long b2 = (value & 0x00ff0000) >> 16;
    unsigned long b3 = (value & 0xff000000) >> 24;

    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

unsigned int    hdmirx_pli_IPCReadULONG(BYTE* src)
{
  volatile unsigned long A;  // prevent gcc -O3 optimization to create non-atomic access
    if (((int)src & 0x3) != 0)
        pr_err("error in pli_IPCReadULONG()...\n");
    A = *(unsigned long *)src;
    return hdmirx_reverseInteger(A);
}

void    hdmirx_pli_IPCWriteULONG(BYTE* des, unsigned long data)
{
  volatile unsigned long A;  // prevent gcc -O3 optimization to create non-atomic access
    if (((int)des & 0x3) != 0)
        pr_err("error in pli_IPCWriteULONG()...\n");

    A = hdmirx_reverseInteger(data);
    *(unsigned long *)des = A;
}

void hdmirx_pli_ipcCopyMemory(unsigned char* des, unsigned char* src, unsigned long len)
{
    unsigned long i;
    unsigned long *psrc, *pdes;

    if ((((int)src & 0x3) != 0) || (((int)des & 0x3) != 0) || ((len & 0x3) != 0)) {
        pr_err("error in pli_IPCCopyMemory() %x %x %lu\n", (unsigned int)(des),
            (unsigned int)(src), len);

    }

    for (i = 0; i < len; i+=4) {
        psrc = (unsigned long *)&src[i];
        pdes = (unsigned long *)&des[i];
        *pdes = hdmirx_reverseInteger(*psrc);
//      printf("%x, %x...\n", src[i], des[i]);
    }
}

int HDMIRX_ICQ_WriteCmd (void *cmd, RINGBUFFER_HEADER* rbHeader)
{
    static void * base_iomap = NULL;
    unsigned int size  = ((INBAND_CMD_PKT_HEADER *)cmd)->size;

    unsigned int read  = hdmirx_pli_IPCReadULONG((BYTE*)&rbHeader->readPtr[0]);
    unsigned int write = hdmirx_pli_IPCReadULONG((BYTE*)&(rbHeader->writePtr));
    unsigned int base  = hdmirx_pli_IPCReadULONG((BYTE*)&(rbHeader->beginAddr));
    unsigned int limit = (rbHeader->beginAddr + rbHeader->size);
    
    limit = hdmirx_pli_IPCReadULONG((BYTE*)&limit);
    if (base_iomap == NULL && size != 0) {
        base_iomap = ioremap((phys_addr_t)ulPhyAddrFilter(base),rbHeader->size);
    }
    if ( read + (read > write ? 0 : limit - base) - write > size )
    {
      if ( write + size <= limit )
      {
            unsigned int offset = write - base;
        void * write_io = (void*)((unsigned int) base_iomap + offset);
        hdmirx_pli_ipcCopyMemory (write_io, cmd, size) ;
      }
      else
      {
          unsigned int offset = write - base;
          void * write_io = (void*)((unsigned int) base_iomap + offset);
          hdmirx_pli_ipcCopyMemory (write_io, cmd, limit - write) ;
          hdmirx_pli_ipcCopyMemory (base_iomap,  (void *)((unsigned int)cmd + limit - write), size - (limit - write)) ;
      }

        write += size ;
        write = write < limit ? write : write - (limit - base) ;
     // rbHeader->writePtr = (0x1FFFFFFF&write);
        write =  (0x1FFFFFFF&write);
        rbHeader->writePtr =  hdmirx_pli_IPCReadULONG((BYTE*)&write);

        //pr_info("new wp:0x%x r:0x%x\n", write, read);

        // sb2 flush
        //*(volatile unsigned int *)0xb801a020;       // sb2 flush
        //__asm__ volatile ("sync");????
    }
    else
    {
        pr_err("errQ rw:(%x %x) s:%u\n", read, write, size);
        return -1;

    }

    return 0;
}

