#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/syscalls.h> /* needed for the _IOW etc stuff used later */
#include <linux/mpage.h>
#include <linux/dcache.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <sound/asound.h>
#include <asm/cacheflush.h>
#include <linux/slab.h>

#include <RPCDriver.h>
#include <linux/dma-mapping.h>
#include "hdmitx_rpc.h"
#include <../driver/dc2vo/dc_rpc.h>
#include "hdmitx.h"
#include "hdmitx_dev.h"
#include "hdmitx_reg.h"
#include "rtk_edid.h"
#include "crt_reg.h"

#ifdef USE_ION_AUDIO_HEAP
#include "../../../staging/android/ion/ion.h"
#include "../../../staging/android/uapi/rtk_phoenix_ion.h"

struct ion_client *rpc_ion_client;
extern struct ion_device *rtk_phoenix_ion_device;
#endif

#ifdef AO_ON_SCPU 
void rtk_ao_kernel_rpc(
int type, 
unsigned char *pArg, unsigned int size_pArg,
unsigned char *pRet, unsigned int size_pRet,
unsigned char *pFuncRet, unsigned int size_pFuncRet);
#define htonl(x) (x)
#endif

enum HDMI_AVMUTE{
	HDMI_CLRAVM =0,
	HDMI_SETAVM 
};

#define CONVERT_FOR_AVCPU(x)        ((unsigned int)(x) | 0xA0000000)

#if 0
#define VO_RESOLUTION (0x18070000 + 0x1100 ) // VO_RESOLUTION_INFO

typedef struct{
    unsigned int magic;  //0x11223344 means valid data
    unsigned int width;  //vo resolution: width
    unsigned int height; //vo resolution: height
} VO_RESOLUTION_INFO ;

static  VO_RESOLUTION_INFO * vo_res_info = NULL;
#endif

static int hdmi_error;
unsigned int tmds_en;

void hdmitx_print_sink_info(asoc_hdmi_t *p_this);

int hdmitx_check_rx_sense(void __iomem *reg_base)
{
	int val ;

	val = field_get(rd_reg(reg_base + MHL_CBUS),7,0); 
	HDMI_INFO("MHL_CBUS=%x",val);
				
	if(val == 0xff)	
		return HDMI_RX_SENSE_ON;
	else
		return HDMI_RX_SENSE_OFF;						
}

int hdmitx_check_tmds_src(void __iomem *reg_base)
{
	int val ;
	int data;

    if(tmds_en ==1)
	{
		val = rd_reg(reg_base + CR);
		HDMI_INFO("CR=%x",val);
		
		if( val == 0x15)
			return TMDS_HDMI_ENABLED;
		else if (val == 0x0)
			return TMDS_HDMI_DISABLED;
		else if (val == 0x11)
			return TMDS_MHL_ENABLED;
		else
			return TMDS_MODE_UNKNOW;
	}
	else
	{
		val = rd_reg(reg_base + TMDS_SCR2);
		data = TMDS_SCR2_REG_TX_PU_src(val);
		HDMI_INFO("TMDS_SCR2=%x",val);
		
		if( data == 0xF)
			return TMDS_HDMI_ENABLED;
		else if (data == 0x0)
			return TMDS_HDMI_DISABLED;
		else if (data == 0x2)
			return TMDS_MHL_ENABLED;
		else
			return TMDS_MODE_UNKNOW;
		
	
	}
		
}

void hdmitx_turn_off_tmds(int vo_mode)
{
#if 1
	void __iomem * base;
	
	if(!tmds_en)
	{
		HDMI_INFO("skip %s",__FUNCTION__);
		return;
	}

	base = ioremap(0x1800d000, 0x4);
	wr_reg(base + CR, 0x2A);
	HDMI_INFO("CR OFF = %x",rd_reg(base + CR));
	iounmap(base);
#endif	             
}

int hdmitx_send_AVmute(void __iomem *reg_base, int flag)
{
	
	if (flag == HDMI_SETAVM){
		
		HDMI_DEBUG("set av mute");
		
		wr_reg((reg_base+ GCPCR), GCPCR_enablegcp(1) |
								  GCPCR_gcp_clearavmute(1) |
								  GCPCR_gcp_setavmute(1) |
								  GCPCR_write_data(0));
		wr_reg((reg_base+ GCPCR), GCPCR_enablegcp(1) |
								  GCPCR_gcp_clearavmute(0) |
								  GCPCR_gcp_setavmute(1) |
								  GCPCR_write_data(1));
			
		return 0;			 
	}		
	else if (flag == HDMI_CLRAVM){
		
		HDMI_DEBUG("clear av mute");
		
		wr_reg((reg_base + GCPCR), GCPCR_enablegcp(1) |
								   GCPCR_gcp_clearavmute(1) |
					               GCPCR_gcp_setavmute(1) |
								   GCPCR_write_data(0));
		wr_reg((reg_base + GCPCR), GCPCR_enablegcp(1) |
								   GCPCR_gcp_clearavmute(1) |
								   GCPCR_gcp_setavmute(0) |
								   GCPCR_write_data(1));
		return 0;
					 
	}else{
	
		HDMI_DEBUG("unknown av mute parameter");
		return 1;
	}	
}


#ifdef USE_ION_AUDIO_HEAP
	
void register_ion_client(const char *name)
{
    rpc_ion_client = ion_client_create(rtk_phoenix_ion_device, name);
	
}

void deregister_ion_client(const char *name)
{
    if (rpc_ion_client != NULL)
    {
        HDMI_INFO("%s :%s", __FUNCTION__,name);
        ion_client_destroy(rpc_ion_client);
    }
}

#ifdef VIDEO_FIRMWARE_ON_SCPU
extern HRESULT *VIDEO_RPC_VOUT_ToAgent_ConfigTVSystem_0_svc (VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM *argp, RPC_STRUCT *rpc, HRESULT *retval);

