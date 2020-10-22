#include "sd_test.h"
#include "sd_test_table.h"
#include "sd_test_hdmi_reg.h"
#include "sd_test_vo_reg.h"
#include "phoenix_hdmiInternal.h"
#include "phoenix_mipi_wrapper.h"
#include <linux/printk.h>
#include <asm/atomic.h>
//#include <fcntl.h>
void test_HDMI_480p(void)
{
    //HDMI PHY
      Wreg32(0x18000190, 0x0000001f); 
    Wreg32(0x18000024, 0x00018100); 
    Wreg32(0x18000194, 0x0001c044);
    Wreg32(0x1800d044, 0x5afc9014);
    Wreg32(0x1800d040, 0x0dff5891); //0x0df61891
      //==================FPGA=========================//
       /*
    Wreg32(HDMI_CR , HDMI_CR_write_en3(1)|HDMI_CR_tmds_encen(0) | HDMI_CR_write_en2(0) | HDMI_CR_enablehdcp(0) | HDMI_CR_write_en1(1) | HDMI_CR_enablehdmi(0));
    Wreg32(HDMI_ICR , HDMI_ICR_write_en3(1)|HDMI_ICR_enaudio(0) | HDMI_ICR_write_en2(1) | HDMI_ICR_envitd(0) | HDMI_ICR_write_en1(0) | HDMI_ICR_vitd(0));
    Wreg32(HDMI_H_PARA1 , HDMI_H_PARA1_hblank(137) | HDMI_H_PARA1_hactive(719));
    Wreg32(HDMI_H_PARA2 , HDMI_H_PARA2_hsync(61) | HDMI_H_PARA2_hfront(15));
    Wreg32(HDMI_H_PARA3 , HDMI_H_PARA3_hback(59));
    Wreg32(HDMI_V_PARA1 , HDMI_V_PARA1_Vact_video(479) | HDMI_V_PARA1_vactive(479));
    Wreg32(HDMI_V_PARA4 , HDMI_V_PARA4_vsync((6<<1)) | HDMI_V_PARA4_vblank(44));
    Wreg32(HDMI_V_PARA5 , HDMI_V_PARA5_vback(29) | HDMI_V_PARA5_vfront((9<<1)));
    Wreg32(HDMI_SYNC_DLY , HDMI_SYNC_DLY_write_en4(1) | HDMI_SYNC_DLY_nor_v(0) | HDMI_SYNC_DLY_write_en3(1)|HDMI_SYNC_DLY_nor_h(0)|
           HDMI_SYNC_DLY_write_en2(1) | HDMI_SYNC_DLY_disp_v(0) | HDMI_SYNC_DLY_write_en1(1) | HDMI_SYNC_DLY_disp_h(0));
    Wreg32(HDMI_DPC , HDMI_DPC_write_en9(1) | HDMI_DPC_dp_vfch_num(1) | HDMI_DPC_write_en8(1) | HDMI_DPC_fp_swen(0)|HDMI_DPC_write_en7(1) | HDMI_DPC_fp(0)| HDMI_DPC_write_en4(1) |
           HDMI_DPC_dp_swen(1) | HDMI_DPC_write_en3(1) | HDMI_DPC_default_phase(1)| HDMI_DPC_write_en2(1) | HDMI_DPC_color_depth(0) | HDMI_DPC_write_en1(1) | HDMI_DPC_dpc_enable(0));
    Wreg32(HDMI_CHNL_SEL , HDMI_CHNL_SEL_video_sd(1) | HDMI_CHNL_SEL_Interlaced_vfmt(0) | HDMI_CHNL_SEL_timing_gen_en(1) | HDMI_CHNL_SEL_chl_sel(0));
    Wreg32(HDMI_CS_TRANS0 , HDMI_CS_TRANS0_c1(0x04a80));
    Wreg32(HDMI_CS_TRANS1 , HDMI_CS_TRANS1_c2(0x1e700));
    Wreg32(HDMI_CS_TRANS2 , HDMI_CS_TRANS2_c3(0x1cc40));
    Wreg32(HDMI_CS_TRANS3 , HDMI_CS_TRANS3_c4(0x04a80));
    Wreg32(HDMI_CS_TRANS4 , HDMI_CS_TRANS4_c5(0x08140));
    Wreg32(HDMI_CS_TRANS5 , HDMI_CS_TRANS5_c6(0x00000));
    Wreg32(HDMI_CS_TRANS6 , HDMI_CS_TRANS6_c7(0x04a80));
    Wreg32(HDMI_CS_TRANS7 , HDMI_CS_TRANS7_c8(0x00000));
    Wreg32(HDMI_CS_TRANS8 , HDMI_CS_TRANS8_c9(0x06680));
    Wreg32(HDMI_CS_TRANS9 , HDMI_CS_TRANS9_k1(0x21b0));
    Wreg32(HDMI_CS_TRANS10 , HDMI_CS_TRANS10_k2(0xba70));
    Wreg32(HDMI_CS_TRANS11 , HDMI_CS_TRANS11_k3(0xc840));
    Wreg32(HDMI_SCHCR , HDMI_SCHCR_write_en12(1) | HDMI_SCHCR_color_transform_en(1) | HDMI_SCHCR_write_en7(1) | HDMI_SCHCR_vsyncpolinv(1)|
           HDMI_SCHCR_write_en6(1) | HDMI_SCHCR_hsyncpolinv(1) | HDMI_SCHCR_write_en5(1) | HDMI_SCHCR_pixelencycbcr422(0) | HDMI_SCHCR_write_en4(1)|
           HDMI_SCHCR_hdmi_videoxs(0) | HDMI_SCHCR_write_en3(1) | HDMI_SCHCR_pixelencfmt(0) | HDMI_SCHCR_write_en2(1) | HDMI_SCHCR_pixelrepeat(0) |
           HDMI_SCHCR_write_en1(1) | HDMI_SCHCR_hdmi_modesel(0));
    Wreg32(HDMI_INTEN , HDMI_INTEN_enriupdint(1) | HDMI_INTEN_enpjupdint(1) | HDMI_INTEN_write_data(0));
    Wreg32(HDMI_CR , HDMI_CR_write_en3(1) | HDMI_CR_tmds_encen(1) | HDMI_CR_write_en2(1) | HDMI_CR_enablehdcp(1) | HDMI_CR_write_en1(1) | HDMI_CR_enablehdmi(1));
    */
    //==================ASIC=========================//
   
    Wreg32(HDMI_CR , HDMI_CR_write_en3(1)|HDMI_CR_tmds_encen(0)|HDMI_CR_write_en2(0)|HDMI_CR_enablehdcp(0)|HDMI_CR_write_en1(1)|HDMI_CR_enablehdmi(0)); 
    Wreg32(HDMI_ICR ,HDMI_ICR_write_en3(1)|HDMI_ICR_enaudio(0)|HDMI_ICR_write_en2(1)|HDMI_ICR_envitd(0)|HDMI_ICR_write_en1(0)|HDMI_ICR_vitd(0));
    Wreg32(HDMI_H_PARA1 , HDMI_H_PARA1_hblank(137)|HDMI_H_PARA1_hactive(719));
    Wreg32(HDMI_H_PARA2 , HDMI_H_PARA2_hsync(61)|HDMI_H_PARA2_hfront(15));
    Wreg32(HDMI_H_PARA3 , HDMI_H_PARA3_hback(59));
    Wreg32(HDMI_V_PARA1 , HDMI_V_PARA1_Vact_video(479)|HDMI_V_PARA1_vactive(479));
    Wreg32(HDMI_V_PARA2 , HDMI_V_PARA2_Vact_space1(0)|HDMI_V_PARA2_Vact_space(0));
    Wreg32(HDMI_V_PARA3 , HDMI_V_PARA3_Vblank3(0)|HDMI_V_PARA3_Vact_space2(0));
    Wreg32(HDMI_V_PARA4 , HDMI_V_PARA4_vsync((6<<1))|HDMI_V_PARA4_vblank(44));
    Wreg32(HDMI_V_PARA5 , HDMI_V_PARA5_vback(29)|HDMI_V_PARA5_vfront((9<<1)));
    Wreg32(HDMI_V_PARA6 , HDMI_V_PARA6_Vsync1((0<<1))|HDMI_V_PARA6_Vblank1(0));
    Wreg32(HDMI_V_PARA7 , HDMI_V_PARA7_Vback1(0)|HDMI_V_PARA7_Vfront1((0<<1)));
    Wreg32(HDMI_V_PARA8 , HDMI_V_PARA8_Vsync2((0<<1))|HDMI_V_PARA8_Vblank2(0));
    Wreg32(HDMI_V_PARA9 , HDMI_V_PARA9_Vback2(0)|HDMI_V_PARA9_Vfront2((0<<1)));
    Wreg32(HDMI_V_PARA10 , HDMI_V_PARA10_G(0x00beef));
    Wreg32(HDMI_V_PARA11 , HDMI_V_PARA11_B(0x00beef)|HDMI_V_PARA11_R(0x00beef));
    Wreg32(HDMI_V_PARA12 , HDMI_V_PARA12_vsynci((0<<1))|HDMI_V_PARA12_vblanki(0));
    Wreg32(HDMI_V_PARA13 , HDMI_V_PARA13_vbacki(0)|HDMI_V_PARA13_vfronti((0<<1)));
    Wreg32(HDMI_SYNC_DLY , HDMI_SYNC_DLY_write_en4(1)|HDMI_SYNC_DLY_nor_v(0)|HDMI_SYNC_DLY_write_en3(1)|HDMI_SYNC_DLY_nor_h(0)|
                           HDMI_SYNC_DLY_write_en2(1)|HDMI_SYNC_DLY_disp_v(0)|HDMI_SYNC_DLY_write_en1(1)|HDMI_SYNC_DLY_disp_h(0));
    Wreg32(HDMI_DPC , HDMI_DPC_write_en9(1)|HDMI_DPC_dp_vfch_num(1)|HDMI_DPC_write_en8(1)|HDMI_DPC_fp_swen(0)|HDMI_DPC_write_en7(1)|HDMI_DPC_fp(0)|HDMI_DPC_write_en4(1)|HDMI_DPC_dp_swen(1)|HDMI_DPC_write_en3(1)|
           HDMI_DPC_default_phase(1)|HDMI_DPC_write_en2(1)|HDMI_DPC_color_depth(0)|HDMI_DPC_write_en1(1)|HDMI_DPC_dpc_enable(0));
    //=====================================================================================================
    // HDMI Control
    //=====================================================================================================
    Wreg32(HDMI_CHNL_SEL , HDMI_CHNL_SEL_video_sd(1)|HDMI_CHNL_SEL_Interlaced_vfmt(0)|HDMI_CHNL_SEL_3D_video_format(0xf)|HDMI_CHNL_SEL_En_3D(0)|HDMI_CHNL_SEL_timing_gen_en(1)|HDMI_CHNL_SEL_chl_sel(0)); 

        //-- Color Transform (YCbCr 4:4:4 -> RGB 4:4:4)
        Wreg32(HDMI_CS_TRANS0, HDMI_CS_TRANS0_c1(0x04a80));
        Wreg32(HDMI_CS_TRANS1, HDMI_CS_TRANS1_c2(0x1e700));
        Wreg32(HDMI_CS_TRANS2, HDMI_CS_TRANS2_c3(0x1cc40));
        Wreg32(HDMI_CS_TRANS3, HDMI_CS_TRANS3_c4(0x04a80));
        Wreg32(HDMI_CS_TRANS4, HDMI_CS_TRANS4_c5(0x08140));
        Wreg32(HDMI_CS_TRANS5, HDMI_CS_TRANS5_c6(0x00000));
        Wreg32(HDMI_CS_TRANS6, HDMI_CS_TRANS6_c7(0x04a80));
        Wreg32(HDMI_CS_TRANS7, HDMI_CS_TRANS7_c8(0x00000));
        Wreg32(HDMI_CS_TRANS8, HDMI_CS_TRANS8_c9(0x06680));
        Wreg32(HDMI_CS_TRANS9, HDMI_CS_TRANS9_k1(0x21b0));
        Wreg32(HDMI_CS_TRANS10, HDMI_CS_TRANS10_k2(0xba70));
        Wreg32(HDMI_CS_TRANS11, HDMI_CS_TRANS11_k3(0xc840));
        
        
        Wreg32(HDMI_SCHCR , HDMI_SCHCR_write_en10(1)|HDMI_SCHCR_422_pixel_repeat(0));
        Wreg32(HDMI_GCPCR , HDMI_GCPCR_enablegcp(1)|HDMI_GCPCR_gcp_clearavmute(0)|HDMI_GCPCR_gcp_setavmute(0)|HDMI_GCPCR_write_data(0));
        Wreg32(HDMI_GCPCR , HDMI_GCPCR_enablegcp(1)|HDMI_GCPCR_gcp_clearavmute(0)|HDMI_GCPCR_gcp_setavmute(0)|HDMI_GCPCR_write_data(1));
        Wreg32(HDMI_RPCR , HDMI_RPCR_write_en6(1)|HDMI_RPCR_prp5period(0)|HDMI_RPCR_write_en5(1)|HDMI_RPCR_prp4period(0)|HDMI_RPCR_write_en4(1)|
                           HDMI_RPCR_prp3period(0)|HDMI_RPCR_write_en3(1)|HDMI_RPCR_prp2period(0)|HDMI_RPCR_write_en2(0)|HDMI_RPCR_prp1period(0)|HDMI_RPCR_write_en1(1)|HDMI_RPCR_prp0period(0));
        Wreg32(HDMI_RPEN , HDMI_RPEN_enprpkt5(1)|HDMI_RPEN_enprpkt4(1)|HDMI_RPEN_enprpkt3(1)|HDMI_RPEN_enprpkt2(1)| HDMI_RPEN_enprpkt1(0)| HDMI_RPEN_enprpkt0(1)| HDMI_RPEN_write_data(0));
        Wreg32(HDMI_DIPCCR , HDMI_DIPCCR_write_en2(1)|HDMI_DIPCCR_vbidipcnt(17)|HDMI_DIPCCR_write_en1(1)|HDMI_DIPCCR_hbidipcnt(1));//hor. max 2 packets

    //=====================================================================================================
    // Scheduler
    //=====================================================================================================
        Wreg32(HDMI_SCHCR , HDMI_SCHCR_write_en12(1)|HDMI_SCHCR_color_transform_en(1)|HDMI_SCHCR_write_en11(0)|HDMI_SCHCR_ycbcr422_algo(0)|HDMI_SCHCR_write_en10(0)|HDMI_SCHCR_422_pixel_repeat(0)|
                            HDMI_SCHCR_write_en9(0)|HDMI_SCHCR_vsyncpolin(0)|HDMI_SCHCR_write_en8(0)|HDMI_SCHCR_hsyncpolin(0)|HDMI_SCHCR_write_en7(1)|HDMI_SCHCR_vsyncpolinv(1)|
                            HDMI_SCHCR_write_en6(1)|HDMI_SCHCR_hsyncpolinv(1)|HDMI_SCHCR_write_en5(1)|HDMI_SCHCR_pixelencycbcr422(0)|HDMI_SCHCR_write_en4(1)|HDMI_SCHCR_hdmi_videoxs(0)|
                            HDMI_SCHCR_write_en3(1)|HDMI_SCHCR_pixelencfmt(0)|HDMI_SCHCR_write_en2(1)|HDMI_SCHCR_pixelrepeat(0)|HDMI_SCHCR_write_en1(1)|HDMI_SCHCR_hdmi_modesel(0)); 

        Wreg32(HDMI_HDCP_KOWR , HDMI_HDCP_KOWR_hdcprekeykeepoutwin(0x29));
        Wreg32(HDMI_INTEN , HDMI_INTEN_enriupdint(1)|HDMI_INTEN_enpjupdint(1)|HDMI_INTEN_write_data(0));
        Wreg32(HDMI_CR , HDMI_CR_write_en3(1)|HDMI_CR_tmds_encen(1)|HDMI_CR_write_en2(1)|HDMI_CR_enablehdcp(1)| HDMI_CR_write_en1(1)|HDMI_CR_enablehdmi(1));
         
}

