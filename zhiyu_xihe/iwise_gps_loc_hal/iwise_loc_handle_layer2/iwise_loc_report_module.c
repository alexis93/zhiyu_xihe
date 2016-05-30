/*-----------------------------------------------------------------
 *	iwise_loc_report_module.c :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#include <time.h>

#include "iwise_loc_handle.h"
#include "iwise_loc_base/iwise_output.h"
#include "iwise_loc_base/iwise_loc_base.h"
#include "iwise_loc_base/iwise_loc_type.h"
#include "iwise_loc_base/Parserlib/rtklib.h"
#include "iwise_net_rtk_module.h"
/* 进制转换-------------------------------*/
void degtodms(double deg, double *dms)
{
	double sgn=1.0;
	if(deg<0.0){
		deg=-deg;
		sgn=-1.0;
	}
	dms[0] = floor(deg);
	dms[1] = floor((deg-dms[0])*60.0);
	dms[2] = ((deg-dms[0])*60.0-dms[1])*100;
	dms[0]*=sgn;
}

/* sol转换成GpsLocation-------------------------*/
static void sol2Gpslocation(iwise_sol_t *sol, event_payload_u_type *payload)
{
    double pos[3]={0}, Qe[9]={0};
	double dms1[3]={0}, dms2[3]={0};
	ecef2pos(sol->rr, pos);
	covenu(pos, sol->qr, Qe);
	degtodms(pos[0]*R2D, dms1);
	degtodms(pos[1]*R2D, dms2);
    
    payload->event_mask |= IWISE_LOC_EVENT_LOCATION_REPORT;
    GpsLocation  *location = &(payload->report_data.location);
    location->size = sizeof(GpsLocation);

    location->latitude = pos[0]*R2D;
	location->longitude = pos[1]*R2D;
    location->flags |= GPS_LOCATION_HAS_LAT_LONG;

    location->altitude = pos[2];
    location->flags |= GPS_LOCATION_HAS_ALTITUDE;
    GPS_LOGD("lat: %lf lon: %lf alt: %lf", location->latitude, location->longitude, location->altitude);
    if((location->latitude == 0.0) && (location->longitude == -90.0))
    {
		GPS_LOGD("error pnt latitude = %lf, longitude = %lf", location->latitude, location->longitude);
	}
    
    location->speed = 2;
    location->flags |= GPS_LOCATION_HAS_SPEED;

    location->bearing = 30;
    location->flags |= GPS_LOCATION_HAS_BEARING;

    location->accuracy = 5;
    location->flags |= GPS_LOCATION_HAS_ACCURACY;

    location->timestamp = sol->time.time;
}    
/* sol转换成ins_gps--------------------*/
static void sol2ins_gps(iwise_sol_t *sol_l, event_payload_u_type *payload)
{
    double pos[3]={0};
	
	ecef2pos(sol_l->rr, pos);
	    
    payload->event_mask |= IWISE_LOC_EVENT_SOL_REPORT;
    iwise_sol_t  *sol_ins = &(payload->report_data.sol);
   
    sol_ins->time.time = sol_l->time.time;
    sol_ins->time.sec = sol_l->time.sec;   //add by zhuyong 2015.12.11 for gps time
    sol_ins->rr[0] = pos[0]*R2D;
    sol_ins->rr[1] = pos[1]*R2D;
    sol_ins->rr[2] = pos[2];
    sol_ins->rr[3] =sqrt(sol_l->qr[0]);
    sol_ins->rr[4] =sqrt(sol_l->qr[1]);
    sol_ins->rr[5] =sqrt(sol_l->qr[2]);
    
  
}    


/* 经纬度转换 60进制转换成十进制-------------------------*/
static double convert_from_hhmm(double val)
{
    int degrees = (int)(floor(val) / 100);
    double minute = val - degrees * 100;
    double dcoord = degrees + minute / 60.0;
    return dcoord;
}

