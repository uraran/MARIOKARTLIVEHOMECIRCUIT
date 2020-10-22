/**************************************************************
// Spec Version                  : 0.0.5
// Parser Version                : DVR_Parser_6.11(120105)
// CModelGen Version             : 5.1 2012.01.05
// Naming Rule                   :  Register_Name
// Naming Rule                   : Module_Register_Name_reg
// Parse Option                  : Only Parse _op1
// Parse Address Region          : All Address Region 
// Decode bit number             : 12 bits
// Firmware Header Generate Date : 2015/3/24 18:8:55
***************************************************************/


#ifndef _EFUSE_REG_H_INCLUDED_
#define _EFUSE_REG_H_INCLUDED_
#ifdef  _EFUSE_USE_STRUCT
typedef struct 
{
unsigned int     datao:32;
}DATA;

typedef struct 
{
unsigned int     reserved_0:8;
unsigned int     write_enable4:1;
unsigned int     en_pro_efuse:1;
unsigned int     write_enable3:1;
unsigned int     auto_rd_req:1;
unsigned int     reserved_1:4;
unsigned int     write_enable2:1;
unsigned int     strobe:1;
unsigned int     write_enable1:1;
unsigned int     a:13;
}EF_CTRL;

typedef struct 
{
unsigned int     reserved_0:15;
unsigned int     pro_efuse_act:1;
unsigned int     reserved_1:15;
unsigned int     crc_en:1;
}EF_CTRL_ST;

typedef struct 
{
unsigned int     reserved_0:8;
unsigned int     crc_ini:8;
unsigned int     reserved_1:8;
unsigned int     crc_out:8;
}EF_CRC;

typedef struct 
{
unsigned int     reserved_0:24;
unsigned int     write_enable2:1;
unsigned int     mr_mode:1;
unsigned int     reserved_1:1;
unsigned int     write_enable1:1;
unsigned int     prog_strobe_pulse_width:4;
}EF_CTRL_NEW;

typedef struct 
{
unsigned int     reserved_0:30;
unsigned int     Rd_done:1;
unsigned int     write_data:1;
}EF_RD_DONE;

#endif

