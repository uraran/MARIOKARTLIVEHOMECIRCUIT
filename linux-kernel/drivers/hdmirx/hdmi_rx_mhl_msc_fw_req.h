#ifndef _HDMI_RX_MHL_MSC_FW_REQ_H
#define _HDMI_RX_MHL_MSC_FW_REQ_H
// ===============================================
// HDMI MSC FW REQ(HDMI_RX_MHL_CBUS_MSC_FW)
// HDMI Register Address base 0x1800f300

#define FW0_REQ_00                                                    0x000
#define FW0_REQ_00_reg_addr                                           "0x000"
#define FW0_REQ_00_reg                                                0x000
#define FW0_REQ_00_inst_addr                                          "0x00C0"
#define FW0_REQ_00_inst                                               0x00C0
#define FW0_REQ_00_fw0_req_shift                                      (8)
#define FW0_REQ_00_fw0_fifo_clr_shift                                 (7)
#define FW0_REQ_00_fw0_fifo_full_shift                                (6)
#define FW0_REQ_00_fw0_fifo_empty_shift                               (5)
#define FW0_REQ_00_fw0_tx_case_shift                                  (2)
#define FW0_REQ_00_fw0_head_shift                                     (0)
#define FW0_REQ_00_fw0_req_mask                                       (0x00000100)
#define FW0_REQ_00_fw0_fifo_clr_mask                                  (0x00000080)
#define FW0_REQ_00_fw0_fifo_full_mask                                 (0x00000040)
#define FW0_REQ_00_fw0_fifo_empty_mask                                (0x00000020)
#define FW0_REQ_00_fw0_tx_case_mask                                   (0x0000001C)
#define FW0_REQ_00_fw0_head_mask                                      (0x00000003)
#define FW0_REQ_00_fw0_req(data)                                      (0x00000100&((data)<<8))
#define FW0_REQ_00_fw0_fifo_clr(data)                                 (0x00000080&((data)<<7))
#define FW0_REQ_00_fw0_fifo_full(data)                                (0x00000040&((data)<<6))
#define FW0_REQ_00_fw0_fifo_empty(data)                               (0x00000020&((data)<<5))
#define FW0_REQ_00_fw0_tx_case(data)                                  (0x0000001C&((data)<<2))
#define FW0_REQ_00_fw0_head(data)                                     (0x00000003&(data))
#define FW0_REQ_00_get_fw0_req(data)                                  ((0x00000100&(data))>>8)
#define FW0_REQ_00_get_fw0_fifo_clr(data)                             ((0x00000080&(data))>>7)
#define FW0_REQ_00_get_fw0_fifo_full(data)                            ((0x00000040&(data))>>6)
#define FW0_REQ_00_get_fw0_fifo_empty(data)                           ((0x00000020&(data))>>5)
#define FW0_REQ_00_get_fw0_tx_case(data)                              ((0x0000001C&(data))>>2)
#define FW0_REQ_00_get_fw0_head(data)                                 (0x00000003&(data))


#define FW0_REQ_01                                                    0x004
#define FW0_REQ_01_reg_addr                                           "0x004"
#define FW0_REQ_01_reg                                                0x004
#define FW0_REQ_01_inst_addr                                          "0x00C1"
#define FW0_REQ_01_inst                                               0x00C1
#define FW0_REQ_01_fw0_cmd1_shift                                     (0)
#define FW0_REQ_01_fw0_cmd1_mask                                      (0x000000FF)
#define FW0_REQ_01_fw0_cmd1(data)                                     (0x000000FF&(data))
#define FW0_REQ_01_get_fw0_cmd1(data)                                 (0x000000FF&(data))


#define FW0_REQ_02                                                    0x008
#define FW0_REQ_02_reg_addr                                           "0x008"
#define FW0_REQ_02_reg                                                0x008
#define FW0_REQ_02_inst_addr                                          "0x00C2"
#define FW0_REQ_02_inst                                               0x00C2
#define FW0_REQ_02_fw0_cmd2_shift                                     (0)
#define FW0_REQ_02_fw0_cmd2_mask                                      (0x000000FF)
#define FW0_REQ_02_fw0_cmd2(data)                                     (0x000000FF&(data))
#define FW0_REQ_02_get_fw0_cmd2(data)                                 (0x000000FF&(data))


