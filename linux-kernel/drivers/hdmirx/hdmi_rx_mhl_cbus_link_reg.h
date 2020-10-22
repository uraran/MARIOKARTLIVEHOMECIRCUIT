#ifndef _HDMI_RX_MHL_CBUS_LINK_H
#define _HDMI_RX_MHL_CBUS_LINK_H
// ===============================================
// HDMI CBSU_STANDBY(HDMI_RX_MHL_CBUS_LINK)
// HDMI Register Address base 0x1800f200

#define CBUS_LINK_00                                                  0x000
#define CBUS_LINK_00_reg_addr                                         "0x000"
#define CBUS_LINK_00_reg                                              0x000
#define CBUS_LINK_00_inst_addr                                        "0x0080"
#define CBUS_LINK_00_inst                                             0x0080
#define CBUS_LINK_00_sync0_hb_8_0_shift                               (9)
#define CBUS_LINK_00_sync0_lb_8_0_shift                               (0)
#define CBUS_LINK_00_sync0_hb_8_0_mask                                (0x0003FE00)
#define CBUS_LINK_00_sync0_lb_8_0_mask                                (0x000001FF)
#define CBUS_LINK_00_sync0_hb_8_0(data)                               (0x0003FE00&((data)<<9))
#define CBUS_LINK_00_sync0_lb_8_0(data)                               (0x000001FF&(data))
#define CBUS_LINK_00_get_sync0_hb_8_0(data)                           ((0x0003FE00&(data))>>9)
#define CBUS_LINK_00_get_sync0_lb_8_0(data)                           (0x000001FF&(data))


#define CBUS_LINK_01                                                  0x004
#define CBUS_LINK_01_reg_addr                                         "0x004"
#define CBUS_LINK_01_reg                                              0x004
#define CBUS_LINK_01_inst_addr                                        "0x0081"
#define CBUS_LINK_01_inst                                             0x0081
#define CBUS_LINK_01_sync1_hb_7_0_shift                               (8)
#define CBUS_LINK_01_sync1_lb_7_0_shift                               (0)
#define CBUS_LINK_01_sync1_hb_7_0_mask                                (0x0000FF00)
#define CBUS_LINK_01_sync1_lb_7_0_mask                                (0x000000FF)
#define CBUS_LINK_01_sync1_hb_7_0(data)                               (0x0000FF00&((data)<<8))
#define CBUS_LINK_01_sync1_lb_7_0(data)                               (0x000000FF&(data))
#define CBUS_LINK_01_get_sync1_hb_7_0(data)                           ((0x0000FF00&(data))>>8)
#define CBUS_LINK_01_get_sync1_lb_7_0(data)                           (0x000000FF&(data))


#define CBUS_LINK_02                                                  0x008
#define CBUS_LINK_02_reg_addr                                         "0x008"
#define CBUS_LINK_02_reg                                              0x008
#define CBUS_LINK_02_inst_addr                                        "0x0082"
#define CBUS_LINK_02_inst                                             0x0082
#define CBUS_LINK_02_sync_bit_time_shift                              (0)
#define CBUS_LINK_02_sync_bit_time_mask                               (0x000000FF)
#define CBUS_LINK_02_sync_bit_time(data)                              (0x000000FF&(data))
#define CBUS_LINK_02_get_sync_bit_time(data)                          (0x000000FF&(data))


#define CBUS_LINK_03                                                  0x00c
#define CBUS_LINK_03_reg_addr                                         "0x00c"
#define CBUS_LINK_03_reg                                              0x00c
#define CBUS_LINK_03_inst_addr                                        "0x0083"
#define CBUS_LINK_03_inst                                             0x0083
#define CBUS_LINK_03_abs_thres_en_shift                               (8)
#define CBUS_LINK_03_abs_threshold_shift                              (0)
#define CBUS_LINK_03_abs_thres_en_mask                                (0x00000100)
#define CBUS_LINK_03_abs_threshold_mask                               (0x000000FF)
#define CBUS_LINK_03_abs_thres_en(data)                               (0x00000100&((data)<<8))
#define CBUS_LINK_03_abs_threshold(data)                              (0x000000FF&(data))
#define CBUS_LINK_03_get_abs_thres_en(data)                           ((0x00000100&(data))>>8)
#define CBUS_LINK_03_get_abs_threshold(data)                          (0x000000FF&(data))