#define DATA_0                                                                       0x18017000
#define DATA_1                                                                       0x18017004
#define DATA_2                                                                       0x18017008
#define DATA_3                                                                       0x1801700C
#define DATA_4                                                                       0x18017010
#define DATA_5                                                                       0x18017014
#define DATA_6                                                                       0x18017018
#define DATA_7                                                                       0x1801701C
#define DATA_8                                                                       0x18017020
#define DATA_9                                                                       0x18017024
#define DATA_10                                                                       0x18017028
#define DATA_11                                                                       0x1801702C
#define DATA_12                                                                       0x18017030
#define DATA_13                                                                       0x18017034
#define DATA_14                                                                       0x18017038
#define DATA_15                                                                       0x1801703C
#define DATA_16                                                                       0x18017040
#define DATA_17                                                                       0x18017044
#define DATA_18                                                                       0x18017048
#define DATA_19                                                                       0x1801704C
#define DATA_20                                                                       0x18017050
#define DATA_21                                                                       0x18017054
#define DATA_22                                                                       0x18017058
#define DATA_23                                                                       0x1801705C
#define DATA_24                                                                       0x18017060
#define DATA_25                                                                       0x18017064
#define DATA_26                                                                       0x18017068
#define DATA_27                                                                       0x1801706C
#define DATA_28                                                                       0x18017070
#define DATA_29                                                                       0x18017074
#define DATA_30                                                                       0x18017078
#define DATA_31                                                                       0x1801707C
#define DATA_32                                                                       0x18017080
#define DATA_33                                                                       0x18017084
#define DATA_34                                                                       0x18017088
#define DATA_35                                                                       0x1801708C
#define DATA_36                                                                       0x18017090
#define DATA_37                                                                       0x18017094
#define DATA_38                                                                       0x18017098
#define DATA_39                                                                       0x1801709C
#define DATA_40                                                                       0x180170A0
#define DATA_41                                                                       0x180170A4
#define DATA_42                                                                       0x180170A8
#define DATA_43                                                                       0x180170AC
#define DATA_44                                                                       0x180170B0
#define DATA_45                                                                       0x180170B4
#define DATA_46                                                                       0x180170B8
#define DATA_47                                                                       0x180170BC
#define DATA_48                                                                       0x180170C0
#define DATA_49                                                                       0x180170C4
#define DATA_50                                                                       0x180170C8
#define DATA_51                                                                       0x180170CC
#define DATA_52                                                                       0x180170D0
#define DATA_53                                                                       0x180170D4
#define DATA_54                                                                       0x180170D8
#define DATA_55                                                                       0x180170DC
#define DATA_56                                                                       0x180170E0
#define DATA_57                                                                       0x180170E4
#define DATA_58                                                                       0x180170E8
#define DATA_59                                                                       0x180170EC
#define DATA_60                                                                       0x180170F0
#define DATA_61                                                                       0x180170F4
#define DATA_62                                                                       0x180170F8
#define DATA_63                                                                       0x180170FC
#define DATA_64                                                                       0x18017100
#define DATA_65                                                                       0x18017104
#define DATA_66                                                                       0x18017108
#define DATA_67                                                                       0x1801710C
#define DATA_68                                                                       0x18017110
#define DATA_69                                                                       0x18017114
#define DATA_70                                                                       0x18017118
#define DATA_71                                                                       0x1801711C
#define DATA_72                                                                       0x18017120
#define DATA_73                                                                       0x18017124
#define DATA_74                                                                       0x18017128
#define DATA_75                                                                       0x1801712C
#define DATA_76                                                                       0x18017130
#define DATA_77                                                                       0x18017134
#define DATA_78                                                                       0x18017138
#define DATA_79                                                                       0x1801713C
#define DATA_80                                                                       0x18017140
#define DATA_81                                                                       0x18017144
#define DATA_82                                                                       0x18017148
#define DATA_83                                                                       0x1801714C
#define DATA_84                                                                       0x18017150
#define DATA_85                                                                       0x18017154
#define DATA_86                                                                       0x18017158
#define DATA_87                                                                       0x1801715C
#define DATA_88                                                                       0x18017160
#define DATA_89                                                                       0x18017164
#define DATA_90                                                                       0x18017168
#define DATA_91                                                                       0x1801716C
#define DATA_92                                                                       0x18017170
#define DATA_93                                                                       0x18017174
#define DATA_94                                                                       0x18017178
#define DATA_95                                                                       0x1801717C
#define DATA_96                                                                       0x18017180
#define DATA_97                                                                       0x18017184
#define DATA_98                                                                       0x18017188
#define DATA_99                                                                       0x1801718C
#define DATA_100                                                                       0x18017190
#define DATA_101                                                                       0x18017194
#define DATA_102                                                                       0x18017198
#define DATA_103                                                                       0x1801719C
#define DATA_104                                                                       0x180171A0
#define DATA_105                                                                       0x180171A4
#define DATA_106                                                                       0x180171A8
#define DATA_107                                                                       0x180171AC
#define DATA_108                                                                       0x180171B0
#define DATA_109                                                                       0x180171B4
#define DATA_110                                                                       0x180171B8
#define DATA_111                                                                       0x180171BC
#define DATA_112                                                                       0x180171C0
#define DATA_113                                                                       0x180171C4
#define DATA_114                                                                       0x180171C8
#define DATA_115                                                                       0x180171CC
#define DATA_116                                                                       0x180171D0
#define DATA_117                                                                       0x180171D4
#define DATA_118                                                                       0x180171D8
#define DATA_119                                                                       0x180171DC
#define DATA_120                                                                       0x180171E0
#define DATA_121                                                                       0x180171E4
#define DATA_122                                                                       0x180171E8
#define DATA_123                                                                       0x180171EC
#define DATA_124                                                                       0x180171F0
#define DATA_125                                                                       0x180171F4
#define DATA_126                                                                       0x180171F8
#define DATA_127                                                                       0x180171FC
#define DATA_0_reg_addr                                                              "0x18017000"
#define DATA_1_reg_addr                                                              "0x18017004"
#define DATA_2_reg_addr                                                              "0x18017008"
#define DATA_3_reg_addr                                                              "0x1801700C"
#define DATA_4_reg_addr                                                              "0x18017010"
#define DATA_5_reg_addr                                                              "0x18017014"
#define DATA_6_reg_addr                                                              "0x18017018"
#define DATA_7_reg_addr                                                              "0x1801701C"
#define DATA_8_reg_addr                                                              "0x18017020"
#define DATA_9_reg_addr                                                              "0x18017024"
#define DATA_10_reg_addr                                                              "0x18017028"
#define DATA_11_reg_addr                                                              "0x1801702C"
#define DATA_12_reg_addr                                                              "0x18017030"
#define DATA_13_reg_addr                                                              "0x18017034"
#define DATA_14_reg_addr                                                              "0x18017038"
#define DATA_15_reg_addr                                                              "0x1801703C"
#define DATA_16_reg_addr                                                              "0x18017040"
#define DATA_17_reg_addr                                                              "0x18017044"
#define DATA_18_reg_addr                                                              "0x18017048"
#define DATA_19_reg_addr                                                              "0x1801704C"
#define DATA_20_reg_addr                                                              "0x18017050"
#define DATA_21_reg_addr                                                              "0x18017054"
#define DATA_22_reg_addr                                                              "0x18017058"
#define DATA_23_reg_addr                                                              "0x1801705C"
#define DATA_24_reg_addr                                                              "0x18017060"
#define DATA_25_reg_addr                                                              "0x18017064"
#define DATA_26_reg_addr                                                              "0x18017068"
#define DATA_27_reg_addr                                                              "0x1801706C"
#define DATA_28_reg_addr                                                              "0x18017070"
#define DATA_29_reg_addr                                                              "0x18017074"
#define DATA_30_reg_addr                                                              "0x18017078"
#define DATA_31_reg_addr                                                              "0x1801707C"
#define DATA_32_reg_addr                                                              "0x18017080"
#define DATA_33_reg_addr                                                              "0x18017084"
#define DATA_34_reg_addr                                                              "0x18017088"
#define DATA_35_reg_addr                                                              "0x1801708C"
#define DATA_36_reg_addr                                                              "0x18017090"
#define DATA_37_reg_addr                                                              "0x18017094"
#define DATA_38_reg_addr                                                              "0x18017098"
#define DATA_39_reg_addr                                                              "0x1801709C"
#define DATA_40_reg_addr                                                              "0x180170A0"
#define DATA_41_reg_addr                                                              "0x180170A4"
#define DATA_42_reg_addr                                                              "0x180170A8"
#define DATA_43_reg_addr                                                              "0x180170AC"
#define DATA_44_reg_addr                                                              "0x180170B0"
#define DATA_45_reg_addr                                                              "0x180170B4"
#define DATA_46_reg_addr                                                              "0x180170B8"
#define DATA_47_reg_addr                                                              "0x180170BC"
#define DATA_48_reg_addr                                                              "0x180170C0"
#define DATA_49_reg_addr                                                              "0x180170C4"
#define DATA_50_reg_addr                                                              "0x180170C8"
#define DATA_51_reg_addr                                                              "0x180170CC"
#define DATA_52_reg_addr                                                              "0x180170D0"
#define DATA_53_reg_addr                                                              "0x180170D4"
#define DATA_54_reg_addr                                                              "0x180170D8"
#define DATA_55_reg_addr                                                              "0x180170DC"
#define DATA_56_reg_addr                                                              "0x180170E0"
#define DATA_57_reg_addr                                                              "0x180170E4"
#define DATA_58_reg_addr                                                              "0x180170E8"
#define DATA_59_reg_addr                                                              "0x180170EC"
#define DATA_60_reg_addr                                                              "0x180170F0"
#define DATA_61_reg_addr                                                              "0x180170F4"
#define DATA_62_reg_addr                                                              "0x180170F8"
#define DATA_63_reg_addr                                                              "0x180170FC"
#define DATA_64_reg_addr                                                              "0x18017100"
#define DATA_65_reg_addr                                                              "0x18017104"
#define DATA_66_reg_addr                                                              "0x18017108"
#define DATA_67_reg_addr                                                              "0x1801710C"
#define DATA_68_reg_addr                                                              "0x18017110"
#define DATA_69_reg_addr                                                              "0x18017114"
#define DATA_70_reg_addr                                                              "0x18017118"
#define DATA_71_reg_addr                                                              "0x1801711C"
#define DATA_72_reg_addr                                                              "0x18017120"
#define DATA_73_reg_addr                                                              "0x18017124"
#define DATA_74_reg_addr                                                              "0x18017128"
#define DATA_75_reg_addr                                                              "0x1801712C"
#define DATA_76_reg_addr                                                              "0x18017130"
#define DATA_77_reg_addr                                                              "0x18017134"
#define DATA_78_reg_addr                                                              "0x18017138"
#define DATA_79_reg_addr                                                              "0x1801713C"
#define DATA_80_reg_addr                                                              "0x18017140"
#define DATA_81_reg_addr                                                              "0x18017144"
#define DATA_82_reg_addr                                                              "0x18017148"
#define DATA_83_reg_addr                                                              "0x1801714C"
#define DATA_84_reg_addr                                                              "0x18017150"
#define DATA_85_reg_addr                                                              "0x18017154"
#define DATA_86_reg_addr                                                              "0x18017158"
#define DATA_87_reg_addr                                                              "0x1801715C"
#define DATA_88_reg_addr                                                              "0x18017160"
#define DATA_89_reg_addr                                                              "0x18017164"
#define DATA_90_reg_addr                                                              "0x18017168"
#define DATA_91_reg_addr                                                              "0x1801716C"
#define DATA_92_reg_addr                                                              "0x18017170"
#define DATA_93_reg_addr                                                              "0x18017174"
#define DATA_94_reg_addr                                                              "0x18017178"
#define DATA_95_reg_addr                                                              "0x1801717C"
#define DATA_96_reg_addr                                                              "0x18017180"
#define DATA_97_reg_addr                                                              "0x18017184"
#define DATA_98_reg_addr                                                              "0x18017188"
#define DATA_99_reg_addr                                                              "0x1801718C"
#define DATA_100_reg_addr                                                              "0x18017190"
#define DATA_101_reg_addr                                                              "0x18017194"
#define DATA_102_reg_addr                                                              "0x18017198"
#define DATA_103_reg_addr                                                              "0x1801719C"
#define DATA_104_reg_addr                                                              "0x180171A0"
#define DATA_105_reg_addr                                                              "0x180171A4"
#define DATA_106_reg_addr                                                              "0x180171A8"
#define DATA_107_reg_addr                                                              "0x180171AC"
#define DATA_108_reg_addr                                                              "0x180171B0"
#define DATA_109_reg_addr                                                              "0x180171B4"
#define DATA_110_reg_addr                                                              "0x180171B8"
#define DATA_111_reg_addr                                                              "0x180171BC"
#define DATA_112_reg_addr                                                              "0x180171C0"
#define DATA_113_reg_addr                                                              "0x180171C4"
#define DATA_114_reg_addr                                                              "0x180171C8"
#define DATA_115_reg_addr                                                              "0x180171CC"
#define DATA_116_reg_addr                                                              "0x180171D0"
#define DATA_117_reg_addr                                                              "0x180171D4"
#define DATA_118_reg_addr                                                              "0x180171D8"
#define DATA_119_reg_addr                                                              "0x180171DC"
#define DATA_120_reg_addr                                                              "0x180171E0"
#define DATA_121_reg_addr                                                              "0x180171E4"
#define DATA_122_reg_addr                                                              "0x180171E8"
#define DATA_123_reg_addr                                                              "0x180171EC"
#define DATA_124_reg_addr                                                              "0x180171F0"
#define DATA_125_reg_addr                                                              "0x180171F4"
#define DATA_126_reg_addr                                                              "0x180171F8"
#define DATA_127_reg_addr                                                              "0x180171FC"
#define DATA_0_reg                                                                   0x18017000
#define DATA_1_reg                                                                   0x18017004
#define DATA_2_reg                                                                   0x18017008
#define DATA_3_reg                                                                   0x1801700C
#define DATA_4_reg                                                                   0x18017010
#define DATA_5_reg                                                                   0x18017014
#define DATA_6_reg                                                                   0x18017018
#define DATA_7_reg                                                                   0x1801701C
#define DATA_8_reg                                                                   0x18017020
#define DATA_9_reg                                                                   0x18017024
#define DATA_10_reg                                                                   0x18017028
#define DATA_11_reg                                                                   0x1801702C
#define DATA_12_reg                                                                   0x18017030
#define DATA_13_reg                                                                   0x18017034
#define DATA_14_reg                                                                   0x18017038
#define DATA_15_reg                                                                   0x1801703C
#define DATA_16_reg                                                                   0x18017040
#define DATA_17_reg                                                                   0x18017044
#define DATA_18_reg                                                                   0x18017048
#define DATA_19_reg                                                                   0x1801704C
#define DATA_20_reg                                                                   0x18017050
#define DATA_21_reg                                                                   0x18017054
#define DATA_22_reg                                                                   0x18017058
#define DATA_23_reg                                                                   0x1801705C
#define DATA_24_reg                                                                   0x18017060
#define DATA_25_reg                                                                   0x18017064
#define DATA_26_reg                                                                   0x18017068
#define DATA_27_reg                                                                   0x1801706C
#define DATA_28_reg                                                                   0x18017070
#define DATA_29_reg                                                                   0x18017074
#define DATA_30_reg                                                                   0x18017078
#define DATA_31_reg                                                                   0x1801707C
#define DATA_32_reg                                                                   0x18017080
#define DATA_33_reg                                                                   0x18017084
#define DATA_34_reg                                                                   0x18017088
#define DATA_35_reg                                                                   0x1801708C
#define DATA_36_reg                                                                   0x18017090
#define DATA_37_reg                                                                   0x18017094
#define DATA_38_reg                                                                   0x18017098
#define DATA_39_reg                                                                   0x1801709C
#define DATA_40_reg                                                                   0x180170A0
#define DATA_41_reg                                                                   0x180170A4
#define DATA_42_reg                                                                   0x180170A8
#define DATA_43_reg                                                                   0x180170AC
#define DATA_44_reg                                                                   0x180170B0
#define DATA_45_reg                                                                   0x180170B4
#define DATA_46_reg                                                                   0x180170B8
#define DATA_47_reg                                                                   0x180170BC
#define DATA_48_reg                                                                   0x180170C0
#define DATA_49_reg                                                                   0x180170C4
#define DATA_50_reg                                                                   0x180170C8
#define DATA_51_reg                                                                   0x180170CC
#define DATA_52_reg                                                                   0x180170D0
#define DATA_53_reg                                                                   0x180170D4
#define DATA_54_reg                                                                   0x180170D8
#define DATA_55_reg                                                                   0x180170DC
#define DATA_56_reg                                                                   0x180170E0
#define DATA_57_reg                                                                   0x180170E4
#define DATA_58_reg                                                                   0x180170E8
#define DATA_59_reg                                                                   0x180170EC
#define DATA_60_reg                                                                   0x180170F0
#define DATA_61_reg                                                                   0x180170F4
#define DATA_62_reg                                                                   0x180170F8
#define DATA_63_reg                                                                   0x180170FC
#define DATA_64_reg                                                                   0x18017100
#define DATA_65_reg                                                                   0x18017104
#define DATA_66_reg                                                                   0x18017108
#define DATA_67_reg                                                                   0x1801710C
#define DATA_68_reg                                                                   0x18017110
#define DATA_69_reg                                                                   0x18017114
#define DATA_70_reg                                                                   0x18017118
#define DATA_71_reg                                                                   0x1801711C
#define DATA_72_reg                                                                   0x18017120
#define DATA_73_reg                                                                   0x18017124
#define DATA_74_reg                                                                   0x18017128
#define DATA_75_reg                                                                   0x1801712C
#define DATA_76_reg                                                                   0x18017130
#define DATA_77_reg                                                                   0x18017134
#define DATA_78_reg                                                                   0x18017138
#define DATA_79_reg                                                                   0x1801713C
#define DATA_80_reg                                                                   0x18017140
#define DATA_81_reg                                                                   0x18017144
#define DATA_82_reg                                                                   0x18017148
#define DATA_83_reg                                                                   0x1801714C
#define DATA_84_reg                                                                   0x18017150
#define DATA_85_reg                                                                   0x18017154
#define DATA_86_reg                                                                   0x18017158
#define DATA_87_reg                                                                   0x1801715C
#define DATA_88_reg                                                                   0x18017160
#define DATA_89_reg                                                                   0x18017164
#define DATA_90_reg                                                                   0x18017168
#define DATA_91_reg                                                                   0x1801716C
#define DATA_92_reg                                                                   0x18017170
#define DATA_93_reg                                                                   0x18017174
#define DATA_94_reg                                                                   0x18017178
#define DATA_95_reg                                                                   0x1801717C
#define DATA_96_reg                                                                   0x18017180
#define DATA_97_reg                                                                   0x18017184
#define DATA_98_reg                                                                   0x18017188
#define DATA_99_reg                                                                   0x1801718C
#define DATA_100_reg                                                                   0x18017190
#define DATA_101_reg                                                                   0x18017194
#define DATA_102_reg                                                                   0x18017198
#define DATA_103_reg                                                                   0x1801719C
#define DATA_104_reg                                                                   0x180171A0
#define DATA_105_reg                                                                   0x180171A4
#define DATA_106_reg                                                                   0x180171A8
#define DATA_107_reg                                                                   0x180171AC
#define DATA_108_reg                                                                   0x180171B0
#define DATA_109_reg                                                                   0x180171B4
#define DATA_110_reg                                                                   0x180171B8
#define DATA_111_reg                                                                   0x180171BC
#define DATA_112_reg                                                                   0x180171C0
#define DATA_113_reg                                                                   0x180171C4
#define DATA_114_reg                                                                   0x180171C8
#define DATA_115_reg                                                                   0x180171CC
#define DATA_116_reg                                                                   0x180171D0
#define DATA_117_reg                                                                   0x180171D4
#define DATA_118_reg                                                                   0x180171D8
#define DATA_119_reg                                                                   0x180171DC
#define DATA_120_reg                                                                   0x180171E0
#define DATA_121_reg                                                                   0x180171E4
#define DATA_122_reg                                                                   0x180171E8
#define DATA_123_reg                                                                   0x180171EC
#define DATA_124_reg                                                                   0x180171F0
#define DATA_125_reg                                                                   0x180171F4
#define DATA_126_reg                                                                   0x180171F8
#define DATA_127_reg                                                                   0x180171FC
#define set_DATA_0_reg(data)   (*((volatile unsigned int*) DATA_0_reg)=data)
#define set_DATA_1_reg(data)   (*((volatile unsigned int*) DATA_1_reg)=data)
#define set_DATA_2_reg(data)   (*((volatile unsigned int*) DATA_2_reg)=data)
#define set_DATA_3_reg(data)   (*((volatile unsigned int*) DATA_3_reg)=data)
#define set_DATA_4_reg(data)   (*((volatile unsigned int*) DATA_4_reg)=data)
#define set_DATA_5_reg(data)   (*((volatile unsigned int*) DATA_5_reg)=data)
#define set_DATA_6_reg(data)   (*((volatile unsigned int*) DATA_6_reg)=data)
#define set_DATA_7_reg(data)   (*((volatile unsigned int*) DATA_7_reg)=data)
#define set_DATA_8_reg(data)   (*((volatile unsigned int*) DATA_8_reg)=data)
#define set_DATA_9_reg(data)   (*((volatile unsigned int*) DATA_9_reg)=data)
#define set_DATA_10_reg(data)   (*((volatile unsigned int*) DATA_10_reg)=data)
#define set_DATA_11_reg(data)   (*((volatile unsigned int*) DATA_11_reg)=data)
#define set_DATA_12_reg(data)   (*((volatile unsigned int*) DATA_12_reg)=data)
#define set_DATA_13_reg(data)   (*((volatile unsigned int*) DATA_13_reg)=data)
#define set_DATA_14_reg(data)   (*((volatile unsigned int*) DATA_14_reg)=data)
#define set_DATA_15_reg(data)   (*((volatile unsigned int*) DATA_15_reg)=data)
#define set_DATA_16_reg(data)   (*((volatile unsigned int*) DATA_16_reg)=data)
#define set_DATA_17_reg(data)   (*((volatile unsigned int*) DATA_17_reg)=data)
#define set_DATA_18_reg(data)   (*((volatile unsigned int*) DATA_18_reg)=data)
#define set_DATA_19_reg(data)   (*((volatile unsigned int*) DATA_19_reg)=data)
#define set_DATA_20_reg(data)   (*((volatile unsigned int*) DATA_20_reg)=data)
#define set_DATA_21_reg(data)   (*((volatile unsigned int*) DATA_21_reg)=data)
#define set_DATA_22_reg(data)   (*((volatile unsigned int*) DATA_22_reg)=data)
#define set_DATA_23_reg(data)   (*((volatile unsigned int*) DATA_23_reg)=data)
#define set_DATA_24_reg(data)   (*((volatile unsigned int*) DATA_24_reg)=data)
#define set_DATA_25_reg(data)   (*((volatile unsigned int*) DATA_25_reg)=data)
#define set_DATA_26_reg(data)   (*((volatile unsigned int*) DATA_26_reg)=data)
#define set_DATA_27_reg(data)   (*((volatile unsigned int*) DATA_27_reg)=data)
#define set_DATA_28_reg(data)   (*((volatile unsigned int*) DATA_28_reg)=data)
#define set_DATA_29_reg(data)   (*((volatile unsigned int*) DATA_29_reg)=data)
#define set_DATA_30_reg(data)   (*((volatile unsigned int*) DATA_30_reg)=data)
#define set_DATA_31_reg(data)   (*((volatile unsigned int*) DATA_31_reg)=data)
#define set_DATA_32_reg(data)   (*((volatile unsigned int*) DATA_32_reg)=data)
#define set_DATA_33_reg(data)   (*((volatile unsigned int*) DATA_33_reg)=data)
#define set_DATA_34_reg(data)   (*((volatile unsigned int*) DATA_34_reg)=data)
#define set_DATA_35_reg(data)   (*((volatile unsigned int*) DATA_35_reg)=data)
#define set_DATA_36_reg(data)   (*((volatile unsigned int*) DATA_36_reg)=data)
#define set_DATA_37_reg(data)   (*((volatile unsigned int*) DATA_37_reg)=data)
#define set_DATA_38_reg(data)   (*((volatile unsigned int*) DATA_38_reg)=data)
#define set_DATA_39_reg(data)   (*((volatile unsigned int*) DATA_39_reg)=data)
#define set_DATA_40_reg(data)   (*((volatile unsigned int*) DATA_40_reg)=data)
#define set_DATA_41_reg(data)   (*((volatile unsigned int*) DATA_41_reg)=data)
#define set_DATA_42_reg(data)   (*((volatile unsigned int*) DATA_42_reg)=data)
#define set_DATA_43_reg(data)   (*((volatile unsigned int*) DATA_43_reg)=data)
#define set_DATA_44_reg(data)   (*((volatile unsigned int*) DATA_44_reg)=data)
#define set_DATA_45_reg(data)   (*((volatile unsigned int*) DATA_45_reg)=data)
#define set_DATA_46_reg(data)   (*((volatile unsigned int*) DATA_46_reg)=data)
#define set_DATA_47_reg(data)   (*((volatile unsigned int*) DATA_47_reg)=data)
#define set_DATA_48_reg(data)   (*((volatile unsigned int*) DATA_48_reg)=data)
#define set_DATA_49_reg(data)   (*((volatile unsigned int*) DATA_49_reg)=data)
#define set_DATA_50_reg(data)   (*((volatile unsigned int*) DATA_50_reg)=data)
#define set_DATA_51_reg(data)   (*((volatile unsigned int*) DATA_51_reg)=data)
#define set_DATA_52_reg(data)   (*((volatile unsigned int*) DATA_52_reg)=data)
#define set_DATA_53_reg(data)   (*((volatile unsigned int*) DATA_53_reg)=data)
#define set_DATA_54_reg(data)   (*((volatile unsigned int*) DATA_54_reg)=data)
#define set_DATA_55_reg(data)   (*((volatile unsigned int*) DATA_55_reg)=data)
#define set_DATA_56_reg(data)   (*((volatile unsigned int*) DATA_56_reg)=data)
#define set_DATA_57_reg(data)   (*((volatile unsigned int*) DATA_57_reg)=data)
#define set_DATA_58_reg(data)   (*((volatile unsigned int*) DATA_58_reg)=data)
#define set_DATA_59_reg(data)   (*((volatile unsigned int*) DATA_59_reg)=data)
#define set_DATA_60_reg(data)   (*((volatile unsigned int*) DATA_60_reg)=data)
#define set_DATA_61_reg(data)   (*((volatile unsigned int*) DATA_61_reg)=data)
#define set_DATA_62_reg(data)   (*((volatile unsigned int*) DATA_62_reg)=data)
#define set_DATA_63_reg(data)   (*((volatile unsigned int*) DATA_63_reg)=data)
#define set_DATA_64_reg(data)   (*((volatile unsigned int*) DATA_64_reg)=data)
#define set_DATA_65_reg(data)   (*((volatile unsigned int*) DATA_65_reg)=data)
#define set_DATA_66_reg(data)   (*((volatile unsigned int*) DATA_66_reg)=data)
#define set_DATA_67_reg(data)   (*((volatile unsigned int*) DATA_67_reg)=data)
#define set_DATA_68_reg(data)   (*((volatile unsigned int*) DATA_68_reg)=data)
#define set_DATA_69_reg(data)   (*((volatile unsigned int*) DATA_69_reg)=data)
#define set_DATA_70_reg(data)   (*((volatile unsigned int*) DATA_70_reg)=data)
#define set_DATA_71_reg(data)   (*((volatile unsigned int*) DATA_71_reg)=data)
#define set_DATA_72_reg(data)   (*((volatile unsigned int*) DATA_72_reg)=data)
#define set_DATA_73_reg(data)   (*((volatile unsigned int*) DATA_73_reg)=data)
#define set_DATA_74_reg(data)   (*((volatile unsigned int*) DATA_74_reg)=data)
#define set_DATA_75_reg(data)   (*((volatile unsigned int*) DATA_75_reg)=data)
#define set_DATA_76_reg(data)   (*((volatile unsigned int*) DATA_76_reg)=data)
#define set_DATA_77_reg(data)   (*((volatile unsigned int*) DATA_77_reg)=data)
#define set_DATA_78_reg(data)   (*((volatile unsigned int*) DATA_78_reg)=data)
#define set_DATA_79_reg(data)   (*((volatile unsigned int*) DATA_79_reg)=data)
#define set_DATA_80_reg(data)   (*((volatile unsigned int*) DATA_80_reg)=data)
#define set_DATA_81_reg(data)   (*((volatile unsigned int*) DATA_81_reg)=data)
#define set_DATA_82_reg(data)   (*((volatile unsigned int*) DATA_82_reg)=data)
#define set_DATA_83_reg(data)   (*((volatile unsigned int*) DATA_83_reg)=data)
#define set_DATA_84_reg(data)   (*((volatile unsigned int*) DATA_84_reg)=data)
#define set_DATA_85_reg(data)   (*((volatile unsigned int*) DATA_85_reg)=data)
#define set_DATA_86_reg(data)   (*((volatile unsigned int*) DATA_86_reg)=data)
#define set_DATA_87_reg(data)   (*((volatile unsigned int*) DATA_87_reg)=data)
#define set_DATA_88_reg(data)   (*((volatile unsigned int*) DATA_88_reg)=data)
#define set_DATA_89_reg(data)   (*((volatile unsigned int*) DATA_89_reg)=data)
#define set_DATA_90_reg(data)   (*((volatile unsigned int*) DATA_90_reg)=data)
#define set_DATA_91_reg(data)   (*((volatile unsigned int*) DATA_91_reg)=data)
#define set_DATA_92_reg(data)   (*((volatile unsigned int*) DATA_92_reg)=data)
#define set_DATA_93_reg(data)   (*((volatile unsigned int*) DATA_93_reg)=data)
#define set_DATA_94_reg(data)   (*((volatile unsigned int*) DATA_94_reg)=data)
#define set_DATA_95_reg(data)   (*((volatile unsigned int*) DATA_95_reg)=data)
#define set_DATA_96_reg(data)   (*((volatile unsigned int*) DATA_96_reg)=data)
#define set_DATA_97_reg(data)   (*((volatile unsigned int*) DATA_97_reg)=data)
#define set_DATA_98_reg(data)   (*((volatile unsigned int*) DATA_98_reg)=data)
#define set_DATA_99_reg(data)   (*((volatile unsigned int*) DATA_99_reg)=data)
#define set_DATA_100_reg(data)   (*((volatile unsigned int*) DATA_100_reg)=data)
#define set_DATA_101_reg(data)   (*((volatile unsigned int*) DATA_101_reg)=data)
#define set_DATA_102_reg(data)   (*((volatile unsigned int*) DATA_102_reg)=data)
#define set_DATA_103_reg(data)   (*((volatile unsigned int*) DATA_103_reg)=data)
#define set_DATA_104_reg(data)   (*((volatile unsigned int*) DATA_104_reg)=data)
#define set_DATA_105_reg(data)   (*((volatile unsigned int*) DATA_105_reg)=data)
#define set_DATA_106_reg(data)   (*((volatile unsigned int*) DATA_106_reg)=data)
#define set_DATA_107_reg(data)   (*((volatile unsigned int*) DATA_107_reg)=data)
#define set_DATA_108_reg(data)   (*((volatile unsigned int*) DATA_108_reg)=data)
#define set_DATA_109_reg(data)   (*((volatile unsigned int*) DATA_109_reg)=data)
#define set_DATA_110_reg(data)   (*((volatile unsigned int*) DATA_110_reg)=data)
#define set_DATA_111_reg(data)   (*((volatile unsigned int*) DATA_111_reg)=data)
#define set_DATA_112_reg(data)   (*((volatile unsigned int*) DATA_112_reg)=data)
#define set_DATA_113_reg(data)   (*((volatile unsigned int*) DATA_113_reg)=data)
#define set_DATA_114_reg(data)   (*((volatile unsigned int*) DATA_114_reg)=data)
#define set_DATA_115_reg(data)   (*((volatile unsigned int*) DATA_115_reg)=data)
#define set_DATA_116_reg(data)   (*((volatile unsigned int*) DATA_116_reg)=data)
#define set_DATA_117_reg(data)   (*((volatile unsigned int*) DATA_117_reg)=data)
#define set_DATA_118_reg(data)   (*((volatile unsigned int*) DATA_118_reg)=data)
#define set_DATA_119_reg(data)   (*((volatile unsigned int*) DATA_119_reg)=data)
#define set_DATA_120_reg(data)   (*((volatile unsigned int*) DATA_120_reg)=data)
#define set_DATA_121_reg(data)   (*((volatile unsigned int*) DATA_121_reg)=data)
#define set_DATA_122_reg(data)   (*((volatile unsigned int*) DATA_122_reg)=data)
#define set_DATA_123_reg(data)   (*((volatile unsigned int*) DATA_123_reg)=data)
#define set_DATA_124_reg(data)   (*((volatile unsigned int*) DATA_124_reg)=data)
#define set_DATA_125_reg(data)   (*((volatile unsigned int*) DATA_125_reg)=data)
#define set_DATA_126_reg(data)   (*((volatile unsigned int*) DATA_126_reg)=data)
#define set_DATA_127_reg(data)   (*((volatile unsigned int*) DATA_127_reg)=data)
#define get_DATA_0_reg   (*((volatile unsigned int*) DATA_0_reg))
#define get_DATA_1_reg   (*((volatile unsigned int*) DATA_1_reg))
#define get_DATA_2_reg   (*((volatile unsigned int*) DATA_2_reg))
#define get_DATA_3_reg   (*((volatile unsigned int*) DATA_3_reg))
#define get_DATA_4_reg   (*((volatile unsigned int*) DATA_4_reg))
#define get_DATA_5_reg   (*((volatile unsigned int*) DATA_5_reg))
#define get_DATA_6_reg   (*((volatile unsigned int*) DATA_6_reg))
#define get_DATA_7_reg   (*((volatile unsigned int*) DATA_7_reg))
#define get_DATA_8_reg   (*((volatile unsigned int*) DATA_8_reg))
#define get_DATA_9_reg   (*((volatile unsigned int*) DATA_9_reg))
#define get_DATA_10_reg   (*((volatile unsigned int*) DATA_10_reg))
#define get_DATA_11_reg   (*((volatile unsigned int*) DATA_11_reg))
#define get_DATA_12_reg   (*((volatile unsigned int*) DATA_12_reg))
#define get_DATA_13_reg   (*((volatile unsigned int*) DATA_13_reg))
#define get_DATA_14_reg   (*((volatile unsigned int*) DATA_14_reg))
#define get_DATA_15_reg   (*((volatile unsigned int*) DATA_15_reg))
#define get_DATA_16_reg   (*((volatile unsigned int*) DATA_16_reg))
#define get_DATA_17_reg   (*((volatile unsigned int*) DATA_17_reg))
#define get_DATA_18_reg   (*((volatile unsigned int*) DATA_18_reg))
#define get_DATA_19_reg   (*((volatile unsigned int*) DATA_19_reg))
#define get_DATA_20_reg   (*((volatile unsigned int*) DATA_20_reg))
#define get_DATA_21_reg   (*((volatile unsigned int*) DATA_21_reg))
#define get_DATA_22_reg   (*((volatile unsigned int*) DATA_22_reg))
#define get_DATA_23_reg   (*((volatile unsigned int*) DATA_23_reg))
#define get_DATA_24_reg   (*((volatile unsigned int*) DATA_24_reg))
#define get_DATA_25_reg   (*((volatile unsigned int*) DATA_25_reg))
#define get_DATA_26_reg   (*((volatile unsigned int*) DATA_26_reg))
#define get_DATA_27_reg   (*((volatile unsigned int*) DATA_27_reg))
#define get_DATA_28_reg   (*((volatile unsigned int*) DATA_28_reg))
#define get_DATA_29_reg   (*((volatile unsigned int*) DATA_29_reg))
#define get_DATA_30_reg   (*((volatile unsigned int*) DATA_30_reg))
#define get_DATA_31_reg   (*((volatile unsigned int*) DATA_31_reg))
#define get_DATA_32_reg   (*((volatile unsigned int*) DATA_32_reg))
#define get_DATA_33_reg   (*((volatile unsigned int*) DATA_33_reg))
#define get_DATA_34_reg   (*((volatile unsigned int*) DATA_34_reg))
#define get_DATA_35_reg   (*((volatile unsigned int*) DATA_35_reg))
#define get_DATA_36_reg   (*((volatile unsigned int*) DATA_36_reg))
#define get_DATA_37_reg   (*((volatile unsigned int*) DATA_37_reg))
#define get_DATA_38_reg   (*((volatile unsigned int*) DATA_38_reg))
#define get_DATA_39_reg   (*((volatile unsigned int*) DATA_39_reg))
#define get_DATA_40_reg   (*((volatile unsigned int*) DATA_40_reg))
#define get_DATA_41_reg   (*((volatile unsigned int*) DATA_41_reg))
#define get_DATA_42_reg   (*((volatile unsigned int*) DATA_42_reg))
#define get_DATA_43_reg   (*((volatile unsigned int*) DATA_43_reg))
#define get_DATA_44_reg   (*((volatile unsigned int*) DATA_44_reg))
#define get_DATA_45_reg   (*((volatile unsigned int*) DATA_45_reg))
#define get_DATA_46_reg   (*((volatile unsigned int*) DATA_46_reg))
#define get_DATA_47_reg   (*((volatile unsigned int*) DATA_47_reg))
#define get_DATA_48_reg   (*((volatile unsigned int*) DATA_48_reg))
#define get_DATA_49_reg   (*((volatile unsigned int*) DATA_49_reg))
#define get_DATA_50_reg   (*((volatile unsigned int*) DATA_50_reg))
#define get_DATA_51_reg   (*((volatile unsigned int*) DATA_51_reg))
#define get_DATA_52_reg   (*((volatile unsigned int*) DATA_52_reg))
#define get_DATA_53_reg   (*((volatile unsigned int*) DATA_53_reg))
#define get_DATA_54_reg   (*((volatile unsigned int*) DATA_54_reg))
#define get_DATA_55_reg   (*((volatile unsigned int*) DATA_55_reg))
#define get_DATA_56_reg   (*((volatile unsigned int*) DATA_56_reg))
#define get_DATA_57_reg   (*((volatile unsigned int*) DATA_57_reg))
#define get_DATA_58_reg   (*((volatile unsigned int*) DATA_58_reg))
#define get_DATA_59_reg   (*((volatile unsigned int*) DATA_59_reg))
#define get_DATA_60_reg   (*((volatile unsigned int*) DATA_60_reg))
#define get_DATA_61_reg   (*((volatile unsigned int*) DATA_61_reg))
#define get_DATA_62_reg   (*((volatile unsigned int*) DATA_62_reg))
#define get_DATA_63_reg   (*((volatile unsigned int*) DATA_63_reg))
#define get_DATA_64_reg   (*((volatile unsigned int*) DATA_64_reg))
#define get_DATA_65_reg   (*((volatile unsigned int*) DATA_65_reg))
#define get_DATA_66_reg   (*((volatile unsigned int*) DATA_66_reg))
#define get_DATA_67_reg   (*((volatile unsigned int*) DATA_67_reg))
#define get_DATA_68_reg   (*((volatile unsigned int*) DATA_68_reg))
#define get_DATA_69_reg   (*((volatile unsigned int*) DATA_69_reg))
#define get_DATA_70_reg   (*((volatile unsigned int*) DATA_70_reg))
#define get_DATA_71_reg   (*((volatile unsigned int*) DATA_71_reg))
#define get_DATA_72_reg   (*((volatile unsigned int*) DATA_72_reg))
#define get_DATA_73_reg   (*((volatile unsigned int*) DATA_73_reg))
#define get_DATA_74_reg   (*((volatile unsigned int*) DATA_74_reg))
#define get_DATA_75_reg   (*((volatile unsigned int*) DATA_75_reg))
#define get_DATA_76_reg   (*((volatile unsigned int*) DATA_76_reg))
#define get_DATA_77_reg   (*((volatile unsigned int*) DATA_77_reg))
#define get_DATA_78_reg   (*((volatile unsigned int*) DATA_78_reg))
#define get_DATA_79_reg   (*((volatile unsigned int*) DATA_79_reg))
#define get_DATA_80_reg   (*((volatile unsigned int*) DATA_80_reg))
#define get_DATA_81_reg   (*((volatile unsigned int*) DATA_81_reg))
#define get_DATA_82_reg   (*((volatile unsigned int*) DATA_82_reg))
#define get_DATA_83_reg   (*((volatile unsigned int*) DATA_83_reg))
#define get_DATA_84_reg   (*((volatile unsigned int*) DATA_84_reg))
#define get_DATA_85_reg   (*((volatile unsigned int*) DATA_85_reg))
#define get_DATA_86_reg   (*((volatile unsigned int*) DATA_86_reg))
#define get_DATA_87_reg   (*((volatile unsigned int*) DATA_87_reg))
#define get_DATA_88_reg   (*((volatile unsigned int*) DATA_88_reg))
#define get_DATA_89_reg   (*((volatile unsigned int*) DATA_89_reg))
#define get_DATA_90_reg   (*((volatile unsigned int*) DATA_90_reg))
#define get_DATA_91_reg   (*((volatile unsigned int*) DATA_91_reg))
#define get_DATA_92_reg   (*((volatile unsigned int*) DATA_92_reg))
#define get_DATA_93_reg   (*((volatile unsigned int*) DATA_93_reg))
#define get_DATA_94_reg   (*((volatile unsigned int*) DATA_94_reg))
#define get_DATA_95_reg   (*((volatile unsigned int*) DATA_95_reg))
#define get_DATA_96_reg   (*((volatile unsigned int*) DATA_96_reg))
#define get_DATA_97_reg   (*((volatile unsigned int*) DATA_97_reg))
#define get_DATA_98_reg   (*((volatile unsigned int*) DATA_98_reg))
#define get_DATA_99_reg   (*((volatile unsigned int*) DATA_99_reg))
#define get_DATA_100_reg   (*((volatile unsigned int*) DATA_100_reg))
#define get_DATA_101_reg   (*((volatile unsigned int*) DATA_101_reg))
#define get_DATA_102_reg   (*((volatile unsigned int*) DATA_102_reg))
#define get_DATA_103_reg   (*((volatile unsigned int*) DATA_103_reg))
#define get_DATA_104_reg   (*((volatile unsigned int*) DATA_104_reg))
#define get_DATA_105_reg   (*((volatile unsigned int*) DATA_105_reg))
#define get_DATA_106_reg   (*((volatile unsigned int*) DATA_106_reg))
#define get_DATA_107_reg   (*((volatile unsigned int*) DATA_107_reg))
#define get_DATA_108_reg   (*((volatile unsigned int*) DATA_108_reg))
#define get_DATA_109_reg   (*((volatile unsigned int*) DATA_109_reg))
#define get_DATA_110_reg   (*((volatile unsigned int*) DATA_110_reg))
#define get_DATA_111_reg   (*((volatile unsigned int*) DATA_111_reg))
#define get_DATA_112_reg   (*((volatile unsigned int*) DATA_112_reg))
#define get_DATA_113_reg   (*((volatile unsigned int*) DATA_113_reg))
#define get_DATA_114_reg   (*((volatile unsigned int*) DATA_114_reg))
#define get_DATA_115_reg   (*((volatile unsigned int*) DATA_115_reg))
#define get_DATA_116_reg   (*((volatile unsigned int*) DATA_116_reg))
#define get_DATA_117_reg   (*((volatile unsigned int*) DATA_117_reg))
#define get_DATA_118_reg   (*((volatile unsigned int*) DATA_118_reg))
#define get_DATA_119_reg   (*((volatile unsigned int*) DATA_119_reg))
#define get_DATA_120_reg   (*((volatile unsigned int*) DATA_120_reg))
#define get_DATA_121_reg   (*((volatile unsigned int*) DATA_121_reg))
#define get_DATA_122_reg   (*((volatile unsigned int*) DATA_122_reg))
#define get_DATA_123_reg   (*((volatile unsigned int*) DATA_123_reg))
#define get_DATA_124_reg   (*((volatile unsigned int*) DATA_124_reg))
#define get_DATA_125_reg   (*((volatile unsigned int*) DATA_125_reg))
#define get_DATA_126_reg   (*((volatile unsigned int*) DATA_126_reg))
#define get_DATA_127_reg   (*((volatile unsigned int*) DATA_127_reg))
#define DATA_0_inst_adr                                                              "0x0000"
#define DATA_1_inst_adr                                                              "0x0001"
#define DATA_2_inst_adr                                                              "0x0002"
#define DATA_3_inst_adr                                                              "0x0003"
#define DATA_4_inst_adr                                                              "0x0004"
#define DATA_5_inst_adr                                                              "0x0005"
#define DATA_6_inst_adr                                                              "0x0006"
#define DATA_7_inst_adr                                                              "0x0007"
#define DATA_8_inst_adr                                                              "0x0008"
#define DATA_9_inst_adr                                                              "0x0009"
#define DATA_10_inst_adr                                                              "0x000A"
#define DATA_11_inst_adr                                                              "0x000B"
#define DATA_12_inst_adr                                                              "0x000C"
#define DATA_13_inst_adr                                                              "0x000D"
#define DATA_14_inst_adr                                                              "0x000E"
#define DATA_15_inst_adr                                                              "0x000F"
#define DATA_16_inst_adr                                                              "0x0010"
#define DATA_17_inst_adr                                                              "0x0011"
#define DATA_18_inst_adr                                                              "0x0012"
#define DATA_19_inst_adr                                                              "0x0013"
#define DATA_20_inst_adr                                                              "0x0014"
#define DATA_21_inst_adr                                                              "0x0015"
#define DATA_22_inst_adr                                                              "0x0016"
#define DATA_23_inst_adr                                                              "0x0017"
#define DATA_24_inst_adr                                                              "0x0018"
#define DATA_25_inst_adr                                                              "0x0019"
#define DATA_26_inst_adr                                                              "0x001A"
#define DATA_27_inst_adr                                                              "0x001B"
#define DATA_28_inst_adr                                                              "0x001C"
#define DATA_29_inst_adr                                                              "0x001D"
#define DATA_30_inst_adr                                                              "0x001E"
#define DATA_31_inst_adr                                                              "0x001F"
#define DATA_32_inst_adr                                                              "0x0020"
#define DATA_33_inst_adr                                                              "0x0021"
#define DATA_34_inst_adr                                                              "0x0022"
#define DATA_35_inst_adr                                                              "0x0023"
#define DATA_36_inst_adr                                                              "0x0024"
#define DATA_37_inst_adr                                                              "0x0025"
#define DATA_38_inst_adr                                                              "0x0026"
#define DATA_39_inst_adr                                                              "0x0027"
#define DATA_40_inst_adr                                                              "0x0028"
#define DATA_41_inst_adr                                                              "0x0029"
#define DATA_42_inst_adr                                                              "0x002A"
#define DATA_43_inst_adr                                                              "0x002B"
#define DATA_44_inst_adr                                                              "0x002C"
#define DATA_45_inst_adr                                                              "0x002D"
#define DATA_46_inst_adr                                                              "0x002E"
#define DATA_47_inst_adr                                                              "0x002F"
#define DATA_48_inst_adr                                                              "0x0030"
#define DATA_49_inst_adr                                                              "0x0031"
#define DATA_50_inst_adr                                                              "0x0032"
#define DATA_51_inst_adr                                                              "0x0033"
#define DATA_52_inst_adr                                                              "0x0034"
#define DATA_53_inst_adr                                                              "0x0035"
#define DATA_54_inst_adr                                                              "0x0036"
#define DATA_55_inst_adr                                                              "0x0037"
#define DATA_56_inst_adr                                                              "0x0038"
#define DATA_57_inst_adr                                                              "0x0039"
#define DATA_58_inst_adr                                                              "0x003A"
#define DATA_59_inst_adr                                                              "0x003B"
#define DATA_60_inst_adr                                                              "0x003C"
#define DATA_61_inst_adr                                                              "0x003D"
#define DATA_62_inst_adr                                                              "0x003E"
#define DATA_63_inst_adr                                                              "0x003F"
#define DATA_64_inst_adr                                                              "0x0040"
#define DATA_65_inst_adr                                                              "0x0041"
#define DATA_66_inst_adr                                                              "0x0042"
#define DATA_67_inst_adr                                                              "0x0043"
#define DATA_68_inst_adr                                                              "0x0044"
#define DATA_69_inst_adr                                                              "0x0045"
#define DATA_70_inst_adr                                                              "0x0046"
#define DATA_71_inst_adr                                                              "0x0047"
#define DATA_72_inst_adr                                                              "0x0048"
#define DATA_73_inst_adr                                                              "0x0049"
#define DATA_74_inst_adr                                                              "0x004A"
#define DATA_75_inst_adr                                                              "0x004B"
#define DATA_76_inst_adr                                                              "0x004C"
#define DATA_77_inst_adr                                                              "0x004D"
#define DATA_78_inst_adr                                                              "0x004E"
#define DATA_79_inst_adr                                                              "0x004F"
#define DATA_80_inst_adr                                                              "0x0050"
#define DATA_81_inst_adr                                                              "0x0051"
#define DATA_82_inst_adr                                                              "0x0052"
#define DATA_83_inst_adr                                                              "0x0053"
#define DATA_84_inst_adr                                                              "0x0054"
#define DATA_85_inst_adr                                                              "0x0055"
#define DATA_86_inst_adr                                                              "0x0056"
#define DATA_87_inst_adr                                                              "0x0057"
#define DATA_88_inst_adr                                                              "0x0058"
#define DATA_89_inst_adr                                                              "0x0059"
#define DATA_90_inst_adr                                                              "0x005A"
#define DATA_91_inst_adr                                                              "0x005B"
#define DATA_92_inst_adr                                                              "0x005C"
#define DATA_93_inst_adr                                                              "0x005D"
#define DATA_94_inst_adr                                                              "0x005E"
#define DATA_95_inst_adr                                                              "0x005F"
#define DATA_96_inst_adr                                                              "0x0060"
#define DATA_97_inst_adr                                                              "0x0061"
#define DATA_98_inst_adr                                                              "0x0062"
#define DATA_99_inst_adr                                                              "0x0063"
#define DATA_100_inst_adr                                                              "0x0064"
#define DATA_101_inst_adr                                                              "0x0065"
#define DATA_102_inst_adr                                                              "0x0066"
#define DATA_103_inst_adr                                                              "0x0067"
#define DATA_104_inst_adr                                                              "0x0068"
#define DATA_105_inst_adr                                                              "0x0069"
#define DATA_106_inst_adr                                                              "0x006A"
#define DATA_107_inst_adr                                                              "0x006B"
#define DATA_108_inst_adr                                                              "0x006C"
#define DATA_109_inst_adr                                                              "0x006D"
#define DATA_110_inst_adr                                                              "0x006E"
#define DATA_111_inst_adr                                                              "0x006F"
#define DATA_112_inst_adr                                                              "0x0070"
#define DATA_113_inst_adr                                                              "0x0071"
#define DATA_114_inst_adr                                                              "0x0072"
#define DATA_115_inst_adr                                                              "0x0073"
#define DATA_116_inst_adr                                                              "0x0074"
#define DATA_117_inst_adr                                                              "0x0075"
#define DATA_118_inst_adr                                                              "0x0076"
#define DATA_119_inst_adr                                                              "0x0077"
#define DATA_120_inst_adr                                                              "0x0078"
#define DATA_121_inst_adr                                                              "0x0079"
#define DATA_122_inst_adr                                                              "0x007A"
#define DATA_123_inst_adr                                                              "0x007B"
#define DATA_124_inst_adr                                                              "0x007C"
#define DATA_125_inst_adr                                                              "0x007D"
#define DATA_126_inst_adr                                                              "0x007E"
#define DATA_127_inst_adr                                                              "0x007F"
#define DATA_0_inst                                                                  0x0000
#define DATA_1_inst                                                                  0x0001
#define DATA_2_inst                                                                  0x0002
#define DATA_3_inst                                                                  0x0003
#define DATA_4_inst                                                                  0x0004
#define DATA_5_inst                                                                  0x0005
#define DATA_6_inst                                                                  0x0006
#define DATA_7_inst                                                                  0x0007
#define DATA_8_inst                                                                  0x0008
#define DATA_9_inst                                                                  0x0009
#define DATA_10_inst                                                                  0x000A
#define DATA_11_inst                                                                  0x000B
#define DATA_12_inst                                                                  0x000C
#define DATA_13_inst                                                                  0x000D
#define DATA_14_inst                                                                  0x000E
#define DATA_15_inst                                                                  0x000F
#define DATA_16_inst                                                                  0x0010
#define DATA_17_inst                                                                  0x0011
#define DATA_18_inst                                                                  0x0012
#define DATA_19_inst                                                                  0x0013
#define DATA_20_inst                                                                  0x0014
#define DATA_21_inst                                                                  0x0015
#define DATA_22_inst                                                                  0x0016
#define DATA_23_inst                                                                  0x0017
#define DATA_24_inst                                                                  0x0018
#define DATA_25_inst                                                                  0x0019
#define DATA_26_inst                                                                  0x001A
#define DATA_27_inst                                                                  0x001B
#define DATA_28_inst                                                                  0x001C
#define DATA_29_inst                                                                  0x001D
#define DATA_30_inst                                                                  0x001E
#define DATA_31_inst                                                                  0x001F
#define DATA_32_inst                                                                  0x0020
#define DATA_33_inst                                                                  0x0021
#define DATA_34_inst                                                                  0x0022
#define DATA_35_inst                                                                  0x0023
#define DATA_36_inst                                                                  0x0024
#define DATA_37_inst                                                                  0x0025
#define DATA_38_inst                                                                  0x0026
#define DATA_39_inst                                                                  0x0027
#define DATA_40_inst                                                                  0x0028
#define DATA_41_inst                                                                  0x0029
#define DATA_42_inst                                                                  0x002A
#define DATA_43_inst                                                                  0x002B
#define DATA_44_inst                                                                  0x002C
#define DATA_45_inst                                                                  0x002D
#define DATA_46_inst                                                                  0x002E
#define DATA_47_inst                                                                  0x002F
#define DATA_48_inst                                                                  0x0030
#define DATA_49_inst                                                                  0x0031
#define DATA_50_inst                                                                  0x0032
#define DATA_51_inst                                                                  0x0033
#define DATA_52_inst                                                                  0x0034
#define DATA_53_inst                                                                  0x0035
#define DATA_54_inst                                                                  0x0036
#define DATA_55_inst                                                                  0x0037
#define DATA_56_inst                                                                  0x0038
#define DATA_57_inst                                                                  0x0039
#define DATA_58_inst                                                                  0x003A
#define DATA_59_inst                                                                  0x003B
#define DATA_60_inst                                                                  0x003C
#define DATA_61_inst                                                                  0x003D
#define DATA_62_inst                                                                  0x003E
#define DATA_63_inst                                                                  0x003F
#define DATA_64_inst                                                                  0x0040
#define DATA_65_inst                                                                  0x0041
#define DATA_66_inst                                                                  0x0042
#define DATA_67_inst                                                                  0x0043
#define DATA_68_inst                                                                  0x0044
#define DATA_69_inst                                                                  0x0045
#define DATA_70_inst                                                                  0x0046
#define DATA_71_inst                                                                  0x0047
#define DATA_72_inst                                                                  0x0048
#define DATA_73_inst                                                                  0x0049
#define DATA_74_inst                                                                  0x004A
#define DATA_75_inst                                                                  0x004B
#define DATA_76_inst                                                                  0x004C
#define DATA_77_inst                                                                  0x004D
#define DATA_78_inst                                                                  0x004E
#define DATA_79_inst                                                                  0x004F
#define DATA_80_inst                                                                  0x0050
#define DATA_81_inst                                                                  0x0051
#define DATA_82_inst                                                                  0x0052
#define DATA_83_inst                                                                  0x0053
#define DATA_84_inst                                                                  0x0054
#define DATA_85_inst                                                                  0x0055
#define DATA_86_inst                                                                  0x0056
#define DATA_87_inst                                                                  0x0057
#define DATA_88_inst                                                                  0x0058
#define DATA_89_inst                                                                  0x0059
#define DATA_90_inst                                                                  0x005A
#define DATA_91_inst                                                                  0x005B
#define DATA_92_inst                                                                  0x005C
#define DATA_93_inst                                                                  0x005D
#define DATA_94_inst                                                                  0x005E
#define DATA_95_inst                                                                  0x005F
#define DATA_96_inst                                                                  0x0060
#define DATA_97_inst                                                                  0x0061
#define DATA_98_inst                                                                  0x0062
#define DATA_99_inst                                                                  0x0063
#define DATA_100_inst                                                                  0x0064
#define DATA_101_inst                                                                  0x0065
#define DATA_102_inst                                                                  0x0066
#define DATA_103_inst                                                                  0x0067
#define DATA_104_inst                                                                  0x0068
#define DATA_105_inst                                                                  0x0069
#define DATA_106_inst                                                                  0x006A
#define DATA_107_inst                                                                  0x006B
#define DATA_108_inst                                                                  0x006C
#define DATA_109_inst                                                                  0x006D
#define DATA_110_inst                                                                  0x006E
#define DATA_111_inst                                                                  0x006F
#define DATA_112_inst                                                                  0x0070
#define DATA_113_inst                                                                  0x0071
#define DATA_114_inst                                                                  0x0072
#define DATA_115_inst                                                                  0x0073
#define DATA_116_inst                                                                  0x0074
#define DATA_117_inst                                                                  0x0075
#define DATA_118_inst                                                                  0x0076
#define DATA_119_inst                                                                  0x0077
#define DATA_120_inst                                                                  0x0078
#define DATA_121_inst                                                                  0x0079
#define DATA_122_inst                                                                  0x007A
#define DATA_123_inst                                                                  0x007B
#define DATA_124_inst                                                                  0x007C
#define DATA_125_inst                                                                  0x007D
#define DATA_126_inst                                                                  0x007E
#define DATA_127_inst                                                                  0x007F
#define DATA_datao_shift                                                             (0)
#define DATA_datao_mask                                                              (0xFFFFFFFF)
#define DATA_datao(data)                                                             (0xFFFFFFFF&((data)<<0))
#define DATA_datao_src(data)                                                         ((0xFFFFFFFF&(data))>>0)
#define DATA_get_datao(data)                                                         ((0xFFFFFFFF&(data))>>0)


