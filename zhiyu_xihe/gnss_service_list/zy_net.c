/*
 * zy_net.c
 *
 *  Created on: 2014年6月4日
 *      Author: zhuyong
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <time.h>
#include <signal.h>
#include <sys/time.h>

#include "zy_logcat.h"
#include "zy_net.h"
#include "zy_mempool.h"
#include "zy_config.h"

extern   udp_server_t udp_server_b;

int create_epoll(epoll_t *epoll,int max_events)
{
	//epoll->epoll_fd = epoll_create1(0);
	epoll->epoll_fd = epoll_create(max_events);
	if (epoll->epoll_fd == -1) {
		//zlog_fatal(SERVERLOG, "create epoll failed!");
		return -1;
	}
	//zlog_debug(SERVERLOG, "create epoll ok!");
	epoll->max_events=max_events;
	//创建epoll数据对象池
	if (zy_mempool_create(epoll->event_data_pool,sizeof(zy_event_data_t),epoll->max_events)==-1) {
		puts("create events error!!!");
		return -2;
	}
	//创建epoll事件队列,用于已经发生的epoll事件
	epoll->events = (struct epoll_event *) calloc(epoll->max_events,sizeof(struct epoll_event));
	if (epoll->events==NULL){
		return -3;
	}
	return 0;

}

int run_epoll(epoll_t *epoll)
{
	int nready;
	//zlog_debug(SERVERLOG, "epoll is running!");

	while (1)
	{
		nready = epoll_wait(epoll->epoll_fd, epoll->events, epoll->max_events, -1);//receive connect
		if (nready == -1) {
			//zlog_fatal(SERVERLOG, "epoll_pwait");
			return -1;
		}
		int i;
		printf(" run epoll :get a log\n");
		for (i = 0; i < nready; i++) {
			zy_event_data_t *ev_data = (zy_event_data_t*) epoll->events[i].data.ptr;
			//执行事件处理函数
			
			(ev_data->handle)(ev_data->client, epoll->events[i].events);//deal with connect
		
		}
	}
	return nready;
}


int add_epoll_listen(epoll_t *epoll,zy_event_data_t **epoll_event,int sock_fd,void *handle_event,void *socket_data)
{
	//创建一个监听端口，并放入epoll并监听
	zy_event_data_t * sock_event_data;
	zy_mempool_get_node(epoll->event_data_pool,(zy_mempoolnode_t **)&sock_event_data);
	(*epoll_event)=sock_event_data;
	sock_event_data->fd = sock_fd;
	sock_event_data->client = socket_data;
	sock_event_data->handle = handle_event;   //需要传入函数指针
    printf("add epoll listen sock_fd =%d\n",sock_fd);
	struct epoll_event *ev;
	ev=&(sock_event_data->ev);
	ev->data.fd = sock_fd;
	ev->data.ptr = sock_event_data;
	ev->events = EPOLLIN | EPOLLET;
	if (epoll_ctl(epoll->epoll_fd, EPOLL_CTL_ADD, sock_fd, ev) < 0) {
		//zlog_fatal(SERVERLOG, "epoll_ctrl error!");
		return -1;
	}

	return 0;
}


/*设置为非阻塞模式*/
void setnonblocking(int sock) {
	int opts;
	opts = fcntl(sock, F_GETFL);
	if (opts < 0) {
		perror("fcntl(F_GETFL)!\n");
		exit(1);
	}
	opts = (opts | O_NONBLOCK);
	if (fcntl(sock, F_SETFL, opts) < 0) {
		perror("fcntl(F_SETFL)!\n");
		exit(1);
	}
}




//需要在使用udp_server的地方先创建一个udp_server对象
int create_udp_server(udp_server_t *udp_server)
{
	//创建服务器需要的客户端对象池
	zy_mempool_create(&(udp_server->mempool),sizeof(udp_server_client_t),udp_server->maxclient);

	//建立服务端socket
	udp_server->server_fd=socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_server->server_fd < 0) {
		perror("create socket failed!\n");
		return -1;
	}

	//设置udp_server
	udp_server->udp_event=udp_handle_event;
	// 设置可复用
	int reuse = 1;
	setsockopt(udp_server->server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	return 0;
}



