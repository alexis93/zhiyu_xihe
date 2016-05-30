/*-----------------------------------------------------------------
 *	gps_common.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version :
 * 	history	: 2015-05-26 created by chenxi
 *-------------------------------------------------------------------*/
 
#ifndef _IWISE_LOC_LOG_H_
#define _IWISE_LOC_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif
#ifdef ANDROID_HAL
#include <utils/Log.h>		/* For the LOG macro */
#else
#include <stdio.h>
#endif

/* --------------------------------------------------------------------- */

/*
 * 编译时如果使用了 NDEBUG 宏可以控制 GPS_LOGV 是否输出
 * GPS_LOG_LEVEL(输出条件):
 * 			VERBOSE	<= 2
 * 			DEBUG	<= 3
 * 			INFO	<= 4
 * 			WARN	<= 5
 * 			ERROR	<= 6
 */
#ifdef GPSNDEBUG
#define GPS_LOG_LEVEL	6
#else
#define GPS_LOG_LEVEL	2		/* 默认日志等级为3 */
#endif

#ifdef ANDROID_HAL
/* Based off the log priorities in android
   /system/core/include/android/log.h */
#define GPS_LOG_UNKNOWN		(0)
#define GPS_LOG_DEFAULT		(1)
#define GPS_LOG_VERBOSE		(2)
#define GPS_LOG_DEBUG		(3)
#define GPS_LOG_INFO		(4)
#define GPS_LOG_WARN		(5)
#define GPS_LOG_ERROR		(6)
#define GPS_LOG_SILENT		(8)
#else
#define GPS_LOG_UNKNOWN GPS_LOG_VERBOSE
#define GPS_LOG_DEFAULT "[*]"
#define GPS_LOG_VERBOSE "[V]"
#define GPS_LOG_DEBUG   "[D]"
#define GPS_LOG_INFO    "[I]"
#define GPS_LOG_WARN    "[W]"
#define GPS_LOG_ERROR   "[E]"
#define GPS_LOG_SILENT  GPS_LOG_VERBOSE
#endif

#ifndef GPS_LOG_TAG
#ifdef ANDROID_HAL
#define GPS_LOG_TAG "iwise_loc_hal"
#else
#define GPS_LOG_TAG " [%04d] [%-20.20s] "
#endif
#endif

#ifndef ANDROID_HAL
#define GPS_LOG_END "\n"
#endif

#ifndef GPS_LOGV
#define GPS_LOGV(fmt, ...){                                            \
	if(GPS_LOG_LEVEL <= 2){                                            \
		GPS_LOG(LOG_VERBOSE, GPS_LOG_TAG, fmt, ##__VA_ARGS__);         \
	}                                                                  \
}
#endif

#ifndef GPS_LOGD
#define GPS_LOGD(fmt, ...){                                            \
	if(GPS_LOG_LEVEL <= 3){                                            \
		GPS_LOG(LOG_DEBUG, GPS_LOG_TAG, fmt, ##__VA_ARGS__);           \
	}                                                                  \
}
#endif

#ifndef GPS_LOGI
#define GPS_LOGI(fmt, ...){                                            \
	if(GPS_LOG_LEVEL <= 4){                                            \
		GPS_LOG(LOG_INFO, GPS_LOG_TAG, fmt, ##__VA_ARGS__);	           \
	}                                                                  \
}
#endif

#ifndef GPS_LOGW
#define GPS_LOGW(fmt, ...) {                                           \
	if(GPS_LOG_LEVEL <= 5){                                            \
		GPS_LOG(LOG_WARN, GPS_LOG_TAG, fmt, ##__VA_ARGS__);	           \
	}                                                                  \
}
#endif

#ifndef GPS_LOGE
#define GPS_LOGE(fmt, ...) {                                           \
	if(GPS_LOG_LEVEL <= 6){                                            \
		GPS_LOG(LOG_ERROR, GPS_LOG_TAG, fmt, ##__VA_ARGS__);           \
	}                                                                  \
}
#endif

#ifndef GPS_LOG
#define GPS_LOG(priority, tag, fmt, ...)		\
	GPS_LOG_PRI(priority, tag, fmt, ##__VA_ARGS__)
#endif

//LOGD(priority, tag, fmt, ##__VA_ARGS__)

#ifndef GPS_LOG_PRI
#ifdef ANDROID_HAL
#define GPS_LOG_PRI(priority, tag, fmt, ...) \
	ALOG(priority, tag, fmt, ##__VA_ARGS__)
#else
#define GPS_LOG_PRI(priority, tag, fmt, ...) \
	printf(GPS_##priority tag fmt GPS_LOG_END, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
