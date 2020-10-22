#include "../flash_writer_u/include/project_config_f.h"
#include "dvrboot_inc/util.h"
#include "dvrboot_inc/string.h"
#include "dvrboot_inc/otp_reg.h"
#include "dvrboot_inc/io.h"
#include "print.h"
#include <command.h>

//#define DEBUG_PRT
//#define EP_GDB_MSG

extern unsigned char rsa_pub_key[];
extern unsigned char rsa_pub_key_end;
extern unsigned char aes_key[];
extern unsigned char aes_key_end;
extern unsigned char aes_seed[];
extern unsigned char aes_seed_end;
extern unsigned char pocono_key[];
extern unsigned char pocono_key_end;
extern unsigned char user_part_key[];
extern unsigned char user_part_key_end;
extern unsigned char cust_data[];
extern unsigned char cust_data_end;

__align__ unsigned char dec_array[32*2]={0};
__align__ unsigned char v_array[1024]={0};
__align__ unsigned char otp_retry_cnt=0;

/************************************************************************
 *  Public variables
 ************************************************************************/

int printf(const char *fmt, ...);
int obfuse_getset(int type, unsigned char* enc_array,unsigned int ofset,unsigned int len);

//-----------------------------------------------------------------------------
// dummy function
//-----------------------------------------------------------------------------
int ctrlc()
{
	return 0;
}

int printf(const char *fmt, ...)
{
	return 1;
}
//-----------------------------------------------------------------------------
void* memcpy(void* dest, const void* src, size_t count) {
    char* dst8 = (char*)dest;
    char* src8 = (char*)src;

    while (count--) {
        *dst8++ = *src8++;
    }
    return dest;
}

/**
 *  * memcmp - Compare two areas of memory
 *   * @cs: One area of memory
 *    * @ct: Another area of memory
 *     * @count: The size of the area.
 *      */
