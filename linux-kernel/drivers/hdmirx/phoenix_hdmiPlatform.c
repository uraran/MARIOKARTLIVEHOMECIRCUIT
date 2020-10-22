//#include <stdio.h>
//#include <stdlib.h>
//#include <stdbool.h>
//#include <sched.h>
//#include <pthread.h>
#include "phoenix_hdmiInternal.h"
//#include "Add_in_common/hdmiSetForScaler.h"

//static bool hdmiPhyEnable = false;
//static pthread_t thread_id_hdmiPhy;
//static void* hdmiPhyThread(void* arg);

/**
 * Hdmi_VideoOutputDisable
 * Disable HDMI Video Output
 *
 * @param
 * @return
 * @ingroup drv_hdmi
 */
void Hdmi_VideoOutputDisable(void)
{
	hdmi_rx_reg_mask32(HDMI_HDMI_AVMCR_VADDR,~_BIT3,0x00, HDMI_RX_MAC);
}

unsigned char HdmiIsAudioSpdifIn(void) {

#if 0
	AIO_PATH_CFG audioCfg;

	audioCfg.id = AIO_PATH_ID_PB;

	Audio_AioGetPathSrc(&audioCfg);

	return (audioCfg.src[0] == AIO_PATH_SRC_SPDIF);
#else
	return 1;
#endif

}

void HdmiGetStruct(HDMIRX_IOCTL_STRUCT_T* ptr) {
	rtd_hdmiRx_cmd(IOCTRL_HDMI_GET_STRUCT, ptr);
}

void HdmiAudioPLLSampleStart(void) {
	rtd_hdmiRx_cmd(IOCTRL_HDMI_AUDIO_PLL_SAMPLE_START, NULL);
}

void HdmiAudioPLLSampleStop(void) {
	rtd_hdmiRx_cmd(IOCTRL_HDMI_AUDIO_PLL_SAMPLE_STOP, NULL);
}

void HdmiAudioPLLSampleDump(void) {
	rtd_hdmiRx_cmd(IOCTRL_HDMI_AUDIO_PLL_SAMPLE_DUMP, NULL);
}

void HdmiSetPreviousSource(int channel) {

#if 0//HDMI_FASTER_BOOT
	FILE *fd;
	int tmp;
        pthread_mutex_lock(&hdmi_file_mutex);
	fd = fopen("/data/save/previous_hdmi_source", "rb");
	if (fd != NULL) {
		fread(&tmp, sizeof(tmp), 1, fd);
		fclose(fd);
		if (channel == tmp) {
			 pthread_mutex_unlock(&hdmi_file_mutex);
 			return;
		}
	}

	fd = fopen("/data/save/previous_hdmi_source", "wb");
	if (fd ==NULL ) {

	} else {
		fwrite(&channel, sizeof(channel),1, fd);
		fclose(fd);
	}
	 pthread_mutex_unlock(&hdmi_file_mutex);

#endif

}

void HdmiGetFastBootStatus(void) {

	#if HDMI_FASTER_BOOT
	FILE *fd;
	fd = fopen("/bin/hdmi_fast_boot_source", "rb");
	if (fd ==NULL ) {
		hdmi.fast_boot_source = HDMI_MAX_CHANNEL;
	} else {
		fread(&hdmi.fast_boot_source, sizeof(hdmi.fast_boot_source),1, fd);
		fclose(fd);
	}
	HDMI_PRINTF("HDMI Fast Boot Source = %d\n", hdmi.fast_boot_source);
      #endif

}

void HdmiSetAPStatus(HDMIRX_APSTATUS_T apstatus) {
	unsigned int value = apstatus;
	rtd_hdmiRx_cmd(IOCTRL_HDMI_SET_APSTATUS, (void *)value);
}

void Hdmi_SetTimer10ms(unsigned short value) {

	rtd_hdmiRx_cmd(IOCTRL_HDMI_SET_TIMER, (void *) &value);


}
unsigned short Hdmi_GetTimer10ms(void) {
	unsigned short value;
	rtd_hdmiRx_cmd(IOCTRL_HDMI_GET_TIMER, (void *) &value);
	return value;
}