extern HRESULT *VIDEO_RPC_VOUT_ToAgent_ConfigHdmiInfoFrame_0_svc (VIDEO_RPC_VOUT_CONFIG_HDMI_INFO_FRAME *argp, RPC_STRUCT *rpc, HRESULT *retval);
/*
extern VIDEO_RPC_VOUT_QUERY_DISP_WIN_OUT *VIDEO_RPC_VOUT_ToAgent_QueryDisplayWin_0_svc (VIDEO_RPC_VOUT_QUERY_DISP_WIN_IN *argp, RPC_STRUCT *rpc, HRESULT *retval);
*/
unsigned long vo_scpu_handler(unsigned long command, unsigned long param1, unsigned long param2)
{


    unsigned long ret ;

    pr_warn("CW:$$$[KERNEL-AUDIO RPC]%x%x%x\n",command,param1,param2);

    switch(command) // command def in "enum ENUM_AUDIO_KERNEL_RPC_CMD" in amw_kernel_rpc.h
   {
    /*    case ENUM_KERNEL_RPC_HDMI_OUT_EDID2:
        {
            AUDIO_HDMI_OUT_EDID_DATA2 *info = (AUDIO_HDMI_OUT_EDID_DATA2 *)param1;
            HRESULT *retval = (HRESULT *)param2;
            KRPC_PRINT("[ENUM_KERNEL_RPC_HDMI_OUT_EDID2]\n");
            AUDIO_RPC_ToAgent_HDMI_OUT_EDID2_0_svc(info, NULL, retval);
            break;
        }
*/
       	case ENUM_VIDEO_KERNEL_RPC_CONFIG_TV_SYSTEM:
		{
            VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM *info = (VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM *)param1;
            HRESULT *retval = (HRESULT *)param2;
            pr_warn("CW in hdmitx_api[ENUM_VIDEO_KERNEL_RPC_CONFIG_TV_SYSTEM]\n");
            retval = VIDEO_RPC_VOUT_ToAgent_ConfigTVSystem_0_svc(info, NULL, retval);
            break;
		}

		case ENUM_VIDEO_KERNEL_RPC_CONFIG_HDMI_INFO_FRAME:
		{
            VIDEO_RPC_VOUT_CONFIG_HDMI_INFO_FRAME *info = (VIDEO_RPC_VOUT_CONFIG_HDMI_INFO_FRAME *)param1;
            HRESULT *retval = (HRESULT *)param2;
            pr_warn("[ENUM_VIDEO_KERNEL_RPC_CONFIG_HDMI_INFO_FRAME]\n");
            retval = VIDEO_RPC_VOUT_ToAgent_ConfigHdmiInfoFrame_0_svc(info, NULL, retval);
            break;
		}
		/*
        case ENUM_VIDEO_KERNEL_RPC_QUERY_DISPLAY_WIN:
        {
            VIDEO_RPC_VOUT_QUERY_DISP_WIN_IN *info = (VIDEO_RPC_VOUT_QUERY_DISP_WIN_IN *)param1;
            VIDEO_RPC_VOUT_QUERY_DISP_WIN_OUT *retval = (VIDEO_RPC_VOUT_QUERY_DISP_WIN_OUT *)param2;
            KRPC_PRINT("[ENUM_VIDEO_KERNEL_RPC_QUERY_DISPLAY_WIN]\n");
            retval = VIDEO_RPC_VOUT_ToAgent_QueryDisplayWin_0_svc(info, NULL, retval);
            break;
        }*/
	  
    }


    return ret;

}

#endif



#ifdef __RTK_HDMI_GENERIC_DEBUG__
void hdmitx_dump_VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM(struct VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM *arg, char *str) 
{	
	HDMI_DEBUG("%s", __func__);	
    HDMI_INFO( "interfaceType 		=0x%x", arg-> interfaceType);
    
	HDMI_INFO( "videoInfo.standard  =0x%x", arg-> videoInfo.standard);
	HDMI_INFO( "videoInfo.enProg 	=0x%x", arg-> videoInfo.enProg);
	HDMI_INFO( "videoInfo.enDIF 	=0x%x", arg-> videoInfo.enDIF);
	HDMI_INFO( "videoInfo.enCompRGB =0x%x", arg-> videoInfo.enCompRGB);
	HDMI_INFO( "videoInfo.pedType   =0x%x", arg-> videoInfo.pedType);
	HDMI_INFO( "videoInfo.dataInt0  =0x%x", arg-> videoInfo.dataInt0);
	HDMI_INFO( "videoInfo.dataInt1  =0x%x", arg-> videoInfo.dataInt1);
		
	HDMI_INFO( "hdmiInfo.hdmiMode 			=0x%x", arg-> hdmiInfo.hdmiMode);
    HDMI_INFO( "hdmiInfo.audioSampleFreq 	=0x%x", arg-> hdmiInfo.audioSampleFreq);	
	HDMI_INFO( "hdmiInfo.audioChannelCount  =0x%x", arg-> hdmiInfo.audioChannelCount);
	HDMI_INFO( "hdmiInfo.dataByte1 			=0x%x", arg-> hdmiInfo.dataByte1);
	HDMI_INFO( "hdmiInfo.dataByte2 			=0x%x", arg-> hdmiInfo.dataByte2);
	HDMI_INFO( "hdmiInfo.dataByte3 			=0x%x", arg-> hdmiInfo.dataByte3);
	HDMI_INFO( "hdmiInfo.dataByte4 			=0x%x", arg-> hdmiInfo.dataByte4);
	HDMI_INFO( "hdmiInfo.dataByte5 			=0x%x", arg-> hdmiInfo.dataByte5);		
	HDMI_INFO( "hdmiInfo.dataInt0 			=0x%x", arg-> hdmiInfo.dataInt0);
	HDMI_INFO( "hdmiInfo.reserved1 			=0x%lx",arg-> hdmiInfo.reserved1);
	HDMI_INFO( "hdmiInfo.reserved2 			=0x%lx",arg-> hdmiInfo.reserved2);
	HDMI_INFO( "hdmiInfo.reserved3 			=0x%lx",arg-> hdmiInfo.reserved3);
	HDMI_INFO( "hdmiInfo.reserved4 			=0x%lx",arg-> hdmiInfo.reserved4);
	
}

void hdmitx_dump_AUDIO_HDMI_OUT_EDID_DATA2(struct AUDIO_HDMI_OUT_EDID_DATA2 *arg, char *str) 
{	
	HDMI_DEBUG("%s", __func__);	
    HDMI_INFO( "Version 		 	=0x%lx", arg-> Version);    
	HDMI_INFO( "HDMI_output_enable  =0x%lx", arg-> HDMI_output_enable);
	HDMI_INFO( "EDID_DATA_addr 		=0x%lx", arg-> EDID_DATA_addr);
	
}
#endif

int RPC_TOAGENT_HDMI_Config_TV_System(struct VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM *arg) 
{	

#if 1
	struct VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM *rpc = NULL;
    int ret = -1;
	unsigned long RPC_ret;    
	struct ion_handle *handle = NULL;
    ion_phys_addr_t dat;
    size_t len;
    
    handle = ion_alloc(rpc_ion_client, 4096, 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, ION_FLAG_NONCACHED |
																					ION_FLAG_SCPUACC | 
																					ION_FLAG_ACPUACC);
    
	if (IS_ERR(handle)) {
        HDMI_ERROR("[%s %d ion_alloc fail]", __FUNCTION__, __LINE__);
        goto exit;
    }

    if(ion_phys(rpc_ion_client, handle, &dat, &len) != 0) {
        HDMI_ERROR("[%s %d fail]", __FUNCTION__, __LINE__);
        goto exit;
    }

    rpc = ion_map_kernel(rpc_ion_client, handle);
	

	memcpy(rpc,arg,sizeof(struct VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM));  
	
#ifndef VIDEO_FIRMWARE_ON_SCPU
	rpc-> interfaceType = htonl(arg-> interfaceType);
	
	rpc-> videoInfo.standard = htonl(arg-> videoInfo.standard);
	rpc-> videoInfo.pedType  = htonl(arg-> videoInfo.pedType);
    rpc-> videoInfo.dataInt0 = htonl(arg-> videoInfo.dataInt0);
	rpc-> videoInfo.dataInt1 = htonl(arg-> videoInfo.dataInt1);
	
	rpc-> hdmiInfo.hdmiMode  = htonl(arg-> hdmiInfo.hdmiMode);
    rpc-> hdmiInfo.audioSampleFreq = htonl(arg-> hdmiInfo.audioSampleFreq);	
	rpc-> hdmiInfo.dataInt0  = htonl(arg-> hdmiInfo.dataInt0);
    rpc-> hdmiInfo.reserved1 = htonl(arg-> hdmiInfo.reserved1);
    rpc-> hdmiInfo.reserved2 = htonl(arg-> hdmiInfo.reserved2);
    rpc-> hdmiInfo.reserved3 = htonl(arg-> hdmiInfo.reserved3);
    rpc-> hdmiInfo.reserved4 = htonl(arg-> hdmiInfo.reserved4);

    if (send_rpc_command(RPC_AUDIO, 
        ENUM_VIDEO_KERNEL_RPC_CONFIG_TV_SYSTEM,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU((dat +sizeof(struct VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM))),
        &RPC_ret)) 
    {
        HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
        goto exit;
    }

#else
    //VO on SCPU
    rpc-> interfaceType =arg-> interfaceType;
	
	rpc-> videoInfo.standard = arg-> videoInfo.standard;
	rpc-> videoInfo.pedType  = arg-> videoInfo.pedType;
    rpc-> videoInfo.dataInt0 = arg-> videoInfo.dataInt0;
	rpc-> videoInfo.dataInt1 = arg-> videoInfo.dataInt1;
	
	rpc-> hdmiInfo.hdmiMode  = arg-> hdmiInfo.hdmiMode;
    rpc-> hdmiInfo.audioSampleFreq = arg-> hdmiInfo.audioSampleFreq;	
	rpc-> hdmiInfo.dataInt0  = arg-> hdmiInfo.dataInt0;
    rpc-> hdmiInfo.reserved1 = arg-> hdmiInfo.reserved1;
    rpc-> hdmiInfo.reserved2 = arg-> hdmiInfo.reserved2;
    rpc-> hdmiInfo.reserved3 = arg-> hdmiInfo.reserved3;
    rpc-> hdmiInfo.reserved4 = arg-> hdmiInfo.reserved4;

     vo_scpu_handler(ENUM_VIDEO_KERNEL_RPC_CONFIG_TV_SYSTEM, rpc, rpc + sizeof(struct VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM));

#endif



    ret = 0;
exit:
    if(handle != NULL) {
        ion_unmap_kernel(rpc_ion_client, handle);
        ion_free(rpc_ion_client, handle);
    }
    return ret;
#endif
        return 0;	
}

