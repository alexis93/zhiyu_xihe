/*-----------------------------------------------------------------
 *	iwise_loc_type.h
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version :
 * 	history	: 2015-06-04 created by lanzhigang
 *-------------------------------------------------------------------*/

#ifndef _IWISE_LOC_TYPE_H_
#define _IWISE_LOC_TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
//add by cx, 如果是在线程序版本，则需要添加新的callback来返回iwise_message_t，我们在gps.h的基础上修改了iwise_gps.h
#ifdef	MAIN_EXE
#include "iwise_gps.h"
#else
#include <hardware/gps.h>
#endif

#include "rtklib.h"
#include "gps_common.h"

/*定义芯片类型*/
#if defined UBLOX
#define GPS_DEVICE           STRFMT_UBX
#elif defined NOVATEL
#define GPS_DEVICE           STRFMT_OEM4
#elif defined HEXIN
#define GPS_DEVICE			STRFMT_HEXIN
#endif

/*定义驱动定位状态*/
#define 	IWISE_STATUS_NULL    							0X00000000
#define 	IWISE_STATUS_INIT    							0X00000001
#define 	IWISE_STATUS_START  							0X00000002
#define 	IWISE_STATUS_STOP   				 			0X00000003
#define 	IWISE_STATUS_DESTROY 							0X00000004

/*定义网络状态*/
#define IWISE_NET_OPEN       								0X00000001
#define IWISE_NET_CLOSE      								0X00000000


/*下传数据操作码定义*/
#define 	IWISE_LOC_IOCTL_INJECT_TIME      	 			0X00000001
#define 	IWISE_LOC_IOCTL_INJECT_POSITION					0X00000002
#define 	IWISE_LOC_IOCTL_DELETE_ASSIST_DATA				0X00000004
#define 	IWISE_LOC_IOCTL_SET_POSITION_MODE				0X00000008
#define 	IWISE_LOC_IOCTL_PGPS_SERVER_OPEN				0X00000100
#define		IWISE_LOC_IOCTL_PGPS_SERVER_CLOSED				0X00000200
#define		IWISE_LOC_IOCTL_SET_PGPS_MAIN_SERVER			0X00010000
#define 	IWISE_LOC_IOCTL_SET_PGPS_EXTENSION				0X00020000
#define 	IWISE_LOC_IOCTL_SET_PGPS_INJECT_EXTERN_DATA		0X00040000
#define 	IWISE_LOC_IOCTL_RESERVED1						0X00080000

/*回调函数操作码的定义*/
#define 	IWISE_LOC_EVENT_LOCATION_REPORT					0X00000001
#define		IWISE_LOC_EVENT_SATELLITE_REPORT				0X00000002
#define 	IWISE_LOC_EVENT_STATUS_REPORT					0X00000004
#define 	IWISE_LOC_EVENT_NMEA_REPORT						0X00000008
#define 	IWISE_LOC_EVENT_GNSS_OBS_DATA_REPORT			0X00000100
#define		IWISE_LOC_EVENT_MESSAGE_REPORT					0x00000200
#define     IWISE_LOC_EVENT_SOL_REPORT                      0x00000400


#define MAX_RINEX_LENGTH		4096
#define MAX_NMEA_LENGTH			120

/* $GPGST
* GPS Pseudorange Nosie Statistics */
struct iwise_loc_nmea_gst_s_type {
    char utc[10];
    double range_rms;  //RMS value  (m)
    double std_major;  //(m)
    double std_minor;  //(m)
    double hdg;        //(degrees)
    double std_lat;     //(m)
    double std_long;     //(m)
    double std_alt;     //(m)
};
typedef struct iwise_loc_nmea_gst_s_type iwise_loc_nmea_gst_s_type; 

/* $GPGSV,NoMsg,MsgNo,NoSv,{,sv,elv,az,cno}*cs<CR><LF> */
/* 多条GPGSV才能解析出一个完整的iwise_loc_nmea_gsv_s_type 
 * 由解析程序来控制是否输出一个完整的包
 */
struct iwise_loc_nmea_gsv_s_type {
	unsigned char sv_in_view;
	struct sv_info_t {
		unsigned char sv;
		unsigned char elevation;
		unsigned short azimuth;
		unsigned char CNo;
	} sv_info[GPS_MAX_SVS];
};
typedef struct iwise_loc_nmea_gsv_s_type iwise_loc_nmea_gsv_s_type; 