#define CBUS_LINK_04                                                  0x010
#define CBUS_LINK_04_reg_addr                                         "0x010"
#define CBUS_LINK_04_reg                                              0x010
#define CBUS_LINK_04_inst_addr                                        "0x0084"
#define CBUS_LINK_04_inst                                             0x0084
#define CBUS_LINK_04_parity_time_shift                                (0)
#define CBUS_LINK_04_parity_time_mask                                 (0x000000FF)
#define CBUS_LINK_04_parity_time(data)                                (0x000000FF&(data))
#define CBUS_LINK_04_get_parity_time(data)                            (0x000000FF&(data))


#define CBUS_LINK_05                                                  0x014
#define CBUS_LINK_05_reg_addr                                         "0x014"
#define CBUS_LINK_05_reg                                              0x014
#define CBUS_LINK_05_inst_addr                                        "0x0085"
#define CBUS_LINK_05_inst                                             0x0085
#define CBUS_LINK_05_parity_fail_shift                                (7)
#define CBUS_LINK_05_parity_irq_en_shift                              (6)
#define CBUS_LINK_05_ctrl_16_resv_shift                               (5)
#define CBUS_LINK_05_parity_limit_shift                               (0)
#define CBUS_LINK_05_parity_fail_mask                                 (0x00000080)
#define CBUS_LINK_05_parity_irq_en_mask                               (0x00000040)
#define CBUS_LINK_05_ctrl_16_resv_mask                                (0x00000020)
#define CBUS_LINK_05_parity_limit_mask                                (0x0000001F)
#define CBUS_LINK_05_parity_fail(data)                                (0x00000080&((data)<<7))
#define CBUS_LINK_05_parity_irq_en(data)                              (0x00000040&((data)<<6))
#define CBUS_LINK_05_ctrl_16_resv(data)                               (0x00000020&((data)<<5))
#define CBUS_LINK_05_parity_limit(data)                               (0x0000001F&(data))
#define CBUS_LINK_05_get_parity_fail(data)                            ((0x00000080&(data))>>7)
#define CBUS_LINK_05_get_parity_irq_en(data)                          ((0x00000040&(data))>>6)
#define CBUS_LINK_05_get_ctrl_16_resv(data)                           ((0x00000020&(data))>>5)
#define CBUS_LINK_05_get_parity_limit(data)                           (0x0000001F&(data))


#define CBUS_LINK_06                                                  0x018
#define CBUS_LINK_06_reg_addr                                         "0x018"
#define CBUS_LINK_06_reg                                              0x018
#define CBUS_LINK_06_inst_addr                                        "0x0086"
#define CBUS_LINK_06_inst                                             0x0086
#define CBUS_LINK_06_ack_fall_shift                                   (0)
#define CBUS_LINK_06_ack_fall_mask                                    (0x0000007F)
#define CBUS_LINK_06_ack_fall(data)                                   (0x0000007F&(data))
#define CBUS_LINK_06_get_ack_fall(data)                               (0x0000007F&(data))


#define CBUS_LINK_07                                                  0x01c
#define CBUS_LINK_07_reg_addr                                         "0x01c"
#define CBUS_LINK_07_reg                                              0x01c
#define CBUS_LINK_07_inst_addr                                        "0x0087"
#define CBUS_LINK_07_inst                                             0x0087
#define CBUS_LINK_07_ack_0_shift                                      (0)
#define CBUS_LINK_07_ack_0_mask                                       (0x0000007F)
#define CBUS_LINK_07_ack_0(data)                                      (0x0000007F&(data))
#define CBUS_LINK_07_get_ack_0(data)                                  (0x0000007F&(data))


#define CBUS_LINK_08                                                  0x020
#define CBUS_LINK_08_reg_addr                                         "0x020"
#define CBUS_LINK_08_reg                                              0x020
#define CBUS_LINK_08_inst_addr                                        "0x0088"
#define CBUS_LINK_08_inst                                             0x0088
#define CBUS_LINK_08_tx_bit_time_shift                                (0)
#define CBUS_LINK_08_tx_bit_time_mask                                 (0x000000FF)
#define CBUS_LINK_08_tx_bit_time(data)                                (0x000000FF&(data))
#define CBUS_LINK_08_get_tx_bit_time(data)                            (0x000000FF&(data))