int RPC_TOAGENT_HDMI_Config_AVI_Info(struct VIDEO_RPC_VOUT_CONFIG_HDMI_INFO_FRAME *arg) 
{	
#if 1	
	struct VIDEO_RPC_VOUT_CONFIG_HDMI_INFO_FRAME *rpc = NULL;
    int ret = -1;
    unsigned long RPC_ret;
    struct ion_handle *handle = NULL;
    ion_phys_addr_t dat;
    size_t len;
    
	handle = ion_alloc(rpc_ion_client, 4096, 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, ION_FLAG_NONCACHED |
																					ION_FLAG_SCPUACC | 
																					ION_FLAG_ACPUACC);
	
	if (IS_ERR(handle)) {
	    HDMI_ERROR("[%s %d ion_alloc fail]", __FUNCTION__, __LINE__);
	    goto exit;
	}
	
	if(ion_phys(rpc_ion_client, handle, &dat, &len) != 0) {
	    HDMI_ERROR("[%s %d fail]", __FUNCTION__, __LINE__);
	    goto exit;
	}
	
	rpc = ion_map_kernel(rpc_ion_client, handle);
	
	memcpy(rpc,arg,sizeof(struct VIDEO_RPC_VOUT_CONFIG_HDMI_INFO_FRAME)); 
#ifndef VIDEO_FIRMWARE_ON_SCPU
	rpc-> hdmiMode  = htonl(arg-> hdmiMode);
    rpc-> audioSampleFreq = htonl(arg-> audioSampleFreq);
    rpc-> dataInt0 =  htonl(arg-> dataInt0);
    rpc-> reserved1 = htonl(arg-> reserved1);
    rpc-> reserved2 = htonl(arg-> reserved2);
    rpc-> reserved3 = htonl(arg-> reserved3);
    rpc-> reserved4 = htonl(arg-> reserved4);
	
	 if (send_rpc_command(RPC_AUDIO, 
        ENUM_VIDEO_KERNEL_RPC_CONFIG_HDMI_INFO_FRAME,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU((dat + sizeof(struct VIDEO_RPC_VOUT_CONFIG_HDMI_INFO_FRAME))),
        &RPC_ret)) 
    {
        HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
        goto exit;
    }
#else
    //vo on scpu
	rpc-> hdmiMode  = arg-> hdmiMode;
    rpc-> audioSampleFreq = arg-> audioSampleFreq;
    rpc-> dataInt0 =  arg-> dataInt0;
    rpc-> reserved1 = arg-> reserved1;
    rpc-> reserved2 = arg-> reserved2;
    rpc-> reserved3 = arg-> reserved3;
    rpc-> reserved4 = arg-> reserved4;
	
	vo_scpu_handler(ENUM_VIDEO_KERNEL_RPC_CONFIG_HDMI_INFO_FRAME, rpc, rpc + sizeof(struct VIDEO_RPC_VOUT_CONFIG_HDMI_INFO_FRAME));
#endif
    


    ret = 0;
exit:
	if(handle != NULL) {
        ion_unmap_kernel(rpc_ion_client, handle);
        ion_free(rpc_ion_client, handle);
    }
    return ret;
#endif
        return 0;	
}

int RPC_TOAGENT_HDMI_Set(struct AUDIO_HDMI_SET *arg) 
{	
#if 1	
	struct AUDIO_HDMI_SET *rpc = NULL;
    int ret = -1;
    unsigned long RPC_ret;
    struct ion_handle *handle = NULL;
    ion_phys_addr_t dat;
    size_t len;
   
	
	handle = ion_alloc(rpc_ion_client, 4096, 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, ION_FLAG_NONCACHED |
																					ION_FLAG_SCPUACC | 
																					ION_FLAG_ACPUACC);
	
	if (IS_ERR(handle)) {
	    HDMI_ERROR("[%s %d ion_alloc fail]", __FUNCTION__, __LINE__);
	    goto exit;
	}
	
	if(ion_phys(rpc_ion_client, handle, &dat, &len) != 0) {
	    HDMI_ERROR("[%s %d fail]", __FUNCTION__, __LINE__);
	    goto exit;
	}
	
	rpc = ion_map_kernel(rpc_ion_client, handle);
	
   	
    pli_ipcCopyMemory((unsigned char*)rpc,(unsigned char*)arg,sizeof(struct AUDIO_HDMI_SET));   //set frequency

    if (send_rpc_command(RPC_AUDIO, 
        ENUM_KERNEL_RPC_HDMI_SET,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU((dat + sizeof(struct AUDIO_HDMI_SET))),
        &RPC_ret)) 
    {
        HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
        goto exit;
    }

    ret = 0;
exit:
	if(handle != NULL) {
        ion_unmap_kernel(rpc_ion_client, handle);
        ion_free(rpc_ion_client, handle);
    }
    return ret;
#endif
        return 0;	
}


int RPC_TOAGENT_HDMI_Mute(struct AUDIO_HDMI_MUTE_INFO *arg) 
{		
#if 1	
	struct AUDIO_HDMI_MUTE_INFO *rpc = NULL;
    int ret = -1;
    unsigned long RPC_ret;
   	struct ion_handle *handle = NULL;
    ion_phys_addr_t dat;
    size_t len;
    
    handle = ion_alloc(rpc_ion_client, 4096, 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, ION_FLAG_NONCACHED |
																					ION_FLAG_SCPUACC | 
																					ION_FLAG_ACPUACC);
	
	if (IS_ERR(handle)) {
	    HDMI_ERROR("[%s %d ion_alloc fail]", __FUNCTION__, __LINE__);
	    goto exit;
	}
	
	if(ion_phys(rpc_ion_client, handle, &dat, &len) != 0) {
	    HDMI_ERROR("[%s %d fail]", __FUNCTION__, __LINE__);
	    goto exit;
	}
	
	rpc = ion_map_kernel(rpc_ion_client, handle);
	
	rpc-> instanceID = htonl(arg->instanceID);
    rpc-> hdmi_mute = arg-> hdmi_mute;
	   	
    if (send_rpc_command(RPC_AUDIO, 
        ENUM_KERNEL_RPC_HDMI_MUTE,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU((dat + sizeof(struct AUDIO_HDMI_MUTE_INFO))),
        &RPC_ret)) 
    {
        HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
        goto exit;
    }

    ret = 0;
exit:
	if(handle != NULL) {
        ion_unmap_kernel(rpc_ion_client, handle);
        ion_free(rpc_ion_client, handle);
    }
    return ret;	
#endif
        return 0;
	
}

