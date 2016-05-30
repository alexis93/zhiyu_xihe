/*-----------------------------------------------------------------
 *	iwise_loc_handle.c :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#include <assert.h>
#include <errno.h>

#include "iwise_loc_handle.h"
#include "iwise_pos_module.h"
#include "iwise_net_xihe_module.h"
#include "iwise_hardware_data_module.h"
#include "iwise_loc_base/iwise_output.h"
#include "iwise_loc_base/iwise_loc_type.h"

iwise_loc_context_t* context = NULL;

/*
 *	参数		:	event_cb_f_type event_cb		I		回调函数指针
 * 	返回		:	int		0:初始化成功	-1：初始化失败
 * 	描述		:	初始化定位驱动的各个模块	
 * 	历史		:
 * 	备注		: 8001
 */
int iwise_loc_init(event_cb_f_type event_cb) 
{
	int ret = 0;
    GPS_LOGD("iwise_loc_init");
	/*初始化各个模块*/
	while(context != NULL) {
		if (context->status == IWISE_STATUS_DESTROY) {
			/*等待线程结束，等待上次清理结束*/
			GPS_LOGD("wait for last destroy");
		}
		sleep(1);
	}
	
	context = malloc(sizeof(iwise_loc_context_t));
	memset(context, 0, sizeof(iwise_loc_context_t));
	
	assert(context != NULL);
	context->event_callback = event_cb;
	context->status = IWISE_STATUS_INIT;

	ret = pthread_mutex_init(&context->data_mutex, NULL);
    if(ret != 0) {
        GPS_LOGD("init data_mutex failed: %s", strerror(errno));
        return -1;
    } else {
        GPS_LOGD("init data_mutex success");
    }
	pthread_cond_init(&context->resolve_cond, NULL);
	ret = pthread_mutex_init(&context->status_mutex, NULL);
    if(ret != 0) {
        GPS_LOGD("init status_mutex failed: %s", strerror(errno));
        return -1;
    } else {
        GPS_LOGD("init status_mutex success");
    }
	pthread_cond_init(&context->status_cond, NULL);
#if defined WRITE_LOG
	ret = iwise_log_file_init(&(context->log_context));
	if(ret < 0) {
		GPS_LOGE("iwise_log_file_init failed");
	} else {
		GPS_LOGE("iwise_log_file_init success");
	}
#endif
//#ifndef	UBLOX
	iwise_net_module_init();
//#endif
	iwise_hardware_module_init();
	iwise_net_xihe_module_init();
	iwise_pos_module_init();


	return 0;
}

/*
 *	参数		:	void
 * 	返回	    :	int		0:驱动启动成功
 * 	描述		:	启动定位驱动
 * 	历史		:
 * 	备注		: 
 */
int iwise_loc_start() 
{
	GPS_LOGD("iwise_loc_start runing");
	pthread_mutex_lock(&context->status_mutex);
	context->status = IWISE_STATUS_START;
	GPS_LOGD("iwise_loc_start  net_status %d", context->xihe_context->net_status);
	pthread_cond_broadcast(&context->status_cond);
	pthread_mutex_unlock(&context->status_mutex);
	return 0;
}

/*
 *	参数		:	void
 * 	返回	    :	int		0:驱动停止成功
 * 	描述		:	停止定位
 * 	历史		:
 * 	备注		: 
 */
int iwise_loc_stop() 
{
	GPS_LOGD("iwise_loc_stop runing");
	GPS_LOGD("iwise_loc_stop net_status %d", context->xihe_context->net_status);
	pthread_mutex_lock(&context->status_mutex);
	context->status = IWISE_STATUS_STOP;
	pthread_mutex_unlock(&context->status_mutex);
	return 0;
}

/*
 *	参数		:	void
 * 	返回	    :	int		0:驱动销毁成功
 * 	描述		:	销毁定位，释放内存
 * 	历史		:
 * 	备注		: 
 */
void iwise_loc_destroy() 
{
	GPS_LOGD("iwise_loc_destroy runing");
	pthread_mutex_lock(&context->status_mutex);
	context->status = IWISE_STATUS_DESTROY;
	pthread_cond_broadcast(&context->status_cond);
	pthread_mutex_unlock(&context->status_mutex);
	
	GPS_LOGD("iwise_loc_destroy is going");
	iwise_hardware_module_destroy();
	GPS_LOGD("iwise_hardware_module_destroy in iwise_loc_destroy  destroy success!");
	iwise_net_xihe_module_destroy(); 
	GPS_LOGD("iwise_net_xihe_module_destroy in iwise_loc_destroy  destroy success!");
	iwise_pos_module_destroy();
	GPS_LOGD("iwise_pos_module_destroy in iwise_loc_destroy  destroy success!");
	
	context->status = IWISE_STATUS_NULL;
#if defined WRITE_LOG
	iwise_log_file_destroy(&(context->log_context));
#endif
	pthread_mutex_destroy(&context->status_mutex);
	pthread_mutex_destroy(&context->data_mutex);
	pthread_cond_destroy(&context->resolve_cond);
	GPS_LOGD("iwise_loc_destroy all success!!");
	free(context);
	context = NULL;
}

