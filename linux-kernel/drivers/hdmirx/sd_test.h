#ifndef SD_TEST_H
#define SD_TEST_H

// if only run kernel to verify MIPI camera , open (TEST_VO_THREAD)define to 1
#define TEST_VO_THREAD 0
#define HDMI_RX_IN 1   // 1: HDMI RX in  0:CAMERA in
#define VO_OUTPUT_COLOR_FMT 0x3 // 2:OUT_YUV422  3:OUT_YUV420
#define CAMERA_480_OUT 0  // 1:480  0: 1080
//#define CRC_CHECK
//#define ReloadMax
#define MIPI_TEST 0
#define MIPI_TEST_1 0
// output_color now, only can choose OUT_YUV422 or OUT_YUV420

// macro definition   
#define Rreg32(Addr)        		*((volatile unsigned int*)(Addr | 0xE6000000))
#define Wreg32(Addr,Value) 		*((volatile unsigned int*)(Addr | 0xE6000000))=Value
#define Rreg8(Addr)         *((volatile char*)(Addr))
#define Wreg8(Addr,Value)  *((volatile char*)(Addr))=Value

void test_VO_480p(unsigned int p_y, unsigned int p_uv);
void test_VO_1080p(unsigned int p_y, unsigned int p_uv);
void sd_test_while_loop(void);
void sd_test_print(void);
void test_HDMI_480p(void);
void test_HDMI_1080p60(void);
void sd_test_vo_detect(void);

#endif
