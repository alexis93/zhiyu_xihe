#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "iwise_loc_base/iwise_loc_base.h"

static const GpsInterface* mGpsInterface = NULL;
static const PGpsInterface* mPGpsInterface =NULL;

#define WAKE_LOCK_NAME  "GPS"

/** Callback with location information.
 *  Can only be called from a thread created by create_thread_cb.
 */
void location_callback(GpsLocation* location)
{
	GPS_LOGI("[Get Location][LAT] %f [LON] %f [ALT] %f", 
			location->latitude, location->longitude, location->altitude);
	return;
}

void message_callback(iwise_message_t* msg)
{
	GPS_LOGI("[spp_std]%lf [ppp_std]%lf [corr_time]%ld [gps_time]%ld [pos_freq]%d", 
						msg->spp_std, msg->ppp_std, msg->corr_time, msg->gps_time, msg->pos_freq);
	GPS_LOGI("[INFO_MSG] %s", msg->info_msg);
	return;
}

 /*add this callback to provide gps[7] data for  ins. */
void sol_callback(iwise_sol_t* sol)
{
	GPS_LOGI("[sol.time]%ld [sol.rr[0]]%lf [sol.rr[1]]%lf [sol.rr[2]]%lf ", 
						sol->time.time, sol->rr[0],sol->rr[1],sol->rr[2]);
	GPS_LOGI("sol_callback");
	
	return;
}

/** Callback with status information.
 *  Can only be called from a thread created by create_thread_cb.
 */
void status_callback(GpsStatus* status)
{
	return;
}

/** Callback with SV status information.
 *  Can only be called from a thread created by create_thread_cb.
 */
void sv_status_callback(GpsSvStatus* sv_info)
{
	return;
}

/** Callback for reporting NMEA sentences.
 *  Can only be called from a thread created by create_thread_cb.
 */
void nmea_callback(GpsUtcTime timestamp, const char* nmea, int length)
{
	return;
}
/*
 * 添加
 * */
void rinex_callback(GpsUtcTime timestamp, const char* rinex, int length)
{
	return;
}
/** Callback to inform framework of the GPS engine's capabilities.
 *  Capability parameter is a bit field of GPS_CAPABILITY_* flags.
 */
void set_capabilities_callback(uint32_t capabilities)
{
	return;
}

/** Callback utility for acquiring the GPS wakelock.
 *  This can be used to prevent the CPU from suspending while handling GPS events.
 */
void acquire_wakelock_callback()
{
	return;
}

/** Callback utility for releasing the GPS wakelock. */
void release_wakelock_callback()
{
	return;
}

/** Callback for requesting NTP time */
void request_utc_time_callback()
{
	return;
}


/** Callback for creating a thread that can call into the Java framework code.
 *  This must be used to create any threads that report events up to the framework.
 */

pthread_t create_thread_callback(const char* name, void (*start)(void *), void* arg)
{
	pthread_t report_thread;
	pthread_create(&report_thread, NULL, start, arg);
	return report_thread;
}

GpsCallbacks mGpsCallbacks = {
	sizeof(GpsCallbacks),
	location_callback,
	status_callback,
	sv_status_callback,
	nmea_callback,
	rinex_callback,
	set_capabilities_callback,
	acquire_wakelock_callback,
	release_wakelock_callback,
	create_thread_callback,
	request_utc_time_callback,
	message_callback,
	sol_callback,
};

static int get_interface()
{
	/* 获得GpsInterface的接口 */
	mGpsInterface = gps__get_gps_interface();
	if (mGpsInterface) {
		//get the pgps interface
		mPGpsInterface = 
			(const PGpsInterface*)mGpsInterface->get_extension(PGPS_INTERFACE);
	}

	return 0;
}

static int gps_init()
{
	/* 首先获得GpsInterface 和 PGpsInterface */
	get_interface();
	
	if(!mGpsInterface){
		GPS_LOGE("NULL");
	}
	
	// 调用两个接口的初始化函数
	if (!mGpsInterface || mGpsInterface->init(&mGpsCallbacks) != 0)
		return -1;

	if(mPGpsInterface)
		mPGpsInterface->init();
	
	GPS_LOGD("GpsInterface->set_position_mode");
	// 设置定位模式
	mGpsInterface->set_position_mode(GPS_POSITION_MODE_XIHE, 0, 0, 0, 0);
	//mGpsInterface->set_position_mode(GPS_POSITION_MODE_STANDALONE, 0, 0, 0, 0);
	GPS_LOGD("PGpsInterface->set_main_server");
	// 设置改正数服务器
	mPGpsInterface->set_main_server("58.49.58.148", 8001);
	
	return 0;
}

static int gps_start()
{
	if (mGpsInterface)
		return mGpsInterface->start();
	else
		return -1;
}

static int gps_stop()
{
	if (mGpsInterface)
		return mGpsInterface->stop() == 0;
	else
		return false;
}

static void gps_cleanup()
{
	if (mGpsInterface)
		mGpsInterface->cleanup();
}

void exit_handler(int s){
	GPS_LOGD("ready to exit！！");
	gps_stop();
	gps_cleanup();
	GPS_LOGD("cleanup, exit！！");
}

int main(int argc, char **argv)
{
	GPS_LOGD("===========START================");
	int ret;
	if(gps_init() == -1){
		GPS_LOGE("gps_init failed！！");
		return -1;
	}
	
	if(gps_start() == -1){
		GPS_LOGE("gps_start failed！！");
		return -1;
	}
	
	struct sigaction sigHandler;
	sigHandler.sa_handler = exit_handler;
	sigemptyset(&sigHandler.sa_mask);
	sigHandler.sa_flags = 0;
	sigaction(SIGINT, &sigHandler, NULL);
	pause();
	
	return 0;
}
