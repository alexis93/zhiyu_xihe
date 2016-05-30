/*
 * zy_epoll.c
 *
 *  Created on: Jul 6, 2015
 *      Author: zhuyong
 */
#include "zy_common.h"
#include "zy_epoll.h"
//#include "zy_udp.h"
#include "zy_tcpserver.h"



int zy_epoll_create(zy_epoll_t *epoll,int max_events){
	//创建epoll
	epoll->epoll_fd = epoll_create(max_events);

	if (epoll->epoll_fd == -1) {
//		zlog_fatal(SERVERLOG, "create epoll failed!");
		return -1;
	}

//	zlog_debug(SERVERLOG, "create epoll ok!");
	epoll->max_events=max_events;

	//创建接收事件的事件队列
	epoll->events = (struct epoll_event *) calloc(epoll->max_events,sizeof(struct epoll_event));
	if (epoll->events==NULL){
		return -3;
	}
	return 0;
}

int zy_epoll_destory(zy_epoll_t *epoll){
	free(epoll->events);
	epoll->events=NULL;
	close(epoll->epoll_fd);
	return 0;
}

int zy_epoll_run(zy_epoll_t *epoll)
{
	int nready;
//	zlog_debug(SERVERLOG, "epoll is running!");

	while (1)
	{



		nready = epoll_wait(epoll->epoll_fd, epoll->events, epoll->max_events, -1);
		if (nready == -1) {
//			zlog_fatal(SERVERLOG, "epoll_pwait");
			return -1;
		}

		int i;
		//printf("nready = %d\n",nready);//if(nready>2) exit(0);
		for (i = 0; i < nready; i++) {
			epoll_node_t *node = epoll->events[i].data.ptr;

			//执行事件处理函数
			if (node->epoll_node_type==1) {
				//处理接收连接消息
				(((zy_tcpserver_t*)node)->accept_handle)(epoll,&(epoll->events[i]));
				//print_all_usernode(&(((zy_tcpserver_t*)node)->mempool));
			}
			else if (node->epoll_node_type==2){
				//处理客户端的协议消息
				(((zy_tcpserver_peer_t*)node)->msg_handle)(epoll,&(epoll->events[i]));
			}
			else if (node->epoll_node_type==3){
			//	(((zy_udp_t*)node)->msg_handle)(epoll,&(epoll->events[i]));

			}
		}
		sleep(0);
	}
	return nready;
}