#define CBUS_LINK_09                                                  0x024
#define CBUS_LINK_09_reg_addr                                         "0x024"
#define CBUS_LINK_09_reg                                              0x024
#define CBUS_LINK_09_inst_addr                                        "0x0089"
#define CBUS_LINK_09_inst                                             0x0089
#define CBUS_LINK_09_chk_win_up_shift                                 (5)
#define CBUS_LINK_09_chk_win_lo_shift                                 (3)
#define CBUS_LINK_09_fast_reply_en_shift                              (2)
#define CBUS_LINK_09_ctrl_1b_resv_shift                               (0)
#define CBUS_LINK_09_chk_win_up_mask                                  (0x000000E0)
#define CBUS_LINK_09_chk_win_lo_mask                                  (0x00000018)
#define CBUS_LINK_09_fast_reply_en_mask                               (0x00000004)
#define CBUS_LINK_09_ctrl_1b_resv_mask                                (0x00000003)
#define CBUS_LINK_09_chk_win_up(data)                                 (0x000000E0&((data)<<5))
#define CBUS_LINK_09_chk_win_lo(data)                                 (0x00000018&((data)<<3))
#define CBUS_LINK_09_fast_reply_en(data)                              (0x00000004&((data)<<2))
#define CBUS_LINK_09_ctrl_1b_resv(data)                               (0x00000003&(data))
#define CBUS_LINK_09_get_chk_win_up(data)                             ((0x000000E0&(data))>>5)
#define CBUS_LINK_09_get_chk_win_lo(data)                             ((0x00000018&(data))>>3)
#define CBUS_LINK_09_get_fast_reply_en(data)                          ((0x00000004&(data))>>2)
#define CBUS_LINK_09_get_ctrl_1b_resv(data)                           (0x00000003&(data))


#define CBUS_LINK_0A                                                  0x028
#define CBUS_LINK_0A_reg_addr                                         "0x028"
#define CBUS_LINK_0A_reg                                              0x028
#define CBUS_LINK_0A_inst_addr                                        "0x008A"
#define CBUS_LINK_0A_inst                                             0x008A
#define CBUS_LINK_0A_tx_ack_fal_shift                                 (0)
#define CBUS_LINK_0A_tx_ack_fal_mask                                  (0x0000007F)
#define CBUS_LINK_0A_tx_ack_fal(data)                                 (0x0000007F&(data))
#define CBUS_LINK_0A_get_tx_ack_fal(data)                             (0x0000007F&(data))


#define CBUS_LINK_0B                                                  0x02c
#define CBUS_LINK_0B_reg_addr                                         "0x02c"
#define CBUS_LINK_0B_reg                                              0x02c
#define CBUS_LINK_0B_inst_addr                                        "0x008B"
#define CBUS_LINK_0B_inst                                             0x008B
#define CBUS_LINK_0B_tx_ack_low_hb_shift                              (0)
#define CBUS_LINK_0B_tx_ack_low_hb_mask                               (0x0000007F)
#define CBUS_LINK_0B_tx_ack_low_hb(data)                              (0x0000007F&(data))
#define CBUS_LINK_0B_get_tx_ack_low_hb(data)                          (0x0000007F&(data))


#define CBUS_LINK_0C                                                  0x030
#define CBUS_LINK_0C_reg_addr                                         "0x030"
#define CBUS_LINK_0C_reg                                              0x030
#define CBUS_LINK_0C_inst_addr                                        "0x008C"
#define CBUS_LINK_0C_inst                                             0x008C
#define CBUS_LINK_0C_tx_ack_low_lb_shift                              (0)
#define CBUS_LINK_0C_tx_ack_low_lb_mask                               (0x0000007F)
#define CBUS_LINK_0C_tx_ack_low_lb(data)                              (0x0000007F&(data))
#define CBUS_LINK_0C_get_tx_ack_low_lb(data)                          (0x0000007F&(data))


