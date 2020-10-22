#ifndef AIO_REGDEF_H
#define AIO_REGDEF_H

enum
{
    HDMI_SEL_PCM_FIFO = 0,
    HDMI_SEL_RAW_FIFO = 2,
    HDMI_SEL_HDMI_FIFO = 4,
    HDMI_SEL_HIGH_BITRATE = 6,
};

enum
{
    CS_FROM_I2S = 0,
    CS_FROM_SPDIF = 1,
    CS_FROM_HDMI = 2,
};

#define HDMI_SEL_MASK					0xFFFFFFF9
#define HDMI_8CH_BIT_MASK               0xFFFFF7FF
#define SYS_AIO_O_CK_CTL				0x18000070
#define SYS_AIO_O_RAW_CK_CTL            0x18000074
#define SYS_AIO_O_HDMI_CK_CTL           0x18000078
#ifdef UNICORN
#define CRT_AIO_I_PCM_CK_CTL			0x18000080
#endif

//Global Control Register
#define AIO_O_I2S_GCTL					0x18006004
#define AIO_O_SPF_GCTL					0x18006050
#define AIO_O_ACDAC_GCTL                0x18006600
#define AIO_O_HDMI_CTRL					0x18006070

#define AIO_O_FIFO_STATUS               0x180060c4
#define	AIO_O_FADE_ADDR			        0x180060c8
#define AIO_O_GO						0x18006304
#define AIO_O_SA_ADDR					0x18006180
#define AIO_O_EA_ADDR					0x180061B0
#define AIO_O_RP_ADDR					0x180061E0
#define AIO_O_WP_ADDR					0x18006220
#define AIO_O_INT_EN_ADDR				0x18006440
#define AIO_O_DDRTH_ADDR				0x18006400
#define AIO_O_INT_STATUS_ADDR			0x18006444
#define AIO_Timer_int_Audio_out			0x18006404
#define AIO_Timer_int_SPDIF_in			0x1800631C
#define AIO_O_SPF_CS1					0x18006058
#define AIO_O_SPF_CS2					0x1800605C
#define AIO_O_I2S_CS1					0x1800600C
#define AIO_O_I2S_CS2					0x18006010
#define AIO_O_HDMI_CS1                  0x18006078
#define AIO_O_HDMI_CS2                  0x1800607c
#define AIO_O_FIFOCTL					0x18006090
#define AIO_O_PAUSE						0x1800630C
#define AIO_O_TIMTHD					0x18006408
#define AO_PLL_DISP_SD5					0x18000128
#define AO_ASPER1						0x1800D054
#define AO_HDMI_ICR						0x1800D0BC
#define AO_HDMI_RPEN					0x1800D0A4
#define MIS_DCPHY_CKSEL                 0xB800011C
#define MARS_CHIP_VER                   0xB801A204
#define MARS_CHIP_VER_D                 0x00D00000

// although hw has 31 bit address map, but 0x80000000&0x7FFFFFFF !=  0xA0000000&0x7FFFFFFF,
// now still use 29 bit mask
//#define AO_HW_ADDR_BIT_MASK             0x7FFFFFFF//Saturn[0:30]
#define AO_HW_ADDR_BIT_MASK             0x1FFFFFFF//Saturn[0:28]  // although hw has 31 bit address map

#define AO_WRITE_SA_FIFO(n,addr)     AIO_write_register((long*)RBUS_VADDR(AIO_O_SA_ADDR+(n<<2)), addr)
#define AO_WRITE_EA_FIFO(n,addr)     AIO_write_register((long*)RBUS_VADDR(AIO_O_EA_ADDR+(n<<2)), addr)
#define AO_WRITE_RP_FIFO(n,addr)     AIO_write_register((long*)RBUS_VADDR(AIO_O_RP_ADDR+(n<<2)), addr)
#define AO_WRITE_WP_FIFO(n,addr)     AIO_write_register((long*)RBUS_VADDR(AIO_O_WP_ADDR+(n<<2)), addr)

#define AO_READ_SA_FIFO(n)      AIO_read_register((long*)RBUS_VADDR(AIO_O_SA_ADDR+(n<<2)))
#define AO_READ_EA_FIFO(n)      AIO_read_register((long*)RBUS_VADDR(AIO_O_EA_ADDR+(n<<2)))
#define AO_READ_RP_FIFO(n)      AIO_read_register((long*)RBUS_VADDR(AIO_O_RP_ADDR+(n<<2)))
#define AO_READ_WP_FIFO(n)      AIO_read_register((long*)RBUS_VADDR(AIO_O_WP_ADDR+(n<<2)))

