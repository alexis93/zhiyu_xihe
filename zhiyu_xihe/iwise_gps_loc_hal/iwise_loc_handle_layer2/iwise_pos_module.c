/*-----------------------------------------------------------------
 *	iwise_pos_module.c :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#include <pthread.h>
#include <errno.h>
#include <assert.h>

#include "iwise_pos_module.h"
#include "iwise_loc_handle.h"
#include "iwise_loc_report_module.h"
#include "iwise_loc_base/iwise_output.h"
#include "iwise_loc_base/iwise_loc_base.h"
#include "iwise_loc_base/iwise_loc_type.h"
#include "iwise_loc_base/Parserlib/gps_common.h"

#define MESSAGE_LENGTH 4096
/*
 * 局部变量保存内部上下文
 * */
static iwise_pos_context_t* pos_context;

/*
 *	参数		:	void * arg		I		线程参数
 * 	返回		:	void *
 * 	描述		:	解算线程的具体执行部分
 * 	历史		:
 * 	备注		: 
 */
static void * pos_handle(void * arg) 
{
	int ret = 0 ;
	int timeout = 0;
	struct timeval now;
	struct timespec ts;
	iwise_gnss_obs_t obs;
	iwise_gnss_nav_t nav;
	iwise_clockorbit_t clockorbit;
	iwise_sol_t sol;
	iwise_ssat_t ssat[MAXSAT];

	char log_buf[2048] = {0};
	int  len;
	iwise_message_t msg[1];
	int pos_ret = 0;
	GPS_LOGD("resolve_thread 初始化");

	ret = pnt_pos_init("/data/zhiyu/ctrl.txt");
	if(ret == 0) {
		GPS_LOGD("pnt_pos_init success!");
	} else {
		GPS_LOGE("pnt_pos_init failed. the path of config file maybe is error or nothing !");
		return (void *)0;
	}

resolve_wait:
	GPS_LOGD("resolve_thread waiting...");

	pthread_mutex_lock(&context->status_mutex);
	while (context->status != IWISE_STATUS_START) {
		pthread_cond_wait(&context->status_cond, &context->status_mutex);
		
		if (context->status == IWISE_STATUS_STOP) {
			GPS_LOGD("resolve_thread stop");
			pthread_mutex_unlock(&context->data_mutex);
		}
		if (context->status == IWISE_STATUS_DESTROY) {
			pthread_mutex_unlock(&context->status_mutex);
			GPS_LOGD("resolve_thread 退出");
			// TODO  退出之前应该做一些清扫工作
			return 0;
		}
	}
	pthread_mutex_unlock(&context->status_mutex);
	GPS_LOGD("resolve_thread 被激活");

	while (1) {

		pthread_mutex_lock(&context->data_mutex);
		gettimeofday(&now, NULL);

		/* gettimeofday在while循环外面，
		 * 两秒内必定退出循环
		 * 超时为break
		 * 被唤醒为continue
		 * */
		while (!context->obs_flag || !context->nav_flag) {
			/*设置两秒超时时间*/
			ts.tv_sec = now.tv_sec + 2;
			ts.tv_nsec = now.tv_usec * 1000UL;
			ret = pthread_cond_timedwait(&context->resolve_cond,
					&context->data_mutex, &ts);
			if (context->status == IWISE_STATUS_STOP) {
				GPS_LOGD("resolve_thread stop");
				pthread_mutex_unlock(&context->data_mutex);
				goto resolve_wait;
			}
			if (context->status == IWISE_STATUS_DESTROY) {
				pthread_mutex_unlock(&context->data_mutex);
                pnt_pos_destroy();
				GPS_LOGD("resolve_thread 退出");
				return (void*) 0;
			}
			if (ret == 0) {
				continue;
			} else if (ret == ETIMEDOUT) {
				/* 超时了 */
				timeout = 1;
				break;
			}
		}
		if (timeout == 1) {
			GPS_LOGD("resolve timeout");
			pthread_mutex_unlock(&context->data_mutex);
			/*超时后产生一个错误的定位*/
			timeout = 0;
		} else {
			//GPS_LOGD("resolve actived");
			memcpy(&obs, &context->gnss_obs, sizeof(obs_t));
			memcpy(&nav, &context->gnss_nav, sizeof(nav_t));
			memcpy(&clockorbit, &context->clockorbit, sizeof(iwise_clockorbit_t));
			context->obs_flag = 0;
			pthread_mutex_unlock(&context->data_mutex);
		#if defined WRITE_LOG
			output_obs2eph(context->log_context.eo_fp, &obs, &nav);
			output_ephf(context->log_context.eph_fp, &nav, nav.n);
		#endif
			memset(msg, 0, sizeof(iwise_message_t));
			memset(log_buf, 0, 2048);
			//GPS_LOGD("pnt_pos_sol start");
            pos_ret = pnt_pos_sol(&obs, &nav, &prcopt_default, &clockorbit, &sol,
					ssat, (char *)msg);
            //GPS_LOGD("pnt_pos_sol end");
			//GPS_LOGD("[spp_std]%lf [ppp_std]%lf [corr_time]%ld [gps_time]%ld [pos_freq]%d", msg->spp_std, msg->ppp_std, msg->corr_time, msg->gps_time, msg->pos_freq);
			//GPS_LOGD("[INFO_MSG] %s", msg->info_msg);
		#ifdef MAIN_EXE
			iwise_loc_report_message(msg);
		#endif
		//#if defined WRITE_LOG
		#if defined GPS_DRIVER_LOG
			len = sprintf(log_buf, "[INFO_MSG]%s [spp_std]%lf [ppp_std]%lf [corr_time]%ld [gps_time]%ld [pos_freq]%d", 
							msg->info_msg, msg->spp_std, msg->ppp_std, msg->corr_time, msg->gps_time, msg->pos_freq);
			output_log(context->log_context.gps_log_fp, log_buf, len);
		#endif
			if (pos_ret == 1) {
				GPS_LOGD("pnt_pos_sol failed [%s]\n", msg->error_msg);
			#if defined WRITE_LOG
				output_resultf(context->log_context.restore_fd, &clockorbit, &obs, &nav, NULL, NULL);
			#endif
				memset(&sol, 0, sizeof(sol_t));
			} else if ((pos_ret != 0) && (pos_ret != 1)) {
				GPS_LOGD("pnt_pos_sol error \n");
			#if defined WRITE_LOG
				output_resultf(context->log_context.restore_fd, &clockorbit, &obs, &nav, NULL, NULL);
				//output_restoref(context->log_context.restore_fd, &obs, &clockorbit, msg->error_msg);
			#endif
				memset(&sol, 0, sizeof(sol_t));
			} else if (pos_ret == 0) {
			#if defined WRITE_LOG
				output_resultf(context->log_context.restore_fd, &clockorbit, &obs, &nav, NULL, NULL);
				output_lon_lat_altf(context->log_context.longlat_fp, &sol);
			#endif
				if(context->mode == GPS_POSITION_MODE_XIHE){
					iwise_loc_report_sol(&sol);
					#ifdef SOL_INS
					iwise_loc_report_sol_ins(&sol);
					#endif
				}
			}
		}
	}
}

