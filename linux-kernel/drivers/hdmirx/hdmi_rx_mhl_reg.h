//*******************************************************************************
//START OF MHL REG
//*******************************************************************************
#ifndef _HDMI_RX_MHL_REG_H
#define _HDMI_RX_MHL_REG_H

// HDMI MHL register(HDMI_RX_MHL)
// HDMI Register Address base 0x1800f600

#define MHL_DAL_CTRL                                                  0x000
#define MHL_DAL_CTRL_reg_addr                                         "0x000"
#define MHL_DAL_CTRL_reg                                              0x000
#define MHL_DAL_CTRL_inst_addr                                        "0x0180"
#define MHL_DAL_CTRL_inst                                             0x0180
#define MHL_DAL_CTRL_mhl_de_sel_shift                                 (28)
#define MHL_DAL_CTRL_mhl_tran_err_thrd_shift                          (24)
#define MHL_DAL_CTRL_mhl_neg_de_err_thrd_shift                        (20)
#define MHL_DAL_CTRL_mhl_pos_de_err_thrd_shift                        (16)
#define MHL_DAL_CTRL_dummy1800d600_15_shift                           (15)
#define MHL_DAL_CTRL_mhl_neg_de_lowbd_shift                           (8)
#define MHL_DAL_CTRL_dummy1800d600_7_6_shift                          (6)
#define MHL_DAL_CTRL_mhl_pos_de_lowbd_shift                           (0)
#define MHL_DAL_CTRL_mhl_de_sel_mask                                  (0x10000000)
#define MHL_DAL_CTRL_mhl_tran_err_thrd_mask                           (0x0F000000)
#define MHL_DAL_CTRL_mhl_neg_de_err_thrd_mask                         (0x00F00000)
#define MHL_DAL_CTRL_mhl_pos_de_err_thrd_mask                         (0x000F0000)
#define MHL_DAL_CTRL_dummy1800d600_15_mask                            (0x00008000)
#define MHL_DAL_CTRL_mhl_neg_de_lowbd_mask                            (0x00007F00)
#define MHL_DAL_CTRL_dummy1800d600_7_6_mask                           (0x000000C0)
#define MHL_DAL_CTRL_mhl_pos_de_lowbd_mask                            (0x0000003F)
#define MHL_DAL_CTRL_mhl_de_sel(data)                                 (0x10000000&((data)<<28))
#define MHL_DAL_CTRL_mhl_tran_err_thrd(data)                          (0x0F000000&((data)<<24))
#define MHL_DAL_CTRL_mhl_neg_de_err_thrd(data)                        (0x00F00000&((data)<<20))
#define MHL_DAL_CTRL_mhl_pos_de_err_thrd(data)                        (0x000F0000&((data)<<16))
#define MHL_DAL_CTRL_dummy1800d600_15(data)                           (0x00008000&((data)<<15))
#define MHL_DAL_CTRL_mhl_neg_de_lowbd(data)                           (0x00007F00&((data)<<8))
#define MHL_DAL_CTRL_dummy1800d600_7_6(data)                          (0x000000C0&((data)<<6))
#define MHL_DAL_CTRL_mhl_pos_de_lowbd(data)                           (0x0000003F&(data))
#define MHL_DAL_CTRL_get_mhl_de_sel(data)                             ((0x10000000&(data))>>28)
#define MHL_DAL_CTRL_get_mhl_tran_err_thrd(data)                      ((0x0F000000&(data))>>24)
#define MHL_DAL_CTRL_get_mhl_neg_de_err_thrd(data)                    ((0x00F00000&(data))>>20)
#define MHL_DAL_CTRL_get_mhl_pos_de_err_thrd(data)                    ((0x000F0000&(data))>>16)
#define MHL_DAL_CTRL_get_dummy1800d600_15(data)                       ((0x00008000&(data))>>15)
#define MHL_DAL_CTRL_get_mhl_neg_de_lowbd(data)                       ((0x00007F00&(data))>>8)
#define MHL_DAL_CTRL_get_dummy1800d600_7_6(data)                      ((0x000000C0&(data))>>6)
#define MHL_DAL_CTRL_get_mhl_pos_de_lowbd(data)                       (0x0000003F&(data))