void test_HDMI_1080p60(void)
{
      //HDMI PHY
      Wreg32(0x18000190, 0x0000001b); 
    Wreg32(0x18000024, 0x00018100); 
    Wreg32(0x18000194, 0x00028022); 
    Wreg32(0x1800d044, 0x5afc9012); 
    Wreg32(0x1800d040, 0x0dff5891); 
    
    //0xb800d034
    Wreg32(HDMI_CR , HDMI_CR_write_en3(1)|HDMI_CR_tmds_encen(0)| HDMI_CR_write_en2(0)|HDMI_CR_enablehdcp(0)|HDMI_CR_write_en1(1)|HDMI_CR_enablehdmi(0)); 
    //0xb800d0bc
    Wreg32(HDMI_ICR , HDMI_ICR_write_en3(1)|HDMI_ICR_enaudio(0)| HDMI_ICR_write_en2(1)|HDMI_ICR_envitd(0)|HDMI_ICR_write_en1(0)|HDMI_ICR_vitd(0));
    //0xb800d234
    Wreg32(HDMI_H_PARA1 , HDMI_H_PARA1_hblank(279)|HDMI_H_PARA1_hactive(1919));
    //0xb800d238
    Wreg32(HDMI_H_PARA2 , HDMI_H_PARA2_hsync(43)|HDMI_H_PARA2_hfront(87));
    //0xb800d23c
    Wreg32(HDMI_H_PARA3 , HDMI_H_PARA3_hback(147));
    //0xb800d240
    Wreg32(HDMI_V_PARA1 , HDMI_V_PARA1_Vact_video(1079)|HDMI_V_PARA1_vactive(1079));
    //0xb800d244
    Wreg32(HDMI_V_PARA2 , HDMI_V_PARA2_Vact_space1(0)|HDMI_V_PARA2_Vact_space(0));
    //0xb800d248
    Wreg32(HDMI_V_PARA3 , HDMI_V_PARA3_Vblank3(0)|HDMI_V_PARA3_Vact_space2(0));
    //0xb800d24c
    Wreg32(HDMI_V_PARA4 , HDMI_V_PARA4_vsync((5<<1))|HDMI_V_PARA4_vblank(44));
    //0xb800d250
    Wreg32(HDMI_V_PARA5 , HDMI_V_PARA5_vback(35)|HDMI_V_PARA5_vfront((4<<1)));
    //0xb800d254
    Wreg32(HDMI_V_PARA6 , HDMI_V_PARA6_Vsync1((0<<1))|HDMI_V_PARA6_Vblank1(0));
    //0xb800d258
    Wreg32(HDMI_V_PARA7 , HDMI_V_PARA7_Vback1(0)|HDMI_V_PARA7_Vfront1((0<<1)));
    //0xb800d25c
    Wreg32(HDMI_V_PARA8 , HDMI_V_PARA8_Vsync2((0<<1))|HDMI_V_PARA8_Vblank2(0));
    //0xb800d260
    Wreg32(HDMI_V_PARA9 , HDMI_V_PARA9_Vback2(0)|HDMI_V_PARA9_Vfront2((0<<1)));
    //0xb800d26c
    Wreg32(HDMI_V_PARA10 , HDMI_V_PARA10_G(0x00beef));
    //0xb800d270
    Wreg32(HDMI_V_PARA11 , HDMI_V_PARA11_B(0x00beef)|HDMI_V_PARA11_R(0x00beef));
    //0xb800d264
    Wreg32(HDMI_V_PARA12 , HDMI_V_PARA12_vsynci((0<<1))|HDMI_V_PARA12_vblanki(0));
    //0xb800d268
    Wreg32(HDMI_V_PARA13 , HDMI_V_PARA13_vbacki(0)|HDMI_V_PARA13_vfronti((0<<1)));
    //0xb800d030
    Wreg32(HDMI_SYNC_DLY , HDMI_SYNC_DLY_write_en4(1)|HDMI_SYNC_DLY_nor_v(0)|HDMI_SYNC_DLY_write_en3(1)|HDMI_SYNC_DLY_nor_h(0)|
           HDMI_SYNC_DLY_write_en2(1)|HDMI_SYNC_DLY_disp_v(0)|HDMI_SYNC_DLY_write_en1(1)|HDMI_SYNC_DLY_disp_h(0));
    //0xb800d154
    Wreg32(HDMI_DPC , HDMI_DPC_write_en9(1)|HDMI_DPC_dp_vfch_num(1)|HDMI_DPC_write_en8(1)|HDMI_DPC_fp_swen(0)|HDMI_DPC_write_en7(1)|HDMI_DPC_fp(0)|HDMI_DPC_write_en4(1)|
           HDMI_DPC_dp_swen(1)|HDMI_DPC_write_en3(1)|HDMI_DPC_default_phase(1)|HDMI_DPC_write_en2(1)|HDMI_DPC_color_depth(0)|HDMI_DPC_write_en1(1)|HDMI_DPC_dpc_enable(0));
      
    //=====================================================================================================
    // HDMI Control
    //=====================================================================================================  
    //0xb800d020
    Wreg32(HDMI_CHNL_SEL , HDMI_CHNL_SEL_video_sd(0)|HDMI_CHNL_SEL_Interlaced_vfmt(0)|HDMI_CHNL_SEL_3D_video_format(0xf)|HDMI_CHNL_SEL_En_3D(0)|HDMI_CHNL_SEL_timing_gen_en(1)|HDMI_CHNL_SEL_chl_sel(0)); 
  
    //-- Color Transform (YCbCr 4:4:4 -> RGB 4:4:4)
    //0xb800d024
    Wreg32(HDMI_CS_TRANS0 , HDMI_CS_TRANS0_c1(0x04a80));
    //0xb800d028
    Wreg32(HDMI_CS_TRANS1 , HDMI_CS_TRANS1_c2(0x1e700));
    //0xb800d02c
    Wreg32(HDMI_CS_TRANS2 , HDMI_CS_TRANS2_c3(0x1cc40));
    //0xb800d200
    Wreg32(HDMI_CS_TRANS3 , HDMI_CS_TRANS3_c4(0x04a80));
    //0xb800d204
    Wreg32(HDMI_CS_TRANS4 , HDMI_CS_TRANS4_c5(0x08140));
    //0xb800d208
    Wreg32(HDMI_CS_TRANS5 , HDMI_CS_TRANS5_c6(0x00000));
    //0xb800d20c
    Wreg32(HDMI_CS_TRANS6 , HDMI_CS_TRANS6_c7(0x04a80));
    //0xb800d210
    Wreg32(HDMI_CS_TRANS7 , HDMI_CS_TRANS7_c8(0x00000));
    //0xb800d214
    Wreg32(HDMI_CS_TRANS8 , HDMI_CS_TRANS8_c9(0x06680));
    //0xb800d218
    Wreg32(HDMI_CS_TRANS9 , HDMI_CS_TRANS9_k1(0x21b0));
    //0xb800d21c
    Wreg32(HDMI_CS_TRANS10 , HDMI_CS_TRANS10_k2(0xba70));
    //0xb800d220
    Wreg32(HDMI_CS_TRANS11 , HDMI_CS_TRANS11_k3(0xc840));
   

    //0xb800d0b8
    Wreg32(HDMI_SCHCR , HDMI_SCHCR_write_en10(1)|HDMI_SCHCR_422_pixel_repeat(0));
    //0xb800d078
    Wreg32(HDMI_GCPCR , HDMI_GCPCR_enablegcp(1)|HDMI_GCPCR_gcp_clearavmute(0)|HDMI_GCPCR_gcp_setavmute(0)|HDMI_GCPCR_write_data(0));
    //0xb800d078
    Wreg32(HDMI_GCPCR , HDMI_GCPCR_enablegcp(1)|HDMI_GCPCR_gcp_clearavmute(0)|HDMI_GCPCR_gcp_setavmute(0)|HDMI_GCPCR_write_data(1));
    //0xb800d0a0
    Wreg32(HDMI_RPCR , HDMI_RPCR_write_en6(1)|HDMI_RPCR_prp5period(0)|HDMI_RPCR_write_en5(1)|HDMI_RPCR_prp4period(0)|HDMI_RPCR_write_en4(1)|HDMI_RPCR_prp3period(0)|
           HDMI_RPCR_write_en3(1)|HDMI_RPCR_prp2period(0)|HDMI_RPCR_write_en2(0)|HDMI_RPCR_prp1period(0)|HDMI_RPCR_write_en1(1)|HDMI_RPCR_prp0period(0));
    //0xb800d0a4
    Wreg32(HDMI_RPEN , HDMI_RPEN_enprpkt5(1)|HDMI_RPEN_enprpkt4(1)|HDMI_RPEN_enprpkt3(1)|HDMI_RPEN_enprpkt2(1)| HDMI_RPEN_enprpkt1(0)| HDMI_RPEN_enprpkt0(1)| HDMI_RPEN_write_data(0));
    //0xb800d0b4
    Wreg32(HDMI_DIPCCR , HDMI_DIPCCR_write_en2(1)|HDMI_DIPCCR_vbidipcnt(17)|HDMI_DIPCCR_write_en1(1)|HDMI_DIPCCR_hbidipcnt(5));

    //=====================================================================================================
    // Scheduler
    //=====================================================================================================  
    //0xb800d0b8
    Wreg32(HDMI_SCHCR , HDMI_SCHCR_write_en12(1)|HDMI_SCHCR_color_transform_en(1)|HDMI_SCHCR_write_en11(0)|HDMI_SCHCR_ycbcr422_algo(0)|HDMI_SCHCR_write_en10(0)|HDMI_SCHCR_422_pixel_repeat(0)|
           HDMI_SCHCR_write_en9(0)|HDMI_SCHCR_vsyncpolin(0)|HDMI_SCHCR_write_en8(0)|HDMI_SCHCR_hsyncpolin(0)|HDMI_SCHCR_write_en7(1)|HDMI_SCHCR_vsyncpolinv(0)|HDMI_SCHCR_write_en6(1)|HDMI_SCHCR_hsyncpolinv(0)|
           HDMI_SCHCR_write_en5(1)|HDMI_SCHCR_pixelencycbcr422(0)|HDMI_SCHCR_write_en4(1)|HDMI_SCHCR_hdmi_videoxs(0)|HDMI_SCHCR_write_en3(1)|HDMI_SCHCR_pixelencfmt(0)| HDMI_SCHCR_write_en2(1)|
           HDMI_SCHCR_pixelrepeat(0)|HDMI_SCHCR_write_en1(1)|HDMI_SCHCR_hdmi_modesel(0)); 
 

    //0xb800d0f0
    Wreg32(HDMI_HDCP_KOWR , HDMI_HDCP_KOWR_hdcprekeykeepoutwin(0x2a));

    //0xb800d000
    Wreg32(HDMI_INTEN , HDMI_INTEN_enriupdint(1)|HDMI_INTEN_enpjupdint(1)|HDMI_INTEN_write_data(0));

    //0xb800d034
    Wreg32(HDMI_CR , HDMI_CR_write_en3(1)|HDMI_CR_tmds_encen(1)| HDMI_CR_write_en2(1)|HDMI_CR_enablehdcp(1)| HDMI_CR_write_en1(1)|HDMI_CR_enablehdmi(1)); 
    return;
}

