//Copyright (C) 2007-2014 Realtek Semiconductor Corporation.
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/rawmidi.h>
#include <sound/initval.h>
#include <linux/mutex.h>
#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/syscalls.h> /* needed for the _IOW etc stuff used later */
#include <linux/mpage.h>
#include <linux/dcache.h>
//#include <linux/pageremap.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <sound/asound.h>
#include <asm/cacheflush.h>
#include "snd-realtek.h"
#include <linux/dma-mapping.h>
#include <RPCDriver.h>
#include <linux/suspend.h>
#include "snd-realtek-compress.h"

#define ENDIAN_CHANGE(x) ((((x)&0xff000000)>>24)|(((x)&0x00ff0000)>>8)|(((x)&0x0000ff00)<<8)|(((x)&0x000000ff)<<24))
#define SHARE_MEM_SIZE 8
#define LPCM_2CH_16BITS (4)

static int snd_card_playback_open(snd_pcm_substream_t * substream);
static int snd_card_playback_close(snd_pcm_substream_t * substream);
static int snd_card_playback_ioctl(struct snd_pcm_substream *substream,  unsigned int cmd, void *arg);
static int snd_card_hw_params(snd_pcm_substream_t * substream, snd_pcm_hw_params_t * hw_params);
static int snd_card_hw_free(snd_pcm_substream_t * substream);
static int snd_card_playback_prepare(snd_pcm_substream_t * substream);
static int snd_card_playback_trigger(snd_pcm_substream_t * substream, int cmd);
static snd_pcm_uframes_t snd_card_playback_pointer(snd_pcm_substream_t * substream);
static int snd_card_playback_copy(struct snd_pcm_substream *substream, int channel, snd_pcm_uframes_t pos, void __user *buf, snd_pcm_uframes_t count);

static int snd_card_capture_open(snd_pcm_substream_t * substream);
static int snd_card_capture_i2s_open(snd_pcm_substream_t * substream);
static int snd_card_capture_close(snd_pcm_substream_t * substream);
static int snd_card_capture_ioctl(struct snd_pcm_substream *substream,  unsigned int cmd, void *arg);
static int snd_card_capture_prepare(snd_pcm_substream_t * substream);
static int snd_card_capture_trigger(snd_pcm_substream_t * substream, int cmd);
static int snd_card_capture_hw_free(snd_pcm_substream_t * substream);
static snd_pcm_uframes_t snd_card_capture_pointer(snd_pcm_substream_t * substream);

static void snd_card_timer_function(unsigned int data);
static void snd_card_capture_lpcm_timer_function(unsigned int data);
static void snd_card_capture_timer_function(unsigned int data);

static int valid_free_size(long base, long limit, long rp, long wp);
static long ring_add(long ring_base, long ring_limit, long ptr, int bytes);
static long ring_valid_data(long ring_base, long ring_limit, long ring_rp, long ring_wp);
static long buf_memcpy2_ring(long base, long limit, long ptr, char* buf, long size);
static int snd_realtek_hw_free_ring (snd_pcm_runtime_t *runtime);
static int snd_RTK_volume_info(snd_kcontrol_t * kcontrol, snd_ctl_elem_info_t * uinfo);
static int snd_RTK_volume_get(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol);
static int snd_RTK_volume_put(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol);
static int snd_RTK_capsrc_info(snd_kcontrol_t * kcontrol, snd_ctl_elem_info_t * uinfo);
static int snd_RTK_capsrc_get(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol);
static int snd_RTK_capsrc_put(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol);
//const char *snd_pcm_access_name_func(snd_pcm_access_t access);

static int snd_realtek_hw_capture_free_ring (snd_pcm_runtime_t *runtime);
static long snd_card_get_ring_data(RINGBUFFER_HEADER *pRing_BE, RINGBUFFER_HEADER *pRing_LE);
static void ring1_to_ring2_general(AUDIO_RINGBUF_PTR* ring1, AUDIO_RINGBUF_PTR* ring2, long size, char* dmem_buf);

/************************************************************************/
/* GLOBAL                                                                     */
/************************************************************************/
static struct platform_device *devices[SNDRV_CARDS];
static int snd_card_enable[SNDRV_CARDS] = {1, [1 ... (SNDRV_CARDS - 1)] = 0};    // only one sound card
static int pcm_substreams[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 8};
static int pcm_capture_substreams[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 1};
static snd_card_t *snd_RTK_cards[SNDRV_CARDS] = SNDRV_DEFAULT_PTR;
static int snd_open_count = 0;
static int snd_open_ai_count = 0;
static char *snd_pcm_id[] = {"RTK_SndCard_PCM_ADC", "RTK_SndCard_PCM_HDMI"}; // 2 PCM instances in 1 sound card
static unsigned int rtk_dec_ao_buffer = RTK_DEC_AO_BUFFER_SIZE;

static spinlock_t playback_lock;
static spinlock_t capture_lock;
static long dec_out_msec = 0;
static long *g_ShareMemPtr = NULL;

static bool is_suspend = false;

#ifdef USE_ION_AUDIO_HEAP
struct ion_client *alsa_client;
static struct ion_handle *alsa_playback_handle;
static struct ion_handle *dec_out_handle[8];
static struct ion_handle *sharemem_handle;
static struct ion_handle *alsa_capture_handle;
static struct ion_handle *enc_in_handle[2];
static struct ion_handle *enc_lpcm_handle;

static ion_phys_addr_t g_ShareMemPtr_dat;
#else
static dma_addr_t g_ShareMemPtr_dat;
#endif

static snd_kcontrol_new_t snd_mars_controls[] = {
MARS_VOLUME("Master Volume", 0, MIXER_ADDR_MASTER),
MARS_CAPSRC("Master Capture Switch", 0, MIXER_ADDR_MASTER),
MARS_VOLUME("Synth Volume", 0, MIXER_ADDR_SYNTH),
//MARS_CAPSRC("Synth Capture Switch", 0, MIXER_ADDR_MASTER),
//MARS_VOLUME("Line Volume", 0, MIXER_ADDR_LINE),
//MARS_CAPSRC("Line Capture Switch", 0, MIXER_ADDR_MASTER),
//MARS_VOLUME("Mic Volume", 0, MIXER_ADDR_MIC),
//MARS_CAPSRC("Mic Capture Switch", 0, MIXER_ADDR_MASTER),
//MARS_VOLUME("CD Volume", 0, MIXER_ADDR_CD),
//MARS_CAPSRC("CD Capture Switch", 0, MIXER_ADDR_MASTER)
};

static snd_pcm_ops_t snd_card_rtk_playback_ops = {
    .open =         snd_card_playback_open,
    .close =        snd_card_playback_close,
    .ioctl =        snd_card_playback_ioctl,
    .hw_params =    snd_card_hw_params,
    .hw_free =      snd_card_hw_free,
    .prepare =      snd_card_playback_prepare,
    .trigger =      snd_card_playback_trigger,
    .pointer =      snd_card_playback_pointer,
//    .wall_clock =   NULL,
#ifdef USE_COPY_OPS
    .copy =         snd_card_playback_copy,
#else
//    .copy =         NULL,
#endif
//    .silence =      NULL,
//    .page =         NULL,
//    .mmap =         NULL,
//    .ack =          NULL,
};

// for I2S in
static snd_pcm_ops_t snd_card_rtk_capture_i2s_ops = {
    .open =         snd_card_capture_i2s_open,
    .close =        snd_card_capture_close,
    .ioctl =        snd_card_capture_ioctl,
    .hw_params =    snd_card_hw_params,
    .hw_free =      snd_card_capture_hw_free,
    .prepare =      snd_card_capture_prepare,
    .trigger =      snd_card_capture_trigger,
    .pointer =      snd_card_capture_pointer,
};


static snd_pcm_ops_t snd_card_rtk_capture_ops = {
    .open =         snd_card_capture_open,
    .close =        snd_card_capture_close,
    .ioctl =        snd_card_capture_ioctl,
    .hw_params =    snd_card_hw_params,
    .hw_free =      snd_card_capture_hw_free,
    .prepare =      snd_card_capture_prepare,
    .trigger =      snd_card_capture_trigger,
    .pointer =      snd_card_capture_pointer,
};

static snd_pcm_ops_t snd_card_rtk_capture_HDMIRX_ops = {
    .open =         snd_card_capture_open,
    .close =        snd_card_capture_close,
    .ioctl =        snd_card_capture_ioctl,
    .hw_params =    snd_card_hw_params,
    .hw_free =      snd_card_capture_hw_free,
    .prepare =      snd_card_capture_prepare,
    .trigger =      snd_card_capture_trigger,
    .pointer =      snd_card_capture_pointer,
};

static snd_pcm_hardware_t snd_card_playback =
{
    .info               = RTK_DMP_INFO,
    .formats            = RTK_DMP_FORMATS,
    .rates              = RTK_DMP_RATES,
    .rate_min           = RTK_DMP_RATE_MIN,
    .rate_max           = RTK_DMP_RATE_MAX,
    .channels_min       = RTK_DMP_CHANNELS_MIN,
    .channels_max       = RTK_DMP_CHANNELS_MAX,
    .buffer_bytes_max   = RTK_DMP_MAX_BUFFER_SIZE,
    .period_bytes_min   = RTK_DMP_MIN_PERIOD_SIZE,
    .period_bytes_max   = RTK_DMP_MAX_PERIOD_SIZE,
    .periods_min        = RTK_DMP_PERIODS_MIN,
    .periods_max        = RTK_DMP_PERIODS_MAX,
    .fifo_size          = RTK_DMP_FIFO_SIZE,
};

static snd_pcm_hardware_t snd_card_mars_capture =
{
    .info               = RTK_DMP_INFO,
    .formats            = RTK_DMP_FORMATS,
    .rates              = RTK_DMP_RATES,
    .rate_min           = RTK_DMP_RATE_MIN,
    .rate_max           = RTK_DMP_RATE_MAX,
    .channels_min       = RTK_DMP_CHANNELS_MIN,
    .channels_max       = RTK_DMP_CHANNELS_MAX,
    .buffer_bytes_max   = RTK_DMP_MAX_BUFFER_SIZE,
    .period_bytes_min   = RTK_DMP_MIN_PERIOD_SIZE,
    .period_bytes_max   = RTK_DMP_CAPTURE_MAX_PERIOD_SIZE,
    .periods_min        = RTK_DMP_PERIODS_MIN,
    .periods_max        = RTK_DMP_PERIODS_MAX,
    .fifo_size          = RTK_DMP_FIFO_SIZE,
};

/************************************************************************/
/* MODULE                                                                     */
/************************************************************************/
module_param_array(snd_card_enable, bool, NULL, 0444);
MODULE_PARM_DESC(snd_card_enable, "Enable this mars soundcard.");
module_param_array(pcm_substreams, int, NULL, 0444);
MODULE_PARM_DESC(pcm_substreams, "PCM substreams # (1-16) for mars driver.");

/************************************************************************/
/* FUNCTION                                                                     */
/************************************************************************/
long snd_monitor_audio_data_queue(void)
{
    long ret = 0;
    if(g_ShareMemPtr)
    {
        ret = *g_ShareMemPtr;
        ret = ENDIAN_CHANGE(ret);
    }
    else
    {
        //ALSA_WARNING("NO exist share memory !!\n");
    }

    //ALSA_VitalPrint("audiofw total latency %d (dec_out_msec %d ao_out_msec %d)\n", ret + dec_out_msec, dec_out_msec, ret);
    ret += dec_out_msec;

    return ret;
}

static int snd_realtek_hw_check_audio_ready(void)
{
    wait_queue_head_t     waitQueue;        /* for blocking read */
    long remain_time;

    //TRACE_CODE("[%s %s %d]\n", __FILE__, __FUNCTION__, __LINE__);

    // Initialize wait queue...
    init_waitqueue_head(&waitQueue);

    do {
        if (RPC_TOAGENT_CHECK_AUDIO_READY() == 0)
        {
            //ALSA_VitalPrint("[ALSA Audio ready .....]\n");
            return 0;   // success
        }
        ALSA_WARNING("Audio NOT ready !!]\n");
        remain_time = interruptible_sleep_on_timeout(&waitQueue, HZ);
    } while(remain_time==0);

    return 1;   // fail
}

static int snd_realtek_hw_capture_init_ringheader_of_AI(snd_pcm_runtime_t *runtime)
{
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;
    RINGBUFFER_HEADER *pAIRingHeader = dpcm->nAIRing;
    RINGBUFFER_HEADER *pAIRingHeader_LE = dpcm->nAIRing_LE;
    AUDIO_RPC_RINGBUFFER_HEADER nRingHeader;
    int ch;

    // init ring header
    for(ch = 0; ch < runtime->channels; ++ch)
    {
//        pAIRingHeader_LE[ch].beginAddr = (unsigned long)dpcm->pAIRingData[ch];
        pAIRingHeader_LE[ch].beginAddr = (unsigned long)dpcm->phy_pAIRingData[ch];
        pAIRingHeader_LE[ch].size = dpcm->nRingSize;
        pAIRingHeader_LE[ch].readPtr[0] = pAIRingHeader_LE[ch].beginAddr;
        pAIRingHeader_LE[ch].writePtr = pAIRingHeader_LE[ch].beginAddr;
        pAIRingHeader_LE[ch].numOfReadPtr = 1;

        pAIRingHeader[ch].beginAddr = htonl((unsigned long)pAIRingHeader_LE[ch].beginAddr);
        pAIRingHeader[ch].size = htonl(pAIRingHeader_LE[ch].size);
        pAIRingHeader[ch].readPtr[0] = htonl(pAIRingHeader_LE[ch].readPtr[0]);
        pAIRingHeader[ch].writePtr = htonl(pAIRingHeader_LE[ch].writePtr);
        pAIRingHeader[ch].numOfReadPtr = htonl(pAIRingHeader_LE[ch].numOfReadPtr);

        //nRingHeader.pRingBufferHeaderList[ch] = (unsigned int) (pAIRingHeader + ch);
        nRingHeader.pRingBufferHeaderList[ch] = (unsigned int) (pAIRingHeader + ch) - (unsigned long)dpcm + dpcm->phy_addr;

//        ALSA_VitalPrint("[ALSA ringHeader %x %x]\n", ch, nRingHeader.pRingBufferHeaderList[ch]);
        ALSA_DbgPrint("[ALSA %d ring %x %x %x %x]\n", ch, (unsigned int)pAIRingHeader_LE[ch].beginAddr
            , (unsigned int)pAIRingHeader_LE[ch].size, (unsigned int)pAIRingHeader_LE[ch].readPtr[0]
            , (unsigned int)pAIRingHeader_LE[ch].writePtr);
    }

    // init AI ring header
    nRingHeader.instanceID = dpcm->AIAgentID;
    nRingHeader.pinID = PCM_OUT;
    nRingHeader.readIdx = -1;
    nRingHeader.listSize =  runtime->channels;

    // RPC set AI ring header
    if (RPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&nRingHeader, nRingHeader.listSize) < 0) {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -1;
    };

    return 0;
}

static int snd_realtek_hw_capture_init_LPCM_ringheader_of_AI(snd_pcm_runtime_t *runtime)
{
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;
    RINGBUFFER_HEADER *pAIRingHeader = &dpcm->nLPCMRing;
    RINGBUFFER_HEADER *pAIRingHeader_LE = &dpcm->nLPCMRing_LE;
    AUDIO_RPC_RINGBUFFER_HEADER nRingHeader;

    // init ring header
    //pAIRingHeader_LE->beginAddr = (unsigned long)dpcm->pLPCMData;
    pAIRingHeader_LE->beginAddr = (unsigned long)dpcm->phy_pLPCMData;
    pAIRingHeader_LE->size = dpcm->nLPCMRingSize;
    pAIRingHeader_LE->readPtr[0] = pAIRingHeader_LE->beginAddr;
    pAIRingHeader_LE->writePtr = pAIRingHeader_LE->beginAddr;
    pAIRingHeader_LE->numOfReadPtr = 1;

    pAIRingHeader->beginAddr = htonl((unsigned long)pAIRingHeader_LE->beginAddr);
    pAIRingHeader->size = htonl(pAIRingHeader_LE->size);
    pAIRingHeader->readPtr[0] = htonl(pAIRingHeader_LE->readPtr[0]);
    pAIRingHeader->writePtr = htonl(pAIRingHeader_LE->writePtr);
    pAIRingHeader->numOfReadPtr = htonl(pAIRingHeader_LE->numOfReadPtr);

    //nRingHeader.pRingBufferHeaderList[0] = (unsigned int) pAIRingHeader;
    nRingHeader.pRingBufferHeaderList[0] = (unsigned int) pAIRingHeader - (unsigned long)dpcm + dpcm->phy_addr;

    // init AI ring header
    nRingHeader.instanceID = dpcm->AIAgentID;
    nRingHeader.pinID = BASE_BS_OUT;
    nRingHeader.readIdx = -1;
    nRingHeader.listSize = 1;

    // RPC set AI ring header
    //if (RPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&nRingHeader, runtime->channels) < 0) {
    if (RPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&nRingHeader, nRingHeader.listSize) < 0) {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -1;
    };

    return 0;
}

