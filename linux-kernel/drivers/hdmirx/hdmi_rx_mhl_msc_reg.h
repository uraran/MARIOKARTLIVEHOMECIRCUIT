#ifndef _HDMI_RX_MHL_MSC_REG_H
#define _HDMI_RX_MHL_MSC_REG_H

// ===============================================
// HDMI MSC_REG(HDMI_RX_MHL_MSC_REG)
// HDMI Register Address base 0x1800F000

#define MSC_REG_00                                                    0x0000
#define MSC_REG_00_reg_addr                                           "0x0000"
#define MSC_REG_00_reg                                                0x0000
#define MSC_REG_00_inst_addr                                          "0x0000"
#define MSC_REG_00_inst                                               0x0000
#define MSC_REG_00_dev_state_shift                                    (0)
#define MSC_REG_00_dev_state_mask                                     (0x000000FF)
#define MSC_REG_00_dev_state(data)                                    (0x000000FF&(data))
#define MSC_REG_00_get_dev_state(data)                                (0x000000FF&(data))


#define MSC_REG_01                                                    0x0004
#define MSC_REG_01_reg_addr                                           "0x0004"
#define MSC_REG_01_reg                                                0x0004
#define MSC_REG_01_inst_addr                                          "0x0001"
#define MSC_REG_01_inst                                               0x0001
#define MSC_REG_01_mhl_ver_shift                                      (0)
#define MSC_REG_01_mhl_ver_mask                                       (0x000000FF)
#define MSC_REG_01_mhl_ver(data)                                      (0x000000FF&(data))
#define MSC_REG_01_get_mhl_ver(data)                                  (0x000000FF&(data))


#define MSC_REG_02                                                    0x0008
#define MSC_REG_02_reg_addr                                           "0x0008"
#define MSC_REG_02_reg                                                0x0008
#define MSC_REG_02_inst_addr                                          "0x0002"
#define MSC_REG_02_inst                                               0x0002
#define MSC_REG_02_dev_cat_shift                                      (7)
#define MSC_REG_02_plim_shift                                         (5)
#define MSC_REG_02_pow_shift                                          (4)
#define MSC_REG_02_dev_type_shift                                     (0)
#define MSC_REG_02_dev_cat_mask                                       (0x00000080)
#define MSC_REG_02_plim_mask                                          (0x00000060)
#define MSC_REG_02_pow_mask                                           (0x00000010)
#define MSC_REG_02_dev_type_mask                                      (0x0000000F)
#define MSC_REG_02_dev_cat(data)                                      (0x00000080&((data)<<7))
#define MSC_REG_02_plim(data)                                         (0x00000060&((data)<<5))
#define MSC_REG_02_pow(data)                                          (0x00000010&((data)<<4))
#define MSC_REG_02_dev_type(data)                                     (0x0000000F&(data))
#define MSC_REG_02_get_dev_cat(data)                                  ((0x00000080&(data))>>7)
#define MSC_REG_02_get_plim(data)                                     ((0x00000060&(data))>>5)
#define MSC_REG_02_get_pow(data)                                      ((0x00000010&(data))>>4)
#define MSC_REG_02_get_dev_type(data)                                 (0x0000000F&(data))


#define MSC_REG_03                                                    0x000c
#define MSC_REG_03_reg_addr                                           "0x000c"
#define MSC_REG_03_reg                                                0x000c
#define MSC_REG_03_inst_addr                                          "0x0003"
#define MSC_REG_03_inst                                               0x0003
#define MSC_REG_03_adop_id_h_shift                                    (0)
#define MSC_REG_03_adop_id_h_mask                                     (0x000000FF)
#define MSC_REG_03_adop_id_h(data)                                    (0x000000FF&(data))
#define MSC_REG_03_get_adop_id_h(data)                                (0x000000FF&(data))


#define MSC_REG_04                                                    0x0010
#define MSC_REG_04_reg_addr                                           "0x0010"
#define MSC_REG_04_reg                                                0x0010
#define MSC_REG_04_inst_addr                                          "0x0004"
#define MSC_REG_04_inst                                               0x0004
#define MSC_REG_04_adop_id_l_shift                                    (0)
#define MSC_REG_04_adop_id_l_mask                                     (0x000000FF)
#define MSC_REG_04_adop_id_l(data)                                    (0x000000FF&(data))
#define MSC_REG_04_get_adop_id_l(data)                                (0x000000FF&(data))


#define MSC_REG_05                                                    0x0014
#define MSC_REG_05_reg_addr                                           "0x0014"
#define MSC_REG_05_reg                                                0x0014
#define MSC_REG_05_inst_addr                                          "0x0005"
#define MSC_REG_05_inst                                               0x0005
#define MSC_REG_05_vid_link_md_shift                                  (6)
#define MSC_REG_05_supp_vga_shift                                     (5)
#define MSC_REG_05_supp_islands_shift                                 (4)
#define MSC_REG_05_supp_ppixel_shift                                  (3)
#define MSC_REG_05_supp_yuv422_shift                                  (2)
#define MSC_REG_05_supp_yuv444_shift                                  (1)
#define MSC_REG_05_supp_rgb444_shift                                  (0)
#define MSC_REG_05_vid_link_md_mask                                   (0x000000C0)
#define MSC_REG_05_supp_vga_mask                                      (0x00000020)
#define MSC_REG_05_supp_islands_mask                                  (0x00000010)
#define MSC_REG_05_supp_ppixel_mask                                   (0x00000008)
#define MSC_REG_05_supp_yuv422_mask                                   (0x00000004)
#define MSC_REG_05_supp_yuv444_mask                                   (0x00000002)
#define MSC_REG_05_supp_rgb444_mask                                   (0x00000001)
#define MSC_REG_05_vid_link_md(data)                                  (0x000000C0&((data)<<6))
#define MSC_REG_05_supp_vga(data)                                     (0x00000020&((data)<<5))
#define MSC_REG_05_supp_islands(data)                                 (0x00000010&((data)<<4))
#define MSC_REG_05_supp_ppixel(data)                                  (0x00000008&((data)<<3))
#define MSC_REG_05_supp_yuv422(data)                                  (0x00000004&((data)<<2))
#define MSC_REG_05_supp_yuv444(data)                                  (0x00000002&((data)<<1))
#define MSC_REG_05_supp_rgb444(data)                                  (0x00000001&(data))
#define MSC_REG_05_get_vid_link_md(data)                              ((0x000000C0&(data))>>6)
#define MSC_REG_05_get_supp_vga(data)                                 ((0x00000020&(data))>>5)
#define MSC_REG_05_get_supp_islands(data)                             ((0x00000010&(data))>>4)
#define MSC_REG_05_get_supp_ppixel(data)                              ((0x00000008&(data))>>3)
#define MSC_REG_05_get_supp_yuv422(data)                              ((0x00000004&(data))>>2)
#define MSC_REG_05_get_supp_yuv444(data)                              ((0x00000002&(data))>>1)
#define MSC_REG_05_get_supp_rgb444(data)                              (0x00000001&(data))


#define MSC_REG_06                                                    0x0018
#define MSC_REG_06_reg_addr                                           "0x0018"
#define MSC_REG_06_reg                                                0x0018
#define MSC_REG_06_inst_addr                                          "0x0006"
#define MSC_REG_06_inst                                               0x0006
#define MSC_REG_06_aud_link_md_shift                                  (2)
#define MSC_REG_06_aud_8ch_shift                                      (1)
#define MSC_REG_06_aud_2ch_shift                                      (0)
#define MSC_REG_06_aud_link_md_mask                                   (0x000000FC)
#define MSC_REG_06_aud_8ch_mask                                       (0x00000002)
#define MSC_REG_06_aud_2ch_mask                                       (0x00000001)
#define MSC_REG_06_aud_link_md(data)                                  (0x000000FC&((data)<<2))
#define MSC_REG_06_aud_8ch(data)                                      (0x00000002&((data)<<1))
#define MSC_REG_06_aud_2ch(data)                                      (0x00000001&(data))
#define MSC_REG_06_get_aud_link_md(data)                              ((0x000000FC&(data))>>2)
#define MSC_REG_06_get_aud_8ch(data)                                  ((0x00000002&(data))>>1)
#define MSC_REG_06_get_aud_2ch(data)                                  (0x00000001&(data))