void test_VO_480p(unsigned int p_y, unsigned int p_uv)
{    
    int vo_color_type;
    if(mipi_top.src_sel == 1)
        vo_color_type = hdmi.tx_timing.output_color;
    else
        vo_color_type = mipi_csi.output_color;
    pr_info("[test_VO_480p %x]\n",vo_color_type);
    //Wreg32(0x180041f8 , 0x4);
    Wreg32(VO_MIX1 , VO_MIX1_v1(1) | VO_MIX1_write_data(1));                              //@FPGA
    //Wreg32(VO_MIX1 , VO_MIX1_i_vbi(1)|VO_MIX1_vbi(1)|VO_MIX1_osd1(1)|VO_MIX1_sub1(1)|VO_MIX1_v1(1)|VO_MIX1_bg(1)|VO_MIX1_write_data(0));  //@ASIC
    //Wreg32(VO_MIX1 , VO_MIX1_i_vbi(0)|VO_MIX1_vbi(0)|VO_MIX1_osd1(0)|VO_MIX1_sub1(0)|VO_MIX1_v1(1)|VO_MIX1_bg(0)|VO_MIX1_write_data(1));  //@ASIC
    Wreg32(VO_MODE , VO_MODE_ch1(1)| VO_MODE_write_data(1));                             //@FPGA
    Wreg32(VO_OUT , VO_OUT_write_en3(1)|VO_OUT_h(1));                                    //@FPGA
    //pr_info("[HERE 1]\n");
    //mdelay(1);
    //Wreg32(VO_OUT  , VO_OUT_write_en3(1)|VO_OUT_h(1)|VO_OUT_write_en2(1)|VO_OUT_i(0)|VO_OUT_write_en1(1)|VO_OUT_p(0));                    //@ASIC
    //Wreg32(VO_MODE , VO_MODE_ch2i(0)|VO_MODE_ch2(0)|VO_MODE_ch1i(0)|VO_MODE_ch1(1)|VO_MODE_write_data(1));                                //@ASIC
    //Wreg32(VO_MODE , VO_MODE_ch2i(1)|VO_MODE_ch2(1)|VO_MODE_ch1i(1)|VO_MODE_ch1(0)|VO_MODE_write_data(0));                                //@ASIC
       
    if(vo_color_type == OUT_YUV422)
    {
        Wreg32(VO_V1 , VO_V1_seq(1)| VO_V1_f422(1)| VO_V1_write_data(1)); //YUV422 @FPGA
    }
    else
    {
        Wreg32(VO_V1 , VO_V1_seq(1)| VO_V1_f422(0)| VO_V1_write_data(1)); //YUV420 @FPGA
    }
    //pr_info("[HERE 2]\n");
    //Wreg32(VO_V1 , VO_V1_seq(1)| VO_V1_f422(0)| VO_V1_write_data(1)); //YUV420   
    //Wreg32(VO_V1 ,VO_V1_d3_dbh(0)|VO_V1_d3_dbw(0)|VO_V1_d3_dup(0)|VO_V1_st(0)|VO_V1_seq(1)|VO_V1_di(0)|VO_V1_f422(1)|VO_V1_vs(0)|VO_V1_vs_near(0)|
    //       VO_V1_vs_yodd(0)|VO_V1_vs_codd(0)|VO_V1_hs(0)|VO_V1_hs_yodd(0)|VO_V1_hs_codd(0)|VO_V1_topfield(0)|VO_V1_dmaweave(0)|VO_V1_write_data(1));   //YUV422 @ASIC    
    //Wreg32(VO_V1_SEQ_SA_C_Y , VO_V1_SEQ_SA_C_Y_a(0x3500000));
    //Wreg32(VO_V1_SEQ_SA_C_C , VO_V1_SEQ_SA_C_C_a(0x3700000));
    Wreg32(VO_V1_SEQ_PITCH_C_Y , VO_V1_SEQ_PITCH_C_Y_p(720));
    Wreg32(VO_V1_SEQ_PITCH_C_C , VO_V1_SEQ_PITCH_C_C_p(720));
    Wreg32(VO_V1_SIZE , VO_V1_SIZE_w(720) | VO_V1_SIZE_h(480));
    Wreg32(VO_M1_SIZE , VO_M1_SIZE_w(720) | VO_M1_SIZE_h(480));
    Wreg32(VO_V1_XY , VO_V1_XY_x(0)| VO_V1_XY_y(0));
    Wreg32(VO_CH1 , VO_CH1_itop(1)|VO_CH1_ireint(1)|VO_CH1_top(1)|VO_CH1_reint(1)|VO_CH1_write_data(0));  //@ASIC
//    Wreg32(0x180041f8 , 0x5);
//    while(1)   
    {
//     Wreg32(0x180041f8 ,(Rreg32(0x180041f8)+1));
    // Irene mark MIPI related reg setting
     //Wreg32(mipi_int_st , fm_drop | fm_done1 | fm_done0 | write_data); //clear int
     pr_info("[in vo_480p p_y=%x, p_uv=%x]\n",p_y,p_uv);
     Wreg32(VO_V1_SEQ_SA_C_Y , VO_V1_SEQ_SA_C_Y_a(p_y)); // 0x1500000
     Wreg32(VO_V1_SEQ_SA_C_C , VO_V1_SEQ_SA_C_C_a(p_uv)); // 0x1700000
     //Wreg32(VO_V1_SEQ_SA_C_Y , VO_V1_SEQ_SA_C_Y_a(0x3500000));  //For PIC
     //Wreg32(VO_V1_SEQ_SA_C_C , VO_V1_SEQ_SA_C_C_a(0x3700000));  //For PIC 
     
        if(HDMI_INTSTV_get_vsyncupdated(*(volatile unsigned int *)(HDMI_INTSTV | 0xE6000000)))
        {
            Wreg32(HDMI_INTSTV , HDMI_INTSTV_vendupdated(0) | HDMI_INTSTV_vsyncupdated(0));
            Wreg32(VO_INTST , VO_INTST_m1fin(1) | VO_INTST_write_data(0)); //@FPGA
            Wreg32(VO_FC , VO_FC_m1go(1) | VO_FC_write_data(1)); //@FPGA
            //Wreg32(VO_INTST , VO_INTST_h_under(0)|VO_INTST_i_under(0)|VO_INTST_p_under(0)|VO_INTST_sub1(0)|VO_INTST_wbfin(0)|VO_INTST_m2fin(0)|VO_INTST_m1fin(1)|VO_INTST_write_data(0)); //@ASIC
            //Wreg32(VO_FC , VO_FC_wbgo(0)|VO_FC_m2go(0)|VO_FC_m1go(1)|VO_FC_write_data(1)); //@ASIC
            
            if(VO_INTST_get_h_under(*(volatile unsigned int *)(VO_INTST | 0xE6000000)))
            {
                Wreg32(VO_INTST , VO_INTST_h_under(1) | VO_INTST_write_data(0));
            }
                       
        }
     
    }
    
    return;
}

