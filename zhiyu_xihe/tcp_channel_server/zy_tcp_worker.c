/*
 ============================================================================
 Name        : zy_arch.c
 Author      : zhuyong
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "zy_common.h"
#include "zy_mempool.h"
//#include "zy_net.h"
//#include "zy_log.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include <pthread.h>
#include "zy_config.h"
#include "zy_tcpserver.h"
//#include "gnss_mysql.h"
//#include "user.h"
#include "zy_protocol.h"
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "zy_tcp_worker.h"

#define MAX_CLIENTS 100
#define MAX_SIZE  1024

static pthread_t location_point_pthread_t;
static pthread_mutex_t location_point_mutex;
static pthread_cond_t  location_point_cond;

zy_epoll_t  epoll;
zy_tcpserver_t nmea_tcpserver;
zy_channel_t channels[MAX_CHANNEL];


int tcp_server_run(void) {
	int res;
	res = zy_epoll_run(&epoll);
	if(res == -1)
	{
		puts("run epoll error!\n");
		return -1;
	}
	return 0;
}




int tcp_server_init(void){
	int i;
	//屏蔽SIGPIPE 信号，防止write的时候socket 关闭导致server退出,
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigaction( SIGPIPE, &sa, 0);
	int res = -1;

	res=init_config();

	if(res==-1)
	{
		puts("config init error!");
		return -1;
	}

	//创建epoll模型

	res = zy_epoll_create(&epoll,MAX_CLIENTS*10);
	if(res !=0)
	{
		puts("create epoll error.");
		return -1;
	}


	for (i=0;i<MAX_CHANNEL;i++){
		channels[i].channel_config=&(channel_config[i]);
		switch(i) {
		case 0:
			zy_tcpserver_config(&(channels[i].rcv_tcpserver),channel_config[i].rcv_server_address,channel_config[i].rcv_server_port,MAX_CLIENTS,do_rcvserver0_protocol);
			break;
		case 1:
			zy_tcpserver_config(&(channels[i].rcv_tcpserver),channel_config[i].rcv_server_address,channel_config[i].rcv_server_port,MAX_CLIENTS,do_rcvserver1_protocol);
			break;
		case 2:
			zy_tcpserver_config(&(channels[i].rcv_tcpserver),channel_config[i].rcv_server_address,channel_config[i].rcv_server_port,MAX_CLIENTS,do_rcvserver2_protocol);
			break;
		case 3:
			zy_tcpserver_config(&(channels[i].rcv_tcpserver),channel_config[i].rcv_server_address,channel_config[i].rcv_server_port,MAX_CLIENTS,do_rcvserver3_protocol);
			break;
		case 4:
			zy_tcpserver_config(&(channels[i].rcv_tcpserver),channel_config[i].rcv_server_address,channel_config[i].rcv_server_port,MAX_CLIENTS,do_rcvserver4_protocol);
			break;
		default:
			zy_tcpserver_config(&(channels[i].rcv_tcpserver),channel_config[i].rcv_server_address,channel_config[i].rcv_server_port,MAX_CLIENTS,do_rcvserver5_protocol);
			break;
		}
		//zy_tcpserver_config(&(channels[i].rcv_tcpserver),channel_config[i].rcv_server_address,channel_config[i].rcv_server_port,MAX_CLIENTS,do_rcvserver0_protocol);

		res = zy_tcpserver_create(&(channels[i].rcv_tcpserver));
		if(res == -1)
		{
			puts("create rcv_tcpserver error.");
			return -1;
		}

		res = zy_tcpserver_start(&(channels[i].rcv_tcpserver),&epoll);
		if(res == -1)
		{
			puts("start rcv_tcpserver listen error.");
			return -1;
		}

		zy_tcpserver_config(&(channels[i].send_tcpserver),channel_config[i].send_server_address,channel_config[i].send_server_port,MAX_CLIENTS,do_sendserver0_protocol);
		res = zy_tcpserver_create(&(channels[i].send_tcpserver));
		if(res == -1)
		{
			puts("create send_tcpserver error.");
			return -1;
		}

		res = zy_tcpserver_start(&(channels[i].send_tcpserver),&epoll);
		if(res == -1)
		{
			puts("start send_tcpserver listen error.");
			return -1;
		}

		//TODO channels[i].send_msg=


	}

    //pthread_mutex_init(&location_point_mutex,NULL);
	//pthread_cond_init(&location_point_cond,NULL);
	//pthread_create(&location_point_pthread_t,NULL,tcp_server_run,NULL);

	return 0;
}