#define FW0_REQ_03                                                    0x00c
#define FW0_REQ_03_reg_addr                                           "0x00c"
#define FW0_REQ_03_reg                                                0x00c
#define FW0_REQ_03_inst_addr                                          "0x00C3"
#define FW0_REQ_03_inst                                               0x00C3
#define FW0_REQ_03_fw0_offset_shift                                   (0)
#define FW0_REQ_03_fw0_offset_mask                                    (0x000000FF)
#define FW0_REQ_03_fw0_offset(data)                                   (0x000000FF&(data))
#define FW0_REQ_03_get_fw0_offset(data)                               (0x000000FF&(data))


#define FW0_REQ_04                                                    0x010
#define FW0_REQ_04_reg_addr                                           "0x010"
#define FW0_REQ_04_reg                                                0x010
#define FW0_REQ_04_inst_addr                                          "0x00C4"
#define FW0_REQ_04_inst                                               0x00C4
#define FW0_REQ_04_fw0_data_shift                                     (0)
#define FW0_REQ_04_fw0_data_mask                                      (0x000000FF)
#define FW0_REQ_04_fw0_data(data)                                     (0x000000FF&(data))
#define FW0_REQ_04_get_fw0_data(data)                                 (0x000000FF&(data))


#define FW0_REQ_05                                                    0x014
#define FW0_REQ_05_reg_addr                                           "0x014"
#define FW0_REQ_05_reg                                                0x014
#define FW0_REQ_05_inst_addr                                          "0x00C5"
#define FW0_REQ_05_inst                                               0x00C5
#define FW0_REQ_05_fw0_rty_ovr_shift                                  (9)
#define FW0_REQ_05_fw0_fin_shift                                      (8)
#define FW0_REQ_05_fw0_cmd_event_shift                                (7)
#define FW0_REQ_05_fw0_data_event_shift                               (6)
#define FW0_REQ_05_fw0_rcv_err_shift                                  (5)
#define FW0_REQ_05_fw0_fin_irq_en_shift                               (4)
#define FW0_REQ_05_fw0_cmd_irq_en_shift                               (3)
#define FW0_REQ_05_fw0_data_irq_en_shift                              (2)
#define FW0_REQ_05_fw0_wait_case_shift                                (0)
#define FW0_REQ_05_fw0_rty_ovr_mask                                   (0x00000200)
#define FW0_REQ_05_fw0_fin_mask                                       (0x00000100)
#define FW0_REQ_05_fw0_cmd_event_mask                                 (0x00000080)
#define FW0_REQ_05_fw0_data_event_mask                                (0x00000040)
#define FW0_REQ_05_fw0_rcv_err_mask                                   (0x00000020)
#define FW0_REQ_05_fw0_fin_irq_en_mask                                (0x00000010)
#define FW0_REQ_05_fw0_cmd_irq_en_mask                                (0x00000008)
#define FW0_REQ_05_fw0_data_irq_en_mask                               (0x00000004)
#define FW0_REQ_05_fw0_wait_case_mask                                 (0x00000003)
#define FW0_REQ_05_fw0_rty_ovr(data)                                  (0x00000200&((data)<<9))
#define FW0_REQ_05_fw0_fin(data)                                      (0x00000100&((data)<<8))
#define FW0_REQ_05_fw0_cmd_event(data)                                (0x00000080&((data)<<7))
#define FW0_REQ_05_fw0_data_event(data)                               (0x00000040&((data)<<6))
#define FW0_REQ_05_fw0_rcv_err(data)                                  (0x00000020&((data)<<5))
#define FW0_REQ_05_fw0_fin_irq_en(data)                               (0x00000010&((data)<<4))
#define FW0_REQ_05_fw0_cmd_irq_en(data)                               (0x00000008&((data)<<3))
#define FW0_REQ_05_fw0_data_irq_en(data)                              (0x00000004&((data)<<2))
#define FW0_REQ_05_fw0_wait_case(data)                                (0x00000003&(data))
#define FW0_REQ_05_get_fw0_rty_ovr(data)                              ((0x00000200&(data))>>9)
#define FW0_REQ_05_get_fw0_fin(data)                                  ((0x00000100&(data))>>8)
#define FW0_REQ_05_get_fw0_cmd_event(data)                            ((0x00000080&(data))>>7)
#define FW0_REQ_05_get_fw0_data_event(data)                           ((0x00000040&(data))>>6)
#define FW0_REQ_05_get_fw0_rcv_err(data)                              ((0x00000020&(data))>>5)
#define FW0_REQ_05_get_fw0_fin_irq_en(data)                           ((0x00000010&(data))>>4)
#define FW0_REQ_05_get_fw0_cmd_irq_en(data)                           ((0x00000008&(data))>>3)
#define FW0_REQ_05_get_fw0_data_irq_en(data)                          ((0x00000004&(data))>>2)
#define FW0_REQ_05_get_fw0_wait_case(data)                            (0x00000003&(data))