/* nmea 转换成GpsLocation---------------------------------------*/
static void nmea2GpsLocation(iwise_nmea_parser_t *nmea, event_payload_u_type *payload)
{
    payload->event_mask |= IWISE_LOC_EVENT_LOCATION_REPORT;

    GpsLocation* location = &(payload->report_data.location);
    location->latitude = convert_from_hhmm(nmea->nmea_gga_info.latitude);
    location->longitude = convert_from_hhmm(nmea->nmea_gga_info.longitude);
    location->flags |= GPS_LOCATION_HAS_LAT_LONG;

    location->altitude = nmea->nmea_gga_info.altitude;
    location->flags |= GPS_LOCATION_HAS_ALTITUDE;

	GPS_LOGD("gga lat: %lf lon: %lf alt: %lf", location->latitude, location->longitude, location->altitude);

    location->speed = 1;
    location->flags |= GPS_LOCATION_HAS_SPEED;

    location->bearing = 1;
    location->flags |= GPS_LOCATION_HAS_BEARING;

    location->accuracy = nmea->nmea_gga_info.accuracy;
    location->flags |= GPS_LOCATION_HAS_ACCURACY;
    //GPS_LOGD("gga accuracy	: %lf", location->accuracy);

    struct timeval tv;
    gettimeofday(&tv, (struct timezone *)NULL);
    long long now = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
    location->timestamp = now;
}



/* nmea_parse 转换成payload_nmea---------------------------------------*/
static void nmeaparse2nmea(iwise_nmea_parser_t *nmea, event_payload_u_type *payload)
{
    payload->event_mask |= IWISE_LOC_EVENT_NMEA_REPORT;
    iwise_loc_nmea_t* nmea_cb=&(payload->report_data.nmea);
    nmea_cb->length = nmea->nmea_report_info.nmea_msg_length;
    memcpy(nmea_cb->nmea_sentences,nmea->nmea_report_info.nmea_msg,sizeof(nmea->nmea_report_info));
        
}

/*
 *	参数		:	iwise_sol_t* sol		I		sol(定位信息)
 * 	返回		:	int		0:上报成功
 * 	描述		:	向上层报告sol
 * 	历史		:
 * 	备注		: 
 */
int iwise_loc_report_sol(iwise_sol_t* sol) 
{
	event_payload_u_type payload;
	sol2Gpslocation(sol, &payload);
	// 调用回调函数
	context->event_callback(IWISE_LOC_EVENT_LOCATION_REPORT, &payload);
	memset(&payload, 0, sizeof(event_payload_u_type));
	GPS_LOGD("position success and call callback  to report a location up!")
	return 0;
}


/*
 *	参数		:	iwise_nmea_parser_t* nmea_parser		I		nmea解析的数据
 * 	返回		:	int		0：上报成功
 * 	描述		:	上报nmea
 * 	历史		:
 * 	备注		: 
 */
 
int iwise_loc_report_nmea(iwise_nmea_parser_t* nmea_parser){
	
	//GPS_LOGI("iwise_loc_report_nmea")
	event_payload_u_type payload;
    nmeaparse2nmea(nmea_parser,&payload);
    set_pos_netrtk(&(payload.report_data.nmea.nmea_sentences),payload.report_data.nmea.length);
	context->event_callback(IWISE_LOC_EVENT_NMEA_REPORT, &payload);
	return 0;
}

/*
int iwise_loc_report_nmea(iwise_nmea_parser_t* nmea_parser){
	event_payload_u_type payload;
	nmea2GpsLocation(nmea_parser,&payload);
	context->event_callback(IWISE_LOC_EVENT_LOCATION_REPORT, &payload);
	return 0;
}
* */


/*
 *	参数		:	iiwise_message_t* msg		I		定位库消息反馈
 * 	返回		:	int		0：上报成功
 * 	描述		:	上报定位库错误消息
 * 	历史		:
 * 	备注		: 
 */
 
int iwise_loc_report_message(iwise_message_t* msg){
	event_payload_u_type payload;
	payload.event_mask = IWISE_LOC_EVENT_MESSAGE_REPORT;
	memcpy(&payload.report_data.message,msg, sizeof(iwise_message_t));
	context->event_callback(IWISE_LOC_EVENT_MESSAGE_REPORT, &payload);
	return 0;
}

/*
 *	参数		:	iwise_sol_t* sol		I		解算后结果（sol）
 * 	返回		:	int		0：上报成功
 * 	描述		:	上报sol
 * 	历史		:
 * 	备注		: 
 */
 
int iwise_loc_report_sol_ins(iwise_sol_t* sol){
	event_payload_u_type payload;
	sol2ins_gps(sol,&payload);
	//memcpy(&payload.report_data.sol,sol, sizeof(iwise_sol_t));
	context->event_callback(IWISE_LOC_EVENT_SOL_REPORT, &payload);
	memset(&payload, 0, sizeof(event_payload_u_type));
	return 0;
}
