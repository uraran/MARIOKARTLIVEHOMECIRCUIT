/* Realtek supports nand chip types */
/* Micorn */
#define MT29F2G08AAD		0x2CDA8095	//SLC, 256 MB, 1 dies
#define MT29F2G08ABAE		0x2CDA9095	//SLC, 256MB, 1 dies
#define MT29F1G08ABADA		0x2CF18095	//SLC, 128MB, 1 dies
#define MT29F4G08ABADA		0x2CDC9095	//SLC, 512MB, 1 dies
#define MT29F32G08CBACA		0x2C68044A	//MLC, 4GB, 1 dies
#define MT29F64G08CBAAA		0x2C88044B	//MLC, 8GB, 1dies
#define MT29F8G08ABABA		0x2C380026	//Micron 1GB   (SLC single die)
#define MT29F4G08ABAEA		0x2CDC90A6	//Micron 4Gb  (SLC single die)
#define MT29F64G08CBABA		0x2C64444B	//Micron 64G	(MLC) 
#define MT29F32G08CBADA		0x2C44444B	//Micron 32Gb (MLC)

/* STMicorn */
#define NAND01GW3B2B		0x20F1801D	//SLC, 128 MB, 1 dies
#define NAND01GW3B2C		0x20F1001D	//SLC, 128 MB, 1 dies, son of NAND01GW3B2B
#define NAND02GW3B2D		0x20DA1095	//SLC, 256 MB, 1 dies
#define NAND04GW3B2B		0x20DC8095	//SLC, 512 MB, 1 dies
#define NAND04GW3B2D		0x20DC1095	//SLC, 512 MB, 1 dies
#define NAND04GW3C2B		0x20DC14A5	//MLC, 512 MB, 1 dies
#define NAND08GW3C2B		0x20D314A5	//MLC, 1GB, 1 dies

/* Hynix Nand */
#define HY27UF081G2A		0xADF1801D	//SLC, 128 MB, 1 dies
#define HY27UF081G2B		0xADF1801D	//SLC, 128 MB, 1 dies

#define HY27UF082G2A		0xADDA801D	//SLC, 256 MB, 1 dies
#define HY27UF082G2B		0xADDA1095	//SLC, 256 MB, 1 dies
#define HY27UF084G2B		0xADDC1095	//SLC, 512 MB, 1 dies
#define HY27UF084G2M		0xADDC8095	//SLC, 512 MB, 1 dies

/* HY27UT084G2M speed is slower, we have to decrease T1, T2 and T3 */
#define HY27UT084G2M		0xADDC8425	//MLC, 512 MB, 1 dies, BB check at last page, SLOW nand
#define HY27UT084G2A		0xADDC14A5	//MLC, 512 MB, 1 dies
#define H27U4G8T2B		0xADDC14A5	//MLC, 512 MB, 1 dies
#define HY27UT088G2A		0xADD314A5	//MLC, 1GB, 1 dies, BB check at last page
#define HY27UG088G5M		0xADDC8095	//SLC, 1GB, 2 dies
#define HY27UG088G5B		0xADDC1095	//SLC, 1GB, 2 dies
#define H27U8G8T2B		0xADD314B6	//MLC, 1GB, 1 dies, 4K page
#define H27UAG8T2A		0xADD59425	//MLC, 2GB, 1 dies, 4K page
#define H27UAG8T2B		0xADD5949A	//MLC, 2GB, 1 dies, 8K page
#define H27U2G8F2C		0xADDA9095	//SLC, 256 MB, 1 dies, 2K page
#define H27U4G8F2D		0xADDC9095	//SLC, 512MB, 1dies, 2K page
#define H27U1G8F2B		0xADF1001D	//SLC, 128MB, 1dies, 2K page
#define H27UBG8T2A		0xADD7949A	//MLC, 4GB, 1 dies, 8K page
#define H27UBG8T2B		0xADD794DA	//MLC, 4GB, 1 dies, 8K page
#define H27UBG8T2C		0xADD79491	//Hynix 32Gb	(MLC)
#define H27UCG8T2B		0xADDE94EB	//Hynix 64Gb