#define FW0_REQ_06                                                    0x018
#define FW0_REQ_06_reg_addr                                           "0x018"
#define FW0_REQ_06_reg                                                0x018
#define FW0_REQ_06_inst_addr                                          "0x00C6"
#define FW0_REQ_06_inst                                               0x00C6
#define FW0_REQ_06_fw0_cmd_rcv_shift                                  (0)
#define FW0_REQ_06_fw0_cmd_rcv_mask                                   (0x000000FF)
#define FW0_REQ_06_fw0_cmd_rcv(data)                                  (0x000000FF&(data))
#define FW0_REQ_06_get_fw0_cmd_rcv(data)                              (0x000000FF&(data))


#define FW0_REQ_07                                                    0x01c
#define FW0_REQ_07_reg_addr                                           "0x01c"
#define FW0_REQ_07_reg                                                0x01c
#define FW0_REQ_07_inst_addr                                          "0x00C7"
#define FW0_REQ_07_inst                                               0x00C7
#define FW0_REQ_07_fw0_data_rcv_shift                                 (0)
#define FW0_REQ_07_fw0_data_rcv_mask                                  (0x000000FF)
#define FW0_REQ_07_fw0_data_rcv(data)                                 (0x000000FF&(data))
#define FW0_REQ_07_get_fw0_data_rcv(data)                             (0x000000FF&(data))


#define FW0_REQ_08                                                    0x020
#define FW0_REQ_08_reg_addr                                           "0x020"
#define FW0_REQ_08_reg                                                0x020
#define FW0_REQ_08_inst_addr                                          "0x00C8"
#define FW0_REQ_08_inst                                               0x00C8
#define FW0_REQ_08_fw0_rd_en_shift                                    (13)
#define FW0_REQ_08_fw0_fifo_len_shift                                 (8)
#define FW0_REQ_08_fw0_rdata_shift                                    (0)
#define FW0_REQ_08_fw0_rd_en_mask                                     (0x00002000)
#define FW0_REQ_08_fw0_fifo_len_mask                                  (0x00001F00)
#define FW0_REQ_08_fw0_rdata_mask                                     (0x000000FF)
#define FW0_REQ_08_fw0_rd_en(data)                                    (0x00002000&((data)<<13))
#define FW0_REQ_08_fw0_fifo_len(data)                                 (0x00001F00&((data)<<8))
#define FW0_REQ_08_fw0_rdata(data)                                    (0x000000FF&(data))
#define FW0_REQ_08_get_fw0_rd_en(data)                                ((0x00002000&(data))>>13)
#define FW0_REQ_08_get_fw0_fifo_len(data)                             ((0x00001F00&(data))>>8)
#define FW0_REQ_08_get_fw0_rdata(data)                                (0x000000FF&(data))