int RPC_TOAGENT_HDMI_OUT_VSDB(struct AUDIO_HDMI_OUT_VSDB_DATA *arg) 
{	
#if 1
	struct AUDIO_HDMI_OUT_VSDB_DATA *rpc = NULL;
    int ret = -1;
    unsigned long RPC_ret;
    struct ion_handle *handle = NULL;
    ion_phys_addr_t dat;
    size_t len;
    
	handle = ion_alloc(rpc_ion_client, 4096, 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, ION_FLAG_NONCACHED |
																					ION_FLAG_SCPUACC | 
																					ION_FLAG_ACPUACC);
	
	if (IS_ERR(handle)) {
	    HDMI_ERROR("[%s %d ion_alloc fail]", __FUNCTION__, __LINE__);
	    goto exit;
	}
	
	if(ion_phys(rpc_ion_client, handle, &dat, &len) != 0) {
	    HDMI_ERROR("[%s %d fail]", __FUNCTION__, __LINE__);
	    goto exit;
	}
	
	rpc = ion_map_kernel(rpc_ion_client, handle);
	
   	pli_ipcCopyMemory((unsigned char*)rpc,(unsigned char*)arg,sizeof(struct AUDIO_HDMI_OUT_VSDB_DATA)); // latency[] - latency[] or fixed value.
	
    if (send_rpc_command(RPC_AUDIO, 
        ENUM_KERNEL_RPC_HDMI_OUT_VSDB,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU((dat + sizeof(struct AUDIO_HDMI_OUT_VSDB_DATA))),
        &RPC_ret)) 
    {
        HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
        goto exit;
    }

    ret = 0;
exit:
	if(handle != NULL) {
        ion_unmap_kernel(rpc_ion_client, handle);
        ion_free(rpc_ion_client, handle);
    }
    return ret;   
#endif
        return 0;

	
}

int RPC_ToAgent_HDMI_OUT_EDID_0(struct AUDIO_HDMI_OUT_EDID_DATA2 *arg) 
{	
#if 1
	struct AUDIO_HDMI_OUT_EDID_DATA2 *rpc = NULL;
    int ret = -1;
    unsigned long RPC_ret;
    struct ion_handle *handle = NULL;
    ion_phys_addr_t dat;
    size_t len;
    
    handle = ion_alloc(rpc_ion_client, 4096, 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, ION_FLAG_NONCACHED |
																					ION_FLAG_SCPUACC | 
																					ION_FLAG_ACPUACC);
	
	if (IS_ERR(handle)) {
	    HDMI_ERROR("[%s %d ion_alloc fail]", __FUNCTION__, __LINE__);
	    goto exit;
	}
	
	if(ion_phys(rpc_ion_client, handle, &dat, &len) != 0) {
	    HDMI_ERROR("[%s %d fail]", __FUNCTION__, __LINE__);
	    goto exit;
	}
	
	rpc = ion_map_kernel(rpc_ion_client, handle);
	
   	pli_ipcCopyMemory((unsigned char*)rpc,(unsigned char*)arg,sizeof(struct AUDIO_HDMI_OUT_EDID_DATA2)); 
	
#ifdef AO_ON_SCPU
    printk(KERN_ALERT "#### hack @ %s %d\n", __FUNCTION__, __LINE__);
    rtk_ao_kernel_rpc(ENUM_KERNEL_RPC_HDMI_OUT_EDID2
        , (unsigned char *)rpc, sizeof(*rpc)
        , (unsigned char *)rpc + sizeof(*rpc), sizeof(long)
        , (unsigned char *)&RPC_ret, sizeof(RPC_ret));
#else
    if (send_rpc_command(RPC_AUDIO, 
        ENUM_KERNEL_RPC_HDMI_OUT_EDID2,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU((dat + sizeof(struct AUDIO_HDMI_OUT_EDID_DATA2))),
        &RPC_ret)) 
    {
        HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
        goto exit;
    }
#endif

    ret = 0;
exit:
	if(handle != NULL) {
        ion_unmap_kernel(rpc_ion_client, handle);
        ion_free(rpc_ion_client, handle);
    }
    return ret;
#endif
        return 0;

	
}

int RPC_ToAgent_QueryDisplayWin_0(struct VIDEO_RPC_VOUT_QUERY_DISP_WIN_OUT *arg) 
{	
	struct VIDEO_RPC_VOUT_QUERY_DISP_WIN_IN *i_rpc= NULL;
	struct VIDEO_RPC_VOUT_QUERY_DISP_WIN_OUT *o_rpc = NULL;
    int ret = -1;
    unsigned long RPC_ret;
    struct ion_handle *handle = NULL;
    ion_phys_addr_t dat;
    size_t len;
    unsigned int offset;
	
    handle = ion_alloc(rpc_ion_client, 4096, 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, ION_FLAG_NONCACHED |
																					ION_FLAG_SCPUACC | 
																					ION_FLAG_ACPUACC);
	
	if (IS_ERR(handle)) {
	    HDMI_ERROR("[%s %d ion_alloc fail]", __FUNCTION__, __LINE__);
	    goto exit;
	}
	
	if(ion_phys(rpc_ion_client, handle, &dat, &len) != 0) {
	    HDMI_ERROR("[%s %d fail]", __FUNCTION__, __LINE__);
	    goto exit;
	}
	
	i_rpc = ion_map_kernel(rpc_ion_client, handle);	
	o_rpc = (struct VIDEO_RPC_VOUT_QUERY_DISP_WIN_OUT *)((unsigned long)(i_rpc + sizeof(i_rpc)+8) & 0xFFFFFFFC);
    offset = (unsigned long)o_rpc - (unsigned long)i_rpc;

	HDMI_DEBUG("i_rpc=%p ,o_rpc=%p\n",i_rpc,o_rpc);
	
    if (send_rpc_command(RPC_AUDIO, 
        ENUM_VIDEO_KERNEL_RPC_QUERY_DISPLAY_WIN,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU(dat + offset),
        &RPC_ret)) 
    {
        HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
        goto exit;
    }
	
	
	 if( RPC_ret != S_OK) {
       HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
       goto exit;
   }
	
	arg->standard= htonl(o_rpc->standard);
	HDMI_DEBUG("%s: standard=%d\n",__FUNCTION__,arg->standard);

    ret = 0;
exit:
	if(handle != NULL) {
        ion_unmap_kernel(rpc_ion_client, handle);
        ion_free(rpc_ion_client, handle);
    }
    return ret;

        return 0;
	
}
#else
//VIDEO_RPC_VOUT_ToAgent_ConfigTVSystem_0
//VIDEO_RPC_VOUT_ToAgent_ConfigHdmiInfoFrame_0
//AUDIO_RPC_ToAgent_AOUT_HDMI_Set_0
//AUDIO_RPC_ToAgent_HDMI_Mute_0
//AUDIO_RPC_ToAgent_HDMI_OUT_VSDB_0
//AUDIO_RPC_ToAgent_HDMI_OUT_EDID_0

