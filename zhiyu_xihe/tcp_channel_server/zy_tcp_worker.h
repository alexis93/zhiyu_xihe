/*
 * zy_tcp_worker.h
 *
 *  Created on: Jul 26, 2015
 *      Author: root
 */
#ifndef ZY_TCP_WORKER_H_
#define ZY_TCP_WORKER_H_


#include "zy_config.h"
#include "zy_tcpserver.h"

typedef struct zy_channel_s zy_channel_t;
struct zy_channel_s {
	channel_config_t *channel_config;
	zy_tcpserver_t rcv_tcpserver;
	zy_tcpserver_t send_tcpserver;
	//int(* send_msg)(char* buf,int len);
};
extern zy_channel_t channels[MAX_CHANNEL];

int tcp_server_init(void);
int tcp_server_run(void);


#endif /* ZY_TCP_WORKER_H_ */
