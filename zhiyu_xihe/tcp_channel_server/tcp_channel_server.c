/*
 ============================================================================
 Name        : tcp_channel_server.c
 Author      : zhuyong
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "zy_tcp_worker.h"
#include "zy_log.h"
//#include "zy_log.h"
int main(int argc,char ** argv) {
	

	//puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	#ifdef WRITE_TCP_LOG
	   //zy_log_init();
	   //fprintf(SERVERLOG,"open log ok!\n");  
  	    
	#endif 	
	int res;
	/*	
	res = zy_log_init();
	if(res == -1)
	{
		puts("log init error!");
		zlog_debug(serverlog,"log init error!");
		return -1;
	}
	zlog_debug(serverlog,"log init ok!");
	*/
	tcp_server_init();
	tcp_server_run();
	return EXIT_SUCCESS;
}
