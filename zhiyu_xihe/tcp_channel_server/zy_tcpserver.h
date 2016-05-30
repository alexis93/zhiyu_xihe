/*
 * zy_epoll_tcpserver.h
 *
 *  Created on: 2015年6月20日
 *      Author: zhuyong
 */

#ifndef ZY_EPOLL_TCPSERVER_H_
#define ZY_EPOLL_TCPSERVER_H_

#include <stdint.h>
#include <sys/epoll.h>
#include "zy_mempool.h"
#include "zy_epoll.h"
#include <pthread.h>

typedef struct zy_tcpserver_s zy_tcpserver_t;
struct zy_tcpserver_s {
	epoll_node_t epoll_node;
	int socket_fd;
	int maxclient;
	int (*accept_handle)(zy_epoll_t* epoll,struct epoll_event *event);   //接收客户端的链接请求，建立会话
	int (*msg_handle)(zy_epoll_t* epoll,struct epoll_event *event);      //每个链接的消息处理函数
	int (*do_protocol)(int fd,char* buf,int count);        //处理TCP连接的协议部分，在复用的时候，需要程序员编写
	char *ip;
//add by zhuyong 2015.9.24
	pthread_mutex_t status_mutex; /* 保护status字段，保证对status的互斥修改 */
	pthread_cond_t status_cond; /* 状态改变条件变量 */
//add end
	uint16_t port;
	zy_epoll_t *epoll;
	zy_mempool_t mempool;
}__attribute((aligned (4)));

typedef struct zy_tcpserver_peer_s zy_tcpserver_peer_t;
typedef struct zy_tcpserver_peer_mempool_s zy_tcpserver_peer_mempool_t;
struct zy_tcpserver_peer_s {
	epoll_node_t epoll_node;
	int socket_fd;
	zy_tcpserver_t *tcpserver;
	zy_tcpserver_peer_mempool_t *peer_mempoolnode;
	int (*msg_handle)(zy_epoll_t* epoll,struct epoll_event *event);    //每个链接的消息处理函数
	int (*do_protocol)(int fd, char* buf, int count);
}__attribute((aligned (4)));

//tcpserver_peer内存池节点

struct zy_tcpserver_peer_mempool_s {
	zy_mempoolnode_t memnode;
	zy_tcpserver_peer_t zy_tcpserver_peer;
};

//首先定义一个tcpserver，然后对它进行配置，最后create它
extern int zy_tcpserver_create(zy_tcpserver_t* tcpserver);
extern int zy_tcpserver_start(zy_tcpserver_t* tcpserver,zy_epoll_t* epoll);
extern int zy_tcpserver_accept(zy_epoll_t* epoll,struct epoll_event *event);
extern int zy_tcpserver_stop(zy_tcpserver_t* tcpserver,zy_epoll_t* epoll);
extern int zy_tcpserver_destory(zy_tcpserver_t* tcpserver);
extern int zy_tcpserver_msg_handle(zy_epoll_t* epoll,struct epoll_event *event);
extern int zy_tcpserver_config(zy_tcpserver_t* tcpserver,char* ip,uint16_t port,int maxclient,int (*do_protocol)(int fd,char* buf,int count));

#endif /* ZY_EPOLL_TCPSERVER_H_ */