#define FW1_REQ_00                                                    0x040
#define FW1_REQ_00_reg_addr                                           "0x040"
#define FW1_REQ_00_reg                                                0x040
#define FW1_REQ_00_inst_addr                                          "0x00D0"
#define FW1_REQ_00_inst                                               0x00D0
#define FW1_REQ_00_fw1_req_shift                                      (8)
#define FW1_REQ_00_fw1_fifo_clr_shift                                 (7)
#define FW1_REQ_00_fw1_fifo_full_shift                                (6)
#define FW1_REQ_00_fw1_fifo_empty_shift                               (5)
#define FW1_REQ_00_fw1_tx_case_shift                                  (2)
#define FW1_REQ_00_fw1_head_shift                                     (0)
#define FW1_REQ_00_fw1_req_mask                                       (0x00000100)
#define FW1_REQ_00_fw1_fifo_clr_mask                                  (0x00000080)
#define FW1_REQ_00_fw1_fifo_full_mask                                 (0x00000040)
#define FW1_REQ_00_fw1_fifo_empty_mask                                (0x00000020)
#define FW1_REQ_00_fw1_tx_case_mask                                   (0x0000001C)
#define FW1_REQ_00_fw1_head_mask                                      (0x00000003)
#define FW1_REQ_00_fw1_req(data)                                      (0x00000100&((data)<<8))
#define FW1_REQ_00_fw1_fifo_clr(data)                                 (0x00000080&((data)<<7))
#define FW1_REQ_00_fw1_fifo_full(data)                                (0x00000040&((data)<<6))
#define FW1_REQ_00_fw1_fifo_empty(data)                               (0x00000020&((data)<<5))
#define FW1_REQ_00_fw1_tx_case(data)                                  (0x0000001C&((data)<<2))
#define FW1_REQ_00_fw1_head(data)                                     (0x00000003&(data))
#define FW1_REQ_00_get_fw1_req(data)                                  ((0x00000100&(data))>>8)
#define FW1_REQ_00_get_fw1_fifo_clr(data)                             ((0x00000080&(data))>>7)
#define FW1_REQ_00_get_fw1_fifo_full(data)                            ((0x00000040&(data))>>6)
#define FW1_REQ_00_get_fw1_fifo_empty(data)                           ((0x00000020&(data))>>5)
#define FW1_REQ_00_get_fw1_tx_case(data)                              ((0x0000001C&(data))>>2)
#define FW1_REQ_00_get_fw1_head(data)                                 (0x00000003&(data))


#define FW1_REQ_01                                                    0x044
#define FW1_REQ_01_reg_addr                                           "0x044"
#define FW1_REQ_01_reg                                                0x044
#define FW1_REQ_01_inst_addr                                          "0x00D1"
#define FW1_REQ_01_inst                                               0x00D1
#define FW1_REQ_01_fw1_cmd1_shift                                     (0)
#define FW1_REQ_01_fw1_cmd1_mask                                      (0x000000FF)
#define FW1_REQ_01_fw1_cmd1(data)                                     (0x000000FF&(data))
#define FW1_REQ_01_get_fw1_cmd1(data)                                 (0x000000FF&(data))


#define FW1_REQ_02                                                    0x048
#define FW1_REQ_02_reg_addr                                           "0x048"
#define FW1_REQ_02_reg                                                0x048
#define FW1_REQ_02_inst_addr                                          "0x00D2"
#define FW1_REQ_02_inst                                               0x00D2
#define FW1_REQ_02_fw1_cmd2_shift                                     (0)
#define FW1_REQ_02_fw1_cmd2_mask                                      (0x000000FF)
#define FW1_REQ_02_fw1_cmd2(data)                                     (0x000000FF&(data))
#define FW1_REQ_02_get_fw1_cmd2(data)                                 (0x000000FF&(data))


#define FW1_REQ_03                                                    0x04c
#define FW1_REQ_03_reg_addr                                           "0x04c"
#define FW1_REQ_03_reg                                                0x04c
#define FW1_REQ_03_inst_addr                                          "0x00D3"
#define FW1_REQ_03_inst                                               0x00D3
#define FW1_REQ_03_fw1_offset_shift                                   (0)
#define FW1_REQ_03_fw1_offset_mask                                    (0x000000FF)
#define FW1_REQ_03_fw1_offset(data)                                   (0x000000FF&(data))
#define FW1_REQ_03_get_fw1_offset(data)                               (0x000000FF&(data))


#define FW1_REQ_04                                                    0x050
#define FW1_REQ_04_reg_addr                                           "0x050"
#define FW1_REQ_04_reg                                                0x050
#define FW1_REQ_04_inst_addr                                          "0x00D4"
#define FW1_REQ_04_inst                                               0x00D4
#define FW1_REQ_04_fw1_data_shift                                     (0)
#define FW1_REQ_04_fw1_data_mask                                      (0x000000FF)
#define FW1_REQ_04_fw1_data(data)                                     (0x000000FF&(data))
#define FW1_REQ_04_get_fw1_data(data)                                 (0x000000FF&(data))