/*
 *	参数		:	GpsPositionMode mode		I		定位模式		
 * 	返回	    :	int		0:定位模式设置成功
 * 	描述		:	设置驱动定位模式
 * 	历史		:
 * 	备注		: 
 */
int iwise_loc_set_position_mode(GpsPositionMode mode) 
{
	pthread_mutex_lock(&context->data_mutex);
	context->mode = mode;
	pthread_mutex_unlock(&context->data_mutex);
	return 0;
}

/*
 *	参数		:	void
 * 	返回	    :	int		填充时间成功
 * 	描述		:	填充定位时间
 * 	历史		:
 * 	备注		: 	没有实现
 */
int iwise_loc_inject_time() 
{
	//TODO
	return 0;
}

/*
 *	参数		:	void
 * 	返回	    :	int		0:填充位置成功
 * 	描述		:	填充定位信息
 * 	历史		:
 * 	备注		: 	没有实现
 */
int iwise_loc_inject_location() 
{
	//TODO
	return 0;
}

/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	删除辅助信息
 * 	历史		:
 * 	备注		: 	没有实现
 */
void iwise_loc_delete_aiding_data() 
{
	//TODO
}

/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	初始化pgps
 * 	历史		:
 * 	备注		: 
 */
void iwise_loc_pgps_init() 
{
	//TODO
}

/*
 *	参数		:	void
 * 	返回		:	void	
 * 	描述		:	打开网络
 * 	历史		:
 * 	备注		: 
 */
void iwise_loc_pgps_data_conn_open() 
{
	if(context != NULL && context->xihe_context != NULL){
		pthread_mutex_lock(&context->xihe_context->server_mutex);
		context->xihe_context->net_status = IWISE_NET_OPEN;
		pthread_cond_broadcast(&context->xihe_context->net_cond);
		pthread_mutex_unlock(&context->xihe_context->server_mutex);
	}
}

/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	关闭网络
 * 	历史		:
 * 	备注		: 
 */
void iwise_loc_pgps_data_conn_closed() 
{
	if(context != NULL && context->xihe_context != NULL){
		pthread_mutex_lock(&context->xihe_context->server_mutex);
		context->xihe_context->net_status = IWISE_NET_CLOSE;
		pthread_cond_broadcast(&context->xihe_context->net_cond);
		pthread_mutex_unlock(&context->xihe_context->server_mutex);
	}
}

/*
 *	参数		:	iwise_server_t* main_server			I		服务器
 * 	返回		:	void
 * 	描述		:	设置服务器地址和端口
 * 	历史		:
 * 	备注		: 
 */
void iwise_loc_pgps_set_pgps_main_server(iwise_server_t* main_server) 
{
	if(context != NULL && context->xihe_context != NULL){
		pthread_mutex_lock(&context->xihe_context->server_mutex);
		memcpy(context->xihe_context->pgps_main_server_addr,
				main_server->iwise_server_addr, 20);
		context->xihe_context->pgps_main_server_port =
				main_server->iwise_server_port;
		pthread_mutex_unlock(&context->xihe_context->server_mutex);
	}
}

/*
 *	参数		:	iwise_server_t* extension_server		I		服务器
 * 	返回		:	void
 * 	描述		:	设置服务器地址
 * 	历史		:
 * 	备注		: 
 */
void iwise_loc_pgps_set_pgps_extension(iwise_server_t* extension_server) 
{
	if(context != NULL && context->xihe_context != NULL){
		pthread_mutex_lock(&context->xihe_context->server_mutex);
		memcpy(context->xihe_context->pgps_extension_server_addr,
				extension_server->iwise_server_addr, 20);
		context->xihe_context->pgps_extension_server_port =
				extension_server->iwise_server_port;
		pthread_mutex_unlock(&context->xihe_context->server_mutex);
	}
}

/*
 *	参数		:	extern_data_t* extern_data		I		扩展的数据
 * 	返回		:	void
 * 	描述		:	注入扩展数据
 * 	历史		:
 * 	备注		: 	没有实现
 */
void iwise_loc_pgps_inject_extern_data(extern_data_t* extern_data)
{

}