#define MSC_REG_07                                                    0x001c
#define MSC_REG_07_reg_addr                                           "0x001c"
#define MSC_REG_07_reg                                                0x001c
#define MSC_REG_07_inst_addr                                          "0x0007"
#define MSC_REG_07_inst                                               0x0007
#define MSC_REG_07_supp_vt_shift                                      (7)
#define MSC_REG_07_video_type_shift                                   (4)
#define MSC_REG_07_vt_game_shift                                      (3)
#define MSC_REG_07_vt_cinema_shift                                    (2)
#define MSC_REG_07_vt_photo_shift                                     (1)
#define MSC_REG_07_vt_graphics_shift                                  (0)
#define MSC_REG_07_supp_vt_mask                                       (0x00000080)
#define MSC_REG_07_video_type_mask                                    (0x00000070)
#define MSC_REG_07_vt_game_mask                                       (0x00000008)
#define MSC_REG_07_vt_cinema_mask                                     (0x00000004)
#define MSC_REG_07_vt_photo_mask                                      (0x00000002)
#define MSC_REG_07_vt_graphics_mask                                   (0x00000001)
#define MSC_REG_07_supp_vt(data)                                      (0x00000080&((data)<<7))
#define MSC_REG_07_video_type(data)                                   (0x00000070&((data)<<4))
#define MSC_REG_07_vt_game(data)                                      (0x00000008&((data)<<3))
#define MSC_REG_07_vt_cinema(data)                                    (0x00000004&((data)<<2))
#define MSC_REG_07_vt_photo(data)                                     (0x00000002&((data)<<1))
#define MSC_REG_07_vt_graphics(data)                                  (0x00000001&(data))
#define MSC_REG_07_get_supp_vt(data)                                  ((0x00000080&(data))>>7)
#define MSC_REG_07_get_video_type(data)                               ((0x00000070&(data))>>4)
#define MSC_REG_07_get_vt_game(data)                                  ((0x00000008&(data))>>3)
#define MSC_REG_07_get_vt_cinema(data)                                ((0x00000004&(data))>>2)
#define MSC_REG_07_get_vt_photo(data)                                 ((0x00000002&(data))>>1)
#define MSC_REG_07_get_vt_graphics(data)                              (0x00000001&(data))


#define MSC_REG_08                                                    0x0020
#define MSC_REG_08_reg_addr                                           "0x0020"
#define MSC_REG_08_reg                                                0x0020
#define MSC_REG_08_inst_addr                                          "0x0008"
#define MSC_REG_08_inst                                               0x0008
#define MSC_REG_08_ld_gui_shift                                       (7)
#define MSC_REG_08_ld_speaker_shift                                   (6)
#define MSC_REG_08_ld_record_shift                                    (5)
#define MSC_REG_08_ld_tuner_shift                                     (4)
#define MSC_REG_08_ld_media_shift                                     (3)
#define MSC_REG_08_ld_audio_shift                                     (2)
#define MSC_REG_08_ld_video_shift                                     (1)
#define MSC_REG_08_ld_display_shift                                   (0)
#define MSC_REG_08_ld_gui_mask                                        (0x00000080)
#define MSC_REG_08_ld_speaker_mask                                    (0x00000040)
#define MSC_REG_08_ld_record_mask                                     (0x00000020)
#define MSC_REG_08_ld_tuner_mask                                      (0x00000010)
#define MSC_REG_08_ld_media_mask                                      (0x00000008)
#define MSC_REG_08_ld_audio_mask                                      (0x00000004)
#define MSC_REG_08_ld_video_mask                                      (0x00000002)
#define MSC_REG_08_ld_display_mask                                    (0x00000001)
#define MSC_REG_08_ld_gui(data)                                       (0x00000080&((data)<<7))
#define MSC_REG_08_ld_speaker(data)                                   (0x00000040&((data)<<6))
#define MSC_REG_08_ld_record(data)                                    (0x00000020&((data)<<5))
#define MSC_REG_08_ld_tuner(data)                                     (0x00000010&((data)<<4))
#define MSC_REG_08_ld_media(data)                                     (0x00000008&((data)<<3))
#define MSC_REG_08_ld_audio(data)                                     (0x00000004&((data)<<2))
#define MSC_REG_08_ld_video(data)                                     (0x00000002&((data)<<1))
#define MSC_REG_08_ld_display(data)                                   (0x00000001&(data))
#define MSC_REG_08_get_ld_gui(data)                                   ((0x00000080&(data))>>7)
#define MSC_REG_08_get_ld_speaker(data)                               ((0x00000040&(data))>>6)
#define MSC_REG_08_get_ld_record(data)                                ((0x00000020&(data))>>5)
#define MSC_REG_08_get_ld_tuner(data)                                 ((0x00000010&(data))>>4)
#define MSC_REG_08_get_ld_media(data)                                 ((0x00000008&(data))>>3)
#define MSC_REG_08_get_ld_audio(data)                                 ((0x00000004&(data))>>2)
#define MSC_REG_08_get_ld_video(data)                                 ((0x00000002&(data))>>1)
#define MSC_REG_08_get_ld_display(data)                               (0x00000001&(data))


#define MSC_REG_09                                                    0x0024
#define MSC_REG_09_reg_addr                                           "0x0024"
#define MSC_REG_09_reg                                                0x0024
#define MSC_REG_09_inst_addr                                          "0x0009"
#define MSC_REG_09_inst                                               0x0009
#define MSC_REG_09_bandwid_shift                                      (0)
#define MSC_REG_09_bandwid_mask                                       (0x000000FF)
#define MSC_REG_09_bandwid(data)                                      (0x000000FF&(data))
#define MSC_REG_09_get_bandwid(data)                                  (0x000000FF&(data))


#define MSC_REG_0A                                                    0x0028
#define MSC_REG_0A_reg_addr                                           "0x0028"
#define MSC_REG_0A_reg                                                0x0028
#define MSC_REG_0A_inst_addr                                          "0x000A"
#define MSC_REG_0A_inst                                               0x000A
#define MSC_REG_0A_feature_flag_shift                                 (5)
#define MSC_REG_0A_ucp_recv_supp_shift                                (4)
#define MSC_REG_0A_ucp_send_supp_shift                                (3)
#define MSC_REG_0A_sp_supp_shift                                      (2)
#define MSC_REG_0A_rap_supp_shift                                     (1)
#define MSC_REG_0A_rcp_supp_shift                                     (0)
#define MSC_REG_0A_feature_flag_mask                                  (0x000000E0)
#define MSC_REG_0A_ucp_recv_supp_mask                                 (0x00000010)
#define MSC_REG_0A_ucp_send_supp_mask                                 (0x00000008)
#define MSC_REG_0A_sp_supp_mask                                       (0x00000004)
#define MSC_REG_0A_rap_supp_mask                                      (0x00000002)
#define MSC_REG_0A_rcp_supp_mask                                      (0x00000001)
#define MSC_REG_0A_feature_flag(data)                                 (0x000000E0&((data)<<5))
#define MSC_REG_0A_ucp_recv_supp(data)                                (0x00000010&((data)<<4))
#define MSC_REG_0A_ucp_send_supp(data)                                (0x00000008&((data)<<3))
#define MSC_REG_0A_sp_supp(data)                                      (0x00000004&((data)<<2))
#define MSC_REG_0A_rap_supp(data)                                     (0x00000002&((data)<<1))
#define MSC_REG_0A_rcp_supp(data)                                     (0x00000001&(data))
#define MSC_REG_0A_get_feature_flag(data)                             ((0x000000E0&(data))>>5)
#define MSC_REG_0A_get_ucp_recv_supp(data)                            ((0x00000010&(data))>>4)
#define MSC_REG_0A_get_ucp_send_supp(data)                            ((0x00000008&(data))>>3)
#define MSC_REG_0A_get_sp_supp(data)                                  ((0x00000004&(data))>>2)
#define MSC_REG_0A_get_rap_supp(data)                                 ((0x00000002&(data))>>1)
#define MSC_REG_0A_get_rcp_supp(data)                                 (0x00000001&(data))


#define MSC_REG_0B                                                    0x002c
#define MSC_REG_0B_reg_addr                                           "0x002c"
#define MSC_REG_0B_reg                                                0x002c
#define MSC_REG_0B_inst_addr                                          "0x000B"
#define MSC_REG_0B_inst                                               0x000B
#define MSC_REG_0B_device_id_h_shift                                  (0)
#define MSC_REG_0B_device_id_h_mask                                   (0x000000FF)
#define MSC_REG_0B_device_id_h(data)                                  (0x000000FF&(data))
#define MSC_REG_0B_get_device_id_h(data)                              (0x000000FF&(data))


