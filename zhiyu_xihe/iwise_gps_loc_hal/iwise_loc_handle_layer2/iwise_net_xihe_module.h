/*-----------------------------------------------------------------
 *	iwise_net_xihe_module.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#ifndef _IWISE_NET_XIHE_MODULE_H_
#define _IWISE_NET_XIHE_MODULE_H_

#include "iwise_loc_base/iwise_loc_type.h"
#include "iwise_loc_base/iwise_serial_parser.h"

/*
 * 硬件数据获取模块的上下文结构体
 * */
typedef struct {
	iwise_serial_parser_t serial_parser;  /* 硬件数据解析器*/
	pthread_t network_thread; /* 精密导航电文获取模块的线程 */
	int net_socket; /* 精密导航电文获取的模块与服务器的链接的socket */
	char pgps_main_server_addr[20]; /* 主服务器的地址 */
	int pgps_main_server_port; /* 主服务器的端口号 */
	char pgps_extension_server_addr[20]; /* 扩展服务器的地址 */
	int pgps_extension_server_port; /* 扩展服务器的端口号 */
	int net_status; /* 网络状态，是否联网，由iwise_loc_pgps_data_conn_open（close）设置*/
	iwise_cert* iwise_net_cert; /* authentication certificate */
	pthread_cond_t net_cond; /* 网络状态条件变量，由iwise_loc_pgps_data_conn_open发出信号 */
	pthread_mutex_t server_mutex; /* 对pgps_server_addr(port),net_status进行保护 */
	iwise_pool_t* pool;
} iwise_net_xihe_context_t;

/*
 * 初始化网络模块
 * 在该初始化函数中会启动网络线程来获取改正数
 * */
int iwise_net_xihe_module_init();
int iwise_net_xihe_module_destroy();

#endif
