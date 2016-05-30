/*
 * zy_tcpclient.h
 *
 *  Created on: Dec 15, 2015
 *      Author: root
 */

#ifndef ZY_TCPCLIENT_H_
#define ZY_TCPCLIENT_H_
#include <stdint.h>

typedef struct zy_tcpclient_s zy_tcpclient_t;
struct zy_tcpclient_s {
	char tcpname[40];
	int socket_fd;
	int (*do_protocol)(int fd,char* buf,int count);        //处理TCP连接的协议部分，在复用的时候，需要程序员编写
	char *ip;
	uint16_t port;
}__attribute((aligned (4)));

//tcpserver_peer内存池节点

//首先定义一个tcpclient，然后对它进行配置，最后create它

extern int zy_tcpclient_start(zy_tcpclient_t* tcpclient);
extern int zy_tcpclient_send(zy_tcpclient_t* tcpclient,char* buf,int len);
extern int zy_tcpclient_stop(zy_tcpclient_t* tcpclient);
extern int zy_tcpclient_config(zy_tcpclient_t* tcpclient,char* name,char* ip,uint16_t port,int (*do_protocol)(int fd,char* buf,int count));



#endif /* ZY_TCPCLIENT_H_ */