int RPC_TOAGENT_HDMI_Config_TV_System(struct VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM *arg) 
{	
#if 1
#if 1
	struct VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM *rpc = NULL;
    int ret = -1;
	unsigned long RPC_ret;
    dma_addr_t dat;
    void *addr = 0;
    
    addr = dma_alloc_coherent(NULL, 4096, &dat, GFP_KERNEL);

    if(!addr)
    {
        HDMI_ERROR("[%s %d fail]",__FUNCTION__, __LINE__);
        goto exit;
    }
    rpc = addr;
	

	memcpy(rpc,arg,sizeof(struct VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM));  
	
	rpc-> interfaceType = htonl(arg-> interfaceType);
	
	rpc-> videoInfo.standard = htonl(arg-> videoInfo.standard);
	rpc-> videoInfo.pedType  = htonl(arg-> videoInfo.pedType);
    rpc-> videoInfo.dataInt0 = htonl(arg-> videoInfo.dataInt0);
	rpc-> videoInfo.dataInt1 = htonl(arg-> videoInfo.dataInt1);
	
	rpc-> hdmiInfo.hdmiMode  = htonl(arg-> hdmiInfo.hdmiMode);
    rpc-> hdmiInfo.audioSampleFreq = htonl(arg-> hdmiInfo.audioSampleFreq);	
	rpc-> hdmiInfo.dataInt0  = htonl(arg-> hdmiInfo.dataInt0);
    rpc-> hdmiInfo.reserved1 = htonl(arg-> hdmiInfo.reserved1);
    rpc-> hdmiInfo.reserved2 = htonl(arg-> hdmiInfo.reserved2);
    rpc-> hdmiInfo.reserved3 = htonl(arg-> hdmiInfo.reserved3);
    rpc-> hdmiInfo.reserved4 = htonl(arg-> hdmiInfo.reserved4);

    if (send_rpc_command(RPC_AUDIO, 
        ENUM_VIDEO_KERNEL_RPC_CONFIG_TV_SYSTEM,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU((dat +sizeof(struct VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM))),
        &RPC_ret)) 
    {
        HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
        goto exit;
    }

    ret = 0;
exit:
    if(addr)
        dma_free_coherent(NULL, 4096, addr, dat);
    return ret;
#endif
        return 0;	

#else
	VO_RESOLUTION_INFO temp_vo_res_info;
	
	vo_res_info= (VO_RESOLUTION_INFO *) ioremap(VO_RESOLUTION,sizeof(VO_RESOLUTION_INFO));
    
	if (arg -> videoInfo.standard ==49){
	
		temp_vo_res_info.width = 3840;
		temp_vo_res_info.height= 2160;
	}
	else if(arg ->videoInfo.standard == VO_STANDARD_HDTV_720P_60 || arg ->videoInfo.standard == VO_STANDARD_HDTV_720P_50 ||
	   arg ->videoInfo.standard == VO_STANDARD_HDTV_720P_30 || arg ->videoInfo.standard == VO_STANDARD_HDTV_720P_25 || 
	   arg ->videoInfo.standard == VO_STANDARD_HDTV_720P_24 || arg ->videoInfo.standard == VO_STANDARD_HDTV_720P_59){
						   
		temp_vo_res_info.width = 1280;
		temp_vo_res_info.height= 720;					   
	}
	else if(arg ->videoInfo.standard == VO_STANDARD_NTSC_M || arg ->videoInfo.standard == VO_STANDARD_NTSC_J ||
			arg ->videoInfo.standard == VO_STANDARD_NTSC_443){
						   
		temp_vo_res_info.width = 720;
		temp_vo_res_info.height= 480;					   
	}
	else if(arg ->videoInfo.standard == VO_STANDARD_PAL_B || arg ->videoInfo.standard == VO_STANDARD_PAL_D ||
			arg ->videoInfo.standard == VO_STANDARD_PAL_G || arg ->videoInfo.standard == VO_STANDARD_PAL_H ||
			arg ->videoInfo.standard == VO_STANDARD_PAL_I || arg ->videoInfo.standard == VO_STANDARD_PAL_N ||
			arg ->videoInfo.standard == VO_STANDARD_PAL_NC|| arg ->videoInfo.standard == VO_STANDARD_PAL_M ||
			arg ->videoInfo.standard == VO_STANDARD_PAL_60){
						   
		temp_vo_res_info.width = 720;
		temp_vo_res_info.height= 576;					   
	}	
	else{
		temp_vo_res_info.width = 1920;
		temp_vo_res_info.height= 1080;		
	}
	HDMI_DEBUG("temp_vo_res_info.width=%d ",temp_vo_res_info.width);		
	HDMI_DEBUG("temp_vo_res_info.height=%d ",temp_vo_res_info.height);	
    temp_vo_res_info.magic = 0x11223344;
    pli_ipcCopyMemory((unsigned char*)vo_res_info, (unsigned char*)&temp_vo_res_info, sizeof(VO_RESOLUTION_INFO));
    
	return 0;    
	
#endif	
}

int RPC_TOAGENT_HDMI_Config_AVI_Info(struct VIDEO_RPC_VOUT_CONFIG_HDMI_INFO_FRAME *arg) 
{	
#if 1	
	struct VIDEO_RPC_VOUT_CONFIG_HDMI_INFO_FRAME *rpc = NULL;
    int ret = -1;
    unsigned long RPC_ret;
    dma_addr_t dat;
    void *addr = 0;
    
    addr = dma_alloc_coherent(NULL, 4096, &dat, GFP_KERNEL);

    if(!addr)
    {
        HDMI_ERROR("[%s %d fail]",__FUNCTION__, __LINE__);
        goto exit;
    }
    rpc = addr;
	
	
	memcpy(rpc,arg,sizeof(struct VIDEO_RPC_VOUT_CONFIG_HDMI_INFO_FRAME)); 

	rpc-> hdmiMode  = htonl(arg-> hdmiMode);
    rpc-> audioSampleFreq = htonl(arg-> audioSampleFreq);
    rpc-> dataInt0 =  htonl(arg-> dataInt0);
    rpc-> reserved1 = htonl(arg-> reserved1);
    rpc-> reserved2 = htonl(arg-> reserved2);
    rpc-> reserved3 = htonl(arg-> reserved3);
    rpc-> reserved4 = htonl(arg-> reserved4);
    
    if (send_rpc_command(RPC_AUDIO, 
        ENUM_VIDEO_KERNEL_RPC_CONFIG_HDMI_INFO_FRAME,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU((dat + sizeof(struct VIDEO_RPC_VOUT_CONFIG_HDMI_INFO_FRAME))),
        &RPC_ret)) 
    {
        HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
        goto exit;
    }

    ret = 0;
exit:
    if(addr)
        dma_free_coherent(NULL, 4096, addr, dat);
    return ret;
#endif
        return 0;	
}

int RPC_TOAGENT_HDMI_Set(struct AUDIO_HDMI_SET *arg) 
{	
#if 1	
	struct AUDIO_HDMI_SET *rpc = NULL;
    int ret = -1;
    unsigned long RPC_ret;
    dma_addr_t dat;
    void *addr = 0;
    
    addr = dma_alloc_coherent(NULL, 4096, &dat, GFP_KERNEL);

    if(!addr)
    {
        HDMI_ERROR("[%s %d fail]",__FUNCTION__, __LINE__);
        goto exit;
    }
    rpc = addr;
	
    pli_ipcCopyMemory((unsigned char*)rpc,(unsigned char*)arg,sizeof(struct AUDIO_HDMI_SET));   //set frequency

    if (send_rpc_command(RPC_AUDIO, 
        ENUM_KERNEL_RPC_HDMI_SET,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU((dat + sizeof(struct AUDIO_HDMI_SET))),
        &RPC_ret)) 
    {
        HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
        goto exit;
    }

    ret = 0;
exit:
    if(addr)
        dma_free_coherent(NULL, 4096, addr, dat);
    return ret;
#endif
        return 0;	
}


