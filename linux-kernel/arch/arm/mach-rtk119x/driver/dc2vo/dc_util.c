#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <asm/io.h> 
#include "dc_rpc.h"


#define USE_ION_MAP

#define IPCREAD(x) IPCRead(&(x))
#define IPCWRITE(x,y) IPCWrite(&(x),y)

extern unsigned int G_LAST_VCTX;

static unsigned long reverseInteger(unsigned long value)
{
	unsigned long b0 = value & 0x000000ff;
	unsigned long b1 = (value & 0x0000ff00) >> 8;
	unsigned long b2 = (value & 0x00ff0000) >> 16;
	unsigned long b3 = (value & 0xff000000) >> 24;

	return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

static unsigned long long reverseLongInteger(unsigned long long value)
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


unsigned long    pli_IPCReadULONG(BYTE* src)
{
  volatile unsigned long A;  // prevent gcc -O3 optimization to create non-atomic access
	if (((int)src & 0x3) != 0)
		printk("error in pli_IPCReadULONG()...\n");
	A = *(unsigned long *)src;
	return reverseInteger(A);
}


unsigned long long  pli_IPCReadULONGLONG(BYTE* src)
{
    volatile unsigned long long A;  // prevent gcc -O3 optimization to create non-atomic access
	if (((int)src & 0x3) != 0)
		printk("error in pli_IPCReadULONG()...\n");

	A = *(unsigned long long*)src;
	return reverseLongInteger(A);
}

void    pli_IPCWriteULONG(BYTE* des, unsigned long data)
{
  volatile unsigned long A;  // prevent gcc -O3 optimization to create non-atomic access
	if (((int)des & 0x3) != 0)
		printk("error in pli_IPCWriteULONG()...\n");

	A = reverseInteger(data);
	*(unsigned long *)des = A;
}
void    pli_IPCWriteULONGLONG(BYTE* des, unsigned long long data)
{
  volatile unsigned long long A; // prevent gcc -O3 optimization to create non-atomic access
	if (((int)des & 0x3) != 0)
		printk("error in pli_IPCWriteULONG()...\n");
	A = reverseLongInteger(data);
	*(unsigned long long*)des = A;
}



void pli_ipcCopyMemory(unsigned char* des, unsigned char* src, unsigned long len)
{
	unsigned long i;
	unsigned long *psrc, *pdes;

	if ((((int)src & 0x3) != 0) || (((int)des & 0x3) != 0) || ((len & 0x3) != 0)) {
		printk("error in pli_IPCCopyMemory() %x %x %lu\n", (unsigned int)(des),
			(unsigned int)(src), len);

	}

	for (i = 0; i < len; i+=4) {
		psrc = (unsigned long *)&src[i];
		pdes = (unsigned long *)&des[i];
		*pdes = reverseInteger(*psrc);
//		printf("%x, %x...\n", src[i], des[i]);
	}
}

static DEFINE_SPINLOCK(lock);
static DEFINE_SPINLOCK(wlock);

int ICQ_WriteCmd (void *cmd, RINGBUFFER_HEADER* rbHeader, void *rbHeaderBase)
{
    //spin_lock_irq(&lock);
#ifdef USE_ION_MAP
    volatile void * base_iomap = rbHeaderBase;
#endif
    unsigned int size  = ((INBAND_CMD_PKT_HEADER *)cmd)->size;
    unsigned int read  = pli_IPCReadULONG((BYTE*)&rbHeader->readPtr[0]) | 0xa0000000;
    unsigned int write = pli_IPCReadULONG((BYTE*)&(rbHeader->writePtr)) | 0xa0000000;
    unsigned int base  = pli_IPCReadULONG((BYTE*)&(rbHeader->beginAddr))| 0xa0000000;
    unsigned int limit = (rbHeader->beginAddr + rbHeader->size);
	limit = pli_IPCReadULONG((BYTE*)&limit) | 0xa0000000;

    if ( read + (read > write ? 0 : limit - base) - write > size )
    {
        //spin_lock_irq(&wlock);
      if ( write + size <= limit ) {
#ifdef USE_ION_MAP
          unsigned int offset = write - base;
          void * write_io = (void*)((unsigned int) base_iomap + offset);
          pli_ipcCopyMemory ((volatile void *)write_io, cmd, size) ;
#else
          pli_ipcCopyMemory ((void *)write, cmd, size) ;
#endif
      }
      else
      {
#ifdef USE_ION_MAP
          unsigned int offset = write - base;
          void * write_io = (void*)((unsigned int) base_iomap + offset);
          pli_ipcCopyMemory ((volatile void *)write_io, cmd, limit - write) ;
          pli_ipcCopyMemory ((volatile void *)base_iomap,  (void *)((unsigned int)cmd + limit - write), size - (limit - write)) ;
#else
          pli_ipcCopyMemory ((void *)write, cmd, limit - write) ;
          pli_ipcCopyMemory ((void *)base,  (void *)((unsigned int)cmd + limit - write), size - (limit - write)) ;
#endif
      }

		write += size ;
		write = write < limit ? write : write - (limit - base) ;
		write =  (0x1FFFFFFF&write);

        #if 0
        { /* writecombine */
            unsigned int offset = write - base;
            volatile int temp_read = * (volatile int *)base_iomap + offset;
        }
        #endif

        //spin_unlock_irq(&wlock);
		rbHeader->writePtr =  pli_IPCReadULONG((BYTE*)&write);

        #if 0
        { /* writecombine */
            volatile int temp_read = rbHeader->writePtr;
        }
        #endif

	  	//printk("new wp:0x%x r:0x%x\n", write, read);

		// sb2 flush
		//*(volatile unsigned int *)0xb801a020;       // sb2 flush
		//__asm__ volatile ("sync");
   	}
	else
	{
		printk("errQ rw:(%x %x) s:%u\n", read, write, size);

        goto err;
	}

    //spin_unlock_irq(&lock);
 	return 0;
err:
    //spin_unlock_irq(&lock);
    return -1;
}

void clkGetPresentation(REFCLOCK* clk, DC_PRESENTATION_INFO *pos)
{
	//printk("dcA 0x%x %llx\n", (unsigned int)clk, clk->videoSystemPTS);
	if( clk != 0)
	{
    		//pos->videoSystemPTS = pli_IPCReadULONGLONG( (BYTE*)&clk->videoSystemPTS );
	    pos->videoContext   = pli_IPCReadULONG( (BYTE*)&clk->videoContext);
	    //pos->videoEndOfSegment = pli_IPCReadULONG( (BYTE*)&clk->videoEndOfSegment);
	}
	else
		pos->videoContext = (unsigned long)-1;

}
void clkResetPresentation(REFCLOCK* clk)
{
	if( clk != 0)
	{
		pli_IPCWriteULONGLONG( (BYTE*)&clk->videoSystemPTS, -1);
    	pli_IPCWriteULONGLONG( (BYTE*)&clk->audioSystemPTS, -1);
    	pli_IPCWriteULONGLONG( (BYTE*)&clk->videoRPTS,      -1);
    	pli_IPCWriteULONGLONG((BYTE*)&clk->audioRPTS,  -1);
    	pli_IPCWriteULONG((BYTE*)&clk->videoContext,(unsigned long)-1);
    	pli_IPCWriteULONG((BYTE*)&clk->audioContext, (unsigned long)-1);
    	pli_IPCWriteULONG((BYTE*)&clk->videoEndOfSegment, (long)-1);
	}

}

