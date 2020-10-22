#ifndef  __RTD1195_LSADC_H_
#define  __RTD1195_LSADC_H_
#ifdef __cplusplus
extern "C" {
#endif

#define MIS_ISR_REG_VADDR					(0x1801B00C)
#define MIS_ISR_MASK_LSADC_INT				(0x00200000)


#define LSADC_ANALOG_CTRL_VALUE 			(0x00011101)
#define LSADC_STATUS_ENABLE_IQR 			(0x03000000)
#define LSADC_CTRL_DEBOUNCE_CNT				(0x00800000)
#define LSADC_CTRL_DEBOUNCE_MASK			(0x00F00000)

#define LSADC_CTRL_MASK_ENABLE				(0x00000001)
#define LSADC_CTRL_MASK_SEL_WAIT			(0x80000000)

#define LSADC_PAD0_VADDR                    (0x1801bc00)
#define LSADC_PAD1_VADDR                    (0x1801bc04)

#define LSADC_CTRL_VADDR             	    (0x1801bc20)
#define LSADC_STATUS_VADDR           	    (0x1801bc24)
#define LSADC_ANALOG_CTRL_VADDR      		(0x1801bc28)
//#define LSADC_PERI_TOP_DEBUG_VADDR      	(0x1801bc2c)

#define LSADC_PAD0_LEVEL_SET0_VADDR         (0x1801bc30)
#define LSADC_PAD0_LEVEL_SET1_VADDR         (0x1801bc34)
#define LSADC_PAD0_LEVEL_SET2_VADDR         (0x1801bc38)
#define LSADC_PAD0_LEVEL_SET3_VADDR         (0x1801bc3c)
#define LSADC_PAD0_LEVEL_SET4_VADDR         (0x1801bc40)
#define LSADC_PAD0_LEVEL_SET5_VADDR         (0x1801bc44)

#define LSADC_PAD1_LEVEL_SET0_VADDR         (0x1801bc48)
#define LSADC_PAD1_LEVEL_SET1_VADDR         (0x1801bc4c)
#define LSADC_PAD1_LEVEL_SET2_VADDR         (0x1801bc50)
#define LSADC_PAD1_LEVEL_SET3_VADDR         (0x1801bc54)
#define LSADC_PAD1_LEVEL_SET4_VADDR         (0x1801bc58)
#define LSADC_PAD1_LEVEL_SET5_VADDR         (0x1801bc5c)

//#define LSADC_INT_PAD0_VADDR                (0x1801bc78)
//#define LSADC_INT_PAD1_VADDR                (0x1801bc7c)

//// define LSADC_STATUS MASK
#define LSADC_STATUS_MASK_PAD0_STATUS		(0x00000001)
#define LSADC_STATUS_MASK_PAD1_STATUS		(0x00000002)
#define LSADC_STATUS_MASK_PAD_CTRL_0		(0x00001000)
#define LSADC_STATUS_MASK_PAD_CTRL_4		(0x00010000)
#define LSADC_STATUS_MASK_ADC_BUSY			(0x00080000)
#define LSADC_STATUS_MASK_PAD_CNT			(0x00F00000)
#define LSADC_STATUS_MASK_IRQ_EN			(0x30000000)

#define LSADC_PAD_MASK_ACTIVE				(0x80000000)
#define LSADC_PAD_MASK_THRESHOLD			(0x00FF0000)
#define LSADC_PAD_MASK_SW					(0x00001000)
#define LSADC_PAD_MASK_CTRL					(0x00000100)
#define LSADC_PAD_MASK_ADC_VAL				(0x0000003F)


#ifndef LITTLE_ENDIAN	// apply BIG_ENDIAN
typedef union
{
	unsigned int     regValue;
	struct
	{
		unsigned int     sel_wait:4;
		unsigned int     sel_adc_ck:4;
		unsigned int     debounce_cnt:4;
		unsigned int     reserved_0:4;
		unsigned int     dout_test_in:8;
		unsigned int     reserved_1:6;
		unsigned int     test_en:1;
		unsigned int     enable:1;
	};
} ST_LSADC_ctrl;

typedef union
{
	unsigned int     regValue;
	struct
	{
		unsigned int     reserved_0:14;
		unsigned int     jd_sbias:2;
		unsigned int     reserved_1:2;
		unsigned int     jd_adsbias:2;
		unsigned int     jd_dummy:2;
		unsigned int     reserved_2:1;
		unsigned int     jd_svr:1;
		unsigned int     reserved_3:3;
		unsigned int     jd_adcksel:1;
		unsigned int     reserved_4:3;
		unsigned int     jd_power:1;
	};
} ST_LSADC_analog_ctrl;

typedef union
{
	unsigned int		regValue;
	struct
	{
		unsigned int     pad_active:1;
		unsigned int     reserved_0:7;
		unsigned int     pad_thred:8;
		unsigned int     pad_sw:4;
		unsigned int     reserved_1:3;
		unsigned int     pad_ctrl:1;
		unsigned int     reserved_2:2;
		unsigned int     adc_val:6;
	};
} ST_LSADC_pad;

typedef union
{
	unsigned int		regValue;
	struct
	{
		unsigned int     irq_en:8;
		unsigned int     pad_cnt:4;
		unsigned int     adc_busy:1;
		unsigned int     reserved_0:2;
		unsigned int     pad_ctrl:5;
		unsigned int     reserved_1:10;
		unsigned int     pad1_status:1;
		unsigned int     pad0_status:1;
	};
} ST_LSADC_status;


#else	// apply LITTLE_ENDIAN

typedef union
{
	unsigned int     regValue;
	struct
	{
		unsigned int     enable:1;
		unsigned int     test_en:1;
		unsigned int     reserved_1:6;
		unsigned int     dout_test_in:8;
		unsigned int     reserved_0:4;
		unsigned int     debounce_cnt:4;
		unsigned int     sel_adc_ck:4;
		unsigned int     sel_wait:4;
	};
} ST_LSADC_ctrl;

typedef union
{
	unsigned int     regValue;
	struct
	{
		unsigned int     jd_power:1;
		unsigned int     reserved_4:3;
		unsigned int     jd_adcksel:1;
		unsigned int     reserved_3:3;
		unsigned int     jd_svr:1;
		unsigned int     reserved_2:1;
		unsigned int     jd_dummy:2;
		unsigned int     jd_adsbias:2;
		unsigned int     reserved_1:2;
		unsigned int     jd_sbias:2;
		unsigned int     reserved_0:14;
	};
} ST_LSADC_analog_ctrl;

typedef union
{
	unsigned int     regValue;
	struct
	{
		unsigned int     adc_val:6;
		unsigned int     reserved_2:2;
		unsigned int     pad_ctrl:1;
		unsigned int     reserved_1:3;
		unsigned int     pad_sw:4;
		unsigned int     pad_thred:8;
		unsigned int     reserved_0:7;
		unsigned int     pad_active:1;
	};
} ST_LSADC_pad;

typedef union
{
	unsigned int		regValue;
	struct
	{
		unsigned int     pad0_status:1;
		unsigned int     pad1_status:1;
		unsigned int     reserved_1:10;
		unsigned int     pad_ctrl:5;
		unsigned int     reserved_0:2;
		unsigned int     adc_busy:1;
		unsigned int     pad_cnt:4;
		unsigned int     irq_en:8;
	};
} ST_LSADC_status;



#endif

extern int lsadc_init(int index, int voltage_mode);
extern int lsdac_get(int index);

#ifdef __cplusplus
}
#endif

#endif//__RTD1195_LSADC_H_