#define EF_CTRL                                                                      0x18017400
#define EF_CTRL_reg_addr                                                             "0x18017400"
#define EF_CTRL_reg                                                                  0x18017400
#define set_EF_CTRL_reg(data)   (*((volatile unsigned int*) EF_CTRL_reg)=data)
#define get_EF_CTRL_reg   (*((volatile unsigned int*) EF_CTRL_reg))
#define EF_CTRL_inst_adr                                                             "0x0000"
#define EF_CTRL_inst                                                                 0x0000
#define EF_CTRL_write_enable4_shift                                                  (23)
#define EF_CTRL_write_enable4_mask                                                   (0x00800000)
#define EF_CTRL_write_enable4(data)                                                  (0x00800000&((data)<<23))
#define EF_CTRL_write_enable4_src(data)                                              ((0x00800000&(data))>>23)
#define EF_CTRL_get_write_enable4(data)                                              ((0x00800000&(data))>>23)
#define EF_CTRL_en_pro_efuse_shift                                                   (22)
#define EF_CTRL_en_pro_efuse_mask                                                    (0x00400000)
#define EF_CTRL_en_pro_efuse(data)                                                   (0x00400000&((data)<<22))
#define EF_CTRL_en_pro_efuse_src(data)                                               ((0x00400000&(data))>>22)
#define EF_CTRL_get_en_pro_efuse(data)                                               ((0x00400000&(data))>>22)
#define EF_CTRL_write_enable3_shift                                                  (21)
#define EF_CTRL_write_enable3_mask                                                   (0x00200000)
#define EF_CTRL_write_enable3(data)                                                  (0x00200000&((data)<<21))
#define EF_CTRL_write_enable3_src(data)                                              ((0x00200000&(data))>>21)
#define EF_CTRL_get_write_enable3(data)                                              ((0x00200000&(data))>>21)
#define EF_CTRL_auto_rd_req_shift                                                    (20)
#define EF_CTRL_auto_rd_req_mask                                                     (0x00100000)
#define EF_CTRL_auto_rd_req(data)                                                    (0x00100000&((data)<<20))
#define EF_CTRL_auto_rd_req_src(data)                                                ((0x00100000&(data))>>20)
#define EF_CTRL_get_auto_rd_req(data)                                                ((0x00100000&(data))>>20)
#define EF_CTRL_write_enable2_shift                                                  (15)
#define EF_CTRL_write_enable2_mask                                                   (0x00008000)
#define EF_CTRL_write_enable2(data)                                                  (0x00008000&((data)<<15))
#define EF_CTRL_write_enable2_src(data)                                              ((0x00008000&(data))>>15)
#define EF_CTRL_get_write_enable2(data)                                              ((0x00008000&(data))>>15)
#define EF_CTRL_strobe_shift                                                         (14)
#define EF_CTRL_strobe_mask                                                          (0x00004000)
#define EF_CTRL_strobe(data)                                                         (0x00004000&((data)<<14))
#define EF_CTRL_strobe_src(data)                                                     ((0x00004000&(data))>>14)
#define EF_CTRL_get_strobe(data)                                                     ((0x00004000&(data))>>14)
#define EF_CTRL_write_enable1_shift                                                  (13)
#define EF_CTRL_write_enable1_mask                                                   (0x00002000)
#define EF_CTRL_write_enable1(data)                                                  (0x00002000&((data)<<13))
#define EF_CTRL_write_enable1_src(data)                                              ((0x00002000&(data))>>13)
#define EF_CTRL_get_write_enable1(data)                                              ((0x00002000&(data))>>13)
#define EF_CTRL_a_shift                                                              (0)
#define EF_CTRL_a_mask                                                               (0x00001FFF)
#define EF_CTRL_a(data)                                                              (0x00001FFF&((data)<<0))
#define EF_CTRL_a_src(data)                                                          ((0x00001FFF&(data))>>0)
#define EF_CTRL_get_a(data)                                                          ((0x00001FFF&(data))>>0)


