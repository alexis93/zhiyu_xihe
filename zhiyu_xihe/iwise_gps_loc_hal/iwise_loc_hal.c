/*-----------------------------------------------------------------
 *	iwise_loc_hal.c :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#include <stdlib.h>
#include <utils/Log.h>
#include "iwise_loc_hal.h"
#include "iwise_loc_interface_layer1/iwise_loc_interface.h"

#define NETWORKEDDRIVER 1	/*如果是网络版驱动，则... */

static const void *iwise_eng_get_extension(const char *name);

/*
 *	参数		:	GpsCallbacks* callbacks		I		回调函数的结构体指针
 * 	返回		:	int		0:初始化成功
 * 	描述		:	初始化定位驱动,向驱动注入回调函数
 * 	历史		:
 * 	备注		: 	当在设置界面里开启gps开关时，就调用该函数；该函数调用iwise_init()
 */
static int iwise_eng_init(GpsCallbacks * callbacks)
{
	return iwise_init(callbacks);
}

/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	销毁定位驱动
 * 	历史		:
 * 	备注		: 	当在设置界面里关闭gps开关时，就调用该函数；该函数调用iwise_close()
 */
static void iwise_eng_cleanup()
{
	//TODO 调用 iwise_close();
	return iwise_close();
}

/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	启动定位
 * 	历史		:
 * 	备注		: 	当打开上层定位应用程序时，就会调用该函数；该函数调用iwise_start_fix().
 */
static int iwise_eng_start()
{
	//TODO 调用iwise_start_fix();
	return iwise_start_fix();
}

/*
 *	参数		:	void
 * 	返回		:	int		0:停止定位成功
 * 	描述		:	停止定位
 * 	历史		:
 * 	备注		: 	当关闭上层定位应用程序时，就会调用该函数；该函数调用iwise_stop_fix().
 */
static int iwise_eng_stop()
{
	//TODO 调用iwise_stop_fix();
	return iwise_stop_fix();
}

/*
 *	参数		:	GpsAidingData f			I		Gps辅助数据
 * 	返回		:	void
 * 	描述		:	删除gps辅助定位数据
 * 	历史		:
 * 	备注		: 	该函数的实现最终通过调用iwise_ioctl函数实现
 */
static void iwise_eng_delete_aiding_data(GpsAidingData f)
{
	//TODO 调用iwise_ioctl();
	iwise_ioctl_data_u_type iwise_data;
	iwise_data.ioctl_mask = IWISE_LOC_IOCTL_DELETE_ASSIST_DATA;
	iwise_data.ioctl_data.gps_aiding_data = f;

	iwise_ioctl(&iwise_data);
}

/*
 *	参数		:	GpsPositionMode mode					I		定位模式
 * 				GpsPositionRecurrence recurrence		I		定位系统参考系
 * 				uint32_t min_interval					I		相邻定位的最小时间间隔
 * 				uint32_t preferred_accuracy				I		最佳定位精度
 * 				uint32_t preferred_time					I		最佳定位时间
 * 	返回		:	int		0：设置定位模式成功
 * 	描述		:	设置定位模式
 * 	历史		:
 * 	备注		: 	该函数的最终实现是通过调用iwise_ioctl实现的
 */
static int iwise_eng_set_position_mode(GpsPositionMode mode, GpsPositionRecurrence recurrence,
				       uint32_t min_interval, uint32_t preferred_accuracy,
				       uint32_t preferred_time)
{
	//TODO 调用iwise_ioctl();
    GPS_LOGD("iwise_eng_set_position_mode: %d", mode);
	iwise_ioctl_data_u_type iwise_data;
	iwise_data.ioctl_mask = IWISE_LOC_IOCTL_SET_POSITION_MODE;
	iwise_data.ioctl_data.position_mode = mode;

	return iwise_ioctl(&iwise_data);
}

/*
 *	参数		:	GpsUtcTime time			I		定位时间
 * 				int64_t timeReference	I		定位时间系统参考系
 * 				int uncertainty
 * 	返回		:	int		0：填充时间成功
 * 	描述		:	注入gps定位时间
 * 	历史		:
 * 	备注		: 
 */
