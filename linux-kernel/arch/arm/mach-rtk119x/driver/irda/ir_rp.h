
#define VENUS_IR_MAJOR			234
#define VENUS_IR_DEVICE_NUM		1
#define VENUS_IR_MINOR_RP		50
#define VENUS_IR_DEVICE_FILE	"venus_irrp"

#define VENUS_IR_IOC_MAGIC			'r'
#define VENUS_IR_IOC_SET_IRIOTCDP		_IOW(VENUS_IR_IOC_MAGIC, 1, int)
#define VENUS_IR_IOC_FLUSH_IRRP			_IOW(VENUS_IR_IOC_MAGIC, 2, int)
#define VENUS_IR_IOC_SET_PROTOCOL		_IOW(VENUS_IR_IOC_MAGIC, 3, int)
#define VENUS_IR_IOC_SET_DEBOUNCE		_IOW(VENUS_IR_IOC_MAGIC, 4, int)
#define VENUS_IR_IOC_SET_IRPSR			_IOW(VENUS_IR_IOC_MAGIC, 5, int)
#define VENUS_IR_IOC_SET_IRPER			_IOW(VENUS_IR_IOC_MAGIC, 6, int)
#define VENUS_IR_IOC_SET_IRSF			_IOW(VENUS_IR_IOC_MAGIC, 7, int)
#define VENUS_IR_IOC_SET_IRCR			_IOW(VENUS_IR_IOC_MAGIC, 8, int)
#define VENUS_IR_IOC_SET_DRIVER_MODE	_IOW(VENUS_IR_IOC_MAGIC, 9, int)
#define VENUS_IR_IOC_SET_FIRST_REPEAT_DELAY		_IOW(VENUS_IR_IOC_MAGIC, 10, int)
#define VENUS_IRRX_RAW_STOP _IOW(VENUS_IR_IOC_MAGIC, 11, int)
#define VENUS_IR_IOC_MAXNR			11

void	Venus_IR_rp_fifoReset(void);
void	Venus_IR_rp_fifoGet(int* p_received, unsigned int size);
void	Venus_IR_rp_fifoPut( unsigned char * p_regValue, unsigned int size);
unsigned int Venus_IR_rp_fifoGetLen(void);
void Venus_IR_rp_WakeupQueue(void);
int venus_ir_rp_init(void);
void venus_ir_rp_cleanup(void);