#define EF_CTRL_ST                                                                   0x18017404
#define EF_CTRL_ST_reg_addr                                                          "0x18017404"
#define EF_CTRL_ST_reg                                                               0x18017404
#define set_EF_CTRL_ST_reg(data)   (*((volatile unsigned int*) EF_CTRL_ST_reg)=data)
#define get_EF_CTRL_ST_reg   (*((volatile unsigned int*) EF_CTRL_ST_reg))
#define EF_CTRL_ST_inst_adr                                                          "0x0001"
#define EF_CTRL_ST_inst                                                              0x0001
#define EF_CTRL_ST_pro_efuse_act_shift                                               (16)
#define EF_CTRL_ST_pro_efuse_act_mask                                                (0x00010000)
#define EF_CTRL_ST_pro_efuse_act(data)                                               (0x00010000&((data)<<16))
#define EF_CTRL_ST_pro_efuse_act_src(data)                                           ((0x00010000&(data))>>16)
#define EF_CTRL_ST_get_pro_efuse_act(data)                                           ((0x00010000&(data))>>16)
#define EF_CTRL_ST_crc_en_shift                                                      (0)
#define EF_CTRL_ST_crc_en_mask                                                       (0x00000001)
#define EF_CTRL_ST_crc_en(data)                                                      (0x00000001&((data)<<0))
#define EF_CTRL_ST_crc_en_src(data)                                                  ((0x00000001&(data))>>0)
#define EF_CTRL_ST_get_crc_en(data)                                                  ((0x00000001&(data))>>0)