static int iwise_eng_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty)
{
	//TODO 调用iwise_ioctl();
/*	iwise_ioctl_data_u_type	iwise_data;
	iwise_data.ioctl_mask = IWISE_LOC_IOCTL_INJECT_TIME;
	iwise_data.ioctl_data.gps_utc_time.time_utc = time;
	iwise_data.ioctl_data.gps_utc_time.time_utc += (int64_t)(android::elapsedRealtime() - timeReference);
	iwise_data.ioctl_data.gps_utc_time.uncertainty = uncertainty;
	
	return iwise_ioctl(&iwise_data);
*/

	return 0;
}

/*
 *	参数		:	double latitude			I		纬度
 * 				double longitude		I		经度
 * 				float accuracy			I		准确度
 * 	返回		:	int		0：填充定位信息成功
 * 	描述		:	注入gps定位信息
 * 	历史		:
 * 	备注		: 
 */
static int iwise_eng_inject_location(double latitude, double longitude, float accuracy)
{
	//TODO 调用iwise_ioctl();
/*  iwise_ioctl_data_u_type	iwise_data;
	iwise_data.ioctl_mask = IWISE_LOC_IOCTL_INJECT_POSITION;
	iwise_data.ioctl_data.location.latitude = latitude;
	iwise_data.ioctl_data.location.longitude = longitude;
	iwise_data.ioctl_data.location.accuracy = accuracy;

	return iwise_ioctl(&iwise_data);
*/
	return 0;
}

/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	pgps初始化
 * 	历史		:
 * 	备注		: 
 */
//Function declarations for sLocEngPGpsInterface
static void iwise_eng_pgps_init()
{
	return;
}

/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	开启网络
 * 	历史		:
 * 	备注		: 
 */
static void iwise_eng_pgps_data_conn_open()
{
	//TODO 调用iwise_ioctl();
	iwise_ioctl_data_u_type iwise_data;
	iwise_data.ioctl_mask = IWISE_LOC_IOCTL_PGPS_SERVER_OPEN;

	iwise_ioctl(&iwise_data);
}

/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	关闭网络
 * 	历史		:
 * 	备注		: 
 */
static void iwise_eng_pgps_data_conn_closed()
{
	//TODO 调用iwise_ioctl();
	iwise_ioctl_data_u_type iwise_data;
	iwise_data.ioctl_mask = IWISE_LOC_IOCTL_PGPS_SERVER_CLOSED;

	iwise_ioctl(&iwise_data);
}

/*
 *	参数		:	const char* hostname		I		服务器IP
 * 				int port					I		服务器端口号
 * 	返回		:	void
 * 	描述		:	设置钟差改正数服务器ip和端口号
 * 	历史		:
 * 	备注		: 
 */
static void iwise_eng_pgps_set_main_server(const char *hostname, int port)
{
	//TODO 调用iwise_ioctl();
	iwise_ioctl_data_u_type iwise_data;
	iwise_data.ioctl_mask = IWISE_LOC_IOCTL_SET_PGPS_MAIN_SERVER;
	memcpy(iwise_data.ioctl_data.pgps_main_server.iwise_server_addr, hostname,
	       strlen(hostname) + 1);
	iwise_data.ioctl_data.pgps_main_server.iwise_server_port = port;

	iwise_ioctl(&iwise_data);
}

/*
 *	参数		:	const char* hostname		I		服务器IP
 * 				int port					I		服务器端口号
 * 	返回		:	void
 * 	描述		:	设置扩展服务器地址
 * 	历史		:
 * 	备注		: 
 */
static void iwise_eng_pgps_extension(const char *hostname, int port)
{
	//TODO 调用iwise_ioctl();
	iwise_ioctl_data_u_type iwise_data;
	iwise_data.ioctl_mask = IWISE_LOC_IOCTL_SET_PGPS_EXTENSION;
	memcpy(iwise_data.ioctl_data.pgps_extension.iwise_server_addr, hostname, strlen(hostname) + 1);
	iwise_data.ioctl_data.pgps_extension.iwise_server_port = port;

	iwise_ioctl(&iwise_data);
}