/* Samsung Nand */
#define K9F1G08U0E		0xECF10095	//SLC, 128 MB, 1 dies
#define K9F1G08U0D		0xECF10015	//SLC, 128 MB, 1 dies
#define K9F2G08U0B		0xECDA1095	//SLC, 256 MB, 1 dies
#define K9G4G08U0A		0xECDC1425	//MLC, 512 MB, 1 dies, BB check at last page
#define K9G4G08U0B		0xECDC14A5	//MLC, 512 MB, 1 dies, BB check at last page
#define K9F4G08U0B		0xECDC1095	//SLC, 512 MB, 1 dies
#define K9G8G08U0A		0xECD314A5	//MLC, 1GB, 1 dies, BB check at last page
#define K9G8G08U0M		0xECD31425	//MLC, 1GB, 1 dies, BB check at last page
#define K9K8G08U0A		0xECD35195	//SLC, 1GB, 1 dies
#define K9F8G08U0M		0xECD301A6	//SLC, 1GB, 1 dies, 4K page
#define K9K8G08U0D		0xECD31195	//SLC, 1GB, 1 dies

/* Toshiba */
#define TC58NVG0S3C		0x98F19095	//128 MB, 1 dies
#define TC58NVG0S3E		0x98D19015	//128 MB, 1 dies
#define TC58NVG1S3C		0x98DA9095	//256 MB, 1 dies
#define TC58NVG1S3E		0x98DA9015	//256 MB, 1 dies
#define TC58NVG2S3E		0x98DC9015	//512 MB, 1 dies
#define TC58NVG5D2F		0x98D79432	//MLC, 4GB, 1 dies, 8K page
#define TC58NVG4D2E		0x98D59432	//MLC, 2GB, 1 dies, 8K page
#define TC58NVG2S0HTA00		0x98DC9026	//SLC,512MB 1 dies,4K page
#define TC58NVG2S0FTA00		0x98D39026	//Toshiba 4Gb  (SLC single die)
#define TC58NVG5D2H		0x98D79432 
#define TC58BVG0S3H		0x98F18015	//SLC, 1GB, 1 dies
#define TC58BVG1S3H		0x98DA9015	//SLC, 2GB, 1 dies
#define TC58NVG0S3H		0x98F18015	//SLC, 1GB, 1 dies
#define TC58NVG1S3H		0x98DA9015	//SLC, 2GB, 1 dies
#define TC58DVG02D5		0x98f10015	//SLC,128MB 1 dies 
#define TC58TEG5DCJT		0x98D78493	//Toshiba 32Gb (MLC)
#define TC58TEG6DCJT		0x98DE8493	//Toshiba 64Gb (MLC)
#define TC58TEG6DDK		0x98DE9493
#define TH58NVG3S0H		0x98D39126

/* Macronix/MXIC */
#define MX30LF4G18AC		0xC2DC9095	//512 MB, 1 dies
#define MX30LF2G18AC		0xC2DA9095	//256 MB, 1 dies
#define MX30LF1G18AC		0xC2F18095	//128 MB, 1 dies
#define MX30LF1G08AM		0xC2F1801D	//128 MB, 1 dies
#define MX30LF1208AA		0xC2F0801D	//64MB, 1 dies
//#define MX30LF2G28AC		0xC2DA9095	//Macronix 2Gb
//#define MX30LF4G28AC		0xC2DC9095	//Macronix 4Gb
#define MX30LF2G28AC		0x9590DAC2	//Macronix 2Gb
#define MX30LF4G28AC		0x9590DCC2	//Macronix 4Gb


#define EN27LN4G08		0xC8DC9095

/* ESMT */
#define F59L1G81A		0x92F18095
#define F59L2G81A		0xC8DA9095
#define F59L4G81A		0xC8DC9095

