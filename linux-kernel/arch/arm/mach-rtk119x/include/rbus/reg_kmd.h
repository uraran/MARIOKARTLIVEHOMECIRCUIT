//Copyright (C) 2007-2015 Realtek Semiconductor Corporation.

#ifndef __REG_MD_H
#define __REG_MD_H

#define MD_COMMAND_ENTRIES      4096
#define INST_CNT_MASK           0xFFFF
#define INST_CNT_MASK_L         0xFFFFLL

// 0xFE00B000
#define MD_REG_BASE             (0x1800B000 - RBUS_BASE_PHYS + RBUS_BASE_VIRT)

#define SMQ_REG_BASE            MD_REG_BASE
#define VMQ_REG_BASE            (MD_REG_BASE + 0x30)
#define KMQ_REG_BASE            (MD_REG_BASE + 0x60)

// FDMA
#define MD_FDMA_DDR_SADDR_reg   (MD_REG_BASE + 0x88)
#define MD_FDMA_FL_SADDR_reg    (MD_REG_BASE + 0x8C)
#define MD_FDMA_CTRL2_reg       (MD_REG_BASE + 0x90)
#define MD_FDMA_CTRL1_reg       (MD_REG_BASE + 0x94)

#define MD_POWER_reg            (MD_REG_BASE + 0x128)

// command opcode
#define MOVE_DATA_MD_1B         0x05
#define MOVE_DATA_MD_2B         0x07
#define MOVE_DATA_MD_4B         0x0D

#define MOVE_DATA_SS            0x00
#define MOVE_DATA_SB            0x10
#define MOVE_DATA_BS            0x20
#define MOVE_DATA_SS_PITCH      0x30

#define SWAP_DATA_2B_SS        (MOVE_DATA_SS|MOVE_DATA_MD_2B)
#define SWAP_DATA_2B_SS_PITCH  (MOVE_DATA_SS_PITCH|MOVE_DATA_MD_2B)

#define SWAP_DATA_4B_SS        (MOVE_DATA_SS|MOVE_DATA_MD_4B)
#define SWAP_DATA_4B_SS_PITCH  (MOVE_DATA_SS_PITCH|MOVE_DATA_MD_4B)

#define BIT0                    0x01
#define BIT1                    0x02
#define BIT2                    0x04
#define BIT3                    0x08
#define BIT4                    0x10
#define BIT5                    0x20
#define BIT6                    0x40
#define BIT7                    0x80

#define MQ_CLR_WRITE_DATA      0
#define MQ_SET_WRITE_DATA      1

