#include <common.h>
#include <command.h>

typedef struct _st_uart_parameter{
  char *cpt_mac;
  unsigned int u_addr;
  unsigned int u_len;
  unsigned int u_second_addr;
  unsigned int u_second_len;
  char c_filename[10];
  char c_second_filename[10];
}st_uart_parameter;

static int do_uart_w(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
  unsigned int uRes=1,option=0,i=0;
  st_uart_parameter s_parameter;
  memset(&s_parameter, (0x00), sizeof(st_uart_parameter)); 
  printf("argc :%d \n",argc);

  if ((argc > 7)||(argc <= 2)) {
    printf("argc :%d ,error!\n",argc);
    cmd_usage(cmdtp);
    return uRes;
  }

  option = (unsigned int)simple_strtoul(argv[1], NULL, 16);
  switch(option){
	case 1://mac
	  s_parameter.cpt_mac = argv[2];
	  printf("burn mac.\n");
	  uRes = do_uart_w_mac(s_parameter.cpt_mac);
	 break;
	case 2: //sn
	case 3: //acs
	  s_parameter.u_addr = (unsigned int)simple_strtoul(argv[2], NULL, 16);
          s_parameter.u_len = (unsigned int)simple_strtoul(argv[3], NULL, 16);
	  if((!s_parameter.u_addr)|(!s_parameter.u_len)){
	   printf("error parameter.\n");
	   break;
	  }
	  if(option==2){
	    strcpy(s_parameter.c_filename,"sn.txt");
	    printf("burn serial.\n");
	  }else if(option==3){
	    strcpy(s_parameter.c_filename,"acs.txt");
	    printf("burn acs.\r\n");
	  }
	  uRes = do_uart_w_file_to_factory(s_parameter);
	 break;
	case 4:
	  s_parameter.cpt_mac = argv[2];
          s_parameter.u_addr = (unsigned int)simple_strtoul(argv[3], NULL, 16);
          s_parameter.u_len = (unsigned int)simple_strtoul(argv[4], NULL, 16);
	  if((!s_parameter.u_addr)||(!s_parameter.u_len)){
           printf("error parameter.\n");
           break;
          }
          strcpy(s_parameter.c_filename,"sn.txt");
	  printf("burn mac,serail.\nmac begin\n");
	  uRes = do_uart_w_mac(s_parameter.cpt_mac);
	  if(uRes==0){
	   printf("serial begin.\n");
	   uRes = do_uart_w_file_to_factory(s_parameter);
	  }
	 break;
	case 5:
	  s_parameter.cpt_mac = argv[2];
          s_parameter.u_addr = (unsigned int)simple_strtoul(argv[3], NULL, 16);
          s_parameter.u_len = (unsigned int)simple_strtoul(argv[4], NULL, 16);
	  if((!s_parameter.u_addr)||(!s_parameter.u_len)){
           printf("error parameter.\n");
           break;
          }
          strcpy(s_parameter.c_filename,"acs.txt");
	  printf("burn mac,acs.\nmac begin.\n");
	  uRes = do_uart_w_mac(s_parameter.cpt_mac);
          if(uRes==0){
	   printf("acs begin.\n");
           uRes = do_uart_w_file_to_factory(s_parameter);
	  }
	 break;
	case 6:
          s_parameter.u_addr = (unsigned int)simple_strtoul(argv[2], NULL, 16);
          s_parameter.u_len = (unsigned int)simple_strtoul(argv[3], NULL, 16);
	  s_parameter.u_second_addr = (unsigned int)simple_strtoul(argv[4], NULL, 16);
          s_parameter.u_second_len = (unsigned int)simple_strtoul(argv[5], NULL, 16);
	  if((!s_parameter.u_addr)||(!s_parameter.u_len)||(!s_parameter.u_second_addr)||(!s_parameter.u_second_len)){
           printf("error parameter.\n");
           break;
          }
	  strcpy(s_parameter.c_filename,"sn.txt");
          strcpy(s_parameter.c_second_filename,"acs.txt");
	  printf("burn serial,acs.\n");
	  uRes = do_uart_w_file_to_factory(s_parameter);
	 break;
	case 7:
	  s_parameter.cpt_mac = argv[2];
	  s_parameter.u_addr = (unsigned int)simple_strtoul(argv[3], NULL, 16);
          s_parameter.u_len = (unsigned int)simple_strtoul(argv[4], NULL, 16);
	  strcpy(s_parameter.c_filename,"sn.txt");
          s_parameter.u_second_addr = (unsigned int)simple_strtoul(argv[5], NULL, 16);
          s_parameter.u_second_len = (unsigned int)simple_strtoul(argv[6], NULL, 16);
	  strcpy(s_parameter.c_second_filename,"acs.txt");
	  printf("burn mac,serial,acs.\nmac begin.\n");
          uRes = do_uart_w_mac(s_parameter.cpt_mac);
	  if(uRes==0){
	   printf("burn serial and acs\n");
	   uRes = do_uart_w_file_to_factory(s_parameter);
	  }
	 break;
	default:
	  return uRes;
	 break;
  }
  
  if(uRes != 0){
    printf("***** FAILFAIL *****\n");
    printf("***** FAILFAIL *****\n");
    printf("***** FAILFAIL *****\n");
    printf("***** FAILFAIL *****\n");
    printf("***** FAILFAIL *****\n");
  }else{
    printf("***** OKOK *****\n");
    printf("***** OKOK *****\n");
    printf("***** OKOK *****\n");
    printf("***** OKOK *****\n");
    printf("***** OKOK *****\n");
  }
 return uRes;
}