#define MSC_REG_0C                                                    0x0030
#define MSC_REG_0C_reg_addr                                           "0x0030"
#define MSC_REG_0C_reg                                                0x0030
#define MSC_REG_0C_inst_addr                                          "0x000C"
#define MSC_REG_0C_inst                                               0x000C
#define MSC_REG_0C_device_id_l_shift                                  (0)
#define MSC_REG_0C_device_id_l_mask                                   (0x000000FF)
#define MSC_REG_0C_device_id_l(data)                                  (0x000000FF&(data))
#define MSC_REG_0C_get_device_id_l(data)                              (0x000000FF&(data))


#define MSC_REG_0D                                                    0x0034
#define MSC_REG_0D_reg_addr                                           "0x0034"
#define MSC_REG_0D_reg                                                0x0034
#define MSC_REG_0D_inst_addr                                          "0x000D"
#define MSC_REG_0D_inst                                               0x000D
#define MSC_REG_0D_scr_size_shift                                     (0)
#define MSC_REG_0D_scr_size_mask                                      (0x000000FF)
#define MSC_REG_0D_scr_size(data)                                     (0x000000FF&(data))
#define MSC_REG_0D_get_scr_size(data)                                 (0x000000FF&(data))


#define MSC_REG_0E                                                    0x0038
#define MSC_REG_0E_reg_addr                                           "0x0038"
#define MSC_REG_0E_reg                                                0x0038
#define MSC_REG_0E_inst_addr                                          "0x000E"
#define MSC_REG_0E_inst                                               0x000E
#define MSC_REG_0E_stat_size_shift                                    (4)
#define MSC_REG_0E_int_size_shift                                     (0)
#define MSC_REG_0E_stat_size_mask                                     (0x000000F0)
#define MSC_REG_0E_int_size_mask                                      (0x0000000F)
#define MSC_REG_0E_stat_size(data)                                    (0x000000F0&((data)<<4))
#define MSC_REG_0E_int_size(data)                                     (0x0000000F&(data))
#define MSC_REG_0E_get_stat_size(data)                                ((0x000000F0&(data))>>4)
#define MSC_REG_0E_get_int_size(data)                                 (0x0000000F&(data))


#define MSC_REG_0F                                                    0x003c
#define MSC_REG_0F_reg_addr                                           "0x003c"
#define MSC_REG_0F_reg                                                0x003c
#define MSC_REG_0F_inst_addr                                          "0x000F"
#define MSC_REG_0F_inst                                               0x000F
#define MSC_REG_0F_cap_0f_shift                                       (0)
#define MSC_REG_0F_cap_0f_mask                                        (0x000000FF)
#define MSC_REG_0F_cap_0f(data)                                       (0x000000FF&(data))
#define MSC_REG_0F_get_cap_0f(data)                                   (0x000000FF&(data))


#define MSC_REG_20                                                    0x0080
#define MSC_REG_20_reg_addr                                           "0x0080"
#define MSC_REG_20_reg                                                0x0080
#define MSC_REG_20_inst_addr                                          "0x0020"
#define MSC_REG_20_inst                                               0x0020
#define MSC_REG_20_rchg_int_7_shift                                   (7)
#define MSC_REG_20_rchg_int_6_shift                                   (6)
#define MSC_REG_20_rchg_int_5_shift                                   (5)
#define MSC_REG_20_cbus_3d_req_shift                                  (4)
#define MSC_REG_20_grt_wrt_shift                                      (3)
#define MSC_REG_20_req_wrt_shift                                      (2)
#define MSC_REG_20_dscr_chg_shift                                     (1)
#define MSC_REG_20_dcap_chg_shift                                     (0)
#define MSC_REG_20_rchg_int_7_mask                                    (0x00000080)
#define MSC_REG_20_rchg_int_6_mask                                    (0x00000040)
#define MSC_REG_20_rchg_int_5_mask                                    (0x00000020)
#define MSC_REG_20_cbus_3d_req_mask                                   (0x00000010)
#define MSC_REG_20_grt_wrt_mask                                       (0x00000008)
#define MSC_REG_20_req_wrt_mask                                       (0x00000004)
#define MSC_REG_20_dscr_chg_mask                                      (0x00000002)
#define MSC_REG_20_dcap_chg_mask                                      (0x00000001)
#define MSC_REG_20_rchg_int_7(data)                                   (0x00000080&((data)<<7))
#define MSC_REG_20_rchg_int_6(data)                                   (0x00000040&((data)<<6))
#define MSC_REG_20_rchg_int_5(data)                                   (0x00000020&((data)<<5))
#define MSC_REG_20_cbus_3d_req(data)                                  (0x00000010&((data)<<4))
#define MSC_REG_20_grt_wrt(data)                                      (0x00000008&((data)<<3))
#define MSC_REG_20_req_wrt(data)                                      (0x00000004&((data)<<2))
#define MSC_REG_20_dscr_chg(data)                                     (0x00000002&((data)<<1))
#define MSC_REG_20_dcap_chg(data)                                     (0x00000001&(data))
#define MSC_REG_20_get_rchg_int_7(data)                               ((0x00000080&(data))>>7)
#define MSC_REG_20_get_rchg_int_6(data)                               ((0x00000040&(data))>>6)
#define MSC_REG_20_get_rchg_int_5(data)                               ((0x00000020&(data))>>5)
#define MSC_REG_20_get_cbus_3d_req(data)                              ((0x00000010&(data))>>4)
#define MSC_REG_20_get_grt_wrt(data)                                  ((0x00000008&(data))>>3)
#define MSC_REG_20_get_req_wrt(data)                                  ((0x00000004&(data))>>2)
#define MSC_REG_20_get_dscr_chg(data)                                 ((0x00000002&(data))>>1)
#define MSC_REG_20_get_dcap_chg(data)                                 (0x00000001&(data))


#define MSC_REG_21                                                    0x0084
#define MSC_REG_21_reg_addr                                           "0x0084"
#define MSC_REG_21_reg                                                0x0084
#define MSC_REG_21_inst_addr                                          "0x0021"
#define MSC_REG_21_inst                                               0x0021
#define MSC_REG_21_dchg_int_7_shift                                   (7)
#define MSC_REG_21_dchg_int_6_shift                                   (6)
#define MSC_REG_21_dchg_int_5_shift                                   (5)
#define MSC_REG_21_dchg_int_4_shift                                   (4)
#define MSC_REG_21_dchg_int_3_shift                                   (3)
#define MSC_REG_21_dchg_int_2_shift                                   (2)
#define MSC_REG_21_edid_chg_shift                                     (1)
#define MSC_REG_21_dchg_int_0_shift                                   (0)
#define MSC_REG_21_dchg_int_7_mask                                    (0x00000080)
#define MSC_REG_21_dchg_int_6_mask                                    (0x00000040)
#define MSC_REG_21_dchg_int_5_mask                                    (0x00000020)
#define MSC_REG_21_dchg_int_4_mask                                    (0x00000010)
#define MSC_REG_21_dchg_int_3_mask                                    (0x00000008)
#define MSC_REG_21_dchg_int_2_mask                                    (0x00000004)
#define MSC_REG_21_edid_chg_mask                                      (0x00000002)
#define MSC_REG_21_dchg_int_0_mask                                    (0x00000001)
#define MSC_REG_21_dchg_int_7(data)                                   (0x00000080&((data)<<7))
#define MSC_REG_21_dchg_int_6(data)                                   (0x00000040&((data)<<6))
#define MSC_REG_21_dchg_int_5(data)                                   (0x00000020&((data)<<5))
#define MSC_REG_21_dchg_int_4(data)                                   (0x00000010&((data)<<4))
#define MSC_REG_21_dchg_int_3(data)                                   (0x00000008&((data)<<3))
#define MSC_REG_21_dchg_int_2(data)                                   (0x00000004&((data)<<2))
#define MSC_REG_21_edid_chg(data)                                     (0x00000002&((data)<<1))
#define MSC_REG_21_dchg_int_0(data)                                   (0x00000001&(data))
#define MSC_REG_21_get_dchg_int_7(data)                               ((0x00000080&(data))>>7)
#define MSC_REG_21_get_dchg_int_6(data)                               ((0x00000040&(data))>>6)
#define MSC_REG_21_get_dchg_int_5(data)                               ((0x00000020&(data))>>5)
#define MSC_REG_21_get_dchg_int_4(data)                               ((0x00000010&(data))>>4)
#define MSC_REG_21_get_dchg_int_3(data)                               ((0x00000008&(data))>>3)
#define MSC_REG_21_get_dchg_int_2(data)                               ((0x00000004&(data))>>2)
#define MSC_REG_21_get_edid_chg(data)                                 ((0x00000002&(data))>>1)
#define MSC_REG_21_get_dchg_int_0(data)                               (0x00000001&(data))