// init ringheader of decoder_outring and AO_inring
static int snd_realtek_hw_init_ringheader_of_DEC_AO(snd_pcm_runtime_t *runtime)
{
    //ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);

    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
    RINGBUFFER_HEADER *pAORingHeader = dpcm->decOutRing;
    AUDIO_RPC_RINGBUFFER_HEADER nRingHeader;
    int ch, j;

    // init ring header
    //for(ch = 0 ; ch < 8 ; ++ch)
    for(ch = 0 ; ch < runtime->channels ; ++ch)
    {
        //pAORingHeader[ch].beginAddr = htonl((unsigned long)dpcm->decOutData[ch]);
        pAORingHeader[ch].beginAddr = htonl((unsigned long)dpcm->phy_decOutData[ch]);
        pAORingHeader[ch].size = htonl(dpcm->nRingSize);
        for(j = 0 ; j < 4 ; ++j)
            pAORingHeader[ch].readPtr[j] = pAORingHeader[ch].beginAddr;
        pAORingHeader[ch].writePtr = pAORingHeader[ch].beginAddr;
        pAORingHeader[ch].numOfReadPtr = htonl(1);
    }

    // init DEC outring header
    nRingHeader.instanceID = dpcm->DECAgentID;
    nRingHeader.pinID = PCM_OUT;
    //nRingHeader.pRingBufferHeaderList[0] = (unsigned int) &dpcm->decOutRing[0];
    //nRingHeader.pRingBufferHeaderList[1] = (unsigned int) &dpcm->decOutRing[1];
    for(ch = 0 ; ch < runtime->channels ; ++ch)
        nRingHeader.pRingBufferHeaderList[ch] = (unsigned long)&dpcm->decOutRing[ch] - (unsigned long)dpcm + dpcm->phy_addr;
    nRingHeader.readIdx = -1;
    nRingHeader.listSize =  runtime->channels;

    // RPC set DEC outring header
    //if (RPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&nRingHeader, runtime->channels) < 0) {
    if (RPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&nRingHeader, nRingHeader.listSize) < 0) {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -1;
    };

    // init AO inring header
    nRingHeader.instanceID = dpcm->AOAgentID;
    nRingHeader.pinID = dpcm->AOpinID;
    //nRingHeader.pRingBufferHeaderList[0] = (unsigned int) &dpcm->decOutRing[0];
    //nRingHeader.pRingBufferHeaderList[1] = (unsigned int) &dpcm->decOutRing[1];
    for(ch = 0 ; ch < runtime->channels ; ++ch)
        nRingHeader.pRingBufferHeaderList[ch] = (unsigned long)&dpcm->decOutRing[ch] - (unsigned long)dpcm + dpcm->phy_addr;
    nRingHeader.readIdx = 0;
    nRingHeader.listSize =  runtime->channels;

    // RPC set AO inring header
    //if (RPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&nRingHeader, runtime->channels) < 0) {
    if (RPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&nRingHeader, nRingHeader.listSize) < 0) {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -1;
    };

    return 0;
}

// connet decoder and AO by RPC
static int snd_realtek_hw_init_connect_decoder_ao(snd_card_RTK_pcm_t *dpcm)
{
    AUDIO_RPC_CONNECTION nConnection;

    nConnection.desInstanceID = dpcm->AOAgentID;
    nConnection.srcInstanceID = dpcm->DECAgentID;
    nConnection.srcPinID = PCM_OUT;
    nConnection.desPinID = dpcm->AOpinID;

    if (RPC_TOAGENT_CONNECT_SVC(&nConnection)) {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -1;
    };
    return 0;
}

// 1. init decoder in_ring
// 2. init decoder inband ring
static int snd_realtek_hw_init_decoder_ring(snd_pcm_runtime_t *runtime) {
    //ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);

    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
    AUDIO_RPC_RINGBUFFER_HEADER ringbuf_header;

    // init HW inring header
    //printk("dma buffer (vir)address %lx\n", (unsigned long) runtime->dma_area);
    //printk("dma buffer (phy)address %lx\n", (unsigned long) runtime->dma_addr);
    //printk("dma buffer size         %d\n", (unsigned long) runtime->dma_bytes);
    //dpcm->decInRing[0].beginAddr = htonl(((unsigned int) runtime->dma_area));
    dpcm->decInRing[0].beginAddr = htonl(((unsigned int) runtime->dma_addr));
    dpcm->decInRing[0].bufferID = htonl(RINGBUFFER_STREAM);
    dpcm->decInRing[0].size = htonl(frames_to_bytes(runtime, runtime->buffer_size));

    dpcm->decInRing[0].writePtr = dpcm->decInRing[0].beginAddr;
    dpcm->decInRing[0].readPtr[0] = dpcm->decInRing[0].beginAddr;
    dpcm->decInRing[0].readPtr[1]  = dpcm->decInRing[0].beginAddr;
    dpcm->decInRing[0].readPtr[2]  = dpcm->decInRing[0].beginAddr;
    dpcm->decInRing[0].readPtr[3]  = dpcm->decInRing[0].beginAddr;
    dpcm->decInRing[0].numOfReadPtr = htonl(1);

    // init HW inring (little endian)
    dpcm->decInRing_LE[0].beginAddr = ((unsigned int)runtime->dma_addr);
    dpcm->decInRing_LE[0].size = frames_to_bytes(runtime, runtime->buffer_size);
    dpcm->decInRing_LE[0].writePtr = dpcm->decInRing_LE[0].beginAddr;
    dpcm->decInRing_LE[0].readPtr[0] = dpcm->decInRing_LE[0].beginAddr;
    dpcm->decInRing_LE[0].readPtr[1] = dpcm->decInRing_LE[0].beginAddr;
    dpcm->decInRing_LE[0].readPtr[2] = dpcm->decInRing_LE[0].beginAddr;
    dpcm->decInRing_LE[0].readPtr[3] = dpcm->decInRing_LE[0].beginAddr;

    // init RPC ring header
    ringbuf_header.instanceID = dpcm->DECAgentID;
    ringbuf_header.pinID = BASE_BS_IN;
    //ringbuf_header.pRingBufferHeaderList[0] = (unsigned long)(&dpcm->decInRing[0]);
    ringbuf_header.pRingBufferHeaderList[0] = (unsigned long)dpcm->decInRing - (unsigned long)dpcm + dpcm->phy_addr;
    ringbuf_header.readIdx = 0;
    ringbuf_header.listSize = 1;

    // RPC set decoder in_ring
    //if (RPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&ringbuf_header, runtime->channels))
    if (RPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&ringbuf_header, ringbuf_header.listSize))
    {
        ALSA_WARNING("[fail %s %d]\n", __FUNCTION__, __LINE__);
        return -1;
    };

    // init inband ring header
    dpcm->decInbandRing.beginAddr = htonl((unsigned long)dpcm->decInbandData - (unsigned long)dpcm + dpcm->phy_addr);
    dpcm->decInbandRing.size = htonl(sizeof(dpcm->decInbandData));
    dpcm->decInbandRing.readPtr[0] = dpcm->decInbandRing.beginAddr;
    dpcm->decInbandRing.writePtr = dpcm->decInbandRing.beginAddr;
    dpcm->decInbandRing.numOfReadPtr = htonl(1);

    // init RPC ring header
    ringbuf_header.instanceID = dpcm->DECAgentID;
    ringbuf_header.pinID = INBAND_QUEUE;
    ringbuf_header.pRingBufferHeaderList[0] = (unsigned long)&dpcm->decInbandRing - (unsigned long)dpcm + dpcm->phy_addr; //physical
    ringbuf_header.readIdx = 0;
    ringbuf_header.listSize = 1;

    //if (RPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&ringbuf_header, runtime->channels))
    if (RPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&ringbuf_header, ringbuf_header.listSize))
    {
        ALSA_WARNING("[fail %s %d]\n", __FUNCTION__, __LINE__);
        return -1;
    };
    return 0;
}

static int snd_realtek_hw_capture_run(snd_card_RTK_capture_pcm_t *dpcm)
{
    // AI pause
    if (RPC_TOAGENT_PAUSE_SVC(dpcm->AIAgentID))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -1;
    }

    // AI run
    if(RPC_TOAGENT_RUN_SVC(dpcm->AIAgentID))
    {
        ALSA_WARNING("[%s %d]\n", __FUNCTION__, __LINE__);
        return -1;
    }

    return 0;
}

static int snd_realtek_hw_resume(snd_card_RTK_pcm_t *dpcm)
{
    //TRACE_CODE("[%s %s %d]\n", __FILE__, __FUNCTION__, __LINE__);

    // decoder run & AO run
    if(RPC_TOAGENT_RUN_SVC(dpcm->DECAgentID))
    {
        ALSA_WARNING("[%s %d]\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if(RPC_TOAGENT_RUN_SVC((dpcm->AOAgentID | dpcm->AOpinID)))
    {
        ALSA_WARNING("[%s %d]\n", __FUNCTION__, __LINE__);
        return -1;
    }

    return 0;
}

// ring is big-endian
int snd_realtek_hw_ring_write(RINGBUFFER_HEADER* ring, void *data, int len, unsigned long offset)
{
    unsigned long base, limit, wp;

    //Convert to virtual address
    base = (unsigned long)(ntohl(ring->beginAddr)) + offset;
    limit = base + ntohl(ring->size);
    wp = (unsigned long)(ntohl(ring->writePtr)) + offset;
    ALSA_VitalPrint("base %x limit %x wp %x \n", (long)base, (long)limit, (long)wp);
    //TRACE_CODE("base %x limit %x wp %x \n", (long)base, (long)limit, (long)wp);
    wp = buf_memcpy2_ring((long)base, (long)limit, (long)wp, (char *)data, (long)len);
    ring->writePtr = htonl(wp - offset); // record physical address
    return len;
}

// AO pause
// decoder pause
// decoder run
// decoder flush
// write decoder info into inband of decoder
static int snd_realtek_hw_init_decoder_info(snd_pcm_runtime_t *runtime)
{
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
    AUDIO_DEC_NEW_FORMAT cmd;
    AUDIO_RPC_SENDIO sendio;

    //TRACE_CODE("[%s %s %d]\n", __FILE__, __FUNCTION__, __LINE__);

    // AO pause
    if (RPC_TOAGENT_PAUSE_SVC(dpcm->AOAgentID | dpcm->AOpinID))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -1;
    }

    // decoder pause
    //if (RPC_TOAGENT_PAUSE_SVC(dpcm->DECAgentID))
    if (RPC_TOAGENT_STOP_SVC(dpcm->DECAgentID))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -1;
    }

    // decoder flush
    sendio.instanceID = dpcm->DECAgentID;
    sendio.pinID = dpcm->DECpinID;
    if (RPC_TOAGENT_FLUSH_SVC(&sendio))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -1;
    }

    // decoder run
    if (RPC_TOAGENT_RUN_SVC(dpcm->DECAgentID))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -1;
    }

    cmd.audioType = htonl(AUDIO_LPCM_DECODER_TYPE);
    cmd.header.type = htonl(AUDIO_DEC_INBAMD_CMD_TYPE_NEW_FORMAT);
    cmd.header.size = htonl(sizeof(AUDIO_DEC_NEW_FORMAT));
    cmd.privateInfo[0]  = htonl(runtime->channels);
    cmd.privateInfo[1]  = htonl(runtime->sample_bits);
    cmd.privateInfo[2]  = htonl(runtime->rate);
    cmd.privateInfo[3]  = htonl(0);
    cmd.privateInfo[4]  = htonl(0);  // (a_format->privateData[0]&0x1)<<4 : float or not
    cmd.privateInfo[5]  = htonl(0);
    cmd.privateInfo[6]  = htonl(0);  // (a_format->privateData[0]&0x1)<<4 : float or not
    cmd.privateInfo[7]  = htonl(0);

    switch (runtime->format)
    {
        case SNDRV_PCM_FORMAT_S16_BE:
        case SNDRV_PCM_FORMAT_U16_BE:
        case SNDRV_PCM_FORMAT_S24_BE:
        case SNDRV_PCM_FORMAT_U24_BE:
        case SNDRV_PCM_FORMAT_S32_BE:
        case SNDRV_PCM_FORMAT_U32_BE:
        case SNDRV_PCM_FORMAT_S24_3BE:
        case SNDRV_PCM_FORMAT_U24_3BE:
        case SNDRV_PCM_FORMAT_S20_3BE:
        case SNDRV_PCM_FORMAT_U20_3BE:
        case SNDRV_PCM_FORMAT_S18_3BE:
        case SNDRV_PCM_FORMAT_U18_3BE:
        case SNDRV_PCM_FORMAT_FLOAT_BE:
        case SNDRV_PCM_FORMAT_FLOAT64_BE:
        case SNDRV_PCM_FORMAT_IEC958_SUBFRAME_BE:
            cmd.privateInfo[7]  = htonl(AUDIO_BIG_ENDIAN);
            break;

        case SNDRV_PCM_FORMAT_S8:
        case SNDRV_PCM_FORMAT_U8:
        case SNDRV_PCM_FORMAT_S16_LE:
        case SNDRV_PCM_FORMAT_U16_LE:
        case SNDRV_PCM_FORMAT_S24_LE:
        case SNDRV_PCM_FORMAT_U24_LE:
        case SNDRV_PCM_FORMAT_S32_LE:
        case SNDRV_PCM_FORMAT_U32_LE:
        case SNDRV_PCM_FORMAT_S24_3LE:
        case SNDRV_PCM_FORMAT_U24_3LE:
        case SNDRV_PCM_FORMAT_S20_3LE:
        case SNDRV_PCM_FORMAT_U20_3LE:
        case SNDRV_PCM_FORMAT_S18_3LE:
        case SNDRV_PCM_FORMAT_U18_3LE:
        case SNDRV_PCM_FORMAT_FLOAT_LE:
        case SNDRV_PCM_FORMAT_FLOAT64_LE:
        case SNDRV_PCM_FORMAT_IEC958_SUBFRAME_LE:
        case SNDRV_PCM_FORMAT_MU_LAW:
        case SNDRV_PCM_FORMAT_A_LAW:
        case SNDRV_PCM_FORMAT_IMA_ADPCM:
        case SNDRV_PCM_FORMAT_MPEG:
        case SNDRV_PCM_FORMAT_GSM:
        case SNDRV_PCM_FORMAT_SPECIAL:
        default:
            cmd.privateInfo[7]  = htonl(AUDIO_LITTLE_ENDIAN);
            break;
    }

    cmd.wPtr = dpcm->decInRing[0].beginAddr;
    //printk("[test] dpcm->decInRing[0].beginAddr %lx\n" , cmd.wPtr); //decoder buffer ring physical address

    //snd_realtek_hw_ring_write(((unsigned long)dpcm->decInbandRing - (unsigned long)dpcm + dpcm->phy_addr), &cmd, sizeof(AUDIO_DEC_NEW_FORMAT));
    snd_realtek_hw_ring_write(&dpcm->decInbandRing, &cmd, sizeof(AUDIO_DEC_NEW_FORMAT), (unsigned long)dpcm - (unsigned long)dpcm->phy_addr);

    //ALSA_VitalPrint("[ALSA LPCM ch %d sample_bit %d sample rate %d]\n", runtime->channels, runtime->sample_bits, runtime->rate);
    return 0;
}

// malloc ring buffer for AI.
static int snd_realtek_hw_capture_malloc_ring(snd_pcm_runtime_t *runtime)
{
    //ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);

    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;
    int ch , bMallocSuccess = 1;

    for(ch = 0 ; ch < runtime->channels ; ++ch)
    {
        if(dpcm->pAIRingData[ch])
        {
            ALSA_WARNING("[re-malloc error !!! %s %d]\n", __FUNCTION__, __LINE__);
#ifdef USE_ION_AUDIO_HEAP
            if(alsa_client != NULL && enc_in_handle[ch] != NULL)
            {
                ion_unmap_kernel(alsa_client, enc_in_handle[ch]);
                ion_free(alsa_client, enc_in_handle[ch]);
                enc_in_handle[ch] = NULL;
            }
#else
            dma_free_coherent(NULL, RTK_ENC_AI_BUFFER_SIZE, dpcm->pAIRingData[ch], dpcm->phy_pAIRingData[ch]);
#endif
            dpcm->pAIRingData[ch] = NULL;
        }

#ifdef USE_ION_AUDIO_HEAP
        ion_phys_addr_t dat;
        size_t len;

        //printk("%s create enc_in_handle[%d]\n", __FUNCTION__, ch);
        enc_in_handle[ch] = ion_alloc(alsa_client, RTK_ENC_AI_BUFFER_SIZE, 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, AUDIO_ION_FLAG);

        if (IS_ERR(enc_in_handle[ch])) {
             ALSA_WARNING("[%s %d ion_alloc fail]\n", __FUNCTION__, __LINE__);
             return bMallocSuccess;
        }

        if(ion_phys(alsa_client, enc_in_handle[ch], &dat, &len) != 0)
        {
            snd_realtek_hw_capture_free_ring(runtime);
            ALSA_WARNING("[malloc ch %d fail and terminate %s %d]\n", ch, __FUNCTION__, __LINE__);
            return bMallocSuccess;
        }

        dpcm->phy_pAIRingData[ch] = dat;
        dpcm->pAIRingData[ch] = ion_map_kernel(alsa_client, enc_in_handle[ch]);
        ALSA_VitalPrint("[ALSA ch %x phy %x vir %x]\n", ch, dpcm->phy_pAIRingData[ch], dpcm->pAIRingData[ch]);
#else
        void *addr = 0;
        addr = dma_alloc_coherent(NULL, RTK_ENC_AI_BUFFER_SIZE, &dpcm->phy_pAIRingData[ch], GFP_KERNEL);
        if(!addr)
        {
            snd_realtek_hw_capture_free_ring(runtime);
            printk("[malloc ch %d fail and terminate %s %d]\n", ch, __FUNCTION__, __LINE__);
            return bMallocSuccess;
        }
        dpcm->pAIRingData[ch] = addr;
#endif
        dpcm->nRingSize = RTK_ENC_AI_BUFFER_SIZE;
    }

    bMallocSuccess = 0; //Success
    return bMallocSuccess;
}

static int snd_realtek_hw_capture_malloc_lpcm_ring(snd_pcm_runtime_t *runtime)
{
    //ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);

    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;

    int bMallocSuccess = 1;

#ifdef USE_ION_AUDIO_HEAP
    ion_phys_addr_t dat;
    size_t len;

    //printk("%s create enc_lpcm_handle\n", __FUNCTION__);
    enc_lpcm_handle = ion_alloc(alsa_client, RTK_ENC_LPCM_BUFFER_SIZE, 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, AUDIO_ION_FLAG);

    if (IS_ERR(enc_lpcm_handle)) {
        ALSA_WARNING("[%s %d ion_alloc fail]\n", __FUNCTION__, __LINE__);
        return bMallocSuccess;
    }

    if(ion_phys(alsa_client, enc_lpcm_handle, &dat, &len) != 0)
    {
        printk("[malloc fail and terminate %s %d]\n", __FUNCTION__, __LINE__);
        return bMallocSuccess;
    }

    dpcm->phy_pLPCMData= dat;
    dpcm->pLPCMData = ion_map_kernel(alsa_client, enc_lpcm_handle);
    ALSA_VitalPrint("[ALSA LPCM phy %x vir %x]\n", dpcm->phy_pLPCMData, dpcm->pLPCMData);
#else
    void *addr = 0;
    addr = dma_alloc_coherent(NULL, RTK_ENC_LPCM_BUFFER_SIZE, &dpcm->phy_pLPCMData,  GFP_KERNEL);
    if(!addr)
    {
        printk("[malloc fail and terminate %s %d]\n", __FUNCTION__, __LINE__);
        return bMallocSuccess;
    }
    dpcm->pLPCMData = addr;
#endif
    dpcm->nLPCMRingSize = RTK_ENC_LPCM_BUFFER_SIZE;

    bMallocSuccess = 0; //Success
    return bMallocSuccess;
}

