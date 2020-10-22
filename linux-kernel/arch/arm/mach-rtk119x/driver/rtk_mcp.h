#define CP_REG_BASE	0x18015000 - 0x18000000 + RBUS_BASE_VIRT
#define TP_REG_BASE	0x18014000 - 0x18000000 + RBUS_BASE_VIRT
//#define CP_REG_BASE	0xB8015000

//#define GET_MAPPED_RBUS_ADDR(x)		(x - 0x18000000 + RBUS_BASE_VIRT)

#define  CP_OTP_LOAD				(CP_REG_BASE + 0x19c)

	//for KCPU & ACPU
	/* MCP General Registers */
#define  K_MCP_CTRL					(CP_REG_BASE + 0x900)
#define  K_MCP_STATUS				(CP_REG_BASE + 0x904)
#define  K_MCP_EN					(CP_REG_BASE + 0x908)

	/* MCP Ring-Buffer Registers */
#define  K_MCP_BASE					(CP_REG_BASE + 0x90c)
#define  K_MCP_LIMIT				(CP_REG_BASE + 0x910)
#define  K_MCP_RDPTR				(CP_REG_BASE + 0x914)
#define  K_MCP_WRPTR				(CP_REG_BASE + 0x918)
#define  K_MCP_DES_COUNT			(CP_REG_BASE + 0x934)
#define  K_MCP_DES_COMPARE			(CP_REG_BASE + 0x938)

	/* MCP Ini_Key Registers */
#define  K_MCP_DES_INI_KEY			(CP_REG_BASE + 0x91C)
#define  K_MCP_AES_INI_KEY			(CP_REG_BASE + 0x924)

/* TP registers */
#define	 TP_KEY_INFO_0		(TP_REG_BASE + 0x58)
#define	 TP_KEY_INFO_1		(TP_REG_BASE + 0x5c)
#define  TP_KEY_CTRL		(TP_REG_BASE + 0x60)

#if 1
///////////////////// MACROS /////////////////////////////////

#define SET_TP_KEY_CTRL(x)           writel((x), (volatile unsigned int*) TP_KEY_CTRL)
#define SET_TP_KEYINFO_0(x)         writel((x), (volatile unsigned int*) TP_KEY_INFO_0)
#define SET_TP_KEYINFO_1(x)             writel((x), (volatile unsigned int*) TP_KEY_INFO_1)

#define SET_MCP_CTRL(x)           writel((x), (volatile unsigned int*) K_MCP_CTRL)
#define SET_MCP_STATUS(x)         writel((x), (volatile unsigned int*) K_MCP_STATUS)
#define SET_MCP_EN(x)             writel((x), (volatile unsigned int*) K_MCP_EN)
#define SET_MCP_BASE(x)           writel((x), (volatile unsigned int*) K_MCP_BASE)
#define SET_MCP_LIMIT(x)          writel((x), (volatile unsigned int*) K_MCP_LIMIT)
#define SET_MCP_RDPTR(x)          writel((x), (volatile unsigned int*) K_MCP_RDPTR)
#define SET_MCP_WRPTR(x)          writel((x), (volatile unsigned int*) K_MCP_WRPTR)
//#define SET_MCP_CTRL1(x)          writel((x), (volatile unsigned int*) MCP_CTRL1)       /* for Jupiter only */
#define SET_MCP_OTP_LOAD(x)       writel((x), (volatile unsigned int*) CP_OTP_LOAD)     /* for Jupiter only */

#define GET_MCP_CTRL()            readl((volatile unsigned int*) K_MCP_CTRL)
#define GET_MCP_STATUS()          readl((volatile unsigned int*) K_MCP_STATUS)
#define GET_MCP_EN()              readl((volatile unsigned int*) K_MCP_EN)
#define GET_MCP_BASE()            readl((volatile unsigned int*) K_MCP_BASE)
#define GET_MCP_LIMIT()           readl((volatile unsigned int*) K_MCP_LIMIT)
#define GET_MCP_RDPTR()           readl((volatile unsigned int*) K_MCP_RDPTR)
#define GET_MCP_WRPTR()           readl((volatile unsigned int*) K_MCP_WRPTR)
//#define GET_MCP_CTRL1()           readl((volatile unsigned int*) MCP_CTRL1)       /* for Jupiter only */
#define GET_MCP_OTP_LOAD()        readl((volatile unsigned int*) CP_OTP_LOAD)     /* for Jupiter only */
//#define GET_MCP_AES_INI_KEY0()    readl((volatile unsigned int*) MCP_AES_INI_KEY0)
//#define GET_MCP_AES_INI_KEY1()    readl((volatile unsigned int*) MCP_AES_INI_KEY1)
//#define GET_MCP_AES_INI_KEY2()    readl((volatile unsigned int*) MCP_AES_INI_KEY2)
//#define GET_MCP_AES_INI_KEY3()    readl((volatile unsigned int*) MCP_AES_INI_KEY3)
#endif

#if 0
///////////////////// MACROS /////////////////////////////////

