#ifndef _IWISE_NET_MODULE_H_
#define _IWISE_NET_MODULE_H_

#include "iwise_loc_base/Parserlib/rtklib.h"

typedef struct {
	stream_t stream_ntrip;
	int com_flag;
	int nmea_flag;
	pthread_t netrtk_thread; /* 的线程 */
	pthread_cond_t net_cond; /* 网络状态条件变量，由iwise_loc_pgps_data_conn_open发出信号 */
	pthread_mutex_t data_mutex; /* 对pgps_server_addr(port),net_status进行保护 */
} iwise_net_rtk_context_t;


/*
 * 初始化网络模块
 * 在该初始化函数中会启动网络线程来获取改正数
 * */
int iwise_net_module_init();
int iwise_net_module_destroy();
void set_pos_netrtk(unsigned char* nmea,int len);
void set_compos_fd(int fd);

#endif