/*
 *	参数		:	int data_type		I		注入数据类型
 * 				char* data			I		需要注入的数据
 * 				int size			I		需要注入的数据长度
 * 	返回		:	void
 * 	描述		:	注入扩展类型数据
 * 	历史		:
 * 	备注		: 
 */
static void iwise_eng_pgps_inject_extern_data(int data_type, char *data, int size)
{
	//TODO 调用iwise_ioctl();
	iwise_ioctl_data_u_type iwise_data;
	iwise_data.ioctl_mask = IWISE_LOC_IOCTL_SET_PGPS_INJECT_EXTERN_DATA;
	iwise_data.ioctl_data.extern_data.data_type = data_type;
	iwise_data.ioctl_data.extern_data.size = size;
	iwise_data.ioctl_data.extern_data.data = calloc(1, size);
	memcpy(iwise_data.ioctl_data.extern_data.data, data, size);

	iwise_ioctl(&iwise_data);
}

// Defines the GpsInterface in gps.h
static const GpsInterface sLocEngInterface = {
	sizeof(GpsInterface),
	iwise_eng_init,
	iwise_eng_start,
	iwise_eng_stop,
	iwise_eng_cleanup,
	iwise_eng_inject_time,
	iwise_eng_inject_location,
	iwise_eng_delete_aiding_data,
	iwise_eng_set_position_mode,
	iwise_eng_get_extension,
};

//Defines the PGpsInterface in gps.h
static const PGpsInterface sLocEngPGpsInterface = {
	sizeof(PGpsInterface),
	iwise_eng_pgps_init,
	iwise_eng_pgps_data_conn_open,
	iwise_eng_pgps_data_conn_closed,
	iwise_eng_pgps_set_main_server,
	iwise_eng_pgps_extension,
	iwise_eng_pgps_inject_extern_data,

};

/*
 *	参数		:	const char* name		I		gps接口选项
 * 	返回		:	const void*
 * 	描述		:	获取gps扩展的接口
 * 	历史		:
 * 	备注		: 
 */
static const void *iwise_eng_get_extension(const char *name)
{
	if (strcmp(name, PGPS_INTERFACE) == 0) {
		//return PGpsInterface
		return &sLocEngPGpsInterface;
	} else
		return NULL;
}

/*
 *	参数		:	void
 * 	返回		:	const GpsInterface* 
 * 	描述		:	获取gps的接口
 * 	历史		:
 * 	备注		: 
 */
const GpsInterface* get_gps_interface()
{
	GPS_LOGD("====================")
	return &sLocEngInterface;
}

/*
 *	参数		:	struct gps_device_t* dev		I		设备
 * 	返回		:	const GpsInterface*
 * 	描述		:	获得gps设备的接口
 * 	历史		:
 * 	备注		: 	直接调用get_gps_interface()
 */
const GpsInterface *gps__get_gps_interface(struct gps_device_t *dev)
{
	return get_gps_interface();
}

/*
 *	参数		:	const struct hw_module_t* module		I		硬件模块
 * 				char const* name						I		硬件名称
 * 				struct hw_device_t** device				I		硬件设备
 * 	返回		:	int		0:注册GPS设备成功
 * 	描述		:	注册，打开gps硬件设备，模块
 * 	历史		:
 * 	备注		: 
 */
static int open_gps(const struct hw_module_t *module, char const *name, struct hw_device_t **device)
{
	struct gps_device_t *dev = malloc(sizeof(struct gps_device_t));
	memset(dev, 0, sizeof(*dev));

	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = (struct hw_module_t *)module;
	dev->get_gps_interface = gps__get_gps_interface;

	*device = (struct hw_device_t *)dev;
	return 0;
}

static struct hw_module_methods_t gps_module_methods = {
	.open = open_gps
};

/* 这里注意，要把const给去掉，不然会段错误 */
struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = GPS_HARDWARE_MODULE_ID,
	.name = "loc_api GPS Module",
	.author = "Wuhan Zhiyu.",
	.methods = &gps_module_methods,
};
