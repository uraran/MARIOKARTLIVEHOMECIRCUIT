#ifndef __GOLDENBOOT_H__
#define __GOLDENBOOT_H__
extern unsigned int __golden_sec_start, __golden_sec_end;

extern unsigned int __golden_magic_1;
extern unsigned int __golden_magic_2;

int is_golden_boot(void) __attribute__((section(".golden_func")));
int set_golden_magic(void) __attribute__((section(".golden_func")));

int start_2nd_bootcode_from_golden(void) __attribute__((section(".golden_func")));
int copy_2nd_bootloader_and_run(char *dst, const char *src, unsigned size) __attribute__((section(".golden_func")));

#endif