int config_udp_server(udp_server_t *udp_server)
{
	struct sockaddr_in addr_serv;
	memset(&addr_serv, 0, sizeof(addr_serv));
	addr_serv.sin_family = PF_INET;
	addr_serv.sin_port = htons(udp_server->port);
	
	addr_serv.sin_addr.s_addr = INADDR_ANY;
	printf("addr_serv.sin_port = %d\n",udp_server->port);
	printf("addr_serv.sin_addr = %d\n",addr_serv.sin_addr.s_addr);
	// 绑定端口和地址
	if (bind(udp_server->server_fd, (struct sockaddr*) &addr_serv, sizeof(struct sockaddr))
			== -1) {
		perror("bind error!\n");
		close(udp_server->server_fd);
		return -1;
	}
	return 0;
}


//启动udp_server监听，并放入epoll
int start_udpserver_listen(epoll_t *epoll,udp_server_t *udp_server)
{
	//创建一个监听端口，并放入epoll并监听
	// 设置非阻塞
	setnonblocking(udp_server->server_fd);
	printf("start udp listen ok =%d\n",udp_server->server_fd);
	
	udp_server->epoll=epoll;
	
   if (add_epoll_listen(epoll,&(udp_server->serverepoll),udp_server->server_fd,udp_server->udp_event,udp_server)==-1){
		return -1;
	}
 
	return 0;
}



//处理UDP数据事件接口
extern void udp_handle_event(udp_server_t *udp_server, uint32_t events)
{
	int ret = -1;
	int sock = -1;
	int len = -1;
	char buf[20];
	fd_set readfd;
	struct timeval timeout;
	struct sockaddr_in local_addr;		//本地地址
	struct sockaddr_in from_addr;		//客户端地址
    
    char client_buf[20]="gps_found";
	char nmea_ack_buf[50]="";
	char gnss_ack_buf[50]="";
	char ins_ack_buf[50]="";
	char gps_gnss_ack_buf[50]="";
	char gps_ins_ack_buf[50]="";
	len =sizeof(struct sockaddr_in);
	//recieve  message from client
	ret = recvfrom(udp_server->server_fd, buf, sizeof(buf), 0, (struct sockaddr *)&from_addr, &len);
	if(0 > ret)
	{
		perror("server recieve err");

	}
	printf("recvfrom  buf : %s\n", buf);

	//如果与ip_found吻合
	if( strstr(buf, client_buf) )
	{
		//send  message to client
		if (config.gnss_server_yn==1) {
			sprintf(gps_gnss_ack_buf,"ack,%s,%d",config.gnss_server_name,config.gnss_server_port);
			ret = sendto(udp_server->server_fd, gps_gnss_ack_buf, strlen(gps_gnss_ack_buf) , 0, (struct sockaddr *)&from_addr, len);
			if(0 > ret)
			{
				perror("nmea server send err");

			}
		}

		if (config.ins_server_yn==1){
			sprintf(gps_ins_ack_buf,"ack,%s,%d",config.ins_server_name,config.ins_server_port);
			ret = sendto(udp_server->server_fd, gps_ins_ack_buf, strlen(gps_ins_ack_buf) , 0, (struct sockaddr *)&from_addr, len);
			if(0 > ret)
			{
				perror("gnss server send err");

			}
		}

		if (config.nmea_server_yn==1) {
			sprintf(nmea_ack_buf,"acknmea,%s,%d",config.nmea_server_name,config.nmea_server_port);
			ret = sendto(udp_server->server_fd, nmea_ack_buf, strlen(nmea_ack_buf) , 0, (struct sockaddr *)&from_addr, len);
			if(0 > ret)
			{
				perror("nmea server send err");

			}
		}

		if (config.gnss_nmea_server_yn==1) {
			sprintf(gnss_ack_buf,"acknmea,%s,%d",config.gnss_nmea_server_name,config.gnss_nmea_server_port);
			ret = sendto(udp_server->server_fd, gnss_ack_buf, strlen(gnss_ack_buf) , 0, (struct sockaddr *)&from_addr, len);
			if(0 > ret)
			{
				perror("gnss server send err");

			}
		}

		if (config.ins_nmea_server_yn==1) {
			sprintf(ins_ack_buf,"acknmea,%s,%d",config.ins_nmea_server_name,config.ins_nmea_server_port);
			ret = sendto(udp_server->server_fd, ins_ack_buf, strlen(ins_ack_buf) , 0, (struct sockaddr *)&from_addr, len);
			if(0 > ret)
			{
				perror("ins server send err");

			}
		}

		printf("client IP is %s\n", inet_ntoa(from_addr.sin_addr));
		printf("sendto success! \n");
	                  				
	}	
	
}





