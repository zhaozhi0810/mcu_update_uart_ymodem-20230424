/*
*********************************************************************************************************
*
*	Ä£¿éÃû³Æ : Ö÷º¯Êý
*	ÎÄ¼þÃû³Æ : main.c
*	°æ    ±¾ : V1.0
*	Ëµ    Ã÷ : Ö÷¹¦ÄÜ
*
*	ÐÞ¸Ä¼ÇÂ¼ 
*		°æ±¾ºÅ    ÈÕÆÚ         ×÷Õß      ËµÃ÷
*		V1.0    2023-04-06     dazhi    Ê×·¢
*
*	Copyright (C), 2022-2030, htjc by dazhi
*
*
*    aarch64-linux-gnu-gcc *.c -o xyzmodem_send
*********************************************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include "stdint.h"
#include "uart_to_mcu.h"
#include "xyzmodem.h"
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
 /* Ô´ÎÄ¼þÂ·¾¶ */
//char SourceFile[] = "./app.bin";    
static const char* my_opt = "f:";



void send_update_cmd_tomcu(uint8_t phase)
{
	uint8_t buf[] = {0xa5,74,0,0xef};   //Éý¼¶ÃüÁî

	if(phase)
	{
		buf[2] = 1;  //确认需要下载
		buf[3] = 0xf0;  //check sum
	}	

	UART_SendPacket(buf, 4);   //4¸ö×Ö½Ú·¢³öÈ¥
}



//ps -ef | grep drv722_22134_server | grep -v grep | wc -l
//尝试启动server进程
//返回0表示没有该进程，1表示存在进程了,-1表示出错
static int is_server_process_start(char * cmd_name)
{
	FILE *ptr = NULL;
	char cmd[256] = "ps -ef | grep %s | grep -v grep | wc -l";
//	int status = 0;
	char buf[64];
	int count;

//	printf("server DEBUG:cmd_name = %s\n",cmd_name);
	snprintf(cmd,sizeof cmd,"ps -ef | grep %s | grep -v grep | wc -l",cmd_name);
	printf("ServerDEBUG: api check serverProcess is running, cmd = %s\n",cmd);

	if((ptr = popen(cmd, "r")) == NULL)
	{
		printf("is_server_process_start popen err\n");
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	if((fgets(buf, sizeof(buf),ptr))!= NULL)//获取进程和子进程的总数
	{
//		printf("server: buf = %s\n",buf);
		count = atoi(buf);
		if(count < 1)//当进程数小于等于2时，说明进程不存在, 1表示有一个，是grep 进程的
		{
			pclose(ptr);
			printf("ServerDEBUG: check serverProcess: no server process,ready to start serverProcess!!\n");
			return 0;  //系统中没有该进程	
		}
		
		printf("ServerDEBUG: check serverProcess: server process is running!!\n");
	}
	pclose(ptr);
	return 1;
}






int main(int argc,char* argv[]) 
{
	char* filename = "./app.bin";
	int get_name = 0;
	int c,ret;



    if(argc != 1)
	{	
	    while(1)
	    {
	        c = getopt(argc, argv, my_opt);
	        //printf("optind: %d\n", optind);
	        if (c < 0)
	        {
	            break;
	        }
	        //printf("option char: %x %c\n", c, c);
	        switch(c)
	        {
	        	case 'f':
	        		filename = optarg;
	        		get_name = 1;
	                //debug_level = atoi(optarg);
	                printf("filename = %s\n",filename);
	                break;	       
	       	 	default:	       
	                break;
	                //return 0;
	        }
	        if(get_name)  //Ìø³ö´óÑ­»·
	        	break;
	    }
	}



	uart_init(argc, argv);


	if(1 == is_server_process_start("drv722_22134_server"))  //存在server进程
	{
		//	system("/root/drv722api_disable_wtd");
		//system("killall drv722_22134_server");   //杀掉进程
		printf("drv722_22134_server is running!!\n");
	}




	//sleep(1);

    if(0 == xymodem_send(filename))
    	printf("%s is done!\n",argv[0]);

    uart_exit() ;

    return 0;
}


