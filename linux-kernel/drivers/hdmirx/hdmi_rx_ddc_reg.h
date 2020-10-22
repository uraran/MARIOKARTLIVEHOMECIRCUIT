#ifndef __HDMI_RX_DDC_REG__H
#define __HDMI_RX_DDC_REG__H

// HDMI MISC DDC
// HDMI Register Address base 0x18037600

#define DDC1_I2C_CR_REG                                                     0x0000
#define DDC1_EDID_CR_REG                                                    0x000c

#define DDC1_EDID_CR_edid_address_shift                                 (5)
#define DDC1_EDID_CR_finish_int_en_shift                                (4)
#define DDC1_EDID_CR_ddc1_force_shift                                   (3)
#define DDC1_EDID_CR_ddc2b_force_shift                                  (2)
#define DDC1_EDID_CR_edid_srwen_shift                                   (1)
#define DDC1_EDID_CR_edid_en_shift                                      (0)
#define DDC1_EDID_CR_edid_address_mask                                  (0x000000E0)
#define DDC1_EDID_CR_finish_int_en_mask                                 (0x00000010)
#define DDC1_EDID_CR_ddc1_force_mask                                    (0x00000008)
#define DDC1_EDID_CR_ddc2b_force_mask                                   (0x00000004)
#define DDC1_EDID_CR_edid_srwen_mask                                    (0x00000002)
#define DDC1_EDID_CR_edid_en_mask                                       (0x00000001)
#define DDC1_EDID_CR_edid_address(data)                                 (0x000000E0&((data)<<5))
#define DDC1_EDID_CR_finish_int_en(data)                                (0x00000010&((data)<<4))
#define DDC1_EDID_CR_ddc1_force(data)                                   (0x00000008&((data)<<3))
#define DDC1_EDID_CR_ddc2b_force(data)                                  (0x00000004&((data)<<2))
#define DDC1_EDID_CR_edid_srwen(data)                                   (0x00000002&((data)<<1))
#define DDC1_EDID_CR_edid_en(data)                                      (0x00000001&(data))
#define DDC1_EDID_CR_get_edid_address(data)                             ((0x000000E0&(data))>>5)
#define DDC1_EDID_CR_get_finish_int_en(data)                            ((0x00000010&(data))>>4)
#define DDC1_EDID_CR_get_ddc1_force(data)                               ((0x00000008&(data))>>3)
#define DDC1_EDID_CR_get_ddc2b_force(data)                              ((0x00000004&(data))>>2)
#define DDC1_EDID_CR_get_edid_srwen(data)                               ((0x00000002&(data))>>1)
#define DDC1_EDID_CR_get_edid_en(data)                                  (0x00000001&(data))

#endif // __HDMI_RX_DDC_REG__H