#define MHL_DAL_STATUS                                                0x004
#define MHL_DAL_STATUS_reg_addr                                       "0x004"
#define MHL_DAL_STATUS_reg                                            0x004
#define MHL_DAL_STATUS_inst_addr                                      "0x0181"
#define MHL_DAL_STATUS_inst                                           0x0181
#define MHL_DAL_STATUS_mhl_de_glitch_cycle_shift                      (4)
#define MHL_DAL_STATUS_mhl_ver_1_2_shift                              (2)
#define MHL_DAL_STATUS_mhl_de_glitch_en_shift                         (1)
#define MHL_DAL_STATUS_pn_swap_shift                                  (0)
#define MHL_DAL_STATUS_mhl_de_glitch_cycle_mask                       (0x000000F0)
#define MHL_DAL_STATUS_mhl_ver_1_2_mask                               (0x00000004)
#define MHL_DAL_STATUS_mhl_de_glitch_en_mask                          (0x00000002)
#define MHL_DAL_STATUS_pn_swap_mask                                   (0x00000001)
#define MHL_DAL_STATUS_mhl_de_glitch_cycle(data)                      (0x000000F0&((data)<<4))
#define MHL_DAL_STATUS_mhl_ver_1_2(data)                              (0x00000004&((data)<<2))
#define MHL_DAL_STATUS_mhl_de_glitch_en(data)                         (0x00000002&((data)<<1))
#define MHL_DAL_STATUS_pn_swap(data)                                  (0x00000001&(data))
#define MHL_DAL_STATUS_get_mhl_de_glitch_cycle(data)                  ((0x000000F0&(data))>>4)
#define MHL_DAL_STATUS_get_mhl_ver_1_2(data)                          ((0x00000004&(data))>>2)
#define MHL_DAL_STATUS_get_mhl_de_glitch_en(data)                     ((0x00000002&(data))>>1)
#define MHL_DAL_STATUS_get_pn_swap(data)                              (0x00000001&(data))


#define MHL_MD_CTRL                                                   0x008
#define MHL_MD_CTRL_reg_addr                                          "0x008"
#define MHL_MD_CTRL_reg                                               0x008
#define MHL_MD_CTRL_inst_addr                                         "0x0182"
#define MHL_MD_CTRL_inst                                              0x0182
#define MHL_MD_CTRL_mhl_mode_err_wd_shift                             (26)
#define MHL_MD_CTRL_mhl_mode_err_int_shift                            (25)
#define MHL_MD_CTRL_mhl_mode_shift                                    (24)
#define MHL_MD_CTRL_mhl_mode_det_per_sel_shift                        (20)
#define MHL_MD_CTRL_mhl_mode_det_per_shift                            (4)
#define MHL_MD_CTRL_mhl_mode_det_shift                                (2)
#define MHL_MD_CTRL_mhl_mode_det_cont_shift                           (1)
#define MHL_MD_CTRL_mhl_mode_det_start_shift                          (0)
#define MHL_MD_CTRL_mhl_mode_err_wd_mask                              (0x04000000)
#define MHL_MD_CTRL_mhl_mode_err_int_mask                             (0x02000000)
#define MHL_MD_CTRL_mhl_mode_mask                                     (0x01000000)
#define MHL_MD_CTRL_mhl_mode_det_per_sel_mask                         (0x00100000)
#define MHL_MD_CTRL_mhl_mode_det_per_mask                             (0x000FFFF0)
#define MHL_MD_CTRL_mhl_mode_det_mask                                 (0x0000000C)
#define MHL_MD_CTRL_mhl_mode_det_cont_mask                            (0x00000002)
#define MHL_MD_CTRL_mhl_mode_det_start_mask                           (0x00000001)
#define MHL_MD_CTRL_mhl_mode_err_wd(data)                             (0x04000000&((data)<<26))
#define MHL_MD_CTRL_mhl_mode_err_int(data)                            (0x02000000&((data)<<25))
#define MHL_MD_CTRL_mhl_mode(data)                                    (0x01000000&((data)<<24))
#define MHL_MD_CTRL_mhl_mode_det_per_sel(data)                        (0x00100000&((data)<<20))
#define MHL_MD_CTRL_mhl_mode_det_per(data)                            (0x000FFFF0&((data)<<4))
#define MHL_MD_CTRL_mhl_mode_det(data)                                (0x0000000C&((data)<<2))
#define MHL_MD_CTRL_mhl_mode_det_cont(data)                           (0x00000002&((data)<<1))
#define MHL_MD_CTRL_mhl_mode_det_start(data)                          (0x00000001&(data))
#define MHL_MD_CTRL_get_mhl_mode_err_wd(data)                         ((0x04000000&(data))>>26)
#define MHL_MD_CTRL_get_mhl_mode_err_int(data)                        ((0x02000000&(data))>>25)
#define MHL_MD_CTRL_get_mhl_mode(data)                                ((0x01000000&(data))>>24)
#define MHL_MD_CTRL_get_mhl_mode_det_per_sel(data)                    ((0x00100000&(data))>>20)
#define MHL_MD_CTRL_get_mhl_mode_det_per(data)                        ((0x000FFFF0&(data))>>4)
#define MHL_MD_CTRL_get_mhl_mode_det(data)                            ((0x0000000C&(data))>>2)
#define MHL_MD_CTRL_get_mhl_mode_det_cont(data)                       ((0x00000002&(data))>>1)
#define MHL_MD_CTRL_get_mhl_mode_det_start(data)                      (0x00000001&(data))


