#ifndef __CUSTOMIZED_H_
#define __CUSTOMIZED_H_	

#include <customized_feature.h>

#define EXECUTE_CUSTOMIZED_FUNCTION	  	env_handler_customized("set Let env");
#define EXECUTE_CUSTOMIZED_FUNCTION_1 	({int __ret=0; __ret = detect_recovery_flag("Check Let Recovery"); __ret; })
#define EXECUTE_CUSTOMIZED_FUNCTION_2	rtk_plat_boot_prep_version();	
#define EXECUTE_CUSTOMIZED_FUNCTION_3(args...)	rtk_modify_dtb(args);
#define EXECUTE_CUSTOMIZED_FUNCTION_4(args...)	({int __ret=1; __ret = rtk_plat_read_all_image_from_eMMC(args); __ret; })

					
										
void env_handler_customized(char *str);
int detect_recovery_flag(char *str);
int rtk_plat_boot_prep_version(void);
int rtk_modify_dtb(int type,enum RTK_DTB_PATH path,int target_addr,int size);
int rtk_plat_read_all_image_from_eMMC(uint fw_desc_table_base, part_desc_entry_v1_t* part_entry, int part_count,
									  void* fw_entry, int fw_count,uchar version);

#ifdef CONFIG_CUSTOMIZED_LOGO
#define use_customize_logo    1
int rtk_plat_load_customize_logo_emmc(void* fw_entry, int fw_count,uchar version);
#else
#define use_customize_logo    0
static inline int rtk_plat_load_customize_logo_emmc(void* fw_entry, int fw_count,uchar version) {return 1;}
#endif

#endif	/* __CUSTOMIZED_H_ */
