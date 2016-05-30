/*-----------------------------------------------------------------
 *	iwise_loc_interface.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-12	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#include <cutils/properties.h>
#include <cutils/sched_policy.h>

#include "iwise_loc_interface.h"
#include "iwise_loc_base/iwise_loc_base.h"
#include "iwise_loc_handle_layer2/iwise_loc_handle.h"

static iwise_loc_interface_context_t inter_context;

static void iwise_interface_process_loc_event(uint64_t event_type,
					      event_payload_u_type * loc_event_payload);

static void iwise_inter_report_position(GpsLocation * location_report_ptr);

static void iwise_inter_report_sv(GpsSvStatus * gnss_report_ptr);

static void iwise_inter_report_status(GpsStatus * status_report_ptr);

static void iwise_inter_report_nmea(iwise_loc_nmea_t * nmea_report_ptr);

static void iwise_inter_report_message(iwise_message_t * message_report_ptr);

static void iwise_inter_report_sol(iwise_sol_t * sol);

static void iwise_inter_report_iwise_position(iwise_loc_gnss_obsdata_t * iwise_location_report_ptr);

/*
 *	参数		:	void* arg		I		线程参数
 * 	返回		:	void
 * 	描述		:	报告线程
 * 	历史		:
 * 	备注		: 
 */
static void* report_thread(void *arg)
{
	uint64_t event_type;
	event_payload_u_type event_data;

	set_sched_policy(gettid(), SP_FOREGROUND);

	while (1) {
		pthread_mutex_lock(&inter_context.data_lock);
		if (inter_context.report_thread_status == REPORT_STATUS_IDEL) {
			inter_context.release_wakelock_cb();
			pthread_cond_wait(&inter_context.data_cond, &inter_context.data_lock);
			inter_context.acquire_wakelock_cb();
		}
		if (inter_context.report_thread_status == REPORT_STATUS_QUIT) {
			pthread_mutex_unlock(&inter_context.data_lock);
			break;
		}
		inter_context.report_thread_status = REPORT_STATUS_IDEL;
		event_type = inter_context.event_type;
		//lzg todo
		memcpy(&event_data, &inter_context.event_payload, sizeof(event_data));
		pthread_mutex_unlock(&inter_context.data_lock);
		
		//GPS_LOGD("report_hread report...");
		iwise_interface_process_loc_event(event_type, &event_data);
	}
	return 0;
}

/*---------------------回调事件处理-------------------------------*----------*/
/*
 *	参数		:	uint64_t event_type						I		回调事件类型
 * 				event_payload_u_type* loc_event_payload	I		回调事件结构体
 * 	返回		:	void
 * 	描述		:	向android上层返回需要返回的结构体
 * 	历史		:	
 * 	备注		: do_send_NMEA
 * 			
 */
static void iwise_interface_process_loc_event(uint64_t event_type,
					      event_payload_u_type * loc_event_payload)
{
	if (event_type & IWISE_LOC_EVENT_GNSS_OBS_DATA_REPORT) {
		iwise_inter_report_iwise_position(&(loc_event_payload->report_data.gnss_obsdata));
	}
	if (event_type & IWISE_LOC_EVENT_LOCATION_REPORT) {
		iwise_inter_report_position(&(loc_event_payload->report_data.location));
	}

	if (event_type & IWISE_LOC_EVENT_SATELLITE_REPORT) {
		iwise_inter_report_sv(&(loc_event_payload->report_data.sv_status));
	}

	if (event_type & IWISE_LOC_EVENT_STATUS_REPORT) {
		iwise_inter_report_status(&(loc_event_payload->report_data.status));
	}

	if (event_type & IWISE_LOC_EVENT_NMEA_REPORT) {
		iwise_inter_report_nmea(&(loc_event_payload->report_data.nmea));
	}
#ifdef MAIN_EXE	
	if (event_type & IWISE_LOC_EVENT_MESSAGE_REPORT) {
		iwise_inter_report_message(&(loc_event_payload->report_data.message));
	}
#endif

#ifdef SOL_INS	
	if (event_type & IWISE_LOC_EVENT_SOL_REPORT) {
		iwise_inter_report_sol(&(loc_event_payload->report_data.sol));
	}
#endif
}

/*
 *	参数		:	GpsLocation *location_report_ptr		I		定位信息
 * 	返回		:	void
 * 	描述		:	向上层返回定位信息
 * 	历史		:
 * 	备注		:
 */
static void iwise_inter_report_position(GpsLocation * location_report_ptr)
{
	if (inter_context.location_cb != NULL) {
		inter_context.location_cb(location_report_ptr);
	}
}