typedef enum {
    MQ_GO=BIT1,
    MQ_CMD_SWAP=BIT2,
    MQ_IDLE=BIT3,
} MD_CTRL;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:28;
unsigned int     mq_idle:1;
unsigned int     mq_cmd_swap:1;
unsigned int     mq_go:1;
unsigned int     write_data:1;
} Fields;
}MD_SMQ_CNTL;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:16;
unsigned int     gsp_length_err:1;
unsigned int     gsp_rx_thd_ints:1;
unsigned int     gsp_txdat_thd_ints:1;
unsigned int     gsp_txbasic_thd_ints:1;
unsigned int     gsp_tx_thd_ints:1;
unsigned int     mq_chksum_err:1;
unsigned int     ur_length_err:1;
unsigned int     ur_rx_timeout:1;
unsigned int     ur_rx_thd:1;
unsigned int     ur_tx_thd:1;
unsigned int     mq_mode_2b4berr:1;
unsigned int     fdma_done:1;
unsigned int     mq_empty:1;
unsigned int     mq_length_err:1;
unsigned int     mq_inst_err:1;
unsigned int     write_data:1;
} Fields;
}MD_SMQ_INT_STATUS;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:15;
unsigned int     scpu_int_en:1;
unsigned int     gsp_length_err_en:1;
unsigned int     gsp_rx_thd_en:1;
unsigned int     gsp_txdat_thd_en:1;
unsigned int     gsp_txbasic_thd_en:1;
unsigned int     gsp_tx_thd_en:1;
unsigned int     mq_chksum_err_en:1;
unsigned int     ur_length_err_en:1;
unsigned int     ur_rx_timeout_en:1;
unsigned int     ur_rx_thd_en:1;
unsigned int     ur_tx_thd_en:1;
unsigned int     mq_mode_2b4b_err_en:1;
unsigned int     fdma_done_en:1;
unsigned int     mq_empty_en:1;
unsigned int     mq_length_err_en:1;
unsigned int     mq_inst_err_en:1;
unsigned int     write_data:1;
} Fields;
}MD_SMQ_INT_ENABLE;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     mq_base:27;
unsigned int     reserved_1:4;
} Fields;
}MD_SMQBASE;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     mq_limit:27;
unsigned int     reserved_1:4;
} Fields;
}MD_SMQLIMIT;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     mq_rdptr:27;
unsigned int     reserved_1:4;
} Fields;
}MD_SMQRDPTR;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     mq_wrptr:27;
unsigned int     reserved_1:4;
} Fields;
}MD_SMQWRPTR;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:27;
unsigned int     mq_inst_remain:5;
} Fields;
}MD_SMQFIFOSTATE;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:16;
unsigned int     mq_inst_cnt:16;
} Fields;
}MD_SMQ_INSTCNT;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     mq_chksum:32;
} Fields;
}MD_SMQ_CHKSUM;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:28;
unsigned int     mq_idle:1;
unsigned int     mq_cmd_swap:1;
unsigned int     mq_go:1;
unsigned int     write_data:1;
} Fields;
}MD_VMQ_CNTL;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:26;
unsigned int     mq_chksum_err:1;
unsigned int     mq_mode_2b4berr:1;
unsigned int     mq_empty:1;
unsigned int     mq_length_err:1;
unsigned int     mq_inst_err:1;
unsigned int     write_data:1;
} Fields;
}MD_VMQ_INT_STATUS;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:25;
unsigned int     vcpu_int_en:1;
unsigned int     mq_chksum_err_en:1;
unsigned int     mq_mode_2b4b_err_en:1;
unsigned int     mq_empty_en:1;
unsigned int     mq_length_err_en:1;
unsigned int     mq_inst_err_en:1;
unsigned int     write_data:1;
} Fields;
}MD_VMQ_INT_ENABLE;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     mq_base:27;
unsigned int     reserved_1:4;
} Fields;
}MD_VMQBASE;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     mq_limit:27;
unsigned int     reserved_1:4;
} Fields;
}MD_VMQLIMIT;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     mq_rdptr:27;
unsigned int     reserved_1:4;
} Fields;
}MD_VMQRDPTR;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     mq_wrptr:27;
unsigned int     reserved_1:4;
} Fields;
}MD_VMQWRPTR;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:27;
unsigned int     mq_inst_remain:5;
} Fields;
}MD_VMQFIFOSTATE;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:16;
unsigned int     mq_inst_cnt:16;
} Fields;
}MD_VMQ_INSTCNT;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     mq_chksum:32;
} Fields;
}MD_VMQ_CHKSUM;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:28;
unsigned int     mq_idle:1;
unsigned int     mq_cmd_swap:1;
unsigned int     mq_go:1;
unsigned int     write_data:1;
} Fields;
}MD_KMQ_CNTL;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:26;
unsigned int     mq_chksum_err:1;
unsigned int     mq_mode_2b4berr:1;
unsigned int     mq_empty:1;
unsigned int     mq_length_err:1;
unsigned int     mq_inst_err:1;
unsigned int     write_data:1;
} Fields;
}MD_KMQ_INT_STATUS;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:25;
unsigned int     kcpu_int_en:1;
unsigned int     mq_chksum_err_en:1;
unsigned int     mq_mode_2b4b_err_en:1;
unsigned int     mq_empty_en:1;
unsigned int     mq_length_err_en:1;
unsigned int     mq_inst_err_en:1;
unsigned int     write_data:1;
} Fields;
}MD_KMQ_INT_ENABLE;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     mq_base:27;
unsigned int     reserved_1:4;
} Fields;
}MD_KMQBASE;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     mq_limit:27;
unsigned int     reserved_1:4;
} Fields;
}MD_KMQLIMIT;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     mq_rdptr:27;
unsigned int     reserved_1:4;
} Fields;
}MD_KMQRDPTR;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     mq_wrptr:27;
unsigned int     reserved_1:4;
} Fields;
}MD_KMQWRPTR;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:27;
unsigned int     mq_inst_remain:5;
} Fields;
}MD_KMQFIFOSTATE;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:16;
unsigned int     mq_inst_cnt:16;
} Fields;
}MD_KMQ_INSTCNT;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     mq_chksum:32;
} Fields;
}MD_KMQ_CHKSUM;