//========================================================================//

#define AIO_O_ACANA_GCTL1                                                            0x18006604
#define AIO_O_ACANA_GCTL1_reg_addr                                                   "0xB8006604"
#define AIO_O_ACANA_GCTL1_reg                                                        0xB8006604
#define set_AIO_O_ACANA_GCTL1_reg(data)   (*((volatile unsigned int*) AIO_O_ACANA_GCTL1_reg)=data)
#define get_AIO_O_ACANA_GCTL1_reg   (*((volatile unsigned int*) AIO_O_ACANA_GCTL1_reg))
#define AIO_O_ACANA_GCTL1_inst_adr                                                   "0x0081"
#define AIO_O_ACANA_GCTL1_inst                                                       0x0081
#define AIO_O_ACANA_GCTL1_write_en1_shift                                            (31)
#define AIO_O_ACANA_GCTL1_write_en1_mask                                             (0x80000000)
#define AIO_O_ACANA_GCTL1_write_en1(data)                                            (0x80000000&((data)<<31))
#define AIO_O_ACANA_GCTL1_write_en1_src(data)                                        ((0x80000000&(data))>>31)
#define AIO_O_ACANA_GCTL1_get_write_en1(data)                                        ((0x80000000&(data))>>31)
#define AIO_O_ACANA_GCTL1_pow_bias_shift                                             (30)
#define AIO_O_ACANA_GCTL1_pow_bias_mask                                              (0x40000000)
#define AIO_O_ACANA_GCTL1_pow_bias(data)                                             (0x40000000&((data)<<30))
#define AIO_O_ACANA_GCTL1_pow_bias_src(data)                                         ((0x40000000&(data))>>30)
#define AIO_O_ACANA_GCTL1_get_pow_bias(data)                                         ((0x40000000&(data))>>30)
#define AIO_O_ACANA_GCTL1_write_en3_shift                                            (27)
#define AIO_O_ACANA_GCTL1_write_en3_mask                                             (0x08000000)
#define AIO_O_ACANA_GCTL1_write_en3(data)                                            (0x08000000&((data)<<27))
#define AIO_O_ACANA_GCTL1_write_en3_src(data)                                        ((0x08000000&(data))>>27)
#define AIO_O_ACANA_GCTL1_get_write_en3(data)                                        ((0x08000000&(data))>>27)
#define AIO_O_ACANA_GCTL1_pow_dac_shift                                              (26)
#define AIO_O_ACANA_GCTL1_pow_dac_mask                                               (0x04000000)
#define AIO_O_ACANA_GCTL1_pow_dac(data)                                              (0x04000000&((data)<<26))
#define AIO_O_ACANA_GCTL1_pow_dac_src(data)                                          ((0x04000000&(data))>>26)
#define AIO_O_ACANA_GCTL1_get_pow_dac(data)                                          ((0x04000000&(data))>>26)
#define AIO_O_ACANA_GCTL1_write_en4_shift                                            (25)
#define AIO_O_ACANA_GCTL1_write_en4_mask                                             (0x02000000)
#define AIO_O_ACANA_GCTL1_write_en4(data)                                            (0x02000000&((data)<<25))
#define AIO_O_ACANA_GCTL1_write_en4_src(data)                                        ((0x02000000&(data))>>25)
#define AIO_O_ACANA_GCTL1_get_write_en4(data)                                        ((0x02000000&(data))>>25)
#define AIO_O_ACANA_GCTL1_rstb_shift                                                 (24)
#define AIO_O_ACANA_GCTL1_rstb_mask                                                  (0x01000000)
#define AIO_O_ACANA_GCTL1_rstb(data)                                                 (0x01000000&((data)<<24))
#define AIO_O_ACANA_GCTL1_rstb_src(data)                                             ((0x01000000&(data))>>24)
#define AIO_O_ACANA_GCTL1_get_rstb(data)                                             ((0x01000000&(data))>>24)
#define AIO_O_ACANA_GCTL1_write_en6_shift                                            (20)
#define AIO_O_ACANA_GCTL1_write_en6_mask                                             (0x00100000)
#define AIO_O_ACANA_GCTL1_write_en6(data)                                            (0x00100000&((data)<<20))
#define AIO_O_ACANA_GCTL1_write_en6_src(data)                                        ((0x00100000&(data))>>20)
#define AIO_O_ACANA_GCTL1_get_write_en6(data)                                        ((0x00100000&(data))>>20)
#define AIO_O_ACANA_GCTL1_unmute_l_shift                                             (19)
#define AIO_O_ACANA_GCTL1_unmute_l_mask                                              (0x00080000)
#define AIO_O_ACANA_GCTL1_unmute_l(data)                                             (0x00080000&((data)<<19))
#define AIO_O_ACANA_GCTL1_unmute_l_src(data)                                         ((0x00080000&(data))>>19)
#define AIO_O_ACANA_GCTL1_get_unmute_l(data)                                         ((0x00080000&(data))>>19)
#define AIO_O_ACANA_GCTL1_write_en7_shift                                            (18)
#define AIO_O_ACANA_GCTL1_write_en7_mask                                             (0x00040000)
#define AIO_O_ACANA_GCTL1_write_en7(data)                                            (0x00040000&((data)<<18))
#define AIO_O_ACANA_GCTL1_write_en7_src(data)                                        ((0x00040000&(data))>>18)
#define AIO_O_ACANA_GCTL1_get_write_en7(data)                                        ((0x00040000&(data))>>18)
#define AIO_O_ACANA_GCTL1_unmute_r_shift                                             (17)
#define AIO_O_ACANA_GCTL1_unmute_r_mask                                              (0x00020000)
#define AIO_O_ACANA_GCTL1_unmute_r(data)                                             (0x00020000&((data)<<17))
#define AIO_O_ACANA_GCTL1_unmute_r_src(data)                                         ((0x00020000&(data))>>17)
#define AIO_O_ACANA_GCTL1_get_unmute_r(data)                                         ((0x00020000&(data))>>17)
#define AIO_O_ACANA_GCTL1_write_en8_shift                                            (16)
#define AIO_O_ACANA_GCTL1_write_en8_mask                                             (0x00010000)
#define AIO_O_ACANA_GCTL1_write_en8(data)                                            (0x00010000&((data)<<16))
#define AIO_O_ACANA_GCTL1_write_en8_src(data)                                        ((0x00010000&(data))>>16)
#define AIO_O_ACANA_GCTL1_get_write_en8(data)                                        ((0x00010000&(data))>>16)
#define AIO_O_ACANA_GCTL1_ckxen_shift                                                (15)
#define AIO_O_ACANA_GCTL1_ckxen_mask                                                 (0x00008000)
#define AIO_O_ACANA_GCTL1_ckxen(data)                                                (0x00008000&((data)<<15))
#define AIO_O_ACANA_GCTL1_ckxen_src(data)                                            ((0x00008000&(data))>>15)
#define AIO_O_ACANA_GCTL1_get_ckxen(data)                                            ((0x00008000&(data))>>15)
#define AIO_O_ACANA_GCTL1_write_en9_shift                                            (14)
#define AIO_O_ACANA_GCTL1_write_en9_mask                                             (0x00004000)
#define AIO_O_ACANA_GCTL1_write_en9(data)                                            (0x00004000&((data)<<14))
#define AIO_O_ACANA_GCTL1_write_en9_src(data)                                        ((0x00004000&(data))>>14)
#define AIO_O_ACANA_GCTL1_get_write_en9(data)                                        ((0x00004000&(data))>>14)
#define AIO_O_ACANA_GCTL1_ckxsel_shift                                               (13)
#define AIO_O_ACANA_GCTL1_ckxsel_mask                                                (0x00002000)
#define AIO_O_ACANA_GCTL1_ckxsel(data)                                               (0x00002000&((data)<<13))
#define AIO_O_ACANA_GCTL1_ckxsel_src(data)                                           ((0x00002000&(data))>>13)
#define AIO_O_ACANA_GCTL1_get_ckxsel(data)                                           ((0x00002000&(data))>>13)
#define AIO_O_ACANA_GCTL1_write_en10_shift                                           (12)
#define AIO_O_ACANA_GCTL1_write_en10_mask                                            (0x00001000)
#define AIO_O_ACANA_GCTL1_write_en10(data)                                           (0x00001000&((data)<<12))
#define AIO_O_ACANA_GCTL1_write_en10_src(data)                                       ((0x00001000&(data))>>12)
#define AIO_O_ACANA_GCTL1_get_write_en10(data)                                       ((0x00001000&(data))>>12)
#define AIO_O_ACANA_GCTL1_cks_sel_shift                                              (11)
#define AIO_O_ACANA_GCTL1_cks_sel_mask                                               (0x00000800)
#define AIO_O_ACANA_GCTL1_cks_sel(data)                                              (0x00000800&((data)<<11))
#define AIO_O_ACANA_GCTL1_cks_sel_src(data)                                          ((0x00000800&(data))>>11)
#define AIO_O_ACANA_GCTL1_get_cks_sel(data)                                          ((0x00000800&(data))>>11)
#define AIO_O_ACANA_GCTL1_write_en11_shift                                           (10)
#define AIO_O_ACANA_GCTL1_write_en11_mask                                            (0x00000400)
#define AIO_O_ACANA_GCTL1_write_en11(data)                                           (0x00000400&((data)<<10))
#define AIO_O_ACANA_GCTL1_write_en11_src(data)                                       ((0x00000400&(data))>>10)
#define AIO_O_ACANA_GCTL1_get_write_en11(data)                                       ((0x00000400&(data))>>10)
#define AIO_O_ACANA_GCTL1_v2i_sel_shift                                              (9)
#define AIO_O_ACANA_GCTL1_v2i_sel_mask                                               (0x00000200)
#define AIO_O_ACANA_GCTL1_v2i_sel(data)                                              (0x00000200&((data)<<9))
#define AIO_O_ACANA_GCTL1_v2i_sel_src(data)                                          ((0x00000200&(data))>>9)
#define AIO_O_ACANA_GCTL1_get_v2i_sel(data)                                          ((0x00000200&(data))>>9)
#define AIO_O_ACANA_GCTL1_write_en12_shift                                           (8)
#define AIO_O_ACANA_GCTL1_write_en12_mask                                            (0x00000100)
#define AIO_O_ACANA_GCTL1_write_en12(data)                                           (0x00000100&((data)<<8))
#define AIO_O_ACANA_GCTL1_write_en12_src(data)                                       ((0x00000100&(data))>>8)
#define AIO_O_ACANA_GCTL1_get_write_en12(data)                                       ((0x00000100&(data))>>8)
#define AIO_O_ACANA_GCTL1_imp_shift                                                  (7)
#define AIO_O_ACANA_GCTL1_imp_mask                                                   (0x00000080)
#define AIO_O_ACANA_GCTL1_imp(data)                                                  (0x00000080&((data)<<7))
#define AIO_O_ACANA_GCTL1_imp_src(data)                                              ((0x00000080&(data))>>7)
#define AIO_O_ACANA_GCTL1_get_imp(data)                                              ((0x00000080&(data))>>7)
#define AIO_O_ACANA_GCTL1_write_en13_shift                                           (6)
#define AIO_O_ACANA_GCTL1_write_en13_mask                                            (0x00000040)
#define AIO_O_ACANA_GCTL1_write_en13(data)                                           (0x00000040&((data)<<6))
#define AIO_O_ACANA_GCTL1_write_en13_src(data)                                       ((0x00000040&(data))>>6)
#define AIO_O_ACANA_GCTL1_get_write_en13(data)                                       ((0x00000040&(data))>>6)
#define AIO_O_ACANA_GCTL1_disc_shift                                                 (5)
#define AIO_O_ACANA_GCTL1_disc_mask                                                  (0x00000020)
#define AIO_O_ACANA_GCTL1_disc(data)                                                 (0x00000020&((data)<<5))
#define AIO_O_ACANA_GCTL1_disc_src(data)                                             ((0x00000020&(data))>>5)
#define AIO_O_ACANA_GCTL1_get_disc(data)                                             ((0x00000020&(data))>>5)
#define AIO_O_ACANA_GCTL1_write_en14_shift                                           (4)
#define AIO_O_ACANA_GCTL1_write_en14_mask                                            (0x00000010)
#define AIO_O_ACANA_GCTL1_write_en14(data)                                           (0x00000010&((data)<<4))
#define AIO_O_ACANA_GCTL1_write_en14_src(data)                                       ((0x00000010&(data))>>4)
#define AIO_O_ACANA_GCTL1_get_write_en14(data)                                       ((0x00000010&(data))>>4)
#define AIO_O_ACANA_GCTL1_en_amp_shift                                               (3)
#define AIO_O_ACANA_GCTL1_en_amp_mask                                                (0x00000008)
#define AIO_O_ACANA_GCTL1_en_amp(data)                                               (0x00000008&((data)<<3))
#define AIO_O_ACANA_GCTL1_en_amp_src(data)                                           ((0x00000008&(data))>>3)
#define AIO_O_ACANA_GCTL1_get_en_amp(data)                                           ((0x00000008&(data))>>3)
#define AIO_O_ACANA_GCTL1_write_en15_shift                                           (2)
#define AIO_O_ACANA_GCTL1_write_en15_mask                                            (0x00000004)
#define AIO_O_ACANA_GCTL1_write_en15(data)                                           (0x00000004&((data)<<2))
#define AIO_O_ACANA_GCTL1_write_en15_src(data)                                       ((0x00000004&(data))>>2)
#define AIO_O_ACANA_GCTL1_get_write_en15(data)                                       ((0x00000004&(data))>>2)
#define AIO_O_ACANA_GCTL1_vrp_sel_shift                                              (0)
#define AIO_O_ACANA_GCTL1_vrp_sel_mask                                               (0x00000003)
#define AIO_O_ACANA_GCTL1_vrp_sel(data)                                              (0x00000003&((data)<<0))
#define AIO_O_ACANA_GCTL1_vrp_sel_src(data)                                          ((0x00000003&(data))>>0)
#define AIO_O_ACANA_GCTL1_get_vrp_sel(data)                                          ((0x00000003&(data))>>0)