#define EF_CRC                                                                       0x18017408
#define EF_CRC_reg_addr                                                              "0x18017408"
#define EF_CRC_reg                                                                   0x18017408
#define set_EF_CRC_reg(data)   (*((volatile unsigned int*) EF_CRC_reg)=data)
#define get_EF_CRC_reg   (*((volatile unsigned int*) EF_CRC_reg))
#define EF_CRC_inst_adr                                                              "0x0002"
#define EF_CRC_inst                                                                  0x0002
#define EF_CRC_crc_ini_shift                                                         (16)
#define EF_CRC_crc_ini_mask                                                          (0x00FF0000)
#define EF_CRC_crc_ini(data)                                                         (0x00FF0000&((data)<<16))
#define EF_CRC_crc_ini_src(data)                                                     ((0x00FF0000&(data))>>16)
#define EF_CRC_get_crc_ini(data)                                                     ((0x00FF0000&(data))>>16)
#define EF_CRC_crc_out_shift                                                         (0)
#define EF_CRC_crc_out_mask                                                          (0x000000FF)
#define EF_CRC_crc_out(data)                                                         (0x000000FF&((data)<<0))
#define EF_CRC_crc_out_src(data)                                                     ((0x000000FF&(data))>>0)
#define EF_CRC_get_crc_out(data)                                                     ((0x000000FF&(data))>>0)