#define MSC_REG_22                                                    0x0088
#define MSC_REG_22_reg_addr                                           "0x0088"
#define MSC_REG_22_reg                                                0x0088
#define MSC_REG_22_inst_addr                                          "0x0022"
#define MSC_REG_22_inst                                               0x0022
#define MSC_REG_22_chg22_7_shift                                      (7)
#define MSC_REG_22_chg22_6_shift                                      (6)
#define MSC_REG_22_chg22_5_shift                                      (5)
#define MSC_REG_22_chg22_4_shift                                      (4)
#define MSC_REG_22_chg22_3_shift                                      (3)
#define MSC_REG_22_chg22_2_shift                                      (2)
#define MSC_REG_22_chg22_1_shift                                      (1)
#define MSC_REG_22_chg22_0_shift                                      (0)
#define MSC_REG_22_chg22_7_mask                                       (0x00000080)
#define MSC_REG_22_chg22_6_mask                                       (0x00000040)
#define MSC_REG_22_chg22_5_mask                                       (0x00000020)
#define MSC_REG_22_chg22_4_mask                                       (0x00000010)
#define MSC_REG_22_chg22_3_mask                                       (0x00000008)
#define MSC_REG_22_chg22_2_mask                                       (0x00000004)
#define MSC_REG_22_chg22_1_mask                                       (0x00000002)
#define MSC_REG_22_chg22_0_mask                                       (0x00000001)
#define MSC_REG_22_chg22_7(data)                                      (0x00000080&((data)<<7))
#define MSC_REG_22_chg22_6(data)                                      (0x00000040&((data)<<6))
#define MSC_REG_22_chg22_5(data)                                      (0x00000020&((data)<<5))
#define MSC_REG_22_chg22_4(data)                                      (0x00000010&((data)<<4))
#define MSC_REG_22_chg22_3(data)                                      (0x00000008&((data)<<3))
#define MSC_REG_22_chg22_2(data)                                      (0x00000004&((data)<<2))
#define MSC_REG_22_chg22_1(data)                                      (0x00000002&((data)<<1))
#define MSC_REG_22_chg22_0(data)                                      (0x00000001&(data))
#define MSC_REG_22_get_chg22_7(data)                                  ((0x00000080&(data))>>7)
#define MSC_REG_22_get_chg22_6(data)                                  ((0x00000040&(data))>>6)
#define MSC_REG_22_get_chg22_5(data)                                  ((0x00000020&(data))>>5)
#define MSC_REG_22_get_chg22_4(data)                                  ((0x00000010&(data))>>4)
#define MSC_REG_22_get_chg22_3(data)                                  ((0x00000008&(data))>>3)
#define MSC_REG_22_get_chg22_2(data)                                  ((0x00000004&(data))>>2)
#define MSC_REG_22_get_chg22_1(data)                                  ((0x00000002&(data))>>1)
#define MSC_REG_22_get_chg22_0(data)                                  (0x00000001&(data))


#define MSC_REG_23                                                    0x008c
#define MSC_REG_23_reg_addr                                           "0x008c"
#define MSC_REG_23_reg                                                0x008c
#define MSC_REG_23_inst_addr                                          "0x0023"
#define MSC_REG_23_inst                                               0x0023
#define MSC_REG_23_chg23_7_shift                                      (7)
#define MSC_REG_23_chg23_6_shift                                      (6)
#define MSC_REG_23_chg23_5_shift                                      (5)
#define MSC_REG_23_chg23_4_shift                                      (4)
#define MSC_REG_23_chg23_3_shift                                      (3)
#define MSC_REG_23_chg23_2_shift                                      (2)
#define MSC_REG_23_chg23_1_shift                                      (1)
#define MSC_REG_23_chg23_0_shift                                      (0)
#define MSC_REG_23_chg23_7_mask                                       (0x00000080)
#define MSC_REG_23_chg23_6_mask                                       (0x00000040)
#define MSC_REG_23_chg23_5_mask                                       (0x00000020)
#define MSC_REG_23_chg23_4_mask                                       (0x00000010)
#define MSC_REG_23_chg23_3_mask                                       (0x00000008)
#define MSC_REG_23_chg23_2_mask                                       (0x00000004)
#define MSC_REG_23_chg23_1_mask                                       (0x00000002)
#define MSC_REG_23_chg23_0_mask                                       (0x00000001)
#define MSC_REG_23_chg23_7(data)                                      (0x00000080&((data)<<7))
#define MSC_REG_23_chg23_6(data)                                      (0x00000040&((data)<<6))
#define MSC_REG_23_chg23_5(data)                                      (0x00000020&((data)<<5))
#define MSC_REG_23_chg23_4(data)                                      (0x00000010&((data)<<4))
#define MSC_REG_23_chg23_3(data)                                      (0x00000008&((data)<<3))
#define MSC_REG_23_chg23_2(data)                                      (0x00000004&((data)<<2))
#define MSC_REG_23_chg23_1(data)                                      (0x00000002&((data)<<1))
#define MSC_REG_23_chg23_0(data)                                      (0x00000001&(data))
#define MSC_REG_23_get_chg23_7(data)                                  ((0x00000080&(data))>>7)
#define MSC_REG_23_get_chg23_6(data)                                  ((0x00000040&(data))>>6)
#define MSC_REG_23_get_chg23_5(data)                                  ((0x00000020&(data))>>5)
#define MSC_REG_23_get_chg23_4(data)                                  ((0x00000010&(data))>>4)
#define MSC_REG_23_get_chg23_3(data)                                  ((0x00000008&(data))>>3)
#define MSC_REG_23_get_chg23_2(data)                                  ((0x00000004&(data))>>2)
#define MSC_REG_23_get_chg23_1(data)                                  ((0x00000002&(data))>>1)
#define MSC_REG_23_get_chg23_0(data)                                  (0x00000001&(data))


#define MSC_REG_24                                                    0x0090
#define MSC_REG_24_reg_addr                                           "0x0090"
#define MSC_REG_24_reg                                                0x0090
#define MSC_REG_24_inst_addr                                          "0x0024"
#define MSC_REG_24_inst                                               0x0024
#define MSC_REG_24_chg24_7_shift                                      (7)
#define MSC_REG_24_chg24_6_shift                                      (6)
#define MSC_REG_24_chg24_5_shift                                      (5)
#define MSC_REG_24_chg24_4_shift                                      (4)
#define MSC_REG_24_chg24_3_shift                                      (3)
#define MSC_REG_24_chg24_2_shift                                      (2)
#define MSC_REG_24_chg24_1_shift                                      (1)
#define MSC_REG_24_chg24_0_shift                                      (0)
#define MSC_REG_24_chg24_7_mask                                       (0x00000080)
#define MSC_REG_24_chg24_6_mask                                       (0x00000040)
#define MSC_REG_24_chg24_5_mask                                       (0x00000020)
#define MSC_REG_24_chg24_4_mask                                       (0x00000010)
#define MSC_REG_24_chg24_3_mask                                       (0x00000008)
#define MSC_REG_24_chg24_2_mask                                       (0x00000004)
#define MSC_REG_24_chg24_1_mask                                       (0x00000002)
#define MSC_REG_24_chg24_0_mask                                       (0x00000001)
#define MSC_REG_24_chg24_7(data)                                      (0x00000080&((data)<<7))
#define MSC_REG_24_chg24_6(data)                                      (0x00000040&((data)<<6))
#define MSC_REG_24_chg24_5(data)                                      (0x00000020&((data)<<5))
#define MSC_REG_24_chg24_4(data)                                      (0x00000010&((data)<<4))
#define MSC_REG_24_chg24_3(data)                                      (0x00000008&((data)<<3))
#define MSC_REG_24_chg24_2(data)                                      (0x00000004&((data)<<2))
#define MSC_REG_24_chg24_1(data)                                      (0x00000002&((data)<<1))
#define MSC_REG_24_chg24_0(data)                                      (0x00000001&(data))
#define MSC_REG_24_get_chg24_7(data)                                  ((0x00000080&(data))>>7)
#define MSC_REG_24_get_chg24_6(data)                                  ((0x00000040&(data))>>6)
#define MSC_REG_24_get_chg24_5(data)                                  ((0x00000020&(data))>>5)
#define MSC_REG_24_get_chg24_4(data)                                  ((0x00000010&(data))>>4)
#define MSC_REG_24_get_chg24_3(data)                                  ((0x00000008&(data))>>3)
#define MSC_REG_24_get_chg24_2(data)                                  ((0x00000004&(data))>>2)
#define MSC_REG_24_get_chg24_1(data)                                  ((0x00000002&(data))>>1)
#define MSC_REG_24_get_chg24_0(data)                                  (0x00000001&(data))


