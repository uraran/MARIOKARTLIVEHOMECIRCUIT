#ifndef PHOENIX_HDMI_HW_CLOCK_DETECT_H
#define PHOENIX_HDMI_HW_CLOCK_DETECT_H

// HDMI clock detection
// HDMI Register Address base 0x18037300

// clock detection
#define HDMI_CLKDETCR                                                      0x0000
#define HDMI_CLKDETSR                                                      0x0004
#define HDMI_GDI_TMDSCLK_SETTING_00                                        0x0008
#define HDMI_GDI_TMDSCLK_SETTING_01                                        0x000c
#define HDMI_GDI_TMDSCLK_SETTING_02                                        0x0010

#define HDMI_CLKDETSR_en_tmds_chg_irq_shift                                (5)
#define HDMI_CLKDETSR_tmds_chg_irq_shift                                   (4)
#define HDMI_CLKDETSR_dummy18061f04_3_2_shift                              (2)
#define HDMI_CLKDETSR_clk_in_target_irq_en_shift                           (1)
#define HDMI_CLKDETSR_clk_in_target_shift                                  (0)
#define HDMI_CLKDETSR_en_tmds_chg_irq_mask                                 (0x00000020)
#define HDMI_CLKDETSR_tmds_chg_irq_mask                                    (0x00000010)
#define HDMI_CLKDETSR_dummy18061f04_3_2_mask                               (0x0000000C)
#define HDMI_CLKDETSR_clk_in_target_irq_en_mask                            (0x00000002)
#define HDMI_CLKDETSR_clk_in_target_mask                                   (0x00000001)
#define HDMI_CLKDETSR_en_tmds_chg_irq(data)                                (0x00000020&((data)<<5))
#define HDMI_CLKDETSR_tmds_chg_irq(data)                                   (0x00000010&((data)<<4))
#define HDMI_CLKDETSR_dummy18061f04_3_2(data)                              (0x0000000C&((data)<<2))
#define HDMI_CLKDETSR_clk_in_target_irq_en(data)                           (0x00000002&((data)<<1))
#define HDMI_CLKDETSR_clk_in_target(data)                                  (0x00000001&(data))
#define HDMI_CLKDETSR_get_en_tmds_chg_irq(data)                            ((0x00000020&(data))>>5)
#define HDMI_CLKDETSR_get_tmds_chg_irq(data)                               ((0x00000010&(data))>>4)
#define HDMI_CLKDETSR_get_dummy18061f04_3_2(data)                          ((0x0000000C&(data))>>2)
#define HDMI_CLKDETSR_get_clk_in_target_irq_en(data)                       ((0x00000002&(data))>>1)
#define HDMI_CLKDETSR_get_clk_in_target(data)                              (0x00000001&(data))

#endif //PHOENIX_HDMI_HW_CLOCK_DETECT_H