void sd_test_print(void)
{
    static int test_counter = 0;
#if 0
    pr_info("[0084=%x,5008=%x,5014=%x,5020=%x,5000=%x]\n",Rreg32(0x18000084),Rreg32(VO_MIX1),Rreg32(VO_3D),Rreg32(VO_V1),Rreg32(VO_MODE));
    pr_info("[5004=%x,5048=%x,504c=%x,5068=%x,5604=%x]\n",Rreg32(VO_OUT),Rreg32(VO_V1_SEQ_PITCH_C_Y),Rreg32(VO_V1_SEQ_PITCH_C_C),Rreg32(VO_V1_SIZE),Rreg32(VO_M1_SIZE));
    pr_info("[5610=%x,5644=%x,d278=%x,56d0=%x,5018=%x]\n",Rreg32(VO_V1_XY),Rreg32(VO_CH1),Rreg32(HDMI_INTSTV),Rreg32(VO_INTST),Rreg32(VO_FC));
    pr_info("[5038=%x,503C=%x,5040=%x,5044=%x,563C=%x]\n",Rreg32(0x18005038),Rreg32(0x1800503C),Rreg32(0x18005040),Rreg32(0x18005044),Rreg32(0x1800563C));
    pr_info("\n");
#endif   

    test_counter ++;
    
    if(test_counter < 50)
        return;
    test_counter = 0;
         
    //pr_info("[4100=%x,4108=%x,410C=%x,4104=%x]\n",Rreg32(0x18004100),Rreg32(0x18004108),Rreg32(0x1800410C),Rreg32(0x18004104));
    //pr_info("[4110=%x,4114=%x,4118=%x,411C=%x]\n",Rreg32(0x18004110),Rreg32(0x18004114),Rreg32(0x18004118),Rreg32(0x1800411C));
    //pr_info("[4120=%x,4124=%x,4128=%x,412C=%x]\n",Rreg32(0x18004120),Rreg32(0x18004124),Rreg32(0x18004128),Rreg32(0x1800412C));
    //pr_info("[4130=%x,4134=%x,4138=%x,413C=%x]\n",Rreg32(0x18004130),Rreg32(0x18004134),Rreg32(0x18004138),Rreg32(0x1800413C));
    //pr_info("[4140=%x,4144=%x,4148=%x,414C=%x]\n",Rreg32(0x18004140),Rreg32(0x18004144),Rreg32(0x18004148),Rreg32(0x1800414C));
    //pr_info("[4150=%x,4154=%x,4158=%x,415C=%x]\n",Rreg32(0x18004150),Rreg32(0x18004154),Rreg32(0x18004158),Rreg32(0x1800415C));
    //pr_info("[4160=%x,4164=%x,4168=%x,416C=%x]\n",Rreg32(0x18004160),Rreg32(0x18004164),Rreg32(0x18004168),Rreg32(0x1800416C));
    //pr_info("[4170=%x,4174=%x,4178=%x,417C=%x]\n",Rreg32(0x18004170),Rreg32(0x18004174),Rreg32(0x18004178),Rreg32(0x1800417C));
    //pr_info("[4180=%x,4184=%x,4188=%x,418C=%x]\n",Rreg32(0x18004180),Rreg32(0x18004184),Rreg32(0x18004188),Rreg32(0x1800418C));
    //pr_info("[4190=%x,4194=%x,4198=%x,419C=%x]\n",Rreg32(0x18004190),Rreg32(0x18004194),Rreg32(0x18004198),Rreg32(0x1800419C));
    //pr_info("[41a0=%x,41a4=%x,41a8=%x,41aC=%x]\n",Rreg32(0x180041a0),Rreg32(0x180041a4),Rreg32(0x180041a8),Rreg32(0x180041aC));
    //pr_info("[41b0=%x,41b4=%x,41b8=%x,41bC=%x]\n",Rreg32(0x180041b0),Rreg32(0x180041b4),Rreg32(0x180041b8),Rreg32(0x180041bC));
    //pr_info("[41c0=%x,41c4=%x,41c8=%x,41cC=%x]\n",Rreg32(0x180041c0),Rreg32(0x180041c4),Rreg32(0x180041c8),Rreg32(0x180041cC));
    pr_info("[--4104=%x]\n",Rreg32(0x18004164));
}