#define MHL_MD_THRESHOLD                                              0x00c
#define MHL_MD_THRESHOLD_reg_addr                                     "0x00c"
#define MHL_MD_THRESHOLD_reg                                          0x00c
#define MHL_MD_THRESHOLD_inst_addr                                    "0x0183"
#define MHL_MD_THRESHOLD_inst                                         0x0183
#define MHL_MD_THRESHOLD_threshold_pp_shift                           (16)
#define MHL_MD_THRESHOLD_threshold_24bits_shift                       (0)
#define MHL_MD_THRESHOLD_threshold_pp_mask                            (0xFFFF0000)
#define MHL_MD_THRESHOLD_threshold_24bits_mask                        (0x0000FFFF)
#define MHL_MD_THRESHOLD_threshold_pp(data)                           (0xFFFF0000&((data)<<16))
#define MHL_MD_THRESHOLD_threshold_24bits(data)                       (0x0000FFFF&(data))
#define MHL_MD_THRESHOLD_get_threshold_pp(data)                       ((0xFFFF0000&(data))>>16)
#define MHL_MD_THRESHOLD_get_threshold_24bits(data)                   (0x0000FFFF&(data))


#define MHL_MD_CNT                                                    0x010
#define MHL_MD_CNT_reg_addr                                           "0x010"
#define MHL_MD_CNT_reg                                                0x010
#define MHL_MD_CNT_inst_addr                                          "0x0184"
#define MHL_MD_CNT_inst                                               0x0184
#define MHL_MD_CNT_det_cnt_pp_shift                                   (16)
#define MHL_MD_CNT_det_cnt_24bit_shift                                (0)
#define MHL_MD_CNT_det_cnt_pp_mask                                    (0xFFFF0000)
#define MHL_MD_CNT_det_cnt_24bit_mask                                 (0x0000FFFF)
#define MHL_MD_CNT_det_cnt_pp(data)                                   (0xFFFF0000&((data)<<16))
#define MHL_MD_CNT_det_cnt_24bit(data)                                (0x0000FFFF&(data))
#define MHL_MD_CNT_get_det_cnt_pp(data)                               ((0xFFFF0000&(data))>>16)
#define MHL_MD_CNT_get_det_cnt_24bit(data)                            (0x0000FFFF&(data))


#define MHL_DEMUX_CTRL                                                0x014
#define MHL_DEMUX_CTRL_reg_addr                                       "0x014"
#define MHL_DEMUX_CTRL_reg                                            0x014
#define MHL_DEMUX_CTRL_inst_addr                                      "0x0185"
#define MHL_DEMUX_CTRL_inst                                           0x0185
#define MHL_DEMUX_CTRL_video_wd_en_shift                              (12)
#define MHL_DEMUX_CTRL_audio_wd_en_shift                              (11)
#define MHL_DEMUX_CTRL_ch_bound_chg_wd_en_shift                       (10)
#define MHL_DEMUX_CTRL_ch_bound_chg_int_en_shift                      (9)
#define MHL_DEMUX_CTRL_ch_bound_chg_det_shift                         (8)
#define MHL_DEMUX_CTRL_demux_sel_shift                                (4)
#define MHL_DEMUX_CTRL_mhl_pp_en_shift                                (1)
#define MHL_DEMUX_CTRL_mhl_en_shift                                   (0)
#define MHL_DEMUX_CTRL_video_wd_en_mask                               (0x00001000)
#define MHL_DEMUX_CTRL_audio_wd_en_mask                               (0x00000800)
#define MHL_DEMUX_CTRL_ch_bound_chg_wd_en_mask                        (0x00000400)
#define MHL_DEMUX_CTRL_ch_bound_chg_int_en_mask                       (0x00000200)
#define MHL_DEMUX_CTRL_ch_bound_chg_det_mask                          (0x00000100)
#define MHL_DEMUX_CTRL_demux_sel_mask                                 (0x00000070)
#define MHL_DEMUX_CTRL_mhl_pp_en_mask                                 (0x00000002)
#define MHL_DEMUX_CTRL_mhl_en_mask                                    (0x00000001)
#define MHL_DEMUX_CTRL_video_wd_en(data)                              (0x00001000&((data)<<12))
#define MHL_DEMUX_CTRL_audio_wd_en(data)                              (0x00000800&((data)<<11))
#define MHL_DEMUX_CTRL_ch_bound_chg_wd_en(data)                       (0x00000400&((data)<<10))
#define MHL_DEMUX_CTRL_ch_bound_chg_int_en(data)                      (0x00000200&((data)<<9))
#define MHL_DEMUX_CTRL_ch_bound_chg_det(data)                         (0x00000100&((data)<<8))
#define MHL_DEMUX_CTRL_demux_sel(data)                                (0x00000070&((data)<<4))
#define MHL_DEMUX_CTRL_mhl_pp_en(data)                                (0x00000002&((data)<<1))
#define MHL_DEMUX_CTRL_mhl_en(data)                                   (0x00000001&(data))
#define MHL_DEMUX_CTRL_get_video_wd_en(data)                          ((0x00001000&(data))>>12)
#define MHL_DEMUX_CTRL_get_audio_wd_en(data)                          ((0x00000800&(data))>>11)
#define MHL_DEMUX_CTRL_get_ch_bound_chg_wd_en(data)                   ((0x00000400&(data))>>10)
#define MHL_DEMUX_CTRL_get_ch_bound_chg_int_en(data)                  ((0x00000200&(data))>>9)
#define MHL_DEMUX_CTRL_get_ch_bound_chg_det(data)                     ((0x00000100&(data))>>8)
#define MHL_DEMUX_CTRL_get_demux_sel(data)                            ((0x00000070&(data))>>4)
#define MHL_DEMUX_CTRL_get_mhl_pp_en(data)                            ((0x00000002&(data))>>1)
#define MHL_DEMUX_CTRL_get_mhl_en(data)                               (0x00000001&(data))