#define FW1_REQ_5                                                     0x054
#define FW1_REQ_5_reg_addr                                            "0x054"
#define FW1_REQ_5_reg                                                 0x054
#define FW1_REQ_5_inst_addr                                           "0x00D5"
#define FW1_REQ_5_inst                                                0x00D5
#define FW1_REQ_5_fw1_rty_ovr_shift                                   (9)
#define FW1_REQ_5_fw1_fin_shift                                       (8)
#define FW1_REQ_5_fw1_cmd_event_shift                                 (7)
#define FW1_REQ_5_fw1_data_event_shift                                (6)
#define FW1_REQ_5_fw1_rcv_err_shift                                   (5)
#define FW1_REQ_5_fw1_fin_irq_en_shift                                (4)
#define FW1_REQ_5_fw1_cmd_irq_en_shift                                (3)
#define FW1_REQ_5_fw1_data_irq_en_shift                               (2)
#define FW1_REQ_5_fw1_wait_case_shift                                 (0)
#define FW1_REQ_5_fw1_rty_ovr_mask                                    (0x00000200)
#define FW1_REQ_5_fw1_fin_mask                                        (0x00000100)
#define FW1_REQ_5_fw1_cmd_event_mask                                  (0x00000080)
#define FW1_REQ_5_fw1_data_event_mask                                 (0x00000040)
#define FW1_REQ_5_fw1_rcv_err_mask                                    (0x00000020)
#define FW1_REQ_5_fw1_fin_irq_en_mask                                 (0x00000010)
#define FW1_REQ_5_fw1_cmd_irq_en_mask                                 (0x00000008)
#define FW1_REQ_5_fw1_data_irq_en_mask                                (0x00000004)
#define FW1_REQ_5_fw1_wait_case_mask                                  (0x00000003)
#define FW1_REQ_5_fw1_rty_ovr(data)                                   (0x00000200&((data)<<9))
#define FW1_REQ_5_fw1_fin(data)                                       (0x00000100&((data)<<8))
#define FW1_REQ_5_fw1_cmd_event(data)                                 (0x00000080&((data)<<7))
#define FW1_REQ_5_fw1_data_event(data)                                (0x00000040&((data)<<6))
#define FW1_REQ_5_fw1_rcv_err(data)                                   (0x00000020&((data)<<5))
#define FW1_REQ_5_fw1_fin_irq_en(data)                                (0x00000010&((data)<<4))
#define FW1_REQ_5_fw1_cmd_irq_en(data)                                (0x00000008&((data)<<3))
#define FW1_REQ_5_fw1_data_irq_en(data)                               (0x00000004&((data)<<2))
#define FW1_REQ_5_fw1_wait_case(data)                                 (0x00000003&(data))
#define FW1_REQ_5_get_fw1_rty_ovr(data)                               ((0x00000200&(data))>>9)
#define FW1_REQ_5_get_fw1_fin(data)                                   ((0x00000100&(data))>>8)
#define FW1_REQ_5_get_fw1_cmd_event(data)                             ((0x00000080&(data))>>7)
#define FW1_REQ_5_get_fw1_data_event(data)                            ((0x00000040&(data))>>6)
#define FW1_REQ_5_get_fw1_rcv_err(data)                               ((0x00000020&(data))>>5)
#define FW1_REQ_5_get_fw1_fin_irq_en(data)                            ((0x00000010&(data))>>4)
#define FW1_REQ_5_get_fw1_cmd_irq_en(data)                            ((0x00000008&(data))>>3)
#define FW1_REQ_5_get_fw1_data_irq_en(data)                           ((0x00000004&(data))>>2)
#define FW1_REQ_5_get_fw1_wait_case(data)                             (0x00000003&(data))


#define FW1_REQ_6                                                     0x058
#define FW1_REQ_6_reg_addr                                            "0x058"
#define FW1_REQ_6_reg                                                 0x058
#define FW1_REQ_6_inst_addr                                           "0x00D6"
#define FW1_REQ_6_inst                                                0x00D6
#define FW1_REQ_6_fw1_cmd_rcv_shift                                   (0)
#define FW1_REQ_6_fw1_cmd_rcv_mask                                    (0x000000FF)
#define FW1_REQ_6_fw1_cmd_rcv(data)                                   (0x000000FF&(data))
#define FW1_REQ_6_get_fw1_cmd_rcv(data)                               (0x000000FF&(data))


#define FW1_REQ_7                                                     0x05c
#define FW1_REQ_7_reg_addr                                            "0x05c"
#define FW1_REQ_7_reg                                                 0x05c
#define FW1_REQ_7_inst_addr                                           "0x00D7"
#define FW1_REQ_7_inst                                                0x00D7
#define FW1_REQ_7_fw1_data_rcv_shift                                  (0)
#define FW1_REQ_7_fw1_data_rcv_mask                                   (0x000000FF)
#define FW1_REQ_7_fw1_data_rcv(data)                                  (0x000000FF&(data))
#define FW1_REQ_7_get_fw1_data_rcv(data)                              (0x000000FF&(data))


