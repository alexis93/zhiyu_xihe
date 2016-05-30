/*-----------------------------------------------------------------
 *	iwise_hardware_data_module.c :
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
#include <unistd.h>

#include "iwise_loc_handle.h"
#include "iwise_loc_report_module.h"
#include "iwise_hardware_data_module.h"
#include "iwise_loc_base/iwise_output.h"
#include "iwise_loc_base/iwise_loc_base.h"
#include "iwise_loc_base/iwise_loc_type.h"
#include "iwise_loc_hardware_layer3/iwise_loc_hardware.h"
#include "iwise_net_log.h"
#include "iwise_loc_handle_layer2/iwise_net_rtk_module.h"

/*
 * 局部变量保存内部上下文
 * */
static iwise_hardware_data_context_t* hard_context;

/*
 *	参数		:	void * arg		I		
 * 	返回		:	void *
 * 	描述		:	该函数是解析线程的具体实现和执行部分
 * 	历史		:
 * 	备注		: 
 */
static void * hardware_handle(void * arg) 
{
	GPS_LOGD("parse_thread(hardware_handle) 初始化");
	fd_set readfd;
	fd_set fdset;
	int serial_fd;
	event_payload_u_type payload;

	char buff[4096];
	int count;
	char read_buf[64];

	/* 标签 */
parse_wait: 
    GPS_LOGD("parse_thread waiting...");
	pthread_mutex_lock(&context->status_mutex);
	while (context->status != IWISE_STATUS_START) {
		pthread_cond_wait(&context->status_cond, &context->status_mutex);
        if (context->status == IWISE_STATUS_STOP) {
			GPS_LOGD("parse_thread stop");
			pthread_mutex_unlock(&context->status_mutex);
            goto parse_wait;
		}
		if (context->status == IWISE_STATUS_DESTROY) {
			GPS_LOGI("parse_thread 退出");
			pthread_mutex_unlock(&context->status_mutex);
			return (void *)0;
		}
	}
	pthread_mutex_unlock(&context->status_mutex);

	GPS_LOGD("parse_thread 被激活，开始从串口读取数据");

	serial_fd = iwise_hardware_open(GPS_DEVICE);
	set_compos_fd(serial_fd); //TODO need add write lock
	if (serial_fd < 0) {
		GPS_LOGE("open serial_fd failed\n");
		return NULL;
	}
	GPS_LOGV("open serial_fd success\n");
	
	FD_ZERO(&fdset);
	FD_SET(serial_fd, &fdset);
	struct timeval timeout;
	int ret;
	int nmea_ret;

	while (1) {
		count = 0;
		readfd = fdset;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		ret = select(serial_fd + 1, &readfd, NULL, NULL, &timeout);
        
        /* 检查此时驱动的状态 */
        pthread_mutex_lock(&context->status_mutex);
		if (context->status == IWISE_STATUS_STOP) {
			GPS_LOGD("parse_thread stop");
            pthread_mutex_unlock(&context->status_mutex);
			iwise_hardware_close(GPS_DEVICE, serial_fd);
			goto parse_wait;
		}
		if (context->status == IWISE_STATUS_DESTROY) {
			GPS_LOGD("parse_thread 退出");
            pthread_mutex_unlock(&context->status_mutex);
            iwise_hardware_close(GPS_DEVICE, serial_fd);
			return (void*) 0;
		}
        pthread_mutex_unlock(&context->status_mutex);
        
		if (ret == -1) {
			/*TODO 当发生错误时应该如何处理，这里只是简单地继续运行*/
			GPS_LOGE("iwise_hardware_data_module select error\n");
			continue;
		} else if (ret > 0 && FD_ISSET(serial_fd, &readfd)) {
			//sleep(1);
			count = iwise_hardware_read(GPS_DEVICE, serial_fd, read_buf,
					sizeof(read_buf));
			if (count==0) {

				continue;
			}
			if (count < 0) {
				GPS_LOGV("iwise_hardware_read count %d", count);
				if (errno == EINTR)
					continue;
				else
					break;
			} else if (count > 0) {
			#if defined WRITE_LOG
				/* 记录日志文件raw */
				output_rawf(context->log_context.raw_fd, read_buf, count);
			#endif
				int i = 0, ret = 0;
				//GPS_LOGV("iwise_hardware_read count %d", count);
				for (i = 0; i < count; i++) {
					ret = iwise_serial_parser_input(GPS_DEVICE,
							&(hard_context->serial_parser), read_buf[i]);
					//GPS_LOGD("ret=%d,buf=%c \n",ret,read_buf[i]);
					if(ret < IWISE_SERIAL_PARSER_NULL){
						/* 解析错误，数据不完整 */
						GPS_LOGW("iwise_serial_parser_input error [%d]", ret);
					}else if(ret == IWISE_SERIAL_PARSER_NULL){
						//GPS_LOGV("Get a NMEA char");
						continue;
					}else if (ret >= IWISE_SERIAL_PARSER_NMEA) { 

					/* 解析出 NMEA GGA 数据后,如果是单点定位模式就上报 */
						if(context->mode == GPS_POSITION_MODE_STANDALONE 
							&& ret == IWISE_SERIAL_PARSER_NMEA_GGA){
							iwise_loc_report_nmea(&(hard_context->serial_parser.nmea_parser));
						#if defined WRITE_LOG
						/* 记录日志文件nmea */
							output_nmea_gga(context->log_context.nmea_fd, &(hard_context->serial_parser.nmea_parser));
			    		#endif

						}
						/* 解析出 NMEA GGA 数据后,如果是xihe定位模式就上报  */
						if(context->mode == GPS_POSITION_MODE_XIHE 
							&& ret == IWISE_SERIAL_PARSER_NMEA_GGA){
							iwise_loc_report_nmea(&(hard_context->serial_parser.nmea_parser));
					#if defined WRITE_LOG
						/* 记录日志文件nmea */
					   output_nmea_gga(context->log_context.nmea_fd, &(hard_context->serial_parser.nmea_parser));
					#endif
						}
						continue;
						
					} else if (ret >= IWISE_SERIAL_PARSER_GNSS_RAW) { /*TODO 如果是原始观测数据*/
						if (ret == IWISE_SERIAL_PARSER_GNSS_OBS) {
							/* 解析出原始观测数据后，拷贝到公共上下文中，置 obs_flag 为 1
							 * 并通过 resolve_cond 信号量通知“结算模块”进行结算
							 **/
							//GPS_LOGV("Get a obs packet");
							pthread_mutex_lock(&context->data_mutex);
							/*TODO 看这里是直接能拷贝的吗*/
							memcpy(&context->gnss_obs,
									&hard_context->serial_parser.raw_parser.obs,
									sizeof(iwise_gnss_obs_t));
							//send_obs_netlog(&(context->gnss_obs));
							send_obs2eph_netlog_ex(&(context->gnss_obs),&(context->gnss_nav));
							//netlog.send_obs(&hard_context->serial_parser.raw_parser.obs,sizeof(iwise_gnss_obs_t));
							//netlog.send_obs("1234567890\n",11);
							context->obs_flag = 1;
							pthread_mutex_unlock(&context->data_mutex);
							pthread_cond_signal(&context->resolve_cond);
						} else if (ret == IWISE_SERIAL_PARSER_GNSS_NAV) {
							/* 解析出星历数据后，拷贝到公共上下文中，并置 nav_flag 为 1 */
							//GPS_LOGV("Get a nav packet nstar : %d", hard_context->serial_parser.raw_parser.nav.n);
							pthread_mutex_lock(&context->data_mutex);
							memcpy(&context->gnss_nav,
									&hard_context->serial_parser.raw_parser.nav,
									sizeof(iwise_gnss_nav_t));
							//send_eph_netlog(&(context->gnss_nav));
							send_obs2eph_netlog_ex(&(context->gnss_obs),&(context->gnss_nav));
							context->nav_flag = 1;
							pthread_mutex_unlock(&context->data_mutex);
						}
						hard_context->serial_parser.is_raw = false;
					}
				}
			}
		}
	}
	return 0;
}

/*
 *	参数		:	void
 * 	返回		:	int		0:初始化成功
 * 	描述		:	初始化解析模块
 * 	历史		:
 * 	备注		: 
 */
int iwise_hardware_module_init() 
{
	GPS_LOGV("iwise_hardware_module_init");
	assert(context != NULL);
	context->hardware_context = malloc(
			sizeof(iwise_hardware_data_context_t) * 1);
	assert(context->hardware_context != NULL);
	iwise_serial_parser_init(&context->hardware_context->serial_parser);
	pthread_create(&context->hardware_context->hardware_thread, NULL,
			hardware_handle, NULL);
	hard_context = context->hardware_context;
	return 0;
}

/*
 *	参数		:	void
 * 	返回		:	int		0:初始化成功
 * 	描述		:	销毁解析模块
 * 	历史		:
 * 	备注		: 
 */
int iwise_hardware_module_destroy() 
{
	GPS_LOGV("iwise_hardware_module_destroy");
	pthread_join(hard_context->hardware_thread, NULL);
	iwise_serial_parser_destroy(&(hard_context->serial_parser));
	if(hard_context){
		free(hard_context);
	}
	return 0;
}
