/*-----------------------------------------------------------------
 *	iwise_loc_interface.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#ifndef _IWISE_LOC_INTERFACE_H_
#define _IWISE_LOC_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>
#include "iwise_loc_base/iwise_loc_type.h"

#define REPORT_STATUS_IDEL 0X00
#define REPORT_STATUS_EVENT 0X01
#define REPORT_STATUS_QUIT  0X02

/*
* GNSS 抽象子层5个接口
* */
int iwise_init(GpsCallbacks * callbacks);
int iwise_start_fix();
int iwise_stop_fix();
void iwise_close();
int iwise_ioctl(iwise_ioctl_data_u_type * ioctl_data);

int iwise_inter_event_cb(uint64_t loc_event,
			 const event_payload_u_type * loc_event_payload);

/*
* GNSS 抽象子层的上下文信息
* 包括 回调函数、数据、
* */
struct iwise_loc_interface_context_s {
	/*
	 * 保存一组回调函数
	 * */
	gps_location_callback location_cb;
	gps_sv_status_callback sv_status_cb;
	gps_status_callback status_cb;
	gps_nmea_callback nmea_cb;
	gps_rinex_callback rinex_cb;
	gps_acquire_wakelock acquire_wakelock_cb;
	gps_release_wakelock release_wakelock_cb;
#ifdef MAIN_EXE
	gps_message_callback message_cb;
#endif

#ifdef SOL_INS
	gps_sol_callback sol_cb;
#endif
	int gnss_data_arrived_flag;
	event_payload_u_type event_payload;

	uint64_t event_type;

	pthread_t report_thread;
	int report_thread_status;
	pthread_mutex_t data_lock;
	pthread_cond_t data_cond;
};
typedef struct iwise_loc_interface_context_s iwise_loc_interface_context_t;

#ifdef __cplusplus
}
#endif
#endif
