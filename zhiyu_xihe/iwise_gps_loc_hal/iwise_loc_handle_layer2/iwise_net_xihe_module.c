/*-----------------------------------------------------------------
 *	iwise_net_xihe_module.c :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#include <pthread.h>
#include <sys/select.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>


#include "iwise_loc_handle.h"
#include "iwise_net_xihe_module.h"
#include "iwise_loc_base/iwise_output.h"
#include "iwise_loc_base/iwise_loc_base.h"
#include "iwise_loc_base/iwise_loc_type.h"

/*
 * 局部变量保存全局上下文和内部上下文
 * */
static iwise_net_xihe_context_t* xihe_context;

/*
 *	参数		:	struct ClockOrbit *clockorbit		I		线程参数
 * 				int port							I		服务器端口
 * 				char *addr							i		服务器地址
 * 	返回		:	int		0：连接服务器成功	-1：连接服务器失败
 * 	描述		:	从网络获取改正数的具体实现
 * 	历史		:
 * 	备注		: 	武大钟差服务器IP:58.49.58.148 port:8001
 * 	            aliyun钟差服务器IP:120.27.28.228 port:8001
 */
static int connect_server(iwise_clockorbit_t *clockorbit, int port, char *addr)
{
	struct sockaddr_in server_addr;

	if(xihe_context->net_socket != 0){
		close(xihe_context->net_socket);
		xihe_context->net_socket = 0;
	}
	if((xihe_context->net_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		GPS_LOGE("create clockorbit server socket failed !");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(addr);
	server_addr.sin_port = htons(port);
	
	/*
	 * 将socket设置为非阻塞
	 * 使用select设置connect超时时间为2s
	 * 连接成功后再将socket设置为阻塞的
	 * 这样能够保证connect函数在2s内返回
	 * */
	int flag = fcntl(xihe_context->net_socket, F_GETFL, 0);
	fcntl(xihe_context->net_socket, F_SETFL, flag|O_NONBLOCK);
	
	int ret = connect(xihe_context->net_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if(ret < 0){
		if(errno != EINPROGRESS && errno != EWOULDBLOCK){
			GPS_LOGE("connect clockorbit server error [%s : %d]", addr, port);  
			return -1;
		}
		
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		fd_set wset;
		FD_ZERO(&wset);
		FD_SET(xihe_context->net_socket, &wset);
		int n = select(xihe_context->net_socket+1, NULL, &wset, NULL, &tv);
		if(n<0){
			GPS_LOGE("connect clockorbit server error in select ! ");
			close(xihe_context->net_socket);
			return -1;
		}else if(n==0){
			GPS_LOGW("connect clockorbit server timeout in select !!");
			close(xihe_context->net_socket);
			return -1;
		}else{
			int len = sizeof(int);
			int err = 0;
			getsockopt(xihe_context->net_socket, SOL_SOCKET, SO_ERROR, &err, &len);
			if(err != 0) {
				GPS_LOGE("connect clockorbit server error in getsockopt! ");
				return -1;
			}
		}
	}
	flag = fcntl(xihe_context->net_socket, F_GETFL, 0);
	fcntl(xihe_context->net_socket, F_SETFL, flag&~O_NONBLOCK);
	GPS_LOGD("clockorbit success server:%s : %d", addr, port);

	/* 设置接收延时时间 
	 * 防止网络线程在recv函数一直阻塞
	 * */
    int status = 0;
	struct timeval tv = {2, 0};
	status = setsockopt(xihe_context->net_socket, SOL_SOCKET, SO_RCVTIMEO,
			(char *) &tv, sizeof(struct timeval));
	if(status < 0) {
		GPS_LOGD("setsockopt recv failed!");
	}
	return 0;
}

/*
 *	参数		:	void *arg		I		线程参数
 * 	返回		:	void *
 * 	描述		:	从网络获取改正数的具体实现
 * 	历史		:
 * 	备注		: iwise_inter_event_cb
 */
static void * xihe_handle(void * arg) 
{
	GPS_LOGD("network_thread 初始化");

	int time_count = 0;
	int ret = 0;
	/* 收到的改正数数据包错误的次数 */
	int parse_error_time = 0;
	iwise_clockorbit_t iwise_clockorbit_temp;

	/* 保存上次的服务器设置，用来判断服务器是否改变了 */
	char addr[20];
	int port;
	
network_wait:
	GPS_LOGD("network_thread waiting...");
	pthread_mutex_lock(&context->status_mutex);
	while (context->status != IWISE_STATUS_START) {
		/* 初始化后如果没有开始定位则等待 */
		pthread_cond_wait(&context->status_cond, &context->status_mutex);
		if (context->status == IWISE_STATUS_STOP) {
			GPS_LOGD("network_thread stop");
			pthread_mutex_unlock(&context->status_mutex);
			goto network_wait;
		}
		if (context->status == IWISE_STATUS_DESTROY) {
			pthread_mutex_unlock(&context->status_mutex);
			GPS_LOGD("network_thread 退出");
			// TODO  退出之前应该做一些清扫工作
			return 0;
		}
	}
	pthread_mutex_unlock(&context->status_mutex);

	GPS_LOGD("network_thread 被激活");

	/*default 服务器IP,port*/
connect_again: 
	time_count = 0;
	parse_error_time = 0;
	memset(&iwise_clockorbit_temp, 0, sizeof(iwise_clockorbit_t));
	GPS_LOGD("mainserver: %s %d", xihe_context->pgps_main_server_addr, 
				xihe_context->pgps_main_server_port );
				
	strncpy(addr, xihe_context->pgps_main_server_addr, 20);
	port = xihe_context->pgps_main_server_port;

	/* 尝试连接服务器，连接失败后检查驱动的状态 */
	while((ret = connect_server(&iwise_clockorbit_temp, port, &addr)) == -1){
		sleep(1);
        pthread_mutex_lock(&context->status_mutex);
		if (context->status == IWISE_STATUS_STOP) {
			pthread_mutex_unlock(&context->status_mutex);
			GPS_LOGD("network_thread stop");
			close(xihe_context->net_socket);
			xihe_context->net_socket = 0;
			goto network_wait;
		}
		if (context->status == IWISE_STATUS_DESTROY) {
			pthread_mutex_unlock(&context->status_mutex);
			GPS_LOGD("network_thread 退出");
			return (void*) 0;
		}
		pthread_mutex_unlock(&context->status_mutex);
		
		/* 连接失败后再重新复制服务器，防止上层修改了服务器设置 */
		strncpy(addr, xihe_context->pgps_main_server_addr, 20);
		port = xihe_context->pgps_main_server_port;
	}

	GPS_LOGD("connect clockorbit server success!\n");
	iwise_clockorbit_t clockorbit;
	memset(&clockorbit, 0, sizeof(iwise_clockorbit_t));

	int bytesused = 1;
	int count = 0;
	/*timeout times*/
	int times = 0;

	iwise_pool_t* pool = xihe_context->pool;
	assert(pool != NULL);

	while (1) {
		count = recv(xihe_context->net_socket, pool->buffer + pool->end, PACKAGE_SIZE, 0);
		pthread_mutex_lock(&context->status_mutex);
		if (context->status == IWISE_STATUS_STOP) {
			pthread_mutex_unlock(&context->status_mutex);
			GPS_LOGD("network_thread stop");
			close(xihe_context->net_socket);
			xihe_context->net_socket = 0;
			goto network_wait;
		}
		if (context->status == IWISE_STATUS_DESTROY) {
			pthread_mutex_unlock(&context->status_mutex);
			GPS_LOGD("network_thread 退出");
			return (void*) 0;
		}
		pthread_mutex_unlock(&context->status_mutex);
		//GPS_LOGV("network_thread keep running");
		
		/* 此处删除对网络状况的检查，在此版驱动里一直尝试连接网络服务器 
		 * 检查服务器设置是否发生改变
		 * */
		pthread_mutex_lock(&xihe_context->server_mutex);
		if ((strlen(xihe_context->pgps_main_server_addr) != 0)&& 
			((strcmp(xihe_context->pgps_main_server_addr, addr) != 0)||
			 (xihe_context->pgps_extension_server_port != port))) {
			memcpy(addr, xihe_context->pgps_main_server_addr, 20);
			port = xihe_context->pgps_main_server_port;
			pthread_mutex_unlock(&xihe_context->server_mutex);

			if (xihe_context->net_socket != 0) {
				close(xihe_context->net_socket);
				xihe_context->net_socket = 0;
			}
			//GPS_LOGD("server changed! connect again!");
			goto connect_again;
		}

		pthread_mutex_unlock(&xihe_context->server_mutex);

		//GPS_LOGV("read from clockorbit server count : %d", count);
		if (count <= 0) {
			/*超时处理*/
			if (count == -1
					&& (errno == EAGAIN || errno == EWOULDBLOCK
							|| errno == EINTR)) {
				GPS_LOGW("read from data_source_sock timeout");
				times++;
				if (times > 9) {
					GPS_LOGE("timeout 10 times, close socket, reconnect!");
					close(xihe_context->net_socket);
					times = 0;
					goto connect_again;
				}
			} else {
				if (count == -1) {
					GPS_LOGE("read data error: %s", strerror(errno));
				}
				/*其它错误,不管是什么错误,重新连接*/
				close(xihe_context->net_socket);
				times = 0;
				goto connect_again;
			}
		} else {
		#if defined WRITE_LOG
			output_clockorbitf(context->log_context.clock_orbit_fd, pool->buffer + pool->end, count);
		#endif
			int length;
			int n;
			int parse_result = 0;
			pool->end += count;
			while (1) {
				if (pool->end - pool->start < 3) {
					//GPS_LOGD("no message is the buffer break");
					break;
				}
				while (pool->start <= pool->end) {
					if (pool->buffer[pool->start] == 0xD3) {
						break;
					}
					pool->start++;
				}
				length = (pool->buffer[pool->start + 1] << 8)
						| pool->buffer[pool->start + 2] + 6;
				if (length > PACKAGE_SIZE) {
					continue;
				}
				//GPS_LOGV("the data length is %d", length);
				if (pool->end - pool->start < length) {
					GPS_LOGV("less than mesage break;");
					break;
				}

				memset(&clockorbit, 0, sizeof(iwise_clockorbit_t));

				n = GetClockOrbitBias(&clockorbit, NULL,
						pool->buffer + pool->start, length, &bytesused);
				pool->start += length;
				if (n == GCOBR_OK) {
					//GPS_LOGD("parse clock ok");
					memcpy(&context->clockorbit, &clockorbit,
							sizeof(iwise_clockorbit_t));
					parse_result = 1;
				} else {
					parse_result = 0;
					pthread_mutex_lock(&context->data_mutex);
					memcpy(&context->clockorbit, &clockorbit,
							sizeof(struct ClockOrbit));
					pthread_mutex_unlock(&context->data_mutex);
					GPS_LOGD("parse clockorbit error");
					if (parse_error_time++ > 3) {
						parse_error_time = 0;
						close(xihe_context->net_socket);
						connect_server(&iwise_clockorbit_temp, port, addr);
						pool->start = 0;
						pool->end = 0;
						break;
					}
				}
			}
			if (parse_result == 1) {
				pthread_mutex_lock(&context->data_mutex);
				//GPS_LOGD("update the clockorbit");

				memcpy(&context->clockorbit, &clockorbit,
						sizeof(iwise_clockorbit_t));
				/*add by li to correct the time,will be much better*/
				if (context->clockorbit.GPSEpochTime
						!= iwise_clockorbit_temp.GPSEpochTime) {
					time_count = 0;        /*reset the time_count*/
					memcpy(&iwise_clockorbit_temp, &clockorbit,
							sizeof(iwise_clockorbit_t));
				} else {
					time_count++;
					if (time_count == 60) {
						memset(&context->clockorbit, 0,
								sizeof(iwise_clockorbit_t));
					}
				}

				pthread_mutex_unlock(&context->data_mutex);
				//GPS_LOGD("clockorbit recv OK");
				memset(&clockorbit, 0, sizeof(iwise_clockorbit_t));
			} else {
				GPS_LOGD("parse end or meet error");
			}

			iwise_pool_compress(pool);
		}
	}
}

/*
 *	参数		:	void
 * 	返回		:	int		0:初始化成功
 * 	描述		:	初始化从网络获取改正数的线程
 * 	历史		:
 * 	备注		: 
 */
int iwise_net_xihe_module_init() 
{
	assert(context != NULL);
	GPS_LOGD("iwise_net_xihe_module_init");
	context->xihe_context = malloc(
			sizeof(iwise_net_xihe_context_t) * 1);
	assert(context->xihe_context != NULL);
	xihe_context = context->xihe_context;
	xihe_context->pool = iwise_pool_init();
	
	/* 初始化时设置网络未开，让模块尝试连接10次 */
	xihe_context->net_status = IWISE_NET_OPEN;
	/* 初始化为默认的服务器地址 120.27.28.228*/

	strncpy(xihe_context->pgps_extension_server_addr, "120.27.28.228", 20);
	//strncpy(xihe_context->pgps_extension_server_addr, "58.49.58.148", 20);
	xihe_context->pgps_extension_server_port = 8001;
	
	pthread_mutex_init(&xihe_context->server_mutex, NULL);
	pthread_cond_init(&xihe_context->net_cond, NULL);
	pthread_create(&xihe_context->network_thread, NULL,
			xihe_handle, NULL);
	return 0;
}

/*
 *	参数		:	void
 * 	返回		:	int		0:初始化成功
 * 	描述		:	销毁从网络获取改正数的线程
 * 	历史		:
 * 	备注		: 
 */
int iwise_net_xihe_module_destroy() 
{
	assert(xihe_context != NULL);
	GPS_LOGD("iwise_net_xihe_module_destroy is runing...");
	pthread_join(xihe_context->network_thread, NULL);
	iwise_pool_destroy(xihe_context->pool);
	pthread_mutex_destroy(&xihe_context->server_mutex);
	pthread_cond_destroy(&xihe_context->net_cond);
	free(xihe_context);
	return 0;
}
