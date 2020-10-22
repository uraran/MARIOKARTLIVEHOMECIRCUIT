#ifndef UIO_RTK_AO_H
#define UIO_RTK_AO_H

typedef struct {
    int ALSA_fd;
    int AO_Flash_buf_fd[8];
}FW_IOC_AO_INIT_RING_BUF_FD;

//#define ION_TEST_MEMORY_EN

#define FW_IOC_MAGIC 'k'
#define FW_IOC_INIT_RINGBUF_AO              _IOWR(FW_IOC_MAGIC, 0, FW_IOC_AO_INIT_RING_BUF_FD)
#define FW_IOC_TEST_ION                     _IOWR(FW_IOC_MAGIC, 1, int)
#define AO_IOC_MAXNR 1

void test_ion_get_fd(unsigned int arg);
void rtk_alsa_AO_init_ringbuf(unsigned long pArg);

#endif