int RPC_TOAGENT_HDMI_Mute(struct AUDIO_HDMI_MUTE_INFO *arg) 
{		
#if 1	
	struct AUDIO_HDMI_MUTE_INFO *rpc = NULL;
    int ret = -1;
    unsigned long RPC_ret;
    dma_addr_t dat;
    void *addr = 0;
    
    addr = dma_alloc_coherent(NULL, 4096, &dat, GFP_KERNEL);

    if(!addr)
    {
        HDMI_ERROR("[%s %d fail]",__FUNCTION__, __LINE__);
        goto exit;
    }

    rpc = addr;
	
	rpc-> instanceID = htonl(arg->instanceID);
    rpc-> hdmi_mute = arg-> hdmi_mute;
	   	
    if (send_rpc_command(RPC_AUDIO, 
        ENUM_KERNEL_RPC_HDMI_MUTE,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU((dat + sizeof(struct AUDIO_HDMI_MUTE_INFO))),
        &RPC_ret)) 
    {
        HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
        goto exit;
    }

    ret = 0;
exit:
    if(addr)
        dma_free_coherent(NULL, 4096, addr, dat);
    return ret;
#endif
        return 0;
	
}

int RPC_TOAGENT_HDMI_OUT_VSDB(struct AUDIO_HDMI_OUT_VSDB_DATA *arg) 
{	
#if 1
	struct AUDIO_HDMI_OUT_VSDB_DATA *rpc = NULL;
    int ret = -1;
    unsigned long RPC_ret;
    dma_addr_t dat;
    void *addr = 0;
    
    addr = dma_alloc_coherent(NULL, 4096, &dat, GFP_KERNEL);

    if(!addr)
    {
        HDMI_ERROR("[%s %d fail]",__FUNCTION__, __LINE__);
        goto exit;
    }
    rpc = addr;
   	pli_ipcCopyMemory((unsigned char*)rpc,(unsigned char*)arg,sizeof(struct AUDIO_HDMI_OUT_VSDB_DATA)); // latency[] - latency[] or fixed value.
	
    if (send_rpc_command(RPC_AUDIO, 
        ENUM_KERNEL_RPC_HDMI_OUT_VSDB,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU((dat + sizeof(struct AUDIO_HDMI_OUT_VSDB_DATA))),
        &RPC_ret)) 
    {
        HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
        goto exit;
    }

    ret = 0;
exit:
    if(addr)
        dma_free_coherent(NULL, 4096, addr, dat);
    return ret;
#endif
        return 0;

	
}

int RPC_ToAgent_HDMI_OUT_EDID_0(struct AUDIO_HDMI_OUT_EDID_DATA2 *arg) 
{	
#if 1
	struct AUDIO_HDMI_OUT_EDID_DATA2 *rpc = NULL;
    int ret = -1;
    unsigned long RPC_ret;
    dma_addr_t dat;
    void *addr = 0;
    
    addr = dma_alloc_coherent(NULL, 4096, &dat, GFP_KERNEL);

    if(!addr)
    {
        HDMI_ERROR("[%s %d fail]",__FUNCTION__, __LINE__);
        goto exit;
    }
    rpc = addr;
	
   	pli_ipcCopyMemory((unsigned char*)rpc,(unsigned char*)arg,sizeof(struct AUDIO_HDMI_OUT_EDID_DATA2)); 
	
    if (send_rpc_command(RPC_AUDIO, 
        ENUM_KERNEL_RPC_HDMI_OUT_EDID2,
        CONVERT_FOR_AVCPU(dat),
        CONVERT_FOR_AVCPU((dat + sizeof(struct AUDIO_HDMI_OUT_EDID_DATA2))),
        &RPC_ret)) 
    {
        HDMI_ERROR("[%s %d RPC fail]",__FUNCTION__, __LINE__);
        goto exit;
    }

    ret = 0;
exit:
    if(addr)
        dma_free_coherent(NULL, 4096, addr, dat);
    return ret;
#endif
        return 0;

	
}

#endif //USE_ION_AUDIO_HEAP

int Force_RPC_TOAGENT_HDMI_Config_TV_System(int tv_system, int vo_mode) 
{	
	struct VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM arg;
	
	HDMI_DEBUG("%s",__FUNCTION__);
		
	arg.interfaceType = VO_ANALOG_AND_DIGITAL;	
	arg.videoInfo.standard  = tv_system;
	arg.videoInfo.enProg    =  1;
	arg.videoInfo.enDIF     =  1;
	arg.videoInfo.enCompRGB =  0;
	arg.videoInfo.pedType   = 1;
	arg.videoInfo.dataInt0  = 4;
	arg.videoInfo.dataInt1  = 0;
	
	arg.hdmiInfo.hdmiMode          = vo_mode;
	arg.hdmiInfo.audioSampleFreq   = 3;
	arg.hdmiInfo.audioChannelCount = 1; 
	arg.hdmiInfo.dataByte1         = 64;
	arg.hdmiInfo.dataByte2         = 168;
	arg.hdmiInfo.dataByte3         = 0;
	arg.hdmiInfo.dataByte4         = 0;
	arg.hdmiInfo.dataByte5         = 0;
	arg.hdmiInfo.dataInt0          = 1;
		
	return RPC_TOAGENT_HDMI_Config_TV_System(&arg);
}

int hdmitx_reset_sink_capability(asoc_hdmi_t* p_this)
{	
	p_this-> sink_cap_available = false;
	memset(&p_this->sink_cap,0x0,sizeof(p_this->sink_cap));
	memset(&p_this->sink_cap.cec_phy_addr,0xff,sizeof(p_this->sink_cap.cec_phy_addr));	

    if(p_this->edid_ptr != NULL)
    {
        kfree(p_this->edid_ptr);
        p_this->edid_ptr = NULL;
    }

	return 0;
}

void hdmitx_set_error_code(int error_code)
{	
	hdmi_error = error_code;
	return ;
}

void hdmitx_dump_error_code(void)
{
	switch (hdmi_error) {
	case HDMI_ERROR_NO_MEMORY:
		HDMI_ERROR("get EDID failed, no memory");
		break;
	case HDMI_ERROR_I2C_ERROR:
		HDMI_ERROR("get EDID failed, I2C error");
		break;
	case HDMI_ERROR_HPD:
		HDMI_ERROR("get EDID failed, cable pulled out");
		break;
	case HDMI_ERROR_INVALID_EDID:
		HDMI_ERROR("get EDID failed, invalid EDID");
		break;	
	default:
		break;
	}
}

int hdmitx_get_sink_capability(asoc_hdmi_t *p_this)
{	
	struct edid *edid = NULL;
	
	if(p_this-> sink_cap_available== true)
		return 0;
	
    hdmitx_set_error_code(HDMI_ERROR_RESERVED);

	if((edid = rtk_get_edid(p_this))==NULL)	
		return -1;
			
	p_this-> sink_cap_available= true;
		
	if(rtk_detect_hdmi_monitor(edid))
		p_this-> sink_cap.hdmi_mode=HDMI_MODE_HDMI;		
	else
		p_this-> sink_cap.hdmi_mode=HDMI_MODE_DVI;
	
	rtk_add_edid_modes(edid, &p_this-> sink_cap);	
	rtk_edid_to_eld(edid, &p_this-> sink_cap);

	hdmitx_print_sink_info(p_this);
	
	return 0;
}