#define MSC_REG_25                                                    0x0094
#define MSC_REG_25_reg_addr                                           "0x0094"
#define MSC_REG_25_reg                                                0x0094
#define MSC_REG_25_inst_addr                                          "0x0025"
#define MSC_REG_25_inst                                               0x0025
#define MSC_REG_25_chg25_7_shift                                      (7)
#define MSC_REG_25_chg25_6_shift                                      (6)
#define MSC_REG_25_chg25_5_shift                                      (5)
#define MSC_REG_25_chg25_4_shift                                      (4)
#define MSC_REG_25_chg25_3_shift                                      (3)
#define MSC_REG_25_chg25_2_shift                                      (2)
#define MSC_REG_25_chg25_1_shift                                      (1)
#define MSC_REG_25_chg25_0_shift                                      (0)
#define MSC_REG_25_chg25_7_mask                                       (0x00000080)
#define MSC_REG_25_chg25_6_mask                                       (0x00000040)
#define MSC_REG_25_chg25_5_mask                                       (0x00000020)
#define MSC_REG_25_chg25_4_mask                                       (0x00000010)
#define MSC_REG_25_chg25_3_mask                                       (0x00000008)
#define MSC_REG_25_chg25_2_mask                                       (0x00000004)
#define MSC_REG_25_chg25_1_mask                                       (0x00000002)
#define MSC_REG_25_chg25_0_mask                                       (0x00000001)
#define MSC_REG_25_chg25_7(data)                                      (0x00000080&((data)<<7))
#define MSC_REG_25_chg25_6(data)                                      (0x00000040&((data)<<6))
#define MSC_REG_25_chg25_5(data)                                      (0x00000020&((data)<<5))
#define MSC_REG_25_chg25_4(data)                                      (0x00000010&((data)<<4))
#define MSC_REG_25_chg25_3(data)                                      (0x00000008&((data)<<3))
#define MSC_REG_25_chg25_2(data)                                      (0x00000004&((data)<<2))
#define MSC_REG_25_chg25_1(data)                                      (0x00000002&((data)<<1))
#define MSC_REG_25_chg25_0(data)                                      (0x00000001&(data))
#define MSC_REG_25_get_chg25_7(data)                                  ((0x00000080&(data))>>7)
#define MSC_REG_25_get_chg25_6(data)                                  ((0x00000040&(data))>>6)
#define MSC_REG_25_get_chg25_5(data)                                  ((0x00000020&(data))>>5)
#define MSC_REG_25_get_chg25_4(data)                                  ((0x00000010&(data))>>4)
#define MSC_REG_25_get_chg25_3(data)                                  ((0x00000008&(data))>>3)
#define MSC_REG_25_get_chg25_2(data)                                  ((0x00000004&(data))>>2)
#define MSC_REG_25_get_chg25_1(data)                                  ((0x00000002&(data))>>1)
#define MSC_REG_25_get_chg25_0(data)                                  (0x00000001&(data))


#define MSC_REG_26                                                    0x0098
#define MSC_REG_26_reg_addr                                           "0x0098"
#define MSC_REG_26_reg                                                0x0098
#define MSC_REG_26_inst_addr                                          "0x0026"
#define MSC_REG_26_inst                                               0x0026
#define MSC_REG_26_chg26_7_shift                                      (7)
#define MSC_REG_26_chg26_6_shift                                      (6)
#define MSC_REG_26_chg26_5_shift                                      (5)
#define MSC_REG_26_chg26_4_shift                                      (4)
#define MSC_REG_26_chg26_3_shift                                      (3)
#define MSC_REG_26_chg26_2_shift                                      (2)
#define MSC_REG_26_chg26_1_shift                                      (1)
#define MSC_REG_26_chg26_0_shift                                      (0)
#define MSC_REG_26_chg26_7_mask                                       (0x00000080)
#define MSC_REG_26_chg26_6_mask                                       (0x00000040)
#define MSC_REG_26_chg26_5_mask                                       (0x00000020)
#define MSC_REG_26_chg26_4_mask                                       (0x00000010)
#define MSC_REG_26_chg26_3_mask                                       (0x00000008)
#define MSC_REG_26_chg26_2_mask                                       (0x00000004)
#define MSC_REG_26_chg26_1_mask                                       (0x00000002)
#define MSC_REG_26_chg26_0_mask                                       (0x00000001)
#define MSC_REG_26_chg26_7(data)                                      (0x00000080&((data)<<7))
#define MSC_REG_26_chg26_6(data)                                      (0x00000040&((data)<<6))
#define MSC_REG_26_chg26_5(data)                                      (0x00000020&((data)<<5))
#define MSC_REG_26_chg26_4(data)                                      (0x00000010&((data)<<4))
#define MSC_REG_26_chg26_3(data)                                      (0x00000008&((data)<<3))
#define MSC_REG_26_chg26_2(data)                                      (0x00000004&((data)<<2))
#define MSC_REG_26_chg26_1(data)                                      (0x00000002&((data)<<1))
#define MSC_REG_26_chg26_0(data)                                      (0x00000001&(data))
#define MSC_REG_26_get_chg26_7(data)                                  ((0x00000080&(data))>>7)
#define MSC_REG_26_get_chg26_6(data)                                  ((0x00000040&(data))>>6)
#define MSC_REG_26_get_chg26_5(data)                                  ((0x00000020&(data))>>5)
#define MSC_REG_26_get_chg26_4(data)                                  ((0x00000010&(data))>>4)
#define MSC_REG_26_get_chg26_3(data)                                  ((0x00000008&(data))>>3)
#define MSC_REG_26_get_chg26_2(data)                                  ((0x00000004&(data))>>2)
#define MSC_REG_26_get_chg26_1(data)                                  ((0x00000002&(data))>>1)
#define MSC_REG_26_get_chg26_0(data)                                  (0x00000001&(data))


#define MSC_REG_27                                                    0x009c
#define MSC_REG_27_reg_addr                                           "0x009c"
#define MSC_REG_27_reg                                                0x009c
#define MSC_REG_27_inst_addr                                          "0x0027"
#define MSC_REG_27_inst                                               0x0027
#define MSC_REG_27_chg27_7_shift                                      (7)
#define MSC_REG_27_chg27_6_shift                                      (6)
#define MSC_REG_27_chg27_5_shift                                      (5)
#define MSC_REG_27_chg27_4_shift                                      (4)
#define MSC_REG_27_chg27_3_shift                                      (3)
#define MSC_REG_27_chg27_2_shift                                      (2)
#define MSC_REG_27_chg27_1_shift                                      (1)
#define MSC_REG_27_chg27_0_shift                                      (0)
#define MSC_REG_27_chg27_7_mask                                       (0x00000080)
#define MSC_REG_27_chg27_6_mask                                       (0x00000040)
#define MSC_REG_27_chg27_5_mask                                       (0x00000020)
#define MSC_REG_27_chg27_4_mask                                       (0x00000010)
#define MSC_REG_27_chg27_3_mask                                       (0x00000008)
#define MSC_REG_27_chg27_2_mask                                       (0x00000004)
#define MSC_REG_27_chg27_1_mask                                       (0x00000002)
#define MSC_REG_27_chg27_0_mask                                       (0x00000001)
#define MSC_REG_27_chg27_7(data)                                      (0x00000080&((data)<<7))
#define MSC_REG_27_chg27_6(data)                                      (0x00000040&((data)<<6))
#define MSC_REG_27_chg27_5(data)                                      (0x00000020&((data)<<5))
#define MSC_REG_27_chg27_4(data)                                      (0x00000010&((data)<<4))
#define MSC_REG_27_chg27_3(data)                                      (0x00000008&((data)<<3))
#define MSC_REG_27_chg27_2(data)                                      (0x00000004&((data)<<2))
#define MSC_REG_27_chg27_1(data)                                      (0x00000002&((data)<<1))
#define MSC_REG_27_chg27_0(data)                                      (0x00000001&(data))
#define MSC_REG_27_get_chg27_7(data)                                  ((0x00000080&(data))>>7)
#define MSC_REG_27_get_chg27_6(data)                                  ((0x00000040&(data))>>6)
#define MSC_REG_27_get_chg27_5(data)                                  ((0x00000020&(data))>>5)
#define MSC_REG_27_get_chg27_4(data)                                  ((0x00000010&(data))>>4)
#define MSC_REG_27_get_chg27_3(data)                                  ((0x00000008&(data))>>3)
#define MSC_REG_27_get_chg27_2(data)                                  ((0x00000004&(data))>>2)
#define MSC_REG_27_get_chg27_1(data)                                  ((0x00000002&(data))>>1)
#define MSC_REG_27_get_chg27_0(data)                                  (0x00000001&(data))