// SMQ
typedef struct _MD_SMQ_INFO
{
    MD_SMQ_CNTL         MdCtrl[1];          //0x1800_B000: Register: CTRL
    MD_SMQ_INT_STATUS   MdInts[1];          //0x1800_B004: Register: INTS
    MD_SMQ_INT_ENABLE   MdInte[1];          //0x1800_B008: Register: INTE
    MD_SMQBASE          MdCmdBase[1];       //0x1800_B00C: Register: CMDBASE
    MD_SMQLIMIT         MdCmdLimit[1];      //0x1800_B010: Register: MDREG_CMDLMT
    MD_SMQRDPTR         MdCmdReadPtr[1];    //0x1800_B014: Register: CMDRPTR
    MD_SMQWRPTR         MdCmdWritePtr[1];   //0x1800_B018: Register: CMDWPTR
    MD_SMQFIFOSTATE     MdFifoState[1];     //0x1800_B01C: Register: FIFOSTATE
    MD_SMQ_INSTCNT      MdInstCnt[1];       //0x1800_B020: Register: INSTCNT
    MD_SMQ_CHKSUM       MdChksum[1];        //0x1800_B024: Register: CHKSUM
    unsigned int        dummy[2];
} MD_SMQ_INFO;

// VMQ
typedef struct _MD_VMQ_INFO
{
    MD_VMQ_CNTL         MdCtrl[1];          //0x1800_B030: Register: CTRL
    MD_VMQ_INT_STATUS   MdInts[1];          //0x1800_B034: Register: INTS
    MD_VMQ_INT_ENABLE   MdInte[1];          //0x1800_B038: Register: INTE
    MD_VMQBASE          MdCmdBase[1];       //0x1800_B03C: Register: CMDBASE
    MD_VMQLIMIT         MdCmdLimit[1];      //0x1800_B040: Register: MDREG_CMDLMT
    MD_VMQRDPTR         MdCmdReadPtr[1];    //0x1800_B044: Register: CMDRPTR
    MD_VMQWRPTR         MdCmdWritePtr[1];   //0x1800_B048: Register: CMDWPTR
    MD_VMQFIFOSTATE     MdFifoState[1];     //0x1800_B04C: Register: FIFOSTATE
    MD_VMQ_INSTCNT      MdInstCnt[1];       //0x1800_B050: Register: INSTCNT
    MD_VMQ_CHKSUM       MdChksum[1];        //0x1800_B054: Register: CHKSUM
    unsigned int        dummy[2];
} MD_VMQ_INFO;

// KMQ
typedef struct _MD_KMQ_INFO
{
    MD_KMQ_CNTL         MdCtrl[1];          //0x1800_B060: Register: CTRL
    MD_KMQ_INT_STATUS   MdInts[1];          //0x1800_B064: Register: INTS
    MD_KMQ_INT_ENABLE   MdInte[1];          //0x1800_B068: Register: INTE
    MD_KMQBASE          MdCmdBase[1];       //0x1800_B06C: Register: CMDBASE
    MD_KMQLIMIT         MdCmdLimit[1];      //0x1800_B070: Register: MDREG_CMDLMT
    MD_KMQRDPTR         MdCmdReadPtr[1];    //0x1800_B074: Register: CMDRPTR
    MD_KMQWRPTR         MdCmdWritePtr[1];   //0x1800_B078: Register: CMDWPTR
    MD_KMQFIFOSTATE     MdFifoState[1];     //0x1800_B07C: Register: FIFOSTATE
    MD_KMQ_INSTCNT      MdInstCnt[1];       //0x1800_B080: Register: INSTCNT
    MD_KMQ_CHKSUM       MdChksum[1];        //0x1800_B084: Register: CHKSUM
    unsigned int        dummy[2];
} MD_KMQ_INFO;

// MD control registers for Move Queue(MQ)
struct _MD_REG_INFO
{
    MD_SMQ_INFO         MdSMQ[1];           //0x1800_B000: SMQ
    MD_VMQ_INFO         MdVMQ[1];           //0x1800_B030: VMQ
    MD_KMQ_INFO         MdKMQ[1];           //0x1800_B060: KMQ
} MD_REG_INFO;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:17;
unsigned int     md_dbg_sel1:7;
unsigned int     md_dbg_sel0:7;
unsigned int     md_dbg_enable:1;
} Fields;
}MD_DBG;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     fdma_ddr_saddr:31;
} Fields;
}MD_FDMA_DDR_SADDR;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:7;
unsigned int     fdma_fl_saddr:25;
} Fields;
}MD_FDMA_FL_SADDR;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:1;
unsigned int     fl_cp_en:1;
unsigned int     fl_mapping_mode:1;
unsigned int     fdma_swap:1;
unsigned int     fdma_max_xfer:2;
unsigned int     fdma_dir:1;
unsigned int     fdma_length:25;
} Fields;
}MD_FDMA_CTRL2;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:28;
unsigned int     write_en2:1;
unsigned int     fdma_stop:1;
unsigned int     write_en1:1;
unsigned int     fdma_go:1;
} Fields;
}MD_FDMA_CTRL1;

typedef union
{
uint32_t  Value;
struct
{
unsigned int     reserved_0:31;
unsigned int     gating_en:1;
} Fields;
}MD_POWER;

#endif