#define FW1_REQ_08                                                    0x060
#define FW1_REQ_08_reg_addr                                           "0x060"
#define FW1_REQ_08_reg                                                0x060
#define FW1_REQ_08_inst_addr                                          "0x00D8"
#define FW1_REQ_08_inst                                               0x00D8
#define FW1_REQ_08_fw1_rd_en_shift                                    (13)
#define FW1_REQ_08_fw1_fifo_len_shift                                 (8)
#define FW1_REQ_08_fw1_rdata_shift                                    (0)
#define FW1_REQ_08_fw1_rd_en_mask                                     (0x00002000)
#define FW1_REQ_08_fw1_fifo_len_mask                                  (0x00001F00)
#define FW1_REQ_08_fw1_rdata_mask                                     (0x000000FF)
#define FW1_REQ_08_fw1_rd_en(data)                                    (0x00002000&((data)<<13))
#define FW1_REQ_08_fw1_fifo_len(data)                                 (0x00001F00&((data)<<8))
#define FW1_REQ_08_fw1_rdata(data)                                    (0x000000FF&(data))
#define FW1_REQ_08_get_fw1_rd_en(data)                                ((0x00002000&(data))>>13)
#define FW1_REQ_08_get_fw1_fifo_len(data)                             ((0x00001F00&(data))>>8)
#define FW1_REQ_08_get_fw1_rdata(data)                                (0x000000FF&(data))


#define FW2_REQ_00                                                    0x080
#define FW2_REQ_00_reg_addr                                           "0x080"
#define FW2_REQ_00_reg                                                0x080
#define FW2_REQ_00_inst_addr                                          "0x00E0"
#define FW2_REQ_00_inst                                               0x00E0
#define FW2_REQ_00_fw2_req_shift                                      (8)
#define FW2_REQ_00_fw2_fifo_clr_shift                                 (7)
#define FW2_REQ_00_fw2_fifo_full_shift                                (6)
#define FW2_REQ_00_fw2_fifo_empty_shift                               (5)
#define FW2_REQ_00_fw2_tx_case_shift                                  (2)
#define FW2_REQ_00_fw2_head_shift                                     (0)
#define FW2_REQ_00_fw2_req_mask                                       (0x00000100)
#define FW2_REQ_00_fw2_fifo_clr_mask                                  (0x00000080)
#define FW2_REQ_00_fw2_fifo_full_mask                                 (0x00000040)
#define FW2_REQ_00_fw2_fifo_empty_mask                                (0x00000020)
#define FW2_REQ_00_fw2_tx_case_mask                                   (0x0000001C)
#define FW2_REQ_00_fw2_head_mask                                      (0x00000003)
#define FW2_REQ_00_fw2_req(data)                                      (0x00000100&((data)<<8))
#define FW2_REQ_00_fw2_fifo_clr(data)                                 (0x00000080&((data)<<7))
#define FW2_REQ_00_fw2_fifo_full(data)                                (0x00000040&((data)<<6))
#define FW2_REQ_00_fw2_fifo_empty(data)                               (0x00000020&((data)<<5))
#define FW2_REQ_00_fw2_tx_case(data)                                  (0x0000001C&((data)<<2))
#define FW2_REQ_00_fw2_head(data)                                     (0x00000003&(data))
#define FW2_REQ_00_get_fw2_req(data)                                  ((0x00000100&(data))>>8)
#define FW2_REQ_00_get_fw2_fifo_clr(data)                             ((0x00000080&(data))>>7)
#define FW2_REQ_00_get_fw2_fifo_full(data)                            ((0x00000040&(data))>>6)
#define FW2_REQ_00_get_fw2_fifo_empty(data)                           ((0x00000020&(data))>>5)
#define FW2_REQ_00_get_fw2_tx_case(data)                              ((0x0000001C&(data))>>2)
#define FW2_REQ_00_get_fw2_head(data)                                 (0x00000003&(data))