#define MHL_STATUS                                                    0x018
#define MHL_STATUS_reg_addr                                           "0x018"
#define MHL_STATUS_reg                                                0x018
#define MHL_STATUS_inst_addr                                          "0x0186"
#define MHL_STATUS_inst                                               0x0186
#define MHL_STATUS_ch_bound_chg_shift                                 (12)
#define MHL_STATUS_mhl_mode_err_shift                                 (8)
#define MHL_STATUS_non_ctrl_glitch_occ_flag_shift                     (5)
#define MHL_STATUS_ctrl_glitch_occr_flag_shift                        (4)
#define MHL_STATUS_mhl_tran_err_flag_shift                            (2)
#define MHL_STATUS_mhl_pos_de_err_flag_shift                          (1)
#define MHL_STATUS_mhl_neg_de_err_flag_shift                          (0)
#define MHL_STATUS_ch_bound_chg_mask                                  (0x00001000)
#define MHL_STATUS_mhl_mode_err_mask                                  (0x00000100)
#define MHL_STATUS_non_ctrl_glitch_occ_flag_mask                      (0x00000020)
#define MHL_STATUS_ctrl_glitch_occr_flag_mask                         (0x00000010)
#define MHL_STATUS_mhl_tran_err_flag_mask                             (0x00000004)
#define MHL_STATUS_mhl_pos_de_err_flag_mask                           (0x00000002)
#define MHL_STATUS_mhl_neg_de_err_flag_mask                           (0x00000001)
#define MHL_STATUS_ch_bound_chg(data)                                 (0x00001000&((data)<<12))
#define MHL_STATUS_mhl_mode_err(data)                                 (0x00000100&((data)<<8))
#define MHL_STATUS_non_ctrl_glitch_occ_flag(data)                     (0x00000020&((data)<<5))
#define MHL_STATUS_ctrl_glitch_occr_flag(data)                        (0x00000010&((data)<<4))
#define MHL_STATUS_mhl_tran_err_flag(data)                            (0x00000004&((data)<<2))
#define MHL_STATUS_mhl_pos_de_err_flag(data)                          (0x00000002&((data)<<1))
#define MHL_STATUS_mhl_neg_de_err_flag(data)                          (0x00000001&(data))
#define MHL_STATUS_get_ch_bound_chg(data)                             ((0x00001000&(data))>>12)
#define MHL_STATUS_get_mhl_mode_err(data)                             ((0x00000100&(data))>>8)
#define MHL_STATUS_get_non_ctrl_glitch_occ_flag(data)                 ((0x00000020&(data))>>5)
#define MHL_STATUS_get_ctrl_glitch_occr_flag(data)                    ((0x00000010&(data))>>4)
#define MHL_STATUS_get_mhl_tran_err_flag(data)                        ((0x00000004&(data))>>2)
#define MHL_STATUS_get_mhl_pos_de_err_flag(data)                      ((0x00000002&(data))>>1)
#define MHL_STATUS_get_mhl_neg_de_err_flag(data)                      (0x00000001&(data))

//*******************************************************************************
//End of MHL
//*******************************************************************************

#endif //_HDMI_RX_MHL_REG_H
