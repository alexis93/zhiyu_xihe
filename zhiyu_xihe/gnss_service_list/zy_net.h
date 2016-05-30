/*
 * zy_net.h
 *
 *  Created on: 2014年6月4日
 *      Author: zhuyong
 */

#ifndef ZY_NET_H_
#define ZY_NET_H_

#include <stdint.h>
#include <sys/epoll.h>
#include "zy_mempool.h"
#define MAX_BUFFER 4096


typedef struct zy_event_data_s 	zy_event_data_t;
typedef struct epoll_s epoll_t;
struct zy_event_data_s {
	zy_mempoolnode_t memnode;
	int fd;			// socket的fd
	void *client;	// 直接关联client,这样不用去遍历client来判断了
	struct epoll_event ev;
	void (*handle)(zy_event_data_t * ev_data, uint32_t events);
};


struct epoll_s {
	zy_mempool_t mempool;
	struct epoll_event *events;
	int epoll_fd;
	int max_events;
	zy_mempool_t *event_data_pool;
};

extern int create_epoll(epoll_t *epoll,int max_events);
extern int run_epoll(epoll_t *epoll);
extern int add_epoll_listen(epoll_t *epoll,zy_event_data_t **epoll_event,int sock_fd,void *handle_event,void *socket_data);


typedef struct udp_server_s udp_server_t;
typedef struct udp_server_client_s udp_server_client_t;
//这里用类似类继承的方式，做struct继承，这样做指针转换时可以用父结构的指针调用。
struct udp_server_client_s {
	zy_mempoolnode_t memnode;
	char * ip;
	uint16_t port;
	int client_fd;
	udp_server_t *udp_server;
	zy_event_data_t *clientepoll;
	void *client_event;
	void *clientdata;
};

//这里用类似类继承的方式，做struct继承，这样做指针转换时可以用父结构的指针调用。
struct udp_server_s {
	zy_mempool_t mempool;
	char *ip;
	uint16_t port;
	int server_fd;
	epoll_t *epoll;
	zy_event_data_t *serverepoll;
	int maxclient;	
	void *udp_event;
};


extern void udp_handle_event(udp_server_t *udp_server, uint32_t events);
extern int accept_client(epoll_t *epoll,udp_server_t *udp_server);
extern int start_udp_server_listen(epoll_t *epoll,udp_server_t *udp_server);
extern int config_udp_server(udp_server_t *udp_server);
extern int create_udp_server(udp_server_t *udp_server);





#endif /* ZY_NET_H_ */