// in decoder-AO path, malloc AO in_ring
static int snd_realtek_hw_malloc_ring(snd_pcm_runtime_t *runtime)
{
    //ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);

    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
    int ch, bMallocSuccess = 1;

    for(ch = 0 ; ch < runtime->channels ; ++ch)
    {
        if(dpcm->decOutData[ch])
        {
            printk("[re-malloc error !!! %s %d]\n", __FUNCTION__, __LINE__);
#ifdef USE_ION_AUDIO_HEAP
            if(alsa_client != NULL && dec_out_handle[ch] != NULL)
            {
                ion_unmap_kernel(alsa_client, dec_out_handle[ch]);
                ion_free(alsa_client, dec_out_handle[ch]);
                dec_out_handle[ch] = NULL;
            }
#else
            dma_free_coherent(NULL, rtk_dec_ao_buffer, dpcm->decOutData[ch], dpcm->phy_decOutData[ch]);
#endif
            dpcm->decOutData[ch] = NULL;
        }

#ifdef USE_ION_AUDIO_HEAP
        ion_phys_addr_t dat;
        size_t len;

        //printk("%s create dec_out_handle[%d]\n", __FUNCTION__, ch);
        dec_out_handle[ch] = ion_alloc(alsa_client, rtk_dec_ao_buffer, 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, AUDIO_ION_FLAG);

        if (IS_ERR(dec_out_handle[ch])) {
            ALSA_WARNING("[%s %d ion_alloc fail]\n", __FUNCTION__, __LINE__);
            return bMallocSuccess;
        }

        if(ion_phys(alsa_client, dec_out_handle[ch], &dat, &len) != 0)
        {
            snd_realtek_hw_free_ring(runtime);
            printk("[malloc ch %d fail and terminate %s %d]\n", ch, __FUNCTION__, __LINE__);
            return bMallocSuccess;
        }

        dpcm->phy_decOutData[ch] = dat;
        dpcm->decOutData[ch] = ion_map_kernel(alsa_client, dec_out_handle[ch]);
#else
        void *addr = 0;

        addr = dma_alloc_coherent(NULL, rtk_dec_ao_buffer, &dpcm->phy_decOutData[ch], GFP_KERNEL);
        if(!addr)
        {
            snd_realtek_hw_free_ring(runtime);
            printk("[malloc ch %d fail and terminate %s %d]\n", ch, __FUNCTION__, __LINE__);
            return bMallocSuccess;
        }

        dpcm->decOutData[ch] = addr;
#endif

        dpcm->nRingSize = rtk_dec_ao_buffer;
        //printk("[test]AO Inring buffer ch %d (vir)address %x (phy)address %x\n", ch, dpcm->decOutData[ch], dpcm->phy_decOutData[ch]);
    }

    bMallocSuccess = 0; //Success
    return bMallocSuccess;
}

static int snd_realtek_hw_create_AI(snd_pcm_substream_t *substream)
{
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;

    if (snd_open_ai_count >= MAX_AI_DEVICES)
    {
        ALSA_WARNING("[too more AI %s %d]\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if(RPC_TOAGENT_CREATE_AI_AGENT(dpcm))
    {
        ALSA_WARNING("[err %s %d]\n", __FUNCTION__, __LINE__);
        return -1;
    }

    snd_open_ai_count ++;
    return 0;
}

// get AO flash pin ID
static int snd_realtek_hw_open(snd_pcm_substream_t *substream)
{
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;

    //TRACE_CODE("[%s %s %d]\n", __FILE__, __FUNCTION__, __LINE__);

    if (snd_open_count >= MAX_PCM_SUBSTREAMS) {
        ALSA_WARNING("[opened audio stream count excess %d]\n", MAX_PCM_SUBSTREAMS);
        return -1;
    }

    // get ao flash pin ID
    if ((dpcm->AOpinID = RPC_TOAGENT_GET_AO_FLASH_PIN(dpcm)) < 0) {
        ALSA_WARNING("[can't get flash pin %s %d]\n", __FUNCTION__, __LINE__);
        return -1;
    }

    // init volume
    dpcm->volume = 31;

    snd_open_count ++;
    ALSA_VitalPrint("[ALSA Open AO AgentID = %d pinID = %d]\n", dpcm->AOAgentID, dpcm->AOpinID);
    return 0;
}

// check if AO exists
// return 0(success), 1(fail)
static int snd_realtek_hw_init(snd_card_RTK_pcm_t *dpcm)
{
    //TRACE_CODE("[%s %s %d]\n", __FILE__, __FUNCTION__, __LINE__);

    if (dpcm->bHWinit != 0)
        return 0;

    if (snd_realtek_hw_check_audio_ready())
    {
        return -1;
    }

    if (RPC_TOAGENT_CREATE_AO_AGENT(&dpcm->AOAgentID, AUDIO_ALSA_OUT))
    {
        ALSA_WARNING("[No AO agent %s %d]\n", __FUNCTION__, __LINE__);
        return -1;
    }
    dpcm->bHWinit = 1;

    return 0;
}

static int snd_realtek_hw_close(snd_card_RTK_pcm_t *dpcm)
{
    AUDIO_RPC_RINGBUFFER_HEADER RBuf;
    int ret = 0;
    int i;
    int channel_max = 8;

    /* re-initialize ring buffer with null ring */
    RBuf.instanceID = dpcm->AOAgentID;
    RBuf.pinID = dpcm->AOpinID;
    RBuf.readIdx = 0;
    RBuf.listSize = 0;
    for ( i = 0; i < channel_max; i++)
        RBuf.pRingBufferHeaderList[i] = 0;

    /* stop AO */
#ifdef AO_ON_SCPU
    ret = KRPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&RBuf, channel_max);
#else
    ret = RPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&RBuf, channel_max);
#endif

    dec_out_msec = 0;
    return ret;
}

static int snd_realtek_hw_capture_free_ring (snd_pcm_runtime_t *runtime)
{
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;
    int ch;

    TRACE_CODE("[ALSA %s %d]\n", __FUNCTION__, __LINE__);

    for (ch = 0; ch < runtime->channels; ch++)
    {
        if(dpcm->pAIRingData[ch] != NULL)
        {
#ifdef USE_ION_AUDIO_HEAP
            if(alsa_client != NULL && enc_in_handle[ch] != NULL)
            {
                ion_unmap_kernel(alsa_client, enc_in_handle[ch]);
                ion_free(alsa_client, enc_in_handle[ch]);
                enc_in_handle[ch] = NULL;
            }
#else
            dma_free_coherent(NULL, RTK_ENC_AI_BUFFER_SIZE, dpcm->pAIRingData[ch], dpcm->phy_pAIRingData[ch]);
#endif
            dpcm->pAIRingData[ch] = NULL;
        }
    }

    //if(IF_ALSA_CAPTURE_LPCM(dpcm))
    if(dpcm->nAIFormat == AUDIO_ALSA_FORMAT_16BITS_LE_LPCM ||
       dpcm->nAIFormat == AUDIO_ALSA_FORMAT_24BITS_LE_LPCM)
    {
#ifdef USE_ION_AUDIO_HEAP
        if(alsa_client != NULL && enc_lpcm_handle != NULL)
        {
            ion_unmap_kernel(alsa_client, enc_lpcm_handle);
            ion_free(alsa_client, enc_lpcm_handle);
            enc_lpcm_handle = NULL;
        }
#else
        dma_free_coherent(NULL, RTK_ENC_LPCM_BUFFER_SIZE, dpcm->pLPCMData, dpcm->phy_pLPCMData);
#endif
    }
    return 0;
}

static int snd_realtek_hw_free_ring (snd_pcm_runtime_t *runtime)
{
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
    int ch;

    //ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);

    //for (ch = 0; ch < 8; ch++)
    for (ch = 0; ch < runtime->channels; ch++)
    {
        if(dpcm->decOutData[ch] != NULL)
        {
#ifdef USE_ION_AUDIO_HEAP
            if(alsa_client != NULL && dec_out_handle[ch] != NULL)
            {
                ion_unmap_kernel(alsa_client, dec_out_handle[ch]);
                ion_free(alsa_client, dec_out_handle[ch]);
                dec_out_handle[ch] = NULL;
            }
#else
            dma_free_coherent(NULL, rtk_dec_ao_buffer, dpcm->decOutData[ch], dpcm->phy_decOutData[ch]);
#endif
            dpcm->decOutData[ch] = NULL;
        }
    }
    //TRACE_CODE("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
    return 0;
}

static void snd_card_runtime_free(snd_pcm_runtime_t *runtime)
{
    ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
#ifndef USE_ION_AUDIO_HEAP
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
#endif

    //snd_realtek_hw_free_ring(runtime);
#ifdef USE_ION_AUDIO_HEAP
    if(alsa_client != NULL && alsa_playback_handle != NULL)
    {
        //printk("%s free alsa_playback_handle\n", __FUNCTION__);
        ion_unmap_kernel(alsa_client, alsa_playback_handle);
        ion_free(alsa_client, alsa_playback_handle);
        alsa_playback_handle = NULL;
    }
#else
    dma_free_coherent(NULL, 4096, dpcm, dpcm->phy_addr);
#endif

    runtime->private_data = NULL;
}

static void snd_card_capture_runtime_free(snd_pcm_runtime_t *runtime)
{
    ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
#ifndef USE_ION_AUDIO_HEAP
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;
#endif

    //snd_realtek_hw_capture_free_ring(runtime);
#ifdef USE_ION_AUDIO_HEAP
    if(alsa_client != NULL && alsa_capture_handle != NULL)
    {
        //printk("%s free alsa_capture_handle\n", __FUNCTION__);
        ion_unmap_kernel(alsa_client, alsa_capture_handle);
        ion_free(alsa_client, alsa_capture_handle);
        alsa_capture_handle = NULL;
    }
#else
    dma_free_coherent(NULL, 4096, dpcm, dpcm->phy_addr);
#endif
    runtime->private_data = NULL;
}

//static void snd_realtek_pcm_elapsed(unsigned long dpcm)
//{
////    TRACE_CODE("[%s %s %d]\n", __FILE__, __FUNCTION__, __LINE__);
//  snd_pcm_period_elapsed(((snd_card_RTK_pcm_t *)dpcm)->substream);
//  ((snd_card_RTK_pcm_t *)dpcm)->bElapsedTaskletFinish = 1;
//}

#if 0
static void snd_realtek_hw_eos_work(struct work_struct *work)
{
    snd_card_RTK_pcm_t *dpcm = container_of(work, snd_card_RTK_pcm_t, nEOSWork);

    //TRACE_CODE("[%s %s %d]\n", __FILE__, __FUNCTION__, __LINE__);

    if (RPC_TOAGENT_INBAND_EOS_SVC(dpcm) < 0)
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return;
    }

    dpcm->nEOSState = SND_REALTEK_EOS_STATE_FINISH;
}
#endif

static int snd_card_playback_open(snd_pcm_substream_t * substream)
{
    ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_pcm_t *dpcm = NULL;
    int ret = ENOMEM;

#if 0
    if(is_suspend)
    {
        ALSA_VitalPrint("[ALSA %s %d] suspend\n", __FUNCTION__, __LINE__);
        return ret;
    }
#endif

#ifdef USE_ION_AUDIO_HEAP
    ion_phys_addr_t dat;
    size_t len;

    //printk("%s create alsa_playback_handle\n", __FUNCTION__);
    alsa_playback_handle = ion_alloc(alsa_client, sizeof(snd_card_RTK_pcm_t), 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, AUDIO_ION_FLAG);

    if (IS_ERR(alsa_playback_handle)) {
        ALSA_WARNING("[%s %d ion_alloc fail]\n", __FUNCTION__, __LINE__);
        goto fail;
    }

    if(ion_phys(alsa_client, alsa_playback_handle, &dat, &len) != 0) {
        printk("[%s %d] alloc memory faild\n", __FUNCTION__, __LINE__);
        goto fail;
    }

    dpcm = ion_map_kernel(alsa_client, alsa_playback_handle);
    memset(dpcm, 0, len);
    dpcm->phy_addr = dat;
#else
    dma_addr_t dat;
    void *p;

    p = dma_alloc_coherent(NULL, sizeof(snd_card_RTK_pcm_t), &dat, GFP_KERNEL);
    if(!p)
    {
        printk("[%s %d] alloc memory faild\n", __FUNCTION__, __LINE__);
        goto fail;
    }

    dpcm = p;
    dpcm->phy_addr = dat;
#endif

    // private data
    runtime->private_data = dpcm;
    runtime->private_free = snd_card_runtime_free;

    // check if AO exists
    if (snd_realtek_hw_init(dpcm))
    {
        ALSA_WARNING("[error %s %d]\n", __FUNCTION__, __LINE__);
        ret = -ENOMEDIUM;
        goto fail;
    }

    memcpy(&runtime->hw, &snd_card_playback, sizeof(struct snd_pcm_hardware));
    dpcm->substream = substream;
    dpcm->nEOSState = SND_REALTEK_EOS_STATE_NONE;
    if (snd_realtek_hw_open(substream) < 0)
    {
        ALSA_WARNING("[error %s %d]\n", __FUNCTION__, __LINE__);
        ret = -ENOMEM;
        goto fail;
    }

    dpcm->card = (RTK_snd_card_t *) (substream->pcm->card->private_data);

    // init dec_out_msec
    dec_out_msec = 0;

    // init timer
    init_timer(&dpcm->nStartTimer);
    dpcm->nStartTimer.data = (unsigned long) substream;
    dpcm->nStartTimer.function = (void (*) (unsigned long))snd_card_timer_function;

    // init
    //INIT_WORK(&dpcm->nEOSWork, snd_realtek_hw_eos_work);

    //spin_lock_init(&dpcm->pcm_nLock);
    spin_lock_init(&playback_lock);

    // not sure...
    //snd_pcm_hw_constraint_minmax(runtime, SNDRV_PCM_HW_PARAM_PERIOD_TIME, 40000/4 , 1000000/4);
    //snd_pcm_hw_constraint_minmax(runtime, SNDRV_PCM_HW_PARAM_BUFFER_TIME, 40000 , 1000000);

    dpcm->card->mixer_volume[MIXER_ADDR_MASTER][0] = dpcm->card->mixer_volume[MIXER_ADDR_MASTER][1] = RPC_TOAGENT_GET_VOLUME(dpcm);

    ret = 0;

    if(g_ShareMemPtr == NULL)
    {
#ifdef USE_ION_AUDIO_HEAP
        //printk("%s create sharemem_handle\n", __FUNCTION__);
        sharemem_handle = ion_alloc(alsa_client, SHARE_MEM_SIZE, 32, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, AUDIO_ION_FLAG);

        if (IS_ERR(sharemem_handle)) {
            ALSA_WARNING("[%s %d ion_alloc fail]\n", __FUNCTION__, __LINE__);
            goto fail;
        }

        if(ion_phys(alsa_client, sharemem_handle, &g_ShareMemPtr_dat, &len) != 0)
        {
            printk("[%s %d] alloc memory faild\n", __FUNCTION__, __LINE__);
            goto fail;
        }

        g_ShareMemPtr = ion_map_kernel(alsa_client, sharemem_handle);
        //printk("phy %p vir %p\n", g_ShareMemPtr_dat, g_ShareMemPtr);
#else
        g_ShareMemPtr = dma_alloc_coherent(NULL, SHARE_MEM_SIZE, &g_ShareMemPtr_dat, GFP_KERNEL);
        if(!g_ShareMemPtr)
        {
            printk("[%s %d] alloc memory faild\n", __FUNCTION__, __LINE__);
            goto fail;
        }
        //printk("g_ShareMemPtr %p\n", g_ShareMemPtr);
#endif
        memset(g_ShareMemPtr, 0, SHARE_MEM_SIZE);
        RPC_TOAGENT_PUT_SHARE_MEMORY(g_ShareMemPtr_dat);
    }

    return ret;
fail:

#ifdef USE_ION_AUDIO_HEAP
    if(alsa_client != NULL && alsa_playback_handle != NULL)
    {
        //printk("%s free alsa_playback_handle\n", __FUNCTION__);
        ion_unmap_kernel(alsa_client, alsa_playback_handle);
        ion_free(alsa_client, alsa_playback_handle);
        alsa_playback_handle = NULL;
        dpcm = NULL;
    }
#else
    if(p)
    {
        dma_free_coherent(NULL, sizeof(snd_card_RTK_pcm_t), p, dat);
        dpcm = NULL;
    }
#endif

    return ret;
}

static int snd_card_capture_open(snd_pcm_substream_t * substream)
{
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_capture_pcm_t *dpcm = NULL;
    int ret = ENOMEM;

    ALSA_VitalPrint("[ALSA %s %s %d %s]\n",substream->pcm->name, __FUNCTION__, __LINE__, __TIME__);

#ifdef USE_ION_AUDIO_HEAP
    ion_phys_addr_t dat;
    size_t len;

    if(alsa_capture_handle)
    {
        ALSA_WARNING("ERR more than 1 capture instance!!!\n");
    }

    alsa_capture_handle = ion_alloc(alsa_client, sizeof(snd_card_RTK_capture_pcm_t), 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, AUDIO_ION_FLAG);

    if (IS_ERR(alsa_capture_handle)) {
        ALSA_WARNING("[%s %d ion_alloc fail]\n", __FUNCTION__, __LINE__);
        goto fail;
    }

    if(ion_phys(alsa_client, alsa_capture_handle, &dat, &len) != 0) {
        printk("[%s %d] alloc memory faild\n", __FUNCTION__, __LINE__);
        goto fail;
    }

    dpcm = ion_map_kernel(alsa_client, alsa_capture_handle);
    memset(dpcm, 0, len);
    dpcm->phy_addr = dat;
#else
    dma_addr_t dat;
    void *p;

    p = dma_alloc_coherent(NULL, sizeof(snd_card_RTK_pcm_t), &dat, GFP_KERNEL);
    if (!p)
    {
        printk("[%s %d] alloc memory faild\n", __FUNCTION__, __LINE__);
        goto fail;
    }

    dpcm = p;
    dpcm->phy_addr = dat;
#endif
    // private data
    runtime->private_data = dpcm;
    runtime->private_free = snd_card_capture_runtime_free;

    memcpy(&runtime->hw, &snd_card_mars_capture, sizeof(struct snd_pcm_hardware));
    dpcm->substream = substream;

    // create AI
    if (snd_realtek_hw_create_AI(substream) < 0)
    {
        ALSA_WARNING("[error %s %d]\n", __FUNCTION__, __LINE__);
        ret = -ENOMEM;
        goto fail;
    }

    dpcm->card = (RTK_snd_card_t *)(substream->pcm->card->private_data);

    // init timer
    init_timer(&dpcm->nStartTimer);
    dpcm->nStartTimer.data = (unsigned long) substream;

    // init todo
//  INIT_WORK(&dpcm->nEOSWork, snd_realtek_hw_eos_work);

    //spin_lock_init(&dpcm->nLock);
    spin_lock_init(&capture_lock);

    ALSA_VitalPrint("[ALSA %s END]\n", __FUNCTION__);

    ret = 0;

    return ret;
fail:

#ifdef USE_ION_AUDIO_HEAP
    if(alsa_client != NULL && alsa_capture_handle != NULL)
    {
        //printk("%s free alsa_capture_handle\n", __FUNCTION__);
        ion_unmap_kernel(alsa_client, alsa_capture_handle);
        ion_free(alsa_client, alsa_capture_handle);
        alsa_capture_handle = NULL;
        dpcm = NULL;
    }
#else
    if(p)
    {
        dma_free_coherent(NULL, sizeof(snd_card_RTK_capture_pcm_t), p, dat);
        dpcm = NULL;
    }
#endif

    return ret;
}