/*Winbond*/
#define W29N01GV		0xEFF18095
#define W29N02GV		0xEFDA9095
#define W29N04GV		0xEFDC9095

/*MIRA*/
#define PSU2GA30BT		0xC8DA9095
#define PSU4GA30AT		0xC8DC9095

/*Spansion*/
#define S34ML01G1	        0x01F1001D
#define S34ML02G1	        0x01DA9095
#define S34ML04G1	        0x01DC9095  
#define S34ML01G2     		0x01F1801D
#define S34ML02G2	        0x01DA9095 
#define S34ML04G2	        0x01DC9095 

/*Zentel*/
#define A5U2GA31BTS	        0xC8DA9095

/*Power Chip*/
#define ASU1GA30HT		0x92F18095

typedef struct __attribute__ ((__packed__)){
        unsigned char   *name;
        unsigned int    id;
        unsigned int    size;   //nand total size
        unsigned int    chipsize;       //die size
        unsigned int    PageSize;
        unsigned int    BlockSize;
        unsigned short  OobSize;
        unsigned char   num_chips;
        unsigned char   isLastPage;     //page position of block to check BB
        unsigned char   CycleID5th; //If CycleID5th do not exist, set it to 0xff
        unsigned char   CycleID6th; //If CycleID6th do not exist, set it to 0xff
        unsigned short  ecc_num;
        unsigned char   T1;
        unsigned char   T2;
        unsigned char   T3;
        unsigned short  eccSelect;//Ecc ability select:   add by alexchang 0319-2010
} device_type_t;

