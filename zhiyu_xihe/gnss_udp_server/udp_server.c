/*
 ============================================================================
 Name        : udp_server.c
 Author      : zcc
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */


#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
//#include <sys/signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>

#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
/* 标准c库 */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#include "zy_mempool.h"
#include "zy_net.h"
#include "zy_logcat.h"

#define MAX_CLIENTS 500
#define MAX_SIZE 1024
#define PORT 8888
#define IP  "0.0.0.0"


udp_server_t udp_server_b; 

int main(void) {
	//屏蔽SIGPIPE 信号，防止write的时候socket 关闭导致server退出,
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigaction( SIGPIPE, &sa, 0);
	int res = -1;
	
	//创建epoll模型
	epoll_t  epoll;
	epoll.event_data_pool = (zy_mempool_t *)calloc(1,sizeof(zy_mempool_t));
	res = create_epoll(&epoll,MAX_CLIENTS);
	if(res !=0)
	{
		puts("create epoll error.");
		return -1;
	}
	printf("create_epoll ok\n");
	
		
	//创建udp——server,接收客户端广播包
	
	udp_server_b.ip =IP ;
	udp_server_b.port = PORT;
	udp_server_b.maxclient = MAX_CLIENTS;
	res = create_udp_server(&udp_server_b);
	if(res == -1)
	{
		puts("create udp server r error.");
		return -1;
	}
	printf("create_udp_server r ok\n");
	res = config_udp_server(&udp_server_b);
	if(res == -1)
	{
		puts("bind error");
		return -1;
	}
	res = start_udpserver_listen(&epoll,&udp_server_b);
	if(res == -1)
	{
		puts("start udpserver_b  listen error.");
		return -1;
	}
	
	
	
	res = run_epoll(&epoll);
	if(res == -1)
	{
		puts("run epoll error!\n");
		return -1;
	}
	return EXIT_SUCCESS;
}