void hdmitx_print_sink_capability(asoc_hdmi_t *p_this)
{	
			
	//audio
	//printk("sink_cap->sampling_rate_cap=%d",p_this-> sink_cap.sampling_rate_cap);
	//printk("sink_cap->max_channel_cap=%d",p_this-> sink_cap.max_channel_cap);
	
	//VSBD
	printk("cec addr:0x%x 0x%x\n",p_this-> sink_cap.cec_phy_addr[0],p_this-> sink_cap.cec_phy_addr[1]);	
	printk("HDMI: DVI dual %d, "
		    "max TMDS clock %d, "
		    "latency present %d %d, "
		    "video latency %d %d, "
		    "audio latency %d %d\n",
		    p_this-> sink_cap.dvi_dual,
		    p_this-> sink_cap.max_tmds_clock,
	      (int) p_this-> sink_cap.latency_present[0],
	      (int) p_this-> sink_cap.latency_present[1],
		    p_this-> sink_cap.video_latency[0],
		    p_this-> sink_cap.video_latency[1],
		    p_this-> sink_cap.audio_latency[0],
		    p_this-> sink_cap.audio_latency[1]);
			
	//printk("structure_all 0x%x\n",p_this-> sink_cap.structure_all);
	//printk("_3D_vic 0x%x\n",p_this-> sink_cap._3D_vic);
		
	//video
	printk("sink_cap height_mm=%d \n",p_this-> sink_cap.display_info.height_mm);
	printk("sink_cap min_vfreq=%d \n",p_this-> sink_cap.display_info.min_vfreq);
	printk("sink_cap max_vfreq=%d \n",p_this-> sink_cap.display_info.max_vfreq);
	printk("sink_cap min_hfreq=%d \n",p_this-> sink_cap.display_info. min_hfreq);
	printk("sink_cap max_hfreq=%d \n",p_this-> sink_cap.display_info.max_hfreq);
	printk("sink_cap pixel_clock=%d \n",p_this-> sink_cap.display_info.pixel_clock);
	printk("sink_cap bpc=%d \n",p_this-> sink_cap.display_info.bpc);
	printk("sink_cap color_formats=%d \n",p_this-> sink_cap.display_info.color_formats);
	printk("vic=%llx\n",p_this-> sink_cap.vic);	
	
	return;
}

void hdmitx_print_sink_info(asoc_hdmi_t *p_this)
{	
	printk("[HDMITx]dump EDID \n");
	
	print_color_formats(p_this-> sink_cap.display_info.color_formats);

	if(p_this-> sink_cap.hdmi_mode==HDMI_MODE_HDMI)		
		printk("[HDMITx]DEVICE MODE: HDMI Mode\n");
	else if(p_this-> sink_cap.hdmi_mode==HDMI_MODE_DVI)
		printk("[HDMITx]DEVICE MODE: DVI Mode\n");

	print_cea_modes(p_this-> sink_cap.vic);
	
	printk("[HDMITx]CEC Address: 0x%x%02x\n",p_this-> sink_cap.cec_phy_addr[0],p_this-> sink_cap.cec_phy_addr[1]);

	print_deep_color(p_this-> sink_cap.display_info.bpc);
	
	if(p_this-> sink_cap.DC_Y444)
			printk("[HDMITx]Support Deep Color in YCbCr444\n");
			
	if(p_this-> sink_cap.max_tmds_clock)
		printk("[HDMITx]Max TMDS clock: %d\n",p_this-> sink_cap.max_tmds_clock);

	printk("[HDMITx]Video Latency: %d %d \n", p_this-> sink_cap.video_latency[0],p_this-> sink_cap.video_latency[1]);									
	printk("[HDMITx]Audio Latency: %d %d \n", p_this-> sink_cap.audio_latency[0],p_this-> sink_cap.audio_latency[1]);
	
	print_color_space(p_this-> sink_cap.color_space);	

    hdmi_print_raw_edid(p_this-> edid_ptr);

	
#if 0	
	hdmi_print_edid(&p_this-> edid_basic);
	
	//print_established_modes(p_this-> sink_cap.est_modes);	
	
	print_color_formats(p_this-> sink_cap.display_info.color_formats);
	
	if(p_this-> sink_cap.hdmi_mode!=0)		
		printk("\n%12s : %s","DEVICE MODE","HDMI Mode");
		
	printk("\n");	
					
	//Video Data Block
	print_cea_modes(p_this-> sink_cap.vic);	
	
	//Audio Data Block
	//printk("\n%12s : Sampling Rate=%d","AUDIO DATA",p_this-> sink_cap.sampling_rate_cap);
	//printk("\n%12s   Max Channel=%d"," ",p_this-> sink_cap.max_channel_cap);
	
	printk("\n");	
	//Vendor Specific Data Block
	printk("\n%12s : CEC Address=0x%x%02x","VENDOR SPEC"
								,p_this-> sink_cap.cec_phy_addr[0],p_this-> sink_cap.cec_phy_addr[1]);
	
	print_deep_color(p_this-> sink_cap.display_info.bpc);
	
	if(p_this-> sink_cap.DC_Y444)
			printk("\n%12s   Support Deep Color in YCbCr444"," ");
			
	//if(p_this-> sink_cap.dvi_dual)
		//	printk("Support DVI dual\n ");
		
	if(p_this-> sink_cap.max_tmds_clock)
		printk("\n%12s   Max TMDS clock=%d"," ",p_this-> sink_cap.max_tmds_clock);

//	if(p_this-> sink_cap.latency_present[0]||p_this-> sink_cap.latency_present[1])
	{	printk("\n%12s   Video Latency= %d %d "," ", p_this-> sink_cap.video_latency[0],
										p_this-> sink_cap.video_latency[1]);
										
		printk("\n%12s   Audio Latency= %d %d "," ", p_this-> sink_cap.audio_latency[0],
											p_this-> sink_cap.audio_latency[1]);
	}
	
	//printk("structure_all 0x%x\n",p_this-> sink_cap.structure_all);
	//printk("_3D_vic 0x%x\n",p_this-> sink_cap._3D_vic);
	
	//Extended-Colorimetry Data Block
	printk("\n");	
	print_color_space(p_this-> sink_cap.color_space);
	
	printk("\n");	
	hdmi_print_raw_edid(p_this-> edid_ptr);	

#endif
	return ;
}

int ops_config_tv_system(void __user *arg)
{
	struct VIDEO_RPC_VOUT_CONFIG_TV_SYSTEM tv_system;

	HDMI_DEBUG("%s", __func__);		
	
	if (copy_from_user(&tv_system, arg, sizeof(tv_system))) {
		HDMI_ERROR("%s:failed to copy from user !", __func__);
		return -EFAULT;
	}
	
	return RPC_TOAGENT_HDMI_Config_TV_System(&tv_system);	
}

int ops_config_avi_info(void __user *arg)
{
	struct VIDEO_RPC_VOUT_CONFIG_HDMI_INFO_FRAME info_frame;
	
	HDMI_DEBUG("%s", __func__);
	
	if (copy_from_user(&info_frame , arg, sizeof(info_frame))) {
		HDMI_ERROR("%s:failed to copy from user !", __func__);
		return -EFAULT;
	}

	return RPC_TOAGENT_HDMI_Config_AVI_Info(&info_frame);
}