#define FW2_REQ_01                                                    0x084
#define FW2_REQ_01_reg_addr                                           "0x084"
#define FW2_REQ_01_reg                                                0x084
#define FW2_REQ_01_inst_addr                                          "0x00E1"
#define FW2_REQ_01_inst                                               0x00E1
#define FW2_REQ_01_fw2_cmd1_shift                                     (0)
#define FW2_REQ_01_fw2_cmd1_mask                                      (0x000000FF)
#define FW2_REQ_01_fw2_cmd1(data)                                     (0x000000FF&(data))
#define FW2_REQ_01_get_fw2_cmd1(data)                                 (0x000000FF&(data))


#define FW2_REQ_02                                                    0x088
#define FW2_REQ_02_reg_addr                                           "0x088"
#define FW2_REQ_02_reg                                                0x088
#define FW2_REQ_02_inst_addr                                          "0x00E2"
#define FW2_REQ_02_inst                                               0x00E2
#define FW2_REQ_02_fw2_cmd2_shift                                     (0)
#define FW2_REQ_02_fw2_cmd2_mask                                      (0x000000FF)
#define FW2_REQ_02_fw2_cmd2(data)                                     (0x000000FF&(data))
#define FW2_REQ_02_get_fw2_cmd2(data)                                 (0x000000FF&(data))


#define FW2_REQ_03                                                    0x08c
#define FW2_REQ_03_reg_addr                                           "0x08c"
#define FW2_REQ_03_reg                                                0x08c
#define FW2_REQ_03_inst_addr                                          "0x00E3"
#define FW2_REQ_03_inst                                               0x00E3
#define FW2_REQ_03_fw2_offset_shift                                   (0)
#define FW2_REQ_03_fw2_offset_mask                                    (0x000000FF)
#define FW2_REQ_03_fw2_offset(data)                                   (0x000000FF&(data))
#define FW2_REQ_03_get_fw2_offset(data)                               (0x000000FF&(data))


#define FW2_REQ_04                                                    0x090
#define FW2_REQ_04_reg_addr                                           "0x090"
#define FW2_REQ_04_reg                                                0x090
#define FW2_REQ_04_inst_addr                                          "0x00E4"
#define FW2_REQ_04_inst                                               0x00E4
#define FW2_REQ_04_fw2_data_shift                                     (0)
#define FW2_REQ_04_fw2_data_mask                                      (0x000000FF)
#define FW2_REQ_04_fw2_data(data)                                     (0x000000FF&(data))
#define FW2_REQ_04_get_fw2_data(data)                                 (0x000000FF&(data))


#define FW2_REQ_05                                                    0x094
#define FW2_REQ_05_reg_addr                                           "0x094"
#define FW2_REQ_05_reg                                                0x094
#define FW2_REQ_05_inst_addr                                          "0x00E5"
#define FW2_REQ_05_inst                                               0x00E5
#define FW2_REQ_05_fw2_rty_ovr_shift                                  (9)
#define FW2_REQ_05_fw2_fin_shift                                      (8)
#define FW2_REQ_05_fw2_cmd_event_shift                                (7)
#define FW2_REQ_05_fw2_data_event_shift                               (6)
#define FW2_REQ_05_fw2_rcv_err_shift                                  (5)
#define FW2_REQ_05_fw2_fin_irq_en_shift                               (4)
#define FW2_REQ_05_fw2_cmd_irq_en_shift                               (3)
#define FW2_REQ_05_fw2_data_irq_en_shift                              (2)
#define FW2_REQ_05_fw2_wait_case_shift                                (0)
#define FW2_REQ_05_fw2_rty_ovr_mask                                   (0x00000200)
#define FW2_REQ_05_fw2_fin_mask                                       (0x00000100)
#define FW2_REQ_05_fw2_cmd_event_mask                                 (0x00000080)
#define FW2_REQ_05_fw2_data_event_mask                                (0x00000040)
#define FW2_REQ_05_fw2_rcv_err_mask                                   (0x00000020)
#define FW2_REQ_05_fw2_fin_irq_en_mask                                (0x00000010)
#define FW2_REQ_05_fw2_cmd_irq_en_mask                                (0x00000008)
#define FW2_REQ_05_fw2_data_irq_en_mask                               (0x00000004)
#define FW2_REQ_05_fw2_wait_case_mask                                 (0x00000003)
#define FW2_REQ_05_fw2_rty_ovr(data)                                  (0x00000200&((data)<<9))
#define FW2_REQ_05_fw2_fin(data)                                      (0x00000100&((data)<<8))
#define FW2_REQ_05_fw2_cmd_event(data)                                (0x00000080&((data)<<7))
#define FW2_REQ_05_fw2_data_event(data)                               (0x00000040&((data)<<6))
#define FW2_REQ_05_fw2_rcv_err(data)                                  (0x00000020&((data)<<5))
#define FW2_REQ_05_fw2_fin_irq_en(data)                               (0x00000010&((data)<<4))
#define FW2_REQ_05_fw2_cmd_irq_en(data)                               (0x00000008&((data)<<3))
#define FW2_REQ_05_fw2_data_irq_en(data)                              (0x00000004&((data)<<2))
#define FW2_REQ_05_fw2_wait_case(data)                                (0x00000003&(data))
#define FW2_REQ_05_get_fw2_rty_ovr(data)                              ((0x00000200&(data))>>9)
#define FW2_REQ_05_get_fw2_fin(data)                                  ((0x00000100&(data))>>8)
#define FW2_REQ_05_get_fw2_cmd_event(data)                            ((0x00000080&(data))>>7)
#define FW2_REQ_05_get_fw2_data_event(data)                           ((0x00000040&(data))>>6)
#define FW2_REQ_05_get_fw2_rcv_err(data)                              ((0x00000020&(data))>>5)
#define FW2_REQ_05_get_fw2_fin_irq_en(data)                           ((0x00000010&(data))>>4)
#define FW2_REQ_05_get_fw2_cmd_irq_en(data)                           ((0x00000008&(data))>>3)
#define FW2_REQ_05_get_fw2_data_irq_en(data)                          ((0x00000004&(data))>>2)
#define FW2_REQ_05_get_fw2_wait_case(data)                            (0x00000003&(data))