#define MSC_REG_30                                                    0x00c0
#define MSC_REG_30_reg_addr                                           "0x00c0"
#define MSC_REG_30_reg                                                0x00c0
#define MSC_REG_30_inst_addr                                          "0x0030"
#define MSC_REG_30_inst                                               0x0030
#define MSC_REG_30_conn_rdy_shift                                     (1)
#define MSC_REG_30_dcap_rdy_shift                                     (0)
#define MSC_REG_30_conn_rdy_mask                                      (0x000000FE)
#define MSC_REG_30_dcap_rdy_mask                                      (0x00000001)
#define MSC_REG_30_conn_rdy(data)                                     (0x000000FE&((data)<<1))
#define MSC_REG_30_dcap_rdy(data)                                     (0x00000001&(data))
#define MSC_REG_30_get_conn_rdy(data)                                 ((0x000000FE&(data))>>1)
#define MSC_REG_30_get_dcap_rdy(data)                                 (0x00000001&(data))


#define MSC_REG_31                                                    0x00c4
#define MSC_REG_31_reg_addr                                           "0x00c4"
#define MSC_REG_31_reg                                                0x00c4
#define MSC_REG_31_inst_addr                                          "0x0031"
#define MSC_REG_31_inst                                               0x0031
#define MSC_REG_31_link_mode_shift                                    (5)
#define MSC_REG_31_muted_shift                                        (4)
#define MSC_REG_31_path_en_shift                                      (3)
#define MSC_REG_31_clk_mode_shift                                     (0)
#define MSC_REG_31_link_mode_mask                                     (0x000000E0)
#define MSC_REG_31_muted_mask                                         (0x00000010)
#define MSC_REG_31_path_en_mask                                       (0x00000008)
#define MSC_REG_31_clk_mode_mask                                      (0x00000007)
#define MSC_REG_31_link_mode(data)                                    (0x000000E0&((data)<<5))
#define MSC_REG_31_muted(data)                                        (0x00000010&((data)<<4))
#define MSC_REG_31_path_en(data)                                      (0x00000008&((data)<<3))
#define MSC_REG_31_clk_mode(data)                                     (0x00000007&(data))
#define MSC_REG_31_get_link_mode(data)                                ((0x000000E0&(data))>>5)
#define MSC_REG_31_get_muted(data)                                    ((0x00000010&(data))>>4)
#define MSC_REG_31_get_path_en(data)                                  ((0x00000008&(data))>>3)
#define MSC_REG_31_get_clk_mode(data)                                 (0x00000007&(data))


#define MSC_REG_32                                                    0x00c8
#define MSC_REG_32_reg_addr                                           "0x00c8"
#define MSC_REG_32_reg                                                0x00c8
#define MSC_REG_32_inst_addr                                          "0x0032"
#define MSC_REG_32_inst                                               0x0032
#define MSC_REG_32_stat_32_shift                                      (0)
#define MSC_REG_32_stat_32_mask                                       (0x000000FF)
#define MSC_REG_32_stat_32(data)                                      (0x000000FF&(data))
#define MSC_REG_32_get_stat_32(data)                                  (0x000000FF&(data))


#define MSC_REG_33                                                    0x00cc
#define MSC_REG_33_reg_addr                                           "0x00cc"
#define MSC_REG_33_reg                                                0x00cc
#define MSC_REG_33_inst_addr                                          "0x0033"
#define MSC_REG_33_inst                                               0x0033
#define MSC_REG_33_stat_33_shift                                      (0)
#define MSC_REG_33_stat_33_mask                                       (0x000000FF)
#define MSC_REG_33_stat_33(data)                                      (0x000000FF&(data))
#define MSC_REG_33_get_stat_33(data)                                  (0x000000FF&(data))


#define MSC_REG_34                                                    0x00d0
#define MSC_REG_34_reg_addr                                           "0x00d0"
#define MSC_REG_34_reg                                                0x00d0
#define MSC_REG_34_inst_addr                                          "0x0034"
#define MSC_REG_34_inst                                               0x0034
#define MSC_REG_34_stat_34_shift                                      (0)
#define MSC_REG_34_stat_34_mask                                       (0x000000FF)
#define MSC_REG_34_stat_34(data)                                      (0x000000FF&(data))
#define MSC_REG_34_get_stat_34(data)                                  (0x000000FF&(data))


#define MSC_REG_35                                                    0x00d4
#define MSC_REG_35_reg_addr                                           "0x00d4"
#define MSC_REG_35_reg                                                0x00d4
#define MSC_REG_35_inst_addr                                          "0x0035"
#define MSC_REG_35_inst                                               0x0035
#define MSC_REG_35_stat_35_shift                                      (0)
#define MSC_REG_35_stat_35_mask                                       (0x000000FF)
#define MSC_REG_35_stat_35(data)                                      (0x000000FF&(data))
#define MSC_REG_35_get_stat_35(data)                                  (0x000000FF&(data))


#define MSC_REG_36                                                    0x00d8
#define MSC_REG_36_reg_addr                                           "0x00d8"
#define MSC_REG_36_reg                                                0x00d8
#define MSC_REG_36_inst_addr                                          "0x0036"
#define MSC_REG_36_inst                                               0x0036
#define MSC_REG_36_stat_36_shift                                      (0)
#define MSC_REG_36_stat_36_mask                                       (0x000000FF)
#define MSC_REG_36_stat_36(data)                                      (0x000000FF&(data))
#define MSC_REG_36_get_stat_36(data)                                  (0x000000FF&(data))


#define MSC_REG_37                                                    0x00dc
#define MSC_REG_37_reg_addr                                           "0x00dc"
#define MSC_REG_37_reg                                                0x00dc
#define MSC_REG_37_inst_addr                                          "0x0037"
#define MSC_REG_37_inst                                               0x0037
#define MSC_REG_37_stat_37_shift                                      (0)
#define MSC_REG_37_stat_37_mask                                       (0x000000FF)
#define MSC_REG_37_stat_37(data)                                      (0x000000FF&(data))
#define MSC_REG_37_get_stat_37(data)                                  (0x000000FF&(data))


#define MSC_REG_40                                                    0x0100
#define MSC_REG_40_reg_addr                                           "0x0100"
#define MSC_REG_40_reg                                                0x0100
#define MSC_REG_40_inst_addr                                          "0x0040"
#define MSC_REG_40_inst                                               0x0040
#define MSC_REG_40_scratch_40_shift                                   (0)
#define MSC_REG_40_scratch_40_mask                                    (0x000000FF)
#define MSC_REG_40_scratch_40(data)                                   (0x000000FF&(data))
#define MSC_REG_40_get_scratch_40(data)                               (0x000000FF&(data))


#define MSC_REG_41                                                    0x0104
#define MSC_REG_41_reg_addr                                           "0x0104"
#define MSC_REG_41_reg                                                0x0104
#define MSC_REG_41_inst_addr                                          "0x0041"
#define MSC_REG_41_inst                                               0x0041
#define MSC_REG_41_scratch_41_shift                                   (0)
#define MSC_REG_41_scratch_41_mask                                    (0x000000FF)
#define MSC_REG_41_scratch_41(data)                                   (0x000000FF&(data))
#define MSC_REG_41_get_scratch_41(data)                               (0x000000FF&(data))


#define MSC_REG_42                                                    0x0108
#define MSC_REG_42_reg_addr                                           "0x0108"
#define MSC_REG_42_reg                                                0x0108
#define MSC_REG_42_inst_addr                                          "0x0042"
#define MSC_REG_42_inst                                               0x0042
#define MSC_REG_42_scratch_42_shift                                   (0)
#define MSC_REG_42_scratch_42_mask                                    (0x000000FF)
#define MSC_REG_42_scratch_42(data)                                   (0x000000FF&(data))
#define MSC_REG_42_get_scratch_42(data)                               (0x000000FF&(data))


#define MSC_REG_43                                                    0x010c
#define MSC_REG_43_reg_addr                                           "0x010c"
#define MSC_REG_43_reg                                                0x010c
#define MSC_REG_43_inst_addr                                          "0x0043"
#define MSC_REG_43_inst                                               0x0043
#define MSC_REG_43_scratch_43_shift                                   (0)
#define MSC_REG_43_scratch_43_mask                                    (0x000000FF)
#define MSC_REG_43_scratch_43(data)                                   (0x000000FF&(data))
#define MSC_REG_43_get_scratch_43(data)                               (0x000000FF&(data))