int ops_set_frequency(void __user *arg)
{
	struct AUDIO_HDMI_SET set;
	
	HDMI_DEBUG("%s", __func__);

	if (copy_from_user(&set, arg, sizeof(set))) {
		HDMI_ERROR("%s:failed to copy from user !", __func__);
		return -EFAULT;
	}	
	
	return RPC_TOAGENT_HDMI_Set(&set);	

}

int ops_set_audio_mute(void __user *arg)
{
	struct AUDIO_HDMI_MUTE_INFO mute_info;

	HDMI_DEBUG("%s", __func__);
	
	if (copy_from_user(&mute_info, arg, sizeof(mute_info))) {
		HDMI_ERROR("%s:failed to copy from user !", __func__);
		return -EFAULT;
	}
	
	return RPC_TOAGENT_HDMI_Mute(&mute_info);
}

int ops_send_audio_vsdb_data(void __user *arg)
{	
	struct AUDIO_HDMI_OUT_VSDB_DATA vsdb_data;
	
	HDMI_DEBUG("%s", __func__);	
	
	if (copy_from_user(&vsdb_data,arg,sizeof(vsdb_data))) {		
		HDMI_ERROR("%s:failed to copy from user ! ", __func__);
		return -EFAULT;
	}
	
	return RPC_TOAGENT_HDMI_OUT_VSDB(&vsdb_data);	
}

int ops_send_audio_edid2(void __user *arg)
{
	struct AUDIO_HDMI_OUT_EDID_DATA2 edid2;
	
	HDMI_DEBUG("%s", __func__);	
	
	if (copy_from_user(&edid2,arg,sizeof(edid2))) {		
		HDMI_ERROR("%s:failed to copy from user !", __func__);
		return -EFAULT;
	}
	
	return RPC_ToAgent_HDMI_OUT_EDID_0(&edid2);	
}

int ops_query_display_standard(void __user *arg)
{
	int ret= 0;	
	int standard =0; 
	struct VIDEO_RPC_VOUT_QUERY_DISP_WIN_OUT display_win;
	
	HDMI_DEBUG("%s", __func__);

	ret = RPC_ToAgent_QueryDisplayWin_0(&display_win);
	standard = (int)display_win.standard;
		
	if (copy_to_user(arg, &standard ,sizeof(standard))) {
		HDMI_ERROR("%s:failed to copy to user !", __func__);
		return -EFAULT;
	}
				
	return ret;	
}

int ops_get_sink_cap(void __user *arg, asoc_hdmi_t *data)
{
	int ret= 0;
	
	HDMI_DEBUG("%s", __func__);
	
	if (!(data-> sink_cap_available)) {
		HDMI_ERROR("[%s]sink cap is not available", __func__);
		return -ENOMSG;
	}

	if (copy_to_user(arg, &data->sink_cap, sizeof(data->sink_cap))) {
		HDMI_ERROR("%s:failed to copy to user !", __func__);
		return -EFAULT;
	}
		
	return ret;	
}

int ops_get_raw_edid(void __user *arg, asoc_hdmi_t *data)
{
	int ret= 0;
	
	HDMI_DEBUG("%s", __func__);
	
	if (!(data->sink_cap_available)) {
		HDMI_ERROR("[%s]sink cap is not available", __func__);
		return -ENOMSG;
	}
					
	if (copy_to_user(arg, data->edid_ptr ,sizeof(struct raw_edid))) {
		HDMI_ERROR("%s:failed to copy to user !", __func__);
		return -EFAULT;
	}	
		
	return ret;	
}
int ops_get_link_status(void __user *arg)
{
	int ret= 0;
	int status =0; 
	
	HDMI_DEBUG("%s", __func__);

	status = show_hpd_status(false);

	if (copy_to_user(arg, &status ,sizeof(status))) {
		HDMI_ERROR("%s:failed to copy to user !", __func__);
		return -EFAULT;
	}
				
	return ret;	
}

int ops_get_video_config(void __user *arg, asoc_hdmi_t *data)
{
	int ret= 0;
	
	HDMI_DEBUG("%s", __func__);	
	
	if (copy_to_user( arg, & data->sink_cap.vic ,sizeof(data->sink_cap.vic))) {
		HDMI_ERROR("%s:failed to copy to user !", __func__);
		return -EFAULT;
	}
				
	return ret;	
}

int ops_send_AVmute(void __user *arg, void __iomem *base)
{
	int flag;
	
	HDMI_DEBUG("%s", __func__);	
	
	if (copy_from_user(&flag, arg, sizeof(flag))) {
		HDMI_ERROR("%s:failed to copy from user !", __func__);
		return -EFAULT;
	}
		
	return hdmitx_send_AVmute(base,flag);
}

int ops_check_tmds_src(void __user *arg, void __iomem *base)
{	
	int tmds_mode = hdmitx_check_tmds_src(base);
	int ret = 0;

	HDMI_DEBUG("%s", __func__);		
	
	if (copy_to_user(arg, &tmds_mode, sizeof(tmds_mode))) {	
		HDMI_ERROR("%s:failed to copy to user !", __func__);
		return -EFAULT;
	}
	
	return ret;	
}

int ops_check_rx_sense(void __user *arg, void __iomem *base)
{	
	int rx_sense = hdmitx_check_rx_sense(base);
	int ret = 0;
	
	HDMI_DEBUG("%s", __func__);	
	
	if (copy_to_user(arg, &rx_sense, sizeof(rx_sense))) {	
		HDMI_ERROR("%s:failed to copy to user !", __func__);
		return -EFAULT;
	}
	
	return ret;	
}

int ops_get_extension_blk_count(void __user *arg, asoc_hdmi_t *data)
{
	int ret= 0;
	int count= 0;
	
	HDMI_DEBUG("%s", __func__);
	
	if (!(data->sink_cap_available)) {
		HDMI_ERROR("[%s]sink cap is not available", __func__);
		return -ENOMSG;
	}
	
	count= (int) data -> edid_ptr[0x7e];	
		
	if (copy_to_user(arg, &count ,sizeof(int))) {
		HDMI_ERROR("%s:failed to copy to user !", __func__);
		return -EFAULT;
	}
			
	return ret;	
}


int ops_get_extended_edid(void __user *arg, asoc_hdmi_t *data)
{
	int ret= 0;
	struct ext_edid ext={0};
	
	HDMI_DEBUG("%s", __func__);
	
	if (!(data->sink_cap_available)) {
		HDMI_ERROR("[%s]sink cap is not available", __func__);
		return -ENOMSG;
	}
		
	if (copy_from_user(&ext,arg,sizeof(struct ext_edid))) {
		HDMI_ERROR("%s:failed to copy from user !", __func__);
		return -EFAULT;
	}
	
	if (ext.current_blk >ext.extension) {
		HDMI_ERROR("[%s] out of range !", __func__);
		return -EFAULT;
	}
	
	HDMI_DEBUG("ext.extension=%d ext.current_blk=%d",ext.extension,ext.current_blk);						

	if(ext.current_blk%2==0)
		memcpy(ext.data,data->edid_ptr+ EDID_LENGTH*(ext.current_blk), sizeof(ext.data));
	else	
		memcpy(ext.data,data->edid_ptr+ EDID_LENGTH*ext.current_blk, EDID_LENGTH*sizeof(unsigned char));	
		
	if (copy_to_user(arg,&ext,sizeof(struct ext_edid))) {
		HDMI_ERROR("%s:failed to copy to user ! ", __func__);
		return -EFAULT;
	}
			
	return ret;	
}
