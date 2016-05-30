/*
 * zy_epoll_tcpserver.c
 *
 *  Created on: 2015年6月20日
 *      Author: zhuyong
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <string.h>
#include "zy_tcpserver.h"
//#include "zy_log.h"
#include <pthread.h>
#include "zy_mempool.h"

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


//建立tcpserver时，需要建立它的连接对象池，每次新连接对象到来，从连接对象池中取出连接对象
//在创建tcpserver对象之前，需要对它进行初始化配置
int zy_tcpserver_create(zy_tcpserver_t* tcpserver){
	//创建服务器需要的客户端对象池
	int ret;
	zy_mempool_create(&(tcpserver->mempool),sizeof(zy_tcpserver_peer_mempool_t),tcpserver->maxclient);

	//建立服务端socket
	tcpserver->epoll_node.epoll_node_type=1;
	tcpserver->socket_fd=socket(AF_INET, SOCK_STREAM, 0);
	if (tcpserver->socket_fd < 0) {
		perror("create socket failed!\n");
		return -1;
	}

	pthread_cond_init(&(tcpserver->status_cond), NULL);
	pthread_mutex_init(&(tcpserver->status_mutex), NULL);


	//设置tcp_server
	tcpserver->accept_handle=zy_tcpserver_accept;
	tcpserver->msg_handle=zy_tcpserver_msg_handle;
	// 设置可复用
	int reuse = 1;
	setsockopt(tcpserver->socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	return 0;
}

int zy_tcpserver_config(zy_tcpserver_t* tcpserver,char* ip,uint16_t port,int maxclient,int (*do_protocol)(int fd,char* buf,int count)){
	tcpserver->ip = ip;
	tcpserver->port = port;
	tcpserver->maxclient = maxclient;
	tcpserver->do_protocol=do_protocol; //协议处理函数，每个tcpserver需要设置
	return 0;
}



//将tcpserver增加到epoll监听事件队列中
int zy_tcpserver_start(zy_tcpserver_t* tcpserver,zy_epoll_t* epoll){

	struct sockaddr_in addr_serv;
	memset(&addr_serv, 0, sizeof(addr_serv));
	addr_serv.sin_family = AF_INET;
	addr_serv.sin_port = htons(tcpserver->port);
	addr_serv.sin_addr.s_addr = inet_addr(tcpserver->ip);
	// 绑定端口和地址
	if (bind(tcpserver->socket_fd, (struct sockaddr*) &addr_serv, sizeof(struct sockaddr_in))
			== -1) {
		perror("bind error!\n");
		close(tcpserver->socket_fd);
		return -1;
	}

	setnonblocking(tcpserver->socket_fd);
	if (listen(tcpserver->socket_fd, 5) == -1) {
		perror("listen failed!\n");
		close(tcpserver->socket_fd);
		return -1;
	}


	//增加tcpserver到epoll
	struct epoll_event *ev;
	ev=&(tcpserver->epoll_node.ev);
	ev->data.ptr = tcpserver;
	ev->events = EPOLLIN | EPOLLET;

	if (epoll_ctl(epoll->epoll_fd, EPOLL_CTL_ADD, tcpserver->socket_fd,ev) < 0) {
		//zlog_fatal(SERVERLOG, "epoll_ctrl error!");
		return -1;
	}
	return 0;
}

int zy_tcpserver_accept(zy_epoll_t* epoll,struct epoll_event *event){
		//printf("Add socket\n");
while(1) {
		struct sockaddr_in in_addr;
		socklen_t in_len;
		int infd;
		in_len = sizeof(in_addr);
		zy_tcpserver_t *tcpserver;
		tcpserver=event->data.ptr;
		infd = accept(tcpserver->socket_fd, (struct sockaddr *) &in_addr, &in_len);
		if (infd == -1) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
				// 我们已经处理完所有的连接了
				return 0;
				break;
			} else {
				//zlog_error(SERVERLOG, "accept failed!");
				return -1;
				break;
			}
		}
		setnonblocking(infd);

//		zlog_debug(CLIENTLOG, "%s:%d %s", sock_event_data->addr,sock_event_data->port, "Client connect");

		//将客户端信息保存到TCP_SERVER的客户端链接池中
		zy_tcpserver_peer_mempool_t *client;
		//add by zhuyong 2015.9.24
		pthread_mutex_lock(&(tcpserver->status_mutex));
		zy_mempool_get_node(&(tcpserver->mempool),(zy_mempoolnode_t **)&client);
		//add by zhuyong 2015.9.24
		pthread_mutex_unlock(&(tcpserver->status_mutex));
		//设置client
		client->zy_tcpserver_peer.epoll_node.epoll_node_type=2;
		client->zy_tcpserver_peer.socket_fd=infd;
		//client->zy_tcpserver_peer.ip=inet_ntoa(in_addr.sin_addr);  //这个地方有问题
		//client->zy_tcpserver_peer.port=in_addr.sin_port;
		client->zy_tcpserver_peer.msg_handle=tcpserver->msg_handle;
		client->zy_tcpserver_peer.do_protocol=tcpserver->do_protocol;
		client->zy_tcpserver_peer.tcpserver=tcpserver;
		client->zy_tcpserver_peer.peer_mempoolnode=client;
		//TODO将client socket加入epoll
		struct epoll_event *ev;
		ev=&(client->zy_tcpserver_peer.epoll_node.ev);
		ev->data.ptr =&(client->zy_tcpserver_peer);
		ev->events = EPOLLIN | EPOLLET;
		//这里ev是局部变量，使用完会释放，是否存在问题？
		if (epoll_ctl(epoll->epoll_fd, EPOLL_CTL_ADD, client->zy_tcpserver_peer.socket_fd,ev) < 0) {
			//zlog_fatal(SERVERLOG, "epoll_ctrl error!");
			return -1;
		}
		return 0;
	}
	return 0;
		//if (add_epoll_listen(epoll,&(client->clientepoll),infd,tcpserver->client_event,client)==-1){
		//	return -1;
		//};
}

#define MAX_BUFFER 1024
int zy_tcpserver_msg_handle(zy_epoll_t* epoll,struct epoll_event *event){

	if ((event->events & EPOLLHUP) || (event->events & EPOLLERR) || (!(event->events & EPOLLIN))) {
//		zlog_error(SERVERLOG, "epoll error");
		return -1;
		//close(ev_data->fd);
	} else if (event->events & EPOLLIN) {
		// 读取请求  //处理输入的TCP数据
		//printf("Read!\n");
		//zlog_debug(clientlog, "%s:%d %s", ev_data->addr, ev_data->port,"Recieve Data");
		char buf[MAX_BUFFER];
		ssize_t count = 0, nread = 0;
		zy_tcpserver_peer_t *tcpclient;
		tcpclient=event->data.ptr;
		while ((nread = read(tcpclient->socket_fd, buf + count, MAX_BUFFER - count)) > 0) {
			count += nread;
			if (count >= MAX_BUFFER) {
				// 收到的包太大了，我们只读MAX_BUFFER 个字节，然后剩下的丢掉
//				zlog_debug(clientlog,
//						"%s:%d Recieve Data is longer than %d bytes, ignore spare bytes",
//						ev_data->addr, ev_data->port, MAX_BUFFER);
				char buffer[400];
				while ((nread = read(tcpclient->socket_fd, buffer, 400)) > 0) {
				}
				break;
			}
		}

		if (nread == -1 && errno != EAGAIN) {
			if (epoll_ctl(epoll->epoll_fd, EPOLL_CTL_DEL, tcpclient->socket_fd,NULL) < 0) {
						//zlog_fatal(SERVERLOG, "epoll_ctrl error!");
						return -1;
					}
			pthread_mutex_lock(&(tcpclient->tcpserver->status_mutex));
			close(tcpclient->socket_fd);
			zy_mempool_return_node(&(tcpclient->tcpserver->mempool),(zy_mempoolnode_t *)tcpclient->peer_mempoolnode);
			//print_all_usernode(&(tcpclient->tcpserver->mempool));
			pthread_mutex_unlock(&(tcpclient->tcpserver->status_mutex));
			return 0;
			//需要增加从epoll里面删除节点，以及从TCP SERVER里面删除节点
			//zlog_error(log, "%s read error", strerror(errno));
		} else if (nread == 0) {
			// 客户端主动关闭
			if (epoll_ctl(epoll->epoll_fd, EPOLL_CTL_DEL, tcpclient->socket_fd,NULL) < 0) {
									//zlog_fatal(SERVERLOG, "epoll_ctrl error!");
									return -1;
								}

			pthread_mutex_lock(&(tcpclient->tcpserver->status_mutex));
			close(tcpclient->socket_fd);
			zy_mempool_return_node(&(tcpclient->tcpserver->mempool),(zy_mempoolnode_t *)tcpclient->peer_mempoolnode);
			//print_all_usernode(&(tcpclient->tcpserver->mempool));
			pthread_mutex_unlock(&(tcpclient->tcpserver->status_mutex));
			//需要增加从epoll里面删除节点，以及从TCP SERVER里面删除节点
			return 0;
		}

		//调用业务处理函数，处理接收到的数据
		tcpclient->do_protocol(tcpclient->socket_fd,buf,count);
				//close(events[i].data.fd);
		return 0;
		}
	return 0;
}

//将tcpserver从epoll监听事件队列中删除
int zy_tcpserver_stop(zy_tcpserver_t* tcpserver,zy_epoll_t* epoll){

	struct epoll_event ev;
	ev.data.ptr = tcpserver;
	ev.events = EPOLLIN | EPOLLET;

	if (epoll_ctl(epoll->epoll_fd, EPOLL_CTL_DEL, tcpserver->socket_fd,&ev) < 0) {
		//zlog_fatal(SERVERLOG, "epoll_ctrl error!");
		return -1;
	}
	return 0;
}

//释放tcpserver的内存池对象
int zy_tcpserver_destory(zy_tcpserver_t* tcpserver){
	free(&(tcpserver->mempool));
	return 0;
}