int do_uart_w_mac(char *mac_value){
 
  int  bRe=1,num=0;
  char *mac_check=NULL;

  printf("ethaddr:%s\n",mac_value);
  setenv("ethaddr",mac_value);

    if((mac_check = getenv("ethaddr")) == NULL){
      printf("getenv ethaddr fail !\n");
      printf("***** FAILFAIL *****\n");
      printf("***** FAILFAIL *****\n");
      printf("***** FAILFAIL *****\n");
      printf("***** FAILFAIL *****\n");
      printf("***** FAILFAIL *****\n");
    }else{
	bRe=saveenv();
 	if(bRe==0){
             if((strncmp(mac_value,mac_check,17))!= 0){
              	 printf("compare mac fail !\n");
              	 printf("***** FAILFAIL *****\n");
              	 printf("***** FAILFAIL *****\n");
              	 printf("***** FAILFAIL *****\n");
              	 printf("***** FAILFAIL *****\n");
              	 printf("***** FAILFAIL *****\n");
             }else
	 	 bRe=0;
	}else
	 printf("mac envsave fail!\n");
    }
  return bRe;
}

int do_uart_w_file_to_factory(st_uart_parameter s_parameter){
  printf("enter.write to factory\n");
  unsigned int uRet=1,num=0,utotalcount=1,addr,len;
  char c_factory_path[13] = {"tmp/factory/"};
  char c_target_path[256] = {'\0'};

  if(s_parameter.u_second_len>0)
   utotalcount++;

  while(utotalcount--){
   memset(&c_target_path,(0x00),sizeof(c_target_path));
   strcat(c_target_path,c_factory_path);
 
   if(utotalcount!=0){
    strcat(c_target_path,s_parameter.c_second_filename);
    addr=s_parameter.u_second_addr;
    len=s_parameter.u_second_len;
   }else{
    strcat(c_target_path,s_parameter.c_filename);
    addr = s_parameter.u_addr;
    len = s_parameter.u_len;
   }
   printf("target :%s\n",c_target_path);
   uRet = factory_write(c_target_path,addr,len);
   if(uRet != 0){
     printf("factory write %s fail!\n",c_target_path);
     break;
   }
   
   if(utotalcount==0){
    uRet = factory_save();
    if (uRet != 0){
      printf("factory save fail.\n");
      break;
    }
   }
  }//while end.
reTag:
   return uRet;
}

U_BOOT_CMD(
	uart_write,	7,	0,	do_uart_w,
	"for uart mp tool burn [mac|serial|acs] key\n",
	"\ncommand option:[command] [case option] [parameter...]\n"
	"[mac] 			: uart_writ 1 [mac val] \n"
	"[serial] 		: uart_writ 2 [serial load addr] [file lenght(hex)]\n"
	"[acs]			: uart_writ 3 [acs load addr] [file lenght(hex)]\n"
	"[mac & serial]  	: uart_writ 4 [mac val] [sn load addr] [file lenght(hex)]\n"
	"[mac & acs]  		: uart_writ 5 [mac val] [acs load addr] [file lenght(hex)]\n"
	"[serial & acs]  	: uart_writ 6 [serial load addr] [file lenght(hex)] [acs load addr] [file lenght(hex)]\n"
	"[mac & serial & acs]  	: uart_writ 7 [mac val] [serial load addr] [file lenght(hex)] [acs load addr] [file lenght(hex)]\n"
);