static int snd_card_capture_i2s_open(snd_pcm_substream_t * substream)
{
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_capture_pcm_t *dpcm = NULL;
    int ret = ENOMEM;

    ALSA_VitalPrint("[ALSA %s %s %d %s]\n",substream->pcm->name, __FUNCTION__, __LINE__, __TIME__);

#ifdef USE_ION_AUDIO_HEAP
    ion_phys_addr_t dat;
    size_t len;

    if(alsa_capture_handle)
    {
        ALSA_WARNING("ERR more than 1 capture instance!!!\n");
    }

    alsa_capture_handle = ion_alloc(alsa_client, sizeof(snd_card_RTK_capture_pcm_t), 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, AUDIO_ION_FLAG);

    if (IS_ERR(alsa_capture_handle)) {
        ALSA_WARNING("[%s %d ion_alloc fail]\n", __FUNCTION__, __LINE__);
        goto fail;
    }

    if(ion_phys(alsa_client, alsa_capture_handle, &dat, &len) != 0) {
        printk("[%s %d] alloc memory faild\n", __FUNCTION__, __LINE__);
        goto fail;
    }

    dpcm = ion_map_kernel(alsa_client, alsa_capture_handle);
    memset(dpcm, 0, len);
    dpcm->phy_addr = dat;
#else
    dma_addr_t dat;
    void *p;

    p = dma_alloc_coherent(NULL, sizeof(snd_card_RTK_pcm_t), &dat, GFP_KERNEL);
    if (!p)
    {
        printk("[%s %d] alloc memory faild\n", __FUNCTION__, __LINE__);
        goto fail;
    }

    dpcm = p;
    dpcm->phy_addr = dat;
#endif
    // private data
    runtime->private_data = dpcm;
    runtime->private_free = snd_card_capture_runtime_free;

    memcpy(&runtime->hw, &snd_card_mars_capture, sizeof(struct snd_pcm_hardware));
    dpcm->substream = substream;

    // create AI
    if (snd_realtek_hw_create_AI(substream) < 0)
    {
        ALSA_WARNING("[error %s %d]\n", __FUNCTION__, __LINE__);
        ret = -ENOMEM;
        goto fail;
    }

    // config I2S
    RPC_TOAGENT_AI_CONFIG_I2S_IN(dpcm);

    dpcm->card = (RTK_snd_card_t *)(substream->pcm->card->private_data);

    // init timer
    init_timer(&dpcm->nStartTimer);
    dpcm->nStartTimer.data = (unsigned long) substream;

    // init todo
//  INIT_WORK(&dpcm->nEOSWork, snd_realtek_hw_eos_work);

    //spin_lock_init(&dpcm->nLock);
    spin_lock_init(&capture_lock);

    ALSA_VitalPrint("[ALSA %s END]\n", __FUNCTION__);

    ret = 0;

    return ret;
fail:

#ifdef USE_ION_AUDIO_HEAP
    if(alsa_client != NULL && alsa_capture_handle != NULL)
    {
        //printk("%s free alsa_capture_handle\n", __FUNCTION__);
        ion_unmap_kernel(alsa_client, alsa_capture_handle);
        ion_free(alsa_client, alsa_capture_handle);
        alsa_capture_handle = NULL;
        dpcm = NULL;
    }
#else
    if(p)
    {
        dma_free_coherent(NULL, sizeof(snd_card_RTK_capture_pcm_t), p, dat);
        dpcm = NULL;
    }
#endif

    return ret;
}


static int snd_card_capture_close(snd_pcm_substream_t * substream)
{
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;
    int ret = -1;
    ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);

    if(snd_open_ai_count)
    {
        // AI pause
        if (RPC_TOAGENT_PAUSE_SVC(dpcm->AIAgentID))
        {
            ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
            goto exit;
        }

        // AI stop
        if (RPC_TOAGENT_STOP_SVC(dpcm->AIAgentID))
        {
            ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
            goto exit;
        }

        // AI destroy
        if (RPC_TOAGENT_DESTROY_SVC(dpcm->AIAgentID))
        {
            ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
            goto exit;
        }
    }

    ret = 0;
exit:

    snd_realtek_hw_capture_free_ring(runtime);

    if(snd_open_ai_count > 0)
        snd_open_ai_count--;
    return ret;
}

static int snd_card_playback_close(snd_pcm_substream_t * substream)
{
    ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
    AUDIO_RPC_SENDIO sendio;
    int ret = 0;

#if 0
    if(is_suspend)
    {
        ALSA_VitalPrint("[ALSA %s %d] suspend\n", __FUNCTION__, __LINE__);
        return ret;
    }
#endif

    if(dpcm->AOpinID)
    {
#ifndef AO_ON_SCPU
        /* stop decoder */
        if(dpcm->DECAgentID)
            RPC_TOAGENT_STOP_SVC(dpcm->DECAgentID);
#endif

        // AO pause
        if (RPC_TOAGENT_PAUSE_SVC(dpcm->AOAgentID | dpcm->AOpinID))
        {
            ret = -1;
            goto exit;
        }

        if(dpcm->bInitRing != 0)
        {
            // decoder flush
#ifdef AO_ON_SCPU
            sendio.instanceID = dpcm->AOAgentID;
            sendio.pinID = dpcm->AOpinID;
#else
            sendio.instanceID = dpcm->DECAgentID;
            sendio.pinID = dpcm->DECpinID;
#endif
            if (RPC_TOAGENT_FLUSH_SVC(&sendio))
            {
                ret = -1;
                goto exit;
            }

            snd_realtek_hw_close(dpcm);
        }

#ifndef AO_ON_SCPU
        /* destroy decoder instance if exist */
        if(dpcm->DECAgentID)
            RPC_TOAGENT_DESTROY_SVC(dpcm->DECAgentID);
#endif
    }
    else
    {
        return -1;
    }

exit:
    RPC_TOAGENT_RELEASE_AO_FLASH_PIN(dpcm);
    snd_open_count--;

    snd_realtek_hw_free_ring(runtime);

    if(g_ShareMemPtr && snd_open_count == 0)
    {
        RPC_TOAGENT_PUT_SHARE_MEMORY(NULL);
#ifdef USE_ION_AUDIO_HEAP
        if(alsa_client != NULL && sharemem_handle != NULL)
        {
            //printk("%s destory sharemem_handle\n", __FUNCTION__);
            ion_unmap_kernel(alsa_client, sharemem_handle);
            ion_free(alsa_client, sharemem_handle);
            sharemem_handle = NULL;
        }
#else
        dma_free_coherent(NULL, 4096, g_ShareMemPtr, g_ShareMemPtr_dat);
#endif
        g_ShareMemPtr = NULL;
    }

    return ret;
}

static int snd_card_capture_ioctl(struct snd_pcm_substream *substream,  unsigned int cmd, void *arg)
{
//  snd_pcm_runtime_t *runtime = substream->runtime;
//  snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;

    TRACE_CODE("[%s %s %d cmd %d]\n", __FILE__, __FUNCTION__, __LINE__, cmd);
    ALSA_VitalPrint("[%s %d cmd %d]\n",__FUNCTION__, __LINE__, cmd);

    switch (cmd)
    {
        case SNDRV_PCM_IOCTL_VOLUME_SET:
            break;
        case SNDRV_PCM_IOCTL_VOLUME_GET:
            break;
        default:
            return snd_pcm_lib_ioctl(substream, cmd, arg);
            break;
    }
    return 0;
}

static int snd_card_playback_ioctl(struct snd_pcm_substream *substream,  unsigned int cmd, void *arg)
{
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;

    //TRACE_CODE("[%s %s %d cmd %d]\n", __FILE__, __FUNCTION__, __LINE__, cmd);

    switch (cmd)
    {
        case SNDRV_PCM_IOCTL_VOLUME_SET:
            ALSA_VitalPrint("[ALSA SET VOLUME]\n");
            get_user(dpcm->volume, (int *) arg);
            if (dpcm->volume < 0)
                dpcm->volume = 0;
            if (dpcm->volume > 31)
                dpcm->volume = 31;
            RPC_TOAGENT_SET_AO_FLASH_VOLUME(substream);
            break;
        case SNDRV_PCM_IOCTL_VOLUME_GET:
            put_user(dpcm->volume, (int *) arg);
            break;
        case SNDRV_PCM_IOCTL_GET_LATENCY:
        {
            int mLatency = 0;
            mLatency = (int)snd_monitor_audio_data_queue();
            put_user(mLatency, (int *) arg);
            break;
        }
        case SNDRV_PCM_IOCTL_GET_FW_DELAY:
        {
            int mLatency = 0;
            snd_pcm_sframes_t n = 0;
            mLatency = snd_monitor_audio_data_queue();
            n = (mLatency * runtime->rate) / 1000;
            put_user(n, (snd_pcm_sframes_t *) arg);
            break;
        }

        default:
            return snd_pcm_lib_ioctl(substream, cmd, arg);
            break;
    }
    return 0;
}

static snd_pcm_uframes_t snd_card_capture_pointer(snd_pcm_substream_t * substream)
{
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;
    snd_pcm_uframes_t ret = 0;

    ret = dpcm->nTotalWrite % runtime->buffer_size;
    return ret;
}

// update runtime->status->hw_ptr
static snd_pcm_uframes_t snd_card_playback_pointer(snd_pcm_substream_t * substream)
{
    //ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
    snd_pcm_uframes_t ret = 0;

    ret = dpcm->nTotalRead % runtime->buffer_size;

//    TRACE_CODE("[%s %s %d %d]\n",__FILE__, __FUNCTION__, __LINE__, ret);
    return ret;
}

static int snd_card_capture_prepare_32bits_BE(snd_pcm_substream_t * substream)
{
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;

    TRACE_CODE("[ @ %s %d]\n", __FUNCTION__, __LINE__);
    // malloc AI ring buf
    if(snd_realtek_hw_capture_malloc_ring(runtime))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // init AI ring header
    if(snd_realtek_hw_capture_init_ringheader_of_AI(runtime))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // private info
    if(RPC_TOAGENT_AI_CONNECT_ALSA(dpcm))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // configure
    if(RPC_TOAGENT_CONFIGURE_AI_HW(runtime))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // run
    if(snd_realtek_hw_capture_run(dpcm))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    dpcm->nStartTimer.function = (void (*) (unsigned long))snd_card_capture_timer_function;

    return 0;
}

static int snd_card_capture_prepare_LPCM(snd_pcm_substream_t * substream)
{
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;

    TRACE_CODE("[ @ %s %d]\n", __FUNCTION__, __LINE__);
    // malloc AI ring buf
    if(snd_realtek_hw_capture_malloc_ring(runtime))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // malloc LPCM ring buf
    if(snd_realtek_hw_capture_malloc_lpcm_ring(runtime))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // init ring header of AI
    if(snd_realtek_hw_capture_init_ringheader_of_AI(runtime))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // init LPCM ring header of AI
    if(snd_realtek_hw_capture_init_LPCM_ringheader_of_AI(runtime))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // private info
    if(RPC_TOAGENT_AI_CONNECT_ALSA(dpcm))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // configure
    if(RPC_TOAGENT_CONFIGURE_AI_HW(runtime))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // run
    if(snd_realtek_hw_capture_run(dpcm))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    dpcm->nStartTimer.function = (void (*) (unsigned long))snd_card_capture_lpcm_timer_function;

    return 0;
}