#define CBUS_LINK_0D                                                  0x034
#define CBUS_LINK_0D_reg_addr                                         "0x034"
#define CBUS_LINK_0D_reg                                              0x034
#define CBUS_LINK_0D_inst_addr                                        "0x008D"
#define CBUS_LINK_0D_inst                                             0x008D
#define CBUS_LINK_0D_retry_lv_shift                                   (5)
#define CBUS_LINK_0D_retry_flag_shift                                 (4)
#define CBUS_LINK_0D_ctrl_1f_resv_shift                               (0)
#define CBUS_LINK_0D_retry_lv_mask                                    (0x000000E0)
#define CBUS_LINK_0D_retry_flag_mask                                  (0x00000010)
#define CBUS_LINK_0D_ctrl_1f_resv_mask                                (0x0000000F)
#define CBUS_LINK_0D_retry_lv(data)                                   (0x000000E0&((data)<<5))
#define CBUS_LINK_0D_retry_flag(data)                                 (0x00000010&((data)<<4))
#define CBUS_LINK_0D_ctrl_1f_resv(data)                               (0x0000000F&(data))
#define CBUS_LINK_0D_get_retry_lv(data)                               ((0x000000E0&(data))>>5)
#define CBUS_LINK_0D_get_retry_flag(data)                             ((0x00000010&(data))>>4)
#define CBUS_LINK_0D_get_ctrl_1f_resv(data)                           (0x0000000F&(data))


#define CBUS_LINK_0E                                                  0x038
#define CBUS_LINK_0E_reg_addr                                         "0x038"
#define CBUS_LINK_0E_reg                                              0x038
#define CBUS_LINK_0E_inst_addr                                        "0x008E"
#define CBUS_LINK_0E_inst                                             0x008E
#define CBUS_LINK_0E_drv_str_shift                                    (6)
#define CBUS_LINK_0E_drv_hi_cbus_shift                                (0)
#define CBUS_LINK_0E_drv_str_mask                                     (0x000000C0)
#define CBUS_LINK_0E_drv_hi_cbus_mask                                 (0x0000003F)
#define CBUS_LINK_0E_drv_str(data)                                    (0x000000C0&((data)<<6))
#define CBUS_LINK_0E_drv_hi_cbus(data)                                (0x0000003F&(data))
#define CBUS_LINK_0E_get_drv_str(data)                                ((0x000000C0&(data))>>6)
#define CBUS_LINK_0E_get_drv_hi_cbus(data)                            (0x0000003F&(data))


#define CBUS_LINK_0F                                                  0x03c
#define CBUS_LINK_0F_reg_addr                                         "0x03c"
#define CBUS_LINK_0F_reg                                              0x03c
#define CBUS_LINK_0F_inst_addr                                        "0x008F"
#define CBUS_LINK_0F_inst                                             0x008F
#define CBUS_LINK_0F_wait_shift                                       (2)
#define CBUS_LINK_0F_req_opp_int_shift                                (0)
#define CBUS_LINK_0F_wait_mask                                        (0x0000003C)
#define CBUS_LINK_0F_req_opp_int_mask                                 (0x00000003)
#define CBUS_LINK_0F_wait(data)                                       (0x0000003C&((data)<<2))
#define CBUS_LINK_0F_req_opp_int(data)                                (0x00000003&(data))
#define CBUS_LINK_0F_get_wait(data)                                   ((0x0000003C&(data))>>2)
#define CBUS_LINK_0F_get_req_opp_int(data)                            (0x00000003&(data))


#define CBUS_LINK_10                                                  0x040
#define CBUS_LINK_10_reg_addr                                         "0x040"
#define CBUS_LINK_10_reg                                              0x040
#define CBUS_LINK_10_inst_addr                                        "0x0090"
#define CBUS_LINK_10_inst                                             0x0090
#define CBUS_LINK_10_req_opp_flt_shift                                (0)
#define CBUS_LINK_10_req_opp_flt_mask                                 (0x000000FF)
#define CBUS_LINK_10_req_opp_flt(data)                                (0x000000FF&(data))
#define CBUS_LINK_10_get_req_opp_flt(data)                            (0x000000FF&(data))