#define MSC_REG_44                                                    0x0110
#define MSC_REG_44_reg_addr                                           "0x0110"
#define MSC_REG_44_reg                                                0x0110
#define MSC_REG_44_inst_addr                                          "0x0044"
#define MSC_REG_44_inst                                               0x0044
#define MSC_REG_44_scratch_44_shift                                   (0)
#define MSC_REG_44_scratch_44_mask                                    (0x000000FF)
#define MSC_REG_44_scratch_44(data)                                   (0x000000FF&(data))
#define MSC_REG_44_get_scratch_44(data)                               (0x000000FF&(data))


#define MSC_REG_45                                                    0x0114
#define MSC_REG_45_reg_addr                                           "0x0114"
#define MSC_REG_45_reg                                                0x0114
#define MSC_REG_45_inst_addr                                          "0x0045"
#define MSC_REG_45_inst                                               0x0045
#define MSC_REG_45_scratch_45_shift                                   (0)
#define MSC_REG_45_scratch_45_mask                                    (0x000000FF)
#define MSC_REG_45_scratch_45(data)                                   (0x000000FF&(data))
#define MSC_REG_45_get_scratch_45(data)                               (0x000000FF&(data))


#define MSC_REG_46                                                    0x0118
#define MSC_REG_46_reg_addr                                           "0x0118"
#define MSC_REG_46_reg                                                0x0118
#define MSC_REG_46_inst_addr                                          "0x0046"
#define MSC_REG_46_inst                                               0x0046
#define MSC_REG_46_scratch_46_shift                                   (0)
#define MSC_REG_46_scratch_46_mask                                    (0x000000FF)
#define MSC_REG_46_scratch_46(data)                                   (0x000000FF&(data))
#define MSC_REG_46_get_scratch_46(data)                               (0x000000FF&(data))


#define MSC_REG_47                                                    0x011c
#define MSC_REG_47_reg_addr                                           "0x011c"
#define MSC_REG_47_reg                                                0x011c
#define MSC_REG_47_inst_addr                                          "0x0047"
#define MSC_REG_47_inst                                               0x0047
#define MSC_REG_47_scratch_47_shift                                   (0)
#define MSC_REG_47_scratch_47_mask                                    (0x000000FF)
#define MSC_REG_47_scratch_47(data)                                   (0x000000FF&(data))
#define MSC_REG_47_get_scratch_47(data)                               (0x000000FF&(data))


#define MSC_REG_48                                                    0x0120
#define MSC_REG_48_reg_addr                                           "0x0120"
#define MSC_REG_48_reg                                                0x0120
#define MSC_REG_48_inst_addr                                          "0x0048"
#define MSC_REG_48_inst                                               0x0048
#define MSC_REG_48_scratch_48_shift                                   (0)
#define MSC_REG_48_scratch_48_mask                                    (0x000000FF)
#define MSC_REG_48_scratch_48(data)                                   (0x000000FF&(data))
#define MSC_REG_48_get_scratch_48(data)                               (0x000000FF&(data))


#define MSC_REG_49                                                    0x0124
#define MSC_REG_49_reg_addr                                           "0x0124"
#define MSC_REG_49_reg                                                0x0124
#define MSC_REG_49_inst_addr                                          "0x0049"
#define MSC_REG_49_inst                                               0x0049
#define MSC_REG_49_scratch_49_shift                                   (0)
#define MSC_REG_49_scratch_49_mask                                    (0x000000FF)
#define MSC_REG_49_scratch_49(data)                                   (0x000000FF&(data))
#define MSC_REG_49_get_scratch_49(data)                               (0x000000FF&(data))


#define MSC_REG_4A                                                    0x0128
#define MSC_REG_4A_reg_addr                                           "0x0128"
#define MSC_REG_4A_reg                                                0x0128
#define MSC_REG_4A_inst_addr                                          "0x004A"
#define MSC_REG_4A_inst                                               0x004A
#define MSC_REG_4A_scratch_4a_shift                                   (0)
#define MSC_REG_4A_scratch_4a_mask                                    (0x000000FF)
#define MSC_REG_4A_scratch_4a(data)                                   (0x000000FF&(data))
#define MSC_REG_4A_get_scratch_4a(data)                               (0x000000FF&(data))


#define MSC_REG_4B                                                    0x012c
#define MSC_REG_4B_reg_addr                                           "0x012c"
#define MSC_REG_4B_reg                                                0x012c
#define MSC_REG_4B_inst_addr                                          "0x004B"
#define MSC_REG_4B_inst                                               0x004B
#define MSC_REG_4B_scratch_4b_shift                                   (0)
#define MSC_REG_4B_scratch_4b_mask                                    (0x000000FF)
#define MSC_REG_4B_scratch_4b(data)                                   (0x000000FF&(data))
#define MSC_REG_4B_get_scratch_4b(data)                               (0x000000FF&(data))


#define MSC_REG_4C                                                    0x0130
#define MSC_REG_4C_reg_addr                                           "0x0130"
#define MSC_REG_4C_reg                                                0x0130
#define MSC_REG_4C_inst_addr                                          "0x004C"
#define MSC_REG_4C_inst                                               0x004C
#define MSC_REG_4C_scratch_4c_shift                                   (0)
#define MSC_REG_4C_scratch_4c_mask                                    (0x000000FF)
#define MSC_REG_4C_scratch_4c(data)                                   (0x000000FF&(data))
#define MSC_REG_4C_get_scratch_4c(data)                               (0x000000FF&(data))


#define MSC_REG_4D                                                    0x0134
#define MSC_REG_4D_reg_addr                                           "0x0134"
#define MSC_REG_4D_reg                                                0x0134
#define MSC_REG_4D_inst_addr                                          "0x004D"
#define MSC_REG_4D_inst                                               0x004D
#define MSC_REG_4D_scratch_4d_shift                                   (0)
#define MSC_REG_4D_scratch_4d_mask                                    (0x000000FF)
#define MSC_REG_4D_scratch_4d(data)                                   (0x000000FF&(data))
#define MSC_REG_4D_get_scratch_4d(data)                               (0x000000FF&(data))


#define MSC_REG_4E                                                    0x0138
#define MSC_REG_4E_reg_addr                                           "0x0138"
#define MSC_REG_4E_reg                                                0x0138
#define MSC_REG_4E_inst_addr                                          "0x004E"
#define MSC_REG_4E_inst                                               0x004E
#define MSC_REG_4E_scratch_4e_shift                                   (0)
#define MSC_REG_4E_scratch_4e_mask                                    (0x000000FF)
#define MSC_REG_4E_scratch_4e(data)                                   (0x000000FF&(data))
#define MSC_REG_4E_get_scratch_4e(data)                               (0x000000FF&(data))


#define MSC_REG_4F                                                    0x013c
#define MSC_REG_4F_reg_addr                                           "0x013c"
#define MSC_REG_4F_reg                                                0x013c
#define MSC_REG_4F_inst_addr                                          "0x004F"
#define MSC_REG_4F_inst                                               0x004F
#define MSC_REG_4F_scratch_4f_shift                                   (0)
#define MSC_REG_4F_scratch_4f_mask                                    (0x000000FF)
#define MSC_REG_4F_scratch_4f(data)                                   (0x000000FF&(data))
#define MSC_REG_4F_get_scratch_4f(data)                               (0x000000FF&(data))


#define MSC_REG_50                                                    0x0140
#define MSC_REG_50_reg_addr                                           "0x0140"
#define MSC_REG_50_reg                                                0x0140
#define MSC_REG_50_inst_addr                                          "0x0050"
#define MSC_REG_50_inst                                               0x0050
#define MSC_REG_50_scratch_50_shift                                   (0)
#define MSC_REG_50_scratch_50_mask                                    (0x000000FF)
#define MSC_REG_50_scratch_50(data)                                   (0x000000FF&(data))
#define MSC_REG_50_get_scratch_50(data)                               (0x000000FF&(data))


#define MSC_REG_51                                                    0x0144
#define MSC_REG_51_reg_addr                                           "0x0144"
#define MSC_REG_51_reg                                                0x0144
#define MSC_REG_51_inst_addr                                          "0x0051"
#define MSC_REG_51_inst                                               0x0051
#define MSC_REG_51_scratch_51_shift                                   (0)
#define MSC_REG_51_scratch_51_mask                                    (0x000000FF)
#define MSC_REG_51_scratch_51(data)                                   (0x000000FF&(data))
#define MSC_REG_51_get_scratch_51(data)                               (0x000000FF&(data))


