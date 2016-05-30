/*-----------------------------------------------------------------
 *	iwise_loc_handle.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#ifndef _IWISE_LOC_H_
#define _IWISE_LOC_H_

#include <pthread.h>

#include "iwise_pos_module.h"
#include "iwise_net_xihe_module.h"
#include "iwise_hardware_data_module.h"
#include "iwise_loc_base/iwise_output.h"
#include "iwise_loc_base/iwise_loc_base.h"

#ifdef TRANS_HAL
#include "iwise_net_trans_module.h"
#endif

extern struct iwise_loc_context_s* context;

typedef struct iwise_loc_context_s {
	GpsPositionMode mode; /* 定位模式，由iwise_loc_set_position_mode设置 */
	int status; /* HAL层状态,由iwise_open,iwise_start_fix等设置*/
	pthread_mutex_t status_mutex; /* 保护status字段，保证对status的互斥修改 */
	pthread_cond_t status_cond; /* 状态改变条件变量 */
	
	iwise_hardware_data_context_t* hardware_context;
	iwise_net_xihe_context_t* xihe_context;
	iwise_pos_context_t* pos_context;

	iwise_clockorbit_t clockorbit; /* clock orbit */
	nav_message_t nav_message; /* 精密导航电文，属于共享数据区 */
	iwise_gnss_obs_t gnss_obs; /* 原始观测值(伪距观测值，载波相位观测值，多普洛观测值)，属于共享数据区 */
	iwise_gnss_nav_t gnss_nav; /* 广播星历，属于共享数据区 */
	//gnss_obsdata_report_t gnss_obsdata_report; /*待上传的原始观测数据，本实例采用RINEX格式来表示，属于共享数据区 */
	iwise_loc_nmea_gga_s_type nmea_gga; /* NMEA协议中的GGA消息，属于共享数据区 */
	iwise_loc_nmea_gsv_s_type nmea_gsv; /* NMEA协议中的GSV消息，属于共享数据区 */
	iwise_loc_nmea_report_s_type nmea_report; /* 待上传的NMEA协议中的原始数据，属于共享数据区 */
	int obs_flag; /* 标志位，0表示没有获取原始观测值，1表示获取原始观测值，属于共享数据区 */
	int nav_flag; /* 标志位 ，0表示没有获取广播星历，1表示获取了广播星历，属于共享数据区 */
	time_t last_receive_nav_message; /* 最后一次接受的精密导航电文的时间，属于共享数据区 */
	pthread_mutex_t data_mutex; /* 对共享数据区进行保护，保证多个模块对共享数据区的互斥访问 */
	pthread_cond_t resolve_cond; /* 定位解算条件变量 */
	GpsLocation* location_list; /* 最近定位结果列表，定位结果审核模块使用，本实例保存最近10次定位结果 */
	iwise_loc_location_size size; /* 最近定位结果列表中保存的定位结果数目 */
	event_cb_f_type event_callback; /* 回调函数，由iwise_open设置 */
	
#if defined WRITE_LOG
	iwise_log_file_t log_context;	/*记录日志文件 */
#endif

} iwise_loc_context_t;

// Function declarations for sLocEngInterface
int iwise_loc_init(event_cb_f_type event_callback);
int iwise_loc_start();
int iwise_loc_stop();
void iwise_loc_destroy();

int iwise_loc_set_position_mode(GpsPositionMode mode);
int iwise_loc_inject_time();
int iwise_loc_inject_location();
void iwise_loc_delete_aiding_data();

void iwise_loc_pgps_init();
void iwise_loc_pgps_data_conn_open();
void iwise_loc_pgps_data_conn_closed();
void iwise_loc_pgps_set_pgps_main_server(iwise_server_t* main_server);
void iwise_loc_pgps_set_pgps_extension(iwise_server_t* extension_server);
void iwise_loc_pgps_inject_extern_data(extern_data_t* extern_data);

#endif