#define CBUS_LINK_11                                                  0x044
#define CBUS_LINK_11_reg_addr                                         "0x044"
#define CBUS_LINK_11_reg                                              0x044
#define CBUS_LINK_11_inst_addr                                        "0x0091"
#define CBUS_LINK_11_inst                                             0x0091
#define CBUS_LINK_11_req_cont_shift                                   (0)
#define CBUS_LINK_11_req_cont_mask                                    (0x000000FF)
#define CBUS_LINK_11_req_cont(data)                                   (0x000000FF&(data))
#define CBUS_LINK_11_get_req_cont(data)                               (0x000000FF&(data))


#define CBUS_LINK_12                                                  0x048
#define CBUS_LINK_12_reg_addr                                         "0x048"
#define CBUS_LINK_12_reg                                              0x048
#define CBUS_LINK_12_inst_addr                                        "0x0092"
#define CBUS_LINK_12_inst                                             0x0092
#define CBUS_LINK_12_req_hold_shift                                   (4)
#define CBUS_LINK_12_resp_hold_shift                                  (0)
#define CBUS_LINK_12_req_hold_mask                                    (0x000000F0)
#define CBUS_LINK_12_resp_hold_mask                                   (0x0000000F)
#define CBUS_LINK_12_req_hold(data)                                   (0x000000F0&((data)<<4))
#define CBUS_LINK_12_resp_hold(data)                                  (0x0000000F&(data))
#define CBUS_LINK_12_get_req_hold(data)                               ((0x000000F0&(data))>>4)
#define CBUS_LINK_12_get_resp_hold(data)                              (0x0000000F&(data))


#define CBUS_LINK_13                                                  0x04c
#define CBUS_LINK_13_reg_addr                                         "0x04c"
#define CBUS_LINK_13_reg                                              0x04c
#define CBUS_LINK_13_inst_addr                                        "0x0093"
#define CBUS_LINK_13_inst                                             0x0093
#define CBUS_LINK_13_glob_time_shift                                  (6)
#define CBUS_LINK_13_link_time_shift                                  (1)
#define CBUS_LINK_13_link_err_shift                                   (0)
#define CBUS_LINK_13_glob_time_mask                                   (0x000000C0)
#define CBUS_LINK_13_link_time_mask                                   (0x0000003E)
#define CBUS_LINK_13_link_err_mask                                    (0x00000001)
#define CBUS_LINK_13_glob_time(data)                                  (0x000000C0&((data)<<6))
#define CBUS_LINK_13_link_time(data)                                  (0x0000003E&((data)<<1))
#define CBUS_LINK_13_link_err(data)                                   (0x00000001&(data))
#define CBUS_LINK_13_get_glob_time(data)                              ((0x000000C0&(data))>>6)
#define CBUS_LINK_13_get_link_time(data)                              ((0x0000003E&(data))>>1)
#define CBUS_LINK_13_get_link_err(data)                               (0x00000001&(data))


#define CBUS_LINK_14                                                  0x050
#define CBUS_LINK_14_reg_addr                                         "0x050"
#define CBUS_LINK_14_reg                                              0x050
#define CBUS_LINK_14_inst_addr                                        "0x0094"
#define CBUS_LINK_14_inst                                             0x0094
#define CBUS_LINK_14_chk_point_shift                                  (2)
#define CBUS_LINK_14_chk_err_shift                                    (1)
#define CBUS_LINK_14_avoid_conf_shift                                 (0)
#define CBUS_LINK_14_chk_point_mask                                   (0x000000FC)
#define CBUS_LINK_14_chk_err_mask                                     (0x00000002)
#define CBUS_LINK_14_avoid_conf_mask                                  (0x00000001)
#define CBUS_LINK_14_chk_point(data)                                  (0x000000FC&((data)<<2))
#define CBUS_LINK_14_chk_err(data)                                    (0x00000002&((data)<<1))
#define CBUS_LINK_14_avoid_conf(data)                                 (0x00000001&(data))
#define CBUS_LINK_14_get_chk_point(data)                              ((0x000000FC&(data))>>2)
#define CBUS_LINK_14_get_chk_err(data)                                ((0x00000002&(data))>>1)
#define CBUS_LINK_14_get_avoid_conf(data)                             (0x00000001&(data))

#endif //_HDMI_RX_MHL_CBUS_LINK_H