void sd_test_vo_detect(void)
{
#if 0 // for 1080p ok
    if(HDMI_INTSTV_get_vsyncupdated(*(volatile unsigned int *)(HDMI_INTSTV | 0xE6000000)))
    {
        unsigned int addry, addruv, offset;
//        unsigned int loop_index[3]={2, 0, 1};
//        unsigned int write_index = loop_index[atomic_read(&hdmi_sw_buf_ctl.write_index)];
                
        if(mipi_top.src_sel == 1)
            offset = (hdmi.tx_timing.pitch) * (hdmi.tx_timing.output_v);
        else
            offset = (mipi_csi.pitch) * (mipi_csi.v_output_len);    
        
        //pr_info("[In vo offset=%d]\n",offset);
//        addry = (unsigned int)((hdmi_ddr_ptr[write_index]));
//        addruv = (unsigned int)((hdmi_ddr_ptr[write_index] + offset));
        Wreg32(0x180041f8 ,(Rreg32(0x180041f8)+1));
        addry = (unsigned int)((hdmi_ddr_ptr[atomic_read(&hdmi_sw_buf_ctl.read_index)]));
        addruv = (unsigned int)((hdmi_ddr_ptr[atomic_read(&hdmi_sw_buf_ctl.read_index)] + offset));
        //addry = (unsigned int)((hdmi_ddr_ptr[0]));
        //addruv = (unsigned int)((hdmi_ddr_ptr[0] + offset));
        
        Wreg32(HDMI_INTSTV , HDMI_INTSTV_vendupdated(0) | HDMI_INTSTV_vsyncupdated(0));
        Wreg32(VO_INTST , VO_INTST_m1fin(1) | VO_INTST_write_data(0)); //@FPGA
        //Wreg32(VO_INTST , VO_INTST_h_under(0)|VO_INTST_i_under(0)|VO_INTST_p_under(0)|VO_INTST_sub1(0)|VO_INTST_wbfin(0)|VO_INTST_m2fin(0)|VO_INTST_m1fin(1)|VO_INTST_write_data(0)); //@ASIC
        //Wreg32(VO_FC , VO_FC_wbgo(0)|VO_FC_m2go(0)|VO_FC_m1go(1)|VO_FC_write_data(1)); //@ASIC
        
        if(VO_INTST_get_h_under(*(volatile unsigned int *)(VO_INTST | 0xE6000000)))
        {
            Wreg32(VO_INTST , VO_INTST_h_under(1) | VO_INTST_write_data(0));
        }
        Wreg32(VO_V1_SEQ_SA_C_Y , VO_V1_SEQ_SA_C_Y_a(addry));
        Wreg32(VO_V1_SEQ_SA_C_C , VO_V1_SEQ_SA_C_C_a(addruv));
        
        //pr_info("VO_INTST=%x\n",Rreg32(VO_INTST));   
        Wreg32(VO_FC , VO_FC_m1go(1) | VO_FC_write_data(1)); //@FPGA
        
        atomic_inc(&hdmi_sw_buf_ctl.read_index);
//        pr_info("1.read_index=%d\n",atomic_read(&hdmi_sw_buf_ctl.read_index));
        atomic_inc(hdmi_hw_buf_avalible_num);
        if(atomic_read(&hdmi_sw_buf_ctl.read_index) == HDMI_SW_BUF_NUM)
            atomic_set(&hdmi_sw_buf_ctl.read_index, 0);
    }
#else
    if(HDMI_INTSTV_get_vsyncupdated(*(volatile unsigned int *)(HDMI_INTSTV | 0xE6000000)))
    {
        unsigned int addry, addruv, offset;
                
        if(mipi_top.src_sel == 1)
            offset = roundup16(hdmi.tx_timing.pitch) * roundup16(hdmi.tx_timing.output_v);
        else
            offset = roundup16(mipi_csi.pitch) * roundup16(mipi_csi.v_output_len);    
            
//        Wreg32(0x180041f8 ,(Rreg32(0x180041f8)+1));
        addry = (unsigned int)((hdmi_ddr_ptr[atomic_read(&hdmi_sw_buf_ctl.read_index)]));
        addruv = (unsigned int)((hdmi_ddr_ptr[atomic_read(&hdmi_sw_buf_ctl.read_index)] + offset));
//        addry = (unsigned int)((hdmi_ddr_ptr[0]));
//        addruv = (unsigned int)((hdmi_ddr_ptr[0] + offset));
        //pr_info("[y=%x,uv=%x]\n",addry,addruv);
        Wreg32(VO_V1_SEQ_SA_C_Y , VO_V1_SEQ_SA_C_Y_a(addry));
        Wreg32(VO_V1_SEQ_SA_C_C , VO_V1_SEQ_SA_C_C_a(addruv));
        
        Wreg32(HDMI_INTSTV , HDMI_INTSTV_vendupdated(0) | HDMI_INTSTV_vsyncupdated(0));
        Wreg32(VO_INTST , VO_INTST_m1fin(1) | VO_INTST_write_data(0)); //@FPGA
        Wreg32(VO_FC , VO_FC_m1go(1) | VO_FC_write_data(1)); //@FPGA
        
        if(VO_INTST_get_h_under(*(volatile unsigned int *)(VO_INTST | 0xE6000000)))
        {
            Wreg32(VO_INTST , VO_INTST_h_under(1) | VO_INTST_write_data(0));
        }
//        Wreg32(VO_FC , VO_FC_m1go(1) | VO_FC_write_data(1)); //@FPGA
        
        atomic_inc(&hdmi_sw_buf_ctl.read_index);
//        pr_info("1.read_index=%d,%d\n",atomic_read(&hdmi_sw_buf_ctl.read_index),atomic_read(&hdmi_hw_buf_avalible_num));
        atomic_inc(&hdmi_hw_buf_avalible_num);
        if(atomic_read(&hdmi_sw_buf_ctl.read_index) == HDMI_SW_BUF_NUM)
            atomic_set(&hdmi_sw_buf_ctl.read_index, 0);
    }
#endif    
}