#define MSC_REG_52                                                    0x0148
#define MSC_REG_52_reg_addr                                           "0x0148"
#define MSC_REG_52_reg                                                0x0148
#define MSC_REG_52_inst_addr                                          "0x0052"
#define MSC_REG_52_inst                                               0x0052
#define MSC_REG_52_scratch_52_shift                                   (0)
#define MSC_REG_52_scratch_52_mask                                    (0x000000FF)
#define MSC_REG_52_scratch_52(data)                                   (0x000000FF&(data))
#define MSC_REG_52_get_scratch_52(data)                               (0x000000FF&(data))


#define MSC_REG_53                                                    0x014c
#define MSC_REG_53_reg_addr                                           "0x014c"
#define MSC_REG_53_reg                                                0x014c
#define MSC_REG_53_inst_addr                                          "0x0053"
#define MSC_REG_53_inst                                               0x0053
#define MSC_REG_53_scratch_53_shift                                   (0)
#define MSC_REG_53_scratch_53_mask                                    (0x000000FF)
#define MSC_REG_53_scratch_53(data)                                   (0x000000FF&(data))
#define MSC_REG_53_get_scratch_53(data)                               (0x000000FF&(data))


#define MSC_REG_54                                                    0x0150
#define MSC_REG_54_reg_addr                                           "0x0150"
#define MSC_REG_54_reg                                                0x0150
#define MSC_REG_54_inst_addr                                          "0x0054"
#define MSC_REG_54_inst                                               0x0054
#define MSC_REG_54_scratch_54_shift                                   (0)
#define MSC_REG_54_scratch_54_mask                                    (0x000000FF)
#define MSC_REG_54_scratch_54(data)                                   (0x000000FF&(data))
#define MSC_REG_54_get_scratch_54(data)                               (0x000000FF&(data))


#define MSC_REG_55                                                    0x0154
#define MSC_REG_55_reg_addr                                           "0x0154"
#define MSC_REG_55_reg                                                0x0154
#define MSC_REG_55_inst_addr                                          "0x0055"
#define MSC_REG_55_inst                                               0x0055
#define MSC_REG_55_scratch_55_shift                                   (0)
#define MSC_REG_55_scratch_55_mask                                    (0x000000FF)
#define MSC_REG_55_scratch_55(data)                                   (0x000000FF&(data))
#define MSC_REG_55_get_scratch_55(data)                               (0x000000FF&(data))


#define MSC_REG_56                                                    0x0158
#define MSC_REG_56_reg_addr                                           "0x0158"
#define MSC_REG_56_reg                                                0x0158
#define MSC_REG_56_inst_addr                                          "0x0056"
#define MSC_REG_56_inst                                               0x0056
#define MSC_REG_56_scratch_56_shift                                   (0)
#define MSC_REG_56_scratch_56_mask                                    (0x000000FF)
#define MSC_REG_56_scratch_56(data)                                   (0x000000FF&(data))
#define MSC_REG_56_get_scratch_56(data)                               (0x000000FF&(data))


#define MSC_REG_57                                                    0x015c
#define MSC_REG_57_reg_addr                                           "0x015c"
#define MSC_REG_57_reg                                                0x015c
#define MSC_REG_57_inst_addr                                          "0x0057"
#define MSC_REG_57_inst                                               0x0057
#define MSC_REG_57_scratch_57_shift                                   (0)
#define MSC_REG_57_scratch_57_mask                                    (0x000000FF)
#define MSC_REG_57_scratch_57(data)                                   (0x000000FF&(data))
#define MSC_REG_57_get_scratch_57(data)                               (0x000000FF&(data))


#define MSC_REG_58                                                    0x0160
#define MSC_REG_58_reg_addr                                           "0x0160"
#define MSC_REG_58_reg                                                0x0160
#define MSC_REG_58_inst_addr                                          "0x0058"
#define MSC_REG_58_inst                                               0x0058
#define MSC_REG_58_scratch_58_shift                                   (0)
#define MSC_REG_58_scratch_58_mask                                    (0x000000FF)
#define MSC_REG_58_scratch_58(data)                                   (0x000000FF&(data))
#define MSC_REG_58_get_scratch_58(data)                               (0x000000FF&(data))


#define MSC_REG_59                                                    0x0164
#define MSC_REG_59_reg_addr                                           "0x0164"
#define MSC_REG_59_reg                                                0x0164
#define MSC_REG_59_inst_addr                                          "0x0059"
#define MSC_REG_59_inst                                               0x0059
#define MSC_REG_59_scratch_59_shift                                   (0)
#define MSC_REG_59_scratch_59_mask                                    (0x000000FF)
#define MSC_REG_59_scratch_59(data)                                   (0x000000FF&(data))
#define MSC_REG_59_get_scratch_59(data)                               (0x000000FF&(data))


#define MSC_REG_5A                                                    0x0168
#define MSC_REG_5A_reg_addr                                           "0x0168"
#define MSC_REG_5A_reg                                                0x0168
#define MSC_REG_5A_inst_addr                                          "0x005A"
#define MSC_REG_5A_inst                                               0x005A
#define MSC_REG_5A_scratch_5a_shift                                   (0)
#define MSC_REG_5A_scratch_5a_mask                                    (0x000000FF)
#define MSC_REG_5A_scratch_5a(data)                                   (0x000000FF&(data))
#define MSC_REG_5A_get_scratch_5a(data)                               (0x000000FF&(data))


#define MSC_REG_5B                                                    0x016c
#define MSC_REG_5B_reg_addr                                           "0x016c"
#define MSC_REG_5B_reg                                                0x016c
#define MSC_REG_5B_inst_addr                                          "0x005B"
#define MSC_REG_5B_inst                                               0x005B
#define MSC_REG_5B_scratch_5b_shift                                   (0)
#define MSC_REG_5B_scratch_5b_mask                                    (0x000000FF)
#define MSC_REG_5B_scratch_5b(data)                                   (0x000000FF&(data))
#define MSC_REG_5B_get_scratch_5b(data)                               (0x000000FF&(data))


#define MSC_REG_5C                                                    0x0170
#define MSC_REG_5C_reg_addr                                           "0x0170"
#define MSC_REG_5C_reg                                                0x0170
#define MSC_REG_5C_inst_addr                                          "0x005C"
#define MSC_REG_5C_inst                                               0x005C
#define MSC_REG_5C_scratch_5c_shift                                   (0)
#define MSC_REG_5C_scratch_5c_mask                                    (0x000000FF)
#define MSC_REG_5C_scratch_5c(data)                                   (0x000000FF&(data))
#define MSC_REG_5C_get_scratch_5c(data)                               (0x000000FF&(data))


#define MSC_REG_5D                                                    0x0174
#define MSC_REG_5D_reg_addr                                           "0x0174"
#define MSC_REG_5D_reg                                                0x0174
#define MSC_REG_5D_inst_addr                                          "0x005D"
#define MSC_REG_5D_inst                                               0x005D
#define MSC_REG_5D_scratch_5d_shift                                   (0)
#define MSC_REG_5D_scratch_5d_mask                                    (0x000000FF)
#define MSC_REG_5D_scratch_5d(data)                                   (0x000000FF&(data))
#define MSC_REG_5D_get_scratch_5d(data)                               (0x000000FF&(data))


#define MSC_REG_5E                                                    0x0178
#define MSC_REG_5E_reg_addr                                           "0x0178"
#define MSC_REG_5E_reg                                                0x0178
#define MSC_REG_5E_inst_addr                                          "0x005E"
#define MSC_REG_5E_inst                                               0x005E
#define MSC_REG_5E_scratch_5e_shift                                   (0)
#define MSC_REG_5E_scratch_5e_mask                                    (0x000000FF)
#define MSC_REG_5E_scratch_5e(data)                                   (0x000000FF&(data))
#define MSC_REG_5E_get_scratch_5e(data)                               (0x000000FF&(data))


#define MSC_REG_5F                                                    0x017c
#define MSC_REG_5F_reg_addr                                           "0x017c"
#define MSC_REG_5F_reg                                                0x017c
#define MSC_REG_5F_inst_addr                                          "0x005F"
#define MSC_REG_5F_inst                                               0x005F
#define MSC_REG_5F_scratch_5f_shift                                   (0)
#define MSC_REG_5F_scratch_5f_mask                                    (0x000000FF)
#define MSC_REG_5F_scratch_5f(data)                                   (0x000000FF&(data))
#define MSC_REG_5F_get_scratch_5f(data)                               (0x000000FF&(data))


#endif // _HDMI_RX_MHL_MSC_REG_H