device_type_t nand_device[] = {
	{"MT29F2G08AAD", MT29F2G08AAD, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"MT29F2G08ABAE", MT29F2G08ABAE, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x6, 0xff, 0xff,0x01, 0x01, 0x01, 0x00}, 	
	{"MT29F1G08ABADA", MT29F1G08ABADA, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff,0x01, 0x01, 0x01, 0x00},	 
	{"MT29F4G08ABADA", MT29F4G08ABADA, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff,0x01, 0x01, 0x01, 0x00},	  	
	{"MT29F8G08ABABA", MT29F8G08ABABA, 0x40000000, 0x40000000, 4096, 128*4096, 224, 1, 0, 0x85, 0xff, 0xff,0x01, 0x01, 0x01, 0x00}, 	 
	{"MT29F4G08ABAEA", MT29F4G08ABAEA, 0x20000000, 0x20000000, 4096, 128*2048, 64, 1, 0, 0x54, 0xff, 0xff,0x01, 0x01, 0x01, 0x0c},	
	{"NAND01GW3B2B", NAND01GW3B2B, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"NAND01GW3B2C", NAND01GW3B2C, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"NAND02GW3B2D", NAND02GW3B2D, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x44, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"NAND04GW3B2B", NAND04GW3B2B, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0x20, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"NAND04GW3B2D", NAND04GW3B2D, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"NAND04GW3C2B", NAND04GW3C2B, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0x24, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"NAND08GW3C2B", NAND08GW3C2B, 0x40000000, 0x40000000, 2048, 128*2048, 64, 1, 1, 0x34, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"HY27UF081G2A", HY27UF081G2A, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"HY27UF081G2B", HY27UF081G2B, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"HY27UF082G2A", HY27UF082G2A, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x00, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"HY27UF082G2B", HY27UF082G2B, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x44, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"HY27UF084G2B", HY27UF084G2B, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"HY27UT084G2A", HY27UT084G2A, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0x24, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"H27U4G8T2B", H27U4G8T2B, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0x24, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"HY27UT088G2A", HY27UT088G2A, 0x40000000, 0x40000000, 2048, 128*2048, 64, 1, 1, 0x34, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"HY27UF084G2M", HY27UF084G2M, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"HY27UT084G2M", HY27UT084G2M, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0xff, 0xff, 0xff, 0x04, 0x04, 0x04, 0x00},
	{"HY27UG088G5M", HY27UG088G5M, 0x40000000, 0x20000000, 2048, 64*2048, 64, 2, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"HY27UG088G5B", HY27UG088G5B, 0x40000000, 0x20000000, 2048, 64*2048, 64, 2, 0, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"H27U8G8T2B", H27U8G8T2B, 0x40000000, 0x40000000, 4096, 128*4096, 128, 1, 1, 0x34, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"H27UAG8T2A", H27UAG8T2A, 0x80000000, 0x80000000, 4096, 128*4096, 224, 1, 1, 0x44, 0x41, 0x18, 0x01, 0x01, 0x01, 0x0c},
	{"H27UAG8T2B", H27UAG8T2B, 0x80000000, 0x80000000, 8192, 256*8192, 448, 1, 1, 0x74, 0x42, 0x18, 0x01, 0x01, 0x01, 0x18},
	{"H27U2G8F2C", H27U2G8F2C, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 1, 0x44, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00}, 	 
	{"H27U4G8F2D", H27U4G8F2D, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 1, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},  	
	{"H27U1G8F2B", H27U1G8F2B, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},  	
	{"K9F1G08U0E", K9F1G08U0E, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0x41, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"K9F1G08U0D", K9F1G08U0D, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0x40, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"K9F2G08U0B", K9F2G08U0B, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x44, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"K9G4G08U0A", K9G4G08U0A, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"K9G4G08U0B", K9G4G08U0B, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"K9F4G08U0B", K9F4G08U0B, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"K9G8G08U0A", K9G8G08U0A, 0x40000000, 0x40000000, 2048, 128*2048, 64, 1, 1, 0x64, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"K9G8G08U0M", K9G8G08U0M, 0x40000000, 0x40000000, 2048, 128*2048, 64, 1, 1, 0x64, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"K9K8G08U0A", K9K8G08U0A, 0x40000000, 0x40000000, 2048, 64*2048, 64, 1, 0, 0x58, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"K9F8G08U0M", K9F8G08U0M, 0x40000000, 0x40000000, 4096, 64*4096, 128, 1, 0, 0x64, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"K9K8G08U0D", K9K8G08U0D, 0x40000000, 0x40000000, 2048, 64*2048, 64, 1, 0, 0x58, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00}, 	
	{"TC58NVG0S3C", TC58NVG0S3C, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"TC58NVG0S3E", TC58NVG0S3E, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0x76, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"TC58NVG1S3C", TC58NVG1S3C, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"TC58NVG2S3E", TC58NVG2S3E, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0x76, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"TC58NVG4D2E", TC58NVG4D2E, 0x80000000, 0x80000000, 8192, 128*8192, 448, 1, 1, 0x76, 0x55, 0x18, 0x01, 0x01, 0x01, 0x18},
	{"TC58NVG2S0HTA00", TC58NVG2S0HTA00, 0x20000000, 0x20000000, 4096, 128*2048, 64, 1, 0, 0x76, 0xff, 0xff, 0x01, 0x01, 0x01, 0x0c},
	{"TC58NVG2S0FTA00", TC58NVG2S0FTA00, 0x20000000, 0x20000000, 4096, 64*4096, 128, 1, 0, 0x76, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"TC58NVG5D2H", TC58NVG5D2H, 0x20000000, 0x20000000, 8192, 128*8192, 80, 1, 0, 0x76, 0x56, 0x18, 0x01, 0x01, 0x01, 0x18},
	{"TC58BVG0S3H", TC58BVG0S3H, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xF2, 0x16, 0xff, 0x01, 0x01, 0x01, 0x0c},
	{"TC58BVG1S3H", TC58BVG1S3H, 0x10000000, 0x10000000, 2048, 128*1024,   64, 1, 0, 0xF6, 0x16, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"TH58NVG3S0H", TH58NVG3S0H, 0x40000000, 0x40000000, 4096, 256*1024, 1280, 1, 0, 0x76, 0xFF, 0xff, 0x01, 0x01, 0x01, 0x01},
	{"TC58NVG0S3H", TC58NVG0S3H, 0x8000000, 0x8000000, 2048, 64*2048, 128, 1, 0, 0x72, 0x16, 0xff, 0x01, 0x01, 0x01, 0x0c},
	{"TC58NVG1S3H", TC58NVG1S3H, 0x10000000, 0x10000000, 2048, 64*2048, 128, 1, 0, 0x76, 0x16, 0xff, 0x01, 0x01, 0x01, 0x0c},
	{"TC58NVG1S3E", TC58NVG1S3E, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x76, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00}, 
	{"TC58DVG02D5", TC58DVG02D5, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},	
	{"W29N01GV", W29N01GV, 0x8000000, 0x8000000, 2048,  64*2048, 64, 1, 0, 0x0, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"W29N02GV", W29N02GV, 0x10000000, 0x10000000, 2048,  64*2048, 64, 1, 0, 0x4, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"W29N04GV", W29N04GV, 0x20000000, 0x20000000, 2048,  64*2048, 64, 1, 0, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"MX30LF4G18AC", MX30LF4G18AC, 0x20000000, 0x20000000, 2048,  64*2048, 64, 1, 0, 0x57, 0x00, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"MX30LF2G18AC", MX30LF2G18AC, 0x10000000, 0x10000000, 2048,  64*2048, 64, 1, 0, 0x07, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"MX30LF1G18AC", MX30LF1G18AC, 0x8000000, 0x8000000, 2048,  64*2048, 64, 1, 0, 0x02, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"MX30LF1G08AM", MX30LF1G08AM, 0x8000000, 0x8000000, 2048,  64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"MX30LF1208AA", MX30LF1208AA, 0x4000000, 0x4000000, 2048,  64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},	
	{"MX30LF2G28AC", MX30LF2G28AC, 0x10000000, 0x10000000, 2048,  64*2048, 64, 1, 0, 0x07, 0xff, 0xff, 0x01, 0x01, 0x01, 0x01},	
	{"MX30LF4G28AC", MX30LF4G28AC, 0x20000000, 0x20000000, 2048,  64*2048, 64, 1, 0, 0x67, 0xff, 0xff, 0x01, 0x01, 0x01, 0x01},	
	{"F59L1G81A", F59L1G81A, 0x8000000, 0x8000000, 2048,  64*2048, 64, 1, 0, 0x40, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},	   
	{"F59L2G81A", F59L2G81A, 0x10000000, 0x10000000, 2048,  64*2048, 64, 1, 0, 0x44, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"EN27LN4G08", EN27LN4G08, 0x20000000, 0x20000000, 2048,  64*2048, 64, 1, 0, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00}, 	
	{"S34ML01G1",S34ML01G1, 0x8000000,  0x8000000,  2048,  64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"S34ML02G1",S34ML02G1, 0x10000000, 0x10000000, 2048,  64*2048, 64, 1, 0, 0x44, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
	{"S34ML04G1",S34ML04G1, 0x20000000, 0x20000000, 2048,  64*2048, 64, 1, 0, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},                
	{"S34ML01G2",S34ML01G2, 0x8000000,  0x8000000,  2048,  64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00},
	{"S34ML02G2",S34ML02G2, 0x10000000, 0x10000000, 2048,  64*2048, 64, 1, 0, 0x46, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00},
	{"S34ML04G2",S34ML04G2, 0x20000000, 0x20000000, 2048,  64*2048, 64, 1, 0, 0x56, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00},
	{"A5U2GA31BTS", A5U2GA31BTS, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00}, 
	{"ASU1GA30HT", ASU1GA30HT, 0x4000000, 0x4000000, 2048,  64*2048, 64, 1, 0, 0x40, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},	 
	{NULL, }
};