int memcmp(const void * cs,const void * ct,size_t count)
{
        const unsigned char *su1, *su2;
        int res = 0;

        for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
                if ((res = *su1 - *su2) != 0)
                        break;
        return res;
}
//-----------------------------------------------------------------------------
int do_program(void)
{
 	UINT32	rsa_pub_key_size  = (unsigned int )(&rsa_pub_key_end  - rsa_pub_key);
    	UINT32	aes_key_size = (unsigned int )(&aes_key_end - aes_key);
    	UINT32	aes_seed_size = (unsigned int )(&aes_seed_end - aes_seed);
	UINT32  pocono_key_size = (unsigned int )(&pocono_key_end - pocono_key);
	UINT32  user_part_key_size = (unsigned int )(&user_part_key_end - user_part_key);
    	UINT32	cust_data_size = (unsigned int )(&cust_data_end - cust_data);
	int res = CMD_RET_SUCCESS;

#ifdef EP_GDB_MSG
	prints("\nefuse programmer (phoenix) v1.03 start....\n");

	prints("\tfor secure boot platform, \n");
	prints("\t-w rsa\n");
#endif
	//0. judge go or not
	if(obfuse_judge())
	{
#ifdef EP_GDB_MSG
	  	prints("this borad is secue boot.\nreturn.");
#endif
		prints("[EP_RESULT]FAILURE(1)\n");
		res = 1; //secue bit is burned.	
	  	return res;
	}
#ifdef EP_GDB_MSG
	prints("judge secure bit success.\r\n");
#endif
	sync();

#ifdef KEY_BURN
#ifdef EP_GDB_MSG
        prints("case:key.\n");
#endif

	//1. burn the rsa public key directly
	if(start_program(0, rsa_pub_key_size, rsa_pub_key))
	{
#ifdef EP_GDB_MSG
		prints("program aes pub key fail\r\n");
#endif
		prints("[EP_RESULT]FAILURE(2)\n");
		res = 2; //public rsa key burn fail
        	return res;
	}
	else
	{
#ifdef EP_GDB_MSG
	 	prints("program aes pub key success.\r\n");
#endif
	}
	sync();

#ifdef EP_GDB_MSG
	//2.1 write aes
	prints("\t-w key \n");
	#ifdef DEBUG_PRT
	hexdump("aes_key :", aes_key, 16);
	#endif
#endif

	if(obfuse_getset(22,aes_key,2304,aes_key_size))
	{
#ifdef EP_GDB_MSG
	 	prints("program aes key fail\r\n");
#endif
		prints("[EP_RESULT]FAILURE(3)\n");
		res = 3; //aes key fail (kh)
	 	return res;
	}
	else
	{
#ifdef EP_GDB_MSG
		prints("program aes key success.\r\n");
#endif
	}
	sync();

#ifdef EP_GDB_MSG
	//2.2 write seed
	prints("\t-w seed \n");
	#ifdef DEBUG_PRT
	hexdump("aes_seed :", aes_seed, 16);
	#endif
#endif
	otp_retry_cnt = 0;

	if(obfuse_getset(22,aes_seed,2048,aes_seed_size))
	{
#ifdef EP_GDB_MSG
		prints("program aes seed fail\r\n");
#endif
		prints("[EP_RESULT]FAILURE(4)\n");
		res = 4;
	  	return res;
	}
	else
	{
#ifdef EP_GDB_MSG
		prints("program aes seed success.\r\n");
#endif
	}
 	sync();

#ifdef EP_GDB_MSG	
	prints("\t-w pocono \n");
#endif
	//nintendo case
	if(do_burn_pocono_key(pocono_key,pocono_key_size))
	{
#ifdef EP_GDB_MSG
	 	prints("program pocono key fail.\r\n");
#endif
		prints("[EP_RESULT]FAILURE(5)\n");
		res = 5;
		return res;
	}
	sync();

#ifdef EP_GDB_MSG	
	prints("\t-w user part \n");
#endif
	if(do_burn_userpart_key(user_part_key,user_part_key_size))
	{
#ifdef EP_GDB_MSG
		prints("program user part key fail.\r\n");
#endif
		prints("[EP_RESULT]FAILURE(6)\n");
		res = 6;
		return res;
	}
	sync();
#endif

#ifdef FINALIZE_BURN
#ifdef EP_GDB_MSG
        prints("case:finalize.\n");
#endif
	
	if(do_burn_efuse_bootenc())
	{
		prints("[EP_RESULT]FAILURE(7)\n");
		res = 7;
	 	return res;
	}
	sync();

	if(do_burn_efuse_hwchk(2))
	{
#ifdef EP_GDB_MSG
	 	//chk later
	 	prints("do_burn_efuse_hwchk(2) fail\r\n");
#endif
		prints("[EP_RESULT]FAILURE(8)\n");
		res = 8;
 	 	return res;
	}
	sync();

	if(do_burn_efuse_rsa_pgm_disable())
	{
		prints("[EP_RESULT]FAILURE(9)\n");
		res = 9;
		return res;
	}
	sync();

	if(do_burn_efuse_other_key_pgm_disable())
	{
		prints("[EP_RESULT]FAILURE(10)\n");
		res = 10;
		return res;
	}
	sync();

	if(do_burn_efuse_jtg_disable())
	{
		prints("[EP_RESULT]FAILURE(11)\n");
		res = 11;
		return res;
	}
	sync();

	if(do_burn_efuse_rbus_disable())
        {
		prints("[EP_RESULT]FAILURE(12)\n");
		res = 12;
                return res;
        }
        sync();

        if(do_burn_efuse_usb2sram_disable())
        {
		prints("[EP_RESULT]FAILURE(13)\n");
		res = 13;
                return res;
        }
        sync();

	if(do_burn_efuse_secureboot())
	{
		prints("[EP_RESULT]FAILURE(14)\n");
		res = 14;
		return res;
	}
	sync();
#endif
	return res;
}

/************************************************************************
 *
 *                          efusemain
 *  Description :
 *  -------------
 *  main function of efuse programmer
 *
 *  Parameters :
 *  ------------
 *
 *
 *
 *  Return values :
 *  ---------------
 *
 *
 *
 *
 ************************************************************************/
int efusemain (int argc, char * const argv[])
{
	int ret = -1;
	extern unsigned int mcp_dscpt_addr;

	mcp_dscpt_addr = 0;
	otp_retry_cnt = 0;
	//init_uart();
	//set_focus_uart(0);
#ifndef FAKE_BURN
	prints("\n[EP_START]\n");
	if ((ret = do_program()) != 0) {
		goto reTag;
	}
#else
#ifdef EP_GDB_MSG
	prints("\r\nfake burn.\r\n");
#endif
	prints("\n[EP_START]\n");
	ret = 0;
#endif
reTag:
	if (ret == 0)
	{
#ifdef EP_GDB_MSG
		prints("program OTP OKOKOKOKOKOK!!!!!!!\n");
#else
		prints("[EP_RESULT]SUCCESS(0)\n");
		prints("[EP_END]\n");
#endif
	}
	else
	{
#ifdef EP_GDB_MSG
		prints("program OTP failfailfail!!!!!!!\n");
#else
		prints("[EP_END]\n");
#endif
	}
	return 0;
}