void sd_test_while_loop(void)
{
    int offset = 720*480;//1920*1080;
    unsigned int addr1y, addr1uv;   
    pr_info("sd_test_while_loop 1, offset=%d\n",offset);
    //test_HDMI_480p();
    addr1y = (unsigned int)((hdmi_ddr_ptr[0]));
    addr1uv = (unsigned int)((hdmi_ddr_ptr[0] + offset));
#if 0
    for(i=0; i<offset; i++)
    {
        //Wreg32(addr1y + i ,tmp_y[i]);
        //pr_info("%x=%x\n",addr1y + i,tmp_y[i]);
        *((unsigned char*)(__va(addr1y + i)))=tmp_y[i];
        //Wreg32(addr1uv + i ,tmp_cbcr[i]);
        *((unsigned char*)(__va(addr1uv + i)))=tmp_cbcr[i];
    }
    
    test_HDMI_1080p60();
  
    //0xb8005000       
    //Wreg32(VO_MODE , VO_MODE_ch2i(0)|VO_MODE_ch2(0)|VO_MODE_ch1i(0)|VO_MODE_ch1(1)|VO_MODE_write_data(1));
    pr_info("In sd_test_while_loop 2 %x,%x,5000=%x\n",addr1y,addr1uv,Rreg32(VO_MODE));
    //test_VO_480p(addr1y, addr1uv);
    test_VO_1080p(addr1y, addr1uv);
    pr_info("In sd_test_while_loop 3\n");
    Wreg32(0x180041f8 , 0x5);
#endif    
    pr_info("In sd_test_while_loop 4,5000=%x\n",Rreg32(VO_MODE));
    while(1)
    {
        if(HDMI_INTSTV_get_vsyncupdated(*(volatile unsigned int *)(HDMI_INTSTV | 0xE6000000)))
        {
          //0xb8005000       
            //Wreg32(VO_MODE , VO_MODE_ch2i(0)|VO_MODE_ch2(0)|VO_MODE_ch1i(0)|VO_MODE_ch1(1)|VO_MODE_write_data(1));
            //0xb800d278
          //sd_test_print();
          Wreg32(0x180041f8 ,(Rreg32(0x180041f8)+1));
          //pr_info("[0x180041f8=%x]\n",Rreg32(0x180041f8));
          Wreg32(HDMI_INTSTV , HDMI_INTSTV_vendupdated(0)|HDMI_INTSTV_vsyncupdated(0));
          //0xb80056d0
          Wreg32(VO_INTST , VO_INTST_h_under(0)|VO_INTST_i_under(0)|VO_INTST_p_under(0)|VO_INTST_sub1(0)|
                 VO_INTST_wbfin(0)|VO_INTST_m2fin(0)|VO_INTST_m1fin(1)|VO_INTST_write_data(0));
              if(VO_INTST_get_h_under(*(volatile unsigned int *)(VO_INTST | 0xE6000000)))
              {
                //0xb80056d0
                Wreg32(VO_INTST , VO_INTST_h_under(1)|VO_INTST_i_under(0)|VO_INTST_p_under(0)|VO_INTST_sub1(0)|
                       VO_INTST_wbfin(0)|VO_INTST_m2fin(0)|VO_INTST_m1fin(0)|VO_INTST_write_data(0));
                 //pr_info("VO_INTST=%x\n",Rreg32(VO_INTST));      
                //mdelay(5);
          }
          //0xb8005018
          Wreg32(VO_FC , VO_FC_wbgo(0)|VO_FC_m2go(0)|VO_FC_m1go(1)|VO_FC_write_data(1));
                         
        }
    }
}