#define EF_CTRL_NEW                                                                  0x1801740C
#define EF_CTRL_NEW_reg_addr                                                         "0x1801740C"
#define EF_CTRL_NEW_reg                                                              0x1801740C
#define set_EF_CTRL_NEW_reg(data)   (*((volatile unsigned int*) EF_CTRL_NEW_reg)=data)
#define get_EF_CTRL_NEW_reg   (*((volatile unsigned int*) EF_CTRL_NEW_reg))
#define EF_CTRL_NEW_inst_adr                                                         "0x0003"
#define EF_CTRL_NEW_inst                                                             0x0003
#define EF_CTRL_NEW_write_enable2_shift                                              (7)
#define EF_CTRL_NEW_write_enable2_mask                                               (0x00000080)
#define EF_CTRL_NEW_write_enable2(data)                                              (0x00000080&((data)<<7))
#define EF_CTRL_NEW_write_enable2_src(data)                                          ((0x00000080&(data))>>7)
#define EF_CTRL_NEW_get_write_enable2(data)                                          ((0x00000080&(data))>>7)
#define EF_CTRL_NEW_mr_mode_shift                                                    (6)
#define EF_CTRL_NEW_mr_mode_mask                                                     (0x00000040)
#define EF_CTRL_NEW_mr_mode(data)                                                    (0x00000040&((data)<<6))
#define EF_CTRL_NEW_mr_mode_src(data)                                                ((0x00000040&(data))>>6)
#define EF_CTRL_NEW_get_mr_mode(data)                                                ((0x00000040&(data))>>6)
#define EF_CTRL_NEW_write_enable1_shift                                              (4)
#define EF_CTRL_NEW_write_enable1_mask                                               (0x00000010)
#define EF_CTRL_NEW_write_enable1(data)                                              (0x00000010&((data)<<4))
#define EF_CTRL_NEW_write_enable1_src(data)                                          ((0x00000010&(data))>>4)
#define EF_CTRL_NEW_get_write_enable1(data)                                          ((0x00000010&(data))>>4)
#define EF_CTRL_NEW_prog_strobe_pulse_width_shift                                    (0)
#define EF_CTRL_NEW_prog_strobe_pulse_width_mask                                     (0x0000000F)
#define EF_CTRL_NEW_prog_strobe_pulse_width(data)                                    (0x0000000F&((data)<<0))
#define EF_CTRL_NEW_prog_strobe_pulse_width_src(data)                                ((0x0000000F&(data))>>0)
#define EF_CTRL_NEW_get_prog_strobe_pulse_width(data)                                ((0x0000000F&(data))>>0)


