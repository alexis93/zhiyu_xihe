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
#include "zy_tcpclient.h"
//#include "gnss_mysql.h"
//#include "user.h"
#include "zy_protocol.h"
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdbool.h>



zy_tcpclient_t nmea_tcpclient;
zy_tcpclient_t gnss_tcpclient;
zy_tcpclient_t gnss_nmea_tcpclient;
zy_tcpclient_t ins_tcpclient;
zy_tcpclient_t ins_nmea_tcpclient;
zy_tcpclient_t log_tcpclient;
zy_tcpclient_t obs_log_tcpclient;
zy_tcpclient_t eph_log_tcpclient;
zy_tcpclient_t clockorbit_log_tcpclient;
zy_tcpclient_t msg_log_tcpclient;

int tcp_client_init(void){

	//屏蔽SIGPIPE 信号，防止write的时候socket 关闭导致client退出,
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


	//创建tcp——client, nmea origion data client 接收客户端连接
	zy_tcpclient_config(&nmea_tcpclient,"nmea_tcpclient",config.nmea_server_address,config.nmea_server_port,do_protocol);

	res = zy_tcpclient_start(&nmea_tcpclient);
	if(res == -1)
	{
		puts("start nmea_tcpclient listen error.");
		return -1;
	}


	//创建tcp——client, gnss(GpsLocation data)接收客户端连接

	zy_tcpclient_config(&gnss_tcpclient,"gnss_tcpclient",config.gnss_server_address,config.gnss_server_port,do_protocol2);

	res = zy_tcpclient_start(&gnss_tcpclient);
	if(res == -1)
	{
		puts("start gnss_tcpclient listen error.");
		return -1;
	}
	
	//创建tcp——client, gnss_nmea(nmea_gga data)接收客户端连接
	zy_tcpclient_config(&gnss_nmea_tcpclient,"gnss_nmea_tcpclient",config.gnss_nmea_server_address,config.gnss_nmea_server_port,do_protocol3);

	res = zy_tcpclient_start(&gnss_nmea_tcpclient);
	if(res == -1)
	{
		puts("start gnss_nmea_tcpclient listen error.");
		return -1;
	}
	
    //创建tcp——client, ins data(ins data(location velocity attitude))接收客户端连接
	zy_tcpclient_config(&ins_tcpclient,"ins_tcpclient",config.ins_server_address,config.ins_server_port,do_protocol4);

	res = zy_tcpclient_start(&ins_tcpclient);
	if(res == -1)
	{
		puts("start ins_tcpclient listen error.");
		return -1;
	}

	 //创建tcp——client, ins_nmea data(nmea_gga)接收客户端连接
	zy_tcpclient_config(&ins_nmea_tcpclient,"ins_nmea_tcpclient",config.ins_nmea_server_address,config.ins_nmea_server_port,do_protocol5);

	res = zy_tcpclient_start(&ins_nmea_tcpclient);
	if(res == -1)
	{
		puts("start ins_nmea_tcpclient listen error.");
		return -1;
	}

	 //创建tcp——client, log data(log)接收客户端连接
	zy_tcpclient_config(&log_tcpclient,"log_tcpclient",config.log_tcpserver_address,config.log_tcpserver_port,do_protocol5);

	res = zy_tcpclient_start(&log_tcpclient);
	if(res == -1)
	{
		puts("start log_tcpclient listen error.");
		return -1;
	}


/*
	 //创建tcp——client, log data(log)接收客户端连接
	zy_tcpclient_config(&obs_log_tcpclient,"obs_log_tcpclient",config.obs_log_tcpserver_address,config.obs_log_tcpserver_port,do_protocol5);

	res = zy_tcpclient_start(&obs_log_tcpclient);
	if(res == -1)
	{
		puts("start obs_log_tcpclient listen error.");
		return -1;
	}
*/

/*
	 //创建tcp——client, log data(log)接收客户端连接
	zy_tcpclient_config(&eph_log_tcpclient,"eph_log_tcpclient",config.eph_log_tcpserver_address,config.eph_log_tcpserver_port,do_protocol5);

	res = zy_tcpclient_start(&eph_log_tcpclient);
	if(res == -1)
	{
		puts("start eph_log_tcpclient listen error.");
		return -1;
	}
*/

/*
	 //创建tcp——client, log data(log)接收客户端连接
	zy_tcpclient_config(&clockorbit_log_tcpclient,"clockorbit_log_tcpclient",config.clockorbit_log_tcpserver_address,config.clockorbit_log_tcpserver_port,do_protocol5);

	res = zy_tcpclient_start(&clockorbit_log_tcpclient);
	if(res == -1)
	{
		puts("start clockorbit_log_tcpclient listen error.");
		return -1;
	}

*/

/*
	 //创建tcp——client, log data(log)接收客户端连接
	zy_tcpclient_config(&msg_log_tcpclient,"msg_log_tcpclient",config.msg_log_tcpserver_address,config.msg_log_tcpserver_port,do_protocol5);

	res = zy_tcpclient_start(&msg_log_tcpclient);
	if(res == -1)
	{
		puts("start msg_log_tcpclient listen error.");
		return -1;
	}
*/
	return 0;
}

int send_all_tcpclient_msg(zy_tcpclient_t* tcpclient,char* buf,int len) {
	return zy_tcpclient_send(tcpclient,buf,len);
}

int send_nmea(char* buf,int len)
{
	return send_all_tcpclient_msg(&nmea_tcpclient,buf,len);
}

int send_gnss(char* buf,int len)
{
	return send_all_tcpclient_msg(&gnss_tcpclient,buf,len);
}

int send_gnss_nmea(char* buf,int len)
{
	return send_all_tcpclient_msg(&gnss_nmea_tcpclient,buf,len);
}

int send_ins(char* buf,int len)
{
	return send_all_tcpclient_msg(&ins_tcpclient,buf,len);
}

int send_ins_nmea(char* buf,int len)
{
	return send_all_tcpclient_msg(&ins_nmea_tcpclient,buf,len);
}

int send_log(char* buf,int len)
{
	return send_all_tcpclient_msg(&log_tcpclient,buf,len);
	//return 1;
}

int send_obs_log(char* buf,int len)
{
	//return send_all_tcpclient_msg(&obs_log_tcpclient,buf,len);
	return 1;
}

int send_eph_log(char* buf,int len)
{
	//return send_all_tcpclient_msg(&eph_log_tcpclient,buf,len);
	return 1;
}

int send_clockorbit_log(char* buf,int len)
{
	//return send_all_tcpclient_msg(&clockorbit_log_tcpclient,buf,len);
	return 1;
}

int send_msg_log(char* buf,int len)
{
	//return send_all_tcpclient_msg(&msg_log_tcpclient,buf,len);
	return 1;
}