void test_VO_1080p(unsigned int p_y, unsigned int p_uv)
{    
    int output_color;
    
    if(mipi_top.src_sel == 1)
        output_color = hdmi.tx_timing.output_color;
    else
        output_color = mipi_csi.output_color;
    Wreg32(0x18000084, 0x0);

    //0xb8005008
    Wreg32(VO_MIX1 , VO_MIX1_i_vbi(1)|VO_MIX1_vbi(1)|VO_MIX1_osd1(1)|VO_MIX1_sub1(1)|VO_MIX1_v1(1)|VO_MIX1_bg(1)|VO_MIX1_write_data(0));
    //0xb8005008
    Wreg32(VO_MIX1 , VO_MIX1_i_vbi(0)|VO_MIX1_vbi(0)|VO_MIX1_osd1(0)|VO_MIX1_sub1(0)|VO_MIX1_v1(1)|VO_MIX1_bg(0)|VO_MIX1_write_data(1));//VO_MIX1_v1(1)
    //0xb8005014
    Wreg32(VO_3D , VO_3D_top_and_bottom(1)|VO_3D_side_by_side(1)|VO_3D_right(1)|VO_3D_write_data(0));
    //0xb8005020
    Wreg32(VO_V1 , VO_V1_d3_dbh(1)|VO_V1_d3_dbw(1)|VO_V1_d3_dup(1)|VO_V1_st(1)|VO_V1_seq(1)|VO_V1_di(1)|VO_V1_f422(1)|VO_V1_vs(1)|VO_V1_vs_near(1)|
           VO_V1_vs_yodd(1)|VO_V1_vs_codd(1)|VO_V1_hs(1)|VO_V1_hs_yodd(1)|VO_V1_hs_codd(1)|VO_V1_topfield(1)|VO_V1_dmaweave(1)|VO_V1_write_data(0));
      
    Wreg32(0x18000084, 0x0);
    pr_info("[0.5000=%x,1.%x,2.%x]\n",Rreg32(VO_MODE),(VO_MODE_ch2i(0)|VO_MODE_ch2(0)|VO_MODE_ch1i(0)|VO_MODE_ch1(1)|VO_MODE_write_data(1)),(VO_MODE_ch2i(1)|VO_MODE_ch2(1)|VO_MODE_ch1i(1)|VO_MODE_ch1(0)|VO_MODE_write_data(0)));
    
    //0xb8005000       
    Wreg32(VO_MODE , VO_MODE_ch2i(0)|VO_MODE_ch2(0)|VO_MODE_ch1i(0)|VO_MODE_ch1(1)|VO_MODE_write_data(1));
    pr_info("[1.5000=%x]\n",Rreg32(VO_MODE));
    //0xb8005000
    Wreg32(VO_MODE , VO_MODE_ch2i(1)|VO_MODE_ch2(1)|VO_MODE_ch1i(1)|VO_MODE_ch1(0)|VO_MODE_write_data(0));                       
    pr_info("[2.5000=%x]\n",Rreg32(VO_MODE));
    //0xb8005004
    Wreg32(VO_OUT , VO_OUT_write_en3(1)|VO_OUT_h(1)|VO_OUT_write_en2(1)|VO_OUT_i(0)|VO_OUT_write_en1(1)|VO_OUT_p(0));       
    //mdelay(1);
    //0xb8005020
    if(output_color == OUT_YUV422)
    //if(1)//(mipi_csi.output_color == OUT_YUV422)
        Wreg32(VO_V1 , VO_V1_d3_dbh(0)|VO_V1_d3_dbw(0)|VO_V1_d3_dup(0)|VO_V1_st(0)|VO_V1_seq(1)|VO_V1_di(0)|VO_V1_f422(1)|VO_V1_vs(0)|VO_V1_vs_near(0)|  //VO_V1_f422(1) 422:1 ; 420:0
           VO_V1_vs_yodd(0)|VO_V1_vs_codd(0)|VO_V1_hs(0)|VO_V1_hs_yodd(0)|VO_V1_hs_codd(0)|VO_V1_topfield(0)|VO_V1_dmaweave(0)|VO_V1_write_data(1));                       
    else   
        Wreg32(VO_V1 , VO_V1_d3_dbh(0)|VO_V1_d3_dbw(0)|VO_V1_d3_dup(0)|VO_V1_st(0)|VO_V1_seq(1)|VO_V1_di(0)|VO_V1_f422(0)|VO_V1_vs(0)|VO_V1_vs_near(0)|  //VO_V1_f422(1) 422:1 ; 420:0
           VO_V1_vs_yodd(0)|VO_V1_vs_codd(0)|VO_V1_hs(0)|VO_V1_hs_yodd(0)|VO_V1_hs_codd(0)|VO_V1_topfield(0)|VO_V1_dmaweave(0)|VO_V1_write_data(1));                       
    //0xb8005048
    Wreg32(VO_V1_SEQ_PITCH_C_Y , VO_V1_SEQ_PITCH_C_Y_p(1920));
    //0xb800504c
    Wreg32(VO_V1_SEQ_PITCH_C_C , VO_V1_SEQ_PITCH_C_C_p(1920));
    //0xb8005068
    Wreg32(VO_V1_SIZE , VO_V1_SIZE_w(1920)|VO_V1_SIZE_h(1080)); 
    //0xb8005604
    Wreg32(VO_M1_SIZE , VO_M1_SIZE_w(1920)|VO_M1_SIZE_h(1080));
    //0xb8005610
    Wreg32(VO_V1_XY , VO_V1_XY_x(0)|VO_V1_XY_y(0));
    //0xb8005644
    Wreg32(VO_CH1 , VO_CH1_itop(1)|VO_CH1_ireint(1)|VO_CH1_top(1)|VO_CH1_reint(1)|VO_CH1_write_data(0));
    
    //Wreg32(VO_MODE , 0x63);
    Wreg32(0x1800563C , 0xC00);
    
    //while(1)
    {
        
     //Wreg32(mipi_int_st , fm_drop | fm_done1 | fm_done0 | write_data); //clear int
     pr_info("[in vo_1080p p_y=%x, p_uv=%x,5000=%x]\n",p_y,p_uv,Rreg32(VO_MODE));
     Wreg32(VO_V1_SEQ_SA_C_Y , VO_V1_SEQ_SA_C_Y_a(p_y));
     Wreg32(VO_V1_SEQ_SA_C_C , VO_V1_SEQ_SA_C_C_a(p_uv));
     
     //Wreg32(VO_V1_SEQ_SA_C_Y , VO_V1_SEQ_SA_C_Y_a(0x1700000));
     //Wreg32(VO_V1_SEQ_SA_C_C , VO_V1_SEQ_SA_C_C_a(0x1500000));
                
         if(HDMI_INTSTV_get_vsyncupdated(*(volatile unsigned int *)(HDMI_INTSTV | 0xE6000000)))
          {
            //0xb800d278
            Wreg32(HDMI_INTSTV , HDMI_INTSTV_vendupdated(0)|HDMI_INTSTV_vsyncupdated(0));
            //0xb80056d0
            Wreg32(VO_INTST , VO_INTST_h_under(0)|VO_INTST_i_under(0)|VO_INTST_p_under(0)|VO_INTST_sub1(0)|
                   VO_INTST_wbfin(0)|VO_INTST_m2fin(0)|VO_INTST_m1fin(1)|VO_INTST_write_data(0));
            //0xb8005018
            Wreg32(VO_FC , VO_FC_wbgo(0)|VO_FC_m2go(0)|VO_FC_m1go(1)|VO_FC_write_data(1));
                           
                if(VO_INTST_get_h_under(*(volatile unsigned int *)(VO_INTST | 0xE6000000)))
                {
                  //0xb80056d0
                  Wreg32(VO_INTST , VO_INTST_h_under(1)|VO_INTST_i_under(0)|VO_INTST_p_under(0)|VO_INTST_sub1(0)|
                         VO_INTST_wbfin(0)|VO_INTST_m2fin(0)|VO_INTST_m1fin(0)|VO_INTST_write_data(0));
                }
                
                
          }
        
    }
    return;
}