#define AIO_O_ACANA_GCTL2                                                            0x18006608
#define AIO_O_ACANA_GCTL2_reg_addr                                                   "0xB8006608"
#define AIO_O_ACANA_GCTL2_reg                                                        0xB8006608
#define set_AIO_O_ACANA_GCTL2_reg(data)   (*((volatile unsigned int*) AIO_O_ACANA_GCTL2_reg)=data)
#define get_AIO_O_ACANA_GCTL2_reg   (*((volatile unsigned int*) AIO_O_ACANA_GCTL2_reg))
#define AIO_O_ACANA_GCTL2_inst_adr                                                   "0x0082"
#define AIO_O_ACANA_GCTL2_inst                                                       0x0082
#define AIO_O_ACANA_GCTL2_write_en1_shift                                            (18)
#define AIO_O_ACANA_GCTL2_write_en1_mask                                             (0x00040000)
#define AIO_O_ACANA_GCTL2_write_en1(data)                                            (0x00040000&((data)<<18))
#define AIO_O_ACANA_GCTL2_write_en1_src(data)                                        ((0x00040000&(data))>>18)
#define AIO_O_ACANA_GCTL2_get_write_en1(data)                                        ((0x00040000&(data))>>18)
#define AIO_O_ACANA_GCTL2_vcmh_sel_shift                                             (17)
#define AIO_O_ACANA_GCTL2_vcmh_sel_mask                                              (0x00020000)
#define AIO_O_ACANA_GCTL2_vcmh_sel(data)                                             (0x00020000&((data)<<17))
#define AIO_O_ACANA_GCTL2_vcmh_sel_src(data)                                         ((0x00020000&(data))>>17)
#define AIO_O_ACANA_GCTL2_get_vcmh_sel(data)                                         ((0x00020000&(data))>>17)
#define AIO_O_ACANA_GCTL2_write_en2_shift                                            (16)
#define AIO_O_ACANA_GCTL2_write_en2_mask                                             (0x00010000)
#define AIO_O_ACANA_GCTL2_write_en2(data)                                            (0x00010000&((data)<<16))
#define AIO_O_ACANA_GCTL2_write_en2_src(data)                                        ((0x00010000&(data))>>16)
#define AIO_O_ACANA_GCTL2_get_write_en2(data)                                        ((0x00010000&(data))>>16)
#define AIO_O_ACANA_GCTL2_ct_bias_shift                                              (0)
#define AIO_O_ACANA_GCTL2_ct_bias_mask                                               (0x0000FFFF)
#define AIO_O_ACANA_GCTL2_ct_bias(data)                                              (0x0000FFFF&((data)<<0))
#define AIO_O_ACANA_GCTL2_ct_bias_src(data)                                          ((0x0000FFFF&(data))>>0)
#define AIO_O_ACANA_GCTL2_get_ct_bias(data)                                          ((0x0000FFFF&(data))>>0)


//========================================================================//
#endif // #ifndef AIO_REGDEF_H