/* 推荐卫星定位输出 GPRMC*/
struct iwise_loc_nmea_rmc_s_type{
	//char utc[10]; //utc time
	GpsUtcTime utc;
	char status; //Navigating status
	double latitude;
	char NS_ind;
	double longitude;
	char EW_ind;  //经度半球 E  /  W	
	double speed; //speed over ground (konts)地面速度
	double bearing; //course over ground (degree)地面航向 以正北为岑烤基准
	char data[6]; //UTC data
	double declination;  //magnetic delination  磁偏角
	char   declination_EW; //magnetic delination E/W indicator  磁偏角方向 E  W
	char   mode;          //模式指示，A=自主定位， D=差分 E=估算 N=数据无效  
};
typedef struct iwise_loc_nmea_rmc_s_type iwise_loc_nmea_rmc_s_type;

/* $GPGGA,hhmmss.ss,Latitude,N,Longitude,E,FS,NoSV,HDOP,msl,m,Altref,m,DiffAge,DiffStation*cs<CR><LF> */
struct iwise_loc_nmea_gga_s_type{
	double latitude;
	double longitude;
	double altitude;
	char NS_ind;
	char EW_ind;	
	GpsUtcTime utc;
	unsigned char sv_used;
	unsigned char fix_status;
	double accuracy;
};
typedef struct iwise_loc_nmea_gga_s_type iwise_loc_nmea_gga_s_type;

struct iwise_loc_nmea_report_s_type{
	unsigned char nmea_type;
	unsigned int nmea_msg_length;
	char nmea_msg[MAX_NMEA_LENGTH];
};
typedef struct iwise_loc_nmea_report_s_type iwise_loc_nmea_report_s_type;

struct iwise_loc_rinex_report_s_type{
	unsigned char rinex_type;
	unsigned int rinex_msg_length;
	char rinex_msg[MAX_RINEX_LENGTH];
};
typedef struct iwise_loc_rinex_report_s_type iwise_loc_rinex_report_s_type;

typedef struct {
	unsigned char rinex_type;
	unsigned int rinex_msg_length;
	char rinex_msg[MAX_RINEX_LENGTH];
} iwise_loc_gnss_obsdata_t;

typedef struct {
	unsigned int length;
	char nmea_sentences[MAX_NMEA_LENGTH];
} iwise_loc_nmea_t;

typedef struct {
	uint64_t event_mask; /* the type of reporting data */
	union {
		GpsLocation location; /*position result corresponding to GpsLocation */
		GpsSvStatus sv_status; /* the infomations of star status corresponding to GpsSvStatus */
		GpsStatus status; /*the infomations of position status  corresponding to GpsStatus */
		iwise_loc_nmea_t nmea; /* the infomations of nmea corresponding to character array and timestamp */
		iwise_loc_gnss_obsdata_t gnss_obsdata; /* raw observation data,this document ues RINEX format to store
		 raw observation data ,corresponding to character array and timestamp */
		iwise_message_t message;
		sol_t  sol;
	} report_data;
} event_payload_u_type;

typedef int32_t (*event_cb_f_type)(uint64_t event_mask,
		const event_payload_u_type *event_payload);

/********************************************************************************/
//服务器ip和端口号Port
typedef struct iwise_server {
	char iwise_server_addr[20];
	int iwise_server_port;
} iwise_server_t;

//精密导航电文服务器和电文类型
typedef struct iwise_precise_server {
	uint32_t message_type;
	iwise_server_t precise_server;
} iwise_precise_server_t;

typedef struct {
	int data_type;
	void *data;
	int size;
} extern_data_t;

//注入扩展数据的还没有添加，后面再添加
typedef struct iwise_ioctl_data_u_type_t {
	uint64_t ioctl_mask; //操作掩码
	struct {
		GpsAidingData gps_aiding_data;
		GpsPositionMode position_mode; 		//设置定位模式
		iwise_server_t pgps_main_server; 	//主服务器
		iwise_server_t pgps_extension; 		//扩展服务器
		extern_data_t extern_data;
	} ioctl_data;
} iwise_ioctl_data_u_type;

/******************************************************************************************/
//TODO
typedef char*	nav_message_t;
typedef int 	iwise_loc_location_size;
typedef char* 	iwise_cert;


#ifdef __cplusplus
}
#endif

#endif