/*
 *	参数		:	void
 * 	返回		:	int		0:初始化成功
 * 	描述		:	初始化解算线程
 * 	历史		:
 * 	备注		: 	在使用楼老师的定位解算库的时候，需要初始化配置文件，
 * 				而该配置文件路径是很重要的，在本版本驱动中，该配置
 * 				文件是存放在/data/zhiyu/ctrl.txt，如果没有
 * 				该配置文件或文件路径错误，均会导致驱动出问题。
 */
int iwise_pos_module_init() 
{
	assert(context != NULL);
	context->pos_context = malloc(sizeof(iwise_pos_context_t) * 1);
	assert(context->pos_context != NULL);
	pos_context = context->pos_context;
	strncpy(pos_context->ctrl_file_path, "/data/zhiyu/ctrl.txt", 100);
	pthread_create(&pos_context->pos_thread, NULL, pos_handle, NULL);
	return 0;
}

/*
 *	参数		:	void
 * 	返回		:	int		0:销毁成功
 * 	描述		:	销毁解算线程
 * 	历史		:
 * 	备注		: 
 */
int iwise_pos_module_destroy() 
{
	GPS_LOGD("iwise_pos_module_destroy destroy is running");
	pthread_join(pos_context->pos_thread, NULL);
	free(pos_context);
	return 0;
}