static int snd_card_capture_prepare(snd_pcm_substream_t * substream)
{
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;

    ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
    if(runtime->status->state == SNDRV_PCM_STATE_XRUN)
    {
        ALSA_WARNING("[SNDRV_PCM_STATE_XRUN appl_ptr %d hw_ptr %d]\n", (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr);
    }

    dpcm->nPeriodJiffies = HZ / (runtime->rate / runtime->period_size);
    if(dpcm->nPeriodJiffies == 0)
        dpcm->nPeriodJiffies = 1;

    ALSA_VitalPrint("\n\n\n");
    ALSA_VitalPrint("[capture : rate %d channels %d format %x\n", runtime->rate, runtime->channels, runtime->format);
    ALSA_VitalPrint("   period_size %d periods %d\n", (int)runtime->period_size, (int)runtime->periods);
    ALSA_VitalPrint("   buffer_size %d period_jiffies %d\n", (int)runtime->buffer_size, (int)dpcm->nPeriodJiffies);
    ALSA_VitalPrint("   start_threshold %d stop_threshold %d]\n", (int)runtime->start_threshold, (int)runtime->stop_threshold);
//    ALSA_VitalPrint("   access %s\n", snd_pcm_access_name_func(runtime->access));
    ALSA_VitalPrint("\n\n\n");

    switch (runtime->access)
    {
        case SNDRV_PCM_ACCESS_MMAP_INTERLEAVED:
        case SNDRV_PCM_ACCESS_RW_INTERLEAVED:
            switch(runtime->format)
            {
                case SNDRV_PCM_FORMAT_S16_LE:
                    TRACE_CODE("[SNDRV_PCM_FORMAT_S16_LE]\n");
                    dpcm->nAIFormat = AUDIO_ALSA_FORMAT_16BITS_LE_LPCM;
                    break;
                case SNDRV_PCM_FORMAT_S24_LE:
                    TRACE_CODE("[SNDRV_PCM_FORMAT_S24_LE]\n");
                    ALSA_WARNING("[ALSA write .wav header err]\n");
                    ALSA_VitalPrint("[ALSA byte #32 of .wav should be 8=>6 manually]\n");
                    dpcm->nAIFormat = AUDIO_ALSA_FORMAT_24BITS_LE_LPCM;
                    break;
                default:
                    ALSA_WARNING("[unsupport format %d %s %d]\n", runtime->format
                        , __FUNCTION__, __LINE__);
                    return -1;
            }
            break;

        case SNDRV_PCM_ACCESS_MMAP_NONINTERLEAVED:
        case SNDRV_PCM_ACCESS_RW_NONINTERLEAVED:
        default:
            ALSA_WARNING("[unsupport access @ %s %d]\n", __FUNCTION__, __LINE__);
            //ALSA_WARNING("[unsupport access %s %s %d]\n", snd_pcm_access_name_func(runtime->access), __FUNCTION__, __LINE__);
            return -1;
    }

#if 0
    dpcm->nAIFormat = AUDIO_ALSA_FORMAT_32BITS_BE_PCM ;
#endif

    if(dpcm->bInitRing)
    {
//        ALSA_CAPTURE_TODO();
        dpcm->nTotalWrite = 0;
        ALSA_WARNING("[Re-Prepare %d %d %s %d]\n", (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr, __FUNCTION__, __LINE__);
        return 0;
    }

    dpcm->nPeriodBytes = frames_to_bytes(runtime, runtime->period_size);
    dpcm->nFrameBytes = frames_to_bytes(runtime, 1);
    substream->ops->silence = NULL;
    substream->ops->copy = NULL;

    switch(dpcm->nAIFormat)
    {
        case AUDIO_ALSA_FORMAT_32BITS_BE_PCM:
            if(snd_card_capture_prepare_32bits_BE(substream))
            {
                ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
                return -ENOMEM;
            }
            break;
        case AUDIO_ALSA_FORMAT_16BITS_LE_LPCM:
        case AUDIO_ALSA_FORMAT_24BITS_LE_LPCM:
            if(snd_card_capture_prepare_LPCM(substream))
            {
                ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
                return -ENOMEM;
            }
            break;
        default:
            ALSA_WARNING("[ALSA err %s %d]\n", __FUNCTION__, __LINE__);
            break;
    }

    dpcm->bInitRing = 1;

    return 0;
}

#ifdef AO_ON_SCPU
static int snd_realtek_hw_init_ringheader_of_AO(snd_pcm_runtime_t *runtime)
{
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
    RINGBUFFER_HEADER *pAORingHeader = dpcm->decOutRing;
    RINGBUFFER_HEADER *pAORingHeader_LE = dpcm->decOutRing_LE;
    AUDIO_RPC_RINGBUFFER_HEADER nKRingHeader;
    int ch, j;

    memset(&nKRingHeader, 0, sizeof(nKRingHeader));
    for(ch = 0 ; ch < runtime->channels ; ++ch)
    {
        //pAORingHeader[ch].beginAddr = htonl((unsigned long)dpcm->decOutData[ch]);
        pAORingHeader_LE[ch].beginAddr = ((unsigned long)dpcm->phy_decOutData[ch]);
        pAORingHeader_LE[ch].size = (dpcm->nRingSize);
        for(j = 0 ; j < 4 ; ++j)
            pAORingHeader_LE[ch].readPtr[j] = pAORingHeader_LE[ch].beginAddr;
        pAORingHeader_LE[ch].writePtr = pAORingHeader_LE[ch].beginAddr;
        pAORingHeader_LE[ch].numOfReadPtr = (1);

        pAORingHeader[ch].beginAddr = htonl(pAORingHeader_LE[ch].beginAddr);
        pAORingHeader[ch].size = htonl(pAORingHeader_LE[ch].size);
        for(j = 0 ; j < 4 ; ++j)
            pAORingHeader[ch].readPtr[j] = htonl(pAORingHeader_LE[ch].readPtr[j]);
        pAORingHeader[ch].writePtr = htonl(pAORingHeader_LE[ch].writePtr);
        pAORingHeader[ch].numOfReadPtr = htonl(pAORingHeader_LE[ch].numOfReadPtr);
    }

    // init AO inring header
    nKRingHeader.instanceID = dpcm->AOAgentID;
    nKRingHeader.pinID = dpcm->AOpinID;
    nKRingHeader.readIdx = 0;
    nKRingHeader.listSize =  runtime->channels;

    KRPC_TOAGENT_INITRINGBUFFER_HEADER_SVC(&nKRingHeader, nKRingHeader.listSize);
    return 0;
}

static int snd_card_playback_prepare(snd_pcm_substream_t * substream)
{
    ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;

    if(is_suspend)
    {
        //ALSA_VitalPrint("[ALSA %s %d] suspend\n", __FUNCTION__, __LINE__);
        return 0;
    }

    if(runtime->status->state == SNDRV_PCM_STATE_XRUN)
    {
        ALSA_WARNING("[SNDRV_PCM_STATE_XRUN appl_ptr %d hw_ptr %d]\n", (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr);
    }

    dpcm->nPeriodJiffies = HZ / (runtime->rate / runtime->period_size);
    if(dpcm->nPeriodJiffies == 0)
        dpcm->nPeriodJiffies = 1;

    if(dpcm->bInitRing)
    {
        dpcm->nHWReadSize = 0;
        dpcm->nTotalRead = 0;
        dpcm->nTotalWrite = 0;
        //todo
        //dpcm->nPreHWPtr = dpcm->nHWPtr;
        ALSA_WARNING("[Re-Prepare %d %d %s %d]\n", (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr, __FUNCTION__, __LINE__);
        return 0;
    }

    if(runtime->format != SNDRV_PCM_FORMAT_S16_LE)
    {
        ALSA_WARNING("[Err @ %s %d]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    //SHOW_CONFIG_INFO("[======START======]\n",);
    ALSA_VitalPrint("[playback : rate %d channels %d\n", runtime->rate, runtime->channels);
    ALSA_VitalPrint("   period_size %d periods %d\n", (int)runtime->period_size, (int)runtime->periods);
    ALSA_VitalPrint("   buffer_size %d period_jiffies %d\n", (int)runtime->buffer_size, (int)dpcm->nPeriodJiffies);
    ALSA_VitalPrint("   start_threshold %d stop_threshold %d]\n", (int)runtime->start_threshold, (int)runtime->stop_threshold);
    //ALSA_VitalPrint("   access %s\n", snd_pcm_access_name_func(runtime->access));
    //ALSA_VitalPrint("[runtime->access %d]\n", runtime->access);
    //ALSA_VitalPrint("[runtime->format %d]\n", runtime->format);
    //ALSA_VitalPrint("[runtime->frame_bits %d]\n", runtime->frame_bits);
    //ALSA_VitalPrint("[runtime->sample_bits %d]\n", runtime->sample_bits);
    //ALSA_VitalPrint("[runtime->silence_threshold %d]\n", runtime->silence_threshold);
    //ALSA_VitalPrint("[runtime->silence_size %d]\n", runtime->silence_size);
    //ALSA_VitalPrint("[runtime->boundary %d]\n", runtime->boundary);
    //ALSA_VitalPrint("[runtime->min_align %d]\n", runtime->min_align);
    //ALSA_VitalPrint("[runtime->hw_ptr_base %x]\n", runtime->hw_ptr_base);
    //ALSA_VitalPrint("[runtime->dma_area %x]\n", runtime->dma_area);
    //SHOW_CONFIG_INFO("[======END======]\n",);

    dpcm->nPeriodBytes = frames_to_bytes(runtime, runtime->period_size);

    // malloc ao_inring
    if(snd_realtek_hw_malloc_ring(runtime))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    snd_realtek_hw_init_ringheader_of_AO(runtime);
    RPC_TOAGENT_AO_INIT_FLASH(substream);

    // AO pause
    if (RPC_TOAGENT_PAUSE_SVC(dpcm->AOAgentID | dpcm->AOpinID))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if(RPC_TOAGENT_RUN_SVC((dpcm->AOAgentID | dpcm->AOpinID)))
    {
        ALSA_WARNING("[%s %d]\n", __FUNCTION__, __LINE__);
        return -1;
    }

    dpcm->bInitRing = 1;

    return 0;
}

void rtk_alsa_AO_init_ringbuf(unsigned long pArg)
{
    FW_IOC_AO_INIT_RING_BUF_FD fd, *p;
    int i;

    p = (FW_IOC_AO_INIT_RING_BUF_FD *)pArg;
    for(i=0 ; i<8 ; ++i)
    {
        if(dec_out_handle[i])
            fd.AO_Flash_buf_fd[i] = ion_share_dma_buf_fd((struct ion_client *)alsa_client, (struct ion_handle *)dec_out_handle[i]);
    }
    fd.ALSA_fd = ion_share_dma_buf_fd((struct ion_client *)alsa_client, (struct ion_handle *)alsa_playback_handle);

    if(copy_to_user((void __user *)(p), &fd, sizeof(fd)))
        ALSA_WARNING("err @ %s %d\n", __FUNCTION__, __LINE__);
}
EXPORT_SYMBOL(rtk_alsa_AO_init_ringbuf);

#else
static int snd_card_playback_prepare(snd_pcm_substream_t * substream)
{
    ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;

    if(is_suspend)
    {
        //ALSA_VitalPrint("[ALSA %s %d] suspend\n", __FUNCTION__, __LINE__);
        return 0;
    }

    if(runtime->status->state == SNDRV_PCM_STATE_XRUN)
    {
        ALSA_WARNING("[SNDRV_PCM_STATE_XRUN appl_ptr %d hw_ptr %d]\n", (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr);
    }

    dpcm->nPeriodJiffies = HZ / (runtime->rate / runtime->period_size);
    if(dpcm->nPeriodJiffies == 0)
        dpcm->nPeriodJiffies = 1;

    if(dpcm->bInitRing)
    {
        dpcm->nHWReadSize = 0;
        //dpcm->nTotalRead = 0;
        dpcm->nTotalWrite = 0;
        //todo
        //dpcm->nPreHWPtr = dpcm->nHWPtr;
        ALSA_WARNING("[Re-Prepare %d %d %s %d]\n", (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr, __FUNCTION__, __LINE__);
        return 0;
    }

    //SHOW_CONFIG_INFO("[======START======]\n",);
    ALSA_VitalPrint("[playback : rate %d channels %d\n", runtime->rate, runtime->channels);
    ALSA_VitalPrint("   period_size %d periods %d\n", (int)runtime->period_size, (int)runtime->periods);
    ALSA_VitalPrint("   buffer_size %d period_jiffies %d\n", (int)runtime->buffer_size, (int)dpcm->nPeriodJiffies);
    ALSA_VitalPrint("   start_threshold %d stop_threshold %d]\n", (int)runtime->start_threshold, (int)runtime->stop_threshold);
    //ALSA_VitalPrint("   access %s\n", snd_pcm_access_name_func(runtime->access));
    //ALSA_VitalPrint("[runtime->access %d]\n", runtime->access);
    //ALSA_VitalPrint("[runtime->format %d]\n", runtime->format);
    //ALSA_VitalPrint("[runtime->frame_bits %d]\n", runtime->frame_bits);
    //ALSA_VitalPrint("[runtime->sample_bits %d]\n", runtime->sample_bits);
    //ALSA_VitalPrint("[runtime->silence_threshold %d]\n", runtime->silence_threshold);
    //ALSA_VitalPrint("[runtime->silence_size %d]\n", runtime->silence_size);
    //ALSA_VitalPrint("[runtime->boundary %d]\n", runtime->boundary);
    //ALSA_VitalPrint("[runtime->min_align %d]\n", runtime->min_align);
    //ALSA_VitalPrint("[runtime->hw_ptr_base %x]\n", runtime->hw_ptr_base);
    //ALSA_VitalPrint("[runtime->dma_area %x]\n", runtime->dma_area);
    //SHOW_CONFIG_INFO("[======END======]\n",);

    dpcm->nPeriodBytes = frames_to_bytes(runtime, runtime->period_size);

    // create decoder agent
    //if (RPC_TOAGENT_CREATE_DECODER_A
    if (RPC_TOAGENT_CREATE_DECODER_AGENT(&dpcm->DECAgentID, &dpcm->DECpinID))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // malloc ao_inring
    if(snd_realtek_hw_malloc_ring(runtime))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // init ringheader of decoder_outring and AO_inring
    if(snd_realtek_hw_init_ringheader_of_DEC_AO(runtime))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // connet decoder and AO by RPC
    if(snd_realtek_hw_init_connect_decoder_ao(dpcm))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // 1. init decoder in_ring
    // 2. init decoder inband ring
    if(snd_realtek_hw_init_decoder_ring(runtime))
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    // AO pause
    // decoder pause
    // decoder run
    // decoder flush
    // write decoder info into inband of decoder
    if (snd_realtek_hw_init_decoder_info(runtime))
    {
        ALSA_WARNING("[%s %d]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    if(snd_realtek_hw_resume(dpcm))
    {
        ALSA_WARNING("[%s %d]\n", __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    dpcm->bInitRing = 1;

    return 0;
}
#endif

static long snd_card_get_ring_data(RINGBUFFER_HEADER *pRing_BE, RINGBUFFER_HEADER *pRing_LE)
{
    long base, limit, rp, wp, data_size;

    base = (long)pRing_LE->beginAddr;
    limit= (long)(pRing_LE->beginAddr + pRing_LE->size);
    //wp = (long)SND_UNCACHE_ADDRESS(ntohl(pRing_BE->writePtr));
    //rp = (long)SND_UNCACHE_ADDRESS(ntohl(pRing_BE->readPtr[0]));
    wp = (long)(ntohl(pRing_BE->writePtr));
    rp = (long)(ntohl(pRing_BE->readPtr[0]));
//    printk("[�� b %x l %x r %x w %x]\n", base, limit, rp, wp);
    data_size = ring_valid_data(base, limit, rp, wp);
    return data_size;
}

// copy data from ring(AI) to dma_buf
#define COPY_FUNC(nFrameSize, p, pRingRp) \
{\
    int i,temp;\
    for(i=0 ; i<nFrameSize ; ++i)\
    {\
        temp = (int)(*pRingRp[0]);\
        temp = ENDIAN_CHANGE(temp);\
        *p = (short)(temp >> 9);\
        p++;\
        temp = (int)(*pRingRp[1]);\
        temp = ENDIAN_CHANGE(temp);\
        *p = (short)(temp >> 9);\
        p++;\
        pRingRp[0]++;\
        pRingRp[1]++;\
    }\
}
static void snd_card_capture_copy(snd_pcm_runtime_t *runtime, long nPeriodCount)
{
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;
    snd_pcm_uframes_t nFrameSize = nPeriodCount * runtime->period_size;
    snd_pcm_uframes_t dma_wp = dpcm->nTotalWrite % runtime->buffer_size;
    snd_pcm_uframes_t wrap_size = runtime->buffer_size - dma_wp;
    long *pRingRp[2];
    long nRingLimit[2];
    int i, loop0, loop1;
    short *p = NULL;

    for(i=0 ; i<2 ; ++i)
    {
        pRingRp[i] = (long *)dpcm->nAIRing_LE[i].readPtr[0];
        nRingLimit[i] = (long)(dpcm->nAIRing_LE[i].beginAddr + dpcm->nAIRing_LE[i].size);
    }

    if(nFrameSize > wrap_size)
    {
        p = (short *)(runtime->dma_area + LPCM_2CH_16BITS * dma_wp);
        if((long)(pRingRp[0] + wrap_size) > nRingLimit[0])
        {
            loop0 = (nRingLimit[0] - (long)pRingRp[0]) >> 2;
            loop1 = wrap_size - loop0;
            COPY_FUNC(loop0, p, pRingRp);
            pRingRp[0] = (long *)dpcm->nAIRing_LE[0].beginAddr;
            pRingRp[1] = (long *)dpcm->nAIRing_LE[1].beginAddr;
            COPY_FUNC(loop1, p, pRingRp);
        }
        else
        {
            COPY_FUNC(wrap_size, p, pRingRp);
        }

        p = (short *)(runtime->dma_area);
        nFrameSize -= wrap_size;
        if((long)(pRingRp[0] + nFrameSize) > nRingLimit[0])
        {
            loop0 = (nRingLimit[0] - (long)pRingRp[0]) >> 2;
            loop1 = nFrameSize - loop0;
            COPY_FUNC(loop0, p, pRingRp);
            pRingRp[0] = (long *)dpcm->nAIRing_LE[0].beginAddr;
            pRingRp[1] = (long *)dpcm->nAIRing_LE[1].beginAddr;
            COPY_FUNC(loop1, p, pRingRp);
        }
        else
        {
            COPY_FUNC(nFrameSize, p, pRingRp);
        }
    }
    else
    {
        p = (short *)(runtime->dma_area + LPCM_2CH_16BITS * dma_wp);
        if((long)(pRingRp[0] + nFrameSize) > nRingLimit[0])
        {
            loop0 = (nRingLimit[0] - (long)pRingRp[0]) >> 2;
            loop1 = nFrameSize - loop0;
            COPY_FUNC(loop0, p, pRingRp);
            pRingRp[0] = (long *)dpcm->nAIRing_LE[0].beginAddr;
            pRingRp[1] = (long *)dpcm->nAIRing_LE[1].beginAddr;
            COPY_FUNC(loop1, p, pRingRp);
        }
        else
        {
            COPY_FUNC(nFrameSize, p, pRingRp);
        }
    }
}

static void snd_card_capture_LPCM_copy(snd_pcm_runtime_t *runtime, long nPeriodCount)
{
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;
    snd_pcm_uframes_t nFrameSize = nPeriodCount * runtime->period_size;
    snd_pcm_uframes_t dma_wp = dpcm->nTotalWrite % runtime->buffer_size;
    AUDIO_RINGBUF_PTR src_ring, dst_ring;

    src_ring.base = (long)dpcm->pLPCMData;
    src_ring.limit = src_ring.base + dpcm->nLPCMRing_LE.size;
    src_ring.rp = src_ring.base + ((long)dpcm->nLPCMRing_LE.readPtr[0] - (long)dpcm->nLPCMRing_LE.beginAddr);

    dst_ring.base = (long)runtime->dma_area;
    dst_ring.limit = (long)runtime->dma_area + runtime->buffer_size * dpcm->nFrameBytes;
    dst_ring.wp = (long)runtime->dma_area + dma_wp * dpcm->nFrameBytes;

    ring1_to_ring2_general(&src_ring, &dst_ring, nFrameSize * dpcm->nFrameBytes, NULL);
}

#if _DBG_CHECK_AI_HW_RING_DATA_EN
static void snd_card_capture_lpcm_check_ringBuf(unsigned int data)
{
    snd_pcm_substream_t * substream = (snd_pcm_substream_t *)data;
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;
    long nDataOfAIInring = 0;
    long nDataOfAILpcmRing = 0;
    nDataOfAIInring = snd_card_get_ring_data(dpcm->nAIRing, dpcm->nAIRing_LE) >> 2;
    nDataOfAILpcmRing = snd_card_get_ring_data(&dpcm->nLPCMRing, &dpcm->nLPCMRing_LE) / dpcm->nFrameBytes;
#if 0
    {
        long wp, rp, base, limit;
        base = (long)dpcm->nAIRing_LE[0].beginAddr;
        limit= (long)(dpcm->nAIRing_LE[0].beginAddr + dpcm->nAIRing_LE[0].size);
        wp = (long)(ntohl(dpcm->nAIRing[0].writePtr));
        rp = (long)(ntohl(dpcm->nAIRing[0].readPtr[0]));
        ALSA_VitalPrint("[ALSA AI HW_ring b %x l %x w %x r %x]\n", base, limit, wp, rp);
    }
#endif
    ALSA_VitalPrint("[ALSA AI size %x data %x LPCM size %x data %x]\n"
        , dpcm->nAIRing_LE[0].size >> 2, nDataOfAIInring, dpcm->nLPCMRing_LE.size / dpcm->nFrameBytes, nDataOfAILpcmRing / dpcm->nFrameBytes);
}
#endif

static void snd_card_capture_lpcm_timer_function(unsigned int data)
{
    snd_pcm_substream_t * substream = (snd_pcm_substream_t *)data;
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;
    snd_pcm_uframes_t nRingDataFrame;
    unsigned long flags;
    long nRingDataSize;
    unsigned int nPeriodCount = 0;

    if(ring_valid_data(0, runtime->boundary, runtime->control->appl_ptr, runtime->status->hw_ptr) > runtime->buffer_size)
    {
        ALSA_WARNING("[hw_ptr %d appl_ptr %d %s %d]\n", (int)runtime->status->hw_ptr, (int)runtime->control->appl_ptr, __FUNCTION__, __LINE__);
    }

#if 0   // DEBUG: check wp/rp
    ALSA_SEC_PRINT(2, "[���� state %d appl_ptr %d hw_ptr %d]\n", (int)runtime->status->state, (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr);
#endif

    _DBG_CHECK_AI_HW_RING_DATA(data);

    nRingDataSize = snd_card_get_ring_data(&dpcm->nLPCMRing, &dpcm->nLPCMRing_LE);
    nRingDataFrame = nRingDataSize / dpcm->nFrameBytes;
    if(nRingDataFrame >= runtime->period_size)
    {
        nPeriodCount = nRingDataFrame / runtime->period_size;
        if(nPeriodCount == runtime->periods)
            nPeriodCount--;

        // dma wirte too faster ?? (NOT sure)

        // copy data from LPCM_ring to dma_buf
        snd_card_capture_LPCM_copy(runtime, nPeriodCount);

        // update LPCM_ring rp.
        dpcm->nLPCMRing_LE.readPtr[0] = ring_add(dpcm->nLPCMRing_LE.beginAddr
            , dpcm->nLPCMRing_LE.beginAddr + dpcm->nLPCMRing_LE.size
            , dpcm->nLPCMRing_LE.readPtr[0]
            , nPeriodCount * runtime->period_size * dpcm->nFrameBytes);
        dpcm->nLPCMRing.readPtr[0] = htonl(dpcm->nLPCMRing_LE.readPtr[0]);

        dpcm->nTotalWrite += nPeriodCount * runtime->period_size;
        snd_pcm_period_elapsed(substream);
    }

    // set timer
    dpcm->nStartTimer.expires = dpcm->nPeriodJiffies + jiffies;
    //spin_lock_irqsave(&dpcm->nLock, flags);
    spin_lock_irqsave(&capture_lock, flags);
    if(dpcm->bStop)
        dpcm->bStop = 0;
    else
        add_timer(&dpcm->nStartTimer);
    //spin_unlock_irqrestore(&dpcm->nLock, flags);
    spin_unlock_irqrestore(&capture_lock, flags);
}

static void snd_card_capture_timer_function(unsigned int data)
{
    snd_pcm_substream_t * substream = (snd_pcm_substream_t *)data;
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;
    snd_pcm_uframes_t nRingDataFrame;
    unsigned long flags;
    long nRingDataSize;
    unsigned int nPeriodCount = 0;
    int ch;

    if(ring_valid_data(0, runtime->boundary, runtime->control->appl_ptr, runtime->status->hw_ptr) > runtime->buffer_size)
    {
        ALSA_WARNING("[hw_ptr %d appl_ptr %d %s %d]\n", (int)runtime->status->hw_ptr, (int)runtime->control->appl_ptr, __FUNCTION__, __LINE__);
    }

#if 0   // DEBUG: check wp/rp
    ALSA_SEC_PRINT(2, "[���� state %d appl_ptr %d hw_ptr %d]\n", (int)runtime->status->state, (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr);
#endif

    nRingDataSize = snd_card_get_ring_data(dpcm->nAIRing, dpcm->nAIRing_LE);
    nRingDataFrame = nRingDataSize >> 2;    // 1 PCM = 32 bits.
    if(nRingDataFrame >= runtime->period_size)
    {
        nPeriodCount = nRingDataFrame / runtime->period_size;
        if(nPeriodCount == runtime->periods)
            nPeriodCount--;

        // dma wirte too faster ?? (NOT sure)

        // copy data from AI_ring to dma_buf
        snd_card_capture_copy(runtime, nPeriodCount);

        // update AI_ring rp.
        for(ch=0 ; ch<2 ; ++ch)
        {
            dpcm->nAIRing_LE[ch].readPtr[0] = ring_add(dpcm->nAIRing_LE[ch].beginAddr
                , dpcm->nAIRing_LE[ch].beginAddr+dpcm->nAIRing_LE[ch].size
                , dpcm->nAIRing_LE[ch].readPtr[0], nPeriodCount * runtime->period_size * 4 /* 1 PCM = 32 bits */);
            dpcm->nAIRing[ch].readPtr[0] = htonl(dpcm->nAIRing_LE[ch].readPtr[0]);
        }

        dpcm->nTotalWrite += nPeriodCount * runtime->period_size;
        snd_pcm_period_elapsed(substream);
    }

    // set timer
    dpcm->nStartTimer.expires = dpcm->nPeriodJiffies + jiffies;
    //spin_lock_irqsave(&dpcm->nLock, flags);
    spin_lock_irqsave(&capture_lock, flags);
    if(dpcm->bStop)
        dpcm->bStop = 0;
    else
        add_timer(&dpcm->nStartTimer);
    //spin_unlock_irqrestore(&dpcm->nLock, flags);
    spin_unlock_irqrestore(&capture_lock, flags);
}

#ifdef AO_ON_SCPU
static int snd_card_playback_copy_subfunc(struct snd_pcm_substream *substream, unsigned char *pLPCM_src, unsigned int nFrames)
{
    // only support 16 bit LE, 2 channels
#define FRAME_BYTES 4
#define ENDIAN_LE_2_BE(x) (((((unsigned long)x)&0xff000000)>>24)|((((unsigned long)x)&0x00ff0000)>>8)|((((unsigned long)x)&0x0000ff00)<<8)|((((unsigned long)x)&0x000000ff)<<24))
#define SAMPLE_CONVERT(pDst, pSrc) { \
    int sample32;\
    short sample16;\
    sample16 = *((short *)(pSrc)); \
    sample32 = sample16 << 9; \
    *((int *)(pDst)) = ENDIAN_LE_2_BE(sample32); }

    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
    RINGBUFFER_HEADER *pRingHdr = &dpcm->decOutRing_LE[0]; // little endian
    volatile RINGBUFFER_HEADER *pRingHdr_share = &dpcm->decOutRing[0];   // big endian
    int i, ch;
    unsigned int ring_free_frames;
    int sample32;
    int *L_wp = NULL;
    int *R_wp = NULL;
    short sample16;
    char *p = (char *)&sample16;
    char *pFrm = NULL;

    ring_free_frames = ((pRingHdr->beginAddr + pRingHdr->size)
        - pRingHdr->writePtr) >> 2;

    if(ring_free_frames >= nFrames)
    {
        pFrm = pLPCM_src;
        L_wp = (int *)((int)dpcm->decOutData[0] + (pRingHdr[0].writePtr - pRingHdr[0].beginAddr));
        R_wp = (int *)((int)dpcm->decOutData[1] + (pRingHdr[1].writePtr - pRingHdr[1].beginAddr));
        for(i=0 ; i<nFrames ; ++i, pFrm += 4, L_wp++, R_wp++)
        {
            SAMPLE_CONVERT(L_wp, pFrm);
            SAMPLE_CONVERT(R_wp, pFrm + 2);
        }
    }
    else
    {
        // wrap arround
        unsigned int len0 = ring_free_frames;
        unsigned int len1 = nFrames - ring_free_frames;
        pFrm = pLPCM_src;
        L_wp = (int *)((int)dpcm->decOutData[0] + (pRingHdr[0].writePtr - pRingHdr[0].beginAddr));
        R_wp = (int *)((int)dpcm->decOutData[1] + (pRingHdr[1].writePtr - pRingHdr[1].beginAddr));
        for(i=0 ; i<len0 ; ++i, pFrm += 4, L_wp++, R_wp++)
        {
            SAMPLE_CONVERT(L_wp, pFrm);
            SAMPLE_CONVERT(R_wp, pFrm + 2);
        }
        L_wp = (int *)dpcm->decOutData[0];
        R_wp = (int *)dpcm->decOutData[1];
        for(i=0 ; i<len1 ; ++i, pFrm += 4, L_wp++, R_wp++)
        {
            SAMPLE_CONVERT(L_wp, pFrm);
            SAMPLE_CONVERT(R_wp, pFrm + 2);
        }
    }

    // update RingHdr
    for(ch=0 ; ch<runtime->channels ; ++ch)
    {
        pRingHdr[ch].writePtr = ring_add(pRingHdr[ch].beginAddr
            , pRingHdr[ch].beginAddr + pRingHdr[ch].size
            , pRingHdr[ch].writePtr, nFrames << 2);
        pRingHdr_share[ch].writePtr = htonl(pRingHdr[ch].writePtr);
    }

    return 0;
}
static void snd_card_timer_function(unsigned int data)
{
#define MIN(a, b) ((a) < (b) ? (a) : (b))
    //TRACE_CODE("[%s %s %d]\n", __FILE__, __FUNCTION__, __LINE__);
    snd_pcm_substream_t * substream = (snd_pcm_substream_t *)data;
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
    RINGBUFFER_HEADER *pRingHdr = &dpcm->decOutRing_LE[0]; // little endian
    volatile RINGBUFFER_HEADER *pRingHdr_share = &dpcm->decOutRing[0];   // big endian
    unsigned long flags;
    snd_pcm_uframes_t nReadAddSize = 0;
    unsigned long HWRingRp;
    long dec_out_valid_size = 0;
    unsigned long AOInringFreeSize = 0; // unit:frame
    unsigned long BSSize = 0;    // unit:frame
    unsigned long nMoveFrame; // unit:frame
    unsigned long nMovePeriod = 0, nBSPeriod = 0, AOInringFreePeriod = 0;
    unsigned char *pStart = NULL;
    int ch, i;

    // update read pointer
    for(ch=0 ; ch<runtime->channels ; ++ch)
        for(i=0 ; i<4 ; ++i)
            pRingHdr[ch].readPtr[i] = ntohl(pRingHdr_share[ch].readPtr[i]);

#if 0
    ALSA_SEC_PRINT(2, "�� alsa b %x l %x w %x r %x\n"
        , pRingHdr->beginAddr, pRingHdr->beginAddr + rtk_dec_ao_buffer
        , pRingHdr->writePtr, pRingHdr->readPtr[0]);
#endif

    //calculate AOInring msec
    dec_out_valid_size = ring_valid_data(
        (unsigned long)(pRingHdr->beginAddr),
        (unsigned long)(pRingHdr->beginAddr) + pRingHdr->size,
        (unsigned long)(pRingHdr->readPtr[0]),
        (unsigned long)(pRingHdr->writePtr));

    //ALSA_VitalPrint("dec_out_valid_size %d\n", dec_out_valid_size);
    if(dec_out_valid_size > 0 && dec_out_valid_size <= dpcm->nRingSize)
        dec_out_msec = ((dec_out_valid_size >> 2) * 1000) / runtime->rate;
    else
        dec_out_msec = 0;
    //ALSA_VitalPrint("dec_out_msec %d\n", dec_out_msec);

    //////////////////////////////////////////////////////////////////////////////////

    if(runtime->control->appl_ptr == runtime->status->hw_ptr)
    {
        ALSA_WARNING("[appl_ptr %d = hw_ptr %s %d]\n", (int)runtime->control->appl_ptr, __FUNCTION__, __LINE__);
    }

    // check overflow of AO flash in-ring
    AOInringFreeSize = pRingHdr->size - ring_valid_data(pRingHdr->beginAddr
        , pRingHdr->beginAddr + pRingHdr->size, pRingHdr->readPtr[0]
        , pRingHdr->writePtr) - 1;
    AOInringFreeSize >>= 2; // unit: frame (2 channel, 16 bits)
    BSSize = ring_valid_data(0, runtime->boundary, runtime->status->hw_ptr, runtime->control->appl_ptr);
    nBSPeriod = BSSize / runtime->period_size;

    if(nBSPeriod == 0 && runtime->status->state != SNDRV_PCM_STATE_DRAINING)
    {
        // NOT enough BS
        goto exit;
    }

    if(AOInringFreeSize < runtime->period_size)
    {
        // AO has NOT enough free space
        goto exit;
    }

    AOInringFreePeriod = AOInringFreeSize / runtime->period_size;

    if(runtime->status->state == SNDRV_PCM_STATE_DRAINING)
        nMovePeriod = MIN(nBSPeriod, AOInringFreePeriod);
    else if(nBSPeriod <= AOInringFreePeriod)
        nMovePeriod = nBSPeriod - 1;
    else
        nMovePeriod = MIN(nBSPeriod, AOInringFreePeriod);
    nMoveFrame = nMovePeriod * runtime->period_size;

    if(nMoveFrame)
    {
        pStart = runtime->dma_area + frames_to_bytes(runtime, dpcm->nTotalRead % runtime->buffer_size);
        // check wrap around
        if((runtime->buffer_size - (dpcm->nTotalRead % runtime->buffer_size)) >= nMoveFrame)
        {
            // BS NOT need to warp arrond
            snd_card_playback_copy_subfunc(substream, pStart, nMoveFrame);
        }
        else
        {
            // BS need to wrap around
            unsigned int nFrameLen0 = runtime->buffer_size - (dpcm->nTotalRead % runtime->buffer_size);
            unsigned int nFrameLen1 = nMoveFrame - nFrameLen0;
            snd_card_playback_copy_subfunc(substream, pStart, nFrameLen0);
            snd_card_playback_copy_subfunc(substream, (unsigned char *)runtime->dma_area, nFrameLen1);
        }

        spin_lock_irqsave(&playback_lock, flags);
        dpcm->nTotalRead += nMoveFrame;
        spin_unlock_irqrestore(&playback_lock, flags);
    }

    if(runtime->status->state == SNDRV_PCM_STATE_DRAINING)
    {
        switch (dpcm->nEOSState)
        {
            case SND_REALTEK_EOS_STATE_NONE:
                //dpcm->nEOSState = SND_REALTEK_EOS_STATE_PROCESSING;
                //schedule_work(&dpcm->nEOSWork);
                if (RPC_TOAGENT_INBAND_EOS_SVC(dpcm) < 0)
                {
                    ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
                    return;
                }
                dpcm->nEOSState = SND_REALTEK_EOS_STATE_FINISH;
                break;
            //case SND_REALTEK_EOS_STATE_PROCESSING:
            //    ALSA_VitalPrint("wilson [ALSA EOS %s %d]\n", __FUNCTION__, __LINE__);
            //    break;
            case SND_REALTEK_EOS_STATE_FINISH:
                snd_pcm_period_elapsed(substream);
                break;
            default:
                break;
        }
    }
    else
    {
        DEBUG_CODE("[snd_pcm_period_elapsed]\n");
        snd_pcm_period_elapsed(substream);
    }

#if 0
    // check if wp and rp both stop
    if(dpcm->nHWPtr == dpcm->nPreHWPtr && runtime->control->appl_ptr == dpcm->nPre_appl_ptr)
        dbg_count++;
    else
        dbg_count = 0;
    if(dbg_count >= (HZ << 1))
    {
//        ALSA_WARNING("[HANG !!! %s %d]\n", __FUNCTION__, __LINE__);
//        ALSA_WARNING("[state %d write_state %d]\n", (int)runtime->status->state, (int)dpcm->nWriteState);
#if 0
        ALSA_WARNING("[state %d]\n", (int)runtime->status->state);
        ALSA_WARNING("[runtime->control->appl_ptr %d runtime->status->hw_ptr %d]\n", (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr);
        ALSA_WARNING("[dpcm->nTotalWrite %d dpcm->nTotalRead %d dpcm->nHWPtr %d]\n", (int)dpcm->nTotalWrite, (int)dpcm->nTotalRead, (int)dpcm->nHWPtr);
        ALSA_WARNING("[b %x l %x w %x r %x]\n",
           (unsigned int)(ntohl(dpcm->decInRing[0].beginAddr)),
           (unsigned int)((ntohl(dpcm->decInRing[0].beginAddr)) + dpcm->decInRing_LE[0].size),
           (unsigned int)(ntohl(dpcm->decInRing[0].writePtr)),
           (unsigned int)(ntohl(dpcm->decInRing[0].readPtr[0])));
#endif
//        ALSA_WARNING("[HWRingRp %x nPeriodCount %d runtime->periods %d]\n", HWRingRp, nPeriodCount, runtime->periods);
//        ALSA_WARNING("[HWRingFreeSize %d HWRingFreeFrame %d dpcm->nPeriodBytes %d]\n", HWRingFreeSize, HWRingFreeFrame, dpcm->nPeriodBytes);
//        ALSA_WARNING("[snd_pcm_playback_avail %d runtime->control->avail_min %d]\n", snd_pcm_playback_avail(runtime), runtime->control->avail_min);
        dbg_count = 0;
    }
#endif

exit:
    dpcm->nStartTimer.expires = dpcm->nPeriodJiffies + jiffies;

    spin_lock_irqsave(&playback_lock, flags);
    if(dpcm->bStop)
        dpcm->bStop = 0;
    else
        add_timer(&dpcm->nStartTimer);
    spin_unlock_irqrestore(&playback_lock, flags);
}
#else
static void snd_card_timer_function(unsigned int data)
{
    //TRACE_CODE("[%s %s %d]\n", __FILE__, __FUNCTION__, __LINE__);
    snd_pcm_substream_t * substream = (snd_pcm_substream_t *)data;
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
    unsigned long flags;

    snd_pcm_uframes_t nReadAddSize = 0;
    unsigned int nPeriodCount = 0;
    unsigned long HWRingRp;
    unsigned long HWRingFreeSize;
    unsigned long HWRingFreeFrame;
    static unsigned long dbg_count = 0;

    //calculate AOInring msec
    long dec_out_valid_size = 0;
    dec_out_valid_size = ring_valid_data(
        (unsigned long)ntohl(dpcm->decOutRing[0].beginAddr),
        (unsigned long)ntohl(dpcm->decOutRing[0].beginAddr) + rtk_dec_ao_buffer,
        (unsigned long)ntohl(dpcm->decOutRing[0].readPtr[0]),
        (unsigned long)ntohl(dpcm->decOutRing[0].writePtr));

    //ALSA_VitalPrint("dec_out_valid_size %d\n", dec_out_valid_size);
    if(dec_out_valid_size > 0 && dec_out_valid_size <= dpcm->nRingSize)
        dec_out_msec = ((dec_out_valid_size >> 2) * 1000) / runtime->rate;
    else
        dec_out_msec = 0;
    //ALSA_VitalPrint("dec_out_msec %d\n", dec_out_msec);

    //////////////////////////////////////////////////////////////////////////////////

    if(runtime->control->appl_ptr == runtime->status->hw_ptr)
    {
        ALSA_WARNING("[appl_ptr %d = hw_ptr %s %d]\n", (int)runtime->control->appl_ptr, __FUNCTION__, __LINE__);
    }

    // update HW rp
    HWRingRp = (long)(ntohl(dpcm->decInRing[0].readPtr[0]));//physical address

    dpcm->nHWPtr = bytes_to_frames(runtime, (unsigned long)HWRingRp - (unsigned long)dpcm->decInRing_LE[0].beginAddr);

    // update HW read size
    if(dpcm->nHWPtr != dpcm->nPreHWPtr)
    {
        nReadAddSize = ring_valid_data(0, runtime->buffer_size, dpcm->nPreHWPtr, dpcm->nHWPtr);

#if 1
        if(nReadAddSize > (runtime->buffer_size >> 1))
        {
#ifdef WORK_AROUND_BUG34499
            nReadAddSize = runtime->buffer_size >> 1;
            dpcm->nHWPtr = ring_add(0, runtime->buffer_size, dpcm->nPreHWPtr, nReadAddSize);
#endif
            //ALSA_WARNING("[ALSA maybe hang because Jitter %d, runtime->buffer size %d %s %d]\n", (int)nReadAddSize, (int)runtime->buffer_size, __FUNCTION__, __LINE__);
        }
#endif
        dpcm->nHWReadSize += nReadAddSize;
        dpcm->nTotalRead += nReadAddSize;
    }

#ifndef USE_COPY_OPS
    // update wp
    nPeriodCount = ring_valid_data(0, runtime->boundary, dpcm->nTotalWrite, runtime->control->appl_ptr) / runtime->period_size;

    if(nPeriodCount == runtime->periods)
        nPeriodCount--;

    //printk("base %lx limit %lx\n",
    //    dpcm->decInRing_LE[0].beginAddr,
    //    dpcm->decInRing_LE[0].beginAddr + dpcm->decInRing_LE[0].size);
    //printk("rp   %lx wp %lx\n", HWRingRp , dpcm->decInRing_LE[0].writePtr);

    HWRingFreeSize = valid_free_size(dpcm->decInRing_LE[0].beginAddr,
        dpcm->decInRing_LE[0].beginAddr + dpcm->decInRing_LE[0].size,
        HWRingRp,
        dpcm->decInRing_LE[0].writePtr);

    HWRingFreeFrame = bytes_to_frames(runtime, HWRingFreeSize);

    if((runtime->period_size * nPeriodCount) > HWRingFreeFrame)
        nPeriodCount = HWRingFreeFrame / runtime->period_size;

    if(HWRingFreeSize <= dpcm->nPeriodBytes)
        nPeriodCount = 0;

    if(nPeriodCount)
    {
        dpcm->decInRing_LE[0].writePtr = ring_add(dpcm->decInRing_LE[0].beginAddr
            , dpcm->decInRing_LE[0].beginAddr + dpcm->decInRing_LE[0].size
            , dpcm->decInRing_LE[0].writePtr
            , frames_to_bytes(runtime, runtime->period_size * nPeriodCount));

        dpcm->nTotalWrite = ring_add(0,
            runtime->boundary,
            dpcm->nTotalWrite,
            runtime->period_size * nPeriodCount);

        // update wp
        //dpcm->decInRing[0].writePtr = htonl(dpcm->decInRing_LE[0].writePtr);
        dpcm->decInRing[0].writePtr = htonl(dpcm->decInRing_LE[0].writePtr);//record physical address
        //printk("update decInRing wp %x\n", htonl(dpcm->decInRing[0].writePtr));
    }
#endif

    //ALSA_VitalPrint("[nReadAddSize %d pre %d cur %d %d]\n", nReadAddSize, dpcm->nPreHWPtr, dpcm->nHWPtr, dpcm->nHWReadSize);
//    ALSA_VitalPrint("[nTotalWrite %d nTotalRead %d nPeriodCount %d]\n", dpcm->nTotalWrite, dpcm->nTotalRead, nPeriodCount);

    if(runtime->status->state == SNDRV_PCM_STATE_DRAINING)
    {
        switch (dpcm->nEOSState)
        {
            case SND_REALTEK_EOS_STATE_NONE:
                //dpcm->nEOSState = SND_REALTEK_EOS_STATE_PROCESSING;
                //schedule_work(&dpcm->nEOSWork);
                if (RPC_TOAGENT_INBAND_EOS_SVC(dpcm) < 0)
                {
                    ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
                    return;
                }
                dpcm->nEOSState = SND_REALTEK_EOS_STATE_FINISH;
                break;
            //case SND_REALTEK_EOS_STATE_PROCESSING:
            //    ALSA_VitalPrint("wilson [ALSA EOS %s %d]\n", __FUNCTION__, __LINE__);
            //    break;
            case SND_REALTEK_EOS_STATE_FINISH:
                if (dpcm->nTotalWrite == dpcm->nTotalRead)
                {
                    DEBUG_CODE("[snd_pcm_period_elapsed]\n");
                    dpcm->nHWReadSize = 0;
                    snd_pcm_period_elapsed(substream);
                }
                break;
            default:
                break;
        }
    }
    else
    {
        //printk("dpcm nHWReadSize %lu, runtime->period_size %lu\n", dpcm->nHWReadSize, runtime->period_size);
        if(dpcm->nHWReadSize >= runtime->period_size)
        {
            dpcm->nHWReadSize %= runtime->period_size;
            DEBUG_CODE("[snd_pcm_period_elapsed]\n");
            snd_pcm_period_elapsed(substream);
        }
    }

#ifndef USE_COPY_OPS
    // check if wp and rp both stop
    if(dpcm->nHWPtr == dpcm->nPreHWPtr && runtime->control->appl_ptr == dpcm->nPre_appl_ptr)
        dbg_count++;
    else
        dbg_count = 0;
    if(dbg_count >= (HZ << 1))
    {
//        ALSA_WARNING("[HANG !!! %s %d]\n", __FUNCTION__, __LINE__);
//        ALSA_WARNING("[state %d write_state %d]\n", (int)runtime->status->state, (int)dpcm->nWriteState);
#if 0
        ALSA_WARNING("[state %d]\n", (int)runtime->status->state);
        ALSA_WARNING("[runtime->control->appl_ptr %d runtime->status->hw_ptr %d]\n", (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr);
        ALSA_WARNING("[dpcm->nTotalWrite %d dpcm->nTotalRead %d dpcm->nHWPtr %d]\n", (int)dpcm->nTotalWrite, (int)dpcm->nTotalRead, (int)dpcm->nHWPtr);
        ALSA_WARNING("[b %x l %x w %x r %x]\n",
           (unsigned int)(ntohl(dpcm->decInRing[0].beginAddr)),
           (unsigned int)((ntohl(dpcm->decInRing[0].beginAddr)) + dpcm->decInRing_LE[0].size),
           (unsigned int)(ntohl(dpcm->decInRing[0].writePtr)),
           (unsigned int)(ntohl(dpcm->decInRing[0].readPtr[0])));
#endif
//        ALSA_WARNING("[HWRingRp %x nPeriodCount %d runtime->periods %d]\n", HWRingRp, nPeriodCount, runtime->periods);
//        ALSA_WARNING("[HWRingFreeSize %d HWRingFreeFrame %d dpcm->nPeriodBytes %d]\n", HWRingFreeSize, HWRingFreeFrame, dpcm->nPeriodBytes);
//        ALSA_WARNING("[snd_pcm_playback_avail %d runtime->control->avail_min %d]\n", snd_pcm_playback_avail(runtime), runtime->control->avail_min);
        dbg_count = 0;
    }
#endif
    dpcm->nPreHWPtr = dpcm->nHWPtr;
//    dpcm->nPre_appl_ptr = runtime->control->appl_ptr;

    // set timer
    //if(nPeriodCount <= 1)
    //    dpcm->nStartTimer.expires = dpcm->nPeriodJiffies + 1 + jiffies;
    //else
    dpcm->nStartTimer.expires = dpcm->nPeriodJiffies + jiffies;

    spin_lock_irqsave(&playback_lock, flags);
    if(dpcm->bStop)
        dpcm->bStop = 0;
    else
        add_timer(&dpcm->nStartTimer);
    spin_unlock_irqrestore(&playback_lock, flags);
}
#endif

static int snd_card_capture_trigger(snd_pcm_substream_t * substream, int cmd)
{
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_capture_pcm_t *dpcm = runtime->private_data;
    int ret = 0;
    unsigned long flags;
    ALSA_VitalPrint("[ALSA %s %d cmd %d]\n", __FUNCTION__, __LINE__, cmd);

    // error checking
    if(snd_pcm_capture_avail(runtime) > runtime->buffer_size
        || snd_pcm_capture_hw_avail(runtime) > runtime->buffer_size)
    {
        ALSA_WARNING("[ERROR BUG %s %s %d]\n", __FILE__, __FUNCTION__, __LINE__);
        ALSA_WARNING("[state %d appl_ptr %d hw_ptr %d]\n", runtime->status->state, (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr);
    }

    switch(cmd)
    {
        case SNDRV_PCM_TRIGGER_START:
            ALSA_VitalPrint("[ALSA trigger start appl_ptr %d hw_ptr %d]\n", (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr);
            // set timer
            dpcm->nStartTimer.expires = 1 + jiffies;
            add_timer(&dpcm->nStartTimer);
            break;
        case SNDRV_PCM_TRIGGER_STOP:
            ALSA_VitalPrint("[ALSA trigger stop appl_ptr %d hw_ptr %d]\n", (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr);
            //spin_lock_irqsave(&dpcm->nLock, flags);
            spin_lock_irqsave(&capture_lock, flags);
            del_timer(&dpcm->nStartTimer);
            dpcm->bStop = 1;
            //spin_unlock_irqrestore(&dpcm->nLock, flags);
            spin_unlock_irqrestore(&capture_lock, flags);
            break;
        default:
            ret = -EINVAL;
    }

    return ret;
}

static int snd_card_playback_trigger(snd_pcm_substream_t * substream, int cmd)
{
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
    int ret = 0;
    unsigned long flags;
    ALSA_VitalPrint("[ALSA %s %d cmd %d]\n", __FUNCTION__, __LINE__, cmd);

    if(cmd != SNDRV_PCM_TRIGGER_SUSPEND && is_suspend)
    {
        ALSA_VitalPrint("[ALSA %s %d] suspend\n", __FUNCTION__, __LINE__);
        return 0;
    }

    // error checking
    if(snd_pcm_playback_avail(runtime) > runtime->buffer_size
        || snd_pcm_playback_hw_avail(runtime) > runtime->buffer_size)
    {
        ALSA_WARNING("[ERROR BUG %s %s %d]\n", __FILE__, __FUNCTION__, __LINE__);
        ALSA_WARNING("[state %d appl_ptr %d hw_ptr %d]\n", runtime->status->state, (int)runtime->control->appl_ptr, (int)runtime->status->hw_ptr);
        ALSA_WARNING("[dpcm->nTotalWrite %d dpcm->nTotalRead %d]\n", (int)dpcm->nTotalWrite, (int)dpcm->nTotalRead);
    }

    switch(cmd)
    {
        case SNDRV_PCM_TRIGGER_STOP:
            spin_lock_irqsave(&playback_lock, flags);
            dpcm->bStop = 1;
            del_timer(&dpcm->nStartTimer);
            spin_unlock_irqrestore(&playback_lock, flags);
            break;

        case SNDRV_PCM_TRIGGER_START:
            dpcm->bStop = 0;
            dpcm->nStartTimer.expires = 1 + jiffies;
            add_timer(&dpcm->nStartTimer);
            break;

        case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
            spin_lock_irqsave(&playback_lock, flags);
            dpcm->bStop = 1;
            spin_unlock_irqrestore(&playback_lock, flags);
            break;

        case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
            dpcm->bStop = 0;
            dpcm->nStartTimer.expires = 1 + jiffies;
            add_timer(&dpcm->nStartTimer);
            break;

        case SNDRV_PCM_TRIGGER_SUSPEND:
            spin_lock_irqsave(&playback_lock, flags);
            //ALSA_VitalPrint("[+]SNDRV_PCM_TRIGGER_SUSPEND dpcm->bStop %d\n", dpcm->bStop);
            if(!dpcm->bStop)
                dpcm->bStop = 1;
            //snd_pcm_suspend_all(dpcm->substream->pcm);
            del_timer(&dpcm->nStartTimer);
            //ALSA_VitalPrint("[-]SNDRV_PCM_TRIGGER_SUSPEND\n");
            spin_unlock_irqrestore(&playback_lock, flags);
            break;

        case SNDRV_PCM_TRIGGER_RESUME:
            //ALSA_VitalPrint("[+]SNDRV_PCM_TRIGGER_RESUME\n");
            dpcm->bStop = 0;
            dpcm->nStartTimer.expires = 1 + jiffies;
            add_timer(&dpcm->nStartTimer);
            //ALSA_VitalPrint("[-]SNDRV_PCM_TRIGGER_RESUME\n");
            break;

        default:
            ALSA_WARNING("[err %d %s %d]\n", cmd, __FUNCTION__, __LINE__);
            ret = -EINVAL;
    }
    return ret;
}

static int snd_card_hw_params(snd_pcm_substream_t * substream, snd_pcm_hw_params_t * hw_params)
{
    //ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
    int err;
    //ALSA_VitalPrint("[ALSA %s demand %d Bytes]\n", __FUNCTION__, params_buffer_bytes(hw_params));
    printk("[ALSA %s demand %d Bytes]\n", __FUNCTION__, params_buffer_bytes(hw_params));
    err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
    if(err != 0 && err != 1)
    {
        ALSA_WARNING("[fail %s %d]\n", __FUNCTION__, __LINE__);
    }
    return err;
}

static int snd_card_capture_hw_free(snd_pcm_substream_t * substream)
{
    DEBUG_CODE("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
    return snd_pcm_lib_free_pages(substream);
}

static int snd_card_hw_free(snd_pcm_substream_t * substream)
{
    //ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
    return snd_pcm_lib_free_pages(substream);
}

#ifdef AO_ON_SCPU
static int snd_card_playback_copy(struct snd_pcm_substream *substream, int channel, snd_pcm_uframes_t pos, void __user *buf, snd_pcm_uframes_t count)
{
    //ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;
    unsigned int writableSize = 0;
    unsigned int flags;
    static int bufFullCount = 0;

    //check ring buffer available
    spin_lock_irqsave(&playback_lock, flags);
    writableSize = runtime->buffer_size - ring_valid_data(0
        , runtime->boundary, dpcm->nTotalRead, (long)runtime->control->appl_ptr);
    spin_unlock_irqrestore(&playback_lock, flags);

    while(count > writableSize)
    {
        if(bufFullCount++ > 1000)
        {
            ALSA_WARNING("[ERR @ %s %d]\n", __FUNCTION__, __LINE__);
            return -EFAULT;
        }

        msleep(1);

        spin_lock_irqsave(&playback_lock, flags);
        writableSize = runtime->buffer_size - ring_valid_data(0
            , runtime->boundary, dpcm->nTotalRead, (long)runtime->control->appl_ptr);
        spin_unlock_irqrestore(&playback_lock, flags);
    }

    char *hwbuf = runtime->dma_area + frames_to_bytes(runtime, pos);
    if (copy_from_user(hwbuf, buf, frames_to_bytes(runtime, count)))
    {
        ALSA_WARNING("[ERR @ %s %d]\n", __FUNCTION__, __LINE__);
	    return -EFAULT;
    }
    return 0;
}
#else
static int snd_card_playback_copy(struct snd_pcm_substream *substream, int channel, snd_pcm_uframes_t pos, void __user *buf, snd_pcm_uframes_t count)
{
    //ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
    snd_pcm_runtime_t *runtime = substream->runtime;
    snd_card_RTK_pcm_t *dpcm = runtime->private_data;

    unsigned int writableSize = 0;
    unsigned int dataSize = 0;
    static int bufFullCount = 0;

    //check ring buffer available
    dpcm->decInRing_LE[0].readPtr[0] = (long)(ntohl(dpcm->decInRing[0].readPtr[0]));
    writableSize = valid_free_size(
        dpcm->decInRing_LE[0].beginAddr,
        dpcm->decInRing_LE[0].beginAddr + dpcm->decInRing_LE[0].size,
        dpcm->decInRing_LE[0].readPtr[0],
        dpcm->decInRing_LE[0].writePtr);

    dataSize = frames_to_bytes(runtime, count);

    while(dataSize > writableSize)
    {
        if(bufFullCount++ > 1000)
            return -EFAULT;

        msleep(1);
        dpcm->decInRing_LE[0].readPtr[0] = (long)(ntohl(dpcm->decInRing[0].readPtr[0]));
        writableSize = valid_free_size(
            dpcm->decInRing_LE[0].beginAddr,
            dpcm->decInRing_LE[0].beginAddr + dpcm->decInRing_LE[0].size,
            dpcm->decInRing_LE[0].readPtr[0],
            dpcm->decInRing_LE[0].writePtr);
    }

    //ALSA_VitalPrint("playback count %ld frames_to_bytes %d\n", pos, count, frames_to_bytes(runtime, pos));
    char *hwbuf = runtime->dma_area + frames_to_bytes(runtime, pos);
    if (copy_from_user(hwbuf, buf, frames_to_bytes(runtime, count)))
	    return -EFAULT;

    unsigned int nPeriodCount = 0;
    unsigned long HWRingFreeSize = 0;
    unsigned long HWRingFreeFrame = 0;

    dpcm->decInRing_LE[0].readPtr[0] = (long)(ntohl(dpcm->decInRing[0].readPtr[0]));//physical address

    // update wp
    nPeriodCount = ring_valid_data(0, runtime->boundary, dpcm->nTotalWrite, runtime->control->appl_ptr) / runtime->period_size;

    if(nPeriodCount == runtime->periods)
        nPeriodCount--;

    HWRingFreeSize = valid_free_size(
        dpcm->decInRing_LE[0].beginAddr,
        dpcm->decInRing_LE[0].beginAddr + dpcm->decInRing_LE[0].size,
        dpcm->decInRing_LE[0].readPtr[0],
        dpcm->decInRing_LE[0].writePtr);

    HWRingFreeFrame = bytes_to_frames(runtime, HWRingFreeSize);

    if((runtime->period_size * nPeriodCount) > HWRingFreeFrame)
        nPeriodCount = HWRingFreeFrame / runtime->period_size;

    if(HWRingFreeSize <= dpcm->nPeriodBytes)
        nPeriodCount = 0;

    if(nPeriodCount)
    {
        dpcm->decInRing_LE[0].writePtr = ring_add(
            dpcm->decInRing_LE[0].beginAddr,
            dpcm->decInRing_LE[0].beginAddr + dpcm->decInRing_LE[0].size,
            dpcm->decInRing_LE[0].writePtr,
            frames_to_bytes(runtime, runtime->period_size * nPeriodCount));
            //dataSize);

        dpcm->nTotalWrite = ring_add(
            0,
            runtime->boundary,
            dpcm->nTotalWrite,
            runtime->period_size * nPeriodCount);
            //count);

        //ALSA_VitalPrint("nPeriodCount %d runtime->period_size %d nTotalWrite %ld\n", nPeriodCount, runtime->period_size, dpcm->nTotalWrite);
        //update wp
        dpcm->decInRing[0].writePtr = htonl(dpcm->decInRing_LE[0].writePtr);//record physical address
        //ALSA_VitalPrint("update decInRing wp %x\n", htonl(dpcm->decInRing[0].writePtr));
    }

    return 0;
}
#endif

static long ring_valid_data(long ring_base, long ring_limit, long ring_rp, long ring_wp)
{
    if (ring_wp >= ring_rp)
    {
        return (ring_wp-ring_rp);
    }
    else
    {
        return (ring_limit-ring_base)-(ring_rp-ring_wp);
    }
}

static long ring_add(long ring_base, long ring_limit, long ptr, int bytes)
{
    ptr += bytes;
    if (ptr >= ring_limit)
    {
        ptr = ring_base + (ptr-ring_limit);
    }
    return ptr;
}

static int valid_free_size(long base, long limit, long rp, long wp)
{
    return (limit-base)-ring_valid_data(base,limit,rp,wp)-1;
}

static long buf_memcpy2_ring(long base, long limit, long ptr, char* buf, long size)
{
    if (ptr + size <= limit)
    {
        memcpy((char*)ptr,buf,size);
    }
    else
    {
        int i = limit-ptr;
        memcpy((char*)ptr,(char*)buf,i);
        memcpy((char*)base,(char*)(buf+i),size-i);
    }

    ptr += size;
    if (ptr >= limit)
    {
        ptr = base + (ptr-limit);
    }
    return ptr;
}

// create PCM instance
static int __init snd_card_create_PCM_instance(RTK_snd_card_t *pSnd, int instance_idx, int playback_substreams, int capture_substreams)
{
    snd_pcm_t *pcm = NULL;
    struct snd_pcm_substream *p = NULL;
    int err;
    int i;

    TRACE_CODE("[%s %s %d]\n", __FILE__, __FUNCTION__, __LINE__);

    // create PCM instance
    if ((err = snd_pcm_new(pSnd->card
        , snd_pcm_id[instance_idx] /* id string */
        , instance_idx
        , playback_substreams
        , capture_substreams,
        &pcm)) < 0)    // for Jupiter Capture Count = 0 (ie. there is no ain)
    {
        ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
        return err;
    }

    // set OPs
    switch(instance_idx)
    {
        case 0:
            snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_card_rtk_playback_ops);
            snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_card_rtk_capture_ops);
            sprintf(pcm->name, "RTK_SndCard_PCM_ADC");
            break;
        case 1:
            snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_card_rtk_capture_i2s_ops);
            sprintf(pcm->name, "RTK_SndCard_PCM_HDMI");
            break;
        default:
            ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
            break;
    }

    pcm->private_data = pSnd;
    pcm->info_flags = 0;

    p = pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream;
    for (i = 0; i < playback_substreams; i++)
    {
        p->dma_buffer.dev.dev = snd_dma_continuous_data(GFP_KERNEL);
#ifdef USE_ION_AUDIO_HEAP
        p->dma_buffer.dev.type = SNDRV_DMA_TYPE_ION_PLAYBACK;
#else
        p->dma_buffer.dev.type = SNDRV_DMA_TYPE_CONTINUOUS;
#endif
        p = p->next;
    }

    p = pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;
    for (i = 0; i < capture_substreams; i++)
    {
        p->dma_buffer.dev.dev = snd_dma_continuous_data(GFP_KERNEL);
#ifdef USE_ION_AUDIO_HEAP
        p->dma_buffer.dev.type = SNDRV_DMA_TYPE_ION_CAPTURE;
#else
        p->dma_buffer.dev.type = SNDRV_DMA_TYPE_CONTINUOUS;
#endif
        p = p->next;
    }

    pSnd->pcm = pcm;

#if 0
    /* NOTE: this may fail */
    if (err = snd_pcm_lib_preallocate_pages_for_all(pcm,
              SNDRV_DMA_TYPE_CONTINUOUS,
              snd_dma_continuous_data(GFP_KERNEL),
              32*1024,
              32*1024))
    {
        printk("snd_pcm_lib_preallocate_pages failed\n");
    }
#endif

    return 0;
}

void snd_realtek_hw_playback_volume_work(struct work_struct *work) {
    RTK_snd_card_t *mars = container_of(work, RTK_snd_card_t, work_volume);
    TRACE_CODE("[���� %s %d]\n", __FUNCTION__, __LINE__);
    RPC_TOAGENT_SET_VOLUME(mars->mixer_volume[MIXER_ADDR_MASTER][0]);
}

static int __init snd_card_mars_new_mixer(RTK_snd_card_t * pRTK_card)
{
    snd_card_t *card = pRTK_card->card;
    unsigned int idx;
    int err;

    spin_lock_init(&pRTK_card->mixer_lock);
    INIT_WORK(&pRTK_card->work_volume, snd_realtek_hw_playback_volume_work);

    strcpy(card->mixername, "RTK_Mixer");

    // add snc_control
    for (idx = 0; idx < ARRAY_SIZE(snd_mars_controls); idx++) {
        if ((err = snd_ctl_add(card, snd_ctl_new1(&snd_mars_controls[idx], pRTK_card))) < 0)
        {
            ALSA_WARNING("[snd_ctl_add faile %s %d]\n", __FUNCTION__, __LINE__);
            return err;
        }
    }
    return 0;
}

static int snd_RTK_volume_info(snd_kcontrol_t * kcontrol, snd_ctl_elem_info_t * uinfo)
{
    //printk("[ALSA %s %d\n", __FUNCTION__, __LINE__);
    uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
    // Jupiter only Master Volume can control
    uinfo->count = 2;
    uinfo->value.integer.min = 0;
    uinfo->value.integer.max = 31;
    return 0;
}

static int snd_RTK_volume_get(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
    RTK_snd_card_t *mars = snd_kcontrol_chip(kcontrol);
    unsigned long flags;
    int addr = kcontrol->private_value;

    //printk("[ALSA %s %d\n", __FUNCTION__, __LINE__);

    spin_lock_irqsave(&mars->mixer_lock, flags);
    switch(addr) {
        case MIXER_ADDR_MASTER:
            mars->mixer_volume[addr][1] = mars->mixer_volume[addr][0] ;
            break;
        default:
            break;
    }
    ucontrol->value.integer.value[0] = mars->mixer_volume[addr][0];
    ucontrol->value.integer.value[1] = mars->mixer_volume[addr][1];
    spin_unlock_irqrestore(&mars->mixer_lock, flags);
    return 0;
}

#define cpy_func(dst, src, size, dmem_buf) memcpy(dst, src, size)
static void ring1_to_ring2_general(AUDIO_RINGBUF_PTR* ring1, AUDIO_RINGBUF_PTR* ring2, long size, char* dmem_buf)
{
    if (ring1->rp + size <= ring1->limit)
    {
        if (ring2->wp + size <= ring2->limit)
        {

            cpy_func((char*)ring2->wp,(char*)ring1->rp,size,dmem_buf);
        }
        else
        {
            int i = ring2->limit-ring2->wp;
            cpy_func((char*)ring2->wp,(char*)ring1->rp,i,dmem_buf);
            cpy_func((char*)ring2->base,(char*)(ring1->rp+i),size-i,dmem_buf);

        }
    }
    else
    {
        if (ring2->wp + size <= ring2->limit)
        {
            int i = ring1->limit-ring1->rp;
            cpy_func((char*)ring2->wp,(char*)ring1->rp,i,dmem_buf);
            cpy_func((char*)(ring2->wp+i),(char*)(ring1->base),size-i,dmem_buf);
        }
        else
        {
            int i,j;
            i = ring1->limit-ring1->rp;
            j = ring2->limit-ring2->wp;
            if (j <= i)
            {
                cpy_func((char*)ring2->wp,(char*)ring1->rp,j,dmem_buf);
                cpy_func((char*)ring2->base,(char*)(ring1->rp+j),i-j,dmem_buf);
                cpy_func((char*)(ring2->base+i-j),(char*)(ring1->base),size-i,dmem_buf);
            }
            else
            {
                cpy_func((char*)ring2->wp,(char*)ring1->rp,i,dmem_buf);
                cpy_func((char*)(ring2->wp+i),(char*)ring1->base,j-i,dmem_buf);
                cpy_func((char*)ring2->base,(char*)(ring1->base+j-i),size-j,dmem_buf);
            }
        }
    }

    ring1->rp += size;
    if (ring1->rp >= ring1->limit)
    {
        ring1->rp = ring1->base + (ring1->rp-ring1->limit);
    }

    ring2->wp += size;
    if (ring2->wp >= ring2->limit)
    {
        ring2->wp = ring2->base + (ring2->wp-ring2->limit);
    }
}

static int snd_RTK_volume_put(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
    RTK_snd_card_t *mars = snd_kcontrol_chip(kcontrol);
    unsigned long flags;
    int change, addr = kcontrol->private_value;
    int master;

    //printk("[ALSA %s %d\n", __FUNCTION__, __LINE__);

    master = ucontrol->value.integer.value[0];
    if (master < 0) {
        master = 0;
    }
    if (master > 31) {
        master = 31;
    }

    spin_lock_irqsave(&mars->mixer_lock, flags);
    change = mars->mixer_volume[addr][0] != master;
    mars->mixer_volume[addr][0] = master;
    mars->mixer_volume[addr][1] = master;
    spin_unlock_irqrestore(&mars->mixer_lock, flags);
    if (addr ==MIXER_ADDR_MASTER) {
        schedule_work(&mars->work_volume);
    }

    return change;
}

static int snd_RTK_capsrc_info(snd_kcontrol_t * kcontrol, snd_ctl_elem_info_t * uinfo)
{
    //printk("[ALSA %s %d\n", __FUNCTION__, __LINE__);
    uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
    uinfo->count = 2;
    uinfo->value.integer.min = 0;
    uinfo->value.integer.max = 1;
    return 0;
}

static int snd_RTK_capsrc_get(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
    RTK_snd_card_t *mars = snd_kcontrol_chip(kcontrol);
    unsigned long flags;
    int addr = kcontrol->private_value;

    //printk("[ALSA %s %d\n", __FUNCTION__, __LINE__);
    spin_lock_irqsave(&mars->mixer_lock, flags);
    ucontrol->value.integer.value[0] = mars->capture_source[addr][0];
    ucontrol->value.integer.value[1] = mars->capture_source[addr][1];
    spin_unlock_irqrestore(&mars->mixer_lock, flags);
    return 0;
}

static int snd_RTK_capsrc_put(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
    RTK_snd_card_t *mars = snd_kcontrol_chip(kcontrol);
    unsigned long flags;
    int change, addr = kcontrol->private_value;
    int left, right;
    //printk("[ALSA %s %d\n", __FUNCTION__, __LINE__);

    left = ucontrol->value.integer.value[0] & 1;
    right = ucontrol->value.integer.value[1] & 1;
    spin_lock_irqsave(&mars->mixer_lock, flags);
    change = mars->capture_source[addr][0] != left &&
             mars->capture_source[addr][1] != right;
    mars->capture_source[addr][0] = left;
    mars->capture_source[addr][1] = right;
    spin_unlock_irqrestore(&mars->mixer_lock, flags);
    return change;
}

#ifdef CONFIG_SYSFS

static ssize_t alsa_latency_show(struct kobject *kobj, struct kobject_attribute *attr, char *buf)
{
    int mLatency = 0;
    mLatency = (int)snd_monitor_audio_data_queue();

    return sprintf(buf, "%u\n", mLatency);
}

static ssize_t alsa_active_show(struct kobject *kobj, struct kobject_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", rtk_dec_ao_buffer);
}

static ssize_t alsa_active_store(struct kobject *kobj, struct kobject_attribute *attr, const char *buf, size_t count)
{
    int ret = 0;

    unsigned long val;

    if (strict_strtoul(buf, 10, &val) < 0)
        return -EINVAL;

    //ALSA_VitalPrint("rtk_dec_ao value %d\n", val);
    if (val >= 4096 && val <= (12*1024))
        rtk_dec_ao_buffer = val;
    else
        ALSA_WARNING("set dec_ao_buffer failed! (size must be from 4k to 12k)\n");

    return count;
}

static struct kobj_attribute alsa_active_attr =
    __ATTR(dec_ao_buffer_size, 0644, alsa_active_show, alsa_active_store);

static struct kobj_attribute alsa_latency_attr =
    __ATTR(latency, 0444, alsa_latency_show, NULL);

static struct attribute *alsa_attrs[] = {
    &alsa_active_attr.attr,
    &alsa_latency_attr.attr,
    NULL,
};

static struct attribute_group rtk_alsa_attr_group = {
    .attrs = alsa_attrs,
};

static struct kobject *alsa_kobj;

static int __init alsa_sysfs_init(void)
{
    int ret;

    alsa_kobj = kobject_create_and_add("rtk_alsa", kernel_kobj);
    if (!alsa_kobj)
        return -ENOMEM;
    ret = sysfs_create_group(alsa_kobj, &rtk_alsa_attr_group);
    if (ret)
        kobject_put(alsa_kobj);
    return ret;
}
#endif

static int snd_card_probe(struct platform_device *devptr)
{
    snd_card_t *card;
    RTK_snd_card_t *pRTKSnd;
    int dev = devptr->id;
    int idx, err;

    TRACE_CODE("[%s %s %d] dev %d\n", __FILE__, __FUNCTION__, __LINE__, dev);

    // create sound card
    err = snd_card_create(
        -1, /* -1 ; idx == -1 == 0xffff means: take any free slot*/
        "RTK_AlsaCard", /* describe ID string of sound card */
        THIS_MODULE,
        sizeof(RTK_snd_card_t), // size of private_data
        &card);
    if (err < 0)
        return err;

    // private data
    pRTKSnd = (RTK_snd_card_t *)card->private_data;
    pRTKSnd->card = card;

    for (idx = 0; idx < MAX_PCM_DEVICES ; idx++)
    {
        // create PCM instance
        if ((err = snd_card_create_PCM_instance(pRTKSnd, idx, pcm_substreams[dev], pcm_capture_substreams[dev])) < 0)
        {
            ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
            goto __nodev;
        }

        // create Compress instance
        if ((err = snd_card_create_compress_instance(pRTKSnd, idx)) < 0) {
            ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
            goto __nodev;
        }
    }

#ifdef RTK_ENABLE_I2S
    // create another instance ONLY for caputer AI I2S in
    snd_card_create_PCM_instance(pRTKSnd, 1, 0, 1);
#endif

    if ((err = snd_card_mars_new_mixer(pRTKSnd)) < 0)
        goto __nodev;

    strcpy(card->driver, "RTK");
    strcpy(card->shortname, "RTK");
    sprintf(card->longname, "REALTEK SndCard ALSA");

    if ((err = snd_card_register(card)) == 0) {
        snd_RTK_cards[dev] = card;
        platform_set_drvdata(devptr, card);
        return 0;
    }

    // error handling
__nodev:
    ALSA_WARNING("[%s %d fail]\n", __FUNCTION__, __LINE__);
    snd_card_free(card);
    return err;
}

static int snd_card_remove(struct platform_device *devptr)
{
    struct snd_card *card = platform_get_drvdata(devptr);

    if (card) {
        snd_card_free(card);
        platform_set_drvdata(devptr, NULL);
    }

    return 0;
}

#ifdef CONFIG_PM
static int rtk_alsa_suspend(struct device *pdev)
{
    ALSA_VitalPrint("[+]%s %d\n", __FUNCTION__, __LINE__);
    struct snd_card *card = dev_get_drvdata(pdev);

    //snd_card_t *card = snd_RTK_cards[0];
    struct RTK_snd_card *pRTKSnd = card->private_data;

    snd_power_change_state(card, SNDRV_CTL_POWER_D3hot);

    is_suspend = true;
    snd_pcm_suspend_all(pRTKSnd->pcm);

    ALSA_VitalPrint("[-]%s %d\n", __FUNCTION__, __LINE__);
    return 0;
}

static int rtk_alsa_resume(struct device *pdev)
{
    ALSA_VitalPrint("[+]%s %d\n", __FUNCTION__, __LINE__);

    struct snd_card *card = dev_get_drvdata(pdev);

    snd_power_change_state(card, SNDRV_CTL_POWER_D0);
    is_suspend = false;

    ALSA_VitalPrint("[-]%s %d\n", __FUNCTION__, __LINE__);
    return 0;
}

static SIMPLE_DEV_PM_OPS(rtk_alsa_pm, rtk_alsa_suspend, rtk_alsa_resume);
#define RTK_ALSA_PM_OPS &rtk_alsa_pm
#else
#define RTK_ALSA_PM_OPS NULL
#endif


#define SND_REALTEK_DRIVER "snd_alsa_rtk"

static struct platform_driver rtk_alsa_driver = {
    .probe      = snd_card_probe,
    .remove     = snd_card_remove,
    .driver     = {
        .name   = SND_REALTEK_DRIVER,
        .owner  = THIS_MODULE,
        .pm     = RTK_ALSA_PM_OPS,
    },
};

static void rtk_alsa_unregister_all(void)
{
    int i;

    ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);

    for(i = 0; i < ARRAY_SIZE(devices); ++i)
        platform_device_unregister(devices[i]);
    platform_driver_unregister(&rtk_alsa_driver);
}

static int __init RTK_alsa_card_init(void)
{
    //ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);
    int i, cards, ret, err;

    err = platform_driver_register(&rtk_alsa_driver);
    if (err < 0)
        return err;

#ifdef USE_ION_AUDIO_HEAP
    alsa_client = ion_client_create(rtk_phoenix_ion_device, "ALSADriver");
#endif

#ifdef CONFIG_SYSFS
    ret = alsa_sysfs_init();
    if (ret)
        ALSA_VitalPrint("%s: unable to create sysfs entry\n", __FUNCTION__);
#endif

    cards = 0;
    for (i = 0; i < SNDRV_CARDS && snd_card_enable[i]; i++)
    {
        struct platform_device *device;
        device = platform_device_register_simple(SND_REALTEK_DRIVER, i, NULL, 0);

        if (IS_ERR(device))
        {
            continue;
        }

        if (!platform_get_drvdata(device)) {
            platform_device_unregister(device);
            continue;
        }

        devices[i] = device;
        cards++;
    }

    if (!cards)
    {
        ALSA_VitalPrint("[ALSA %s %d fail]\n", __FUNCTION__, __LINE__);
        rtk_alsa_unregister_all();
        return -ENODEV;
    }

    return 0;
}

static void __exit RTK_alsa_card_exit(void)
{
    //ALSA_VitalPrint("[ALSA %s %d]\n", __FUNCTION__, __LINE__);

#ifdef USE_ION_AUDIO_HEAP
    if (alsa_client != NULL)
    {
        printk("%s ion_client_destroy\n", __FUNCTION__);
        ion_client_destroy(alsa_client);
    }
#endif

    rtk_alsa_unregister_all();
}

module_init(RTK_alsa_card_init)
module_exit(RTK_alsa_card_exit)