#ifdef MAIN_EXE
/*
 *	参数		:	iwise_message_t * message				I		定位返回信息
 * 	返回		:	void
 * 	描述		:	向上层返回定位返回信息
 * 	历史		:
 * 	备注		:
 */
static void iwise_inter_report_message(iwise_message_t * message)
{
	if (inter_context.message_cb != NULL) {
		inter_context.message_cb(message);
	}
}
#endif


#ifdef SOL_INS
/*
 *	参数		:	sol_t * sol				I		定位解算结果返回
 * 	返回		:	void
 * 	描述		:	向上层返回定位解算结果
 * 	历史		:
 * 	备注		:
 */
static void iwise_inter_report_sol(iwise_sol_t * sol)
{
	if (inter_context.sol_cb != NULL) {
		inter_context.sol_cb(sol);
	}
}
#endif

/*
 *	参数		:	GpsSvStatus* gnss_report_ptr			I		卫星状态信息
 * 	返回		:	void
 * 	描述		:	向上层返回卫星状态信息
 * 	历史		:
 * 	备注		:
 */
static void iwise_inter_report_sv(GpsSvStatus * gnss_report_ptr)
{
	if ((inter_context.sv_status_cb != NULL) && (gnss_report_ptr != NULL)) {
		inter_context.sv_status_cb(gnss_report_ptr);
	}
}

/*
 *	参数		:	GpsStatus* status_report_ptr			I		状态信息
 * 	返回		:	void
 * 	描述		:	向上层返回卫星状态信息
 * 	历史		:
 * 	备注		:
 */
static void iwise_inter_report_status(GpsStatus * status_report_ptr)
{
	if (inter_context.status_cb != NULL) {
		inter_context.status_cb(status_report_ptr);
	}
}

/*
 *	参数		:	iwise_loc_nmea_t* nmea_report_ptr		I		nmea信息
 * 	返		:	void
 * 	描述		:	向上层返回卫星状态信息
 * 	历史		:report_hread report
 * 	备注		:
 */
static void iwise_inter_report_nmea(iwise_loc_nmea_t * nmea_report_ptr)
{
	if (inter_context.nmea_cb != NULL) {
		struct timeval tv;
		gettimeofday(&tv, (struct timezone *)NULL);
		long long now = tv.tv_sec * 1000LL + tv.tv_usec / 1000;

		inter_context.nmea_cb(now, nmea_report_ptr->nmea_sentences,
				      nmea_report_ptr->length);
	}
}

/*
 *	参数		:	iwise_loc_gnss_obsdata_t* iwise_location_report_ptr		I		rinex信息
 * 	返回		:	void
 * 	描述		:	用rinex格式返回卫星观测值的信息
 * 	历史		:
 * 	备注		:	添加的,没有实现.
 */
static void iwise_inter_report_iwise_position(iwise_loc_gnss_obsdata_t * iwise_location_report_ptr)
{

}

/*
 *	参数		:	uint64_t loc_event								I		回调函数事件码
 * 				const event_payload_u_type * loc_event_payload	I		回调函数的数据结构体
 * 	返回		:	int32_t		0：回调成功
 * 	描述		:	定位模块中调用，向上层HAL返回GpsLocation, GpsStatus等
 * 	历史		:
 * 	备注		: 
 */
int32_t iwise_inter_event_cb(uint64_t loc_event, const event_payload_u_type * loc_event_payload)
{

	//GPS_LOGD("iwise_inter_event_cb running...");
	pthread_mutex_lock(&inter_context.data_lock);
	inter_context.event_type = loc_event;
	memcpy(&inter_context.event_payload, loc_event_payload, sizeof(event_payload_u_type));
	inter_context.report_thread_status |= REPORT_STATUS_EVENT;
	pthread_cond_signal(&inter_context.data_cond);
	pthread_mutex_unlock(&inter_context.data_lock);
	//GPS_LOGD("iwise_inter_event_cb ended...");
	return 0;
}

/*
 *	参数		:	GpsCallbacks* callbacks			I		回调函数
 * 	返回		:	int		0：初始化成功；	-1：初始化失败
 * 	描述		:	驱动初始化
 * 	历史		:
 * 	备注		: 
 */