#define FW2_REQ_06                                                    0x098
#define FW2_REQ_06_reg_addr                                           "0x098"
#define FW2_REQ_06_reg                                                0x098
#define FW2_REQ_06_inst_addr                                          "0x00E6"
#define FW2_REQ_06_inst                                               0x00E6
#define FW2_REQ_06_fw2_cmd_rcv_shift                                  (0)
#define FW2_REQ_06_fw2_cmd_rcv_mask                                   (0x000000FF)
#define FW2_REQ_06_fw2_cmd_rcv(data)                                  (0x000000FF&(data))
#define FW2_REQ_06_get_fw2_cmd_rcv(data)                              (0x000000FF&(data))


#define FW2_REQ_07                                                    0x09c
#define FW2_REQ_07_reg_addr                                           "0x09c"
#define FW2_REQ_07_reg                                                0x09c
#define FW2_REQ_07_inst_addr                                          "0x00E7"
#define FW2_REQ_07_inst                                               0x00E7
#define FW2_REQ_07_fw2_data_rcv_shift                                 (0)
#define FW2_REQ_07_fw2_data_rcv_mask                                  (0x000000FF)
#define FW2_REQ_07_fw2_data_rcv(data)                                 (0x000000FF&(data))
#define FW2_REQ_07_get_fw2_data_rcv(data)                             (0x000000FF&(data))


#define FW2_REQ_08                                                    0x0a0
#define FW2_REQ_08_reg_addr                                           "0x0a0"
#define FW2_REQ_08_reg                                                0x0a0
#define FW2_REQ_08_inst_addr                                          "0x00E8"
#define FW2_REQ_08_inst                                               0x00E8
#define FW2_REQ_08_fw2_rd_en_shift                                    (13)
#define FW2_REQ_08_fw2_fifo_len_shift                                 (8)
#define FW2_REQ_08_fw2_rdata_shift                                    (0)
#define FW2_REQ_08_fw2_rd_en_mask                                     (0x00002000)
#define FW2_REQ_08_fw2_fifo_len_mask                                  (0x00001F00)
#define FW2_REQ_08_fw2_rdata_mask                                     (0x000000FF)
#define FW2_REQ_08_fw2_rd_en(data)                                    (0x00002000&((data)<<13))
#define FW2_REQ_08_fw2_fifo_len(data)                                 (0x00001F00&((data)<<8))
#define FW2_REQ_08_fw2_rdata(data)                                    (0x000000FF&(data))
#define FW2_REQ_08_get_fw2_rd_en(data)                                ((0x00002000&(data))>>13)
#define FW2_REQ_08_get_fw2_fifo_len(data)                             ((0x00001F00&(data))>>8)
#define FW2_REQ_08_get_fw2_rdata(data)                                (0x000000FF&(data))

#endif //_HDMI_RX_MHL_MSC_FW_REQ_H
