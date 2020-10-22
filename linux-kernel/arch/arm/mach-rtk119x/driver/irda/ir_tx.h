
#define VENUS_IRTX_MAJOR			234
#define VENUS_IRTX_DEVICE_NUM		1
#define VENUS_IRTX_MINOR_RP		51
#define VENUS_IRTX_DEVICE_FILE	"venus_irtx"

#define VENUS_IRTX_IOC_MAGIC			'w'
#define VENUS_IRTX_IOC_SET_TX_TABLE		_IOW(VENUS_IRTX_IOC_MAGIC, 1, int)
#define VENUS_IRTX_IOC_MAXNR			1

void	Venus_irtx_fifoReset(void);
void	Venus_irtx_fifoGet( u32 * p_received, u32 size);
void	Venus_irtx_fifoPut( u32 * p_regValue, u32 size);
unsigned int Venus_irtx_fifoGetLen(void);
unsigned int Venus_irtx_fifoAvail(void);
/*
void Venus_IR_rp_WakeupQueue(void);
int venus_ir_rp_init(void);
void venus_ir_rp_cleanup(void);
*/

int venus_ir_tx_init(void);
void venus_ir_tx_cleanup(void);