int iwise_init(GpsCallbacks * callbacks)
{
	/*先初始化报告线程，调用真正的初始化函数 */
	GPS_LOGD("enter iwise_init( gps init )");
	if (inter_context.report_thread) {
		return -1;
	}
	callbacks->set_capabilities_cb(GPS_CAPABILITY_SCHEDULING | GPS_CAPABILITY_MSA
				       | GPS_CAPABILITY_MSB);

#ifdef SOL_INS
	inter_context.sol_cb = callbacks->sol_cb;
#endif			       
				       
#ifdef MAIN_EXE
	inter_context.message_cb = callbacks->message_cb;
#endif
	inter_context.location_cb = callbacks->location_cb;
	inter_context.sv_status_cb = callbacks->sv_status_cb;
	inter_context.status_cb = callbacks->status_cb;
	inter_context.nmea_cb = callbacks->nmea_cb;
	inter_context.rinex_cb = callbacks->rinex_cb;

	inter_context.acquire_wakelock_cb = callbacks->acquire_wakelock_cb;
	inter_context.release_wakelock_cb = callbacks->release_wakelock_cb;

	iwise_loc_init(iwise_inter_event_cb);

	pthread_mutex_init(&(inter_context.data_lock), NULL);
	pthread_cond_init(&(inter_context.data_cond), NULL);
	inter_context.report_thread_status = 0;
	inter_context.report_thread = callbacks->create_thread_cb("iwise_api", report_thread, NULL);
	GPS_LOGD("iwise_init (gps init) over\n");
	return 0;
}

/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	定位开始的函数
 * 	历史		:
 * 	备注		: 
 */
int iwise_start_fix()
{
	return iwise_loc_start();
}

/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	定位停止函数
 * 	历史		:	
 * 	备注		: 	
 */
int iwise_stop_fix()
{
	return iwise_loc_stop();
}

/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	定位销毁函数
 * 	历史		:
 * 	备注		: 
 */
void iwise_close()
{
	iwise_loc_destroy();

	if (inter_context.report_thread != NULL) {
		pthread_mutex_lock(&inter_context.data_lock);
		inter_context.acquire_wakelock_cb();
		inter_context.report_thread_status = 1;
		pthread_cond_signal(&inter_context.data_cond);
		pthread_mutex_unlock(&inter_context.data_lock);

		void *joinvalue;
		pthread_join(inter_context.report_thread, &joinvalue);
		inter_context.report_thread = NULL;
	}

	pthread_mutex_destroy(&inter_context.data_lock);
	pthread_cond_destroy(&inter_context.data_cond);
}

/*
 *	参数		:	iwise_ioctl_data_u_type *ioctl_data			I		向下注入数据的结构体
 * 	返回		:	int		0:调用成功
 * 	描述		:	上层接口通过该函数向下层注入其他数据类型
 * 	历史		:
 * 	备注		: 
 */
int iwise_ioctl(iwise_ioctl_data_u_type * ioctl_data)
{
	GPS_LOGD("iwise_ioctl runing");
	if (ioctl_data->ioctl_mask & IWISE_LOC_IOCTL_INJECT_TIME) {
		iwise_loc_inject_time();
	}

	if (ioctl_data->ioctl_mask & IWISE_LOC_IOCTL_INJECT_POSITION) {
		iwise_loc_inject_location();
	}

	if (ioctl_data->ioctl_mask & IWISE_LOC_IOCTL_DELETE_ASSIST_DATA) {
		iwise_loc_delete_aiding_data();
	}

	if (ioctl_data->ioctl_mask & IWISE_LOC_IOCTL_SET_POSITION_MODE) {
		GPS_LOGD("set gps position mode: %d", ioctl_data->ioctl_data.position_mode);
		iwise_loc_set_position_mode(ioctl_data->ioctl_data.position_mode);
		GPS_LOGD("set gps position mode: %d", ioctl_data->ioctl_data.position_mode);
	}

	if (ioctl_data->ioctl_mask & IWISE_LOC_IOCTL_PGPS_SERVER_OPEN) {
		iwise_loc_pgps_data_conn_open();
	}

	if (ioctl_data->ioctl_mask & IWISE_LOC_IOCTL_PGPS_SERVER_CLOSED) {
		iwise_loc_pgps_data_conn_closed();
	}

	if (ioctl_data->ioctl_mask & IWISE_LOC_IOCTL_SET_PGPS_MAIN_SERVER) {
		iwise_loc_pgps_set_pgps_main_server(&(ioctl_data->ioctl_data.pgps_main_server));
	}

	if (ioctl_data->ioctl_mask & IWISE_LOC_IOCTL_SET_PGPS_EXTENSION) {
		iwise_loc_pgps_set_pgps_extension(&(ioctl_data->ioctl_data.pgps_extension));
	}

	if (ioctl_data->ioctl_mask & IWISE_LOC_IOCTL_SET_PGPS_INJECT_EXTERN_DATA) {
		iwise_loc_pgps_inject_extern_data(&(ioctl_data->ioctl_data.extern_data));
	}
	return 0;
}
