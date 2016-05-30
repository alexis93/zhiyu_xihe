/*
 * zy_epoll.h
 *
 *  Created on: Jul 6, 2015
 *      Author: zhuyong
 */

#ifndef ZY_EPOLL_H_
#define ZY_EPOLL_H_
 #include <sys/epoll.h>

typedef struct zy_epoll_s zy_epoll_t;
struct zy_epoll_s {
	int epoll_fd;
	int max_events;
	struct epoll_event *events;
};

typedef struct epoll_node_s epoll_node_t;
struct epoll_node_s {
	int epoll_node_type; //1-tcpserver,2-tcpserver_peer
	struct epoll_event ev; //存放事件配置
};

extern int zy_epoll_create(zy_epoll_t *epoll,int max_events);//创建epoll
extern int zy_epoll_run(zy_epoll_t *epoll);     //运行epoll
extern int zy_epoll_destory(zy_epoll_t *epoll); //销毁epoll


#endif /* ZY_EPOLL_H_ */