#define EF_RD_DONE                                                                   0x18017410
#define EF_RD_DONE_reg_addr                                                          "0x18017410"
#define EF_RD_DONE_reg                                                               0x18017410
#define set_EF_RD_DONE_reg(data)   (*((volatile unsigned int*) EF_RD_DONE_reg)=data)
#define get_EF_RD_DONE_reg   (*((volatile unsigned int*) EF_RD_DONE_reg))
#define EF_RD_DONE_inst_adr                                                          "0x0004"
#define EF_RD_DONE_inst                                                              0x0004
#define EF_RD_DONE_Rd_done_shift                                                     (1)
#define EF_RD_DONE_Rd_done_mask                                                      (0x00000002)
#define EF_RD_DONE_Rd_done(data)                                                     (0x00000002&((data)<<1))
#define EF_RD_DONE_Rd_done_src(data)                                                 ((0x00000002&(data))>>1)
#define EF_RD_DONE_get_Rd_done(data)                                                 ((0x00000002&(data))>>1)
#define EF_RD_DONE_write_data_shift                                                  (0)
#define EF_RD_DONE_write_data_mask                                                   (0x00000001)
#define EF_RD_DONE_write_data(data)                                                  (0x00000001&((data)<<0))
#define EF_RD_DONE_write_data_src(data)                                              ((0x00000001&(data))>>0)
#define EF_RD_DONE_get_write_data(data)                                              ((0x00000001&(data))>>0)


#endif