#define SET_MCP_CTRL(x)           writel((x), (void __iomem *) K_MCP_CTRL)
#define SET_MCP_STATUS(x)         writel((x), (void __iomem *) K_MCP_STATUS)
#define SET_MCP_EN(x)             writel((x), (void __iomem *) K_MCP_EN)
#define SET_MCP_BASE(x)           writel((x), (void __iomem *) K_MCP_BASE)
#define SET_MCP_LIMIT(x)          writel((x), (void __iomem *) K_MCP_LIMIT)
#define SET_MCP_RDPTR(x)          writel((x), (void __iomem *) K_MCP_RDPTR)
#define SET_MCP_WRPTR(x)          writel((x), (void __iomem *) K_MCP_WRPTR)
//#define SET_MCP_CTRL1(x)          writel((x), (void __iomem *) MCP_CTRL1)       /* for Jupiter only */
#define SET_MCP_OTP_LOAD(x)       writel((x), (void __iomem *) CP_OTP_LOAD)     /* for Jupiter only */

#define GET_MCP_CTRL()            readl((void __iomem *) K_MCP_CTRL)
#define GET_MCP_STATUS()          readl((void __iomem *) K_MCP_STATUS)
#define GET_MCP_EN()              readl((void __iomem *) K_MCP_EN)
#define GET_MCP_BASE()            readl((void __iomem *) K_MCP_BASE)
#define GET_MCP_LIMIT()           readl((void __iomem *) K_MCP_LIMIT)
#define GET_MCP_RDPTR()           readl((void __iomem *) K_MCP_RDPTR)
#define GET_MCP_WRPTR()           readl((void __iomem *) K_MCP_WRPTR)
//#define GET_MCP_CTRL1()           readl((void __iomem *) MCP_CTRL1)       /* for Jupiter only */
#define GET_MCP_OTP_LOAD()        readl((void __iomem *) CP_OTP_LOAD)     /* for Jupiter only */
//#define GET_MCP_AES_INI_KEY0()    readl((void __iomem *) MCP_AES_INI_KEY0)
//#define GET_MCP_AES_INI_KEY1()    readl((void __iomem **) MCP_AES_INI_KEY1)
//#define GET_MCP_AES_INI_KEY2()    readl((void __iomem *) MCP_AES_INI_KEY2)
//#define GET_MCP_AES_INI_KEY3()    readl((void __iomem *) MCP_AES_INI_KEY3)
#endif

//for IOCTL
#define MCP_DESC_ENTRY_COUNT    64
#define MCP_IOCTL_DO_COMMAND    0x70000001
#define MCP_IOCTL_TEST_AES_H    0x71000001

#define MCP_BCM_ECB          0x0
#define MCP_BCM_CBC          0x1
#define MCP_BCM_CTR          0x2

    #define MCP_WRITE_DATA        (0x01)
    #define MCP_RING_EMPTY        (0x01 <<1)
    #define MCP_ERROR             (0x01 <<2)
    #define MCP_COMPARE           (0x01 <<3)     

    #define MCP_GO                (0x01<<1)
    #define MCP_IDEL              (0x01<<2)
    #define MCP_SWAP              (0x01<<3)
    #define MCP_CLEAR             (0x01<<4)

// Descriptor Definition
#define MARS_MCP_MODE(x)     (x & 0x1F)   

#define MCP_ALGO_DES         0x00
#define MCP_ALGO_3DES        0x01
#define MCP_ALGO_RC4         0x02
#define MCP_ALGO_MD5         0x03
#define MCP_ALGO_SHA_1       0x04
#define MCP_ALGO_AES         0x05
#define MCP_ALGO_AES_G       0x06
#define MCP_ALGO_AES_H       0x07
#define MCP_ALGO_CMAC        0x08

#define MARS_MCP_BCM(x)     ((x & 0x3) << 6)  
#define MCP_BCM_ECB          0x0
#define MCP_BCM_CBC          0x1
#define MCP_BCM_CTR          0x2

#define MARS_MCP_ENC(x)     ((x & 0x1) << 5)  

#define MARS_MCP_KEY_SEL(x) ((x & 0x1) << 12)  
#define MCP_KEY_SEL_OTP      0x1
#define MCP_KEY_SEL_DESC     0x0

#define MARS_MCP_IV_SEL(x)  ((x & 0x1) << 11)  
#define MCP_IV_SEL_REG      0x1
#define MCP_IV_SEL_DESC     0x0

#define MCP_AES_ECB_Decryption(key, p_in, p_out, len)       MCP_AES_Decryption(MCP_BCM_ECB, key, NULL, p_in, p_out, len)
#define MCP_AES_ECB_Encryption(key, p_in, p_out, len)       MCP_AES_Encryption(MCP_BCM_ECB, key, NULL, p_in, p_out, len)

#define mcp_dump_data_with_text(data, len ,fmt, args...)  do {\
                                                            printk(fmt, ## args);\
                                                            mcp_dump_mem(data, len);\
                                                        }while(0)       

//==================================== Debug ===================================
#define MCP_DEBUG_EN
#ifdef MCP_DEBUG_EN
#define mcp_debug(fmt, args...)         printk("[MCP] Debug, " fmt, ## args)
#else
#define mcp_debug(fmt, args...)
#endif

#define mcp_info(fmt, args...)          printk("[MCP] Info, " fmt, ## args)
#define mcp_warning(fmt, args...)       printk("[MCP] Warning, " fmt, ## args)

typedef struct 
{
    unsigned long flags;    
    unsigned long key[6];  
    unsigned long iv[4];  
    unsigned long data_in;      // data in : physical address
    unsigned long data_out;     // data out : physical address 
    unsigned long length;       // data length
}mcp_desc;

typedef struct 
{
    mcp_desc*     p_desc;
    unsigned long n_desc;    
}mcp_desc_set;

